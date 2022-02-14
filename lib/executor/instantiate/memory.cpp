// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate memory instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::MemorySection &MemSec) {
  // Prepare pointers vector for compiled functions.
  ModInst.MemoryPtrs.resize(ModInst.getMemNum() + MemSec.getContent().size());

  // Iterate and istantiate memory types.
  for (const auto &MemType : MemSec.getContent()) {
    // Insert memory instance to store manager.
    uint32_t NewMemInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewMemInstAddr = StoreMgr.pushMemory(
          MemType, Conf.getRuntimeConfigure().getMaxMemoryPage());
    } else {
      NewMemInstAddr = StoreMgr.importMemory(
          MemType, Conf.getRuntimeConfigure().getMaxMemoryPage());
    }
    ModInst.addMemAddr(NewMemInstAddr);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
