// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate tag instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TagSection &TagSec) {

  // Iterate through tags from tag section to instantiate
  for (const auto &T : TagSec.getContent()) {
    // Add Tag with corresponding Type.
    auto TypePtr = *ModInst.getFuncType(T.getTypeIdx());
    ModInst.addTag(*TypePtr);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
