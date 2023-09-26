#pragma once

#include "runtime/instance/module.h"
#include "ffmpeg_env.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample{

class WasmEdgeFFmpegSWResampleModule: public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegSWResampleModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
