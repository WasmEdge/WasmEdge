// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate table instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TableSection &TabSec) {
  // Iterate through the table types to instantiate table instance.
  for (const auto &TabType : TabSec.getContent()) {
    // Create and add the table instance into the module instance.
    ModInst.addTable(TabType);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
