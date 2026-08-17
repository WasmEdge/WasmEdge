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
Component::Executor::instantiateComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

/// Register a named Component. See "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Component::Executor::registerComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp, std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

/// Register an instantiated Component. See
/// "include/executor/component/executor.h".
Expect<void> Component::Executor::registerComponent(
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
Component::Executor::invoke(
    const Runtime::Instance::Component::FunctionInstance *FuncInst,
    Span<const ComponentValVariant> Params,
    Span<const ComponentValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Matching arguments and function type.
  // TODO: COMPONENT - type matching.
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
  if (Parent != nullptr && Parent->getRoot()->concurrency().Poisoned) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    cannot enter component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  // Host-entry reentrance. No task of the current chain can already belong
  // to the same instance tree.
  Component::Task *Caller = AsyncRt.currentTask();
  for (Component::Task *C = Caller; C != nullptr; C = C->CallerTask) {
    if (Parent != nullptr && C->Inst != nullptr &&
        C->Inst->getRoot() == Parent->getRoot()) {
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

  // Teardown aborts the parked vehicles. Only the embedder thread can do it,
  // because on a vehicle it joins its own thread.
  const bool OnEmbedder = AsyncRt.currentVehicle() == nullptr;
  AsyncRt.InvokeDepth += 1;
  auto TaskOrErr = AsyncRt.liftCall(FuncInst, std::move(OnStart),
                                    std::move(OnResolve), Caller);
  if (!TaskOrErr) {
    AsyncRt.InvokeDepth -= 1;
    if (AsyncRt.InvokeDepth == 0 && OnEmbedder &&
        AsyncRt.trapLatch().has_value()) {
      const auto Err = *AsyncRt.trapLatch();
      AsyncRt.teardown();
      return Unexpect(Err);
    }
    return Unexpect(TaskOrErr.error());
  }
  Component::Task *T = *TaskOrErr;

  // Async-typed exports: drive the scheduler until the task resolves.
  if (T->FTAsync && T->St != Component::Task::State::Resolved) {
    auto PumpRes = AsyncRt.pumpUntil(
        [T]() { return T->St == Component::Task::State::Resolved; });
    if (!PumpRes) {
      AsyncRt.noteTrap(PumpRes.error(), Parent);
      AsyncRt.InvokeDepth -= 1;
      if (AsyncRt.InvokeDepth == 0 && OnEmbedder) {
        const auto Err = AsyncRt.trapLatch().value_or(PumpRes.error());
        AsyncRt.teardown();
        return Unexpect(Err);
      }
      return Unexpect(PumpRes.error());
    }
  }
  AsyncRt.InvokeDepth -= 1;
  // At the outermost embedder invoke, drain the tasks and vehicles left
  // behind, so none of them outlives the call and sees torn-down state. A
  // thread parked by thread.suspend is the exception: it belongs to the
  // component instance, not to the call, and a later call may resume it.
  if (AsyncRt.InvokeDepth == 0 && OnEmbedder && !AsyncRt.hasParkedThreads()) {
    AsyncRt.teardown();
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
