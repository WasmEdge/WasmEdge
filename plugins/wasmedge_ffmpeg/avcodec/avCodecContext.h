#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecCtxCodecID  : public WasmEdgeFFmpegAVCodec<AVCodecCtxCodecID> {
public:
  AVCodecCtxCodecID(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId);
};

class AVCodecCtxCodecType  : public WasmEdgeFFmpegAVCodec<AVCodecCtxCodecType> {
public:
  AVCodecCtxCodecType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId);
};

class AVCodecCtxTimeBase  : public WasmEdgeFFmpegAVCodec<AVCodecCtxTimeBase> {
public:
  AVCodecCtxTimeBase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId,uint32_t NumPtr,uint32_t DenPtr);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge