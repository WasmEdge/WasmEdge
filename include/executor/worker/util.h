#include "ast/instruction.h"
#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

namespace {

using OpCode = AST::Instruction::OpCode;
using Value = AST::ValVariant;

} // namespace

template <typename T> inline T retrieveValue(const ValueEntry &Val) {
  T Value;
  Val.getValue(Value);
  return Value;
}

inline bool isValueTypeEqual(const ValueEntry &Val1, const ValueEntry &Val2) {
  AST::ValType ValTp1 = Val1.getType();
  AST::ValType ValTp2 = Val2.getType();
  return ValTp1 == ValTp2;
}

inline bool isInRange(OpCode X, OpCode Y, OpCode Z) {
  auto XC = static_cast<unsigned char>(X);
  auto YC = static_cast<unsigned char>(Y);
  auto ZC = static_cast<unsigned char>(Z);
  return (XC <= YC && YC <= ZC);
}

inline bool isControlOp(OpCode Opcode) {
  return isInRange(OpCode::Unreachable, Opcode, OpCode::Call_indirect);
}

inline bool isParametricOp(OpCode Opcode) {
  return isInRange(OpCode::Drop, Opcode, OpCode::Select);
}

inline bool isVariableOp(OpCode Opcode) {
  return isInRange(OpCode::Local__get, Opcode, OpCode::Global__set);
}

inline bool isLoadOp(OpCode Opcode) {
  return isInRange(OpCode::I32__load, Opcode, OpCode::I64__load32_u);
}

inline bool isStoreOp(OpCode Opcode) {
  return isInRange(OpCode::I32__store, Opcode, OpCode::I64__store32);
}

inline bool isMemoryOp(OpCode Opcode) {
  return isInRange(OpCode::I32__load, Opcode, OpCode::Memory__grow);
}

inline bool isConstNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__const, Opcode, OpCode::F64__const);
}

inline bool isNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__eqz, Opcode, OpCode::F64__reinterpret_i64);
}

inline bool isBinaryOp(OpCode Opcode) {
  bool Ret = false;
  switch (Opcode) {
  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_u:
    Ret = true;
    break;
  default:
    Ret = false;
    break;
  }
  return Ret;
}

inline bool isComparisonOp(OpCode Opcode) {
  bool Ret = false;
  switch (Opcode) {
  case OpCode::I32__le_s:
  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I64__eq:
  case OpCode::I64__lt_u:
    Ret = true;
    break;
  default:
    Ret = false;
    break;
  }
  return Ret;
}

} // namespace Executor
} // namespace SSVM
