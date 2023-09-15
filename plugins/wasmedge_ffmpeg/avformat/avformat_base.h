#pragma once

#include "ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

template <typename T>
class WasmEdgeFFmpegAVFormat : public Runtime::HostFunction<T> {
public:
  WasmEdgeFFmpegAVFormat(
      std::shared_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  std::shared_ptr<WasmEdgeFFmpegEnv> Env;
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
