// SPDX-License-Identifier: Apache-2.0
#include "ast/expression.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

/// Load to construct Expression node. See "include/ast/expression.h".
Expect<void> Expression::loadBinary(FileMgr &Mgr) {
  if (auto Res = loadInstrSeq(Mgr)) {
    Instrs = std::move(*Res);
  } else {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  return {};
}

} // namespace AST
} // namespace SSVM
