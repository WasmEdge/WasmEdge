// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVDevice {

class WasmEdgeFFmpegAVDeviceModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVDeviceModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace AVDevice
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
