#include "ast/instruction.h"
#include "executor/worker.h"

namespace SSVM {
namespace AST {

/// Load binary of block instructions. See "include/ast/instruction.h".
Loader::ErrCode BlockControlInstruction::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the block return type.
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  BlockType = static_cast<ValType>(Byte);

  /// Read instructions and make nodes until Opcode::End.
  while (Status == Loader::ErrCode::Success) {
    /// Read the opcode and check if error.
    if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
      break;
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);

    /// When reach end, this block is ended.
    if (Code == Instruction::OpCode::End)
      break;

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if ((Status = makeInstructionNode(Code, NewInst)) !=
        Loader::ErrCode::Success)
      break;
    if ((Status = NewInst->loadBinary(Mgr)) != Loader::ErrCode::Success)
      break;
    Body.push_back(std::move(NewInst));
  }
  return Status;
}

/// Load binary of if-else instructions. See "include/ast/instruction.h".
Loader::ErrCode IfElseControlInstruction::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the block return type.
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  BlockType = static_cast<ValType>(Byte);

  /// Read instructions and make nodes until OpCode::End.
  bool IsElseStatement = false;
  while (Status == Loader::ErrCode::Success) {
    /// Read the opcode and check if error.
    if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
      break;
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);

    /// When reach end, this if-else block is ended.
    if (Code == Instruction::OpCode::End)
      break;

    /// If an OpCode::Else read, switch to Else statement.
    if (Code == Instruction::OpCode::Else) {
      IsElseStatement = true;
      continue;
    }

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if ((Status = makeInstructionNode(Code, NewInst)) !=
        Loader::ErrCode::Success)
      break;
    if ((Status = NewInst->loadBinary(Mgr)) != Loader::ErrCode::Success)
      break;
    if (IsElseStatement)
      ElseStatement.push_back(std::move(NewInst));
    else
      IfStatement.push_back(std::move(NewInst));
  }
  return Status;
}

/// Load binary of branch instructions. See "include/ast/instruction.h".
Loader::ErrCode BrControlInstruction::loadBinary(FileMgr &Mgr) {
  return Mgr.readU32(LabelIdx);
}

/// Load binary of branch table instructions. See
/// "include/loader/instruction.h".
Loader::ErrCode BrTableControlInstruction::loadBinary(FileMgr &Mgr) {
  unsigned int VecCnt = 0;
  unsigned int Idx = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the vector of labels.
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    if ((Status = Mgr.readU32(Idx)) != Loader::ErrCode::Success)
      return Status;
    LabelTable.push_back(Idx);
  }

  /// Read default label.
  return Mgr.readU32(LabelIdx);
}

/// Load binary of call instructions. See "include/ast/instruction.h".
Loader::ErrCode CallControlInstruction::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read function index.
  if ((Status = Mgr.readU32(FuncIdx)) != Loader::ErrCode::Success)
    return Status;

  /// Read the 0x00 checking code in indirect_call case.
  if (Code == Instruction::OpCode::Call_indirect) {
    unsigned char Byte = 0xFF;
    Status = Mgr.readByte(Byte);
    if (Status == Loader::ErrCode::Success && Byte != 0x00)
      Status = Loader::ErrCode::InvalidGrammar;
  }
  return Status;
}

/// Load binary of variable instructions. See "include/ast/instruction.h".
Loader::ErrCode VariableInstruction::loadBinary(FileMgr &Mgr) {
  return Mgr.readU32(VarIdx);
}

/// Load binary of memory instructions. See "include/ast/instruction.h".
Loader::ErrCode MemoryInstruction::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the 0x00 checking code in memory.grow and memory.size cases.
  unsigned char Byte = 0xFF;
  if (Code == Instruction::OpCode::Memory__grow ||
      Code == Instruction::OpCode::Memory__size) {
    Status = Mgr.readByte(Byte);
    if (Status == Loader::ErrCode::Success && Byte == 0x00)
      return Status;
  }

  /// Read memory arguments.
  if (Status == Loader::ErrCode::Success)
    Status = Mgr.readU32(Align);
  if (Status == Loader::ErrCode::Success)
    Status = Mgr.readU32(Offset);
  return Status;
}

/// Load binary of const numeric instructions. See "include/ast/instruction.h".
Loader::ErrCode ConstInstruction::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the const number of corresbonding value type.
  switch (Code) {
  case Instruction::OpCode::I32__const: {
    int32_t Val = 0;
    Status = Mgr.readS32(Val);
    Num = static_cast<uint32_t>(Val);
    break;
  }
  case Instruction::OpCode::I64__const: {
    int64_t Val = 0;
    Status = Mgr.readS64(Val);
    Num = static_cast<uint64_t>(Val);
    break;
  }
  case Instruction::OpCode::F32__const: {
    float Val = 0;
    Status = Mgr.readF32(Val);
    Num = Val;
    break;
  }
  case Instruction::OpCode::F64__const: {
    double Val = 0;
    Status = Mgr.readF64(Val);
    Num = Val;
    break;
  }
  default:
    Status = Loader::ErrCode::InvalidGrammar;
    break;
  }
  return Status;
}

/// Instruction node maker. See "include/ast/instruction.h".
Loader::ErrCode makeInstructionNode(Instruction::OpCode Code,
                                    std::unique_ptr<Instruction> &NewInst) {
  return dispatchInstruction(Code, [&NewInst, &Code](auto &&Arg) {
    if constexpr (std::is_void_v<typename std::decay_t<decltype(Arg)>::type>) {
      /// If the Code not matched, return null pointer.
      return Loader::ErrCode::InvalidGrammar;
    } else {
      /// Make the instruction node according to Code.
      NewInst = std::make_unique<typename std::decay_t<decltype(Arg)>::type>(Code);
      return Loader::ErrCode::Success;
    }
  });
}

} // namespace AST
} // namespace SSVM
