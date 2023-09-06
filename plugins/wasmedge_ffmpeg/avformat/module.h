#pragma once

#include "runtime/instance/module.h"
#include "ffmpeg_env.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{
namespace AVFormat{

class WasmEdgeFFmpegAVFormatModule: public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVFormatModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);


};

}
}
};
}

