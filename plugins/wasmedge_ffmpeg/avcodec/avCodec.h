#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecID : public WasmEdgeFFmpegAVCodec<AVCodecID> {
public:
  AVCodecID(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecType : public WasmEdgeFFmpegAVCodec<AVCodecType> {
public:
  AVCodecType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
