// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Executor {

// Instantiate function instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::FunctionSection &FuncSec,
                                   const AST::CodeSection &CodeSec) {

  // Get the function type indices.
  auto TypeIdxs = FuncSec.getContent();
  auto CodeSegs = CodeSec.getContent();

  if (CodeSegs.size() == 0) {
    return {};
  }
  // The module will always choose the `for` loop in `else` case under
  // interpreter mode. Instead, if we do branch in the `for` loop which might
  // cause meaningless branch misses. Therefore we should check the first item
  // and dispatch it into different cases to reduce branch misses.
  if (CodeSegs[0].getSymbol() != false) {
    for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
      auto Symbol = CodeSegs[I].getSymbol();
      ModInst.addFunc(
          TypeIdxs[I],
          (*ModInst.getType(TypeIdxs[I]))->getCompositeType().getFuncType(),
          std::move(Symbol));
    }
  } else {
    // Iterate through the code segments to instantiate function instances.
    for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
      // Create and add the function instance into the module instance.
      ModInst.addFunc(
          TypeIdxs[I],
          (*ModInst.getType(TypeIdxs[I]))->getCompositeType().getFuncType(),
          CodeSegs[I].getLocals(), CodeSegs[I].getExpr().getInstrs());
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
