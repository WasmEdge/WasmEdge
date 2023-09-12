#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecAllocContext3 : public WasmEdgeFFmpegAVCodec<AVCodecAllocContext3> {
public:
  AVCodecAllocContext3(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t avCodecPtr,uint32_t avCodecCtxPtr);
};

class AVCodecParametersFromContext : public WasmEdgeFFmpegAVCodec<AVCodecParametersFromContext> {
public:
  AVCodecParametersFromContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t avCodecCtxPtr,uint32_t avCodecParamPtr);
};

class AVCodecParametersFree : public WasmEdgeFFmpegAVCodec<AVCodecParametersFree> {
public:
  AVCodecParametersFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecParamPtr);
};

class AVCodecFreeContext : public WasmEdgeFFmpegAVCodec<AVCodecFreeContext> {
public:
  AVCodecFreeContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecCtxPtr);
};

class AVCodecParametersAlloc : public WasmEdgeFFmpegAVCodec<AVCodecParametersAlloc> {
public:
  AVCodecParametersAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecParamPtr);
};

class AVCodecGetType : public WasmEdgeFFmpegAVCodec<AVCodecGetType> {
public:
  AVCodecGetType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId);
};


class AVCodecOpen2 : public WasmEdgeFFmpegAVCodec<AVCodecOpen2> {
public:
  AVCodecOpen2(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecCtxId,uint32_t avCodecId,uint32_t avDictionaryId);
};

class AVCodecFindDecoder : public WasmEdgeFFmpegAVCodec<AVCodecFindDecoder> {
public:
  AVCodecFindDecoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecIDIndex,uint32_t avCodecId);
};

class AVCodecIsEncoder : public WasmEdgeFFmpegAVCodec<AVCodecIsEncoder> {
public:
  AVCodecIsEncoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId);
};

class AVCodecIsDecoder : public WasmEdgeFFmpegAVCodec<AVCodecIsDecoder> {
public:
  AVCodecIsDecoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId);
};

class AVCodecClose : public WasmEdgeFFmpegAVCodec<AVCodecClose> {
public:
  AVCodecClose(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId);
};

class AVCodecParametersToContext : public WasmEdgeFFmpegAVCodec<AVCodecParametersToContext> {
public:
  AVCodecParametersToContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t avCodecId,uint32_t avCodecParamId);
};

}
}
}
}
