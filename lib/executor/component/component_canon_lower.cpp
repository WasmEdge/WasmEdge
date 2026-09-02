// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "runtime/component/taskmgr.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/spdlog.h"

#include <cstdint>
#include <memory>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

CanonLowerHostFunc::CanonLowerHostFunc(
    ComponentExecutor *CompExec, AST::FunctionType FlatSig,
    Runtime::Instance::ComponentFunctionInstance *CalleeFunc,
    const Runtime::Component::CanonOptions &Options) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(CompExec), Callee(CalleeFunc),
      Opts(Options), ParamTypes(CalleeFunc->getFuncType().getParamValTypes()),
      ResultTypes(CalleeFunc->getFuncType().getResultValTypes()),
      MaxFlatParams(Options.Async ? LiftLowerContext::MaxFlatAsyncParams
                                  : LiftLowerContext::MaxFlatParams),
      MaxFlatResults(Options.Async ? 0 : LiftLowerContext::MaxFlatResults),
      HasOutPtr(false), ParamSlotCount(0) {
  // A trailing out-pointer exists when the results spill or lower is async.
  const bool CalleeHasResults = !ResultTypes.empty();
  if (Opts.Async) {
    HasOutPtr = CalleeHasResults;
  } else {
    HasOutPtr = FlatSig.getReturnTypes().empty() && CalleeHasResults;
  }
  ParamSlotCount = static_cast<uint32_t>(FlatSig.getParamTypes().size()) -
                   (HasOutPtr ? 1U : 0U);
  DefType.getCompositeType().getFuncType() = std::move(FlatSig);
}

// Callee type indices are its own; the handle tables stay with the caller.
LiftLowerContext CanonLowerHostFunc::makeCallerContext() const noexcept {
  const auto *CalleeComp = Callee->getComponentInstance();
  LiftLowerContext Ctx(
      Opts, Exec,
      CalleeComp != nullptr && CalleeComp != Opts.Inst ? CalleeComp : nullptr);
  Ctx.setCrossComponent(true);
  return Ctx;
}

Expect<void> CanonLowerHostFunc::run(const Runtime::CallingFrame &,
                                     Span<const ValVariant> Args,
                                     Span<ValVariant> Rets) {
  auto &TaskMgr = Exec->getTaskManager();
  // Lowered imports are unreachable from argument lowering and post-return.
  if (Opts.Inst != nullptr && !Opts.Inst->isLeaveAllowed()) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }

  const auto *CalleeComp = Callee->getComponentInstance();
  const bool GuestCallee = !Callee->isHostFunction();
  // Adapter reentrance: a call to itself or a lexical relative traps.
  if (GuestCallee && Opts.Inst != nullptr && CalleeComp != nullptr &&
      CalleeComp->isLinealRelativeOf(Opts.Inst)) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }

  const bool CalleeAsyncType = Callee->getFuncType().isAsync();
  Runtime::Component::Task *CallerTask = TaskMgr.getCurrentTask();
  if (!Opts.Async && CalleeAsyncType && GuestCallee &&
      (CallerTask == nullptr || !Exec->mayBlock(*CallerTask))) {
    // A sync lower of an async-typed function needs a task that can block.
    spdlog::error(ErrCode::Value::ComponentCannotBlockSync);
    return Unexpect(ErrCode::Value::ComponentCannotBlockSync);
  }

  // Params lift lazily at task start; the out-pointer is used at resolve.
  auto SavedArgs =
      std::make_shared<std::vector<ValVariant>>(Args.begin(), Args.end());
  std::optional<uint64_t> OutPtr;
  if (HasOutPtr) {
    if (Args.size() < ParamSlotCount + 1) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error("    canon lower: missing trailing out-ptr"sv);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    OutPtr = makeCallerContext().liftPtr(Args[ParamSlotCount]);
  }

  auto Sub = std::make_shared<Runtime::Instance::Component::Subtask>();
  Runtime::Instance::Component::Subtask *SubPtr = Sub.get();

  // Caller-side parameter production.
  auto OnStart = [this, SavedArgs,
                  Sub]() -> Expect<std::vector<ComponentValVariant>> {
    auto Ctx = makeCallerContext();
    Ctx.setLiftedBorrows(&Sub->Lenders);
    EXPECTED_TRY(auto Params,
                 Ctx.liftValues(
                     Span<const ValVariant>(SavedArgs->data(), ParamSlotCount),
                     ParamTypes, MaxFlatParams));
    Sub->Status = Runtime::Instance::Component::Subtask::State::Started;
    Sub->noteProgress();
    return Params;
  };

  // The sync direct case stashes its lowered flat results for the return.
  auto SyncFlat = std::make_shared<std::vector<ValVariant>>();
  auto OnResolve = [this, Sub, OutPtr, SyncFlat](
                       std::optional<std::vector<ComponentValVariant>> Results)
      -> Expect<void> {
    if (!Results.has_value()) {
      Sub->Status =
          Sub->Status == Runtime::Instance::Component::Subtask::State::Starting
              ? Runtime::Instance::Component::Subtask::State::
                    CancelledBeforeStarted
              : Runtime::Instance::Component::Subtask::State::
                    CancelledBeforeReturned;
      Sub->noteProgress();
      return {};
    }
    auto Ctx = makeCallerContext();
    EXPECTED_TRY(auto Flat, Ctx.lowerValues(*Results, ResultTypes,
                                            MaxFlatResults, OutPtr));
    *SyncFlat = std::move(Flat);
    Sub->Status = Runtime::Instance::Component::Subtask::State::Returned;
    Sub->noteProgress();
    return {};
  };

  // Run the callee as a task, guest and host alike.
  EXPECTED_TRY(auto *CalleeTask,
               Exec->liftCall(Callee, OnStart, OnResolve, CallerTask));
  SubPtr->Callee = CalleeTask;
  SubPtr->OnCancel = [CompExec = Exec, CalleeTask]() {
    CompExec->getTaskManager().requestCancellation(*CalleeTask);
  };

  if (!Opts.Async) {
    if (!SubPtr->isResolved()) {
      // Wait for the resolution; the eager may-block check already ran. A
      // host callee is exempt from it, but no task means no thread to park.
      if (CallerTask == nullptr) {
        spdlog::error(ErrCode::Value::ComponentCannotBlockSync);
        return Unexpect(ErrCode::Value::ComponentCannotBlockSync);
      }
      EXPECTED_TRY(auto Reason,
                   TaskMgr.taskWait(
                       *CallerTask, [SubPtr]() { return SubPtr->isResolved(); },
                       /*Cancellable=*/false));
      if (Reason == Runtime::Component::ResumeReason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
    }
    SubPtr->deliverResolve();
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

  // Async lower: resolved during the eager call means plain state, no handle.
  if (SubPtr->isResolved()) {
    SubPtr->deliverResolve();
    Rets[0] = static_cast<uint32_t>(SubPtr->Status);
    return {};
  }
  const uint32_t Idx = Opts.Inst->addWaitable(Sub);
  SubPtr->TableIdx = Idx;
  Rets[0] = static_cast<uint32_t>(SubPtr->Status) | (Idx << 4);
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
