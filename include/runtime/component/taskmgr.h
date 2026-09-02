// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/taskmgr.h - Task Manager definition ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Task Manager. It
/// owns every task and thread below one embedder entry and hands control
/// between them with a strict hand-off; the executor's scheduler decides
/// which one runs.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "common/spdlog.h"
#include "runtime/component/hostreactor.h"
#include "runtime/component/task.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/waitable.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// The task manager: owns every task and thread below one embedder entry.
class TaskManager {
public:
  TaskManager() = default;
  ~TaskManager() noexcept { teardown(); }

  /// \name Ownership of the tasks, task threads, and thread contexts.
  /// @{
  /// Allocate a task owned by the manager.
  Task *newTask() noexcept {
    Tasks.push_back(std::make_unique<Task>());
    return Tasks.back().get();
  }

  /// Register a spawned-thread context owned by the manager.
  /// Create the task thread of T and hand it Body (not yet resumed).
  TaskThread *newThread(Task *T,
                        std::function<void(ResumeReason)> Body) noexcept {
    TaskThread *Thread = launchThread(T, std::move(Body));
    Thread->ContextStack.push_back(&T->Implicit);
    T->Implicit.Thread = Thread;
    T->Thread = Thread;
    return Thread;
  }

  /// Create a task thread for a spawned thread of T, parked until resumed.
  /// Its context is the thread's own SpawnContext.
  TaskThread *newSpawnThread(Task *T,
                             std::function<void(ResumeReason)> Body) noexcept {
    TaskThread *Thread = launchThread(T, std::move(Body));
    Thread->ContextStack.push_back(&Thread->SpawnContext);
    Thread->SpawnContext.Thread = Thread;
    // The spawned thread starts suspended until an explicit resume.
    Waiting.push_back(Thread);
    // The instance owns its threads: hook its destructor so they end with it.
    if (T != nullptr && T->Opts.Inst != nullptr) {
      const auto *Inst = T->Opts.Inst;
      if (!Inst->hasDestroyHook()) {
        Inst->setDestroyHook([this, Inst]() { abortThreadsOf(Inst); });
        Adopted.push_back(Inst);
      }
    }
    return Thread;
  }
  /// @}

  /// \name The current activation.
  /// @{
  /// The task executing on the current task thread; null on the scheduler
  /// thread, which runs no task.
  Task *getCurrentTask() const noexcept {
    return Current == nullptr || Current->TaskStack.empty()
               ? nullptr
               : Current->TaskStack.back();
  }
  /// The innermost thread-context record (context.get/set, thread.index).
  ThreadContext *getCurrentContext() const noexcept {
    return Current == nullptr || Current->ContextStack.empty()
               ? nullptr
               : Current->ContextStack.back();
  }
  TaskThread *getCurrentThread() const noexcept { return Current; }

  /// The scope of a synchronous task nested in the current one: an implicit
  /// task of Inst, as a core start function runs under, or a task the caller
  /// built.
  class NestedTaskGuard {
  public:
    NestedTaskGuard(TaskManager &Mgr,
                    const Instance::ComponentInstance *Inst) noexcept
        : TaskMgr(Mgr) {
      Task *T = TaskMgr.newTask();
      T->Opts.Inst = Inst;
      T->CallerTask = TaskMgr.getCurrentTask();
      T->Status = Task::State::Started;
      TaskMgr.pushNestedTask(T);
    }
    NestedTaskGuard(TaskManager &Mgr, Task &T) noexcept : TaskMgr(Mgr) {
      TaskMgr.pushNestedTask(&T);
    }
    ~NestedTaskGuard() noexcept { TaskMgr.popNestedTask(); }
    NestedTaskGuard(const NestedTaskGuard &) = delete;
    NestedTaskGuard &operator=(const NestedTaskGuard &) = delete;

  private:
    TaskManager &TaskMgr;
  };

  /// @}

  /// \name Thread hand-off.
  /// @{
  /// Hand control to Thread until it blocks or finishes.
  void resumeThread(TaskThread *Thread, ResumeReason Reason) noexcept {
    if (Thread->Finished) {
      return;
    }
    HandoffSemaphore Mine;
    Thread->Arg = Reason;
    Thread->Back = &Mine;
    TaskThread *Prev = Current;
    Current = Thread;
    Thread->Run.release();
    Mine.acquire();
    Current = Prev;
  }

