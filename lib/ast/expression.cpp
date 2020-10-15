// SPDX-License-Identifier: Apache-2.0
#include "common/ast/expression.h"
#include "support/log.h"

namespace SSVM {
namespace AST {

/// Load to construct Expression node. See "include/common/ast/expression.h".
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
