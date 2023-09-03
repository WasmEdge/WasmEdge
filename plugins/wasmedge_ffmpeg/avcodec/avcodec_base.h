#pragma once

#include "../ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasmEdgeFFmpegAVCodec : public Runtime::HostFunction<T> {
public:
  WasmEdgeFFmpegAVCodec(WasmEdgeFFmpeg::WasmEdgeFFmpegEnv &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeFFmpeg::WasmEdgeFFmpegEnv &Env;
};

} // namespace Host
} // namespace WasmEdge
