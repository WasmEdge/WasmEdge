// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/task.h - Task definition ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the task of the component model.
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
#include "runtime/instance/component/function.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {

namespace Instance {
class ComponentInstance;
namespace Component {
struct ResourceHandle;
} // namespace Component
} // namespace Instance

namespace Component {

class CallingFrame;

/// One export activation: the task and subtask state machines of the
/// specification, and the rules of the suspension points of its threads.
class Task {
public:
  /// Produce the component-level arguments (caller side).
  using OnStartCallback =
      std::function<Expect<std::vector<ComponentValVariant>>()>;
  /// Consume the component-level results. A nullopt means cancelled.
  using OnResolveCallback = std::function<Expect<void>(
      std::optional<std::vector<ComponentValVariant>>)>;
  /// The body of a detached host activation the host asks for.
  using SpawnBody = std::function<Expect<void>(CallingFrame &)>;

  /// The activation of a `canon lift` of Func, called from Caller.
  Task(const Instance::ComponentFunctionInstance &Func,
       OnStartCallback StartFunc, OnResolveCallback ResolveFunc,
       Task *Caller) noexcept
      : FuncType(&Func.getFuncType()), TypeInst(Func.getTypeInstance()),
        FuncTypeAsync(FuncType->isAsync()), CoreFunc(Func.getLowerFunction()),
        HostFunc(Func.isHostFunction() ? &Func.getHostFunc() : nullptr),
        Opts(Func.getCanonOptions()), OnStart(std::move(StartFunc)),
        OnResolve(std::move(ResolveFunc)), CallerTask(Caller),
        GuestCaller(Caller != nullptr && !Caller->isHost()) {
    if (CallerTask != nullptr) {
      CallerTask->addDependent();
    }
    if (HostFunc != nullptr) {
      Opts.Async = true;
    }
  }
  /// An implicit synchronous activation of Inst, or a detached host one.
  Task(Instance::ComponentInstance *Inst, Task *Caller,
       bool IsDetached = false) noexcept
      : FuncTypeAsync(IsDetached), Detached(IsDetached), CallerTask(Caller),
        GuestCaller(Caller != nullptr && !Caller->isHost()),
        SubtaskStatus(SubtaskState::Started) {
    if (CallerTask != nullptr) {
      CallerTask->addDependent();
    }
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
  /// Host tasks run no guest code.
  bool isHost() const noexcept { return HostFunc != nullptr || isDetached(); }
  const CanonOptions &getCanonOptions() const noexcept { return Opts; }
  Instance::ComponentInstance *getInstance() const noexcept {
    return Opts.Inst;
  }
  /// The caller while it is pinned; null once this task is over.
  Task *getCaller() const noexcept { return CallerTask; }
  /// Whether a guest task called it, rather than a host or the embedder.
  bool hasGuestCaller() const noexcept { return GuestCaller; }
  /// @}

  /// \name The states of the two state machines.
  /// @{
  bool isResolved() const noexcept { return Status == TaskState::Resolved; }
  bool isAborted() const noexcept { return Status == TaskState::Aborted; }
  /// A cancellation was delivered and the task has not resolved yet.
  bool isCancelDelivered() const noexcept {
    return Status == TaskState::CancelDelivered;
  }
  /// Whether the executor may reclaim it: over, thread-less and unpinned.
  bool isReclaimable() const noexcept {
    return (isResolved() || isAborted()) && Threads.empty() &&
           NumDependents == 0;
  }
  /// The subtask state its caller sees, as the code the ABI carries.
  uint32_t getSubtaskCode() const noexcept {
    return static_cast<uint32_t>(SubtaskStatus);
  }
  /// @}

  /// \name The transitions of the two state machines.
  /// @{
  /// Enter the implicit thread through the backpressure and exclusive gate;
  /// a cancellation delivered at the gate leaves it CancelDelivered instead.
  Thread::WakeReason onEnter(Thread &Cur) noexcept;

  /// The subtask starts: produce the component-level arguments.
  Expect<std::vector<ComponentValVariant>> onStart() noexcept {
    assuming(Status == TaskState::Initial &&
             SubtaskStatus == SubtaskState::Starting);
    SubtaskStatus = SubtaskState::Started;
    if (!OnStart) {
      return std::vector<ComponentValVariant>{};
    }
    return OnStart();
  }

  /// `task.return`: resolve with the component-level results.
  Expect<void> onReturn(std::vector<ComponentValVariant> Results) noexcept {
    if (Status == TaskState::Resolved) {
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
    Status = TaskState::Resolved;
    SubtaskStatus = SubtaskState::Returned;
    releaseCaller();
    return {};
  }

  /// `task.cancel`: resolve as cancelled.
  Expect<void> onCancel() noexcept {
    if (Status != TaskState::CancelDelivered) {
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
    Status = TaskState::Resolved;
    SubtaskStatus = SubtaskStatus == SubtaskState::Starting
                        ? SubtaskState::CancelledBeforeStarted
                        : SubtaskState::CancelledBeforeReturned;
    releaseCaller();
    return {};
  }

  /// Request cancellation; returns the thread to resume now, or null: the
  /// gated implicit thread before the start, else a ready one of the instance.
  Thread *onCancelRequest() noexcept;

  /// Deliver a pending cancellation at a suspension point that takes one.
  bool onCancelDelivery() noexcept {
    if (Status == TaskState::PendingCancel) {
      Status = TaskState::CancelDelivered;
      return true;
    }
    return false;
  }

  /// The caller collects the subtask state; the first resolution collected
  /// releases the handles it lent.
  uint32_t onCollect() noexcept {
    const bool WasDelivered = isDelivered();
    DeliveredCode = getSubtaskCode();
    if (isResolved() && !WasDelivered) {
      releaseLenders();
    }
    return DeliveredCode;
  }

  /// Leave the implicit thread and hand the exclusive slot back.
  Expect<void> onExit() noexcept {
    if (FuncTypeAsync && needsExclusive() && hasExclusive()) {
      releaseExclusive();
    }
    if (Thread *Left = std::exchange(ImplicitThread, nullptr)) {
      EXPECTED_TRY(removeThread(*Left));
    }
    return {};
  }

  /// The threads of the activation were aborted, or its instance goes: give
  /// the exclusive slot back and forget the threads, instance and callbacks.
  void onAbort() noexcept;

  /// The caller was aborted or its instance goes: forget its lenders and
  /// callbacks, and unpin it.
  void onCallerAbort() noexcept {
    // The lent handles went with the caller.
    Lenders.clear();
    releaseCaller();
  }
  /// @}

  /// \name Suspension points.
  /// A suspension point that takes a cancellation also wakes on a pending
  /// one; the caller delivers it with `onCancelDelivery` once woken.
  /// @{
  /// Park Cur until Ready() holds.
  Thread::WakeReason wait(Thread &Cur, std::function<bool()> Ready) noexcept {
    return park(Cur, std::move(Ready), std::nullopt, {}, 0,
                /*TakesCancel=*/false, /*InPlace=*/false,
                /*BetweenCallbacks=*/false);
  }
  /// Park Cur until Ready() holds, Until passes, an entry of Entries is ready,
  /// or a cancellation pends.
  Thread::WakeReason
  waitAny(Thread &Cur, std::function<bool()> Ready,
          std::optional<std::chrono::steady_clock::time_point> Until,
          std::vector<PollEntry> Entries) noexcept {
    return park(Cur, std::move(Ready), Until, std::move(Entries), 0,
                /*TakesCancel=*/true, /*InPlace=*/false,
                /*BetweenCallbacks=*/false);
  }
  /// Wait on the waitable set WaitingSet without ceding the stack when
  /// Ready() already holds.
  Thread::WakeReason collect(Thread &Cur, std::function<bool()> Ready,
                             uint32_t WaitingSet) noexcept {
    return park(Cur, std::move(Ready), std::nullopt, {}, WaitingSet,
                /*TakesCancel=*/false, /*InPlace=*/true,
                /*BetweenCallbacks=*/false);
  }
  /// Park Cur ready to run again; it always cedes the stack.
  Thread::WakeReason yield(Thread &Cur) noexcept {
    return park(Cur, nullptr, std::nullopt, {}, 0, /*TakesCancel=*/false,
                /*InPlace=*/false, /*BetweenCallbacks=*/false);
  }
  /// `collect` and `yield` of a callback loop between two callbacks; the
  /// wait also wakes on a pending cancellation.
  Thread::WakeReason collectCallback(Thread &Cur, std::function<bool()> Ready,
                                     uint32_t WaitingSet) noexcept {
    return park(Cur, std::move(Ready), std::nullopt, {}, WaitingSet,
                /*TakesCancel=*/true, /*InPlace=*/true,
                /*BetweenCallbacks=*/true);
  }
  Thread::WakeReason yieldCallback(Thread &Cur) noexcept {
    return park(Cur, nullptr, std::nullopt, {}, 0, /*TakesCancel=*/false,
                /*InPlace=*/false, /*BetweenCallbacks=*/true);
  }
  /// Park Cur with no condition.
  Thread::WakeReason suspend(Thread &Cur) noexcept { return Cur.onSuspend(); }
  /// Park Cur and name Other as the thread to run next.
  Thread::WakeReason switchTo(Thread &Cur, Thread &Other,
                              bool IsYield) noexcept {
    return Cur.onSwitch(Other, IsYield);
  }
  /// Hand over to Other only if it is ready, else park Cur, ready again when
  /// IsYield.
  Thread::WakeReason promote(Thread &Cur, Thread &Other,
                             bool IsYield) noexcept {
    return Cur.onPromote(Other, IsYield);
  }
  /// @}

  /// \name The rules of the activation.
  /// @{
  bool needsExclusive() const noexcept {
    // Exclusivity is only meaningful for async-typed functions.
    return FuncTypeAsync && (!Opts.Async || Opts.Callback != nullptr);
  }
  /// Whether Target may take the stack while this non-async-typed call runs:
  /// a thread of its instance, or of a host task.
  bool mayResume(const Thread &Target) const noexcept {
    const auto &Owner = Target.getOwner();
    return Owner.isHost() || Owner.Opts.Inst == Opts.Inst;
  }
  /// @}

  /// \name The threads of the activation.
  /// @{
  /// The thread the activation runs on; null before it enters.
  Thread *getImplicitThread() const noexcept { return ImplicitThread; }
  Span<Thread *const> getThreads() const noexcept { return Threads; }
  /// Register Entry for this activation and in the instance thread table.
  void addThread(Thread &Entry) noexcept;
  /// Unregister Entry; the last thread to leave owes a result.
  Expect<void> removeThread(Thread &Entry) noexcept;
  /// @}

  /// \name The holders of a pointer to the task, and the subtask view.
  /// @{
  /// Pin the task for one holder: a subtask handle, a caller waiting for it,
  /// a callee it called, a borrow handle scoped to it.
  void addDependent() noexcept { NumDependents += 1; }
  void releaseDependent() noexcept {
    assuming(NumDependents > 0);
    if (NumDependents > 0) {
      NumDependents -= 1;
    }
  }
  /// The subtask state changed since the caller last collected it.
  bool hasPendingEvent() const noexcept {
    return getSubtaskCode() != DeliveredCode;
  }
  /// The caller collected the resolution.
  bool isDelivered() const noexcept {
    return DeliveredCode >= static_cast<uint32_t>(SubtaskState::Returned);
  }
  bool isCancelRequested() const noexcept { return CancelRequested; }
  /// Whether a `subtask.cancel` of the caller waits on the subtask.
  bool isSyncWaiter() const noexcept { return SyncWaiter; }
  void setSyncWaiter(bool IsWaiter) noexcept { SyncWaiter = IsWaiter; }
  /// The waitable set the handle joined; 0 for none.
  uint32_t getSetIdx() const noexcept { return SetIdx; }
  void setSetIdx(uint32_t Idx) noexcept { SetIdx = Idx; }
  /// @}

  /// \name Borrows and lenders.
  /// @{
  /// A borrow handle scoped to this task exists: it pins the task.
  void addBorrow() noexcept {
    NumBorrows += 1;
    addDependent();
  }
  void dropBorrow() noexcept {
    if (NumBorrows > 0) {
      NumBorrows -= 1;
      releaseDependent();
    }
  }
  /// A handle of the caller lent until the resolution is delivered.
  void addLender(Instance::Component::ResourceHandle *Handle) noexcept;
  /// Release the lent handles.
  void releaseLenders() noexcept;
  /// @}

  /// \name The exclusive slot of the instance, held by the implicit thread.
  /// @{
  bool isExclusiveFree() const noexcept;
  bool hasExclusive() const noexcept;
  void takeExclusive() noexcept;
  void releaseExclusive() noexcept;
  /// @}

  /// \name Requests to the scheduler.
  /// @{
  void postSpawn(SpawnBody Body) noexcept { Spawns.push_back(std::move(Body)); }
  std::vector<SpawnBody> takeSpawns() noexcept {
    return std::exchange(Spawns, {});
  }
  /// @}

private:
  /// The task state of the specification: resolution and cancellation.
  enum class TaskState : uint8_t {
    Initial,
    PendingCancel,
    CancelDelivered,
    Resolved,
    Aborted,
  };

  /// The subtask state of the specification; each value is its ABI code.
  enum class SubtaskState : uint32_t {
    Starting = 0,
    Started = 1,
    Returned = 2,
    CancelledBeforeStarted = 3,
    CancelledBeforeReturned = 4,
  };

  /// Forget the caller and its callbacks.
  void releaseCaller() noexcept {
    OnStart = nullptr;
    OnResolve = nullptr;
    if (Task *Caller = std::exchange(CallerTask, nullptr)) {
      Caller->releaseDependent();
    }
  }

  /// The one suspension point of a wait or a yield; it hands the exclusive
  /// slot back between two callbacks, and wakes on a cancellation it takes.
  Thread::WakeReason
  park(Thread &Cur, std::function<bool()> Ready,
       std::optional<std::chrono::steady_clock::time_point> Until,
       std::vector<PollEntry> Entries, uint32_t WaitingSet, bool TakesCancel,
       bool InPlace, bool BetweenCallbacks) noexcept {
    if (TakesCancel) {
      if (Ready) {
        Ready = [Inner = std::move(Ready), this]() {
          return Status == TaskState::PendingCancel || Inner();
        };
      } else if (Until.has_value() || !Entries.empty()) {
        Ready = [this]() { return Status == TaskState::PendingCancel; };
      }
    }
    const bool Release = needsExclusive() && &Cur == ImplicitThread &&
                         hasExclusive() && BetweenCallbacks;
    if (Release) {
      releaseExclusive();
      if (Ready) {
        Ready = [Inner = std::move(Ready), this]() {
          return isExclusiveFree() && Inner();
        };
      } else {
        Ready = [this]() { return isExclusiveFree(); };
      }
    }
    Thread::WakeReason Wake = Thread::WakeReason::Normal;
    if (!InPlace || !Ready()) {
      Wake =
          Cur.onWait(std::move(Ready), Until, std::move(Entries), WaitingSet);
    }
    if (Release && Wake != Thread::WakeReason::Aborted) {
      takeExclusive();
    }
    return Wake;
  }

  /// \name Data of task.
  /// @{
  /// The shape of the activation.
  const AST::Component::FuncType *FuncType = nullptr;
  const Instance::ComponentInstance *TypeInst = nullptr;
  bool FuncTypeAsync = false;
  Instance::FunctionInstance *CoreFunc = nullptr;
  HostFunctionBase *HostFunc = nullptr;
  bool Detached = false;
  CanonOptions Opts;
  OnStartCallback OnStart;
  OnResolveCallback OnResolve;
  /// The caller, pinned until this task resolves or either side aborts.
  Task *CallerTask = nullptr;
  bool GuestCaller = false;
  /// The two state machines.
  TaskState Status = TaskState::Initial;
  SubtaskState SubtaskStatus = SubtaskState::Starting;
  /// The threads of the activation.
  Thread *ImplicitThread = nullptr;
  std::vector<Thread *> Threads;
  /// The holders of a pointer to the task, and the subtask view.
  uint32_t NumDependents = 0;
  uint32_t DeliveredCode = 0;
  bool CancelRequested = false;
  bool SyncWaiter = false;
  uint32_t SetIdx = 0;
  /// The borrows received and the handles lent.
  uint32_t NumBorrows = 0;
  std::vector<Instance::Component::ResourceHandle *> Lenders;
  /// The requests to the scheduler.
  std::vector<SpawnBody> Spawns;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
