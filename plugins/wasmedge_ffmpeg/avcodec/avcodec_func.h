#pragma once
#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecAllocContext3
    : public WasmEdgeFFmpegAVCodec<AVCodecAllocContext3> {
public:
  AVCodecAllocContext3(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t AvCodecPtr);
};

class AVCodecParametersFromContext
    : public WasmEdgeFFmpegAVCodec<AVCodecParametersFromContext> {
public:
  AVCodecParametersFromContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t AvCodecParamId);
};

class AVCodecParametersFree
    : public WasmEdgeFFmpegAVCodec<AVCodecParametersFree> {
public:
  AVCodecParametersFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamId);
};

class AVCodecFreeContext : public WasmEdgeFFmpegAVCodec<AVCodecFreeContext> {
public:
  AVCodecFreeContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecParametersAlloc
    : public WasmEdgeFFmpegAVCodec<AVCodecParametersAlloc> {
public:
  AVCodecParametersAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamPtr);
};

class AVCodecGetType : public WasmEdgeFFmpegAVCodec<AVCodecGetType> {
public:
  AVCodecGetType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecOpen2 : public WasmEdgeFFmpegAVCodec<AVCodecOpen2> {
public:
  AVCodecOpen2(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t AvCodecId,
                       uint32_t AvDictionaryId);
};

class AVCodecFindDecoder : public WasmEdgeFFmpegAVCodec<AVCodecFindDecoder> {
public:
  AVCodecFindDecoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ID,
                       uint32_t AvCodecId);
};

class AVCodecIsEncoder : public WasmEdgeFFmpegAVCodec<AVCodecIsEncoder> {
public:
  AVCodecIsEncoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecIsDecoder : public WasmEdgeFFmpegAVCodec<AVCodecIsDecoder> {
public:
  AVCodecIsDecoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecClose : public WasmEdgeFFmpegAVCodec<AVCodecClose> {
public:
  AVCodecClose(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecParametersToContext
    : public WasmEdgeFFmpegAVCodec<AVCodecParametersToContext> {
public:
  AVCodecParametersToContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t avCodecId,
                       uint32_t avCodecParamId);
};

class AVCodecReceiveFrame : public WasmEdgeFFmpegAVCodec<AVCodecReceiveFrame> {
public:
  AVCodecReceiveFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t FrameId);
};

class AVCodecSendPacket : public WasmEdgeFFmpegAVCodec<AVCodecSendPacket> {
public:
  AVCodecSendPacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t PacketId);
};

class AVCodecFindEncoder : public WasmEdgeFFmpegAVCodec<AVCodecFindEncoder> {
public:
  AVCodecFindEncoder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ID,
                       uint32_t AVCodecPtr);
};

class AVCodecReceivePacket
    : public WasmEdgeFFmpegAVCodec<AVCodecReceivePacket> {
public:
  AVCodecReceivePacket(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVCodecCtxId, uint32_t PacketId);
};

class AVCodecSendFrame : public WasmEdgeFFmpegAVCodec<AVCodecSendFrame> {
public:
  AVCodecSendFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVCodecCtxId, uint32_t FrameId);
};

class AVCodecFindDecoderByName
    : public WasmEdgeFFmpegAVCodec<AVCodecFindDecoderByName> {
public:
  AVCodecFindDecoderByName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVCodecPtr,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVCodecFindEncoderByName
    : public WasmEdgeFFmpegAVCodec<AVCodecFindEncoderByName> {
public:
  AVCodecFindEncoderByName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVCodecPtr,
                       uint32_t NamePtr, uint32_t NameLen);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
