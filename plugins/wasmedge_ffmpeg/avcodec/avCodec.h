#pragma once
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

class AVCodecMaxLowres : public WasmEdgeFFmpegAVCodec<AVCodecMaxLowres> {
public:
  AVCodecMaxLowres(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecCapabilities : public WasmEdgeFFmpegAVCodec<AVCodecCapabilities> {
public:
  AVCodecCapabilities(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecGetNameLen : public WasmEdgeFFmpegAVCodec<AVCodecGetNameLen> {
public:
  AVCodecGetNameLen(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecGetName : public WasmEdgeFFmpegAVCodec<AVCodecGetName> {
public:
  AVCodecGetName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVCodecGetLongNameLen
    : public WasmEdgeFFmpegAVCodec<AVCodecGetLongNameLen> {
public:
  AVCodecGetLongNameLen(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecGetLongName : public WasmEdgeFFmpegAVCodec<AVCodecGetLongName> {
public:
  AVCodecGetLongName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t LongNamePtr, uint32_t LongNameLen);
};

class AVCodecProfiles : public WasmEdgeFFmpegAVCodec<AVCodecProfiles> {
public:
  AVCodecProfiles(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecPixFmtsIsNull
    : public WasmEdgeFFmpegAVCodec<AVCodecPixFmtsIsNull> {
public:
  AVCodecPixFmtsIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecPixFmtsIter : public WasmEdgeFFmpegAVCodec<AVCodecPixFmtsIter> {
public:
  AVCodecPixFmtsIter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                        uint32_t Idx);
};

class AVCodecSupportedFrameratesIsNull
    : public WasmEdgeFFmpegAVCodec<AVCodecSupportedFrameratesIsNull> {
public:
  AVCodecSupportedFrameratesIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecSupportedFrameratesIter
    : public WasmEdgeFFmpegAVCodec<AVCodecSupportedFrameratesIter> {
public:
  AVCodecSupportedFrameratesIter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t Idx, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecSupportedSampleRatesIsNull
    : public WasmEdgeFFmpegAVCodec<AVCodecSupportedSampleRatesIsNull> {
public:
  AVCodecSupportedSampleRatesIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecSupportedSampleRatesIter
    : public WasmEdgeFFmpegAVCodec<AVCodecSupportedSampleRatesIter> {
public:
  AVCodecSupportedSampleRatesIter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t Idx);
};

class AVCodecChannelLayoutIsNull
    : public WasmEdgeFFmpegAVCodec<AVCodecChannelLayoutIsNull> {
public:
  AVCodecChannelLayoutIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecChannelLayoutIter
    : public WasmEdgeFFmpegAVCodec<AVCodecChannelLayoutIter> {
public:
  AVCodecChannelLayoutIter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                        uint32_t Idx);
};

class AVCodecSampleFmtsIsNull
    : public WasmEdgeFFmpegAVCodec<AVCodecSampleFmtsIsNull> {
public:
  AVCodecSampleFmtsIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecSampleFmtsIter
    : public WasmEdgeFFmpegAVCodec<AVCodecSampleFmtsIter> {
public:
  AVCodecSampleFmtsIter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                        uint32_t Idx);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
