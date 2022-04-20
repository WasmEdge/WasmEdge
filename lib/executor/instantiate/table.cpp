// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate table instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TableSection &TabSec) {
  // Iterate and instantiate table types.
  for (const auto &TabType : TabSec.getContent()) {
    // Insert table instance to store manager.
    Runtime::Instance::TableInstance *TabInst = nullptr;
    if (InsMode == InstantiateMode::Instantiate) {
      TabInst = StoreMgr.pushTable(TabType);
    } else {
      TabInst = StoreMgr.importTable(TabType);
    }
    ModInst.addTable(TabInst);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
