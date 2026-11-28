// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "system/poll.h"

#include "common/spdlog.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
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
  // A synchronous callee shares the stack of its caller; others get one.
  if (CallerThread != nullptr && !T->isFuncTypeAsync()) {
    Runtime::Component::Thread *Nested = Store->newThread(*T, *CallerThread);
    auto Res =
        T->isHost() ? runHostTask(*T, *Nested) : runGuestTask(*T, *Nested);
    Nested->onEnd();
    // After an abort the store of the callee may be gone.
    if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
      return Unexpect(Res);
    }
    startSpawnedTasks(*T);
    if (!Res) {
      setTrap(Res.error(), *Inst);
      return Unexpect(Res);
    }
    return T;
  }
  Runtime::Component::Thread *Stack = newRootThread(
      *Store, *T, [this, T, Inst](Runtime::Component::Thread &Cur) {
        auto Res = T->isHost() ? runHostTask(*T, Cur) : runGuestTask(*T, Cur);
        if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
          setTrap(Res.error(), *Inst);
        }
      });
  if (Stack == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    the host cannot start another thread"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  resumeThread(*Stack);
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
    T.onExit();
    return {};
  }

  const auto &Opts = T.getCanonOptions();
  auto *Inst = Opts.Inst;
  const auto *TypeInst = T.getTypeInstance();
  const auto *FuncType = T.getFuncType();
  auto Body = [&]() -> Expect<void> {
    EXPECTED_TRY(auto Args, T.onStart());
    std::vector<ValVariant> Flat;
    EXPECTED_TRY(lowerFlatValues(Opts, *TypeInst, FuncType->getParamValTypes(),
                                 Args, MaxFlatParams, Flat));

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
  // After an abort the instance may be gone, and after a trap its tree is
  // poisoned: the activation exits only when its body returned.
  EXPECTED_TRY(Body());
  T.onExit();
  return {};
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
  // A host task waits at no gate.
  T.onEnter(Cur);
  EXPECTED_TRY(auto Args, T.onStart());
  // The ends the arguments carry enter the host table of the root of the host,
  // and the ends the results name leave it: the body only sees handles.
  auto &HostRoot = *T.getInstance()->getRoot();
  for (auto &Arg : Args) {
    EXPECTED_TRY(lowerHostValue(HostRoot, Arg));
  }
  std::vector<ComponentValVariant> Rets(T.getFuncType()->getResultArity());
  Runtime::Component::CallingFrame Frame(T, Cur, *this);
  EXPECTED_TRY(T.getHostFunction()->run(Frame, Args, Rets));
  const auto RetTypes = T.getFuncType()->getResultValTypes();
  for (size_t I = 0; I < Rets.size(); ++I) {
    EXPECTED_TRY(checkHostValue(*T.getTypeInstance(), RetTypes[I], Rets[I]));
    EXPECTED_TRY(liftHostValue(HostRoot, Rets[I]));
  }
  if (!T.isResolved()) {
    EXPECTED_TRY(T.onReturn(std::move(Rets)));
  }
  T.onExit();
  return {};
}

