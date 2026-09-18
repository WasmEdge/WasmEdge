// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "system/poll.h"

#include "common/spdlog.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Run the task of a lifted function. See
// "include/executor/component/executor.h".
Expect<Runtime::Component::Task *> ComponentExecutor::liftCall(
    const Runtime::Instance::ComponentFunctionInstance *FuncInst,
    Runtime::Component::Task::OnStartCallback OnStart,
    Runtime::Component::Task::OnResolveCallback OnResolve,
    Runtime::Component::Task *Caller,
    Runtime::Component::Thread *CallerThread) {
  auto *Inst = FuncInst->getComponentInstance();
  auto *Store = Inst->getRoot();
  Runtime::Component::Task *T = Store->newTask(*FuncInst, std::move(OnStart),
                                               std::move(OnResolve), Caller);
  // A synchronous guest callee shares its caller's stack; others get one.
  if (CallerThread != nullptr && !T->isFuncTypeAsync() && !T->isHost()) {
    Runtime::Component::Thread *Nested = Store->newThread(*T, *CallerThread);
    auto Res = runGuestTask(*T, *Nested);
    Nested->onEnd();
    // After an abort the store of the callee may be gone.
    if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
      return Unexpect(Res);
    }
    if (!Res) {
      setTrap(Res.error(), *Inst);
      return Unexpect(Res);
    }
    return T;
  }
  Runtime::Component::Thread *Stack =
      newRootThread(*Store, *T, [this, T, Inst]() {
        Runtime::Component::Thread *Cur = getCurrentThread(*Inst);
        assuming(Cur != nullptr);
        auto Res = T->isHost() ? runHostTask(*T, *Cur) : runGuestTask(*T, *Cur);
        if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
          setTrap(Res.error(), *Inst);
        }
      });
  resumeThread(*Stack, Runtime::Component::Thread::WakeReason::Normal);
  return T;
}

// Run the body of a guest task. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runGuestTask(Runtime::Component::Task &T,
                                             Runtime::Component::Thread &Cur) {
  const Runtime::Component::Thread::WakeReason Entered = T.onEnter(Cur);
  if (Entered == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  if (T.isCancelDelivered()) {
    // Cancelled while waiting to enter: it resolves as cancelled unstarted.
    EXPECTED_TRY(T.onCancel());
    return T.onExit();
  }

  const auto &Opts = T.getCanonOptions();
  auto *Inst = Opts.Inst;
  const auto *TypeInst = T.getTypeInstance();
  const auto *FuncType = T.getFuncType();
  auto Body = [&]() -> Expect<void> {
    // The callee may not leave the instance while its arguments lower.
    EXPECTED_TRY(auto Args, T.onStart());
    std::vector<ValVariant> Flat;
    Inst->setMayLeave(false);
    auto Lowered =
        lowerFlatValues(Opts, *TypeInst, FuncType->getParamValTypes(), Args,
                        MaxFlatParams, Flat);
    Inst->setMayLeave(true);
    EXPECTED_TRY(Lowered);

    if (!Opts.Async) {
      // A synchronous lift returns flat results; post-return frees them.
      EXPECTED_TRY(auto FlatRets, invokeCore(T.getCoreFunction(), Flat));
      EXPECTED_TRY(auto Rets, liftFlatValues(Opts, *TypeInst,
                                             FuncType->getResultValTypes(),
                                             FlatRets, MaxFlatResults));
      EXPECTED_TRY(T.onReturn(std::move(Rets)));
      if (Opts.PostReturn != nullptr) {
        Inst->setMayLeave(false);
        auto Post = invokeCore(Opts.PostReturn, FlatRets);
        Inst->setMayLeave(true);
        EXPECTED_TRY(Post);
      }
      return {};
    }
    if (Opts.Callback == nullptr) {
      // An async stackful lift: the core function calls task.return itself.
      EXPECTED_TRY(invokeCore(T.getCoreFunction(), Flat));
      return {};
    }
    // An async callback lift runs its callback until it asks to exit.
    EXPECTED_TRY(auto Packed, invokeCore(T.getCoreFunction(), Flat));
    return runCallbackLoop(T, Cur, Packed[0].get<uint32_t>());
  };
  auto Res = Body();
  // After an abort the instance may be gone.
  if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
    return Res;
  }
  if (!Res) {
    // After a trap the tree is poisoned and the activation never exits.
    return Res;
  }
  return T.onExit();
}

