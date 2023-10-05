#pragma once

#include "ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale {

template <typename T>
class WasmEdgeFFmpegSWScale : public Runtime::HostFunction<T> {
public:
  WasmEdgeFFmpegSWScale(
      std::shared_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  std::shared_ptr<WasmEdgeFFmpegEnv> Env;
};

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
