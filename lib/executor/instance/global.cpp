#include "executor/instance/global.h"
#include <type_traits>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of global type. See "include/executor/instance/global.h".
ErrCode GlobalInstance::setGlobalType(AST::ValType &ValueType,
                                      AST::ValMut &Mutibility) {
  Type = ValueType;
  Mut = Mutibility;
  return ErrCode::Success;
}

/// Getter of value. See "include/executor/instance/global.h".
template <typename T> ErrCode GlobalInstance::getValue(T &Val) {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return ErrCode::TypeNotMatch;
  }
  return ErrCode::Success;
}
template <typename T> ErrCode GlobalInstance::setValue(T &Val) {
  Executor::ErrCode Status = Executor::ErrCode::TypeNotMatch;
  switch (Type) {
  case AST::ValType::I32:
    if (std::is_same<T, int32_t>::value)
      Status = Executor::ErrCode::Success;
    break;
  case AST::ValType::I64:
    if (std::is_same<T, int64_t>::value)
      Status = Executor::ErrCode::Success;
    break;
  case AST::ValType::F32:
    if (std::is_same<T, float>::value)
      Status = Executor::ErrCode::Success;
    break;
  case AST::ValType::F64:
    if (std::is_same<T, double>::value)
      Status = Executor::ErrCode::Success;
    break;
  default:
    break;
  }
  if (Status == Executor::ErrCode::Success)
    Value = Val;
  return Status;
}
template <>
ErrCode GlobalInstance::setValue<AST::ValVariant>(AST::ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
