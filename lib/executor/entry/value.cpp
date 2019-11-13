#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Initializers of value entry. See "include/executor/entry/value.h".
ErrCode ValueEntry::InitValueEntry(const ValueEntry &VE) {
  Type = VE.Type;
  Value = VE.Value;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const AST::ValType &VT) {
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
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const AST::ValType &VT,
                                   const AST::ValVariant &Val) {
  Type = VT;
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const AST::ValVariant &Val) {
  switch (Val.index()) {
  case 0:
    Type = AST::ValType::I32;
    Value = std::get<0>(Val);
    break;
  case 1:
    Type = AST::ValType::I64;
    Value = std::get<1>(Val);
    break;
  case 2:
    Type = AST::ValType::F32;
    Value = std::get<2>(Val);
    break;
  case 3:
    Type = AST::ValType::F64;
    Value = std::get<3>(Val);
  default:
    return ErrCode::TypeNotMatch;
    break;
  }
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const uint32_t &Val) {
  Type = AST::ValType::I32;
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const uint64_t &Val) {
  Type = AST::ValType::I64;
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const float &Val) {
  Type = AST::ValType::F32;
  Value = Val;
  return ErrCode::Success;
}

ErrCode ValueEntry::InitValueEntry(const double &Val) {
  Type = AST::ValType::F64;
  Value = Val;
  return ErrCode::Success;
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
