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
#include "runtime/component/callingframe.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/spdlog.h"

#include <array>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {

Expect<void> trapAborted() noexcept {
  return Unexpect(ErrCode::Value::ComponentAsyncAborted);
}

} // namespace

Expect<void>
ComponentExecutor::runHostTaskBody(Runtime::Component::Task &T) noexcept {
  // The host is an async-stackful lift without backpressure, exclusive
  // thread, or core function: it runs on component-level values and resolves
  // the task itself.
  T.Implicit.Thread = TaskMgr.getCurrentThread();
  T.Status = Runtime::Component::Task::State::Started;
  EXPECTED_TRY(auto Args, T.OnStart());
  std::vector<ComponentValVariant> Rets(T.FuncType->getResultList().size());
  Runtime::Component::CallingFrame Frame(TaskMgr, T);
  EXPECTED_TRY(T.Host->run(Frame, Args, Rets));
  if (T.Status != Runtime::Component::Task::State::Resolved) {
    EXPECTED_TRY(TaskMgr.taskReturn(T, std::move(Rets)));
  }
  return TaskMgr.taskExit(T);
}

Expect<void>
ComponentExecutor::runTaskBody(Runtime::Component::Task &T) noexcept {
  if (T.Host != nullptr) {
    return runHostTaskBody(T);
  }
  const auto *Inst = T.Opts.Inst;

  // Backpressure gating on entering the implicit thread.
  if (T.IsFuncTypeAsync) {
    auto HasBackpressure = [Inst, &T]() {
      return Inst->getBackpressure() > 0 ||
             (T.needsExclusive() && Inst->getExclusiveTask() != nullptr);
    };
    if (HasBackpressure() || Inst->getNumWaitingToEnter() > 0) {
      Inst->incWaitingToEnter();
      auto ReasonOrErr = TaskMgr.taskWait(
          T, [HasBackpressure]() { return !HasBackpressure(); },
          /*Cancellable=*/true);
      // A teardown wake-up must not touch possibly-gone instance state.
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == Runtime::Component::ResumeReason::Abort) {
        return trapAborted();
      }
      Inst->decWaitingToEnter();
      if (Reason == Runtime::Component::ResumeReason::Cancelled) {
        return TaskMgr.taskCancel(T);
      }
    }
    if (T.needsExclusive()) {
      Inst->setExclusiveTask(&T);
    }
  }
  T.Implicit.Thread = TaskMgr.getCurrentThread();
  T.Implicit.Index = Inst->addThread(&T.Implicit);

  // Produce and lower the arguments.
  T.Status = Runtime::Component::Task::State::Started;
  EXPECTED_TRY(auto Args, T.OnStart());

  const auto ParamTypes = T.FuncType->getParamValTypes();
  Component::LiftLowerContext Ctx{T.Opts, this};
  Ctx.setBorrowTask(&T);
  Inst->setMayLeave(false);
  auto FlatArgsOr = Ctx.lowerValues(Args, ParamTypes,
                                    Component::LiftLowerContext::MaxFlatParams);
  Inst->setMayLeave(true);
  EXPECTED_TRY(auto FlatArgs, std::move(FlatArgsOr));

  if (T.Core == nullptr) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  const auto &CoreParams = T.Core->getFuncType().getParamTypes();

  if (!T.Opts.Async) {
    // Sync-lifted; the function type itself may still be async.
    EXPECTED_TRY(auto CoreRets,
                 getCoreExecutor().invoke(T.Core, FlatArgs, CoreParams));
    std::vector<ValVariant> FlatRets;
    FlatRets.reserve(CoreRets.size());
    for (const auto &R : CoreRets) {
      FlatRets.push_back(R.first);
    }
    if (T.Status != Runtime::Component::Task::State::Resolved) {
      const auto ResultTypes = T.FuncType->getResultValTypes();
      EXPECTED_TRY(auto Results,
                   Ctx.liftValues(FlatRets, ResultTypes,
                                  Component::LiftLowerContext::MaxFlatResults));
      EXPECTED_TRY(TaskMgr.taskReturn(T, std::move(Results)));
    }
    if (T.Opts.PostReturn != nullptr) {
      Inst->setMayLeave(false);
      auto PRRes = getCoreExecutor().invoke(
          T.Opts.PostReturn, FlatRets,
          T.Opts.PostReturn->getFuncType().getParamTypes());
      Inst->setMayLeave(true);
      if (!PRRes) {
        return Unexpect(PRRes.error());
      }
    }
    return TaskMgr.taskExit(T);
  }

  if (T.Opts.Callback == nullptr) {
    // Async stackful: the core function blocks through built-ins.
    EXPECTED_TRY(getCoreExecutor().invoke(T.Core, FlatArgs, CoreParams));
    return TaskMgr.taskExit(T);
  }

  // Async with callback: run the event loop.
  EXPECTED_TRY(auto First,
               getCoreExecutor().invoke(T.Core, FlatArgs, CoreParams));
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
      if (Reason == Runtime::Component::ResumeReason::Abort) {
        return trapAborted();
      }
      if (Reason == Runtime::Component::ResumeReason::Cancelled) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else {
        Ev = {Runtime::Instance::Component::AsyncEventCode::None, 0, 0};
      }
    } else {
      auto *WSet = T.Opts.Inst->getWaitableSet(SetIdx);
      if (WSet == nullptr) {
        spdlog::error(ErrCode::Value::ComponentHandleUnknown);
        spdlog::error("    handle index {}"sv, SetIdx);
        return Unexpect(ErrCode::Value::ComponentHandleUnknown);
      }
      EXPECTED_TRY(drainPostedTransmits());
      WSet->NumWaiting += 1;
      auto ReasonOrErr = TaskMgr.taskWait(
          T, [WSet]() { return WSet->hasPendingEvent(); },
          /*Cancellable=*/true, /*AlwaysReleaseExcl=*/true,
          /*FastPath=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == Runtime::Component::ResumeReason::Abort) {
        return trapAborted();
      }
      WSet->NumWaiting -= 1;
      if (Reason == Runtime::Component::ResumeReason::Cancelled) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    std::array<ValVariant, 3> CbArgs{ValVariant(static_cast<uint32_t>(Ev.Code)),
                                     ValVariant(Ev.P1), ValVariant(Ev.P2)};
    EXPECTED_TRY(auto CbRets,
                 getCoreExecutor().invoke(
                     T.Opts.Callback, CbArgs,
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

Expect<Runtime::Component::Task *> ComponentExecutor::liftCall(
    const Runtime::Instance::ComponentFunctionInstance *FuncInst,
    Runtime::Component::Task::OnStartCallback OnStart,
    Runtime::Component::Task::OnResolveCallback OnResolve,
    Runtime::Component::Task *CallerTask) noexcept {
  const auto *Inst = FuncInst->getComponentInstance();

  Runtime::Component::Task *T = TaskMgr.newTask();
  T->FuncType = &FuncInst->getFuncType();
  T->IsFuncTypeAsync = FuncInst->getFuncType().isAsync();
  T->Core = FuncInst->getLowerFunction();
  T->Host = FuncInst->isHostFunction() ? &FuncInst->getHostFunc() : nullptr;
  T->Opts = FuncInst->getCanonOptions();
  if (T->Host != nullptr) {
    // A host task is the async-stackful lift of the host.
    T->Opts.Async = true;
  }
  T->OnStart = std::move(OnStart);
  T->OnResolve = std::move(OnResolve);
  T->CallerTask = CallerTask;

  // A synchronous call enters a guest instance; the host is reentrant.
  const bool GuardInst =
      !T->IsFuncTypeAsync && T->Host == nullptr && Inst != nullptr;
  if (GuardInst && Inst->isEntered()) {
    spdlog::error(ErrCode::Value::ComponentCannotEnter);
    return Unexpect(ErrCode::Value::ComponentCannotEnter);
  }
  if (!T->IsFuncTypeAsync && TaskMgr.getCurrentThread() != nullptr) {
    // Nested synchronous execution on the current task thread.
    std::optional<Runtime::Instance::ComponentInstance::EnteredGuard> Guard;
    if (GuardInst) {
      Guard.emplace(*Inst, true);
    }
    Runtime::Component::TaskManager::NestedTaskGuard TaskGuard{TaskMgr, *T};
    auto Res = runTaskBody(*T);
    if (!Res) {
      TaskMgr.noteTrap(Res.error(), Inst);
      return Unexpect(Res.error());
    }
    return T;
  }

  // Own task thread, run eagerly until the first block: an async function
  // type, or a root activation entered from the scheduler thread.
  TaskMgr.newThread(
      T, [this, T, Inst, GuardInst](Runtime::Component::ResumeReason Reason) {
        if (Reason == Runtime::Component::ResumeReason::Abort) {
          return;
        }
        std::optional<Runtime::Instance::ComponentInstance::EnteredGuard> Guard;
        if (GuardInst) {
          Guard.emplace(*Inst, true);
        }
        auto Res = runTaskBody(*T);
        if (!Res) {
          TaskMgr.noteTrap(Res.error(), T->Opts.Inst);
        }
      });
  TaskMgr.resumeThread(T->Thread, Runtime::Component::ResumeReason::Normal);
  if (TaskMgr.getTrapLatch().has_value()) {
    return Unexpect(*TaskMgr.getTrapLatch());
  }
  return T;
}

Expect<void> ComponentExecutor::resourceDtorCall(
    const Runtime::Instance::ComponentInstance *Impl,
    Runtime::Instance::FunctionInstance *Dtor, uint64_t Rep) noexcept {
  // Implicit sync destructor task on the implementing instance.
  Runtime::Component::Task *T = TaskMgr.newTask();
  T->Opts.Inst = Impl;
  T->CallerTask = TaskMgr.getCurrentTask();
  T->Status = Runtime::Component::Task::State::Started;
  T->Implicit.Thread = TaskMgr.getCurrentThread();
  if (Impl != nullptr) {
    T->Implicit.Index = Impl->addThread(&T->Implicit);
  }
  Runtime::Component::TaskManager::NestedTaskGuard TaskGuard{TaskMgr, *T};
  std::array<ValVariant, 1> DtorArgs{ValVariant(Rep)};
  std::array<ValType, 1> DtorTypes{ValType(TypeCode::I32)};
  auto Res = getCoreExecutor().invoke(Dtor, DtorArgs, DtorTypes);
  if (T->Implicit.Index != 0) {
    Impl->removeThread(T->Implicit.Index);
    T->Implicit.Index = 0;
  }
  T->Status = Runtime::Component::Task::State::Resolved;
  if (!Res) {
    return Unexpect(Res.error());
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
