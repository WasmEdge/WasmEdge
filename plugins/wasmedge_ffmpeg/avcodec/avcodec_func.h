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
                       uint32_t AvCodecCtxPtr);
};

class AVCodecParametersFromContext
    : public WasmEdgeFFmpegAVCodec<AVCodecParametersFromContext> {
public:
  AVCodecParametersFromContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamId, uint32_t AvCodecCtxId);
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
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t AvCodecParamId);
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

class AVPacketRescaleTs : public WasmEdgeFFmpegAVCodec<AVPacketRescaleTs> {
public:
  AVPacketRescaleTs(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVPacketId,
                       int32_t SrcNum, int32_t SrcDen, int32_t DestNum,
                       int32_t DestDen);
};

class AVPacketMakeWritable
    : public WasmEdgeFFmpegAVCodec<AVPacketMakeWritable> {
public:
  AVPacketMakeWritable(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AVPacketId);
};

class AVCodecParametersCopy
    : public WasmEdgeFFmpegAVCodec<AVCodecParametersCopy> {
public:
  AVCodecParametersCopy(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFormatCtxId, uint32_t AVCodecParamId,
                       uint32_t StreamIdx);
};

class AVCodecVersion : public WasmEdgeFFmpegAVCodec<AVCodecVersion> {
public:
  AVCodecVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVCodecFlushBuffers : public WasmEdgeFFmpegAVCodec<AVCodecFlushBuffers> {
public:
  AVCodecFlushBuffers(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVCodecCtxId);
};

class AVCodecConfigurationLength
    : public WasmEdgeFFmpegAVCodec<AVCodecConfigurationLength> {
public:
  AVCodecConfigurationLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVCodecConfiguration
    : public WasmEdgeFFmpegAVCodec<AVCodecConfiguration> {
public:
  AVCodecConfiguration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVCodecLicenseLength
    : public WasmEdgeFFmpegAVCodec<AVCodecLicenseLength> {
public:
  AVCodecLicenseLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVCodecLicense : public WasmEdgeFFmpegAVCodec<AVCodecLicense> {
public:
  AVCodecLicense(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
