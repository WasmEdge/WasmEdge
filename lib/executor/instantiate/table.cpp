// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate table instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TableSection &TabSec) {
  // Iterate and instantiate table types.
  for (const auto &TabType : TabSec.getContent()) {
    // Insert table instance to store manager.
    uint32_t NewTabInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewTabInstAddr = StoreMgr.pushTable(TabType);
    } else {
      NewTabInstAddr = StoreMgr.importTable(TabType);
    }
    ModInst.addTableAddr(NewTabInstAddr);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
