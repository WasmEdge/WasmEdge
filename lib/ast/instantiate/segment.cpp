#include "ast/segment.h"

namespace AST {

/// Instantiation of code segment. See "include/ast/segment.h".
Executor::ErrCode
CodeSegment::instantiate(StoreMgr &Mgr,
                         std::unique_ptr<FunctionInstance> &FuncInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;
  /// Instantiation will only move locals and expression to function instance.
  if ((Status = FuncInst->setLocals(Locals)) != Executor::ErrCode::Success)
    return Status;
  return Expr->instantiate(Mgr, FuncInst);
}

} // namespace AST