#include "executor/executor.h"
#include "ast/module.h"
#include "ast/section.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Set AST Module node to executor. See "include/executor/executor.h".
ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongExecutorFlow;

  /// Get ownership of module.
  Mod = std::move(Module);
  return ErrCode::Success;
}

/// Instantiate module. See "include/loader/loader.h".
ErrCode Executor::instantiate() {
  /// Check is the correct state.
  if (Stat != State::ModuleSet)
    return ErrCode::WrongExecutorFlow;

  /// Instantiate module.
  ErrCode Result = instantiate(Mod.get());
  if (Result == ErrCode::Success)
    Stat = State::Instantiated;
  return Result;
}

/// Invoke start function. See "include/loader/loader.h".
ErrCode Executor::run() {
  /// Check is the correct state.
  if (Stat != State::Instantiated)
    return ErrCode::WrongExecutorFlow;

  /// Instantiate module.
  ErrCode Result = ErrCode::Success; /// instantiate(Mod.get());
  if (Result == ErrCode::Success)
    Stat = State::Finished;
  return Result;
}

} // namespace Executor
} // namespace SSVM
