#pragma once

#include "ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

template <typename T>
class WasmEdgeFFmpegAVUtil : public Runtime::HostFunction<T> {
public:
  WasmEdgeFFmpegAVUtil(
      std::shared_ptr<WasmEdgeFFmpeg::WasmEdgeFFmpegEnv> HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  std::shared_ptr<WasmEdgeFFmpegEnv> Env;
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
