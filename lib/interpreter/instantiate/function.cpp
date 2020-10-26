// SPDX-License-Identifier: Apache-2.0
#include "runtime/instance/function.h"
#include "ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate function instance. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiate(
    Runtime::StoreManager &StoreMgr, Runtime::Instance::ModuleInstance &ModInst,
    const AST::FunctionSection &FuncSec, const AST::CodeSection &CodeSec) {

  /// Get the function type indices.
  auto TypeIdxs = FuncSec.getContent();
  auto CodeSegs = CodeSec.getContent();

  /// Iterate through code segments to make function instances.
  for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
    /// Make a new function instance.
    auto *FuncType = *ModInst.getFuncType(TypeIdxs[I]);
    auto NewFuncInst = std::make_unique<Runtime::Instance::FunctionInstance>(
        ModInst.Addr, *FuncType, CodeSegs[I]->getLocals(),
        CodeSegs[I]->getInstrs());

    if (auto Symbol = CodeSegs[I]->getSymbol()) {
      NewFuncInst->setSymbol(std::move(Symbol));
    }

    /// Insert function instance to store manager.
    uint32_t NewFuncInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewFuncInstAddr = StoreMgr.pushFunction(std::move(NewFuncInst));
    } else {
      NewFuncInstAddr = StoreMgr.importFunction(std::move(NewFuncInst));
    }
    ModInst.addFuncAddr(NewFuncInstAddr);
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
