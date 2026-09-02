// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errinfo.h"
#include "common/spdlog.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Instantiate a component. See executor.h.
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiateComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}

// Register a named component. See executor.h.
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::registerComponent(Runtime::Component::StoreManager &StoreMgr,
                                     const AST::Component::Component &Comp,
                                     std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

// Register an instantiated component. See executor.h.
Expect<void> ComponentExecutor::registerComponent(
    Runtime::Component::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  StoreMgr.registerInstance(&CompInst);
  return {};
}

// Run one embedder entry. See executor.h.
Expect<void> ComponentExecutor::runEntry(
    const std::function<Expect<void>()> &Body) noexcept {
  // Only the scheduler thread may tear down; a task thread would self-join.
  const bool OnScheduler = TaskMgr.getCurrentThread() == nullptr;
  EntryDepth += 1;
  auto Res = Body();
  EntryDepth -= 1;
  if (EntryDepth != 0 || !OnScheduler) {
    return Res;
  }
  const auto Trap = TaskMgr.getTrapLatch();
  if (Trap.has_value() || !TaskMgr.hasParkedThreads()) {
    TaskMgr.teardown();
  }
  if (!Res && Trap.has_value()) {
    return Unexpect(*Trap);
  }
  return Res;
}

// Invoke a component function. See executor.h.
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
  const auto DeclaredTypes = FuncType.getParamValTypes();
  // A caller that passes values without types, such as the spec-test
  // harness, takes the parameter types from the type of the function.
  if (ParamTypes.empty() && Params.size() == DeclaredTypes.size()) {
    ParamTypes = DeclaredTypes;
  }
  const std::vector<ComponentValType> GivenTypes(ParamTypes.begin(),
                                                 ParamTypes.end());
  if (Params.size() != GivenTypes.size() ||
      GivenTypes.size() != DeclaredTypes.size()) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(DeclaredTypes, GivenTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  // The types and values the embedder hands over are the declared ones; the
  // lowering and the host function read the values unchecked.
  Component::LiftLowerContext Ctx{FuncInst->getCanonOptions(), this};
  for (size_t I = 0; I < DeclaredTypes.size(); ++I) {
    if (!Component::LiftLowerContext::valTypeEq(
            Ctx.getInstance(), DeclaredTypes[I], Ctx.getInstance(),
            GivenTypes[I])) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error(ErrInfo::InfoMismatch(DeclaredTypes, GivenTypes));
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    EXPECTED_TRY(Ctx.checkValue(Params[I], DeclaredTypes[I]));
  }

  const auto *Parent = FuncInst->getComponentInstance();
  // Poisoned instance tree: after any trap inside it, entries trap.
  if (Parent != nullptr && Parent->getRoot()->isPoisoned()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  // Host-entry reentrance: no task of the chain may share the instance tree.
  Runtime::Component::Task *Caller = TaskMgr.getCurrentTask();
  for (Runtime::Component::Task *C = Caller; C != nullptr; C = C->CallerTask) {
    if (Parent != nullptr && C->Opts.Inst != nullptr &&
        C->Opts.Inst->getRoot() == Parent->getRoot()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
  }

  // Collect the argument and result plumbing for the task.
  std::vector<ComponentValVariant> ArgVals(Params.begin(), Params.end());
  const auto ReturnTypes = FuncType.getResultValTypes();
  auto Captured =
      std::make_shared<std::optional<std::vector<ComponentValVariant>>>();
  auto Cancelled = std::make_shared<bool>(false);
  auto OnStart = [ArgVals = std::move(ArgVals)]() {
    return Expect<std::vector<ComponentValVariant>>(ArgVals);
  };
  auto OnResolve = [Captured, Cancelled](
                       std::optional<std::vector<ComponentValVariant>> Results)
      -> Expect<void> {
    if (Results.has_value()) {
      *Captured = std::move(*Results);
    } else {
      // A cancelled host call resolves with no values.
      *Captured = std::vector<ComponentValVariant>{};
      *Cancelled = true;
    }
    return {};
  };

  EXPECTED_TRY(runEntry([&]() -> Expect<void> {
    EXPECTED_TRY(auto *T, liftCall(FuncInst, std::move(OnStart),
                                   std::move(OnResolve), Caller));
    // A task on its own thread parked before resolving: run the scheduler
    // until it resolves.
    if (T->Status != Runtime::Component::Task::State::Resolved) {
      auto PumpRes = pumpUntil(
          [T]() {
            return T->Status == Runtime::Component::Task::State::Resolved;
          },
          T->Thread);
      if (!PumpRes) {
        TaskMgr.noteTrap(PumpRes.error(), Parent);
        return Unexpect(PumpRes.error());
      }
    }
    return {};
  }));

  if (!Captured->has_value()) {
    spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
    return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
  }
  auto &Results = **Captured;
  if (!*Cancelled && Results.size() != ReturnTypes.size()) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error("    expected {} result(s), got {}"sv, ReturnTypes.size(),
                  Results.size());
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Returns;
  Returns.reserve(Results.size());
  for (size_t I = 0; I < Results.size(); ++I) {
    Returns.emplace_back(std::move(Results[I]), ReturnTypes[I]);
  }
  return Returns;
}

} // namespace Executor
} // namespace WasmEdge
