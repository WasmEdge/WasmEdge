// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_async_runtime.cpp - async task runtime ------------------===//
//
// The task scheduler of the component-model async proposal: strict hand-off
// between OS-thread vehicles (one runnable at a time, mirroring the spec's
// definitional interpreter), the canon-lift task bodies for all four
// lift shapes (sync, sync-of-async-type, async stackful, async callback),
// and trap propagation with instance-tree poisoning.
//
//===----------------------------------------------------------------------===//

#include "executor/component/async_runtime.h"
#include "executor/component/canonical_abi.h"
#include "executor/executor.h"

#include "common/errcode.h"
#include "common/spdlog.h"

#include <algorithm>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using Runtime::Instance::ComponentInstance;
namespace AsyncComp = Runtime::Instance::Component;

TaskVehicle::~TaskVehicle() noexcept {
  if (Thread.joinable()) {
    if (!Finished) {
      // Never-resumed vehicle: release the parked thread so its body can
      // observe the abort and finish.
      Arg = ResumeSignal::Abort;
      Run.release();
    }
    Thread.join();
  }
}

void TaskVehicle::launch(std::function<void(ResumeSignal)> Body) noexcept {
  Thread = std::thread([this, Body = std::move(Body)]() {
    Run.acquire();
    try {
      Body(Arg);
    } catch (...) {
      // A HandoffSem operation on an already-destroyed peer during teardown
      // can throw; the body has nothing left to do in that case.
    }
    Finished = true;
    if (Back != nullptr) {
      HandoffSem *B = Back;
      Back = nullptr;
      B->release();
    }
  });
}

TaskVehicle *
AsyncRuntime::newVehicle(ComponentTask *T,
                         std::function<void(ResumeSignal)> Body) noexcept {
  Vehicles.push_back(std::make_unique<TaskVehicle>());
  TaskVehicle *V = Vehicles.back().get();
  V->Owner = T;
  V->TaskStack.push_back(T);
  V->CtxStack.push_back(&T->Implicit);
  T->Vehicle = V;
  T->Implicit.Owner = T;
  T->Implicit.Vehicle = V;
  V->launch(std::move(Body));
  return V;
}

TaskVehicle *
AsyncRuntime::newSpawnVehicle(ComponentTask *T, ComponentThreadCtx *Ctx,
                              std::function<void(ResumeSignal)> Body) noexcept {
  Vehicles.push_back(std::make_unique<TaskVehicle>());
  TaskVehicle *V = Vehicles.back().get();
  V->Owner = T;
  V->TaskStack.push_back(T);
  V->CtxStack.push_back(Ctx);
  Ctx->Owner = T;
  Ctx->Vehicle = V;
  V->launch(std::move(Body));
  // The spawned thread starts suspended: parked with no ready predicate
  // until thread.resume-later and friends wake it.
  Waiting.push_back(V);
  return V;
}

void AsyncRuntime::resumeVehicle(TaskVehicle *V, ResumeSignal Sig) noexcept {
  HandoffSem Mine;
  V->Arg = Sig;
  V->Back = &Mine;
  TaskVehicle *Prev = Current;
  Current = V;
  V->Run.release();
  Mine.acquire();
  Current = Prev;
}

Expect<ResumeSignal> AsyncRuntime::parkCurrent(std::function<bool()> Ready,
                                               bool Cancellable) noexcept {
  // During teardown a resumed vehicle must unwind rather than re-park.
  if (Aborting) {
    return ResumeSignal::Abort;
  }
  TaskVehicle *Self = Current;
  if (Self == nullptr) {
    // Embedder thread: cannot park. Pump other vehicles until ready.
    EXPECTED_TRY(pumpUntil(Ready));
    return ResumeSignal::Normal;
  }
  Self->ReadyFn = std::move(Ready);
  Self->Cancellable = Cancellable;
  Waiting.push_back(Self);
  HandoffSem *B = Self->Back;
  Self->Back = nullptr;
  B->release();
  Self->Run.acquire();
  return Self->Arg;
}