// Run the body of a spawned host task. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runSpawnedTask(
    Runtime::Component::Task &T, Runtime::Component::Thread &Cur,
    const Runtime::Component::Task::SpawnBody &Body) {
  Runtime::Component::CallingFrame Frame(T, Cur, *this);
  auto Res = Body(Frame);
  if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
    return Res;
  }
  if (!T.isResolved()) {
    static_cast<void>(T.onReturn({}));
  }
  EXPECTED_TRY(Res);
  T.onExit();
  return {};
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
    T->onEnter(*Nested);
    auto Res = Body();
    if (Res) {
      static_cast<void>(T->onReturn({}));
      T->onExit();
    }
    Nested->onEnd();
    // After an abort the store may be gone.
    if (!Res && Res.error() == ErrCode::Value::ComponentAsyncAborted) {
      return Res;
    }
    if (!Res) {
      setTrap(Res.error(), *Inst);
      return Res;
    }
    Store->drainEndedTask(*T);
    return Res;
  }
  // From the embedder thread: a stack of its own, pumped until it finishes.
  Runtime::Component::Thread *Stack = newRootThread(
      *Store, *T, [this, T, Inst, &Body](Runtime::Component::Thread &Thread) {
        T->onEnter(Thread);
        auto Res = Body();
        if (Res) {
          static_cast<void>(T->onReturn({}));
          T->onExit();
        }
        if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
          setTrap(Res.error(), *Inst);
        }
      });
  if (Stack == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    the host cannot start another thread"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  resumeThread(*Stack);
  auto Res =
      pump(*Store, [Stack]() { return Stack->isEnded(); }, Stack, nullptr);
  // A trap leaves the stack parked on the frames of this call: the trapped
  // stores unwind it before the body goes.
  if (!Stack->isEnded()) {
    for (auto *Trapped : Store->getStoreRoots()) {
      if (Trapped->Trap.has_value()) {
        Trapped->abortTasks(Trapped->Trap->getEnum() !=
                            ErrCode::Value::Terminated);
      }
    }
  }
  return Res;
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
    return Inst.findWaitableSet(SetIdx) == nullptr ||
           findPendingWaitableIdx(Inst, SetIdx) != 0;
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
  if (const uint32_t Idx = findPendingWaitableIdx(Inst, SetIdx); Idx != 0) {
    return takePendingEvent(Inst, Idx);
  }
  return Component::WaitableEvent{};
}

// Hand the stack to a thread. See "include/executor/component/executor.h".
void ComponentExecutor::resumeThread(Runtime::Component::Thread &Target) {
  Runtime::Component::Thread *Next = &Target;
  while (Next != nullptr && !Next->isEnded()) {
    auto *Store = Next->getOwner().getInstance()->getRoot();
    auto *Entry = Store->getEntryRoot();
    Store->Waiting.erase(
        std::remove(Store->Waiting.begin(), Store->Waiting.end(), Next),
        Store->Waiting.end());
    // A thread that nests may be gone once its stack hands back: the turn is
    // followed by the root thread of the stack.
    Runtime::Component::Thread &Running = Next->getRoot();
    Runtime::Component::Thread *Prev = std::exchange(Entry->Current, &Running);
    Next = Next->onResume(Runtime::Component::Thread::WakeReason::Normal);
    Entry->Current = Prev;
    // The parked thread joins the waiting list of its store when it can be
    // picked again, and the tasks the activation spawned start.
    Runtime::Component::Thread *Parked = Running.getInnermost();
    if (Parked != nullptr && Parked->isWaiting() &&
        Parked->getOwner().getInstance() != nullptr) {
      Parked->getOwner().getInstance()->getRoot()->pushWaiting(*Parked);
    }
    for (Runtime::Component::Thread *Sharing : Running.getStackThreads()) {
      startSpawnedTasks(Sharing->getOwner());
    }
    // The non-async-typed guest call the stack parks in takes the turn over,
    // unless a loop already runs: its loop resumes the named thread first.
    while (!Running.isPinned()) {
      Runtime::Component::Thread *Inner = Running.getInnermost();
      if (Inner == nullptr || !(Inner->isWaiting() || Inner->isSuspended())) {
        break;
      }
      // A host task the call nests parks the stack for it.
      Runtime::Component::Thread *Guest = getGuestThread(Running);
      if (Guest == nullptr) {
        break;
      }
      Runtime::Component::Task &Call = Guest->getOwner();
      if (Call.isFuncTypeAsync() || Call.getInstance() == nullptr ||
          Call.isResolved() || Call.isAborted()) {
        break;
      }
      // The loop holds the call, which goes with it when nothing else does. A
      // trap latches on the store, where the caller finds it.
      auto *CallStore = Call.getInstance()->getRoot();
      Call.addDependent();
      auto Res = pump(
          *CallStore,
          [Guest, &Call]() {
            return Guest->isEnded() || Call.isResolved() || Call.isAborted();
          },
          &Running, std::exchange(Next, nullptr));
      Call.releaseDependent();
      if (!Res) {
        return;
      }
      if (!Guest->isRoot()) {
        CallStore->drainEndedTask(Call);
      }
    }
  }
}

