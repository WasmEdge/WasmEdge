// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Executor {

// Instantiate function instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::FunctionSection &FuncSec,
                                   const AST::CodeSection &CodeSec) {

  // Get the function type indices.
  auto TypeIdxs = FuncSec.getContent();
  auto CodeSegs = CodeSec.getContent();

  // Iterate through code segments to make function instances.
  for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
    // Insert function instance to store manager.
    Runtime::Instance::FunctionInstance *FuncInst = nullptr;
    auto *FuncType = *ModInst.getFuncType(TypeIdxs[I]);
    if (InsMode == InstantiateMode::Instantiate) {
      if (auto Symbol = CodeSegs[I].getSymbol()) {
        FuncInst =
            StoreMgr.pushFunction(&ModInst, *FuncType, std::move(Symbol));
      } else {
        FuncInst =
            StoreMgr.pushFunction(&ModInst, *FuncType, CodeSegs[I].getLocals(),
                                  CodeSegs[I].getExpr().getInstrs());
      }
    } else {
      if (auto Symbol = CodeSegs[I].getSymbol()) {
        FuncInst =
            StoreMgr.importFunction(&ModInst, *FuncType, std::move(Symbol));
      } else {
        FuncInst = StoreMgr.importFunction(&ModInst, *FuncType,
                                           CodeSegs[I].getLocals(),
                                           CodeSegs[I].getExpr().getInstrs());
      }
    }
    ModInst.addFunc(FuncInst);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
