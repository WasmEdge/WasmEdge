// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- component_lower_thunk.cpp - canon lower core function -------------===//
//
// Runtime behavior of `canon lower` for the four {sync,async} combinations.
// The callee runs as a task.
//
//===----------------------------------------------------------------------===//

#include "executor/component/async_runtime.h"
#include "executor/component/executor.h"
#include "executor/component/lower_thunk.h"
#include "executor/executor.h"

#include "common/spdlog.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Executor {
namespace Component {

using namespace std::literals;
namespace AsyncComp = Runtime::Instance::Component;

namespace {

// Release the argument lends taken by the borrows lifted for this call.
void deliverSubtaskResolve(AsyncComp::SubtaskObj &Sub) noexcept {
  if (Sub.Delivered) {
    return;
  }
  for (const auto &[Inst, Idx] : Sub.Lenders) {
    if (auto *Slot = Inst->handles().handleGet(Idx);
        Slot != nullptr && Slot->Lends > 0) {
      Slot->Lends -= 1;
    }
  }
  Sub.Lenders.clear();
  Sub.Delivered = true;
}

// Queue (or refresh) the SUBTASK progress event once the subtask has a
// handle in the caller's table.
void noteSubtaskProgress(AsyncComp::SubtaskObj *Sub) noexcept {
  if (!Sub->TableIdx.has_value()) {
    return;
  }
  const uint32_t Idx = *Sub->TableIdx;
  Sub->setPendingEvent([Sub, Idx]() -> AsyncComp::AsyncEvent {
    if (Sub->resolved()) {
      deliverSubtaskResolve(*Sub);
    }
    return {AsyncComp::AsyncEventCode::Subtask, Idx,
            static_cast<uint32_t>(Sub->State)};
  });
}

} // namespace

CanonLowerHostFunc::CanonLowerHostFunc(
    Executor *ExecIn, const CanonicalABI::FlatFuncType &FlatSig,
    Runtime::Instance::Component::FunctionInstance *CalleeIn,
    Runtime::Instance::MemoryInstance *MemoryIn,
    Runtime::Instance::FunctionInstance *ReallocIn,
    const Runtime::Instance::ComponentInstance *CompInstIn,
    StringEncoding EncIn, bool AsyncLowerIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Callee(CalleeIn),
      Memory(MemoryIn), Realloc(ReallocIn), CompInst(CompInstIn),
      HasOutPtr(false), ParamSlotCount(0), Enc(EncIn),
      AsyncLower(AsyncLowerIn) {
  // The trailing out-pointer exists when a sync lower spills its results, or
  // when an async lower carries any result.
  const bool CalleeHasResults =
      !CalleeIn->getFuncType().getResultList().empty();
  if (AsyncLowerIn) {
    HasOutPtr = CalleeHasResults;
  } else {
    HasOutPtr = FlatSig.Results.empty() && CalleeHasResults;
  }
  ParamSlotCount =
      static_cast<uint32_t>(FlatSig.Params.size()) - (HasOutPtr ? 1U : 0U);
  // Populate DefType from the pre-flighted flat ABI signature.
  auto &FT = DefType.getCompositeType().getFuncType();
  auto &Params = FT.getParamTypes();
  auto &Returns = FT.getReturnTypes();
  Params.reserve(FlatSig.Params.size());
  Returns.reserve(FlatSig.Results.size());
  for (const auto &P : FlatSig.Params) {
    Params.push_back(P);
  }
  for (const auto &R : FlatSig.Results) {
    Returns.push_back(R);
  }
}

Expect<void> CanonLowerHostFunc::run(const Runtime::CallingFrame &,
                                     Span<const ValVariant> Args,
                                     Span<ValVariant> Rets) {
  auto &Rt = Exec->getAsyncRuntime();
  // Lowered imports are unreachable from the argument-lowering and
  // post-return regions.
  if (CompInst != nullptr && !CompInst->concurrency().MayLeave) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    spdlog::error("    cannot leave component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }

  const auto *CalleeComp = Callee->getComponentInstance();
  const bool GuestCallee = !Callee->isHostFunction();
  // Reentrance rule for adapters: calls between an instance and itself or a
  // lexical relative trap unconditionally.
  if (GuestCallee && CompInst != nullptr && CalleeComp != nullptr &&
      CalleeComp->isLinealRelativeOf(CompInst)) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    cannot enter component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }

  const bool CalleeAsyncType = Callee->getFuncType().isAsync();
  Task *CallerTask = Rt.currentTask();
  if (!AsyncLower && CalleeAsyncType && GuestCallee &&
      (CallerTask == nullptr || !Rt.mayBlock(*CallerTask))) {
    // A task can call an async-typed function through a sync lower only when
    // the task can block to wait for the resolution.
    spdlog::error(ErrCode::Value::ComponentCannotBlockSync);
    spdlog::error("    cannot block a synchronous task before returning"sv);
    return Unexpect(ErrCode::Value::ComponentCannotBlockSync);
  }

  // Caller-side lift and lower context. The callee type indices are its own,
  // but the handle tables stay with the caller.
  auto MakeCallerCx = [this]() {
    CanonicalABI::CanonCtx Cx{Exec, Memory, Realloc, CompInst,
                              {},   {},     nullptr, Enc};
    Cx.CrossComponent = true;
    if (const auto *CC = Callee->getComponentInstance();
        CC != nullptr && CC != CompInst) {
      Cx.TypeResolver = [CC](uint32_t I) { return CC->getType(I); };
      Cx.ResourceResolver = [CC](uint32_t I) { return CC->getTypeResource(I); };
    }
    return Cx;
  };

  const auto &CFT = Callee->getFuncType();
  std::vector<ComponentValType> ParamTypes;
  ParamTypes.reserve(CFT.getParamList().size());
  for (const auto &P : CFT.getParamList()) {
    ParamTypes.push_back(P.getValType());
  }
  std::vector<ComponentValType> ResultTypes;
  ResultTypes.reserve(CFT.getResultList().size());
  for (const auto &R : CFT.getResultList()) {
    ResultTypes.push_back(R.getValType());
  }

  // Save the flat arguments. The parameters lift lazily at task start, and
  // the out-pointer is consumed at resolve time.
  auto SavedArgs =
      std::make_shared<std::vector<ValVariant>>(Args.begin(), Args.end());
  const bool Memory64 =
      Memory != nullptr && Memory->getMemoryType().getLimit().is64();
  std::optional<uint64_t> OutPtr;
  if (HasOutPtr) {
    if (Args.size() < ParamSlotCount + 1) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error("    canon lower thunk: missing trailing out-ptr"sv);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    OutPtr = Memory64 ? Args[ParamSlotCount].get<uint64_t>()
                      : uint64_t(Args[ParamSlotCount].get<uint32_t>());
  }

  auto Sub = std::make_shared<AsyncComp::SubtaskObj>();
  AsyncComp::SubtaskObj *SubP = Sub.get();

  const uint32_t MaxFlatParams = AsyncLower ? CanonicalABI::MaxFlatAsyncParams
                                            : CanonicalABI::MaxFlatParams;
  const uint32_t MaxFlatResults = AsyncLower ? 0 : CanonicalABI::MaxFlatResults;

  // Caller-side parameter production.
  const uint32_t ParamSlots = ParamSlotCount;
  auto OnStart = [SavedArgs, Sub, ParamTypes, MaxFlatParams, ParamSlots,
                  MakeCallerCx]() -> Expect<std::vector<ComponentValVariant>> {
    auto Cx = MakeCallerCx();
    Cx.LiftedBorrows = &Sub->Lenders;
    CanonicalABI::FlatIter VI(
        Span<const ValVariant>(SavedArgs->data(), ParamSlots));
    EXPECTED_TRY(auto Params, CanonicalABI::liftFlatValues(Cx, VI, ParamTypes,
                                                           MaxFlatParams));
    Sub->State = AsyncComp::SubtaskState::Started;
    noteSubtaskProgress(Sub.get());
    return Params;
  };

  // Caller-side result consumption. The lowered flat results of the sync
  // direct case are stashed for the return.
  auto SyncFlat = std::make_shared<std::vector<ValVariant>>();
  auto OnResolve =
      [Sub, ResultTypes, MaxFlatResults, OutPtr, SyncFlat,
       MakeCallerCx](std::optional<std::vector<ComponentValVariant>> Results)
      -> Expect<void> {
    if (!Results.has_value()) {
      Sub->State = Sub->State == AsyncComp::SubtaskState::Starting
                       ? AsyncComp::SubtaskState::CancelledBeforeStarted
                       : AsyncComp::SubtaskState::CancelledBeforeReturned;
      noteSubtaskProgress(Sub.get());
      return {};
    }
    auto Cx = MakeCallerCx();
    EXPECTED_TRY(auto Flat,
                 CanonicalABI::lowerFlatValues(Cx, *Results, ResultTypes,
                                               MaxFlatResults, OutPtr));
    *SyncFlat = std::move(Flat);
    Sub->State = AsyncComp::SubtaskState::Returned;
    noteSubtaskProgress(Sub.get());
    return {};
  };

  // Run the callee.
  if (!GuestCallee) {
    // Host component functions resolve immediately on component values.
    EXPECTED_TRY(auto Params, OnStart());
    EXPECTED_TRY(auto Out, Callee->getHostFunc()(Params));
    std::vector<ComponentValVariant> Values;
    Values.reserve(Out.size());
    for (auto &P : Out) {
      Values.push_back(std::move(P.first));
    }
    EXPECTED_TRY(OnResolve(std::move(Values)));
  } else {
    auto TaskOrErr = Exec->getAsyncRuntime().liftCall(Callee, OnStart,
                                                      OnResolve, CallerTask);
    if (!TaskOrErr) {
      return Unexpect(TaskOrErr.error());
    }
    SubP->Callee = *TaskOrErr;
    SubP->OnCancel = [Ex = this->Exec, T = *TaskOrErr]() {
      Ex->getAsyncRuntime().requestCancellation(*T);
    };
  }

  if (!AsyncLower) {
    if (!SubP->resolved()) {
      // Wait for the resolution. The eager may-block check above already
      // rejected a caller that cannot block.
      EXPECTED_TRY(auto Sig,
                   Exec->getAsyncRuntime().taskWait(
                       *CallerTask, [SubP]() { return SubP->resolved(); },
                       /*Cancellable=*/false));
      if (Sig == ResumeSignal::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
    }
    deliverSubtaskResolve(*SubP);
    if (SyncFlat->size() != Rets.size()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error(
          "    canon lower thunk: flat result arity mismatch (got {}, "
          "expected {})"sv,
          SyncFlat->size(), Rets.size());
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    for (size_t I = 0; I < SyncFlat->size(); ++I) {
      Rets[I] = (*SyncFlat)[I];
    }
    return {};
  }

  // Async lower: resolved during the eager call → plain state, no handle.
  if (SubP->resolved()) {
    deliverSubtaskResolve(*SubP);
    Rets[0] = static_cast<uint32_t>(SubP->State);
    return {};
  }
  const uint32_t Idx = CompInst->handles().waitableAdd(Sub);
  SubP->TableIdx = Idx;
  Rets[0] = static_cast<uint32_t>(SubP->State) | (Idx << 4);
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
