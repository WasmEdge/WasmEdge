#pragma once

#include "ffmpeg_env.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class WasmEdgeFFmpegAVFormatModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVFormatModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge