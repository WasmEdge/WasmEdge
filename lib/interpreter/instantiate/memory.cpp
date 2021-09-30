// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

/// Instantiate memory instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::MemorySection &MemSec) {
  /// Iterate and istantiate memory types.
  for (const auto &MemType : MemSec.getContent()) {
    /// Insert memory instance to store manager.
    uint32_t NewMemInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewMemInstAddr = StoreMgr.pushMemory(
          MemType.getLimit(), Conf.getRuntimeConfigure().getMaxMemoryPage());
    } else {
      NewMemInstAddr = StoreMgr.importMemory(
          MemType.getLimit(), Conf.getRuntimeConfigure().getMaxMemoryPage());
    }
    ModInst.addMemAddr(NewMemInstAddr);
  }
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
