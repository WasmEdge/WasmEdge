// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/task.h - Task definition ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the task of the component model: one export activation,
/// which produces a result, owns an implicit thread, and rules the suspension
/// points of every thread running for it.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/spdlog.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/thread.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class CallingFrame;

/// One export activation: it produces a result, owns an implicit thread, and
/// rules the suspension points of every thread running for it.
class Task {
public:
  /// What the callback of a callback-lifted activation asks for next; the
  /// low 4 bits of its packed result.
  enum class CallbackCode : uint32_t {
    Exit = 0,
    Yield = 1,
    Wait = 2,
  };

  /// Produce the component-level arguments (caller side).
  using OnStartCallback =
      std::function<Expect<std::vector<ComponentValVariant>>()>;
  /// Consume the component-level results. A nullopt means cancelled.
  using OnResolveCallback = std::function<Expect<void>(
      std::optional<std::vector<ComponentValVariant>>)>;
  /// The body of a detached host activation the host asks for.
  using SpawnBody = std::function<Expect<void>(CallingFrame &)>;

  /// The activation of a `canon lift` of Func, called from Caller. A host
  /// function runs as the async-stackful lift of the host.
  Task(const Instance::ComponentFunctionInstance &Func,
       OnStartCallback StartFunc, OnResolveCallback ResolveFunc,
       Task *Caller) noexcept
      : FuncType(&Func.getFuncType()), TypeInst(Func.getTypeInstance()),
        FuncTypeAsync(FuncType->isAsync()), CoreFunc(Func.getLowerFunction()),
        HostFunc(Func.isHostFunction() ? &Func.getHostFunc() : nullptr),
        Opts(Func.getCanonOptions()), OnStart(std::move(StartFunc)),
        OnResolve(std::move(ResolveFunc)), CallerTask(Caller) {
    if (HostFunc != nullptr) {
      Opts.Async = true;
    }
  }
  /// An implicit synchronous activation of Inst, or a detached host
  /// activation. Neither has results.
  Task(const Instance::ComponentInstance *Inst, Task *Caller,
       bool IsDetached = false) noexcept
      : FuncTypeAsync(IsDetached), Detached(IsDetached), CallerTask(Caller),
        Status(State::Started), HasStarted(true) {
    Opts.Inst = Inst;
    Opts.Async = IsDetached;
  }
  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  /// \name The shape of the activation.
  /// @{
  const AST::Component::FuncType *getFuncType() const noexcept {
    return FuncType;
  }
  /// The instance whose type-index space the function type reads against.
  const Instance::ComponentInstance *getTypeInstance() const noexcept {
    return TypeInst;
  }
  bool isFuncTypeAsync() const noexcept { return FuncTypeAsync; }
  Instance::FunctionInstance *getCoreFunction() const noexcept {
    return CoreFunc;
  }
  HostFunctionBase *getHostFunction() const noexcept { return HostFunc; }
  bool isDetached() const noexcept { return Detached; }
  const CanonOptions &getCanonOptions() const noexcept { return Opts; }
  const Instance::ComponentInstance *getInstance() const noexcept {
    return Opts.Inst;
  }
  Task *getCaller() const noexcept { return CallerTask; }
  /// @}

  /// \name The queries on the activation.
  /// @{
  /// Host tasks run no guest code: resuming one is never reentrance.
  bool isHost() const noexcept { return HostFunc != nullptr || isDetached(); }
  bool isResolved() const noexcept { return Status == State::Resolved; }
  /// The thread the activation runs on; null before it enters.
  Thread *getImplicitThread() const noexcept { return ImplicitThread; }
  /// The subtask state code its caller sees: starting (0), started (1),
  /// returned (2), cancelled before (3) or after (4) the start.
  uint32_t getSubtaskCode() const noexcept {
    if (Status == State::Resolved) {
      if (!ResolvedByCancel) {
        return 2;
      }
      return HasStarted ? 4 : 3;
    }
    return HasStarted ? 1 : 0;
  }
  Span<Thread *const> getThreads() const noexcept { return Threads; }
  bool needsExclusive() const noexcept {
    // Exclusivity is only meaningful for async-typed functions.
    return FuncTypeAsync && (!Opts.Async || Opts.Callback != nullptr);
  }
  /// Whether blocking is unconditionally allowed. A synchronous, unresolved
  /// task may still block when another thread can run: mayBlock decides.
  bool mayBlockAlways() const noexcept {
    return FuncTypeAsync || Status == State::Resolved;
  }
  /// Whether Cur may block here: another thread of the instance tree, among
  /// the waiting ones, is ready to take the stack in its place.
  bool mayBlock(const Thread &Cur, Span<Thread *const> Waiting) const noexcept {
    if (mayBlockAlways()) {
      return true;
    }
    const auto *Root = Opts.Inst != nullptr ? Opts.Inst->getRoot() : nullptr;
    for (const Thread *Entry : Waiting) {
      if (Entry == &Cur || !Entry->isReady()) {
        continue;
      }
      const auto *Inst = Entry->getOwner().Opts.Inst;
      if (Inst == nullptr || Inst->getRoot() != Root) {
        continue;
      }
      return true;
    }
    return false;
  }
  /// Whether Target may take the stack while this non-async-typed call runs.
  bool mayResume(const Thread &Target) const noexcept {
    const auto &Owner = Target.getOwner();
    // A host task runs no guest code, so resuming it is never reentrance.
    if (Owner.isHost()) {
      return true;
    }
    // Resuming a thread of another instance would be unexpected reentrance.
    if (Owner.Opts.Inst != Opts.Inst) {
      return false;
    }
    // The implicit thread of a task that needs the stack exclusively cannot
    // run under this call, ready or not.
    return &Target != Owner.ImplicitThread || !Owner.needsExclusive();
  }
  /// Whether this activation or one it was called from runs in Root.
  bool isUnderRoot(const Instance::ComponentInstance *Root) const noexcept {
    for (const Task *T = this; T != nullptr; T = T->CallerTask) {
      if (T->Opts.Inst != nullptr && T->Opts.Inst->getRoot() == Root) {
        return true;
      }
    }
    return false;
  }
  /// @}

