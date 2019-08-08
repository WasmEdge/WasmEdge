#include "ast/expression.h"

namespace SSVM {
namespace AST {

/// Instantiation of expression. See "include/ast/expression.h".
template <typename T>
Executor::ErrCode Expression::instantiate(StoreMgr &Mgr,
                                          std::unique_ptr<T> &Instance) {
  /// Instantiation will only move instructions to instance.
  return Instance->setExpression(Instr);
}

} // namespace AST
} // namespace SSVM