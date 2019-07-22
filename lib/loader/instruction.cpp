#include "loader/instruction.h"

namespace AST {

/// Load binary of block instructions. See "include/loader/instruction.h".
Base::ErrCode BlockControlInstruction::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the block return type.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
      Base::ErrCode::Success)
    return Status;
  BlockType = static_cast<Base::ValType>(Byte);

  /// Read instructions and make nodes until Opcode::End.
  while (Status == Base::ErrCode::Success) {
    /// Read the opcode and check if error.
    if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
        Base::ErrCode::Success)
      break;
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);

    /// When reach end, this block is ended.
    if (Code == Instruction::OpCode::End)
      break;

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if ((Status = makeInstructionNode(Code, NewInst)) != Base::ErrCode::Success)
      break;
    if ((Status = NewInst->loadBinary(Mgr)) != Base::ErrCode::Success)
      break;
    Body.push_back(std::move(NewInst));
  }
  return Status;
}

/// Load binary of if-else instructions. See "include/loader/instruction.h".
Base::ErrCode IfElseControlInstruction::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the block return type.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
      Base::ErrCode::Success)
    return Status;
  BlockType = static_cast<Base::ValType>(Byte);

  /// Read instructions and make nodes until OpCode::End.
  bool IsElseStatement = false;
  while (Status == Base::ErrCode::Success) {
    /// Read the opcode and check if error.
    if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
        Base::ErrCode::Success)
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
    if ((Status = makeInstructionNode(Code, NewInst)) != Base::ErrCode::Success)
      break;
    if ((Status = NewInst->loadBinary(Mgr)) != Base::ErrCode::Success)
      break;
    if (IsElseStatement)
      ElseStatement.push_back(std::move(NewInst));
    else
      IfStatement.push_back(std::move(NewInst));
  }
  return Status;
}

/// Load binary of branch instructions. See "include/loader/instruction.h".
Base::ErrCode BrControlInstruction::loadBinary(FileMgr &Mgr) {
  return static_cast<Base::ErrCode>(Mgr.readU32(LabelIdx));
}

/// Load binary of branch table instructions. See
/// "include/loader/instruction.h".
Base::ErrCode BrTableControlInstruction::loadBinary(FileMgr &Mgr) {
  unsigned int VecCnt = 0;
  unsigned int Idx = 0;
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the vector of labels.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(VecCnt))) !=
      Base::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(Idx))) !=
        Base::ErrCode::Success)
      return Status;
    LabelTable.push_back(Idx);
  }

  /// Read default label.
  return static_cast<Base::ErrCode>(Mgr.readU32(LabelIdx));
}

/// Load binary of call instructions. See "include/loader/instruction.h".
Base::ErrCode CallControlInstruction::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read function index.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(FuncIdx))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read the 0x00 checking code in indirect_call case.
  if (Code == Instruction::OpCode::Call_indirect) {
    unsigned char Byte = 0xFF;
    Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte));
    if (Status == Base::ErrCode::Success && Byte != 0x00)
      Status = Base::ErrCode::InvalidGrammar;
  }
  return Status;
}

/// Load binary of variable instructions. See "include/loader/instruction.h".
Base::ErrCode VariableInstruction::loadBinary(FileMgr &Mgr) {
  return static_cast<Base::ErrCode>(Mgr.readU32(Idx));
}

/// Load binary of memory instructions. See "include/loader/instruction.h".
Base::ErrCode MemoryInstruction::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the 0x00 checking code in memory.grow and memory.size cases.
  unsigned char Byte = 0xFF;
  if (Code == Instruction::OpCode::Memory__grow ||
      Code == Instruction::OpCode::Memory__size) {
    Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte));
    if (Status == Base::ErrCode::Success && Byte == 0x00)
      return Status;
  }

  /// Read memory arguments.
  if (Status == Base::ErrCode::Success)
    Status = static_cast<Base::ErrCode>(Mgr.readU32(Align));
  if (Status == Base::ErrCode::Success)
    Status = static_cast<Base::ErrCode>(Mgr.readU32(Offset));
  return Status;
}

/// Load binary of const numeric instructions. See
/// "include/loader/instruction.h".
Base::ErrCode ConstInstruction::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the const number of corresbonding value type.
  switch (Code) {
  case Instruction::OpCode::I32__const: {
    int32_t Val = 0;
    Status = static_cast<Base::ErrCode>(Mgr.readS32(Val));
    Num = Val;
    break;
  }
  case Instruction::OpCode::I64__const: {
    int64_t Val = 0;
    Status = static_cast<Base::ErrCode>(Mgr.readS64(Val));
    Num = Val;
    break;
  }
  case Instruction::OpCode::F32__const: {
    float Val = 0;
    Status = static_cast<Base::ErrCode>(Mgr.readF32(Val));
    Num = Val;
    break;
  }
  case Instruction::OpCode::F64__const: {
    double Val = 0;
    Status = static_cast<Base::ErrCode>(Mgr.readF64(Val));
    Num = Val;
    break;
  }
  default:
    Status = Base::ErrCode::InvalidGrammar;
    break;
  }
  return Status;
}

