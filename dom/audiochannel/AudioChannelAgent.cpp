/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AudioChannelAgent.h"
#include "AudioChannelService.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsXULAppAPI.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioChannelAgent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioChannelAgent)
  tmp->Shutdown();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCallback)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(AudioChannelAgent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCallback)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AudioChannelAgent)
  NS_INTERFACE_MAP_ENTRY(nsIAudioChannelAgent)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioChannelAgent)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioChannelAgent)

AudioChannelAgent::AudioChannelAgent()
  : mAudioChannelType(AUDIO_AGENT_CHANNEL_ERROR)
  , mInnerWindowID(0)
  , mIsRegToService(false)
  , mNotifyPlayback(false)
{
}

AudioChannelAgent::~AudioChannelAgent()
{
  Shutdown();
}

void
AudioChannelAgent::Shutdown()
{
  if (mIsRegToService) {
    NotifyStoppedPlaying();
  }
}

NS_IMETHODIMP AudioChannelAgent::GetAudioChannelType(int32_t *aAudioChannelType)
{
  *aAudioChannelType = mAudioChannelType;
  return NS_OK;
}

NS_IMETHODIMP
AudioChannelAgent::Init(nsIDOMWindow* aWindow, int32_t aChannelType,
                        nsIAudioChannelAgentCallback *aCallback)
{
  return InitInternal(aWindow, aChannelType, aCallback,
                      /* useWeakRef = */ false);
}

NS_IMETHODIMP
AudioChannelAgent::InitWithWeakCallback(nsIDOMWindow* aWindow,
                                        int32_t aChannelType,
                                        nsIAudioChannelAgentCallback *aCallback)
{
  return InitInternal(aWindow, aChannelType, aCallback,
                      /* useWeakRef = */ true);
}

nsresult
AudioChannelAgent::InitInternal(nsIDOMWindow* aWindow, int32_t aChannelType,
                                nsIAudioChannelAgentCallback *aCallback,
                                bool aUseWeakRef)
{
  // We syncd the enum of channel type between nsIAudioChannelAgent.idl and
  // AudioChannelBinding.h the same.
  MOZ_ASSERT(int(AUDIO_AGENT_CHANNEL_NORMAL) == int(AudioChannel::Normal) &&
             int(AUDIO_AGENT_CHANNEL_CONTENT) == int(AudioChannel::Content) &&
             int(AUDIO_AGENT_CHANNEL_NOTIFICATION) == int(AudioChannel::Notification) &&
             int(AUDIO_AGENT_CHANNEL_ALARM) == int(AudioChannel::Alarm) &&
             int(AUDIO_AGENT_CHANNEL_TELEPHONY) == int(AudioChannel::Telephony) &&
             int(AUDIO_AGENT_CHANNEL_RINGER) == int(AudioChannel::Ringer) &&
             int(AUDIO_AGENT_CHANNEL_SYSTEM) == int(AudioChannel::System) &&
             int(AUDIO_AGENT_CHANNEL_PUBLICNOTIFICATION) == int(AudioChannel::Publicnotification),
             "Enum of channel on nsIAudioChannelAgent.idl should be the same with AudioChannelBinding.h");

  if (mAudioChannelType != AUDIO_AGENT_CHANNEL_ERROR ||
      aChannelType > AUDIO_AGENT_CHANNEL_SYSTEM ||
      aChannelType < AUDIO_AGENT_CHANNEL_NORMAL) {
    return NS_ERROR_FAILURE;
  }

  if (NS_WARN_IF(!aWindow)) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> pInnerWindow = do_QueryInterface(aWindow);
  MOZ_ASSERT(pInnerWindow->IsInnerWindow());
  mInnerWindowID = pInnerWindow->WindowID();

  nsCOMPtr<nsPIDOMWindow> topWindow = pInnerWindow->GetScriptableTop();
  if (NS_WARN_IF(!topWindow)) {
    return NS_OK;
  }

  mWindow = do_QueryInterface(topWindow);
  if (mWindow) {
    mWindow = mWindow->GetOuterWindow();
  }

  if (NS_WARN_IF(!mWindow)) {
    return NS_ERROR_FAILURE;
  }

  mAudioChannelType = aChannelType;

  if (aUseWeakRef) {
    mWeakCallback = do_GetWeakReference(aCallback);
  } else {
    mCallback = aCallback;
  }

  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelAgent, InitInternal, this = %p, type = %d, "
          "owner = %p, hasCallback = %d\n", this, mAudioChannelType,
          mWindow.get(), (!!mCallback || !!mWeakCallback)));

  return NS_OK;
}

