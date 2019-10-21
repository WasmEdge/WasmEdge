#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Instantiate types in module instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::TypeSection *TypeSec) {
  if (TypeSec == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;

  /// Iterate and instantiate types.
  auto &FuncTypes = TypeSec->getContent();
  for (auto FuncType = FuncTypes.begin(); FuncType != FuncTypes.end();
       FuncType++) {
    /// Copy param and return lists to module instance.
    auto &Param = (*FuncType)->getParamTypes();
    auto &Return = (*FuncType)->getReturnTypes();
    if ((Status = ModInst->addFuncType(Param, Return)) != ErrCode::Success) {
      return Status;
    }
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM