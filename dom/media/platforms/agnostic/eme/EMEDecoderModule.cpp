/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EMEDecoderModule.h"
#include "EMEAudioDecoder.h"
#include "EMEVideoDecoder.h"
#include "MediaDataDecoderProxy.h"
#include "mozIGeckoMediaPluginService.h"
#include "mozilla/CDMProxy.h"
#include "mozilla/unused.h"
#include "nsServiceManagerUtils.h"
#include "MediaInfo.h"
#include "nsClassHashtable.h"
#include "GMPDecoderModule.h"

namespace mozilla {

typedef MozPromiseRequestHolder<CDMProxy::DecryptPromise> DecryptPromiseRequestHolder;

class EMEDecryptor : public MediaDataDecoder {

public:

  EMEDecryptor(MediaDataDecoder* aDecoder,
               MediaDataDecoderCallback* aCallback,
               CDMProxy* aProxy,
               TaskQueue* aDecodeTaskQueue)
    : mDecoder(aDecoder)
    , mCallback(aCallback)
    , mTaskQueue(aDecodeTaskQueue)
    , mProxy(aProxy)
    , mSamplesWaitingForKey(new SamplesWaitingForKey(this, mTaskQueue, mProxy))
    , mIsShutdown(false)
  {
  }

  RefPtr<InitPromise> Init() override {
    MOZ_ASSERT(!mIsShutdown);
    return mDecoder->Init();
  }

  nsresult Input(MediaRawData* aSample) override {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    MOZ_ASSERT(!mIsShutdown);
    if (mSamplesWaitingForKey->WaitIfKeyNotUsable(aSample)) {
      return NS_OK;
    }

    nsAutoPtr<MediaRawDataWriter> writer(aSample->CreateWriter());
    mProxy->GetSessionIdsForKeyId(aSample->mCrypto.mKeyId,
                                  writer->mCrypto.mSessionIds);

    mDecrypts.Put(aSample, new DecryptPromiseRequestHolder());
    mDecrypts.Get(aSample)->Begin(mProxy->Decrypt(aSample)->Then(
      mTaskQueue, __func__, this,
      &EMEDecryptor::Decrypted,
      &EMEDecryptor::Decrypted));
    return NS_OK;
  }

  void Decrypted(const DecryptResult& aDecrypted) {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    MOZ_ASSERT(aDecrypted.mSample);

    nsAutoPtr<DecryptPromiseRequestHolder> holder;
    mDecrypts.RemoveAndForget(aDecrypted.mSample, holder);
    if (holder) {
      holder->Complete();
    } else {
      // Decryption is not in the list of decrypt operations waiting
      // for a result. It must have been flushed or drained. Ignore result.
      return;
    }

    if (mIsShutdown) {
      NS_WARNING("EME decrypted sample arrived after shutdown");
      return;
    }

    if (aDecrypted.mStatus == GMPNoKeyErr) {
      // Key became unusable after we sent the sample to CDM to decrypt.
      // Call Input() again, so that the sample is enqueued for decryption
      // if the key becomes usable again.
      Input(aDecrypted.mSample);
    } else if (GMP_FAILED(aDecrypted.mStatus)) {
      if (mCallback) {
        mCallback->Error();
      }
    } else {
      MOZ_ASSERT(!mIsShutdown);
      nsresult rv = mDecoder->Input(aDecrypted.mSample);
      Unused << NS_WARN_IF(NS_FAILED(rv));
    }
  }

  nsresult Flush() override {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    MOZ_ASSERT(!mIsShutdown);
    for (auto iter = mDecrypts.Iter(); !iter.Done(); iter.Next()) {
      nsAutoPtr<DecryptPromiseRequestHolder>& holder = iter.Data();
      holder->DisconnectIfExists();
      iter.Remove();
    }
    nsresult rv = mDecoder->Flush();
    Unused << NS_WARN_IF(NS_FAILED(rv));
    mSamplesWaitingForKey->Flush();
    return rv;
  }

  nsresult Drain() override {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    MOZ_ASSERT(!mIsShutdown);
    for (auto iter = mDecrypts.Iter(); !iter.Done(); iter.Next()) {
      nsAutoPtr<DecryptPromiseRequestHolder>& holder = iter.Data();
      holder->DisconnectIfExists();
      iter.Remove();
    }
    nsresult rv = mDecoder->Drain();
    Unused << NS_WARN_IF(NS_FAILED(rv));
    return rv;
  }

  nsresult Shutdown() override {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    MOZ_ASSERT(!mIsShutdown);
    mIsShutdown = true;
    nsresult rv = mDecoder->Shutdown();
    Unused << NS_WARN_IF(NS_FAILED(rv));
    mSamplesWaitingForKey->BreakCycles();
    mSamplesWaitingForKey = nullptr;
    mDecoder = nullptr;
    mProxy = nullptr;
    mCallback = nullptr;
    return rv;
  }

private:

  RefPtr<MediaDataDecoder> mDecoder;
  MediaDataDecoderCallback* mCallback;
  RefPtr<TaskQueue> mTaskQueue;
  RefPtr<CDMProxy> mProxy;
  nsClassHashtable<nsRefPtrHashKey<MediaRawData>, DecryptPromiseRequestHolder> mDecrypts;
  RefPtr<SamplesWaitingForKey> mSamplesWaitingForKey;
  bool mIsShutdown;
};

class EMEMediaDataDecoderProxy : public MediaDataDecoderProxy {
public:
  EMEMediaDataDecoderProxy(nsIThread* aProxyThread, MediaDataDecoderCallback* aCallback, CDMProxy* aProxy, FlushableTaskQueue* aTaskQueue)
   : MediaDataDecoderProxy(aProxyThread, aCallback)
   , mSamplesWaitingForKey(new SamplesWaitingForKey(this, aTaskQueue, aProxy))
   , mProxy(aProxy)
  {
  }

