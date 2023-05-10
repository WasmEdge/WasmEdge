// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "base.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasiThreadSpawn : public WasiThreads<WasiThreadSpawn> {
public:
  WasiThreadSpawn(WasiThreadsEnvironment &HostEnv) : WasiThreads(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t ThreadStartArg);
};

} // namespace Host
} // namespace WasmEdge
