// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate tag instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TagSection &TagSec) {

  // Iterate through tags from tag section to instantiate
  for (const auto &TgType : TagSec.getContent()) {
    // Add Tag with corresponding Type.
    auto FuncTypePtr = *ModInst.getFuncType(TgType.getTypeIdx());
    ModInst.addTag(TgType, FuncTypePtr);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
