#include "loader/instruction.h"

namespace AST {

/// Load binary of block instructions. See "include/loader/instruction.h".
bool BlockControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the block return type.
  unsigned char Byte = 0;
  if (!Mgr.readByte(Byte))
    return false;
  BlockType = static_cast<Base::ValType>(Byte);

  /// Read instructions and make nodes until Opcode::End.
  while (Mgr.readByte(Byte)) {
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);
    if (Code == Instruction::OpCode::End)
      return true;
    std::unique_ptr<Instruction> NewInst = makeInstructionNode(Code);
    if (NewInst == nullptr || !NewInst->loadBinary(Mgr))
      return false;
    Body.push_back(std::move(NewInst));
  }
  return false;
}

/// Load binary of if-else instructions. See "include/loader/instruction.h".
bool IfElseControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the block return type.
  unsigned char Byte = 0;
  if (!Mgr.readByte(Byte))
    return false;
  BlockType = static_cast<Base::ValType>(Byte);

  /// Read instructions and make nodes until OpCode::End.
  bool IsElseStatement = false;
  while (Mgr.readByte(Byte)) {
    Instruction::OpCode Code = static_cast<Instruction::OpCode>(Byte);
    if (Code == Instruction::OpCode::End)
      return true;
    /// If an OpCode::Else read, switch to Else statement.
    if (Code == Instruction::OpCode::Else) {
      IsElseStatement = true;
      continue;
    }
    std::unique_ptr<Instruction> NewInst = makeInstructionNode(Code);
    if (NewInst == nullptr || !NewInst->loadBinary(Mgr))
      return false;
    if (IsElseStatement)
      ElseStatement.push_back(std::move(NewInst));
    else
      IfStatement.push_back(std::move(NewInst));
  }
  return false;
}

/// Load binary of branch instructions. See "include/loader/instruction.h".
bool BrControlInstruction::loadBinary(FileMgr &Mgr) {
  return Mgr.readU32(LabelIdx);
}

/// Load binary of branch table instructions. See
/// "include/loader/instruction.h".
bool BrTableControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the vector of labels.
  unsigned int VecCnt = 0;
  unsigned int Idx = 0;
  if (!Mgr.readU32(VecCnt))
    return false;
  for (int i = 0; i < VecCnt; i++) {
    if (!Mgr.readU32(Idx))
      return false;
    LabelTable.push_back(Idx);
  }

  /// Read default label.
  return Mgr.readU32(LabelIdx);
}

/// Load binary of call instructions. See "include/loader/instruction.h".
bool CallControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read function index.
  if (!Mgr.readU32(FuncIdx))
    return false;

  /// Read the 0x00 checking code in indirect_call case.
  if (Code == Instruction::OpCode::Call_indirect) {
    unsigned char Byte = 0xFF;
    return Mgr.readByte(Byte) && Byte == 0x00;
  }
  return true;
}

/// Load binary of variable instructions. See "include/loader/instruction.h".
bool VariableInstruction::loadBinary(FileMgr &Mgr) { return Mgr.readU32(Idx); }

/// Load binary of memory instructions. See "include/loader/instruction.h".
bool MemoryInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the 0x00 checking code in memory.grow and memory.size cases.
  unsigned char Byte = 0xFF;
  if (Code == Instruction::OpCode::Memory__grow ||
      Code == Instruction::OpCode::Memory__size)
    return Mgr.readByte(Byte) && Byte == 0x00;

  /// Read memory arguments.
  return Mgr.readU32(Align) && Mgr.readU32(Offset);
}

/// Load binary of const numeric instructions. See
/// "include/loader/instruction.h".
bool ConstInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the const number of corresbonding value type.
  switch (Code) {
  case Instruction::OpCode::I32__const: {
    int32_t Val = 0;
    if (!Mgr.readS32(Val))
      return false;
    Num = Val;
    break;
  }
  case Instruction::OpCode::I64__const: {
    int64_t Val = 0;
    if (!Mgr.readS64(Val))
      return false;
    Num = Val;
    break;
  }
  case Instruction::OpCode::F32__const: {
    float Val = 0;
    if (!Mgr.readF32(Val))
      return false;
    Num = Val;
    break;
  }
  case Instruction::OpCode::F64__const: {
    double Val = 0;
    if (!Mgr.readF64(Val))
      return false;
    Num = Val;
    break;
  }
  default:
    return false;
  }
  return true;
}

/// Instruction node maker. See "include/loader/instruction.h".
std::unique_ptr<Instruction> makeInstructionNode(Instruction::OpCode Code) {
  /// Make the instruction node according to Code.
  switch (Code) {
    /// The OpCode::End and OpCode::Else will not make nodes.
  case Instruction::OpCode::Unreachable:
  case Instruction::OpCode::Nop:
  case Instruction::OpCode::Return:
    return std::make_unique<ControlInstruction>(Code);

  case Instruction::OpCode::Block:
  case Instruction::OpCode::Loop:
    return std::make_unique<BlockControlInstruction>(Code);

  case Instruction::OpCode::If:
    return std::make_unique<IfElseControlInstruction>(Code);

  case Instruction::OpCode::Br:
  case Instruction::OpCode::Br_if:
    return std::make_unique<BrControlInstruction>(Code);

  case Instruction::OpCode::Br_table:
    return std::make_unique<BrTableControlInstruction>(Code);

  case Instruction::OpCode::Call:
  case Instruction::OpCode::Call_indirect:
    return std::make_unique<CallControlInstruction>(Code);

  case Instruction::OpCode::Drop:
  case Instruction::OpCode::Select:
    return std::make_unique<ParametricInstruction>(Code);

  case Instruction::OpCode::Local__get:
  case Instruction::OpCode::Local__set:
  case Instruction::OpCode::Local__tee:
  case Instruction::OpCode::Global__get:
  case Instruction::OpCode::Global__set:
    return std::make_unique<ParametricInstruction>(Code);

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
    return std::make_unique<MemoryInstruction>(Code);

  case Instruction::OpCode::I32__const:
  case Instruction::OpCode::I64__const:
  case Instruction::OpCode::F32__const:
  case Instruction::OpCode::F64__const:
    return std::make_unique<ConstInstruction>(Code);

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
    return std::make_unique<NumericInstruction>(Code);

  default:
    break;
  }
  /// If the Code not matched, return null pointer.
  return nullptr;
}

} // namespace AST