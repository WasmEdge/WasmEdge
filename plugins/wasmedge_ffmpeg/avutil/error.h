#include "avutil_base.h"
#include "runtime/callingframe.h"
#pragma once

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil{

class AVUtilAVStrError : public WasmEdgeFFmpegAVUtil<AVUtilAVStrError> {
public:
    AVUtilAVStrError(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVUtil(HostEnv) {}
    Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t errnum,uint32_t errbuf,uint32_t bufLen);
};

class AVUtilAVError : public WasmEdgeFFmpegAVUtil<AVUtilAVError> {
public:
    AVUtilAVError(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVUtil(HostEnv) {}
    Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t errnum);
};

class AVUtilAVUNError : public WasmEdgeFFmpegAVUtil<AVUtilAVUNError> {
public:
    AVUtilAVUNError(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVUtil(HostEnv) {}
    Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t errnum);
};

}
}
}
}
