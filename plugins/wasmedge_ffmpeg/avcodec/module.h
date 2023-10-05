#pragma once

#include "ffmpeg_env.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class WasmEdgeFFmpegAVCodecModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVCodecModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
