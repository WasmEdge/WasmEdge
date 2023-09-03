#pragma once

#include "runtime/instance/module.h"
#include "ffmpeg_env.h"

namespace WasmEdge{
namespace Host{

class WasmEdgeFFmpegAVFormatModule: public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVFormatModule(std::shared_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> Env);
};

};
}

