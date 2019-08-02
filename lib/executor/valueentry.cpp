#include "executor/valueentry.h"

/// Getter of I32 value. See "include/executor/valueentry.h".
Executor::ErrCode ValueEntry::getValueI32(int32_t &Val) {
  if (Type != AST::ValType::I32)
    return Executor::ErrCode::TypeNotMatch;
  Val = std::get<0>(Value);
  return Executor::ErrCode::Success;
}

/// Getter of I64 value. See "include/executor/valueentry.h".
Executor::ErrCode ValueEntry::getValueI64(int64_t &Val) {
  if (Type != AST::ValType::I64)
    return Executor::ErrCode::TypeNotMatch;
  Val = std::get<1>(Value);
  return Executor::ErrCode::Success;
}

/// Getter of F32 value. See "include/executor/valueentry.h".
Executor::ErrCode ValueEntry::getValueF32(float &Val) {
  if (Type != AST::ValType::F32)
    return Executor::ErrCode::TypeNotMatch;
  Val = std::get<2>(Value);
  return Executor::ErrCode::Success;
}

/// Getter of F64 value. See "include/executor/valueentry.h".
Executor::ErrCode ValueEntry::getValueF64(double &Val) {
  if (Type != AST::ValType::F64)
    return Executor::ErrCode::TypeNotMatch;
  Val = std::get<3>(Value);
  return Executor::ErrCode::Success;
}