// Run the callback loop of a guest task. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCallbackLoop(Runtime::Component::Task &T,
                                                Runtime::Component::Thread &Cur,
                                                uint32_t Code) {
  const auto &Opts = T.getCanonOptions();
  auto *Inst = Opts.Inst;
  while (true) {
    const uint32_t Kind = Code & 0xFU;
    Component::WaitableEvent Pending;
    if (Kind == static_cast<uint32_t>(CallbackCode::Exit)) {
      break;
    }
    // A pending cancellation is delivered before the code is acted on.
    if (T.onCancelDelivery()) {
      Pending.Code = Component::EventCode::TaskCancelled;
    } else if (Kind == static_cast<uint32_t>(CallbackCode::Yield)) {
      const Runtime::Component::Thread::WakeReason Wake = T.yieldCallback(Cur);
      if (Wake == Runtime::Component::Thread::WakeReason::Aborted) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      if (T.onCancelDelivery()) {
        Pending.Code = Component::EventCode::TaskCancelled;
      }
    } else if (Kind == static_cast<uint32_t>(CallbackCode::Wait)) {
      EXPECTED_TRY(Pending, waitOnWaitableSet(T, Cur, *Inst, Code >> 4, true));
    } else {
      spdlog::error(ErrCode::Value::ComponentBadCallbackCode);
      return Unexpect(ErrCode::Value::ComponentBadCallbackCode);
    }
    EXPECTED_TRY(
        auto Next,
        invokeCore(Opts.Callback,
                   std::vector<ValVariant>{
                       ValVariant(static_cast<uint32_t>(Pending.Code)),
                       ValVariant(Pending.Idx), ValVariant(Pending.Payload)}));
    Code = Next[0].get<uint32_t>();
  }
  return {};
}

// Run the body of a host task. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runHostTask(Runtime::Component::Task &T,
                                            Runtime::Component::Thread &Cur) {
  const Runtime::Component::Thread::WakeReason Entered = T.onEnter(Cur);
  if (Entered == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  EXPECTED_TRY(auto Args, T.onStart());
  // The ends the arguments carry enter the table of the root of the host,
  // and the ends the results name leave it: the body only sees handles.
  auto &HostRoot = *T.getInstance()->getRoot();
  for (auto &Arg : Args) {
    EXPECTED_TRY(lowerHostStreams(HostRoot, Arg));
  }
  std::vector<ComponentValVariant> Rets(T.getFuncType()->getResultArity());
  Runtime::Component::CallingFrame Frame(T, Cur, *this);
  EXPECTED_TRY(T.getHostFunction()->run(Frame, Args, Rets));
  for (auto &Ret : Rets) {
    EXPECTED_TRY(liftHostStreams(HostRoot, Ret));
  }
  if (!T.isResolved()) {
    EXPECTED_TRY(T.onReturn(std::move(Rets)));
  }
  return T.onExit();
}

// Run the body of a spawned host task. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runSpawnedTask(
    Runtime::Component::Task &T, Runtime::Component::Thread &Cur,
    const Runtime::Component::Task::SpawnBody &Body) {
  const Runtime::Component::Thread::WakeReason Entered = T.onEnter(Cur);
  if (Entered == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Runtime::Component::CallingFrame Frame(T, Cur, *this);
  auto Res = Body(Frame);
  if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
    return Res;
  }
  if (!T.isResolved()) {
    static_cast<void>(T.onReturn({}));
  }
  EXPECTED_TRY(Res);
  return T.onExit();
}

