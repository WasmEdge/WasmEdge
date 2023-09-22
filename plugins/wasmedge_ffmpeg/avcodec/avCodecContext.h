#include "avcodec_base.h"
#include "runtime/callingframe.h"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

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
  Expect<int32_t > body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId,uint32_t NumPtr,uint32_t DenPtr);
};

class AVCodecCtxWidth : public WasmEdgeFFmpegAVCodec<AVCodecCtxWidth> {
public:
  AVCodecCtxWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId);
};

class AVCodecCtxHeight : public WasmEdgeFFmpegAVCodec<AVCodecCtxHeight> {
public:
  AVCodecCtxHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId);
};

class AVCodecCtxSampleAspectRatio : public WasmEdgeFFmpegAVCodec<AVCodecCtxSampleAspectRatio> {
public:
  AVCodecCtxSampleAspectRatio(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId,uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxChannelLayout : public WasmEdgeFFmpegAVCodec<AVCodecCtxChannelLayout> {
public:
  AVCodecCtxChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,uint32_t AvCodecCtxId);
};
} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge