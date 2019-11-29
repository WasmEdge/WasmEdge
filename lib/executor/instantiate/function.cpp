// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/function.h"
#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Instantiate function instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::FunctionSection *FuncSec,
                              AST::CodeSection *CodeSec) {
  if (FuncSec == nullptr || CodeSec == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;

  /// Get the function type indices.
  auto &TypeIdxs = FuncSec->getContent();

  /// Iterate through code segments to make function instances.
  auto &CodeSegs = CodeSec->getContent();
  auto CodeSeg = CodeSegs.begin();
  auto TypeIdx = TypeIdxs.begin();
  while (CodeSeg != CodeSegs.end() && TypeIdx != TypeIdxs.end()) {
    /// Make a new function instance.
    auto NewFuncInst = std::make_unique<Instance::FunctionInstance>();
    unsigned int NewFuncInstId = 0;
    auto &Locals = (*CodeSeg)->getLocals();
    auto &Instrs = (*CodeSeg)->getInstrs();
    Instance::ModuleInstance::FType *FuncType = nullptr;

    /// Get function type pointer.
    if ((Status = ModInst->getFuncType(*TypeIdx, FuncType)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Set function instance data.
    if ((Status = NewFuncInst->setModuleAddr(ModInst->Addr)) !=
        ErrCode::Success) {
      return Status;
    }
    if ((Status = NewFuncInst->setFuncType(FuncType)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = NewFuncInst->setLocals(Locals)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = NewFuncInst->setInstrs(Instrs)) != ErrCode::Success) {
      return Status;
    }

    /// Insert function instance to store manager.
    if ((Status = StoreMgr.insertFunctionInst(NewFuncInst, NewFuncInstId)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Set external value (function address) to module instance.
    if ((Status = ModInst->addFuncAddr(NewFuncInstId)) != ErrCode::Success) {
      return Status;
    }

    CodeSeg++;
    TypeIdx++;
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
