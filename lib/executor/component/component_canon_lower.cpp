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
  Runtime::Component::Task *Caller = getCurrentTask();
  Runtime::Component::Thread *CallerThread = getCurrentThread();
  if (Caller == nullptr || CallerThread == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    a canonical built-in ran outside any task"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const auto &Opts = Canon.getOptions();
  const auto *Inst = Opts.Inst;
  const auto *Callee = Canon.getCallee();
  const auto *CalleeInst = Callee->getComponentInstance();
  const auto *CalleeTypeInst = Callee->getTypeInstance();
  const auto &FuncType = Callee->getFuncType();

  // The instance tree of the callee may be entered: not a lexical relative,
  // and not trapped.
  if (!Callee->isHostFunction() && (Inst->isLinealRelative(CalleeInst) ||
                                    CalleeInst->getRoot()->isPoisoned())) {
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

  // The callee lifts its arguments out of the caller when it starts and
  // lowers its results into the caller when it resolves.
  std::vector<ValVariant> RetFlat;
  auto OnStart = [this, &Opts, CalleeTypeInst, ParamTypes, ArgFlat,
                  MaxParams]() -> Expect<std::vector<ComponentValVariant>> {
    return liftValues(Opts, *CalleeTypeInst, ParamTypes, ArgFlat, MaxParams);
  };
  // An asynchronous call takes every result through memory.
  const uint32_t MaxResults = Opts.Async ? 0 : MaxFlatResults;
  auto OnResolve =
      [this, &Opts, CalleeTypeInst, ResultTypes, RetPtr, MaxResults,
       &RetFlat](std::optional<std::vector<ComponentValVariant>> Vals)
      -> Expect<void> {
    if (!Vals.has_value()) {
      return {};
    }
    return lowerValues(Opts, *CalleeTypeInst, ResultTypes, *Vals, MaxResults,
                       RetFlat, RetPtr);
  };
  // A synchronous call of an async-typed guest callee blocks until the callee
  // resolves, which the caller must be allowed to do before it starts.
  if (!Opts.Async && FuncType.isAsync() && !Callee->isHostFunction()) {
    EXPECTED_TRY(checkMayBlock(*Caller, *CallerThread));
  }
  EXPECTED_TRY(
      Runtime::Component::Task * T,
      liftCall(Callee, std::move(OnStart), std::move(OnResolve), Caller));
  Runtime::Component::Thread *CalleeThread = T->getImplicitThread();
  auto IsEnded = [T, CalleeThread]() {
    return T->isResolved() || CalleeThread == nullptr ||
           CalleeThread->isEnded();
  };

  if (!Opts.Async) {
    // A synchronous call waits for the callee to resolve, if it may block.
    if (!IsEnded()) {
      // A host callee runs no guest code: waiting for it is never a block
      // that another thread of the tree has to cover.
      if (!Callee->isHostFunction()) {
        EXPECTED_TRY(checkMayBlock(*Caller, *CallerThread));
      }
      const Runtime::Component::Thread::Reason Wake =
          Caller->wait(*CallerThread, IsEnded, /*Cancellable=*/false);
      if (Wake == Runtime::Component::Thread::Reason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
    }
    if (Trap.has_value()) {
      return Unexpect(*Trap);
    }
    T->releaseLenders();
    for (size_t I = 0; I < Rets.size() && I < RetFlat.size(); ++I) {
      Rets[I] = RetFlat[I];
    }
    return {};
  }

  // An asynchronous call returns the state of the callee: resolved already,
  // or a subtask handle to wait on.
  if (Trap.has_value()) {
    return Unexpect(*Trap);
  }
  if (T->isResolved()) {
    T->releaseLenders();
    Rets[0] = ValVariant(T->getSubtaskCode());
    return {};
  }
  T->setDeliveredCode(T->getSubtaskCode());
  const uint32_t Idx = Inst->addSubtask(findTask(T));
  Rets[0] = ValVariant((Idx << 4) | T->getSubtaskCode());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
