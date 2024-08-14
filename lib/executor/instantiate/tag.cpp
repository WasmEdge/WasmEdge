// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate tag instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TagSection &TagSec) {
  // Iterate through the tags to instantiate the tag instances.
  for (const auto &TgType : TagSec.getContent()) {
    // Add Tag with corresponding Type.
    auto SubTypePtr = *ModInst.getType(TgType.getTypeIdx());
    ModInst.addTag(TgType, SubTypePtr);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