NS_IMETHODIMP AudioChannelAgent::NotifyStartedPlaying(uint32_t aNotifyPlayback,
                                                      float *aVolume,
                                                      bool* aMuted)
{
  MOZ_ASSERT(aVolume);
  MOZ_ASSERT(aMuted);

  // Window-less AudioChannelAgents are muted by default.
  if (!mWindow) {
    *aVolume = 0;
    *aMuted = true;
    return NS_OK;
  }

  RefPtr<AudioChannelService> service = AudioChannelService::GetOrCreate();
  if (mAudioChannelType == AUDIO_AGENT_CHANNEL_ERROR ||
      service == nullptr || mIsRegToService) {
    return NS_ERROR_FAILURE;
  }

  service->RegisterAudioChannelAgent(this, aNotifyPlayback,
    static_cast<AudioChannel>(mAudioChannelType));

  service->GetState(mWindow, mAudioChannelType, aVolume, aMuted);

  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelAgent, NotifyStartedPlaying, this = %p, mute = %d, "
          "volume = %f\n", this, *aMuted, *aVolume));

  mNotifyPlayback = aNotifyPlayback;
  mIsRegToService = true;
  return NS_OK;
}

NS_IMETHODIMP AudioChannelAgent::NotifyStoppedPlaying()
{
  if (mAudioChannelType == AUDIO_AGENT_CHANNEL_ERROR ||
      !mIsRegToService) {
    return NS_ERROR_FAILURE;
  }

  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelAgent, NotifyStoppedPlaying, this = %p\n", this));

  RefPtr<AudioChannelService> service = AudioChannelService::GetOrCreate();
  if (service) {
    service->UnregisterAudioChannelAgent(this, mNotifyPlayback);
  }

  mIsRegToService = false;
  return NS_OK;
}

already_AddRefed<nsIAudioChannelAgentCallback>
AudioChannelAgent::GetCallback()
{
  nsCOMPtr<nsIAudioChannelAgentCallback> callback = mCallback;
  if (!callback) {
    callback = do_QueryReferent(mWeakCallback);
  }
  return callback.forget();
}

void
AudioChannelAgent::WindowVolumeChanged()
{
  nsCOMPtr<nsIAudioChannelAgentCallback> callback = GetCallback();
  if (!callback) {
    return;
  }

  float volume = 1.0;
  bool muted = false;

  RefPtr<AudioChannelService> service = AudioChannelService::GetOrCreate();
  if (service) {
    service->GetState(mWindow, mAudioChannelType, &volume, &muted);
  }

  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelAgent, WindowVolumeChanged, this = %p, mute = %d, "
          "volume = %f\n", this, muted, volume));

  callback->WindowVolumeChanged(volume, muted);
}

uint64_t
AudioChannelAgent::WindowID() const
{
  return mWindow ? mWindow->WindowID() : 0;
}

void
AudioChannelAgent::WindowAudioCaptureChanged(uint64_t aInnerWindowID)
{
  if (aInnerWindowID != mInnerWindowID) {
    return;
  }

  nsCOMPtr<nsIAudioChannelAgentCallback> callback = GetCallback();
  if (!callback) {
    return;
  }

  callback->WindowAudioCaptureChanged();
}
