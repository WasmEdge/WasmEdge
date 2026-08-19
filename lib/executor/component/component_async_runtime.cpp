// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_async_runtime.cpp - async task runtime ------------------===//
//
// The task scheduler of the async proposal: strict hand-off between
// OS-thread vehicles, the four canon-lift task bodies, and trap propagation.
//
//===----------------------------------------------------------------------===//

#include "executor/component/async_runtime.h"
#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errcode.h"
#include "common/spdlog.h"

#include <algorithm>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using Runtime::Instance::ComponentInstance;

namespace Component {
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
  Thread = std::thread([this, Fn = std::move(Body)]() {
    Run.acquire();
    try {
      Fn(Arg);
    } catch (...) {
      // During teardown, a HandoffSem operation on an already-destroyed peer
      // can throw. The body then has nothing left to do.
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
AsyncRuntime::newVehicle(Task *T,
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
AsyncRuntime::newSpawnVehicle(Task *T, ThreadCtx *Ctx,
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
  // The instance owns its threads: hook its destructor so they end with it.
  if (T != nullptr && T->Inst != nullptr) {
    auto &Conc = T->Inst->concurrency();
    if (!Conc.OnDestroy) {
      const auto *Inst = T->Inst;
      Conc.OnDestroy = [this, Inst]() { abortThreadsOf(Inst); };
      Adopted.push_back(Inst);
    }
  }
  return V;
}

void AsyncRuntime::abortThreadsOf(
    const Runtime::Instance::ComponentInstance *Inst) noexcept {
  Adopted.erase(std::remove(Adopted.begin(), Adopted.end(), Inst),
                Adopted.end());
  auto Owned = [Inst](const TaskVehicle *V) {
    return V != nullptr && V->Owner != nullptr && V->Owner->Inst == Inst;
  };
  // A vehicle resumed with Abort must not park again while this runs.
  const bool WasAborting = Aborting;
  Aborting = true;
  bool Progress = true;
  while (Progress) {
    Progress = false;
    for (size_t I = 0; I < Vehicles.size(); ++I) {
      TaskVehicle *V = Vehicles[I].get();
      if (V != nullptr && !V->Finished && Owned(V)) {
        Waiting.erase(std::remove(Waiting.begin(), Waiting.end(), V),
                      Waiting.end());
        resumeVehicle(V, ResumeSignal::Abort);
        Progress = true;
      }
    }
  }
  Aborting = WasAborting;
  Vehicles.erase(
      std::remove_if(Vehicles.begin(), Vehicles.end(),
                     [&Owned](const std::unique_ptr<TaskVehicle> &V) {
                       return Owned(V.get());
                     }),
      Vehicles.end());
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
                                               bool Cancellable,
                                               const Task *Pin) noexcept {
  // During teardown a resumed vehicle must unwind rather than re-park.
  if (Aborting) {
    return ResumeSignal::Abort;
  }
  TaskVehicle *Self = Current;
  if (Self == nullptr) {
    // Embedder thread: cannot park. Pump other vehicles until ready. With no
    // condition the activation is suspended until another thread resumes it.
    if (!Ready) {
      ThreadCtx *Ctx = currentCtx();
      if (Ctx == nullptr) {
        spdlog::error(ErrCode::Value::ComponentAsyncDeadlock);
        spdlog::error(
            "    deadlock detected: event loop cannot make further progress"sv);
        return Unexpect(ErrCode::Value::ComponentAsyncDeadlock);
      }
      Ctx->Suspended = true;
      EXPECTED_TRY(pumpUntil([Ctx]() { return !Ctx->Suspended; }, Pin));
      return ResumeSignal::Normal;
    }
    EXPECTED_TRY(pumpUntil(Ready, Pin));
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

bool AsyncRuntime::canResumeUnder(const Task &Pin,
                                  const TaskVehicle &V) const noexcept {
  // Resuming a thread of another instance would be unexpected reentrance.
  if (V.Owner == nullptr || V.Owner->Inst != Pin.Inst) {
    return false;
  }
  // The implicit thread of a task that needs the stack exclusively cannot run
  // while the pinned call holds it. Threads it spawned still can.
  return V.CtxStack.empty() || V.CtxStack.back() != &V.Owner->Implicit ||
         !V.Owner->needsExclusive();
}

Expect<void> AsyncRuntime::pumpUntil(const std::function<bool()> &Done,
                                     const Task *Pin) noexcept {
  // A non-async-typed call in progress restricts what may take the stack.
  const Task *Restrict =
      Pin != nullptr && !Pin->mayBlockAlways() ? Pin : nullptr;
  while (!Done()) {
    if (Trap.has_value()) {
      return Unexpect(*Trap);
    }
    TaskVehicle *Pick = nullptr;
    for (size_t I = 0; I < Waiting.size(); ++I) {
      TaskVehicle *V = Waiting[I];
      if (V->ReadyFn && V->ReadyFn() &&
          (Restrict == nullptr || canResumeUnder(*Restrict, *V))) {
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

void AsyncRuntime::noteTrap(
    ErrCode Err, const Runtime::Instance::ComponentInstance *Inst) noexcept {
  if (Err.getEnum() == ErrCode::Value::ComponentAsyncAborted) {
    return;
  }
  if (!Trap.has_value()) {
    Trap = Err;
  }
  if (Inst != nullptr) {
    Inst->getRoot()->concurrency().Poisoned = true;
  }
}

void AsyncRuntime::teardown() noexcept {
  // Entered only on the embedder thread, with every live vehicle parked or
  // finished. Resume them with Abort, then the destructors join.
  if (!Trap.has_value()) {
    Trap = ErrCode(ErrCode::Value::ComponentAsyncAborted);
  }
  Aborting = true;
  Waiting.clear();
  // Abort every running vehicle so its thread exits before any HandoffSem
  // dies. The loop is index-based, because an unwinding body can grow it.
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
  // No instance may call back into a runtime that has nothing left to abort.
  for (const auto *Inst : Adopted) {
    Inst->concurrency().OnDestroy = nullptr;
  }
  Adopted.clear();
  Trap.reset();
  Aborting = false;
}

// ----------------------------------------------------------------------------
// Task-level helpers.

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

Expect<void>
AsyncRuntime::taskReturn(Task &T,
                         std::vector<ComponentValVariant> Results) noexcept {
  if (T.St == Task::State::Resolved) {
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
  T.St = Task::State::Resolved;
  return {};
}

Expect<void> AsyncRuntime::taskCancel(Task &T) noexcept {
  if (T.St != Task::State::CancelDelivered) {
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
  T.St = Task::State::Resolved;
  return {};
}

Expect<void> AsyncRuntime::taskExit(Task &T) noexcept {
  auto &Conc = T.Inst->concurrency();
  if (T.Implicit.Registered) {
    T.Inst->concurrency().threadRemove(T.Implicit.Index);
    T.Implicit.Registered = false;
  }
  if (T.FTAsync && T.needsExclusive() && Conc.ExclusiveTask == &T) {
    Conc.ExclusiveTask = nullptr;
  }
  if (T.St != Task::State::Resolved) {
    spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
    spdlog::error("    async-lifted export failed to produce a result"sv);
    return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
  }
  return {};
}

// Whether blocking here would stall the whole call. A synchronous task keeps
// the stack, so it may only block while another thread of its own component
// instance is ready to take over.
bool AsyncRuntime::mayBlock(const Task &T) const noexcept {
  if (T.mayBlockAlways()) {
    return true;
  }
  // An activation driven by the embedder thread is ready to take the stack
  // back unless it is itself suspended. A task nested synchronously on the
  // current vehicle is not another thread and does not count.
  if (Current != nullptr && !T.Implicit.Suspended &&
      std::find(EmbedderCtxs.begin(), EmbedderCtxs.end(), &T.Implicit) !=
          EmbedderCtxs.end()) {
    return true;
  }
  // Any other ready thread of the same instance tree makes blocking legal,
  // even a sibling instance's thread that the event loop then refuses to
  // resume. Whether one may actually take the stack is the event loop's
  // decision, and a pump that finds none reports the deadlock instead.
  const auto *Root = T.Inst != nullptr ? T.Inst->getRoot() : nullptr;
  for (const TaskVehicle *V : Waiting) {
    if (V == Current || V->Finished || !V->ReadyFn || !V->ReadyFn()) {
      continue;
    }
    if (V->Owner == nullptr || V->Owner->Inst == nullptr ||
        V->Owner->Inst->getRoot() != Root) {
      continue;
    }
    return true;
  }
  return false;
}

// Deliver a pending cancellation, or park the current vehicle until Ready
// holds. A parked task releases its exclusive slot and reacquires it after.
Expect<ResumeSignal>
AsyncRuntime::taskWait(Task &T, std::function<bool()> Ready, bool Cancellable,
                       bool AlwaysReleaseExcl, bool FastPath) noexcept {
  if (Cancellable && T.St == Task::State::PendingCancel) {
    T.St = Task::State::CancelDelivered;
    return ResumeSignal::Cancelled;
  }
  auto &Conc = T.Inst->concurrency();
  // A mid-core blocking operation releases the exclusive slot only once the
  // task resolves. The callback loop always releases it.
  const bool ReleaseExcl = T.needsExclusive() && Conc.ExclusiveTask == &T &&
                           (AlwaysReleaseExcl || T.St == Task::State::Resolved);
  if (ReleaseExcl) {
    Conc.ExclusiveTask = nullptr;
  }
  std::function<bool()> Wrapped;
  if (ReleaseExcl) {
    Wrapped = [Cond = std::move(Ready), &Conc]() {
      return Conc.ExclusiveTask == nullptr && Cond();
    };
  } else {
    Wrapped = std::move(Ready);
  }
  // Fast path: an event-wait whose condition already holds resolves in
  // place. Yield, backpressure, and suspend must cede.
  if (FastPath && Wrapped && Wrapped()) {
    if (ReleaseExcl) {
      Conc.ExclusiveTask = &T;
    }
    return ResumeSignal::Normal;
  }
  EXPECTED_TRY(auto Sig, parkCurrent(std::move(Wrapped), Cancellable, &T));
  if (ReleaseExcl && Sig != ResumeSignal::Abort) {
    Conc.ExclusiveTask = &T;
  }
  return Sig;
}

// ----------------------------------------------------------------------------
// The canon-lift task body.

Expect<void> AsyncRuntime::runTaskBody(Task &T) noexcept {
  auto &Conc = T.Inst->concurrency();

  // Backpressure gating on entering the implicit thread.
  if (T.FTAsync) {
    auto HasBackpressure = [&Conc, &T]() {
      return Conc.Backpressure > 0 ||
             (T.needsExclusive() && Conc.ExclusiveTask != nullptr);
    };
    if (HasBackpressure() || Conc.NumWaitingToEnter > 0) {
      Conc.NumWaitingToEnter += 1;
      auto SigOrErr = taskWait(
          T, [HasBackpressure]() { return !HasBackpressure(); },
          /*Cancellable=*/true);
      // A teardown wake-up must not touch the instance state, because the
      // instance can already be gone.
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return trapAborted();
      }
      Conc.NumWaitingToEnter -= 1;
      if (Sig == ResumeSignal::Cancelled) {
        return taskCancel(T);
      }
    }
    if (T.needsExclusive()) {
      Conc.ExclusiveTask = &T;
    }
  }
  T.Implicit.Owner = &T;
  T.Implicit.Vehicle = T.Vehicle;
  T.Implicit.Index = T.Inst->concurrency().threadAdd(&T.Implicit);
  T.Implicit.Registered = true;

  // Produce and lower the arguments.
  T.St = Task::State::Started;
  EXPECTED_TRY(auto Args, T.OnStart());

  const auto ParamTypes = paramTypesOf(*T.FT);
  Component::CanonicalABI::CanonCtx Cx{&Exec, T.Mem, T.Realloc, T.Inst,
                                       {},    {},    nullptr,   T.Enc};
  Cx.BorrowTask = &T;
  Conc.MayLeave = false;
  auto FlatArgsOr = Component::CanonicalABI::lowerFlatValues(
      Cx, Args, ParamTypes, Component::CanonicalABI::MaxFlatParams);
  Conc.MayLeave = true;
  EXPECTED_TRY(auto FlatArgs, std::move(FlatArgsOr));

  if (T.Core == nullptr) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  const auto &CoreParams = T.Core->getFuncType().getParamTypes();

  if (!T.OptAsync) {
    // Sync-lifted; the function type itself may still be async.
    EXPECTED_TRY(auto CoreRets,
                 Exec.core().invoke(T.Core, FlatArgs, CoreParams));
    if (T.St != Task::State::Resolved) {
      const auto ResultTypes = resultTypesOf(*T.FT);
      Component::CanonicalABI::FlatIter VI(CoreRets);
      EXPECTED_TRY(auto Results, Component::CanonicalABI::liftFlatValues(
                                     Cx, VI, ResultTypes,
                                     Component::CanonicalABI::MaxFlatResults));
      EXPECTED_TRY(taskReturn(T, std::move(Results)));
    }
    if (T.PostReturn != nullptr) {
      std::vector<ValVariant> PRArgs;
      PRArgs.reserve(CoreRets.size());
      for (const auto &P : CoreRets) {
        PRArgs.push_back(P.first);
      }
      Conc.MayLeave = false;
      auto PRRes = Exec.core().invoke(
          T.PostReturn, PRArgs, T.PostReturn->getFuncType().getParamTypes());
      Conc.MayLeave = true;
      if (!PRRes) {
        return Unexpect(PRRes.error());
      }
    }
    return taskExit(T);
  }

  if (T.Callback == nullptr) {
    // Async stackful: the core function blocks through built-ins.
    EXPECTED_TRY(Exec.core().invoke(T.Core, FlatArgs, CoreParams));
    return taskExit(T);
  }

  // Async with callback: run the event loop.
  EXPECTED_TRY(auto First, Exec.core().invoke(T.Core, FlatArgs, CoreParams));
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
    // taskWait releases the exclusive slot around the park, so a
    // peer can enter the instance while this task waits.
    Runtime::Instance::Component::AsyncEvent Ev;
    if (Code == static_cast<uint32_t>(
                    Runtime::Instance::Component::AsyncCallbackCode::Yield)) {
      auto SigOrErr = taskWait(
          T, []() { return true; }, /*Cancellable=*/true,
          /*AlwaysReleaseExcl=*/true);
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return trapAborted();
      }
      if (Sig == ResumeSignal::Cancelled) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else {
        Ev = {Runtime::Instance::Component::AsyncEventCode::None, 0, 0};
      }
    } else {
      auto *WSet = T.Inst->handles().waitableSetGet(SetIdx);
      if (WSet == nullptr) {
        spdlog::error(ErrCode::Value::ComponentHandleUnknown);
        spdlog::error("    unknown handle index {}"sv, SetIdx);
        return Unexpect(ErrCode::Value::ComponentHandleUnknown);
      }
      WSet->NumWaiting += 1;
      auto SigOrErr = taskWait(
          T, [WSet]() { return WSet->hasPendingEvent(); },
          /*Cancellable=*/true, /*AlwaysReleaseExcl=*/true,
          /*FastPath=*/true);
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return trapAborted();
      }
      WSet->NumWaiting -= 1;
      if (Sig == ResumeSignal::Cancelled) {
        Ev = {Runtime::Instance::Component::AsyncEventCode::TaskCancelled, 0,
              0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    std::array<ValVariant, 3> CbArgs{ValVariant(static_cast<uint32_t>(Ev.Code)),
                                     ValVariant(Ev.P1), ValVariant(Ev.P2)};
    EXPECTED_TRY(auto CbRets,
                 Exec.core().invoke(T.Callback, CbArgs,
                                    T.Callback->getFuncType().getParamTypes()));
    if (CbRets.empty()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    Packed = CbRets[0].first.get<uint32_t>();
  }
  return taskExit(T);
}

// ----------------------------------------------------------------------------
// canon lift entry: build the task for a lifted function, then run it nested
// on the current vehicle (sync) or on its own vehicle (async).

Expect<Task *> AsyncRuntime::liftCall(
    const Runtime::Instance::Component::FunctionInstance *FuncInst,
    Task::OnStartFn OnStart, Task::OnResolveFn OnResolve,
    Task *CallerTask) noexcept {
  const auto *Inst = FuncInst->getComponentInstance();

  Task *T = newTask();
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
  T->OnStart = std::move(OnStart);
  T->OnResolve = std::move(OnResolve);
  T->CallerTask = CallerTask;
  T->CallerInst = CallerTask != nullptr ? CallerTask->Inst : nullptr;

  if (!T->FTAsync) {
    // Nested synchronous execution on the current vehicle. The instance
    // reentrance guard mirrors the existing sync semantics.
    if (Inst != nullptr && Inst->concurrency().entered()) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      spdlog::error("    cannot enter component instance"sv);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
    std::optional<Runtime::Instance::Component::ConcurrencyState::EnterGuard>
        Guard;
    if (Inst != nullptr) {
      Guard.emplace(Inst->concurrency());
    }
    pushNestedTask(T);
    auto Res = runTaskBody(*T);
    popNestedTask();
    if (!Res) {
      T->Failed = Res.error();
      noteTrap(Res.error(), Inst);
      return Unexpect(Res.error());
    }
    return T;
  }

  // Async function type: dedicated vehicle, run eagerly until first block.
  newVehicle(T, [this, T](ResumeSignal Sig) {
    if (Sig == ResumeSignal::Abort) {
      return;
    }
    auto Res = runTaskBody(*T);
    if (!Res) {
      T->Failed = Res.error();
      noteTrap(Res.error(), T->Inst);
    }
  });
  resumeVehicle(T->Vehicle, ResumeSignal::Normal);
  if (trapLatch().has_value()) {
    return Unexpect(*trapLatch());
  }
  return T;
}

void AsyncRuntime::requestCancellation(Task &T) noexcept {
  // A cancellable suspension point wakes with the Cancelled signal,
  // otherwise the cancel stays pending.
  if (T.St == Task::State::Initial) {
    T.St = Task::State::CancelDelivered;
    if (T.Vehicle != nullptr) {
      resumeParked(T.Vehicle, ResumeSignal::Cancelled);
    }
    return;
  }
  if (T.St != Task::State::Started) {
    return;
  }
  if (T.Vehicle != nullptr && T.Vehicle->Cancellable) {
    T.St = Task::State::CancelDelivered;
    if (resumeParked(T.Vehicle, ResumeSignal::Cancelled)) {
      return;
    }
    T.St = Task::State::PendingCancel;
    return;
  }
  T.St = Task::State::PendingCancel;
}

Expect<void>
AsyncRuntime::resourceDtorCall(const Runtime::Instance::ComponentInstance *Impl,
                               Runtime::Instance::FunctionInstance *Dtor,
                               uint64_t Rep) noexcept {
  // Implicit synchronous destructor task. The current-task context switches
  // to the implementing instance, so built-ins act on its state.
  Task *T = newTask();
  T->Inst = Impl;
  T->CallerTask = currentTask();
  T->CallerInst = T->CallerTask != nullptr ? T->CallerTask->Inst : nullptr;
  T->St = Task::State::Started;
  if (Impl != nullptr) {
    T->Implicit.Owner = T;
    T->Implicit.Index = Impl->concurrency().threadAdd(&T->Implicit);
    T->Implicit.Registered = true;
  }
  pushNestedTask(T);
  std::array<ValVariant, 1> DtorArgs{ValVariant(Rep)};
  std::array<ValType, 1> DtorTypes{ValType(TypeCode::I32)};
  auto Res = Exec.core().invoke(Dtor, DtorArgs, DtorTypes);
  popNestedTask();
  if (T->Implicit.Registered && Impl != nullptr) {
    Impl->concurrency().threadRemove(T->Implicit.Index);
    T->Implicit.Registered = false;
  }
  T->St = Task::State::Resolved;
  if (!Res) {
    return Unexpect(Res.error());
  }
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
