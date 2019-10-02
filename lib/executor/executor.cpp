#include "executor/executor.h"
#include "ast/module.h"
#include "ast/section.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Set and instantiate host function. See "include/executor/executor.h".
ErrCode Executor::setHostFunction(std::unique_ptr<HostFunction> &Func,
                                  std::string &ModName, std::string &FuncName) {
  auto NewFuncInst = std::make_unique<Instance::FunctionInstance>();
  NewFuncInst->setNames(ModName, FuncName);
  return ErrCode::Success;
}

/// Set AST Module node to executor. See "include/executor/executor.h".
ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongExecutorFlow;

  /// Get ownership of module.
  Mod = std::move(Module);
  return ErrCode::Success;
}

/// Instantiate module. See "include/loader/executor.h".
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

/// Set arguments. See "include/loader/executor.h".
ErrCode Executor::setArgs(std::vector<std::unique_ptr<ValueEntry>> &Args) {
  /// Check is the correct state.
  if (Stat != State::Instantiated)
    return ErrCode::WrongExecutorFlow;

  /// Push args to stack.
  for (auto It = Args.begin(); It != Args.end(); It++) {
    StackMgr.push(std::move(*It));
  }
  Args.clear();
  Stat = State::ArgsSet;
  return ErrCode::Success;
}

/// Invoke start function. See "include/loader/executor.h".
ErrCode Executor::run() {
  /// Check is the correct state.
  if (Stat != State::ArgsSet)
    return ErrCode::WrongExecutorFlow;

  /// Run start function.
  ErrCode Result = Engine.runStartFunction(ModInst->getStartAddr());
  Stat = State::Finished;
  return Result;
}

} // namespace Executor
} // namespace SSVM