// Run a resource destructor. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runResourceDtor(
    Runtime::Instance::ComponentInstance *Impl, Runtime::Component::Thread *Cur,
    Runtime::Instance::FunctionInstance *Dtor, uint64_t Rep) {
  return runImplicitTask(Impl, Cur, [this, Dtor, Rep]() -> Expect<void> {
    const auto &Params = Dtor->getFuncType().getParamTypes();
    const ValVariant Slot =
        !Params.empty() && Params[0].getCode() == TypeCode::I64
            ? ValVariant(Rep)
            : ValVariant(static_cast<uint32_t>(Rep));
    EXPECTED_TRY(invokeCore(Dtor, std::vector<ValVariant>{Slot}));
    return {};
  });
}

// Run an implicit synchronous task. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::runImplicitTask(Runtime::Instance::ComponentInstance *Inst,
                                   Runtime::Component::Thread *Cur,
                                   const std::function<Expect<void>()> &Body) {
  auto *Store = Inst->getRoot();
  Runtime::Component::Task *T =
      Store->newTask(Inst, Cur != nullptr ? &Cur->getOwner() : nullptr, false);
  if (Cur != nullptr) {
    Runtime::Component::Thread *Nested = Store->newThread(*T, *Cur);
    const Runtime::Component::Thread::WakeReason Entered = T->onEnter(*Nested);
    if (Entered == Runtime::Component::Thread::WakeReason::Aborted) {
      Nested->onEnd();
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    auto Res = Body();
    if (Res) {
      static_cast<void>(T->onReturn({}));
      Res = T->onExit();
    }
    Nested->onEnd();
    // After an abort the store may be gone.
    if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
      return Res;
    }
    if (!Res) {
      setTrap(Res.error(), *Inst);
    }
    return Res;
  }
  // From the embedder thread: a stack of its own, pumped until it finishes.
  Runtime::Component::Thread *Stack =
      newRootThread(*Store, *T, [this, T, Inst, &Body]() {
        Runtime::Component::Thread *Own = getCurrentThread(*Inst);
        assuming(Own != nullptr);
        if (T->onEnter(*Own) ==
            Runtime::Component::Thread::WakeReason::Aborted) {
          return;
        }
        auto Res = Body();
        if (Res) {
          static_cast<void>(T->onReturn({}));
          Res = T->onExit();
        }
        if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
          setTrap(Res.error(), *Inst);
        }
      });
  resumeThread(*Stack, Runtime::Component::Thread::WakeReason::Normal);
  return pump(*Store, [Stack]() { return Stack->isEnded(); }, Stack);
}

// Wait on a waitable set. See "include/executor/component/executor.h".
Expect<Component::WaitableEvent> ComponentExecutor::waitOnWaitableSet(
    Runtime::Component::Task &T, Runtime::Component::Thread &Cur,
    const Runtime::Instance::ComponentInstance &Inst, uint32_t SetIdx,
    bool BetweenCallbacks) {
  if (Inst.findWaitableSet(SetIdx) == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    unknown handle index {}"sv, SetIdx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  auto Ready = [this, &Inst, SetIdx]() {
    const auto *Members = Inst.findWaitableSet(SetIdx);
    if (Members == nullptr) {
      return true;
    }
    for (const uint32_t Idx : *Members) {
      if (hasPendingEvent(Inst, Idx)) {
        return true;
      }
    }
    return false;
  };
  // An event already pending completes the wait in place.
  const Runtime::Component::Thread::WakeReason Wake =
      BetweenCallbacks ? T.collectCallback(Cur, Ready, SetIdx)
                       : T.collect(Cur, Ready, SetIdx);
  if (Wake == Runtime::Component::Thread::WakeReason::Aborted) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  // Between callbacks the wait also ends on a pending cancellation.
  if (BetweenCallbacks && T.onCancelDelivery()) {
    return Component::WaitableEvent{Component::EventCode::TaskCancelled, 0, 0};
  }
  if (const auto *Members = Inst.findWaitableSet(SetIdx); Members != nullptr) {
    for (const uint32_t Idx : *Members) {
      if (hasPendingEvent(Inst, Idx)) {
        return takePendingEvent(Inst, Idx);
      }
    }
  }
  return Component::WaitableEvent{};
}

