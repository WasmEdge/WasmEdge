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
                                           uint32_t AVFilterContextId,
                                           uint32_t AVFrameId) {

  FFMPEG_PTR_FETCH(AvFilterCtx, AVFilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(AvFrame, AVFrameId, AVFrame);
  return av_buffersink_get_frame(AvFilterCtx, AvFrame);
}

Expect<int32_t> AVBufferSinkGetSamples::body(const Runtime::CallingFrame &,
                                             uint32_t AVFilterContextId,
                                             uint32_t AVFrameId,
                                             int32_t Samples) {

  FFMPEG_PTR_FETCH(AvFilterCtx, AVFilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(AvFrame, AVFrameId, AVFrame);
  return av_buffersink_get_samples(AvFilterCtx, AvFrame, Samples);
}

Expect<int32_t> AvBufferSinkSetFrameSize::body(const Runtime::CallingFrame &,
                                               uint32_t AVFilterContextId,
                                               int32_t Value) {

  FFMPEG_PTR_FETCH(AvFilterCtx, AVFilterContextId, AVFilterContext);
  av_buffersink_set_frame_size(AvFilterCtx, Value);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVBufferSrcGetNbFailedRequests::body(const Runtime::CallingFrame &,
                                     uint32_t AVFilterContextId) {

  FFMPEG_PTR_FETCH(AvFilterCtx, AVFilterContextId, AVFilterContext);
  return av_buffersrc_get_nb_failed_requests(AvFilterCtx);
}

Expect<int32_t> AVBufferSrcAddFrame::body(const Runtime::CallingFrame &,
                                          uint32_t AVFilterContextId,
                                          uint32_t AVFrameId) {

  FFMPEG_PTR_FETCH(AvFilterCtx, AVFilterContextId, AVFilterContext);
  FFMPEG_PTR_FETCH(AvFrame, AVFrameId, AVFrame);
  return av_buffersrc_add_frame(AvFilterCtx, AvFrame);
}

Expect<int32_t> AVBufferSrcClose::body(const Runtime::CallingFrame &,
                                       uint32_t AVFilterContextId, int64_t Pts,
                                       uint32_t Flags) {

  FFMPEG_PTR_FETCH(AvFilterCtx, AVFilterContextId, AVFilterContext);
  return av_buffersrc_close(AvFilterCtx, Pts, Flags);
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
