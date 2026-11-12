// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "buffer_source_sink.h"

extern "C" {
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

#include <cstring>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

namespace {
constexpr int32_t MaxSinkSamples = 1 << 20;

bool isConfiguredFilter(const WasmEdgeFFmpegEnv &Env, uint32_t CtxId,
                        const AVFilterContext *Ctx, const char *Name,
                        const char *AudioName) {
  if (Ctx->filter == nullptr || Ctx->filter->name == nullptr ||
      (std::strcmp(Ctx->filter->name, Name) != 0 &&
       std::strcmp(Ctx->filter->name, AudioName) != 0)) {
    return false;
  }
  uint32_t const GraphId = Env.parentIdOf(CtxId);
  return GraphId != 0 &&
         Env.hasFlag(GraphId, WasmEdgeFFmpegEnv::GraphConfigured);
}

bool isConfiguredSink(const WasmEdgeFFmpegEnv &Env, uint32_t CtxId,
                      const AVFilterContext *Ctx) {
  return isConfiguredFilter(Env, CtxId, Ctx, "buffersink", "abuffersink");
}

bool isConfiguredAudioSink(const WasmEdgeFFmpegEnv &Env, uint32_t CtxId,
                           const AVFilterContext *Ctx) {
  return isConfiguredSink(Env, CtxId, Ctx) &&
         std::strcmp(Ctx->filter->name, "abuffersink") == 0;
}

bool isConfiguredSource(const WasmEdgeFFmpegEnv &Env, uint32_t CtxId,
                        const AVFilterContext *Ctx) {
  return isConfiguredFilter(Env, CtxId, Ctx, "buffer", "abuffer");
}

bool frameFitsSourceLink(const AVFilterContext *Ctx, const AVFrame *Frame) {
  if (Ctx->nb_outputs < 1 || Ctx->outputs[0] == nullptr) {
    return false;
  }
  const AVFilterLink *Link = Ctx->outputs[0];
  if (Link->type != AVMEDIA_TYPE_VIDEO) {
    return true;
  }
  return hasBuffers(Frame) && Frame->width == Link->w &&
         Frame->height == Link->h && Frame->format == Link->format;
}
} // namespace

Expect<int32_t> AVBufferSinkGetFrame::body(const Runtime::CallingFrame &,
                                           uint32_t FilterContextId,
                                           uint32_t FrameId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(Frame, FrameId, AVFrame);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_CHECK(Frame, static_cast<int32_t>(ErrNo::InternalError));
  if (!isConfiguredSink(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  av_frame_unref(Frame);
  return av_buffersink_get_frame(FilterCtx, Frame);
}

Expect<int32_t> AVBufferSinkGetSamples::body(const Runtime::CallingFrame &,
                                             uint32_t FilterContextId,
                                             uint32_t FrameId,
                                             int32_t Samples) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(Frame, FrameId, AVFrame);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_CHECK(Frame, static_cast<int32_t>(ErrNo::InternalError));
  if (Samples < 0 || Samples > MaxSinkSamples ||
      !isConfiguredAudioSink(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  av_frame_unref(Frame);
  return av_buffersink_get_samples(FilterCtx, Frame, Samples);
}

Expect<int32_t> AvBufferSinkSetFrameSize::body(const Runtime::CallingFrame &,
                                               uint32_t FilterContextId,
                                               int32_t Value) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  if (Value < 0 || Value > MaxSinkSamples ||
      !isConfiguredAudioSink(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  av_buffersink_set_frame_size(FilterCtx, static_cast<unsigned>(Value));
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVBufferSrcGetNbFailedRequests::body(const Runtime::CallingFrame &,
                                     uint32_t FilterContextId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  if (!isConfiguredSource(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return av_buffersrc_get_nb_failed_requests(FilterCtx);
}

Expect<int32_t> AVBufferSrcAddFrame::body(const Runtime::CallingFrame &,
                                          uint32_t FilterContextId,
                                          uint32_t FrameId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(Frame, FrameId, AVFrame);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  if (FrameId != 0) {
    FFMPEG_PTR_CHECK(Frame, static_cast<int32_t>(ErrNo::InternalError));
  }
  if (!isConfiguredSource(*Env.get(), FilterContextId, FilterCtx) ||
      (Frame != nullptr && !frameFitsSourceLink(FilterCtx, Frame))) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return av_buffersrc_add_frame(FilterCtx, Frame);
}

Expect<int32_t> AVBufferSrcClose::body(const Runtime::CallingFrame &,
                                       uint32_t FilterContextId, int64_t Pts,
                                       uint32_t Flags) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  if (!isConfiguredSource(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return av_buffersrc_close(FilterCtx, Pts, Flags);
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
