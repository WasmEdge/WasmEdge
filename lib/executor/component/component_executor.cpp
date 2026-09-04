// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

/// Instantiate a Component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiateComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

/// Register a named Component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::registerComponent(Runtime::Component::StoreManager &StoreMgr,
                                     const AST::Component::Component &Comp,
                                     std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

/// Register an instantiated Component. See executor.h.
Expect<void> ComponentExecutor::registerComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  return StoreMgr.registerInstance(&CompInst).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  });
}
/// Invoke component function. See "include/executor/component/executor.h".
Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentExecutor::invoke(
    const Runtime::Instance::ComponentFunctionInstance *FuncInst,
    Span<const ComponentValVariant> Params,
    Span<const ComponentValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // TODO: COMPONENT - match the arguments against the function type.
  const auto &ExpectedFuncType = FuncInst->getFuncType();
  const size_t ExpectedArity = ExpectedFuncType.getParamList().size();
  if (Params.size() != ParamTypes.size() || ParamTypes.size() < ExpectedArity) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error("    expected {} argument(s), got {}"sv, ExpectedArity,
                  ParamTypes.size());
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  // Host component functions consume component-level values directly.
  if (FuncInst->isHostFunction()) {
    return FuncInst->getHostFunc()(Params);
  }

  const auto *Parent = FuncInst->getComponentInstance();
  // Poisoned instance tree: after any trap inside it, entries trap.
  if (Parent != nullptr && Parent->getRoot()->concurrency().isPoisoned()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    cannot enter component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  // Host-entry reentrance: no task of the chain may share the instance tree.
  Runtime::Component::Task *Caller = TaskMgr.currentTask();
  for (Runtime::Component::Task *C = Caller; C != nullptr; C = C->CallerTask) {
    if (Parent != nullptr && C->Opts.Inst != nullptr &&
        C->Opts.Inst->getRoot() == Parent->getRoot()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      spdlog::error("    cannot enter component instance"sv);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
  }

  // Collect the argument and result plumbing for the task.
  std::vector<ComponentValVariant> ArgVals(Params.begin(), Params.end());
  std::vector<ComponentValType> ReturnTypes;
  for (const auto &Type : ExpectedFuncType.getResultList()) {
    ReturnTypes.push_back(Type.getValType());
  }
  auto Captured =
      std::make_shared<std::optional<std::vector<ComponentValVariant>>>();
  auto OnStart = [ArgVals = std::move(ArgVals)]() {
    return Expect<std::vector<ComponentValVariant>>(ArgVals);
  };
  auto OnResolve =
      [Captured](std::optional<std::vector<ComponentValVariant>> Results)
      -> Expect<void> {
    if (Results.has_value()) {
      *Captured = std::move(*Results);
    } else {
      // A cancelled host call resolves with no values.
      *Captured = std::vector<ComponentValVariant>{};
    }
    return {};
  };

  // Only the embedder thread may tear down; a task thread would self-join.
  const bool OnEmbedder = TaskMgr.currentThread() == nullptr;
  TaskMgr.enterInvoke();
  auto TaskOrErr =
      liftCall(FuncInst, std::move(OnStart), std::move(OnResolve), Caller);
  if (!TaskOrErr) {
    if (TaskMgr.leaveInvoke() == 0 && OnEmbedder &&
        TaskMgr.trapLatch().has_value()) {
      const auto Err = *TaskMgr.trapLatch();
      TaskMgr.teardown();
      return Unexpect(Err);
    }
    return Unexpect(TaskOrErr.error());
  }
  Runtime::Component::Task *T = *TaskOrErr;

  // Async-typed exports: drive the scheduler until the task resolves.
  if (T->FTAsync && T->St != Runtime::Component::Task::State::Resolved) {
    auto PumpRes = TaskMgr.pumpUntil(
        [T]() { return T->St == Runtime::Component::Task::State::Resolved; });
    if (!PumpRes) {
      TaskMgr.noteTrap(PumpRes.error(), Parent);
      if (TaskMgr.leaveInvoke() == 0 && OnEmbedder) {
        const auto Err = TaskMgr.trapLatch().value_or(PumpRes.error());
        TaskMgr.teardown();
        return Unexpect(Err);
      }
      return Unexpect(PumpRes.error());
    }
  }
  // Drain tasks and threads at the outermost invoke, except suspended ones.
  if (TaskMgr.leaveInvoke() == 0 && OnEmbedder && !TaskMgr.hasParkedThreads()) {
    TaskMgr.teardown();
  }

  if (!Captured->has_value()) {
    spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
    spdlog::error("    async-lifted export failed to produce a result"sv);
    return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
  }
  auto &Results = **Captured;
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Returns;
  Returns.reserve(Results.size());
  for (size_t I = 0; I < Results.size() && I < ReturnTypes.size(); ++I) {
    Returns.emplace_back(std::move(Results[I]), ReturnTypes[I]);
  }
  return Returns;
}

} // namespace Executor
} // namespace WasmEdge
