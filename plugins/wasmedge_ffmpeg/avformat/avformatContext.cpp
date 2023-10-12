#include "avformatContext.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVFormatCtxIFormat::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AvFormatCtxId,
                                         uint32_t AvInputFormatPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvInputFormatId, MemInst, uint32_t, AvInputFormatPtr,
                "Failed when accessing the return AVInputFormat Memory");

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);

  AVInputFormat const *AvInputFormat = AvFormatCtx->iformat;
  FFMPEG_PTR_STORE((void *)AvInputFormat, AvInputFormatId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatCtxProbeScore::body(const Runtime::CallingFrame &,
                                            uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->probe_score;
}

Expect<uint32_t> AVFormatCtxNbStreams::body(const Runtime::CallingFrame &,
                                            uint32_t avFormatCtxId) {

  auto ffmpegMemory = Env.get();
  AVFormatContext *avFormatCtx =
      static_cast<AVFormatContext *>(ffmpegMemory->fetchData(avFormatCtxId));
  return avFormatCtx->nb_streams;
};

Expect<int64_t> AVFormatCtxBitRate::body(const Runtime::CallingFrame &,
                                         uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->bit_rate;
}

Expect<int64_t> AVFormatCtxDuration::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return AvFormatContext->duration;
}

Expect<uint32_t> AVFormatCtxNbChapters::body(const Runtime::CallingFrame &,
                                             uint32_t avFormatCtxId) {

  auto ffmpegMemory = Env.get();
  AVFormatContext *avFormatCtx =
      static_cast<AVFormatContext *>(ffmpegMemory->fetchData(avFormatCtxId));
  return avFormatCtx->nb_chapters;
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
