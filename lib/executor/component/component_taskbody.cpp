// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_taskbody.cpp - canon lift task bodies -------------------===//
//
// The guest-driving entries of the async task runtime: the four canon-lift
// task bodies, the canon lift entry that starts them, and the implicit
// destructor task. The scheduler below them is Runtime::Component::TaskManager.
//
//===----------------------------------------------------------------------===//

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errcode.h"
#include "common/spdlog.h"

#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace RtComp = Runtime::Component;

namespace {

Expect<void> trapAborted() noexcept {
  return Unexpect(ErrCode::Value::ComponentAsyncAborted);
}

std::vector<ComponentValType>
paramTypesOf(const AST::Component::FuncType &FT) noexcept {
  std::vector<ComponentValType> Types;
  Types.reserve(FT.getParamList().size());
  for (const auto &P : FT.getParamList()) {
    Types.push_back(P.getValType());
  }
  return Types;
}

std::vector<ComponentValType>
resultTypesOf(const AST::Component::FuncType &FT) noexcept {
  std::vector<ComponentValType> Types;
  Types.reserve(FT.getResultList().size());
  for (const auto &R : FT.getResultList()) {
    Types.push_back(R.getValType());
  }
  return Types;
}

} // namespace

Expect<void> ComponentExecutor::runTaskBody(RtComp::Task &T) noexcept {
  auto &Conc = T.Opts.Inst->concurrency();

  // Backpressure gating on entering the implicit thread.
  if (T.FTAsync) {
    auto HasBackpressure = [&Conc, &T]() {
      return Conc.getBackpressure() > 0 ||
             (T.needsExclusive() && Conc.getExclusiveTask() != nullptr);
    };
    if (HasBackpressure() || Conc.getNumWaitingToEnter() > 0) {
      Conc.incWaitingToEnter();
      auto ReasonOrErr = TaskMgr.taskWait(
          T, [HasBackpressure]() { return !HasBackpressure(); },
          /*Cancellable=*/true);
      // A teardown wake-up must not touch possibly-gone instance state.
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == RtComp::ResumeReason::Abort) {
        return trapAborted();
      }
      Conc.decWaitingToEnter();
      if (Reason == RtComp::ResumeReason::Cancelled) {
        return TaskMgr.taskCancel(T);
      }
    }
    if (T.needsExclusive()) {
      Conc.setExclusiveTask(&T);
    }
  }
  T.Implicit.Thread = T.Thread;
  T.Implicit.Index = T.Opts.Inst->concurrency().threadAdd(&T.Implicit);
  T.Implicit.Registered = true;

  // Produce and lower the arguments.
  T.St = RtComp::Task::State::Started;
  EXPECTED_TRY(auto Args, T.OnStart());

  const auto ParamTypes = paramTypesOf(*T.FT);
  Component::CanonicalABI::Context Cx{T.Opts, this};
  Cx.BorrowTask = &T;
  Conc.setMayLeave(false);
  auto FlatArgsOr = Component::CanonicalABI::lowerFlatValues(
      Cx, Args, ParamTypes, Component::CanonicalABI::MaxFlatParams);
  Conc.setMayLeave(true);
  EXPECTED_TRY(auto FlatArgs, std::move(FlatArgsOr));

  if (T.Core == nullptr) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  const auto &CoreParams = T.Core->getFuncType().getParamTypes();

  if (!T.Opts.Async) {
    // Sync-lifted; the function type itself may still be async.
    EXPECTED_TRY(auto CoreRets, core().invoke(T.Core, FlatArgs, CoreParams));
    if (T.St != RtComp::Task::State::Resolved) {
      const auto ResultTypes = resultTypesOf(*T.FT);
      Component::CanonicalABI::FlatIter VI(CoreRets);
      EXPECTED_TRY(auto Results, Component::CanonicalABI::liftFlatValues(
                                     Cx, VI, ResultTypes,
                                     Component::CanonicalABI::MaxFlatResults));
      EXPECTED_TRY(TaskMgr.taskReturn(T, std::move(Results)));
    }
    if (T.Opts.PostReturn != nullptr) {
      std::vector<ValVariant> PRArgs;
      PRArgs.reserve(CoreRets.size());
      for (const auto &P : CoreRets) {
        PRArgs.push_back(P.first);
      }
      Conc.setMayLeave(false);
      auto PRRes =
          core().invoke(T.Opts.PostReturn, PRArgs,
                        T.Opts.PostReturn->getFuncType().getParamTypes());
      Conc.setMayLeave(true);
      if (!PRRes) {
        return Unexpect(PRRes.error());
      }
    }
    return TaskMgr.taskExit(T);
  }

  if (T.Opts.Callback == nullptr) {
    // Async stackful: the core function blocks through built-ins.
    EXPECTED_TRY(core().invoke(T.Core, FlatArgs, CoreParams));
    return TaskMgr.taskExit(T);
  }

  // Async with callback: run the event loop.
  EXPECTED_TRY(auto First, core().invoke(T.Core, FlatArgs, CoreParams));
  if (First.empty()) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  uint32_t Packed = First[0].first.get<uint32_t>();
  while (true) {
    const uint32_t Code = Packed & 0xFU;
    const uint32_t SetIdx = Packed >> 4;
    if (Code > static_cast<uint32_t>(
                   Runtime::Instance::Component::AsyncCallbackCode::Max)) {
      spdlog::error(ErrCode::Value::ComponentBadCallbackCode);
      spdlog::error("    unsupported callback code"sv);
      return Unexpect(ErrCode::Value::ComponentBadCallbackCode);
    }
    if (Code == static_cast<uint32_t>(
                    Runtime::Instance::Component::AsyncCallbackCode::Exit)) {
      break;
    }
    // taskWait releases the exclusive slot, so a peer can enter.
    Runtime::Instance::Component::AsyncEvent Ev;
    if (Code == static_cast<uint32_t>(
                    Runtime::Instance::Component::AsyncCallbackCode::Yield)) {
      auto ReasonOrErr = TaskMgr.taskWait(
          T, []() { return true; }, /*Cancellable=*/true,
          /*AlwaysReleaseExcl=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == RtComp::ResumeReason::Abort) {
        return trapAborted();
      }
      if (Reason == RtComp::ResumeReason::Cancelled) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else {
        Ev = {Runtime::Instance::Component::AsyncEventCode::None, 0, 0};
      }
    } else {
      auto *WSet = T.Opts.Inst->handles().waitableSetGet(SetIdx);
      if (WSet == nullptr) {
        spdlog::error(ErrCode::Value::ComponentHandleUnknown);
        spdlog::error("    unknown handle index {}"sv, SetIdx);
        return Unexpect(ErrCode::Value::ComponentHandleUnknown);
      }
      WSet->NumWaiting += 1;
      auto ReasonOrErr = TaskMgr.taskWait(
          T, [WSet]() { return WSet->hasPendingEvent(); },
          /*Cancellable=*/true, /*AlwaysReleaseExcl=*/true,
          /*FastPath=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == RtComp::ResumeReason::Abort) {
        return trapAborted();
      }
      WSet->NumWaiting -= 1;
      if (Reason == RtComp::ResumeReason::Cancelled) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    std::array<ValVariant, 3> CbArgs{ValVariant(static_cast<uint32_t>(Ev.Code)),
                                     ValVariant(Ev.P1), ValVariant(Ev.P2)};
    EXPECTED_TRY(auto CbRets,
                 core().invoke(T.Opts.Callback, CbArgs,
                               T.Opts.Callback->getFuncType().getParamTypes()));
    if (CbRets.empty()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    Packed = CbRets[0].first.get<uint32_t>();
  }
  return TaskMgr.taskExit(T);
}

// ----------------------------------------------------------------------------
// canon lift entry: build the task, then run it nested or on its own thread.

Expect<RtComp::Task *> ComponentExecutor::liftCall(
    const Runtime::Instance::ComponentFunctionInstance *FuncInst,
    RtComp::Task::OnStartCallback OnStart,
    RtComp::Task::OnResolveCallback OnResolve,
    RtComp::Task *CallerTask) noexcept {
  const auto *Inst = FuncInst->getComponentInstance();

  RtComp::Task *T = TaskMgr.newTask();
  T->FT = &FuncInst->getFuncType();
  T->FTAsync = FuncInst->getFuncType().isAsync();
  T->Core = FuncInst->getLowerFunction();
  T->Opts = FuncInst->getCanonOptions();
  T->OnStart = std::move(OnStart);
  T->OnResolve = std::move(OnResolve);
  T->CallerTask = CallerTask;
  T->CallerInst = CallerTask != nullptr ? CallerTask->Opts.Inst : nullptr;

  if (!T->FTAsync) {
    // Nested synchronous execution on the current task thread.
    if (Inst != nullptr && Inst->concurrency().entered()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      spdlog::error("    cannot enter component instance"sv);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
    std::optional<
        Runtime::Instance::Component::ConcurrencyManager::EnteredGuard>
        Guard;
    if (Inst != nullptr) {
      Guard.emplace(Inst->concurrency(), true);
    }
    TaskMgr.pushNestedTask(T);
    auto Res = runTaskBody(*T);
    TaskMgr.popNestedTask();
    if (!Res) {
      T->Failed = Res.error();
      TaskMgr.noteTrap(Res.error(), Inst);
      return Unexpect(Res.error());
    }
    return T;
  }

  // Async function type: dedicated task thread, run eagerly until first block.
  TaskMgr.newThread(T, [this, T](RtComp::ResumeReason Reason) {
    if (Reason == RtComp::ResumeReason::Abort) {
      return;
    }
    auto Res = runTaskBody(*T);
    if (!Res) {
      T->Failed = Res.error();
      TaskMgr.noteTrap(Res.error(), T->Opts.Inst);
    }
  });
  TaskMgr.resumeThread(T->Thread, RtComp::ResumeReason::Normal);
  if (TaskMgr.trapLatch().has_value()) {
    return Unexpect(*TaskMgr.trapLatch());
  }
  return T;
}

Expect<void> ComponentExecutor::resourceDtorCall(
    const Runtime::Instance::ComponentInstance *Impl,
    Runtime::Instance::FunctionInstance *Dtor, uint64_t Rep) noexcept {
  // Implicit sync destructor task on the implementing instance.
  RtComp::Task *T = TaskMgr.newTask();
  T->Opts.Inst = Impl;
  T->CallerTask = TaskMgr.currentTask();
  T->CallerInst = T->CallerTask != nullptr ? T->CallerTask->Opts.Inst : nullptr;
  T->St = RtComp::Task::State::Started;
  if (Impl != nullptr) {
    T->Implicit.Index = Impl->concurrency().threadAdd(&T->Implicit);
    T->Implicit.Registered = true;
  }
  TaskMgr.pushNestedTask(T);
  std::array<ValVariant, 1> DtorArgs{ValVariant(Rep)};
  std::array<ValType, 1> DtorTypes{ValType(TypeCode::I32)};
  auto Res = core().invoke(Dtor, DtorArgs, DtorTypes);
  TaskMgr.popNestedTask();
  if (T->Implicit.Registered && Impl != nullptr) {
    Impl->concurrency().threadRemove(T->Implicit.Index);
    T->Implicit.Registered = false;
  }
  T->St = RtComp::Task::State::Resolved;
  if (!Res) {
    return Unexpect(Res.error());
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