Expect<void>
AsyncRuntime::pumpUntil(const std::function<bool()> &Done) noexcept {
  while (!Done()) {
    if (Trap.has_value()) {
      return Unexpect(*Trap);
    }
    TaskVehicle *Pick = nullptr;
    for (size_t I = 0; I < Waiting.size(); ++I) {
      TaskVehicle *V = Waiting[I];
      if (V->ReadyFn && V->ReadyFn()) {
        Pick = V;
        Waiting.erase(Waiting.begin() + static_cast<ptrdiff_t>(I));
        break;
      }
    }
    if (Pick == nullptr) {
      spdlog::error(ErrCode::Value::ComponentAsyncDeadlock);
      spdlog::error(
          "    deadlock detected: event loop cannot make further progress"sv);
      return Unexpect(ErrCode::Value::ComponentAsyncDeadlock);
    }
    Pick->ReadyFn = nullptr;
    resumeVehicle(Pick, ResumeSignal::Normal);
    if (Trap.has_value()) {
      return Unexpect(*Trap);
    }
  }
  return {};
}

bool AsyncRuntime::resumeParked(TaskVehicle *V, ResumeSignal Sig) noexcept {
  auto It = std::find(Waiting.begin(), Waiting.end(), V);
  if (It == Waiting.end()) {
    return false;
  }
  Waiting.erase(It);
  V->ReadyFn = nullptr;
  resumeVehicle(V, Sig);
  return true;
}

void AsyncRuntime::noteTrap(ErrCode Err,
                            const ComponentInstance *Inst) noexcept {
  if (Err.getEnum() == ErrCode::Value::ComponentAsyncAborted) {
    return;
  }
  if (!Trap.has_value()) {
    Trap = Err;
  }
  if (Inst != nullptr) {
    Inst->getRoot()->getConcurrency().Poisoned = true;
  }
}

void AsyncRuntime::teardown() noexcept {
  // Only ever entered with control on the embedder thread, when every live
  // vehicle is parked (Waiting) or finished. Resume the parked ones with
  // Abort until nothing is left, then the vehicle destructors join.
  if (!Trap.has_value()) {
    Trap = ErrCode(ErrCode::Value::ComponentAsyncAborted);
  }
  Aborting = true;
  Waiting.clear();
  // Resume every still-running vehicle with Abort so its body unwinds and
  // its thread exits before any HandoffSem is destroyed. Index-based (the
  // vector may grow if an unwinding body briefly registers a vehicle) and
  // repeated until a full pass finds nothing left to abort.
  bool Progress = true;
  while (Progress) {
    Progress = false;
    for (size_t I = 0; I < Vehicles.size(); ++I) {
      TaskVehicle *V = Vehicles[I].get();
      if (V != nullptr && !V->Finished) {
        resumeVehicle(V, ResumeSignal::Abort);
        Progress = true;
      }
    }
  }
  Vehicles.clear();
  Tasks.clear();
  EmbedderTasks.clear();
  Trap.reset();
  Aborting = false;
}

// ----------------------------------------------------------------------------
// Task-level helpers (spec Task methods).

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

Expect<void> Executor::componentTaskReturn(
    ComponentTask &T, std::vector<ComponentValVariant> Results) noexcept {
  if (T.St == ComponentTask::State::Resolved) {
    spdlog::error(ErrCode::Value::ComponentTaskResolvedTwice);
    spdlog::error("    `task.return` or `task.cancel` called more than once "
                  "for current task"sv);
    return Unexpect(ErrCode::Value::ComponentTaskResolvedTwice);
  }
  if (T.NumBorrows > 0) {
    spdlog::error(ErrCode::Value::ComponentBorrowsRemain);
    spdlog::error("    borrow handles still remain at the end of the call"sv);
    return Unexpect(ErrCode::Value::ComponentBorrowsRemain);
  }
  if (T.OnResolve) {
    EXPECTED_TRY(T.OnResolve(std::move(Results)));
  }
  T.St = ComponentTask::State::Resolved;
  return {};
}

