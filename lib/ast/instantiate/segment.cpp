#include "ast/segment.h"

namespace SSVM {
namespace AST {

/// Instantiation of global segment. See "include/ast/segment.h".
Executor::ErrCode
GlobalSegment::instantiate(Executor::StoreManager &Mgr,
                           std::unique_ptr<Executor::GlobalInstance> &GlobInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;
  /// Instantiation sets global type and moves expression to function instance.
  if ((Status = Global->instantiate(Mgr, GlobInst)) !=
      Executor::ErrCode::Success)
    return Status;
  return Expr->instantiate(Mgr, GlobInst);
}

/// Instantiation of code segment. See "include/ast/segment.h".
Executor::ErrCode
CodeSegment::instantiate(Executor::StoreManager &Mgr,
                         std::unique_ptr<Executor::FunctionInstance> &FuncInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;
  /// Instantiation will only move locals and expression to function instance.
  if ((Status = FuncInst->setLocals(Locals)) != Executor::ErrCode::Success)
    return Status;
  return Expr->instantiate(Mgr, FuncInst);
}

} // namespace AST
} // namespace SSVM
