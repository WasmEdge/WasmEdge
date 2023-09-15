#pragma once
#include "avformat_base.h"
#include "runtime/callingframe.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVInputFormatName : public WasmEdgeFFmpegAVFormat<AVInputFormatName> {
public:
    AVInputFormatName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};


class AVInputFormatLongName : public WasmEdgeFFmpegAVFormat<AVInputFormatLongName> {
public:
  AVInputFormatLongName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};

class AVInputFormatExtensions : public WasmEdgeFFmpegAVFormat<AVInputFormatExtensions> {
public:
  AVInputFormatExtensions(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};

class AVInputFormatMimeType : public WasmEdgeFFmpegAVFormat<AVInputFormatMimeType> {
public:
  AVInputFormatMimeType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
