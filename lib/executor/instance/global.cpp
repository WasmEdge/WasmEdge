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

/// Setter of init expression. See "include/executor/instance/global.h".
ErrCode GlobalInstance::setExpression(
    std::vector<std::unique_ptr<AST::Instruction>> &Expr) {
  Instrs = std::move(Expr);
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

/// Setter of value. See "include/executor/instance/global.h".
template <typename T> ErrCode GlobalInstance::setValue(T Val) {
  ErrCode Status = ErrCode::TypeNotMatch;
  switch (Type) {
  case AST::ValType::I32:
    if (std::is_same<T, int32_t>::value)
      Status = ErrCode::Success;
    break;
  case AST::ValType::I64:
    if (std::is_same<T, int64_t>::value)
      Status = ErrCode::Success;
    break;
  case AST::ValType::F32:
    if (std::is_same<T, float>::value)
      Status = ErrCode::Success;
    break;
  case AST::ValType::F64:
    if (std::is_same<T, double>::value)
      Status = ErrCode::Success;
    break;
  default:
    break;
  }
  if (Status == ErrCode::Success)
    Value = Val;
  return Status;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
