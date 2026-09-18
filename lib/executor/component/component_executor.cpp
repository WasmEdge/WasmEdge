// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Instantiate a component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiateComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

// Register a named component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::registerComponent(Runtime::Component::StoreManager &StoreMgr,
                                     const AST::Component::Component &Comp,
                                     std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

// Register an instantiated component. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::registerComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  StoreMgr.registerInstance(&CompInst);
  return {};
}

// Invoke a component function. See "include/executor/component/executor.h".
Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentExecutor::invoke(
    const Runtime::Instance::ComponentFunctionInstance *FuncInst,
    Span<const ComponentValVariant> Params,
    Span<const ComponentValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  const auto &FuncType = FuncInst->getFuncType();
  const auto *Inst = FuncInst->getComponentInstance();
  const auto ExpTypes = FuncType.getParamValTypes();

  // Match the arguments and the function type.
  if (Params.size() != ExpTypes.size() ||
      (!ParamTypes.empty() && ParamTypes.size() != ExpTypes.size())) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(ExpTypes.size(), Params.size()));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  for (size_t I = 0; I < ExpTypes.size(); ++I) {
    if (!ParamTypes.empty()) {
      EXPECTED_TRY(const bool Same,
                   matchValType(*Inst, ExpTypes[I], *Inst, ParamTypes[I]));
      if (!Same) {
        spdlog::error(ErrCode::Value::FuncSigMismatch);
        spdlog::error(ErrInfo::InfoMismatch(
            ExpTypes, std::vector<ComponentValType>(ParamTypes.begin(),
                                                    ParamTypes.end())));
        return Unexpect(ErrCode::Value::FuncSigMismatch);
      }
    }
    EXPECTED_TRY(checkValue(*Inst, ExpTypes[I], Params[I]));
  }

  // A trapped instance tree rejects every further entry.
  if (Inst->getRoot()->isPoisoned()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }

  std::vector<ComponentValVariant> Results;
  bool Cancelled = false;
  EXPECTED_TRY(runEntry([&]() -> Expect<void> {
    EXPECTED_TRY(Runtime::Component::Task * T,
                 liftCall(
                     FuncInst,
                     [&Params]() -> Expect<std::vector<ComponentValVariant>> {
                       return std::vector<ComponentValVariant>(Params.begin(),
                                                               Params.end());
                     },
                     [&Results, &Cancelled](
                         std::optional<std::vector<ComponentValVariant>> Vals)
                         -> Expect<void> {
                       if (Vals.has_value()) {
                         Results = std::move(*Vals);
                       } else {
                         Cancelled = true;
                       }
                       return {};
                     },
                     nullptr));
    // The embedder thread pumps until the activation resolves or ends: an
    // async callback loop lives on after it returned.
    Runtime::Component::Thread *Root = T->getImplicitThread();
    return pump(
        [T, Root]() {
          return T->isResolved() || Root == nullptr || Root->isEnded();
        },
        T->isFuncTypeAsync() ? nullptr : Root);
  }));

  std::vector<std::pair<ComponentValVariant, ComponentValType>> Returns;
  if (!Cancelled) {
    const auto RetTypes = FuncType.getResultValTypes();
    if (Results.size() != RetTypes.size()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error(
          ErrInfo::InfoMismatch(RetTypes.size(), Results.size()));
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    Returns.reserve(Results.size());
    for (size_t I = 0; I < Results.size(); ++I) {
      Returns.emplace_back(std::move(Results[I]), RetTypes[I]);
    }
  }
  return Returns;
}

// Run one embedder entry. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runEntry(const std::function<Expect<void>()> &Body) {
  EntryDepth += 1;
  auto Res = Body();
  EntryDepth -= 1;
  if (EntryDepth > 0) {
    return Res;
  }
  // A latched trap aborts everything still parked; otherwise the host tasks
  // the entry left ready take their turn, since they run no guest code.
  if (!Trap.has_value()) {
    while (Runtime::Component::Thread *Next =
               findReadyThread([](const Runtime::Component::Thread &Parked) {
                 return Parked.getOwner().isHost();
               })) {
      resume(*Next, Runtime::Component::Thread::Reason::Normal);
    }
  }
  if (Trap.has_value()) {
    const ErrCode Err = *Trap;
    cleanup();
    if (Res) {
      return Unexpect(Err);
    }
    return Res;
  }
  drainEndedTasks();
  return Res;
}

