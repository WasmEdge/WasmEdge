#include "loader/expression.h"

namespace AST {

/// Load binary to construct Expression node. See "include/loader/expression.h".
Base::ErrCode Expression::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read opcode until the End code.
  while (Status == Base::ErrCode::Success) {
    /// Read the opcode and check if error.
    if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
        Base::ErrCode::Success)
      break;
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);

    /// When reach end, this expression is ended.
    if (Code == Instruction::OpCode::End)
      break;

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if ((Status = makeInstructionNode(Code, NewInst)) != Base::ErrCode::Success)
      break;
    if ((Status = NewInst->loadBinary(Mgr)) != Base::ErrCode::Success)
      break;
    Inst.push_back(std::move(NewInst));
  }
  return Status;
}

} // namespace AST