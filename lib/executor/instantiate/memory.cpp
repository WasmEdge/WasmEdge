// SPDX-License-Identifier: Apache-2.0

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

/// Instantiate memory instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::MemorySection &MemSec) {
  /// Iterate and istantiate memory types.
  for (const auto &MemType : MemSec.getContent()) {
    /// Insert memory instance to store manager.
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
