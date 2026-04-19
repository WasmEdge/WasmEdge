// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/engine/const_fold.h"
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
  // Under interpreter mode, the module always chooses the `for` loop in the
  // `else` case. Branching in the `for` loop might cause meaningless branch
  // misses, so check the first item and dispatch it into different cases to
  // reduce branch misses.
  if (CodeSegs[0].getSymbol() != false) {
    for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
      auto Symbol = CodeSegs[I].getSymbol();
      ModInst.addFunc(
          TypeIdxs[I],
          (*ModInst.getType(TypeIdxs[I]))->getCompositeType().getFuncType(),
          std::move(Symbol));
    }
  } else {
    // At O0, skip constant folding to preserve debuggability.
    const bool DoFold = Conf.getCompilerConfigure().getOptimizationLevel() !=
                        CompilerConfigure::OptimizationLevel::O0;
    for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
      auto Src = CodeSegs[I].getExpr().getInstrs();
      AST::InstrVec OptInstrs(Src.begin(), Src.end());
      if (DoFold) {
        optimizeConstantExpressions(OptInstrs);
      }
      ModInst.addFunc(
          TypeIdxs[I],
          (*ModInst.getType(TypeIdxs[I]))->getCompositeType().getFuncType(),
          CodeSegs[I].getLocals(), std::move(OptInstrs));
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
