#pragma once

#include "ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

template <typename T>
class WasmEdgeFFmpegSWResample : public Runtime::HostFunction<T> {
public:
  WasmEdgeFFmpegSWResample(
      std::shared_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  std::shared_ptr<WasmEdgeFFmpegEnv> Env;
};

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