// Hand the stack to a thread. See "include/executor/component/executor.h".
void ComponentExecutor::resumeThread(
    Runtime::Component::Thread &Target,
    Runtime::Component::Thread::WakeReason Wake) {
  Runtime::Component::Thread *Next = &Target;
  Runtime::Component::Thread::WakeReason Woke = Wake;
  while (Next != nullptr && !Next->isEnded()) {
    auto *Store = Next->getOwner().getInstance()->getRoot();
    auto *Entry = Store->getEntryRoot();
    Store->Waiting.erase(
        std::remove(Store->Waiting.begin(), Store->Waiting.end(), Next),
        Store->Waiting.end());
    Runtime::Component::Thread *Prev = std::exchange(Entry->Current, Next);
    Runtime::Component::Thread *Named = Next->onResume(Woke);
    Entry->Current = Prev;
    // The parked thread joins the waiting list of its store when it can be
    // picked again, and the tasks the activation spawned start.
    Runtime::Component::Thread *Parked = Next->getInnermost();
    if (Parked != nullptr && Parked->isWaiting() &&
        Parked->getOwner().getInstance() != nullptr) {
      Parked->getOwner().getInstance()->getRoot()->pushWaiting(*Parked);
    }
    startSpawnedTasks(Next->getOwner());
    if (Parked != nullptr && &Parked->getOwner() != &Next->getOwner()) {
      startSpawnedTasks(Parked->getOwner());
    }
    Next = Named;
    Woke = Runtime::Component::Thread::WakeReason::Normal;
  }
  // A non-async-typed guest call that parked runs its own loop over the
  // threads of its instance until it returns, unless one already does.
  Runtime::Component::Thread &Stack = Target.getRoot();
  Runtime::Component::Thread *Parked = Stack.getInnermost();
  if (Stack.isPinned() || Parked == nullptr ||
      !(Parked->isWaiting() || Parked->isSuspended())) {
    return;
  }
  Runtime::Component::Task &Call = Parked->getOwner();
  if (Call.isFuncTypeAsync() || Call.isHost() ||
      Call.getInstance() == nullptr) {
    return;
  }
  // A trap latches on the store, where the caller finds it.
  static_cast<void>(pump(
      *Call.getInstance()->getRoot(),
      [Parked, &Call]() {
        return Parked->isEnded() || Call.isResolved() || Call.isAborted();
      },
      &Stack));
}

// Run the scheduler loop. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::pump(Runtime::Instance::ComponentInstance &Root,
                                     const std::function<bool()> &Done,
                                     Runtime::Component::Thread *Pin) {
  // The non-async-typed guest call in progress on the pinned stack, if any.
  auto SyncCall = [Pin]() -> const Runtime::Component::Task * {
    if (Pin == nullptr || Pin->isEnded()) {
      return nullptr;
    }
    Runtime::Component::Thread *Inner = Pin->getInnermost();
    if (Inner == nullptr || Inner->isEnded()) {
      return nullptr;
    }
    const Runtime::Component::Task &PinTask = Inner->getOwner();
    if (PinTask.isFuncTypeAsync() || PinTask.isHost()) {
      return nullptr;
    }
    return &PinTask;
  };
  // While such a call is in progress, only a thread it may resume takes the
  // stack.
  auto Eligible = [&SyncCall](const Runtime::Component::Thread &Ready) {
    const Runtime::Component::Task *Call = SyncCall();
    return Call == nullptr || Call->mayResume(Ready);
  };
  // The progress of this store needs the host tasks of the stores it imports
  // from.
  const auto Roots = Root.getStoreRoots();
  if (Pin != nullptr) {
    Pin->setPinned(true);
  }
  Expect<void> Res;
  while (!Done()) {
    if (const auto Trap = getTrap(Roots); Trap.has_value()) {
      Res = Unexpect(*Trap);
      break;
    }
    pollWaiting(Roots, false);
    if (Runtime::Component::Thread *Next = findReadyThread(Roots, Eligible);
        Next != nullptr) {
      resumeThread(*Next, Runtime::Component::Thread::WakeReason::Normal);
      continue;
    }
    if (Done()) {
      break;
    }
    // Nothing is ready: wait on the OS for a handle or the earliest sleep.
    if (pollWaiting(Roots, true)) {
      continue;
    }
    // Nothing may run: a synchronous call cannot block before it returns,
    // and a store with nothing to run is deadlocked.
    const ErrCode::Value Code = SyncCall() != nullptr
                                    ? ErrCode::Value::ComponentCannotBlockSync
                                    : ErrCode::Value::ComponentAsyncDeadlock;
    spdlog::error(Code);
    setTrap(ErrCode(Code), Root);
    Res = Unexpect(Code);
    break;
  }
  if (Res) {
    if (const auto Trap = getTrap(Roots); Trap.has_value()) {
      Res = Unexpect(*Trap);
    }
  }
  if (Pin != nullptr) {
    Pin->setPinned(false);
  }
  return Res;
}

