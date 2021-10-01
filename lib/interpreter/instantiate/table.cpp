// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

/// Instantiate table instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::TableSection &TabSec) {
  /// Iterate and instantiate table types.
  for (const auto &TabType : TabSec.getContent()) {
    /// Insert table instance to store manager.
    uint32_t NewTabInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewTabInstAddr = StoreMgr.pushTable(TabType.getInner());
    } else {
      NewTabInstAddr = StoreMgr.importTable(TabType.getInner());
    }
    ModInst.addTableAddr(NewTabInstAddr);
  }
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