Expect<void> Executor::componentTaskCancel(ComponentTask &T) noexcept {
  if (T.St != ComponentTask::State::CancelDelivered) {
    spdlog::error(ErrCode::Value::ComponentTaskNotCancelled);
    spdlog::error("    `task.cancel` called by task which has not been "
                  "cancelled"sv);
    return Unexpect(ErrCode::Value::ComponentTaskNotCancelled);
  }
  if (T.NumBorrows > 0) {
    spdlog::error(ErrCode::Value::ComponentBorrowsRemain);
    spdlog::error("    borrow handles still remain at the end of the call"sv);
    return Unexpect(ErrCode::Value::ComponentBorrowsRemain);
  }
  if (T.OnResolve) {
    EXPECTED_TRY(T.OnResolve(std::nullopt));
  }
  T.St = ComponentTask::State::Resolved;
  return {};
}

Expect<void> Executor::componentTaskExit(ComponentTask &T) noexcept {
  auto &Conc = T.Inst->getConcurrency();
  if (T.Implicit.Registered) {
    T.Inst->threadRemove(T.Implicit.Index);
    T.Implicit.Registered = false;
  }
  if (T.FTAsync && T.needsExclusive() && Conc.ExclusiveTask == &T) {
    Conc.ExclusiveTask = nullptr;
  }
  if (T.St != ComponentTask::State::Resolved) {
    spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
    spdlog::error("    async-lifted export failed to produce a result"sv);
    return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
  }
  return {};
}

// Deliver a pending cancellation at a cancellable suspension point, or park
// the current vehicle until Ready holds. A task that holds its instance's
// exclusive slot releases it while parked so peers can enter, and reacquires
// it (waiting until free) before resuming.
Expect<ResumeSignal> Executor::componentTaskWait(ComponentTask &T,
                                                 std::function<bool()> Ready,
                                                 bool Cancellable,
                                                 bool AlwaysReleaseExcl,
                                                 bool FastPath) noexcept {
  if (Cancellable && T.St == ComponentTask::State::PendingCancel) {
    T.St = ComponentTask::State::CancelDelivered;
    return ResumeSignal::Cancelled;
  }
  auto &Conc = T.Inst->getConcurrency();
  // A mid-core blocking op releases the instance's exclusive slot only once
  // the task has resolved (so an unresolved sync-lifted task keeps peers out
  // per the backpressure model); the callback loop always releases.
  const bool ReleaseExcl =
      T.needsExclusive() && Conc.ExclusiveTask == &T &&
      (AlwaysReleaseExcl || T.St == ComponentTask::State::Resolved);
  if (ReleaseExcl) {
    Conc.ExclusiveTask = nullptr;
  }
  std::function<bool()> Wrapped;
  if (ReleaseExcl) {
    Wrapped = [Ready = std::move(Ready), &Conc]() {
      return Conc.ExclusiveTask == nullptr && Ready();
    };
  } else {
    Wrapped = std::move(Ready);
  }
  // Fast path (spec Thread.wait_until): when the awaited condition already
  // holds, an event-wait resolves in place instead of parking and reporting
  // STARTED to its caller. Yield / backpressure / suspend points do not take
  // this path — they must actually cede control.
  if (FastPath && Wrapped && Wrapped()) {
    if (ReleaseExcl) {
      Conc.ExclusiveTask = &T;
    }
    return ResumeSignal::Normal;
  }
  EXPECTED_TRY(auto Sig, AsyncRt.parkCurrent(std::move(Wrapped), Cancellable));
  if (ReleaseExcl && Sig != ResumeSignal::Abort) {
    Conc.ExclusiveTask = &T;
  }
  return Sig;
}

// ----------------------------------------------------------------------------
// The canon-lift task body (spec canon_lift + thread_func).

