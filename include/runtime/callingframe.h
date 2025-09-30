// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/callingframe.h - Calling frame definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of CallingFrame class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/module.h"

namespace WasmEdge {

namespace Executor {
class Executor;
}

namespace Runtime {

class CallingFrame {
public:
  CallingFrame(Executor::Executor *E,
               const Instance::ModuleInstance *M) noexcept
      : Exec(E), Module(M) {}

  /// Get the current executor.
  Executor::Executor *getExecutor() const noexcept { return Exec; }

  /// Get the current module on this frame.
  const Instance::ModuleInstance *getModule() const noexcept { return Module; }

  /// Helper function of getting the WASI module.
  const Instance::ModuleInstance *getWASIModule() const noexcept {
    if (Module) {
      return Module->getWASIModule();
    }
    return nullptr;
  }

  /// Helper function of getting the memory instance by index from the module.
  Instance::MemoryInstance *getMemoryByIndex(uint32_t Index) const noexcept {
    if (Module) {
      if (auto Res = Module->getMemory(Index); Res) {
        return *Res;
      }
    }
    return nullptr;
  }

private:
  Executor::Executor *Exec;
  const Instance::ModuleInstance *Module;
};

} // namespace Runtime
} // namespace WasmEdge
