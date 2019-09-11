#include "ast/instruction.h"
#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

namespace {

using OpCode = AST::Instruction::OpCode;

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

inline bool isTestNumericOp(OpCode Opcode) {
  return Opcode == OpCode::I32__eqz || Opcode == OpCode::I64__eqz;
}

inline bool isRelationNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__eq, Opcode, OpCode::F64__ge) &&
         Opcode != OpCode::I64__eqz;
}

inline bool isUnaryNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__clz, Opcode, OpCode::I32__popcnt) ||
         isInRange(OpCode::I64__clz, Opcode, OpCode::I64__popcnt) ||
         isInRange(OpCode::F32__abs, Opcode, OpCode::F32__sqrt) ||
         isInRange(OpCode::F64__abs, Opcode, OpCode::F64__sqrt);
}

inline bool isBinaryNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__add, Opcode, OpCode::I32__rotr) ||
         isInRange(OpCode::I64__add, Opcode, OpCode::I64__rotr) ||
         isInRange(OpCode::F32__add, Opcode, OpCode::F32__copysign) ||
         isInRange(OpCode::F64__add, Opcode, OpCode::F64__copysign);
}

inline bool isCastNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__wrap_i64, Opcode, OpCode::F64__reinterpret_i64);
}

} // namespace Executor
} // namespace SSVM
