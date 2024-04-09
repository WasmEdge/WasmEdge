#pragma once
#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecParamCodecId : public WasmEdgeFFmpegAVCodec<AVCodecParamCodecId> {
public:
  AVCodecParamCodecId(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecParamId);
};

class AVCodecParamCodecType
    : public WasmEdgeFFmpegAVCodec<AVCodecParamCodecType> {
public:
  AVCodecParamCodecType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamId);
};

class AVCodecParamSetCodecTag
    : public WasmEdgeFFmpegAVCodec<AVCodecParamSetCodecTag> {
public:
  AVCodecParamSetCodecTag(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecParamId, uint32_t CodecTag);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
