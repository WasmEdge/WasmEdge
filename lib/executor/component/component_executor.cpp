// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "system/stacktrace.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

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
  const auto &TypeInst = *FuncInst->getTypeInstance();
  const auto ExpTypes = FuncType.getParamValTypes();

  if (Params.size() != ExpTypes.size() ||
      (!ParamTypes.empty() && ParamTypes.size() != ExpTypes.size())) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(ExpTypes.size(), Params.size()));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  for (size_t I = 0; I < ExpTypes.size(); ++I) {
    if (!ParamTypes.empty()) {
      EXPECTED_TRY(const bool Same, matchValType(TypeInst, ExpTypes[I],
                                                 TypeInst, ParamTypes[I]));
      if (!Same) {
        spdlog::error(ErrCode::Value::FuncSigMismatch);
        spdlog::error(ErrInfo::InfoMismatch(
            ExpTypes, std::vector<ComponentValType>(ParamTypes.begin(),
                                                    ParamTypes.end())));
        return Unexpect(ErrCode::Value::FuncSigMismatch);
      }
    }
    EXPECTED_TRY(checkHostValue(TypeInst, ExpTypes[I], Params[I]));
  }

  std::vector<ComponentValVariant> Results;
  bool Cancelled = false;
  auto &Root = *Inst->getRoot();
  EXPECTED_TRY(runEntry(Root, /*IsInstantiate=*/false, [&]() -> Expect<void> {
    // A trapped instance tree rejects every further entry.
    if (Inst->getRoot()->isPoisoned()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
    EXPECTED_TRY(Runtime::Component::Task * T,
                 liftCall(
                     FuncInst,
                     [&Params]() -> Expect<std::vector<ComponentValVariant>> {
                       return std::vector<ComponentValVariant>(Params.begin(),
                                                               Params.end());
                     },
                     [this, &Results, &Cancelled, &Root](
                         std::optional<std::vector<ComponentValVariant>> Vals)
                         -> Expect<void> {
                       if (Vals.has_value()) {
                         // The embedder names an end a result carries by its
                         // handle in the host table of the root, as a host
                         // does.
                         Results = std::move(*Vals);
                         for (auto &Val : Results) {
                           EXPECTED_TRY(lowerHostValue(Root, Val));
                         }
                       } else {
                         Cancelled = true;
                       }
                       return {};
                     },
                     nullptr, nullptr));
    // The embedder thread pumps until the activation resolves, by any of
    // its threads; an async callback loop lives on after it returned.
    return pump(
        Root, [T]() { return T->isResolved() || T->isAborted(); },
        T->isFuncTypeAsync() ? nullptr : T->getImplicitThread(), nullptr);
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
Expect<void>
ComponentExecutor::runEntry(Runtime::Instance::ComponentInstance &Root,
                            bool IsInstantiate,
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
  Root.Instantiating = IsInstantiate;
  InEntry = true;
  auto Res = Body();
  Root.Instantiating = false;
  const auto Roots = Root.EntryStores;
  // The host tasks the entry left ready take their turn, but for those a
  // guest call nests.
  if (!getTrap(Roots).has_value()) {
    while (Runtime::Component::Thread *Next = findReadyThread(
               Roots, [this](const Runtime::Component::Thread &Parked) {
                 return getGuestThread(Parked) == nullptr;
               })) {
      resumeThread(*Next);
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
Runtime::Component::Thread *ComponentExecutor::newRootThread(
    Runtime::Instance::ComponentInstance &Store, Runtime::Component::Task &T,
    std::function<void(Runtime::Component::Thread &)> Body) {
  Runtime::Component::Thread *Stack =
      Store.newThread(T, [this, &Store, Run = std::move(Body)]() {
        InEntry = true;
        Runtime::Component::Thread *Cur = getCurrentThread(Store);
        assuming(Cur != nullptr);
        Run(*Cur);
      });
  // A thread that is ended at birth got no stack from the host.
  return Stack->isEnded() ? nullptr : Stack;
}

// Call a core function on the current stack. See
// "include/executor/component/executor.h".
Expect<std::vector<ValVariant>>
ComponentExecutor::invokeCore(const Runtime::Instance::FunctionInstance *Func,
                              Span<const ValVariant> Args) {
  // A nested call adds frames to the native stack of its caller: the last
  // quarter of it, and at least StackMargin, stays free for the trap.
  static constexpr const uintptr_t StackMargin = UINT64_C(64) << 10;
#if __has_builtin(__builtin_stack_address)
  const auto SP = reinterpret_cast<uintptr_t>(__builtin_stack_address());
#elif defined(__GNUC__) || defined(__clang__)
  const auto SP = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
#elif defined(_MSC_VER)
  const auto SP = reinterpret_cast<uintptr_t>(_AddressOfReturnAddress());
#else
  volatile char StackProbe;
  const auto SP = reinterpret_cast<uintptr_t>(&StackProbe);
#endif
  const auto [Low, High] = getThreadStackBounds();
  const bool OnThreadStack = Low < SP && SP <= High;
  if (unlikely(OnThreadStack &&
               SP - Low < std::max(StackMargin, (High - Low) / 4))) {
    spdlog::error(ErrCode::Value::CallStackExhausted);
    return Unexpect(ErrCode::Value::CallStackExhausted);
  }
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
