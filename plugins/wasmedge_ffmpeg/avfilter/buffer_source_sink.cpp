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

bool isConfiguredSource(const WasmEdgeFFmpegEnv &Env, uint32_t CtxId,
                        const AVFilterContext *Ctx) {
  return isConfiguredFilter(Env, CtxId, Ctx, "buffer", "abuffer");
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
  if (!isConfiguredSink(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return av_buffersink_get_samples(FilterCtx, Frame, Samples);
}

Expect<int32_t> AvBufferSinkSetFrameSize::body(const Runtime::CallingFrame &,
                                               uint32_t FilterContextId,
                                               int32_t Value) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  if (!isConfiguredSink(*Env.get(), FilterContextId, FilterCtx)) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  av_buffersink_set_frame_size(FilterCtx, Value);
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
  if (!isConfiguredSource(*Env.get(), FilterContextId, FilterCtx)) {
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
