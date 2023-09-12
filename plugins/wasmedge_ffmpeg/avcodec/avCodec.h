#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodec_ID : public WasmEdgeFFmpegAVCodec<AVCodec_ID> {
public:
  AVCodec_ID(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId);
};

class AVCodec_Type : public WasmEdgeFFmpegAVCodec<AVCodec_Type> {
public:
  AVCodec_Type(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId);
};

}
}
}
}


