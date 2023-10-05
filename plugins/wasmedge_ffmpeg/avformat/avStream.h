#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVStreamId : public WasmEdgeFFmpegAVFormat<AVStreamId> {
public:
  AVStreamId(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamIndex : public WasmEdgeFFmpegAVFormat<AVStreamIndex> {
public:
  AVStreamIndex(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamCodecPar : public WasmEdgeFFmpegAVFormat<AVStreamCodecPar> {
public:
  AVStreamCodecPar(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t CodecParameterPtr);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
