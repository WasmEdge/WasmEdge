// SPDX-License-Identifier: Apache-2.0

#include "ast/expression.h"

namespace WasmEdge {
namespace AST {

/// Load to construct Expression node. See "include/ast/expression.h".
Expect<void> Expression::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  if (auto Res = loadInstrSeq(Mgr, Conf)) {
    Instrs = std::move(*Res);
  } else {
    spdlog::error(ErrInfo::InfoAST(NodeAttr));
    return Unexpect(Res);
  }
  return {};
}

} // namespace AST
} // namespace WasmEdge
