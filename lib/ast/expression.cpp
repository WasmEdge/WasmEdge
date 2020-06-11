// SPDX-License-Identifier: Apache-2.0
#include "common/ast/expression.h"
#include "support/log.h"

namespace SSVM {
namespace AST {

/// Load to construct Expression node. See "include/common/ast/expression.h".
Expect<void> Expression::loadBinary(FileMgr &Mgr) {
  /// Read opcode until the End code.
  while (true) {
    OpCode Code;
    uint32_t Offset = Mgr.getOffset();

    /// Read the opcode and check if error.
    if (auto Res = Mgr.readByte()) {
      Code = static_cast<OpCode>(*Res);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }

    /// When reach end, this expression is ended.
    if (Code == OpCode::End)
      break;

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if (auto Res = makeInstructionNode(Code, Offset)) {
      NewInst = std::move(*Res);
    } else {
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    if (auto Res = NewInst->loadBinary(Mgr)) {
      Instrs.push_back(std::move(NewInst));
    } else {
      return Unexpect(Res);
    }
  }

  return {};
}

} // namespace AST
} // namespace SSVM
