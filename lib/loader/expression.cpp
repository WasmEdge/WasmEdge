#include "loader/expression.h"

namespace AST {

/// Load binary to construct Expression node. See "include/loader/expression.h".
bool Expression::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  /// Read opcode until the End code.
  while (Mgr.readByte(Byte)) {
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);
    /// When reach end, this expression is ended.
    if (Code == Instruction::OpCode::End)
      return true;
    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst = makeInstructionNode(Code);
    if (NewInst == nullptr || !NewInst->loadBinary(Mgr))
      return false;
    Inst.push_back(std::move(NewInst));
  }
  return false;
}

} // namespace AST