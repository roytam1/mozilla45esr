/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/TaskQueue.h"

#include <string.h>
#if defined(XP_WIN)
#else
#include <unistd.h>
#endif

#include "FFmpegLog.h"
#include "FFmpegDataDecoder.h"
#include "prsystem.h"
#include "FFmpegRuntimeLinker.h"

#include "libavutil/pixfmt.h"
#if LIBAVCODEC_VERSION_MAJOR < 54
#define AVPixelFormat PixelFormat
#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#define AV_PIX_FMT_YUVJ420P PIX_FMT_YUVJ420P
#define AV_PIX_FMT_NONE PIX_FMT_NONE
#endif

namespace mozilla
{

bool FFmpegDataDecoder<LIBAV_VER>::sFFmpegInitDone = false;
StaticMutex FFmpegDataDecoder<LIBAV_VER>::sMonitor;

FFmpegDataDecoder<LIBAV_VER>::FFmpegDataDecoder(FlushableTaskQueue* aTaskQueue,
                                                MediaDataDecoderCallback* aCallback,
                                                AVCodecID aCodecID)
  : mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mCodecContext(nullptr)
  , mFrame(NULL)
  , mExtraData(nullptr)
  , mCodecID(aCodecID)
  , mMonitor("FFMpegaDataDecoder")
  , mIsFlushing(false)
  , mCodecParser(nullptr)
{
  MOZ_COUNT_CTOR(FFmpegDataDecoder);
}

FFmpegDataDecoder<LIBAV_VER>::~FFmpegDataDecoder()
{
  MOZ_COUNT_DTOR(FFmpegDataDecoder);
  if (mCodecParser) {
#if defined(XP_WIN)
    reinterpret_cast<void(*)(AVCodecParserContext*)>(FFmpegRuntimeLinker::avc_ptr[_parser_close])(mCodecParser);
#else
    av_parser_close(mCodecParser);
#endif
    mCodecParser = nullptr;
  }
}

/**
 * FFmpeg calls back to this function with a list of pixel formats it supports.
 * We choose a pixel format that we support and return it.
 * For now, we just look for YUV420P as it is the only non-HW accelerated format
 * supported by FFmpeg's H264 decoder.
 */
static AVPixelFormat
ChoosePixelFormat(AVCodecContext* aCodecContext, const AVPixelFormat* aFormats)
{
  FFMPEG_LOG("Choosing FFmpeg pixel format for video decoding.");
  for (; *aFormats > -1; aFormats++) {
    if (*aFormats == AV_PIX_FMT_YUV420P) {
      FFMPEG_LOG("Requesting pixel format YUV420P.");
      return AV_PIX_FMT_YUV420P;
    } else if (*aFormats == AV_PIX_FMT_YUVJ420P) {
      FFMPEG_LOG("Requesting pixel format YUVJ420P.");
      return AV_PIX_FMT_YUVJ420P;
    }
  }

  NS_WARNING("FFmpeg does not share any supported pixel formats.");
  return AV_PIX_FMT_NONE;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::InitDecoder()
{
  FFMPEG_LOG("Initialising FFmpeg decoder.");

  AVCodec* codec = FindAVCodec(mCodecID);
  if (!codec) {
    NS_WARNING("Couldn't find ffmpeg decoder");
    return NS_ERROR_FAILURE;
  }

  StaticMutexAutoLock mon(sMonitor);

  if (!(mCodecContext = 
#if defined(XP_WIN)
       reinterpret_cast<AVCodecContext*(*)(const AVCodec*)>(FFmpegRuntimeLinker::avc_ptr[_alloc_context3])(codec)
#else
       avcodec_alloc_context3(codec)
#endif
      )) {
    NS_WARNING("Couldn't init ffmpeg context");
    return NS_ERROR_FAILURE;
  }

  mCodecContext->opaque = this;

  // FFmpeg takes this as a suggestion for what format to use for audio samples.
  uint32_t major, minor, micro;
  FFmpegRuntimeLinker::GetVersion(major, minor, micro);
  // LibAV 0.8 produces rubbish float interleaved samples, request 16 bits audio.
  mCodecContext->request_sample_fmt = major == 53 ?
    AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_FLT;

  // FFmpeg will call back to this to negotiate a video pixel format.
  mCodecContext->get_format = ChoosePixelFormat;

  mCodecContext->thread_count = PR_GetNumberOfProcessors();
  mCodecContext->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
  mCodecContext->thread_safe_callbacks = false;

  if (mExtraData) {
    mCodecContext->extradata_size = mExtraData->Length();
    // FFmpeg may use SIMD instructions to access the data which reads the
    // data in 32 bytes block. Must ensure we have enough data to read.
    mExtraData->AppendElements(FF_INPUT_BUFFER_PADDING_SIZE);
    mCodecContext->extradata = mExtraData->Elements();
  } else {
    mCodecContext->extradata_size = 0;
  }

  if (codec->capabilities & CODEC_CAP_DR1) {
    mCodecContext->flags |= CODEC_FLAG_EMU_EDGE;
  }

  if (
#if defined(XP_WIN)
      reinterpret_cast<int(*)(AVCodecContext*,const AVCodec*,AVDictionary**)>(FFmpegRuntimeLinker::avc_ptr[_open2])(mCodecContext, codec, nullptr)
#else
      avcodec_open2(mCodecContext, codec, nullptr)
#endif
     < 0) {
    NS_WARNING("Couldn't initialise ffmpeg decoder");
#if defined(XP_WIN)
    reinterpret_cast<int(*)(AVCodecContext*)>(FFmpegRuntimeLinker::avc_ptr[_close])(mCodecContext);
    reinterpret_cast<void(*)(void*)>(FFmpegRuntimeLinker::avc_ptr[_freep])(&mCodecContext);
#else
    avcodec_close(mCodecContext);
    av_freep(&mCodecContext);
#endif
    return NS_ERROR_FAILURE;
  }

  if (mCodecContext->codec_type == AVMEDIA_TYPE_AUDIO &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_FLT &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_FLTP &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_S16 &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_S16P) {
    NS_WARNING("FFmpeg audio decoder outputs unsupported audio format.");
    return NS_ERROR_FAILURE;
  }

  mCodecParser =
#if defined(XP_WIN)
           reinterpret_cast<AVCodecParserContext*(*)(int)>(FFmpegRuntimeLinker::avc_ptr[_parser_init])(mCodecID);
#else
           av_parser_init(mCodecID);
#endif
  if (mCodecParser) {
    mCodecParser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
  }

  FFMPEG_LOG("FFmpeg init successful.");
  return NS_OK;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::Shutdown()
{
  if (mTaskQueue) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &FFmpegDataDecoder<LIBAV_VER>::ProcessShutdown);
    mTaskQueue->Dispatch(runnable.forget());
  } else {
    ProcessShutdown();
  }
  return NS_OK;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::Flush()
{
  MOZ_ASSERT(mCallback->OnReaderTaskQueue());
  mIsFlushing = true;
  mTaskQueue->Flush();
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &FFmpegDataDecoder<LIBAV_VER>::ProcessFlush);
  MonitorAutoLock mon(mMonitor);
  mTaskQueue->Dispatch(runnable.forget());
  while (mIsFlushing) {
    mon.Wait();
  }
  return NS_OK;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::Drain()
{
  MOZ_ASSERT(mCallback->OnReaderTaskQueue());
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &FFmpegDataDecoder<LIBAV_VER>::ProcessDrain);
  mTaskQueue->Dispatch(runnable.forget());
  return NS_OK;
}

void
FFmpegDataDecoder<LIBAV_VER>::ProcessFlush()
{
  MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
  if (mCodecContext) {
#if defined(XP_WIN)
    reinterpret_cast<void(*)(AVCodecContext*)>(FFmpegRuntimeLinker::avc_ptr[_flush_buffers])(mCodecContext);
#else
    avcodec_flush_buffers(mCodecContext);
#endif
  }
  MonitorAutoLock mon(mMonitor);
  mIsFlushing = false;
  mon.NotifyAll();
}

void
FFmpegDataDecoder<LIBAV_VER>::ProcessShutdown()
{
  StaticMutexAutoLock mon(sMonitor);

  if (sFFmpegInitDone && mCodecContext) {
#if defined(XP_WIN)
    reinterpret_cast<int(*)(AVCodecContext*)>(FFmpegRuntimeLinker::avc_ptr[_close])(mCodecContext);
    reinterpret_cast<void(*)(void*)>(FFmpegRuntimeLinker::avc_ptr[_freep])(&mCodecContext);
    reinterpret_cast<void(*)(AVFrame**)>(FFmpegRuntimeLinker::avc_ptr[_frame_free])(&mFrame);
#else
    avcodec_close(mCodecContext);
    av_freep(&mCodecContext);
#if LIBAVCODEC_VERSION_MAJOR >= 55
    av_frame_free(&mFrame);
#elif LIBAVCODEC_VERSION_MAJOR == 54
    avcodec_free_frame(&mFrame);
#else
    av_freep(&mFrame);
#endif
#endif
  }
}

AVFrame*
FFmpegDataDecoder<LIBAV_VER>::PrepareFrame()
{
  MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
#if LIBAVCODEC_VERSION_MAJOR >= 55
  if (mFrame) {
#if defined(XP_WIN)
    reinterpret_cast<void(*)(AVFrame*)>(FFmpegRuntimeLinker::avc_ptr[_frame_unref])(mFrame);
#else
    av_frame_unref(mFrame);
#endif
  } else {
    mFrame = 
#if defined(XP_WIN)
        reinterpret_cast<AVFrame*(*)()>(FFmpegRuntimeLinker::avc_ptr[_frame_alloc])();
#else
        av_frame_alloc();
#endif
  }
#elif LIBAVCODEC_VERSION_MAJOR == 54
  if (mFrame) {
    avcodec_get_frame_defaults(mFrame);
  } else {
    mFrame = avcodec_alloc_frame();
  }
#else
  av_freep(&mFrame);
  mFrame = avcodec_alloc_frame();
#endif
  return mFrame;
}

/* static */ AVCodec*
FFmpegDataDecoder<LIBAV_VER>::FindAVCodec(AVCodecID aCodec)
{
  StaticMutexAutoLock mon(sMonitor);
  if (!sFFmpegInitDone) {
#if defined(XP_WIN)
    reinterpret_cast<void(*)()>(FFmpegRuntimeLinker::avc_ptr[_register_all])();
#else
    avcodec_register_all();
#endif
#ifdef DEBUG
#if defined(XP_WIN)
    reinterpret_cast<void(*)(int)>(FFmpegRuntimeLinker::avc_ptr[_log_set_level])(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_DEBUG);
#endif
#endif
    sFFmpegInitDone = true;
  }
#if defined(XP_WIN)
  return reinterpret_cast<AVCodec*(*)(enum AVCodecID)>(FFmpegRuntimeLinker::avc_ptr[_find_decoder])(aCodec);
#else
  return avcodec_find_decoder(aCodec);
#endif
}
  
} // namespace mozilla
