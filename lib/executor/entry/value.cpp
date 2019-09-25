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
ErrCode ValueEntry::setValue(const ValueEntry &Val) {
  if (Val.Type != this->Type) {
    return ErrCode::TypeNotMatch;
  }
  this->Value = Val.Value;
  return ErrCode::Success;
}

ErrCode ValueEntry::setValue(const AST::ValVariant &Val) {
  if (Val.index() != Value.index()) {
    return ErrCode::TypeNotMatch;
  }
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