// Run the scheduler loop. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::pump(Runtime::Instance::ComponentInstance &Root,
                                     const std::function<bool()> &Done,
                                     Runtime::Component::Thread *Pin,
                                     Runtime::Component::Thread *First) {
  // The non-async-typed guest call in progress on the pinned stack, if any.
  auto SyncCall = [this, Pin]() -> const Runtime::Component::Task * {
    if (Pin == nullptr || Pin->isEnded()) {
      return nullptr;
    }
    const Runtime::Component::Thread *Inner = getGuestThread(*Pin);
    if (Inner == nullptr || Inner->getOwner().isFuncTypeAsync()) {
      return nullptr;
    }
    return &Inner->getOwner();
  };
  // While such a call is in progress, only a thread it may resume takes the
  // stack; a host task takes its turn as the guest call that nests it.
  auto Eligible = [this, &SyncCall](const Runtime::Component::Thread &Ready) {
    const Runtime::Component::Task *Call = SyncCall();
    if (Call == nullptr) {
      return true;
    }
    const Runtime::Component::Thread *Guest = getGuestThread(Ready);
    return Call->mayResume(Guest != nullptr ? *Guest : Ready);
  };
  // The progress of this store needs the host tasks of the stores it imports
  // from.
  const auto Roots = Root.getStoreRoots();
  if (Pin != nullptr) {
    Pin->setPinned(true);
  }
  // The thread a switch named runs before any other.
  if (First != nullptr) {
    resumeThread(*First);
  }
  Expect<void> Res;
  while (!Done()) {
    if (const auto Trap = getTrap(Roots); Trap.has_value()) {
      Res = Unexpect(*Trap);
      break;
    }
    pollWaiting(Roots, Eligible, false);
    if (Runtime::Component::Thread *Next = findReadyThread(Roots, Eligible);
        Next != nullptr) {
      resumeThread(*Next);
      continue;
    }
    // Nothing is ready: wait on the OS for a handle or the earliest sleep.
    if (pollWaiting(Roots, Eligible, true)) {
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
    Span<Runtime::Instance::ComponentInstance *const> Roots,
    const std::function<bool(const Runtime::Component::Thread &)> &Eligible,
    bool Block) {
  std::vector<PollEntry> Entries;
  std::vector<std::pair<Runtime::Component::Thread *, size_t>> Owners;
  std::optional<std::chrono::steady_clock::time_point> Earliest;
  for (const auto *Store : Roots) {
    for (Runtime::Component::Thread *Parked : Store->Waiting) {
      // A thread that may not take the stack is not waited for either.
      if (!Parked->isWaiting() || !isStackEntered(*Store, *Parked) ||
          !Eligible(*Parked)) {
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
    Runtime::Component::Thread *Stack = newRootThread(
        *Store, *T,
        [this, T, Inst,
         Body = std::move(Spawn)](Runtime::Component::Thread &Cur) {
          auto Res = runSpawnedTask(*T, Cur, Body);
          if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
            setTrap(Res.error(), *Inst);
          }
        });
    // A task the host has no thread for traps the store of its spawner.
    if (Stack == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTrap);
      spdlog::error("    the host cannot start another thread"sv);
      setTrap(ErrCode::Value::ComponentTrap, *Inst);
      break;
    }
    // The task enters now: an abort before its first turn finds the stack.
    T->onEnter(*Stack);
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
      if (!Parked->isEnded() && isStackEntered(*Store, *Parked) &&
          Parked->isReady() && Eligible(*Parked)) {
        return Parked;
      }
    }
  }
  return nullptr;
}

} // namespace Executor
} // namespace WasmEdge
