// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "runtime/instance/memory.h"
#include "threadbase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeThreadCreate : public WasmEdgeThread<WasmEdgeThreadCreate> {
public:
  WasmEdgeThreadCreate(WasmEdgeThreadEnvironment &HostEnv)
      : WasmEdgeThread(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Thread,
                    uint32_t Attr, uint32_t StartRoutine, uint32_t Arg);
};

class WasmEdgeThreadJoin : public WasmEdgeThread<WasmEdgeThreadJoin> {
public:
  WasmEdgeThreadJoin(WasmEdgeThreadEnvironment &HostEnv)
      : WasmEdgeThread(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Thread,
                    uint32_t Retval);
};

} // namespace Host
} // namespace WasmEdge
