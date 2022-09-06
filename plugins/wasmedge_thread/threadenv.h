// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "vm/vm.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {

// enum class ErrNo : uint32_t {
//   Success = 0,         // No error occurred.
//   InvalidArgument = 1, // Caller module passed an invalid argument.
//   InvalidPointer = 2,  // An invalid pointer is given
// };

class WasmEdgeThreadEnvironment {
public:
  WasmEdgeThreadEnvironment() noexcept {}
  ~WasmEdgeThreadEnvironment() noexcept {}

  Expect<void> pthreadCreate(Executor::Executor *Exec, uint64_t *WasiThreadPtr,
                             uint32_t WasiThreadFunc, uint32_t Arg) const;
  Expect<void> pthreadJoin(uint64_t WasiThread, void **WasiRetval) const;

  static Plugin::PluginRegister Register;

private:
};

} // namespace Host
} // namespace WasmEdge
