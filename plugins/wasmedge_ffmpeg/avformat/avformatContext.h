#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {


class AVFormatCtxIFormat : public WasmEdgeFFmpegAVFormat<AVFormatCtxIFormat> {
public:
    AVFormatCtxIFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avInputFormatPtr);
};

}
}
}
}
