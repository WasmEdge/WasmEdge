// SPDX-License-Identifier: Apache-2.0

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

/// Load to construct Expression node. See "include/loader/loader.h".
Expect<void> Loader::loadExpression(AST::Expression &Expr) {
  if (auto Res = loadInstrSeq()) {
    Expr.getInstrs() = std::move(*Res);
  } else {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
    return Unexpect(Res);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
