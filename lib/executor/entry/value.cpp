#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Initializers of value entry. See "include/executor/entry/value.h".
ErrCode ValueEntry::InitValueEntry(const ValueEntry &VE) {
  Value = VE.Value;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const AST::ValType &VT) {
  switch (VT) {
  case AST::ValType::I32:
    Value = (uint32_t)0;
    break;
  case AST::ValType::I64:
    Value = (uint64_t)0;
    break;
  case AST::ValType::F32:
    Value = (float)0.0;
    break;
  case AST::ValType::F64:
    Value = (double)0.0;
    break;
  default:
    break;
  }
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const AST::ValType &VT,
                                   const AST::ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const AST::ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const uint32_t &Val) {
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const uint64_t &Val) {
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const float &Val) {
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const double &Val) {
  Value = Val;
  return ErrCode::Success;
}

/// Setter for value. See "include/executor/entry/value.h".
ErrCode ValueEntry::setValue(const ValueEntry &Val) {
  Value = Val.Value;
  return ErrCode::Success;
}

ErrCode ValueEntry::setValue(const AST::ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

/// Getter for value. See "include/executor/entry/value.h".
ErrCode ValueEntry::getValue(AST::ValVariant &Val) const {
  Val = Value;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
