#include "executor/executor.h"

namespace SSVM {
namespace Executor {

ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  Mod = std::move(Module);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