/// Instruction node maker. See "include/loader/instruction.h".
Base::ErrCode makeInstructionNode(Instruction::OpCode Code,
                                  std::unique_ptr<Instruction> &NewInst) {
  /// Make the instruction node according to Code.
  switch (Code) {
    /// The OpCode::End and OpCode::Else will not make nodes.
  case Instruction::OpCode::Unreachable:
  case Instruction::OpCode::Nop:
  case Instruction::OpCode::Return:
    NewInst = std::make_unique<ControlInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::Block:
  case Instruction::OpCode::Loop:
    NewInst = std::make_unique<BlockControlInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::If:
    NewInst = std::make_unique<IfElseControlInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::Br:
  case Instruction::OpCode::Br_if:
    NewInst = std::make_unique<BrControlInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::Br_table:
    NewInst = std::make_unique<BrTableControlInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::Call:
  case Instruction::OpCode::Call_indirect:
    NewInst = std::make_unique<CallControlInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::Drop:
  case Instruction::OpCode::Select:
    NewInst = std::make_unique<ParametricInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::Local__get:
  case Instruction::OpCode::Local__set:
  case Instruction::OpCode::Local__tee:
  case Instruction::OpCode::Global__get:
  case Instruction::OpCode::Global__set:
    NewInst = std::make_unique<VariableInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::I32__load:
  case Instruction::OpCode::I64__load:
  case Instruction::OpCode::F32__load:
  case Instruction::OpCode::F64__load:
  case Instruction::OpCode::I32__load8_s:
  case Instruction::OpCode::I32__load8_u:
  case Instruction::OpCode::I32__load16_s:
  case Instruction::OpCode::I32__load16_u:
  case Instruction::OpCode::I64__load8_s:
  case Instruction::OpCode::I64__load8_u:
  case Instruction::OpCode::I64__load16_s:
  case Instruction::OpCode::I64__load16_u:
  case Instruction::OpCode::I64__load32_s:
  case Instruction::OpCode::I64__load32_u:
  case Instruction::OpCode::I32__store:
  case Instruction::OpCode::I64__store:
  case Instruction::OpCode::F32__store:
  case Instruction::OpCode::F64__store:
  case Instruction::OpCode::I32__store8:
  case Instruction::OpCode::I32__store16:
  case Instruction::OpCode::I64__store8:
  case Instruction::OpCode::I64__store16:
  case Instruction::OpCode::I64__store32:
  case Instruction::OpCode::Memory__size:
  case Instruction::OpCode::Memory__grow:
    NewInst = std::make_unique<MemoryInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::I32__const:
  case Instruction::OpCode::I64__const:
  case Instruction::OpCode::F32__const:
  case Instruction::OpCode::F64__const:
    NewInst = std::make_unique<ConstInstruction>(Code);
    return Base::ErrCode::Success;

  case Instruction::OpCode::I32__eqz:
  case Instruction::OpCode::I32__eq:
  case Instruction::OpCode::I32__ne:
  case Instruction::OpCode::I32__lt_s:
  case Instruction::OpCode::I32__lt_u:
  case Instruction::OpCode::I32__gt_s:
  case Instruction::OpCode::I32__gt_u:
  case Instruction::OpCode::I32__le_s:
  case Instruction::OpCode::I32__le_u:
  case Instruction::OpCode::I32__ge_s:
  case Instruction::OpCode::I32__ge_u:
  case Instruction::OpCode::I64__eqz:
  case Instruction::OpCode::I64__eq:
  case Instruction::OpCode::I64__ne:
  case Instruction::OpCode::I64__lt_s:
  case Instruction::OpCode::I64__lt_u:
  case Instruction::OpCode::I64__gt_s:
  case Instruction::OpCode::I64__gt_u:
  case Instruction::OpCode::I64__le_s:
  case Instruction::OpCode::I64__le_u:
  case Instruction::OpCode::I64__ge_s:
  case Instruction::OpCode::I64__ge_u:
  case Instruction::OpCode::F32__eq:
  case Instruction::OpCode::F32__ne:
  case Instruction::OpCode::F32__lt:
  case Instruction::OpCode::F32__gt:
  case Instruction::OpCode::F32__le:
  case Instruction::OpCode::F32__ge:
  case Instruction::OpCode::F64__eq:
  case Instruction::OpCode::F64__ne:
  case Instruction::OpCode::F64__lt:
  case Instruction::OpCode::F64__gt:
  case Instruction::OpCode::F64__le:
  case Instruction::OpCode::F64__ge:
  case Instruction::OpCode::I32__clz:
  case Instruction::OpCode::I32__ctz:
  case Instruction::OpCode::I32__popcnt:
  case Instruction::OpCode::I32__add:
  case Instruction::OpCode::I32__sub:
  case Instruction::OpCode::I32__mul:
  case Instruction::OpCode::I32__div_s:
  case Instruction::OpCode::I32__div_u:
  case Instruction::OpCode::I32__rem_s:
  case Instruction::OpCode::I32__rem_u:
  case Instruction::OpCode::I32__and:
  case Instruction::OpCode::I32__or:
  case Instruction::OpCode::I32__xor:
  case Instruction::OpCode::I32__shl:
  case Instruction::OpCode::I32__shr_s:
  case Instruction::OpCode::I32__shr_u:
  case Instruction::OpCode::I32__rotl:
  case Instruction::OpCode::I32__rotr:
  case Instruction::OpCode::I64__clz:
  case Instruction::OpCode::I64__ctz:
  case Instruction::OpCode::I64__popcnt:
  case Instruction::OpCode::I64__add:
  case Instruction::OpCode::I64__sub:
  case Instruction::OpCode::I64__mul:
  case Instruction::OpCode::I64__div_s:
  case Instruction::OpCode::I64__div_u:
  case Instruction::OpCode::I64__rem_s:
  case Instruction::OpCode::I64__rem_u:
  case Instruction::OpCode::I64__and:
  case Instruction::OpCode::I64__or:
  case Instruction::OpCode::I64__xor:
  case Instruction::OpCode::I64__shl:
  case Instruction::OpCode::I64__shr_s:
  case Instruction::OpCode::I64__shr_u:
  case Instruction::OpCode::I64__rotl:
  case Instruction::OpCode::I64__rotr:
  case Instruction::OpCode::F32__abs:
  case Instruction::OpCode::F32__neg:
  case Instruction::OpCode::F32__ceil:
  case Instruction::OpCode::F32__floor:
  case Instruction::OpCode::F32__trunc:
  case Instruction::OpCode::F32__nearest:
  case Instruction::OpCode::F32__sqrt:
  case Instruction::OpCode::F32__add:
  case Instruction::OpCode::F32__sub:
  case Instruction::OpCode::F32__mul:
  case Instruction::OpCode::F32__div:
  case Instruction::OpCode::F32__min:
  case Instruction::OpCode::F32__max:
  case Instruction::OpCode::F32__copysign:
  case Instruction::OpCode::F64__abs:
  case Instruction::OpCode::F64__neg:
  case Instruction::OpCode::F64__ceil:
  case Instruction::OpCode::F64__floor:
  case Instruction::OpCode::F64__trunc:
  case Instruction::OpCode::F64__nearest:
  case Instruction::OpCode::F64__sqrt:
  case Instruction::OpCode::F64__add:
  case Instruction::OpCode::F64__sub:
  case Instruction::OpCode::F64__mul:
  case Instruction::OpCode::F64__div:
  case Instruction::OpCode::F64__min:
  case Instruction::OpCode::F64__max:
  case Instruction::OpCode::F64__copysign:
  case Instruction::OpCode::I32__wrap_i64:
  case Instruction::OpCode::I32__trunc_f32_s:
  case Instruction::OpCode::I32__trunc_f32_u:
  case Instruction::OpCode::I32__trunc_f64_s:
  case Instruction::OpCode::I32__trunc_f64_u:
  case Instruction::OpCode::I64__extend_i32_s:
  case Instruction::OpCode::I64__extend_i32_u:
  case Instruction::OpCode::I64__trunc_f32_s:
  case Instruction::OpCode::I64__trunc_f32_u:
  case Instruction::OpCode::I64__trunc_f64_s:
  case Instruction::OpCode::I64__trunc_f64_u:
  case Instruction::OpCode::F32__convert_i32_s:
  case Instruction::OpCode::F32__convert_i32_u:
  case Instruction::OpCode::F32__convert_i64_s:
  case Instruction::OpCode::F32__convert_i64_u:
  case Instruction::OpCode::F32__demote_f64:
  case Instruction::OpCode::F64__convert_i32_s:
  case Instruction::OpCode::F64__convert_i32_u:
  case Instruction::OpCode::F64__convert_i64_s:
  case Instruction::OpCode::F64__convert_i64_u:
  case Instruction::OpCode::F64__promote_f32:
  case Instruction::OpCode::I32__reinterpret_f32:
  case Instruction::OpCode::I64__reinterpret_f64:
  case Instruction::OpCode::F32__reinterpret_i32:
  case Instruction::OpCode::F64__reinterpret_i64:
    NewInst = std::make_unique<NumericInstruction>(Code);
    return Base::ErrCode::Success;

  default:
    break;
  }
  /// If the Code not matched, return null pointer.
  return Base::ErrCode::InvalidGrammar;
}

} // namespace AST