  nsresult Input(MediaRawData* aSample) override;
  nsresult Shutdown() override;

private:
  RefPtr<SamplesWaitingForKey> mSamplesWaitingForKey;
  RefPtr<CDMProxy> mProxy;
};

nsresult
EMEMediaDataDecoderProxy::Input(MediaRawData* aSample)
{
  if (mSamplesWaitingForKey->WaitIfKeyNotUsable(aSample)) {
    return NS_OK;
  }

  nsAutoPtr<MediaRawDataWriter> writer(aSample->CreateWriter());
  mProxy->GetSessionIdsForKeyId(aSample->mCrypto.mKeyId,
                                writer->mCrypto.mSessionIds);

  return MediaDataDecoderProxy::Input(aSample);
}

nsresult
EMEMediaDataDecoderProxy::Shutdown()
{
  nsresult rv = MediaDataDecoderProxy::Shutdown();

  mSamplesWaitingForKey->BreakCycles();
  mSamplesWaitingForKey = nullptr;
  mProxy = nullptr;

  return rv;
}

EMEDecoderModule::EMEDecoderModule(CDMProxy* aProxy,
                                   PDMFactory* aPDM,
                                   bool aCDMDecodesAudio,
                                   bool aCDMDecodesVideo)
  : mProxy(aProxy)
  , mPDM(aPDM)
  , mCDMDecodesAudio(aCDMDecodesAudio)
  , mCDMDecodesVideo(aCDMDecodesVideo)
{
}

EMEDecoderModule::~EMEDecoderModule()
{
}

static already_AddRefed<MediaDataDecoderProxy>
CreateDecoderWrapper(MediaDataDecoderCallback* aCallback, CDMProxy* aProxy, FlushableTaskQueue* aTaskQueue)
{
  nsCOMPtr<mozIGeckoMediaPluginService> gmpService = do_GetService("@mozilla.org/gecko-media-plugin-service;1");
  if (!gmpService) {
    return nullptr;
  }

  nsCOMPtr<nsIThread> thread;
  nsresult rv = gmpService->GetThread(getter_AddRefs(thread));
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  RefPtr<MediaDataDecoderProxy> decoder(new EMEMediaDataDecoderProxy(thread, aCallback, aProxy, aTaskQueue));
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
EMEDecoderModule::CreateVideoDecoder(const VideoInfo& aConfig,
                                     layers::LayersBackend aLayersBackend,
                                     layers::ImageContainer* aImageContainer,
                                     FlushableTaskQueue* aVideoTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  MOZ_ASSERT(aConfig.mCrypto.mValid);

  if (mCDMDecodesVideo) {
    RefPtr<MediaDataDecoderProxy> wrapper = CreateDecoderWrapper(aCallback, mProxy, aVideoTaskQueue);
    wrapper->SetProxyTarget(new EMEVideoDecoder(mProxy,
                                                aConfig,
                                                aLayersBackend,
                                                aImageContainer,
                                                aVideoTaskQueue,
                                                wrapper->Callback()));
    return wrapper.forget();
  }

  MOZ_ASSERT(mPDM);
  RefPtr<MediaDataDecoder> decoder(
    mPDM->CreateDecoder(aConfig,
                        aVideoTaskQueue,
                        aCallback,
                        aLayersBackend,
                        aImageContainer));
  if (!decoder) {
    return nullptr;
  }

  RefPtr<MediaDataDecoder> emeDecoder(new EMEDecryptor(decoder,
                                                         aCallback,
                                                         mProxy,
                                                         AbstractThread::GetCurrent()->AsTaskQueue()));
  return emeDecoder.forget();
}

already_AddRefed<MediaDataDecoder>
EMEDecoderModule::CreateAudioDecoder(const AudioInfo& aConfig,
                                     FlushableTaskQueue* aAudioTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  MOZ_ASSERT(aConfig.mCrypto.mValid);

  if (mCDMDecodesAudio) {
    RefPtr<MediaDataDecoderProxy> wrapper = CreateDecoderWrapper(aCallback, mProxy, aAudioTaskQueue);
    wrapper->SetProxyTarget(new EMEAudioDecoder(mProxy,
                                                aConfig,
                                                aAudioTaskQueue,
                                                wrapper->Callback()));
    return wrapper.forget();
  }

  MOZ_ASSERT(mPDM);
  RefPtr<MediaDataDecoder> decoder(
    mPDM->CreateDecoder(aConfig, aAudioTaskQueue, aCallback));
  if (!decoder) {
    return nullptr;
  }

  RefPtr<MediaDataDecoder> emeDecoder(new EMEDecryptor(decoder,
                                                         aCallback,
                                                         mProxy,
                                                         AbstractThread::GetCurrent()->AsTaskQueue()));
  return emeDecoder.forget();
}

PlatformDecoderModule::ConversionRequired
EMEDecoderModule::DecoderNeedsConversion(const TrackInfo& aConfig) const
{
  if (aConfig.IsVideo()) {
    return kNeedAVCC;
  } else {
    return kNeedNone;
  }
}

bool
EMEDecoderModule::SupportsMimeType(const nsACString& aMimeType) const
{
  Maybe<nsCString> gmp;
  gmp.emplace(NS_ConvertUTF16toUTF8(mProxy->KeySystem()));
  return GMPDecoderModule::SupportsMimeType(aMimeType, gmp);
}

} // namespace mozilla
