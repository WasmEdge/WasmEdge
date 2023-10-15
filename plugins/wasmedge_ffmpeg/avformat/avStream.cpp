#include "avStream.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVStreamId::body(const Runtime::CallingFrame &,
                                 uint32_t AvFormatCtxId, uint32_t StreamIdx) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= StreamIdx; I++)
    AvStream++;

  return static_cast<AVStream *>(*AvStream)->id;
}

Expect<int32_t> AVStreamIndex::body(const Runtime::CallingFrame &,
                                    uint32_t AvFormatCtxId,
                                    uint32_t StreamIdx) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  // No check here (Check)
  // Get the Requried AVStream
  for (unsigned int I = 1; I <= StreamIdx; I++)
    AvStream++;

  return static_cast<AVStream *>(*AvStream)->index;
}

Expect<int32_t> AVStreamCodecPar::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx,
                                       uint32_t CodecParameterPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(CodecParamId, MemInst, uint32_t, CodecParameterPtr,
                "Failed when accessing the return CodecParameter Memory");

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVStream **AvStream = AvFormatContext->streams;

  // No check here (Check)
  // Get the Required AVStream
  for (unsigned int I = 1; I <= StreamIdx; I++)
    AvStream++;

  AVCodecParameters *CodecParam =
      (static_cast<AVStream *>(*AvStream))->codecpar;
  FFMPEG_PTR_STORE(CodecParam, CodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
