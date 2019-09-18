#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Constructor of value entry. See "include/executor/entry/value.h".
ValueEntry::ValueEntry(const AST::ValType &VT) {
  Type = VT;
  switch (Type) {
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
}

/// Setter for value. See "include/executor/entry/value.h".
template <typename T> TypeVE<T, ErrCode> ValueEntry::setValue(const T &Val) {
  if ((std::is_same<T, uint32_t>::value && Type == AST::ValType::I32) ||
      (std::is_same<T, uint64_t>::value && Type == AST::ValType::I64) ||
      (std::is_same<T, float>::value && Type == AST::ValType::F32) ||
      (std::is_same<T, double>::value && Type == AST::ValType::F64)) {
    Value = Val;
    return ErrCode::Success;
  }
  return ErrCode::TypeNotMatch;
}

template <>
TypeVE<ValueEntry, ErrCode> ValueEntry::setValue(const ValueEntry &Val) {
  if (Val.Type != this->Type) {
    return ErrCode::TypeNotMatch;
  }
  this->Value = Val.Value;
  return ErrCode::Success;
}

template <>
TypeVE<AST::ValVariant, ErrCode>
ValueEntry::setValue(const AST::ValVariant &Val) {
  if (Val.index() != Value.index()) {
    return ErrCode::TypeNotMatch;
  }
  Value = Val;
  return ErrCode::Success;
}

/// Getter for value. See "include/executor/entry/value.h".
template <typename T> TypeV<T, ErrCode> ValueEntry::getValue(T &Val) const {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return ErrCode::TypeNotMatch;
  }
  return ErrCode::Success;
}

template <>
TypeV<AST::ValVariant, ErrCode>
ValueEntry::getValue(AST::ValVariant &Val) const {
  Val = Value;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