  /// \name Subtask handle state.
  /// @{
  uint32_t getDeliveredCode() const noexcept { return DeliveredCode; }
  void setDeliveredCode(uint32_t Code) noexcept { DeliveredCode = Code; }
  bool isDelivered() const noexcept { return Delivered; }
  void setDelivered() noexcept { Delivered = true; }
  uint32_t getSetIdx() const noexcept { return SetIdx; }
  void setSetIdx(uint32_t Idx) noexcept { SetIdx = Idx; }
  bool isSyncWaiter() const noexcept { return SyncWaiter; }
  void setSyncWaiter(bool IsWaiter) noexcept { SyncWaiter = IsWaiter; }
  bool isCancelRequested() const noexcept { return CancelRequested; }
  /// @}

  /// \name Requests to the scheduler.
  /// @{
  void postSpawn(SpawnBody Body) noexcept { Spawns.push_back(std::move(Body)); }
  std::vector<SpawnBody> takeSpawns() noexcept {
    return std::exchange(Spawns, {});
  }
  const std::shared_ptr<Thread::Signal> &getWakeUp() const noexcept {
    return WakeUp;
  }
  void setWakeUp(std::shared_ptr<Thread::Signal> WakeSignal) noexcept {
    WakeUp = std::move(WakeSignal);
  }
  /// @}

  /// \name Borrows and lenders.
  /// @{
  void addBorrow() noexcept { NumBorrows += 1; }
  void dropBorrow() noexcept {
    if (NumBorrows > 0) {
      NumBorrows -= 1;
    }
  }
  /// A handle of the caller lent for the call: it cannot be dropped until
  /// the resolution is delivered.
  void addLender(Instance::ComponentInstance::ResourceHandle *Handle) noexcept {
    Handle->NumLends += 1;
    Lenders.push_back(Handle);
  }
  /// The resolution was delivered: the lends end.
  void releaseLenders() noexcept {
    for (auto *Handle : Lenders) {
      if (Handle->NumLends > 0) {
        Handle->NumLends -= 1;
      }
    }
    Lenders.clear();
  }
  /// @}

  /// \name The exclusive slot of the instance, held by the implicit thread.
  /// @{
  bool isExclusiveFree() const noexcept {
    return Opts.Inst == nullptr || Opts.Inst->getExclusiveThread() == nullptr;
  }
  bool hasExclusive() const noexcept {
    return Opts.Inst != nullptr &&
           Opts.Inst->getExclusiveThread() == ImplicitThread;
  }
  void takeExclusive() noexcept {
    if (Opts.Inst != nullptr) {
      Opts.Inst->setExclusiveThread(ImplicitThread);
    }
  }
  void releaseExclusive() noexcept {
    if (Opts.Inst != nullptr) {
      Opts.Inst->setExclusiveThread(nullptr);
    }
  }
  /// @}

