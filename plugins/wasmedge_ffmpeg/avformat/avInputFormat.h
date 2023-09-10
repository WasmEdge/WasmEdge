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

class AVInputFormat_name : public WasmEdgeFFmpegAVFormat<AVInputFormat_name> {
public:
    AVInputFormat_name(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};


class AVInputFormat_long_name : public WasmEdgeFFmpegAVFormat<AVInputFormat_long_name> {
public:
    AVInputFormat_long_name(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};

class AVInputFormat_extensions : public WasmEdgeFFmpegAVFormat<AVInputFormat_extensions> {
public:
    AVInputFormat_extensions(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};

class AVInputFormat_mime_type : public WasmEdgeFFmpegAVFormat<AVInputFormat_mime_type> {
public:
    AVInputFormat_mime_type(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr);
};

}
}
}
}