  /// Take the first parked thread that is ready and that Eligible accepts;
  /// null when there is none.
  TaskThread *takeReadyThread(
      const std::function<bool(const TaskThread &)> &Eligible) noexcept {
    for (size_t I = 0; I < Waiting.size(); ++I) {
      TaskThread *Thread = Waiting[I];
      if (Thread->ReadyFn && Thread->ReadyFn() && Eligible(*Thread)) {
        Waiting.erase(Waiting.begin() + static_cast<ptrdiff_t>(I));
        Thread->ReadyFn = nullptr;
        return Thread;
      }
    }
    return nullptr;
  }

  /// Take Thread off the waiting list for resumption; false when it is not
  /// parked.
  bool takeParkedThread(TaskThread *Thread) noexcept {
    auto It = std::find(Waiting.begin(), Waiting.end(), Thread);
    if (It == Waiting.end()) {
      return false;
    }
    Waiting.erase(It);
    Thread->ReadyFn = nullptr;
    return true;
  }

  /// Park a new thread as ready, for the scheduler to start at its next turn.
  void parkReadyThread(TaskThread *Thread) noexcept {
    Thread->ReadyFn = []() { return true; };
    Waiting.push_back(Thread);
  }

  const std::vector<TaskThread *> &getWaitingThreads() const noexcept {
    return Waiting;
  }

  /// The wake-up source of the host operations in flight.
  HostReactor &getReactor() noexcept { return *Reactor; }
  /// Shared handle for a helper thread that outlives the wait it wakes.
  std::shared_ptr<HostReactor> getReactorHandle() const noexcept {
    return Reactor;
  }
  /// @}

  /// \name Host transmit ends handed to the scheduler.
  /// @{
  /// Record a state whose posted host end awaits the scheduler.
  void addPostedTransmit(
      std::shared_ptr<Instance::Component::TransmitState> S) noexcept {
    PostedTransmits.push_back(std::move(S));
  }
  /// Take every state handed over since the last take.
  std::vector<std::shared_ptr<Instance::Component::TransmitState>>
  takePostedTransmits() noexcept {
    return std::exchange(PostedTransmits, {});
  }
  /// @}