// Poll the waiting threads. See "include/executor/component/executor.h".
bool ComponentExecutor::pollWaiting(
    Span<Runtime::Instance::ComponentInstance *const> Roots, bool Block) {
  std::vector<PollEntry> Entries;
  std::vector<std::pair<Runtime::Component::Thread *, size_t>> Owners;
  std::optional<std::chrono::steady_clock::time_point> Earliest;
  for (const auto *Store : Roots) {
    for (Runtime::Component::Thread *Parked : Store->Waiting) {
      if (!Parked->isWaiting()) {
        continue;
      }
      const auto Interests = Parked->getInterests();
      for (size_t I = 0; I < Interests.size(); ++I) {
        Entries.push_back(Interests[I]);
        Owners.emplace_back(Parked, I);
      }
      if (const auto &Deadline = Parked->getDeadline(); Deadline.has_value()) {
        Earliest =
            Earliest.has_value() ? std::min(*Earliest, *Deadline) : *Deadline;
      }
    }
  }
  if (!Block) {
    if (!Entries.empty()) {
      Poll::wait(Entries, std::chrono::steady_clock::now());
    }
  } else if (Entries.empty() && !Earliest.has_value()) {
    return false;
  } else {
    Poll::wait(Entries, Earliest);
  }
  for (size_t I = 0; I < Entries.size(); ++I) {
    if (Entries[I].Ready) {
      Owners[I].first->onIoReady(Owners[I].second);
    }
  }
  return true;
}

// Start the spawned tasks. See "include/executor/component/executor.h".
void ComponentExecutor::startSpawnedTasks(Runtime::Component::Task &Spawner) {
  auto *Inst = Spawner.getInstance();
  for (auto &Spawn : Spawner.takeSpawns()) {
    // A spawner that was aborted has no instance to spawn in.
    if (Inst == nullptr) {
      continue;
    }
    auto *Store = Inst->getRoot();
    Runtime::Component::Task *T = Store->newTask(Inst, nullptr, true);
    Runtime::Component::Thread *Stack =
        newRootThread(*Store, *T, [this, T, Inst, Body = std::move(Spawn)]() {
          Runtime::Component::Thread *Cur = getCurrentThread(*Inst);
          assuming(Cur != nullptr);
          auto Res = runSpawnedTask(*T, *Cur, Body);
          if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
            setTrap(Res.error(), *Inst);
          }
        });
    Stack->onReady();
    Store->pushWaiting(*Stack);
  }
}

// Find the first eligible ready thread. See
// "include/executor/component/executor.h".
Runtime::Component::Thread *ComponentExecutor::findReadyThread(
    Span<Runtime::Instance::ComponentInstance *const> Roots,
    const std::function<bool(const Runtime::Component::Thread &)> &Eligible)
    const noexcept {
  for (const auto *Store : Roots) {
    for (Runtime::Component::Thread *Parked : Store->Waiting) {
      if (!Parked->isEnded() && Parked->isReady() && Eligible(*Parked)) {
        return Parked;
      }
    }
  }
  return nullptr;
}

} // namespace Executor
} // namespace WasmEdge