Expect<void> Executor::runComponentTaskBody(ComponentTask &T) noexcept {
  auto &Conc = T.Inst->getConcurrency();

  // --- enter_implicit_thread: backpressure gating for async-typed tasks.
  if (T.FTAsync) {
    auto HasBackpressure = [&Conc, &T]() {
      return Conc.Backpressure > 0 ||
             (T.needsExclusive() && Conc.ExclusiveTask != nullptr);
    };
    if (HasBackpressure() || Conc.NumWaitingToEnter > 0) {
      Conc.NumWaitingToEnter += 1;
      auto SigOrErr = componentTaskWait(
          T, [HasBackpressure]() { return !HasBackpressure(); },
          /*Cancellable=*/true);
      // Teardown wake-ups must not touch instance state: the instance may
      // already be gone.
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return trapAborted();
      }
      Conc.NumWaitingToEnter -= 1;
      if (Sig == ResumeSignal::Cancelled) {
        return componentTaskCancel(T);
      }
    }
    if (T.needsExclusive()) {
      Conc.ExclusiveTask = &T;
    }
  }
  T.Implicit.Owner = &T;
  T.Implicit.Vehicle = T.Vehicle;
  T.Implicit.Index = T.Inst->threadAdd(&T.Implicit);
  T.Implicit.Registered = true;

  // --- task.start: produce and lower the arguments.
  T.St = ComponentTask::State::Started;
  EXPECTED_TRY(auto Args, T.OnStart());

  const auto ParamTypes = paramTypesOf(*T.FT);
  CanonicalABI::CanonCtx Cx{this, T.Mem, T.Realloc, T.Inst,
                            {},   {},    nullptr,   T.Enc};
  Cx.BorrowTask = &T;
  Conc.MayLeave = false;
  auto FlatArgsOr = CanonicalABI::lowerFlatValues(Cx, Args, ParamTypes,
                                                  CanonicalABI::MaxFlatParams);
  Conc.MayLeave = true;
  EXPECTED_TRY(auto FlatArgs, std::move(FlatArgsOr));

  if (T.Core == nullptr) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  const auto &CoreParams = T.Core->getFuncType().getParamTypes();

  if (!T.OptAsync) {
    // --- sync-lifted (the function type itself may still be async).
    EXPECTED_TRY(auto CoreRets, invoke(T.Core, FlatArgs, CoreParams));
    if (T.St != ComponentTask::State::Resolved) {
      const auto ResultTypes = resultTypesOf(*T.FT);
      CanonicalABI::FlatIter VI(CoreRets);
      EXPECTED_TRY(auto Results,
                   CanonicalABI::liftFlatValues(Cx, VI, ResultTypes,
                                                CanonicalABI::MaxFlatResults));
      EXPECTED_TRY(componentTaskReturn(T, std::move(Results)));
    }
    if (T.PostReturn != nullptr) {
      std::vector<ValVariant> PRArgs;
      PRArgs.reserve(CoreRets.size());
      for (const auto &P : CoreRets) {
        PRArgs.push_back(P.first);
      }
      Conc.MayLeave = false;
      auto PRRes = invoke(T.PostReturn, PRArgs,
                          T.PostReturn->getFuncType().getParamTypes());
      Conc.MayLeave = true;
      if (!PRRes) {
        return Unexpect(PRRes.error());
      }
    }
    return componentTaskExit(T);
  }

  if (T.Callback == nullptr) {
    // --- async stackful: the core function blocks through built-ins.
    EXPECTED_TRY(invoke(T.Core, FlatArgs, CoreParams));
    return componentTaskExit(T);
  }

  // --- async with callback: run the event loop.
  EXPECTED_TRY(auto First, invoke(T.Core, FlatArgs, CoreParams));
  if (First.empty()) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  uint32_t Packed = First[0].first.get<uint32_t>();
  while (true) {
    const uint32_t Code = Packed & 0xFU;
    const uint32_t SetIdx = Packed >> 4;
    if (Code > static_cast<uint32_t>(AsyncComp::AsyncCallbackCode::Max)) {
      spdlog::error(ErrCode::Value::ComponentBadCallbackCode);
      spdlog::error("    unsupported callback code"sv);
      return Unexpect(ErrCode::Value::ComponentBadCallbackCode);
    }
    if (Code == static_cast<uint32_t>(AsyncComp::AsyncCallbackCode::Exit)) {
      break;
    }
    // componentTaskWait releases and reacquires the exclusive slot around
    // the park, so peers may enter the instance while this task waits.
    AsyncComp::AsyncEvent Ev;
    if (Code == static_cast<uint32_t>(AsyncComp::AsyncCallbackCode::Yield)) {
      auto SigOrErr = componentTaskWait(
          T, []() { return true; }, /*Cancellable=*/true,
          /*AlwaysReleaseExcl=*/true);
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return trapAborted();
      }
      if (Sig == ResumeSignal::Cancelled) {
        Ev = {AsyncComp::AsyncEventCode::TaskCancelled, 0, 0};
      } else {
        Ev = {AsyncComp::AsyncEventCode::None, 0, 0};
      }
    } else {
      auto *WSet = T.Inst->waitableSetGet(SetIdx);
      if (WSet == nullptr) {
        spdlog::error(ErrCode::Value::ComponentHandleUnknown);
        spdlog::error("    unknown handle index {}"sv, SetIdx);
        return Unexpect(ErrCode::Value::ComponentHandleUnknown);
      }
      WSet->NumWaiting += 1;
      auto SigOrErr = componentTaskWait(
          T, [WSet]() { return WSet->hasPendingEvent(); },
          /*Cancellable=*/true, /*AlwaysReleaseExcl=*/true,
          /*FastPath=*/true);
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return trapAborted();
      }
      WSet->NumWaiting -= 1;
      if (Sig == ResumeSignal::Cancelled) {
        Ev = {AsyncComp::AsyncEventCode::TaskCancelled, 0, 0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    std::array<ValVariant, 3> CbArgs{ValVariant(static_cast<uint32_t>(Ev.Code)),
                                     ValVariant(Ev.P1), ValVariant(Ev.P2)};
    EXPECTED_TRY(
        auto CbRets,
        invoke(T.Callback, CbArgs, T.Callback->getFuncType().getParamTypes()));
    if (CbRets.empty()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    Packed = CbRets[0].first.get<uint32_t>();
  }
  return componentTaskExit(T);
}

// ----------------------------------------------------------------------------
// canon lift entry: build the task for a lifted component function and run
// it either nested on the current vehicle (sync function types) or on its
// own vehicle (async function types, eagerly resumed until first block).

Expect<ComponentTask *> Executor::componentLiftCall(
    const Runtime::Instance::Component::FunctionInstance *FuncInst,
    ComponentTask::OnStartFn OnStart, ComponentTask::OnResolveFn OnResolve,
    ComponentTask *CallerTask) noexcept {
  const auto *Inst = FuncInst->getComponentInstance();

  ComponentTask *T = AsyncRt.newTask();
  T->FT = &FuncInst->getFuncType();
  T->FTAsync = FuncInst->getFuncType().isAsync();
  T->Inst = Inst;
  T->Core = FuncInst->getLowerFunction();
  T->Mem = FuncInst->getMemoryInstance();
  T->Realloc = FuncInst->getAllocFunction();
  T->PostReturn = FuncInst->getPostReturnFunction();
  T->Callback = FuncInst->getCallbackFunction();
  T->Enc = FuncInst->getStringEncoding();
  T->OptAsync = FuncInst->isAsyncLifted();
  T->AlwaysTaskReturn = FuncInst->isAlwaysTaskReturn();
  T->OnStart = std::move(OnStart);
  T->OnResolve = std::move(OnResolve);
  T->CallerTask = CallerTask;
  T->CallerInst = CallerTask != nullptr ? CallerTask->Inst : nullptr;

  if (!T->FTAsync) {
    // Nested synchronous execution on the current vehicle. The instance
    // reentrance guard mirrors the existing sync semantics.
    if (Inst != nullptr && Inst->isEntered()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      spdlog::error("    cannot enter component instance"sv);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
    struct EnterGuard {
      const ComponentInstance *P;
      ~EnterGuard() {
        if (P != nullptr) {
          P->setEntered(false);
        }
      }
    } Guard{Inst};
    if (Inst != nullptr) {
      Inst->setEntered(true);
    }
    AsyncRt.pushNestedTask(T);
    auto Res = runComponentTaskBody(*T);
    AsyncRt.popNestedTask();
    if (!Res) {
      T->Failed = Res.error();
      AsyncRt.noteTrap(Res.error(), Inst);
      return Unexpect(Res.error());
    }
    return T;
  }

  // Async function type: dedicated vehicle, run eagerly until first block.
  AsyncRt.newVehicle(T, [this, T](ResumeSignal Sig) {
    if (Sig == ResumeSignal::Abort) {
      return;
    }
    auto Res = runComponentTaskBody(*T);
    if (!Res) {
      T->Failed = Res.error();
      AsyncRt.noteTrap(Res.error(), T->Inst);
    }
  });
  AsyncRt.resumeVehicle(T->Vehicle, ResumeSignal::Normal);
  if (AsyncRt.trapLatch().has_value()) {
    return Unexpect(*AsyncRt.trapLatch());
  }
  return T;
}

void Executor::componentRequestCancellation(ComponentTask &T) noexcept {
  // Spec Task.request_cancellation. The waiting-to-enter and cancellable
  // suspension points wake with the Cancelled signal; otherwise the cancel
  // stays pending until the next cancellable suspension.
  if (T.St == ComponentTask::State::Initial) {
    T.St = ComponentTask::State::CancelDelivered;
    if (T.Vehicle != nullptr) {
      AsyncRt.resumeParked(T.Vehicle, ResumeSignal::Cancelled);
    }
    return;
  }
  if (T.St != ComponentTask::State::Started) {
    return;
  }
  if (T.Vehicle != nullptr && T.Vehicle->Cancellable) {
    T.St = ComponentTask::State::CancelDelivered;
    if (AsyncRt.resumeParked(T.Vehicle, ResumeSignal::Cancelled)) {
      return;
    }
    T.St = ComponentTask::State::PendingCancel;
    return;
  }
  T.St = ComponentTask::State::PendingCancel;
}

Expect<void> Executor::componentResourceDtorCall(
    const Runtime::Instance::ComponentInstance *Impl,
    Runtime::Instance::FunctionInstance *Dtor, uint32_t Rep) noexcept {
  // Implicit synchronous destructor task: current-task context switches to
  // the implementing instance so canonical built-ins called inside the
  // destructor act on its state.
  ComponentTask *T = AsyncRt.newTask();
  T->Inst = Impl;
  T->CallerTask = AsyncRt.currentTask();
  T->CallerInst = T->CallerTask != nullptr ? T->CallerTask->Inst : nullptr;
  T->St = ComponentTask::State::Started;
  if (Impl != nullptr) {
    T->Implicit.Owner = T;
    T->Implicit.Index = Impl->threadAdd(&T->Implicit);
    T->Implicit.Registered = true;
  }
  AsyncRt.pushNestedTask(T);
  std::array<ValVariant, 1> DtorArgs{ValVariant(Rep)};
  std::array<ValType, 1> DtorTypes{ValType(TypeCode::I32)};
  auto Res = invoke(Dtor, DtorArgs, DtorTypes);
  AsyncRt.popNestedTask();
  if (T->Implicit.Registered && Impl != nullptr) {
    Impl->threadRemove(T->Implicit.Index);
    T->Implicit.Registered = false;
  }
  T->St = ComponentTask::State::Resolved;
  if (!Res) {
    return Unexpect(Res.error());
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
