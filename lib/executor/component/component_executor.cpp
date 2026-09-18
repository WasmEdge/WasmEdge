// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

thread_local bool ComponentExecutor::InEntry = false;

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
  return StoreMgr.registerInstance(&CompInst).map_error([](ErrCode Err) {
    spdlog::error(Err);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Err;
  });
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
  auto *Inst = FuncInst->getComponentInstance();
  const auto ExpTypes = FuncType.getParamValTypes();

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
    EXPECTED_TRY(checkHostValue(*Inst, ExpTypes[I], Params[I]));
  }

  std::vector<ComponentValVariant> Results;
  bool Cancelled = false;
  auto &Root = *Inst->getRoot();
  EXPECTED_TRY(runEntry(
      Root, Runtime::Instance::ComponentInstance::EntryKind::Invoke,
      [&]() -> Expect<void> {
        // A trapped instance tree rejects every further entry.
        if (Inst->getRoot()->isPoisoned()) {
          spdlog::error(ErrCode::Value::ComponentCannotEnter);
          return Unexpect(ErrCode::Value::ComponentCannotEnter);
        }
        EXPECTED_TRY(
            Runtime::Component::Task * T,
            liftCall(
                FuncInst,
                [&Params]() -> Expect<std::vector<ComponentValVariant>> {
                  return std::vector<ComponentValVariant>(Params.begin(),
                                                          Params.end());
                },
                [this, &Results, &Cancelled,
                 &Root](std::optional<std::vector<ComponentValVariant>> Vals)
                    -> Expect<void> {
                  if (Vals.has_value()) {
                    // The embedder names an end a result carries by its
                    // handle in the table of the root, as a host does.
                    Results = std::move(*Vals);
                    for (auto &Val : Results) {
                      EXPECTED_TRY(lowerHostStreams(Root, Val));
                    }
                  } else {
                    Cancelled = true;
                  }
                  return {};
                },
                nullptr, nullptr));
        // The embedder thread pumps until the activation resolves or ends: an
        // async callback loop lives on after it returned.
        Runtime::Component::Thread *Stack = T->getImplicitThread();
        return pump(
            Root,
            [T, Stack]() {
              return T->isResolved() || T->isAborted() || Stack == nullptr ||
                     Stack->isEnded();
            },
            T->isFuncTypeAsync() ? nullptr : Stack);
      }));

  std::vector<std::pair<ComponentValVariant, ComponentValType>> Returns;
  if (!Cancelled) {
    const auto RetTypes = FuncType.getResultValTypes();
    if (Results.size() != RetTypes.size()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error(ErrInfo::InfoMismatch(RetTypes.size(), Results.size()));
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
Expect<void> ComponentExecutor::runEntry(
    Runtime::Instance::ComponentInstance &Root,
    Runtime::Instance::ComponentInstance::EntryKind Kind,
    const std::function<Expect<void>()> &Body) {
  // An OS thread runs one entry at a time.
  if (InEntry) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    entered from within an entry"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  // The entry locks every store it runs, in address order.
  auto Stores = Root.getStoreRoots();
  std::vector<std::unique_lock<std::mutex>> Locks;
  Locks.reserve(Stores.size());
  for (auto *Store : Stores) {
    Locks.emplace_back(Store->EntryMutex);
  }
  for (auto *Store : Stores) {
    Store->EntryRoot = &Root;
  }
  Root.EntryStores = std::move(Stores);
  Root.Entry = Kind;
  InEntry = true;
  auto Res = Body();
  Root.Entry = Runtime::Instance::ComponentInstance::EntryKind::None;
  const auto Roots = Root.EntryStores;
  // The host tasks the entry left ready take their turn.
  if (!getTrap(Roots).has_value()) {
    while (Runtime::Component::Thread *Next = findReadyThread(
               Roots, [](const Runtime::Component::Thread &Parked) {
                 return Parked.getOwner().isHost();
               })) {
      resumeThread(*Next, Runtime::Component::Thread::WakeReason::Normal);
    }
  }
  // A store that trapped is torn down; an orderly termination poisons none.
  const std::optional<ErrCode> Trap = getTrap(Roots);
  for (auto *Store : Roots) {
    if (Store->Trap.has_value()) {
      Store->abortTasks(Store->Trap->getEnum() != ErrCode::Value::Terminated);
      Store->Trap.reset();
    }
  }
  for (auto *Store : Roots) {
    Store->drainEndedTasks();
    Store->drainClosedStreams();
    Store->EntryRoot = nullptr;
  }
  InEntry = false;
  Root.EntryStores.clear();
  if (Trap.has_value() && Res) {
    return Unexpect(*Trap);
  }
  return Res;
}

// A root thread. See "include/executor/component/executor.h".
Runtime::Component::Thread *
ComponentExecutor::newRootThread(Runtime::Instance::ComponentInstance &Store,
                                 Runtime::Component::Task &T,
                                 std::function<void()> Body) {
  return Store.newThread(T, [Run = std::move(Body)]() {
    InEntry = true;
    Run();
  });
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

// Latch a trap. See "include/executor/component/executor.h".
void ComponentExecutor::setTrap(
    ErrCode Err, const Runtime::Instance::ComponentInstance &Inst) noexcept {
  if (Err.getEnum() == ErrCode::Value::ComponentAsyncAborted) {
    return;
  }
  auto *Root = Inst.getRoot();
  if (!Root->Trap.has_value()) {
    Root->Trap = Err;
  }
}

// The first latched trap. See "include/executor/component/executor.h".
std::optional<ErrCode> ComponentExecutor::getTrap(
    Span<Runtime::Instance::ComponentInstance *const> Roots) const noexcept {
  for (const auto *Store : Roots) {
    if (Store->Trap.has_value()) {
      return Store->Trap;
    }
  }
  return std::nullopt;
}

} // namespace Executor
} // namespace WasmEdge