  /// \name The transitions of the activation.
  /// @{
  /// Enter the implicit thread: gate a guest activation on backpressure and
  /// the exclusive slot, then register the thread it runs on.
  Thread::Reason onEnter(Thread &Cur) noexcept {
    ImplicitThread = &Cur;
    const auto *Inst = Opts.Inst;
    // The host runs no guest code, so no gate holds it.
    if (FuncTypeAsync && !isHost()) {
      auto HasBackpressure = [this, Inst]() {
        return Inst->getBackpressure() > 0 ||
               (needsExclusive() && Inst->getExclusiveThread() != nullptr);
      };
      if (HasBackpressure() || Inst->getNumWaitingToEnter() > 0) {
        Inst->incWaitingToEnter();
        const Thread::Reason Wake = wait(
            Cur, [HasBackpressure]() { return !HasBackpressure(); },
            /*Cancellable=*/true);
        // A teardown wake-up must not touch possibly-gone instance state.
        if (Wake == Thread::Reason::Abort) {
          return Wake;
        }
        Inst->decWaitingToEnter();
        if (Wake == Thread::Reason::Cancelled) {
          return Wake;
        }
      }
      if (needsExclusive()) {
        takeExclusive();
      }
    }
    addThread(Cur);
    return Thread::Reason::Normal;
  }

  /// Produce the component-level arguments of the activation.
  Expect<std::vector<ComponentValVariant>> onStart() noexcept {
    assuming(Status == State::Initial);
    Status = State::Started;
    HasStarted = true;
    if (!OnStart) {
      return std::vector<ComponentValVariant>{};
    }
    return OnStart();
  }

  /// `task.return`: resolve with the component-level results.
  Expect<void> onReturn(std::vector<ComponentValVariant> Results) noexcept {
    if (Status == State::Resolved) {
      spdlog::error(ErrCode::Value::ComponentTaskResolvedTwice);
      return Unexpect(ErrCode::Value::ComponentTaskResolvedTwice);
    }
    if (NumBorrows > 0) {
      spdlog::error(ErrCode::Value::ComponentBorrowsRemain);
      return Unexpect(ErrCode::Value::ComponentBorrowsRemain);
    }
    if (OnResolve) {
      EXPECTED_TRY(OnResolve(std::move(Results)));
    }
    Status = State::Resolved;
    return {};
  }

  /// `task.cancel`: resolve as cancelled.
  Expect<void> onCancel() noexcept {
    if (Status != State::CancelDelivered) {
      spdlog::error(ErrCode::Value::ComponentTaskNotCancelled);
      return Unexpect(ErrCode::Value::ComponentTaskNotCancelled);
    }
    if (NumBorrows > 0) {
      spdlog::error(ErrCode::Value::ComponentBorrowsRemain);
      return Unexpect(ErrCode::Value::ComponentBorrowsRemain);
    }
    if (OnResolve) {
      EXPECTED_TRY(OnResolve(std::nullopt));
    }
    Status = State::Resolved;
    ResolvedByCancel = true;
    return {};
  }

  /// Ask the activation to stop: the thread the executor is to wake as
  /// cancelled, or null when the cancellation queues.
  Thread *onCancelRequest() noexcept {
    CancelRequested = true;
    // The activation has not started: the gated thread wakes as cancelled.
    if (Status == State::Initial) {
      Status = State::CancelDelivered;
      if (ImplicitThread != nullptr && !ImplicitThread->isRunning()) {
        return ImplicitThread;
      }
      return nullptr;
    }
    if (Status != State::Started) {
      return nullptr;
    }
    for (Thread *Entry : Threads) {
      if (!Entry->isCancellable()) {
        continue;
      }
      // The implicit thread takes no cancellation while another thread holds
      // the stack exclusively.
      if (needsExclusive() && Entry == ImplicitThread && !isExclusiveFree() &&
          !hasExclusive()) {
        continue;
      }
      Status = State::CancelDelivered;
      return Entry;
    }
    Status = State::PendingCancel;
    return nullptr;
  }

  /// Deliver a queued cancellation at a cancellable suspension point.
  bool onCancelDelivery(bool Cancellable) noexcept {
    if (Cancellable && Status == State::PendingCancel) {
      Status = State::CancelDelivered;
      return true;
    }
    return false;
  }

  /// Leave the implicit thread and hand the exclusive slot back.
  Expect<void> onExit() noexcept {
    if (ImplicitThread != nullptr) {
      EXPECTED_TRY(removeThread(*ImplicitThread));
    }
    if (FuncTypeAsync && needsExclusive() && hasExclusive()) {
      releaseExclusive();
    }
    return {};
  }
  /// @}

  /// \name Suspension points.
  /// @{
  /// Park Cur until Ready() holds.
  Thread::Reason wait(Thread &Cur, std::function<bool()> Ready,
                      bool Cancellable) noexcept {
    return park(Cur, std::move(Ready), Cancellable, /*InPlace=*/false);
  }

  /// Wait for an event that may already be pending: when it is, the wait
  /// completes without ceding the stack.
  Thread::Reason collect(Thread &Cur, std::function<bool()> Ready,
                         bool Cancellable) noexcept {
    return park(Cur, std::move(Ready), Cancellable, /*InPlace=*/true);
  }

