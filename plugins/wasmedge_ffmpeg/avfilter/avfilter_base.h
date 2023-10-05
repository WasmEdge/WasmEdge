#pragma once

#include "ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

template <typename T>
class WasmEdgeFFmpegAVFilter : public Runtime::HostFunction<T> {
public:
  WasmEdgeFFmpegAVFilter(
      std::shared_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  std::shared_ptr<WasmEdgeFFmpegEnv> Env;
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
