#pragma once

#include "runtime/instance/module.h"
#include "ffmpeg_env.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVCodec {

class WasmEdgeFFmpegAVCodecModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVCodecModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

}
}
};
}

