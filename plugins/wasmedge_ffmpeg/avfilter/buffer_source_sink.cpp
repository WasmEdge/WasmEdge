// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "buffer_source_sink.h"

extern "C" {
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

Expect<int32_t> AVBufferSinkGetFrame::body(const Runtime::CallingFrame &,
                                           uint32_t FilterContextId,
                                           uint32_t FrameId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(Frame, FrameId, AVFrame);
  return av_buffersink_get_frame(FilterCtx, Frame);
}

Expect<int32_t> AVBufferSinkGetSamples::body(const Runtime::CallingFrame &,
                                             uint32_t FilterContextId,
                                             uint32_t FrameId,
                                             int32_t Samples) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(Frame, FrameId, AVFrame);
  return av_buffersink_get_samples(FilterCtx, Frame, Samples);
}

Expect<int32_t> AvBufferSinkSetFrameSize::body(const Runtime::CallingFrame &,
                                               uint32_t FilterContextId,
                                               int32_t Value) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  av_buffersink_set_frame_size(FilterCtx, Value);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVBufferSrcGetNbFailedRequests::body(const Runtime::CallingFrame &,
                                     uint32_t FilterContextId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  return av_buffersrc_get_nb_failed_requests(FilterCtx);
}

Expect<int32_t> AVBufferSrcAddFrame::body(const Runtime::CallingFrame &,
                                          uint32_t FilterContextId,
                                          uint32_t FrameId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(Frame, FrameId, AVFrame);
  return av_buffersrc_add_frame(FilterCtx, Frame);
}

Expect<int32_t> AVBufferSrcClose::body(const Runtime::CallingFrame &,
                                       uint32_t FilterContextId, int64_t Pts,
                                       uint32_t Flags) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterContextId, AVFilterContext);
  return av_buffersrc_close(FilterCtx, Pts, Flags);
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
