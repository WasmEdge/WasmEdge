// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class WasmEdgeFFmpegAVFilterModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeFFmpegAVFilterModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
