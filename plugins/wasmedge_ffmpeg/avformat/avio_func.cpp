#include "avio_func.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<void> AVIOClose::body(const Runtime::CallingFrame &,
                             uint32_t AvFormatCtxId) {

  auto ffmpegMemory = Env.get();
  AVFormatContext *avFormatCtx =
      static_cast<AVFormatContext *>(ffmpegMemory->fetchData(AvFormatCtxId));
  avio_close(avFormatCtx->pb);
  return {};
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
