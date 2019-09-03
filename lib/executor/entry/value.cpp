#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Constructor of value entry. See "include/executor/entry/value.h".
ValueEntry::ValueEntry(AST::ValType VT) {
  Type = VT;
  switch (Type) {
  case AST::ValType::I32:
    Value = (int32_t)0;
    break;
  case AST::ValType::I64:
    Value = (int64_t)0;
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
template <typename T> ErrCode ValueEntry::setValue(T &Val) {
  if ((std::is_same<T, int32_t>::value && Type == AST::ValType::I32) ||
      (std::is_same<T, int64_t>::value && Type == AST::ValType::I64) ||
      (std::is_same<T, float>::value && Type == AST::ValType::F32) ||
      (std::is_same<T, double>::value && Type == AST::ValType::F64)) {
    Value = Val;
    return ErrCode::Success;
  }
  return ErrCode::TypeNotMatch;
}
template <>
ErrCode ValueEntry::setValue<AST::ValVariant>(AST::ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

/// Getter for value. See "include/executor/entry/value.h".
template <typename T> ErrCode ValueEntry::getValue(T &Val) const {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return ErrCode::TypeNotMatch;
  }
  return ErrCode::Success;
}
template <>
ErrCode ValueEntry::getValue<AST::ValVariant>(AST::ValVariant &Val) const {
  Val = Value;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
