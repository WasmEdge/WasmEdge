#include "executor/executionengine.h"

namespace SSVM {
namespace ExecutionEngine {

ErrCode ExecutionEngine::setModule(std::unique_ptr<AST::Module> &Module) {
  Mod = std::move(Module);
  return ErrCode::Success;
}

} // namespace ExecutionEngine
} // namespace SSVM