  /// Park Cur as ready; it always cedes the stack.
  Thread::Reason yield(Thread &Cur, bool Cancellable) noexcept {
    return wait(Cur, []() { return true; }, Cancellable);
  }

  /// Park Cur with no readiness predicate.
  Thread::Reason suspend(Thread &Cur, bool Cancellable) noexcept {
    if (onCancelDelivery(Cancellable)) {
      return Thread::Reason::Cancelled;
    }
    return Cur.onSuspend(Cancellable);
  }

  /// Park Cur and name Other as the thread to run next.
  Thread::Reason switchTo(Thread &Cur, Thread &Other, Thread::Next NextState,
                          bool Cancellable) noexcept {
    if (onCancelDelivery(Cancellable)) {
      return Thread::Reason::Cancelled;
    }
    return Cur.onSwitch(Other, NextState, Cancellable);
  }

  /// Hand over to Other only if it is ready, else park Cur as NextState says.
  Thread::Reason promote(Thread &Cur, Thread &Other, Thread::Next NextState,
                         bool Cancellable) noexcept {
    if (onCancelDelivery(Cancellable)) {
      return Thread::Reason::Cancelled;
    }
    return Cur.onPromote(Other, NextState, Cancellable);
  }
  /// @}

  /// \name The threads of the activation.
  /// @{
  /// Register Entry for this activation and in the instance thread table.
  void addThread(Thread &Entry) noexcept {
    Threads.push_back(&Entry);
    if (Opts.Inst != nullptr) {
      Entry.setIndex(Opts.Inst->addThread(&Entry));
    }
  }
  /// Unregister Entry; the last thread to leave owes a result.
  Expect<void> removeThread(Thread &Entry) noexcept {
    Threads.erase(std::remove(Threads.begin(), Threads.end(), &Entry),
                  Threads.end());
    if (Entry.getIndex() != 0 && Opts.Inst != nullptr) {
      Opts.Inst->removeThread(Entry.getIndex());
    }
    Entry.setIndex(0);
    if (Threads.empty() && Status != State::Resolved) {
      spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
      return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
    }
    return {};
  }
  /// @}

private:
  /// The resolution and cancellation state of the activation.
  enum class State : uint8_t {
    Initial,
    Started,
    PendingCancel,
    CancelDelivered,
    Resolved,
  };

  /// The one suspension point of a wait, in place or parked.
  Thread::Reason park(Thread &Cur, std::function<bool()> Ready,
                      bool Cancellable, bool InPlace) noexcept {
    if (onCancelDelivery(Cancellable)) {
      return Thread::Reason::Cancelled;
    }
    // A resolved activation waiting on its implicit thread hands the
    // exclusive slot back for the wait.
    const bool Release = needsExclusive() && Status == State::Resolved &&
                         &Cur == ImplicitThread && hasExclusive();
    if (Release) {
      releaseExclusive();
      Ready = [Inner = std::move(Ready), this]() {
        return isExclusiveFree() && Inner();
      };
    }
    Thread::Reason Wake = Thread::Reason::Normal;
    if (!InPlace || !Ready()) {
      Wake = Cur.onWait(std::move(Ready), Cancellable);
    }
    if (Release && Wake != Thread::Reason::Abort) {
      takeExclusive();
    }
    return Wake;
  }

  /// \name Data of task.
  /// @{
  const AST::Component::FuncType *FuncType = nullptr;
  const Instance::ComponentInstance *TypeInst = nullptr;
  bool FuncTypeAsync = false;
  Instance::FunctionInstance *CoreFunc = nullptr;
  /// The host function this task runs instead of a core function.
  HostFunctionBase *HostFunc = nullptr;
  /// A host task spawned with no caller; it ends when its body returns.
  bool Detached = false;
  /// The canonical options of the `canon lift` this activation runs.
  CanonOptions Opts;
  OnStartCallback OnStart;
  OnResolveCallback OnResolve;
  Task *CallerTask = nullptr;
  State Status = State::Initial;
  bool HasStarted = false;
  bool ResolvedByCancel = false;
  uint32_t NumBorrows = 0;
  std::vector<Instance::ComponentInstance::ResourceHandle *> Lenders;
  uint32_t DeliveredCode = 0;
  bool Delivered = false;
  uint32_t SetIdx = 0;
  bool SyncWaiter = false;
  bool CancelRequested = false;
  std::vector<SpawnBody> Spawns;
  std::shared_ptr<Thread::Signal> WakeUp;
  /// The thread the activation itself runs on, set when it enters.
  Thread *ImplicitThread = nullptr;
  /// Every thread registered for this activation, the implicit one included.
  std::vector<Thread *> Threads;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
