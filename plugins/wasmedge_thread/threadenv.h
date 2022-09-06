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

  void init(WasmEdge::VM::VM *VM);

  void fini() noexcept;

  Expect<void> pthreadCreate(uint64_t *WasiThreadPtr, uint32_t WasiThreadFunc,
                             uint32_t Arg) const;
  Expect<void> pthreadJoin(uint64_t WasiThread, void **WasiRetval) const;

  static Plugin::PluginRegister Register;

private:
  WasmEdge::VM::VM *VM = nullptr;
};

} // namespace Host
} // namespace WasmEdge
