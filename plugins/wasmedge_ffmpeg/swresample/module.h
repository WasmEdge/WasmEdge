#pragma once

#include "ffmpeg_env.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

class WasmEdgeFFmpegSWResampleModule
    : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegSWResampleModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
