// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate memory instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::MemorySection &MemSec) {
  // Prepare pointers vector for compiled functions.
  ModInst.MemoryPtrs.resize(ModInst.getMemoryNum() +
                            MemSec.getContent().size());

  // Iterate through the memory types to instantiate memory instances.
  for (const auto &MemType : MemSec.getContent()) {
    // Create and add the memory instance into the module instance.
    ModInst.addMemory(MemType, Conf.getRuntimeConfigure().getMaxMemoryPage());
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
