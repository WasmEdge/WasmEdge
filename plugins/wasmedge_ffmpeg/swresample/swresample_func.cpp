// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "swresample_func.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

Expect<uint32_t> SWResampleVersion::body(const Runtime::CallingFrame &) {
  return swresample_version();
}

Expect<int64_t> SWRGetDelay::body(const Runtime::CallingFrame &,
                                  uint32_t SWRContextId, int64_t Base) {
  FFMPEG_PTR_FETCH(SWRContext, SWRContextId, SwrContext);
  return swr_get_delay(SWRContext, Base);
}

Expect<int32_t> SWRInit::body(const Runtime::CallingFrame &,
                              uint32_t SWRContextId) {
  FFMPEG_PTR_FETCH(SWRContext, SWRContextId, SwrContext);
  return swr_init(SWRContext);
}

Expect<int32_t>
SWRAllocSetOpts::body(const Runtime::CallingFrame &Frame, uint32_t SwrCtxPtr,
                      uint32_t SWRContextId, uint64_t OutChLayoutId,
                      uint32_t OutSampleFmtId, int32_t OutSampleRate,
                      uint64_t InChLayoutId, uint32_t InSampleFmtId,
                      int32_t InSampleRate, int32_t LogOffset) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwrCtxId, MemInst, uint32_t, SwrCtxPtr, "")
  FFMPEG_PTR_FETCH(ExistSWRContext, SWRContextId, SwrContext);

  // A SWRContextId of 0 is the legitimate "allocate a fresh context" request.
  // A nonzero id that does not resolve is forged or stale; passing it through
  // as null would silently allocate a new context and bypass handle-id
  // validation, so reject it instead of changing the call's semantics.
  if (SWRContextId != 0 && ExistSWRContext == nullptr) {
    *SwrCtxId = 0;
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  uint64_t const OutChLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(OutChLayoutId);
  AVSampleFormat const OutSampleFmt =
      FFmpegUtils::SampleFmt::fromSampleID(OutSampleFmtId);
  uint64_t const InChLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(InChLayoutId);
  AVSampleFormat const InSampleFmt =
      FFmpegUtils::SampleFmt::fromSampleID(InSampleFmtId);

  AVChannelLayout AVOutChLayout;
  int const OutRet = av_channel_layout_from_mask(&AVOutChLayout, OutChLayout);
  if (OutRet < 0) {
    *SwrCtxId = 0;
    spdlog::error("[WasmEdge-FFmpeg] SWRAllocSetOpts: "
                  "av_channel_layout_from_mask failed ({}) for output "
                  "channel layout mask {:#x}"sv,
                  OutRet, OutChLayout);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVChannelLayout AVInChLayout;
  int const InRet = av_channel_layout_from_mask(&AVInChLayout, InChLayout);
  if (InRet < 0) {
    av_channel_layout_uninit(&AVOutChLayout);
    *SwrCtxId = 0;
    spdlog::error("[WasmEdge-FFmpeg] SWRAllocSetOpts: "
                  "av_channel_layout_from_mask failed ({}) for input channel "
                  "layout mask {:#x}"sv,
                  InRet, InChLayout);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  SwrContext *const PrevSWRContext = ExistSWRContext;
  int const AllocRet = swr_alloc_set_opts2(
      &ExistSWRContext, &AVOutChLayout, OutSampleFmt, OutSampleRate,
      &AVInChLayout, InSampleFmt, InSampleRate, LogOffset,
      nullptr); // Always being used as null in rust sdk.
  if (AllocRet < 0) {
    av_channel_layout_uninit(&AVOutChLayout);
    av_channel_layout_uninit(&AVInChLayout);
    if (PrevSWRContext != nullptr) {
      Env.get()->deallocByValue(PrevSWRContext);
    }
    *SwrCtxId = 0;
    spdlog::error("[WasmEdge-FFmpeg] SWRAllocSetOpts: swr_alloc_set_opts2 "
                  "failed ({})"sv,
                  AllocRet);
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  av_channel_layout_uninit(&AVOutChLayout);
  av_channel_layout_uninit(&AVInChLayout);

  FFMPEG_PTR_STORE(ExistSWRContext, SwrCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOptSetDict::body(const Runtime::CallingFrame &,
                                   uint32_t SWRContextId, uint32_t DictId) {
  FFMPEG_PTR_FETCH(SWRContext, SWRContextId, SwrContext);
  FFMPEG_PTR_FETCH(AvDictionary, DictId, AVDictionary *);
  return av_opt_set_dict(SWRContext, AvDictionary);
}

Expect<int32_t> SWRConvertFrame::body(const Runtime::CallingFrame &,
                                      uint32_t SWRContextId,
                                      uint32_t FrameOutputId,
                                      uint32_t FrameInputId) {
  FFMPEG_PTR_FETCH(SWRContext, SWRContextId, SwrContext);
  FFMPEG_PTR_FETCH(OuputFrame, FrameOutputId, AVFrame);
  FFMPEG_PTR_FETCH(InputFrame, FrameInputId, AVFrame);

  return swr_convert_frame(SWRContext, OuputFrame, InputFrame);
}

Expect<int32_t> SWRFree::body(const Runtime::CallingFrame &,
                              uint32_t SWRContextId) {
  FFMPEG_PTR_FETCH(SWRContext, SWRContextId, SwrContext);
  SwrContext *const FreedCtx = SWRContext;
  swr_free(&SWRContext);
  Env.get()->deallocByValue(FreedCtx);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
SWResampleConfigurationLength::body(const Runtime::CallingFrame &) {
  const char *Config = swresample_configuration();
  return strlen(Config);
}

Expect<int32_t>
SWResampleConfiguration::body(const Runtime::CallingFrame &Frame,
                              uint32_t ConfigPtr, uint32_t ConfigLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ConfigBuf, MemInst, char, ConfigPtr, ConfigLen, "");

  const char *Config = swresample_configuration();
  copyCStringToBuffer(ConfigBuf.data(), ConfigLen, Config);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SWResampleLicenseLength::body(const Runtime::CallingFrame &) {
  const char *License = swresample_license();
  return strlen(License);
}

Expect<int32_t> SWResampleLicense::body(const Runtime::CallingFrame &Frame,
                                        uint32_t LicensePtr,
                                        uint32_t LicenseLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LicenseBuf, MemInst, char, LicensePtr, LicenseLen, "");

  const char *License = swresample_license();
  copyCStringToBuffer(LicenseBuf.data(), LicenseLen, License);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
