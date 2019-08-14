#include "executor/globalinst.h"
#include <type_traits>

namespace SSVM {
namespace Executor {

/// Setter of global type. See "include/executor/globalinst.h".
Executor::ErrCode GlobalInstance::setGlobalType(AST::ValType &ValueType,
                                                AST::ValMut &Mutibility) {
  Type = ValueType;
  Mut = Mutibility;
  return Executor::ErrCode::Success;
}

/// Setter of initialization expression. See "include/executor/globalinst.h".
Executor::ErrCode GlobalInstance::setExpression(
    std::vector<std::unique_ptr<AST::Instruction>> &Expr) {
  Instrs = std::move(Expr);
  return Executor::ErrCode::Success;
}

/// Getter of value. See "include/executor/globalinst.h".
template <typename T> Executor::ErrCode GlobalInstance::getValue(T &Val) {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return Executor::ErrCode::TypeNotMatch;
  }
  return Executor::ErrCode::Success;
}

/// Setter of value. See "include/executor/globalinst.h".
template <typename T> Executor::ErrCode GlobalInstance::setValue(T Val) {
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

} // namespace Executor
} // namespace SSVM
