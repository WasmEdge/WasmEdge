#include "avcodec_base.h"
#include "runtime/callingframe.h"


namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecParam_codecId : public WasmEdgeFFmpegAVCodec<AVCodecParam_codecId> {
public:
  AVCodecParam_codecId(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecParamId);
};

class AVCodecParam_codecType : public WasmEdgeFFmpegAVCodec<AVCodecParam_codecType> {
public:
  AVCodecParam_codecType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecParamId);
};


}
}
}
}
