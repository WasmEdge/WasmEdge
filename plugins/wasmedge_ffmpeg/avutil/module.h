#pragma once

#include "runtime/instance/module.h"
#include "ffmpeg_env.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{
namespace AVUtil{

class WasmEdgeFFmpegAVUtilModule: public Runtime::Instance::ModuleInstance {
public:
    WasmEdgeFFmpegAVUtilModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

}
}
};
}

