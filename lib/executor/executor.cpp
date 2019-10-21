#include "executor/executor.h"
#include "ast/module.h"
#include "ast/section.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Set and instantiate host function. See "include/executor/executor.h".
ErrCode Executor::setHostFunction(std::unique_ptr<HostFunction> &Func,
                                  const std::string &ModName,
                                  const std::string &FuncName) {
  ErrCode Status = ErrCode::Success;
  auto NewFuncInst = std::make_unique<Instance::FunctionInstance>(true);
  unsigned int NewHostFuncId = 0;
  unsigned int NewFuncInstId = 0;
  Instance::ModuleInstance::FType *FuncType = Func->getFuncType();

  /// Set function instance data.
  if ((Status = NewFuncInst->setNames(ModName, FuncName)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = NewFuncInst->setFuncType(FuncType)) != ErrCode::Success) {
    return Status;
  }

  /// Insert host function to host function manager.
  if ((Status = HostFuncMgr.insertHostFunction(Func, NewHostFuncId)) !=
      ErrCode::Success) {
    return Status;
  }
  if ((Status = NewFuncInst->setHostFuncAddr(NewHostFuncId)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Insert function instance to store manager.
  if ((Status = StoreMgr.insertFunctionInst(NewFuncInst, NewFuncInstId)) !=
      ErrCode::Success) {
    return Status;
  }
  return Status;
}

/// Set start function name. See "include/loader/executor.h".
ErrCode Executor::setStartFuncName(const std::string &Name) {
  StartFunc = Name;
  return ErrCode::Success;
}

/// Set AST Module node to executor. See "include/executor/executor.h".
ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongExecutorFlow;

  /// Get ownership of module.
  Mod = std::move(Module);
  Stat = State::ModuleSet;
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

/// Reset Executor. See "include/loader/executor.h".
ErrCode Executor::reset(bool Force) {
  if (!Force && Stat != State::Finished) {
    return ErrCode::WrongExecutorFlow;
  }
  Mod.reset();
  ModInst = nullptr;
  Engine.reset();
  StackMgr.reset();
  StoreMgr.reset();
  HostFuncMgr.reset();
  Stat = State::Inited;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
