#include "executor/globalinst.h"

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

/// Setter of value. See "include/executor/globalinst.h".
Executor::ErrCode
GlobalInstance::setValue(std::variant<int32_t, int64_t, float, double> &Val) {
  switch (Val.index()) {
  case 0:
    Value = std::get<0>(Val);
    break;
  case 1:
    Value = std::get<1>(Val);
    break;
  case 2:
    Value = std::get<2>(Val);
    break;
  case 3:
    Value = std::get<3>(Val);
    break;
  default:
    break;
  }
  return Executor::ErrCode::Success;
}