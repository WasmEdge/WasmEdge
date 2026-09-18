// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

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
    Runtime::Component::Task *Caller) {
  Runtime::Component::Task *T =
      addTask(std::make_shared<Runtime::Component::Task>(
          *FuncInst, std::move(OnStart), std::move(OnResolve), Caller));
  // A synchronous guest callee shares the stack of its caller; an
  // asynchronous or host one gets a stack of its own.
  if (getCurrentThread() != nullptr && !T->isFuncTypeAsync() && !T->isHost()) {
    Runtime::Component::Thread *Nested = newNestedThread(*T);
    auto Res = runTask(*T, *Nested);
    Nested->onEnd();
    if (!Res) {
      setTrap(Res.error(), T->getInstance());
      return Unexpect(Res);
    }
    return T;
  }
  Runtime::Component::Thread *Root = newThread(*T, [this, T]() {
    Runtime::Component::Thread *Cur = getCurrentThread();
    assuming(Cur != nullptr);
    auto Res = T->isHost() ? runHostTask(*T, *Cur) : runTask(*T, *Cur);
    if (!Res) {
      setTrap(Res.error(), T->getInstance());
    }
  });
  resume(*Root, Runtime::Component::Thread::Reason::Normal);
  return T;
}

// Run the body of a guest task. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runTask(Runtime::Component::Task &T,
                                        Runtime::Component::Thread &Cur) {
  const Runtime::Component::Thread::Reason Entered = T.onEnter(Cur);
  if (Entered == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  if (Entered == Runtime::Component::Thread::Reason::Cancelled) {
    // Cancelled while waiting to enter: it resolves as cancelled unstarted.
    EXPECTED_TRY(T.onCancel());
    return T.onExit();
  }

  const auto &Opts = T.getCanonOptions();
  const auto *Inst = Opts.Inst;
  const auto *TypeInst = T.getTypeInstance();
  const auto *FuncType = T.getFuncType();
  // A non-async-typed export cannot be entered while it executes; one that
  // runs marks the instance entered until it leaves.
  const bool WasEntered = Inst->isEntered();
  auto Body = [&]() -> Expect<void> {
    if (!T.isFuncTypeAsync() && WasEntered) {
      spdlog::error(ErrCode::Value::ComponentCannotEnter);
      return Unexpect(ErrCode::Value::ComponentCannotEnter);
    }
    Inst->setEntered(!T.isFuncTypeAsync() || WasEntered);

    // The caller produces the arguments; they lower into the callee, which
    // may not leave the instance meanwhile.
    EXPECTED_TRY(auto Args, T.onStart());
    std::vector<ValVariant> Flat;
    Inst->setMayLeave(false);
    auto Lowered = lowerValues(Opts, *TypeInst, FuncType->getParamValTypes(),
                               Args, MaxFlatParams, Flat);
    Inst->setMayLeave(true);
    EXPECTED_TRY(Lowered);

    if (!Opts.Async) {
      // A synchronous lift: the core function returns the flat results, and
      // post-return frees what they point at.
      EXPECTED_TRY(auto FlatRets, invokeCore(T.getCoreFunction(), Flat));
      EXPECTED_TRY(auto Rets,
                   liftValues(Opts, *TypeInst, FuncType->getResultValTypes(),
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
    // An async callback lift: the core function and then the callback return
    // what to do next, until they ask to exit.
    EXPECTED_TRY(auto Packed, invokeCore(T.getCoreFunction(), Flat));
    return runCallbackLoop(T, Cur, Packed[0].get<uint32_t>());
  };
  auto Res = Body();
  Inst->setEntered(WasEntered);
  if (!Res) {
    // After an abort nothing of the instance may be touched; after a trap the
    // tree is poisoned and the activation never exits.
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
  const auto *Inst = Opts.Inst;
  while (true) {
    const uint32_t Kind = Code & 0xFU;
    Event Pending;
    if (Kind ==
        static_cast<uint32_t>(Runtime::Component::Task::CallbackCode::Exit)) {
      break;
    }
    // The callback loop holds no guest frames: it hands the exclusive slot
    // back while it parks.
    const bool Held = T.hasExclusive();
    if (Held) {
      T.releaseExclusive();
    }
    if (Kind ==
        static_cast<uint32_t>(Runtime::Component::Task::CallbackCode::Yield)) {
      const Runtime::Component::Thread::Reason Wake = T.yield(Cur, true);
      if (Wake == Runtime::Component::Thread::Reason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      if (Wake == Runtime::Component::Thread::Reason::Cancelled) {
        Pending.Code = EventCode::TaskCancelled;
      }
    } else if (Kind == static_cast<uint32_t>(
                           Runtime::Component::Task::CallbackCode::Wait)) {
      EXPECTED_TRY(Pending, waitOnSet(T, Cur, *Inst, Code >> 4, true));
    } else {
      spdlog::error(ErrCode::Value::ComponentBadCallbackCode);
      return Unexpect(ErrCode::Value::ComponentBadCallbackCode);
    }
    if (Held) {
      // The slot comes back in place when nobody took it meanwhile.
      const Runtime::Component::Thread::Reason Wake =
          T.collect(Cur, [&T]() { return T.isExclusiveFree(); }, false);
      if (Wake == Runtime::Component::Thread::Reason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      T.takeExclusive();
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
  const Runtime::Component::Thread::Reason Entered = T.onEnter(Cur);
  if (Entered == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  EXPECTED_TRY(auto Args, T.onStart());
  std::vector<ComponentValVariant> Rets(T.getFuncType()->getResultArity());
  Runtime::Component::CallingFrame Frame(T, Cur, *this);
  EXPECTED_TRY(T.getHostFunction()->run(Frame, Args, Rets));
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
  const Runtime::Component::Thread::Reason Entered = T.onEnter(Cur);
  if (Entered == Runtime::Component::Thread::Reason::Abort) {
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
    const Runtime::Instance::ComponentInstance *Impl,
    Runtime::Instance::FunctionInstance *Dtor, uint64_t Rep) {
  return runImplicitTask(Impl, [this, Dtor, Rep]() -> Expect<void> {
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
Expect<void> ComponentExecutor::runImplicitTask(
    const Runtime::Instance::ComponentInstance *Inst,
    const std::function<Expect<void>()> &Body) {
  Runtime::Component::Task *T =
      addTask(std::make_shared<Runtime::Component::Task>(Inst, getCurrentTask(),
                                                         false));
  if (getCurrentThread() != nullptr) {
    // Nested in the current stack.
    Runtime::Component::Thread *Nested = newNestedThread(*T);
    const Runtime::Component::Thread::Reason Entered = T->onEnter(*Nested);
    if (Entered == Runtime::Component::Thread::Reason::Abort) {
      Nested->onEnd();
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    auto Res = Body();
    if (Res) {
      static_cast<void>(T->onReturn({}));
      Res = T->onExit();
    }
    Nested->onEnd();
    if (!Res) {
      setTrap(Res.error(), Inst);
    }
    return Res;
  }
  // From the embedder thread: a stack of its own, pumped until it finishes,
  // since the body may suspend.
  Runtime::Component::Thread *Root = newThread(*T, [this, T, &Body]() {
    Runtime::Component::Thread *Cur = getCurrentThread();
    assuming(Cur != nullptr);
    if (T->onEnter(*Cur) == Runtime::Component::Thread::Reason::Abort) {
      return;
    }
    auto Res = Body();
    if (Res) {
      static_cast<void>(T->onReturn({}));
      Res = T->onExit();
    }
    if (!Res) {
      setTrap(Res.error(), T->getInstance());
    }
  });
  resume(*Root, Runtime::Component::Thread::Reason::Normal);
  return pump([Root]() { return Root->isEnded(); }, Root);
}

// Wait on a waitable set. See "include/executor/component/executor.h".
Expect<ComponentExecutor::Event>
ComponentExecutor::waitOnSet(Runtime::Component::Task &T,
                             Runtime::Component::Thread &Cur,
                             const Runtime::Instance::ComponentInstance &Inst,
                             uint32_t SetIdx, bool Cancellable) {
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
  if (!Ready()) {
    EXPECTED_TRY(checkMayBlock(T, Cur));
  }
  // An event already pending completes the wait in place; the thread names
  // the set it waits on, which keeps the set alive meanwhile.
  Cur.setWaitingSet(SetIdx);
  const Runtime::Component::Thread::Reason Wake =
      T.collect(Cur, Ready, Cancellable);
  if (Wake == Runtime::Component::Thread::Reason::Abort) {
    return Unexpect(ErrCode::Value::ComponentAsyncAborted);
  }
  Cur.setWaitingSet(0);
  if (Wake == Runtime::Component::Thread::Reason::Cancelled) {
    return Event{EventCode::TaskCancelled, 0, 0};
  }
  if (const auto *Members = Inst.findWaitableSet(SetIdx); Members != nullptr) {
    for (const uint32_t Idx : *Members) {
      if (hasPendingEvent(Inst, Idx)) {
        return takePendingEvent(Inst, Idx);
      }
    }
  }
  return Event{};
}

// Check whether a task may block. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::checkMayBlock(Runtime::Component::Task &T,
                                              Runtime::Component::Thread &Cur) {
  if (T.mayBlock(Cur, Waiting)) {
    return {};
  }
  spdlog::error(ErrCode::Value::ComponentCannotBlockSync);
  return Unexpect(ErrCode::Value::ComponentCannotBlockSync);
}

// Hand the stack to a thread. See "include/executor/component/executor.h".
void ComponentExecutor::resume(Runtime::Component::Thread &Target,
                               Runtime::Component::Thread::Reason Wake) {
  Runtime::Component::Thread *Next = &Target;
  Runtime::Component::Thread::Reason Woke = Wake;
  while (Next != nullptr && !Next->isEnded()) {
    Waiting.erase(std::remove(Waiting.begin(), Waiting.end(), Next),
                  Waiting.end());
    Runtime::Component::Thread *Prev = Current;
    Current = Next;
    Runtime::Component::Thread *Named = Next->onResume(Woke);
    Current = Prev;
    // The thread that parked joins the waiting list when it can be picked
    // again; the detached tasks the activation asked for start as well.
    Runtime::Component::Thread *Parked = Next->getInnermost();
    if (Parked != nullptr && !Parked->isEnded() && Parked->isWaiting() &&
        std::find(Waiting.begin(), Waiting.end(), Parked) == Waiting.end()) {
      Waiting.push_back(Parked);
    }
    startSpawns(Next->getOwner());
    if (Parked != nullptr && &Parked->getOwner() != &Next->getOwner()) {
      startSpawns(Parked->getOwner());
    }
    Next = Named;
    Woke = Runtime::Component::Thread::Reason::Normal;
  }
}

// Run the scheduler loop. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::pump(const std::function<bool()> &Done,
                                     Runtime::Component::Thread *Pin) {
  auto Eligible = [Pin](const Runtime::Component::Thread &Ready) {
    // While a non-async-typed call is in progress on the pinned stack, only
    // a thread the innermost such call may resume takes the stack.
    if (Pin == nullptr || Pin->isEnded()) {
      return true;
    }
    Runtime::Component::Thread *Inner = Pin->getInnermost();
    if (Inner == nullptr || Inner->isEnded()) {
      return true;
    }
    const Runtime::Component::Task &PinTask = Inner->getOwner();
    if (PinTask.isFuncTypeAsync() || PinTask.isHost()) {
      return true;
    }
    return PinTask.mayResume(Ready);
  };
  while (!Done()) {
    if (Trap.has_value()) {
      return Unexpect(*Trap);
    }
    if (Runtime::Component::Thread *Next = findReadyThread(Eligible);
        Next != nullptr) {
      resume(*Next, Runtime::Component::Thread::Reason::Normal);
      continue;
    }
    if (Done()) {
      break;
    }
    // Nothing is ready: sleep until the earliest deadline of a parked thread
    // or until a host operation on another OS thread wakes the scheduler.
    std::optional<std::chrono::steady_clock::time_point> Earliest;
    bool External = false;
    for (const Runtime::Component::Thread *Parked : Waiting) {
      if (Parked->isEnded()) {
        continue;
      }
      External = External || Parked->isExternal();
      if (const auto &Deadline = Parked->getDeadline(); Deadline.has_value()) {
        Earliest =
            Earliest.has_value() ? std::min(*Earliest, *Deadline) : *Deadline;
      }
    }
    if (Earliest.has_value()) {
      static_cast<void>(WakeUp->acquireUntil(*Earliest));
      continue;
    }
    if (External) {
      WakeUp->acquire();
      continue;
    }
    spdlog::error(ErrCode::Value::ComponentAsyncDeadlock);
    setTrap(ErrCode(ErrCode::Value::ComponentAsyncDeadlock),
            Pin != nullptr ? Pin->getOwner().getInstance() : nullptr);
    return Unexpect(ErrCode::Value::ComponentAsyncDeadlock);
  }
  if (Trap.has_value()) {
    return Unexpect(*Trap);
  }
  return {};
}

// Start the spawned tasks. See "include/executor/component/executor.h".
void ComponentExecutor::startSpawns(Runtime::Component::Task &Spawner) {
  for (auto &Body : Spawner.takeSpawns()) {
    Runtime::Component::Task *T =
        addTask(std::make_shared<Runtime::Component::Task>(
            Spawner.getInstance(), nullptr, true));
    Runtime::Component::Thread *Root =
        newThread(*T, [this, T, Body = std::move(Body)]() {
          Runtime::Component::Thread *Cur = getCurrentThread();
          assuming(Cur != nullptr);
          auto Res = runSpawnedTask(*T, *Cur, Body);
          if (!Res) {
            setTrap(Res.error(), T->getInstance());
          }
        });
    Root->onReady();
    Waiting.push_back(Root);
  }
}

// Find the first eligible ready thread. See
// "include/executor/component/executor.h".
Runtime::Component::Thread *ComponentExecutor::findReadyThread(
    const std::function<bool(const Runtime::Component::Thread &)> &Eligible)
    const noexcept {
  for (Runtime::Component::Thread *Parked : Waiting) {
    if (!Parked->isEnded() && Parked->isReady() && Eligible(*Parked)) {
      return Parked;
    }
  }
  return nullptr;
}

} // namespace Executor
} // namespace WasmEdge
