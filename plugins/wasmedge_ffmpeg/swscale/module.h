#pragma once

#include "runtime/instance/module.h"
#include "ffmpeg_env.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale{

class WasmEdgeFFmpegSWScaleModule: public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegSWScaleModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
