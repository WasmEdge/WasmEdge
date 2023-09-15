#pragma once
#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVIOClose : public WasmEdgeFFmpegAVFormat<AVIOClose> {
public:
    AVIOClose(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t AvFormatCtxId);
};


} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge

