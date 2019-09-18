#include "executor/instance/global.h"
#include <type_traits>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Constructor of global instance. See "include/executor/instance/global.h".
GlobalInstance::GlobalInstance(const AST::ValType &ValueType,
                               const AST::ValMut &Mutibility) {
  Type = ValueType;
  Mut = Mutibility;
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

/// Getter of value. See "include/executor/instance/global.h".
template <typename T> TypeV<T, ErrCode> GlobalInstance::getValue(T &Val) const {
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
GlobalInstance::getValue(AST::ValVariant &Val) const {
  Val = Value;
  return ErrCode::Success;
}

/// Setter of value. See "include/executor/instance/global.h".
template <typename T> TypeV<T, ErrCode> GlobalInstance::setValue(const T &Val) {
  if (!std::holds_alternative<T>(Value)) {
    return ErrCode::TypeNotMatch;
  }
  Value = Val;
  return ErrCode::Success;
}

template <>
TypeV<AST::ValVariant, ErrCode>
GlobalInstance::setValue(const AST::ValVariant &Val) {
  if (Val.index() != Value.index()) {
    return ErrCode::TypeNotMatch;
  }
  Value = Val;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
