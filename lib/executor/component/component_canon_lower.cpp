// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- component_canon_lower.cpp - canon lower core function -------------===//
//
// Runtime behavior of `canon lower` for the four {sync,async} combinations.
// The callee runs as a task.
//
//===----------------------------------------------------------------------===//

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "runtime/component/taskmgr.h"

#include "common/spdlog.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Executor {
namespace Component {

using namespace std::literals;
namespace InstComp = Runtime::Instance::Component;
namespace RtComp = Runtime::Component;

namespace {

// Release the argument lends taken by the borrows lifted for this call.
void deliverSubtaskResolve(InstComp::Subtask &Sub) noexcept {
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

// Queue the SUBTASK progress event once the subtask has a handle.
void noteSubtaskProgress(InstComp::Subtask *Sub) noexcept {
  if (!Sub->TableIdx.has_value()) {
    return;
  }
  const uint32_t Idx = *Sub->TableIdx;
  Sub->setPendingEvent([Sub, Idx]() -> InstComp::AsyncEvent {
    if (Sub->resolved()) {
      deliverSubtaskResolve(*Sub);
    }
    return {InstComp::AsyncEventCode::Subtask, Idx,
            static_cast<uint32_t>(Sub->St)};
  });
}

} // namespace

CanonLowerHostFunc::CanonLowerHostFunc(
    ComponentExecutor *ExecIn, AST::FunctionType FlatSig,
    Runtime::Instance::ComponentFunctionInstance *CalleeIn,
    const Runtime::Component::CanonOptions &OptsIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Callee(CalleeIn),
      Opts(OptsIn), HasOutPtr(false), ParamSlotCount(0) {
  // A trailing out-pointer exists when the results spill or lower is async.
  const bool CalleeHasResults =
      !CalleeIn->getFuncType().getResultList().empty();
  if (Opts.Async) {
    HasOutPtr = CalleeHasResults;
  } else {
    HasOutPtr = FlatSig.getReturnTypes().empty() && CalleeHasResults;
  }
  ParamSlotCount = static_cast<uint32_t>(FlatSig.getParamTypes().size()) -
                   (HasOutPtr ? 1U : 0U);
  DefType.getCompositeType().getFuncType() = std::move(FlatSig);
}

Expect<void> CanonLowerHostFunc::run(const Runtime::CallingFrame &,
                                     Span<const ValVariant> Args,
                                     Span<ValVariant> Rets) {
  auto &Rt = Exec->taskManager();
  // Lowered imports are unreachable from argument lowering and post-return.
  if (Opts.Inst != nullptr && !Opts.Inst->concurrency().mayLeave()) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    spdlog::error("    cannot leave component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }

  const auto *CalleeComp = Callee->getComponentInstance();
  const bool GuestCallee = !Callee->isHostFunction();
  // Adapter reentrance: a call to itself or a lexical relative traps.
  if (GuestCallee && Opts.Inst != nullptr && CalleeComp != nullptr &&
      CalleeComp->isLinealRelativeOf(Opts.Inst)) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    spdlog::error("    cannot enter component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }

  const bool CalleeAsyncType = Callee->getFuncType().isAsync();
  RtComp::Task *CallerTask = Rt.currentTask();
  if (!Opts.Async && CalleeAsyncType && GuestCallee &&
      (CallerTask == nullptr || !Rt.mayBlock(*CallerTask))) {
    // A sync lower of an async-typed function needs a task that can block.
    spdlog::error(ErrCode::Value::ComponentCannotBlockSync);
    spdlog::error("    cannot block a synchronous task before returning"sv);
    return Unexpect(ErrCode::Value::ComponentCannotBlockSync);
  }

  // Callee type indices are its own; the handle tables stay with the caller.
  auto MakeCallerCx = [this]() {
    CanonicalABI::Context Cx{Opts, Exec};
    Cx.CrossComponent = true;
    if (const auto *CC = Callee->getComponentInstance();
        CC != nullptr && CC != Opts.Inst) {
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

  // Params lift lazily at task start; the out-pointer is used at resolve.
  auto SavedArgs =
      std::make_shared<std::vector<ValVariant>>(Args.begin(), Args.end());
  const bool Memory64 =
      Opts.Mem != nullptr && Opts.Mem->getMemoryType().getLimit().is64();
  std::optional<uint64_t> OutPtr;
  if (HasOutPtr) {
    if (Args.size() < ParamSlotCount + 1) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error("    canon lower: missing trailing out-ptr"sv);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    OutPtr = Memory64 ? Args[ParamSlotCount].get<uint64_t>()
                      : uint64_t(Args[ParamSlotCount].get<uint32_t>());
  }

  auto Sub = std::make_shared<InstComp::Subtask>();
  InstComp::Subtask *SubP = Sub.get();

  const uint32_t MaxFlatParams = Opts.Async ? CanonicalABI::MaxFlatAsyncParams
                                            : CanonicalABI::MaxFlatParams;
  const uint32_t MaxFlatResults = Opts.Async ? 0 : CanonicalABI::MaxFlatResults;

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
    Sub->St = InstComp::Subtask::State::Started;
    noteSubtaskProgress(Sub.get());
    return Params;
  };

  // The sync direct case stashes its lowered flat results for the return.
  auto SyncFlat = std::make_shared<std::vector<ValVariant>>();
  auto OnResolve =
      [Sub, ResultTypes, MaxFlatResults, OutPtr, SyncFlat,
       MakeCallerCx](std::optional<std::vector<ComponentValVariant>> Results)
      -> Expect<void> {
    if (!Results.has_value()) {
      Sub->St = Sub->St == InstComp::Subtask::State::Starting
                    ? InstComp::Subtask::State::CancelledBeforeStarted
                    : InstComp::Subtask::State::CancelledBeforeReturned;
      noteSubtaskProgress(Sub.get());
      return {};
    }
    auto Cx = MakeCallerCx();
    EXPECTED_TRY(auto Flat,
                 CanonicalABI::lowerFlatValues(Cx, *Results, ResultTypes,
                                               MaxFlatResults, OutPtr));
    *SyncFlat = std::move(Flat);
    Sub->St = InstComp::Subtask::State::Returned;
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
    auto TaskOrErr = Exec->liftCall(Callee, OnStart, OnResolve, CallerTask);
    if (!TaskOrErr) {
      return Unexpect(TaskOrErr.error());
    }
    SubP->Callee = *TaskOrErr;
    SubP->OnCancel = [Ex = this->Exec, T = *TaskOrErr]() {
      Ex->taskManager().requestCancellation(*T);
    };
  }

  if (!Opts.Async) {
    if (!SubP->resolved()) {
      // Wait for the resolution; the eager may-block check already ran.
      EXPECTED_TRY(auto Reason,
                   Exec->taskManager().taskWait(
                       *CallerTask, [SubP]() { return SubP->resolved(); },
                       /*Cancellable=*/false));
      if (Reason == RtComp::ResumeReason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
    }
    deliverSubtaskResolve(*SubP);
    if (SyncFlat->size() != Rets.size()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error("    canon lower: flat result arity mismatch (got {}, "
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
    Rets[0] = static_cast<uint32_t>(SubP->St);
    return {};
  }
  const uint32_t Idx = Opts.Inst->handles().waitableAdd(Sub);
  SubP->TableIdx = Idx;
  Rets[0] = static_cast<uint32_t>(SubP->St) | (Idx << 4);
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
