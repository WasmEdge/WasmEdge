// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

// Load to construct Expression node. See "include/loader/loader.h".
Expect<void> Loader::loadExpression(AST::Expression &Expr,
                                    std::optional<uint64_t> SizeBound) {
  return loadInstrSeq(SizeBound)
      .map_error([](auto E) {
        // For the section size mismatch case, check in caller.
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
        return E;
      })
      .and_then([&](auto Instrs) {
        Expr.getInstrs() = Instrs;
        return Expect<void>{};
      });
}

} // namespace Loader
} // namespace WasmEdge
