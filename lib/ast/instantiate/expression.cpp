#include "ast/expression.h"

namespace AST {

/// Instantiation of expression. See "include/ast/expression.h".
Executor::ErrCode
Expression::instantiate(StoreMgr &Mgr,
                        std::unique_ptr<FunctionInstance> &FuncInst) {
  /// Instantiation will only move instructions to function instance.
  return FuncInst->setExpression(Inst);
}

} // namespace AST