// Call a core function on the current stack. See
// "include/executor/component/executor.h".
Expect<std::vector<ValVariant>>
ComponentExecutor::invokeCore(const Runtime::Instance::FunctionInstance *Func,
                              Span<const ValVariant> Args) {
  EXPECTED_TRY(auto Rets,
               Core.invoke(Func, Args, Func->getFuncType().getParamTypes()));
  std::vector<ValVariant> Vals;
  Vals.reserve(Rets.size());
  for (auto &[Val, Type] : Rets) {
    Vals.push_back(Val);
  }
  return Vals;
}

// Own a task. See "include/executor/component/executor.h".
Runtime::Component::Task *
ComponentExecutor::addTask(std::shared_ptr<Runtime::Component::Task> T) {
  T->setWakeUp(WakeUp);
  OwnedTasks.push_back(std::move(T));
  return OwnedTasks.back().get();
}

// The owning handle of a task. See "include/executor/component/executor.h".
std::shared_ptr<Runtime::Component::Task>
ComponentExecutor::findTask(Runtime::Component::Task *T) const noexcept {
  for (const auto &Owned : OwnedTasks) {
    if (Owned.get() == T) {
      return Owned;
    }
  }
  return nullptr;
}

// A thread on a stack of its own. See "include/executor/component/executor.h".
Runtime::Component::Thread *
ComponentExecutor::newThread(Runtime::Component::Task &T,
                             Runtime::Component::Thread::Body Body) {
  OwnedThreads.push_back(
      std::make_unique<Runtime::Component::Thread>(T, std::move(Body)));
  return OwnedThreads.back().get();
}

// A thread on the current stack. See "include/executor/component/executor.h".
Runtime::Component::Thread *
ComponentExecutor::newNestedThread(Runtime::Component::Task &T) {
  Runtime::Component::Thread *Outer = getCurrentThread();
  assuming(Outer != nullptr);
  OwnedThreads.push_back(
      std::make_unique<Runtime::Component::Thread>(T, *Outer));
  return OwnedThreads.back().get();
}

// Latch a trap. See "include/executor/component/executor.h".
void ComponentExecutor::setTrap(
    ErrCode Err, const Runtime::Instance::ComponentInstance *Inst) noexcept {
  if (Err.getEnum() == ErrCode::Value::ComponentAsyncAborted) {
    return;
  }
  if (!Trap.has_value()) {
    Trap = Err;
  }
  // An orderly termination is no trap: the tree stays usable.
  if (Inst != nullptr && Err.getEnum() != ErrCode::Value::Terminated) {
    Inst->getRoot()->setPoisoned();
  }
}

// Abort everything. See "include/executor/component/executor.h".
void ComponentExecutor::cleanup() noexcept {
  // The unwinding of one stack can post work, so the scan restarts until
  // every stack has ended.
  bool Progress = true;
  while (Progress) {
    Progress = false;
    for (size_t I = 0; I < OwnedThreads.size(); ++I) {
      Runtime::Component::Thread *Root = OwnedThreads[I].get();
      if (!Root->isRoot() || Root->isEnded()) {
        continue;
      }
      Runtime::Component::Thread *Top = Root->getInnermost();
      if (Top == nullptr || Top->isEnded() || Top->isRunning()) {
        continue;
      }
      Top->onResume(Runtime::Component::Thread::Reason::Abort);
      Progress = true;
    }
  }
  // An aborted thread never left its instance's thread table.
  for (auto &Parked : OwnedThreads) {
    if (Parked->getIndex() != 0) {
      const auto *Inst = Parked->getOwner().getInstance();
      if (Inst != nullptr) {
        Inst->removeThread(Parked->getIndex());
      }
      Parked->setIndex(0);
    }
  }
  Waiting.clear();
  Current = nullptr;
  OwnedThreads.clear();
  OwnedTasks.clear();
  Trap.reset();
}

// Drop the ended tasks and threads. See
// "include/executor/component/executor.h".
void ComponentExecutor::drainEndedTasks() noexcept {
  Waiting.erase(std::remove_if(Waiting.begin(), Waiting.end(),
                               [](const Runtime::Component::Thread *Parked) {
                                 return Parked->isEnded();
                               }),
                Waiting.end());
  OwnedThreads.erase(
      std::remove_if(
          OwnedThreads.begin(), OwnedThreads.end(),
          [](const std::unique_ptr<Runtime::Component::Thread> &Parked) {
            return Parked->isEnded();
          }),
      OwnedThreads.end());
  // A task lives while a thread runs for it or a subtask handle names it.
  OwnedTasks.erase(
      std::remove_if(OwnedTasks.begin(), OwnedTasks.end(),
                     [](const std::shared_ptr<Runtime::Component::Task> &T) {
                       return T.use_count() == 1 && T->isResolved() &&
                              T->getThreads().empty();
                     }),
      OwnedTasks.end());
}

} // namespace Executor
} // namespace WasmEdge