  /// \name The task lifecycle.
  /// @{
  /// task.return: resolve T with its component-level results.
  Expect<void> taskReturn(Task &T,
                          std::vector<ComponentValVariant> Results) noexcept {
    using namespace std::literals;
    if (T.Status == Task::State::Resolved) {
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
    T.Status = Task::State::Resolved;
    return {};
  }

  /// task.cancel: resolve T as cancelled.
  Expect<void> taskCancel(Task &T) noexcept {
    using namespace std::literals;
    if (T.Status != Task::State::CancelDelivered) {
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
    T.Status = Task::State::Resolved;
    return {};
  }

  /// Thread-exit bookkeeping of the implicit thread of T.
  Expect<void> taskExit(Task &T) noexcept {
    using namespace std::literals;
    const auto *Inst = T.Opts.Inst;
    if (T.Implicit.Index != 0) {
      Inst->removeThread(T.Implicit.Index);
      T.Implicit.Index = 0;
    }
    if (T.IsFuncTypeAsync && T.needsExclusive() &&
        Inst->getExclusiveTask() == &T) {
      Inst->setExclusiveTask(nullptr);
    }
    if (T.Status != Task::State::Resolved) {
      spdlog::error(ErrCode::Value::ComponentNoAsyncResult);
      spdlog::error("    async-lifted export failed to produce a result"sv);
      return Unexpect(ErrCode::Value::ComponentNoAsyncResult);
    }
    return {};
  }

  /// Cancellable suspension point with the exclusive-slot and fast-path flags.
  Expect<ResumeReason> taskWait(Task &T, std::function<bool()> Ready,
                                bool Cancellable,
                                bool AlwaysReleaseExcl = false,
                                bool FastPath = false) noexcept {
    if (Cancellable && T.Status == Task::State::PendingCancel) {
      T.Status = Task::State::CancelDelivered;
      return ResumeReason::Cancelled;
    }
    const auto *Inst = T.Opts.Inst;
    // A mid-core block releases the exclusive slot only once resolved.
    const bool ReleaseExcl =
        T.needsExclusive() && Inst->getExclusiveTask() == &T &&
        (AlwaysReleaseExcl || T.Status == Task::State::Resolved);
    if (ReleaseExcl) {
      Inst->setExclusiveTask(nullptr);
    }
    std::function<bool()> Wrapped;
    if (ReleaseExcl) {
      Wrapped = [Cond = std::move(Ready), Inst]() {
        return Inst->getExclusiveTask() == nullptr && Cond();
      };
    } else {
      Wrapped = std::move(Ready);
    }
    // Fast path: an event-wait whose condition already holds resolves here.
    if (FastPath && Wrapped && Wrapped()) {
      if (ReleaseExcl) {
        Inst->setExclusiveTask(&T);
      }
      return ResumeReason::Normal;
    }
    EXPECTED_TRY(auto Reason, parkCurrent(std::move(Wrapped), Cancellable, &T));
    if (ReleaseExcl && Reason != ResumeReason::Abort) {
      Inst->setExclusiveTask(&T);
    }
    return Reason;
  }

  /// Deliver or queue a cancellation request for T.
  void requestCancellation(Task &T) noexcept {
    // A cancellable suspension point wakes with the Cancelled signal.
    if (T.Status == Task::State::Initial) {
      T.Status = Task::State::CancelDelivered;
      if (T.Thread != nullptr) {
        resumeParked(T.Thread, ResumeReason::Cancelled);
      }
      return;
    }
    if (T.Status != Task::State::Started) {
      return;
    }
    if (T.Thread != nullptr && T.Thread->Cancellable) {
      T.Status = Task::State::CancelDelivered;
      if (resumeParked(T.Thread, ResumeReason::Cancelled)) {
        return;
      }
      T.Status = Task::State::PendingCancel;
      return;
    }
    T.Status = Task::State::PendingCancel;
  }
  /// @}

  /// \name Trap propagation and teardown.
  /// @{
  /// Record the first trap and poison the instance tree containing Inst.
  void noteTrap(ErrCode Err, const Instance::ComponentInstance *Inst) noexcept {
    if (Err.getEnum() == ErrCode::Value::ComponentAsyncAborted) {
      return;
    }
    if (!Trap.has_value()) {
      Trap = Err;
    }
    // An orderly termination is no trap: the tree stays usable.
    if (Inst != nullptr && Err.getEnum() != ErrCode::Value::Terminated) {
      Inst->getRoot()->setPoisoned();
    }
  }
  const std::optional<ErrCode> &getTrapLatch() const noexcept { return Trap; }

  /// Set while teardown() unwinds; an unwinding thread must not touch Inst.
  bool isAborting() const noexcept { return Aborting; }

  /// Whether a parked, still-resumable thread outlives its host call.
  bool hasParkedThreads() const noexcept {
    for (const TaskThread *Thread : Waiting) {
      if (!Thread->Finished) {
        return true;
      }
    }
    return false;
  }

  /// Abort and join every live task thread (after a trap or at destruction).
  void teardown() noexcept {
    // Entered on the scheduler thread with every live thread parked.
    if (!Trap.has_value()) {
      Trap = ErrCode(ErrCode::Value::ComponentAsyncAborted);
    }
    Aborting = true;
    abortThreads([](const TaskThread &) { return true; });
    Threads.clear();
    Tasks.clear();
    PostedTransmits.clear();
    // No instance may call back into a manager that has nothing left to abort.
    for (const auto *Inst : Adopted) {
      Inst->clearDestroyHook();
    }
    Adopted.clear();
    Trap.reset();
    Aborting = false;
  }

  /// Whether Thread sits on the waiting list.
  bool isParked(const TaskThread *Thread) const noexcept {
    return std::find(Waiting.begin(), Waiting.end(), Thread) != Waiting.end();
  }
  /// @}

private:
  /// Allocate a task thread carrying T and start its OS thread on Body (not
  /// yet resumed); the caller pushes its innermost context.
  TaskThread *launchThread(Task *T,
                           std::function<void(ResumeReason)> Body) noexcept {
    Threads.push_back(std::make_unique<TaskThread>());
    TaskThread *Thread = Threads.back().get();
    Thread->TaskStack.push_back(T);
    Thread->launch(std::move(Body));
    return Thread;
  }

  /// Abort and join every live thread Owned accepts; index-based, since the
  /// unwinding can grow the list.
  void
  abortThreads(const std::function<bool(const TaskThread &)> &Owned) noexcept {
    bool Progress = true;
    while (Progress) {
      Progress = false;
      for (size_t I = 0; I < Threads.size(); ++I) {
        TaskThread *Thread = Threads[I].get();
        if (Thread != nullptr && !Thread->Finished && Owned(*Thread)) {
          Waiting.erase(std::remove(Waiting.begin(), Waiting.end(), Thread),
                        Waiting.end());
          resumeThread(Thread, ResumeReason::Abort);
          Progress = true;
        }
      }
    }
  }

  /// Make T the current task of the current task thread.
  void pushNestedTask(Task *T) noexcept {
    assuming(Current != nullptr);
    Current->TaskStack.push_back(T);
    Current->ContextStack.push_back(&T->Implicit);
  }
  void popNestedTask() noexcept {
    assuming(Current != nullptr);
    if (!Current->TaskStack.empty()) {
      Current->TaskStack.pop_back();
    }
    if (!Current->ContextStack.empty()) {
      Current->ContextStack.pop_back();
    }
  }

  /// Remove Thread from the waiting list and resume it; false when not parked.
  bool resumeParked(TaskThread *Thread, ResumeReason Reason) noexcept {
    if (!takeParkedThread(Thread)) {
      return false;
    }
    resumeThread(Thread, Reason);
    return true;
  }

  /// Park the current task thread until Ready() holds; a null Ready suspends
  /// it until thread.resume-later. Pin is the parking task: while a
  /// non-async-typed one is parked, the scheduler resumes only what may run
  /// under its call.
  Expect<ResumeReason> parkCurrent(std::function<bool()> Ready,
                                   bool Cancellable,
                                   const Task *Pin = nullptr) noexcept {
    // During teardown a resumed task thread must unwind rather than re-park.
    if (Aborting) {
      return ResumeReason::Abort;
    }
    TaskThread *Self = Current;
    assuming(Self != nullptr);
    Self->ReadyFn = std::move(Ready);
    Self->Cancellable = Cancellable;
    Self->Pin = Pin != nullptr && !Pin->mayBlockAlways() ? Pin : nullptr;
    Waiting.push_back(Self);
    HandoffSemaphore *B = Self->Back;
    Self->Back = nullptr;
    B->release();
    Self->Run.acquire();
    Self->Pin = nullptr;
    return Self->Arg;
  }

  /// Abort and join the threads Inst owns; runs from its destructor.
  void abortThreadsOf(const Instance::ComponentInstance *Inst) noexcept {
    Adopted.erase(std::remove(Adopted.begin(), Adopted.end(), Inst),
                  Adopted.end());
    auto Owned = [Inst](const TaskThread &Thread) {
      const Task *Owner = Thread.getOwner();
      return Owner != nullptr && Owner->Opts.Inst == Inst;
    };
    // A task thread resumed with Abort must not park again while this runs.
    const bool WasAborting = Aborting;
    Aborting = true;
    abortThreads(Owned);
    Aborting = WasAborting;
    Threads.erase(
        std::remove_if(Threads.begin(), Threads.end(),
                       [&Owned](const std::unique_ptr<TaskThread> &Thread) {
                         return Thread != nullptr && Owned(*Thread);
                       }),
        Threads.end());
  }

  /// \name Data of task manager.
  /// @{
  /// Threads parked on the waiting list.
  std::vector<TaskThread *> Waiting;
  /// The task thread holding control; null while the scheduler thread does.
  TaskThread *Current = nullptr;
  std::vector<std::unique_ptr<Task>> Tasks;
  std::vector<std::unique_ptr<TaskThread>> Threads;
  /// Instances whose destructor hook points here; teardown clears them.
  std::vector<const Instance::ComponentInstance *> Adopted;
  /// States with a host end posted against a parked peer.
  std::vector<std::shared_ptr<Instance::Component::TransmitState>>
      PostedTransmits;
  std::shared_ptr<HostReactor> Reactor = std::make_shared<HostReactor>();
  std::optional<ErrCode> Trap;
  // Set while teardown() unwinds; a thread resumed with Abort must not park.
  bool Aborting = false;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
