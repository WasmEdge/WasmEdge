#pragma once
#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVLogSetLevel : public WasmEdgeFFmpegAVUtil<AVLogSetLevel> {
public:
  AVLogSetLevel(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t LogLevelId);
};

class AVLogGetLevel : public WasmEdgeFFmpegAVUtil<AVLogGetLevel> {
public:
  AVLogGetLevel(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVLogSetFlags : public WasmEdgeFFmpegAVUtil<AVLogSetFlags> {
public:
  AVLogSetFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t FlagsId);
};

class AVLogGetFlags : public WasmEdgeFFmpegAVUtil<AVLogGetFlags> {
public:
  AVLogGetFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
