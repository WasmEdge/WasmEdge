#include "executor/executor.h"
#include "ast/module.h"
#include "ast/section.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Set AST Module node to executor. See "include/executor/executor.h".
ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  Mod = std::move(Module);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
