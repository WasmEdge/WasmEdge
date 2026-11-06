// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/spdlog.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// canon lower: call a component function from core code. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runCanonLower(const Component::CanonFunction &Canon,
                                 Span<const ValVariant> Args,
                                 Span<ValVariant> Rets) {
  Runtime::Component::Task *Caller = getCurrentTask(*Canon.getInstance());
  Runtime::Component::Thread *CallerThread =
      getCurrentThread(*Canon.getInstance());
  if (Caller == nullptr || CallerThread == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const auto &Opts = Canon.getOptions();
  auto *Inst = Opts.Inst;
  const auto *Callee = Canon.getCallee();
  const auto *CalleeInst = Callee->getComponentInstance();
  const auto *CalleeTypeInst = Callee->getTypeInstance();
  const auto &FuncType = Callee->getFuncType();

  // A trapped instance tree may not be entered again.
  if (!Callee->isHostFunction() && CalleeInst->getRoot()->isPoisoned()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }

  // The last flat argument is the pointer the results go to when they do
  // not fit the flat results.
  const bool IsMem64 = Canon.isMemory64();
  const auto ParamTypes = FuncType.getParamValTypes();
  const auto ResultTypes = FuncType.getResultValTypes();
  std::vector<ValType> FlatResultTypes;
  for (const auto &Ty : ResultTypes) {
    EXPECTED_TRY(flattenType(*CalleeTypeInst, Ty, IsMem64, FlatResultTypes));
  }
  const bool ResultsIndirect = Opts.Async
                                   ? !FlatResultTypes.empty()
                                   : FlatResultTypes.size() > MaxFlatResults;
  const uint32_t MaxParams = Opts.Async ? MaxFlatAsyncParams : MaxFlatParams;
  std::vector<ValVariant> ArgFlat(Args.begin(), Args.end());
  std::optional<uint64_t> RetPtr;
  if (ResultsIndirect) {
    const auto &Last = ArgFlat.back();
    RetPtr = IsMem64 ? Last.get<uint64_t>()
                     : static_cast<uint64_t>(Last.get<uint32_t>());
    ArgFlat.pop_back();
  }

  std::vector<ValVariant> RetFlat;
  auto OnStart = [this, &Opts, CalleeTypeInst, ParamTypes, ArgFlat,
                  MaxParams]() -> Expect<std::vector<ComponentValVariant>> {
    return liftFlatValues(Opts, *CalleeTypeInst, ParamTypes, ArgFlat,
                          MaxParams);
  };
  // An asynchronous call takes every result through memory.
  const uint32_t MaxResults = Opts.Async ? 0 : MaxFlatResults;
  // An asynchronous call outlives this frame, so it names no local.
  std::vector<ValVariant> *SyncRets = Opts.Async ? nullptr : &RetFlat;
  auto OnResolve =
      [this, &Opts, CalleeTypeInst, ResultTypes, RetPtr, MaxResults,
       SyncRets](std::optional<std::vector<ComponentValVariant>> Vals)
      -> Expect<void> {
    if (!Vals.has_value()) {
      return {};
    }
    std::vector<ValVariant> Unused;
    return lowerFlatValues(Opts, *CalleeTypeInst, ResultTypes, *Vals,
                           MaxResults, SyncRets != nullptr ? *SyncRets : Unused,
                           RetPtr);
  };
  EXPECTED_TRY(Runtime::Component::Task * T,
               liftCall(Callee, std::move(OnStart), std::move(OnResolve),
                        Caller, CallerThread));
  // The implicit thread is forgotten once the callee exits or is aborted.
  auto IsEnded = [T]() {
    return T->isResolved() || T->isAborted() ||
           T->getImplicitThread() == nullptr ||
           T->getImplicitThread()->isEnded();
  };

  if (!Opts.Async) {
    if (!IsEnded()) {
      // The waiting caller pins the callee it reads when it wakes.
      T->addDependent();
      const Runtime::Component::Thread::WakeReason Wake =
          Caller->wait(*CallerThread, IsEnded);
      T->releaseDependent();
      if (Wake == Runtime::Component::Thread::WakeReason::Aborted) {
        T->onCallerAbort();
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      // The instance of the callee went away under it.
      if (T->isAborted()) {
        spdlog::error(ErrCode::Value::ComponentTrap);
        spdlog::error("    the callee was aborted before it returned"sv);
        return Unexpect(ErrCode::Value::ComponentTrap);
      }
    }
    if (const auto Trap = getTrap(Inst->getRoot()->getStoreRoots());
        Trap.has_value()) {
      return Unexpect(*Trap);
    }
    T->releaseLenders();
    for (size_t I = 0; I < Rets.size() && I < RetFlat.size(); ++I) {
      Rets[I] = RetFlat[I];
    }
    return {};
  }

  // An asynchronous call returns the callee state or a subtask handle.
  if (const auto Trap = getTrap(Inst->getRoot()->getStoreRoots());
      Trap.has_value()) {
    return Unexpect(*Trap);
  }
  const uint32_t Code = T->onCollect();
  if (T->isResolved()) {
    Rets[0] = ValVariant(Code);
    return {};
  }
  const uint32_t Idx = Inst->addSubtask(*T);
  T->addDependent();
  Rets[0] = ValVariant((Idx << 4) | Code);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
