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
/// owns every task and thread below one embedder entry, and it schedules them
/// with a strict hand-off. The executor drives the guest code on top of it.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/expected.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "runtime/component/task.h"
#include "runtime/instance/component/component.h"

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

/// The scheduler: owns every task and thread below one embedder entry.
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
  ThreadContext *newSpawnCtx() noexcept {
    SpawnContexts.push_back(std::make_unique<ThreadContext>());
    return SpawnContexts.back().get();
  }

  /// Create a task thread for T and hand it Body (not yet resumed).
  TaskThread *newThread(Task *T,
                        std::function<void(ResumeReason)> Body) noexcept {
    Threads.push_back(std::make_unique<TaskThread>());
    TaskThread *V = Threads.back().get();
    V->Owner = T;
    V->TaskStack.push_back(T);
    V->ContextStack.push_back(&T->Implicit);
    T->Thread = V;
    T->Implicit.Thread = V;
    V->launch(std::move(Body));
    return V;
  }

  /// Create a task thread for a spawned thread of T, parked until resumed.
  TaskThread *newSpawnThread(Task *T, ThreadContext *Ctx,
                             std::function<void(ResumeReason)> Body) noexcept {
    Threads.push_back(std::make_unique<TaskThread>());
    TaskThread *V = Threads.back().get();
    V->Owner = T;
    V->TaskStack.push_back(T);
    V->ContextStack.push_back(Ctx);
    Ctx->Thread = V;
    V->launch(std::move(Body));
    // The spawned thread starts suspended until an explicit resume.
    Waiting.push_back(V);
    // The instance owns its threads: hook its destructor so they end with it.
    if (T != nullptr && T->Opts.Inst != nullptr) {
      auto &Conc = T->Opts.Inst->concurrency();
      if (!Conc.hasDestroyHook()) {
        const auto *Inst = T->Opts.Inst;
        Conc.setDestroyHook([this, Inst]() { abortThreadsOf(Inst); });
        Adopted.push_back(Inst);
      }
    }
    return V;
  }
  /// @}

  /// \name The current activation.
  /// @{
  /// The task executing on the current task thread (or the embedder thread).
  Task *currentTask() noexcept {
    auto &Stack = Current != nullptr ? Current->TaskStack : EmbedderTasks;
    return Stack.empty() ? nullptr : Stack.back();
  }
  /// The innermost thread-context record (context.get/set, thread.index).
  ThreadContext *currentCtx() noexcept {
    auto &Stack = Current != nullptr ? Current->ContextStack : EmbedderContexts;
    return Stack.empty() ? nullptr : Stack.back();
  }
  TaskThread *currentThread() noexcept { return Current; }

  void pushNestedTask(Task *T) noexcept {
    if (Current != nullptr) {
      Current->TaskStack.push_back(T);
      Current->ContextStack.push_back(&T->Implicit);
    } else {
      EmbedderTasks.push_back(T);
      EmbedderContexts.push_back(&T->Implicit);
    }
  }
  void popNestedTask() noexcept {
    auto &Stack = Current != nullptr ? Current->TaskStack : EmbedderTasks;
    auto &Ctxs = Current != nullptr ? Current->ContextStack : EmbedderContexts;
    if (!Stack.empty()) {
      Stack.pop_back();
    }
    if (!Ctxs.empty()) {
      Ctxs.pop_back();
    }
  }
  /// @}

  /// \name The scheduler.
  /// @{
  /// Hand control to V until it blocks or finishes.
  void resumeThread(TaskThread *V, ResumeReason Reason) noexcept {
    HandoffSemaphore Mine;
    V->Arg = Reason;
    V->Back = &Mine;
    TaskThread *Prev = Current;
    Current = V;
    V->Run.release();
    Mine.acquire();
    Current = Prev;
  }

  /// Resume ready parked threads until Done(); traps on a deadlock.
  Expect<void> pumpUntil(const std::function<bool()> &Done,
                         const Task *Pin = nullptr) noexcept {
    using namespace std::literals;
    // A non-async-typed call in progress restricts what may take the stack.
    const Task *Restrict =
        Pin != nullptr && !Pin->mayBlockAlways() ? Pin : nullptr;
    while (!Done()) {
      if (Trap.has_value()) {
        return Unexpect(*Trap);
      }
      TaskThread *Pick = nullptr;
      for (size_t I = 0; I < Waiting.size(); ++I) {
        TaskThread *V = Waiting[I];
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
      resumeThread(Pick, ResumeReason::Normal);
      if (Trap.has_value()) {
        return Unexpect(*Trap);
      }
    }
    return {};
  }

  /// Whether T may block here; a sync task needs another runnable thread.
  bool mayBlock(const Task &T) const noexcept {
    if (T.mayBlockAlways()) {
      return true;
    }
    // An embedder-driven activation is ready unless it is itself suspended.
    if (Current != nullptr && !T.Implicit.Suspended &&
        std::find(EmbedderContexts.begin(), EmbedderContexts.end(),
                  &T.Implicit) != EmbedderContexts.end()) {
      return true;
    }
    // Any other ready thread of the tree makes blocking legal.
    const auto *Root =
        T.Opts.Inst != nullptr ? T.Opts.Inst->getRoot() : nullptr;
    for (const TaskThread *V : Waiting) {
      if (V == Current || V->Finished || !V->ReadyFn || !V->ReadyFn()) {
        continue;
      }
      if (V->Owner == nullptr || V->Owner->Opts.Inst == nullptr ||
          V->Owner->Opts.Inst->getRoot() != Root) {
        continue;
      }
      return true;
    }
    return false;
  }
  /// @}

  /// \name The task lifecycle.
  /// @{
  /// task.return: resolve T with its component-level results.
  Expect<void> taskReturn(Task &T,
                          std::vector<ComponentValVariant> Results) noexcept {
    using namespace std::literals;
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

  /// task.cancel: resolve T as cancelled.
  Expect<void> taskCancel(Task &T) noexcept {
    using namespace std::literals;
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

  /// Thread-exit bookkeeping of the implicit thread of T.
  Expect<void> taskExit(Task &T) noexcept {
    using namespace std::literals;
    auto &Conc = T.Opts.Inst->concurrency();
    if (T.Implicit.Registered) {
      T.Opts.Inst->concurrency().threadRemove(T.Implicit.Index);
      T.Implicit.Registered = false;
    }
    if (T.FTAsync && T.needsExclusive() && Conc.getExclusiveTask() == &T) {
      Conc.setExclusiveTask(nullptr);
    }
    if (T.St != Task::State::Resolved) {
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
    if (Cancellable && T.St == Task::State::PendingCancel) {
      T.St = Task::State::CancelDelivered;
      return ResumeReason::Cancelled;
    }
    auto &Conc = T.Opts.Inst->concurrency();
    // A mid-core block releases the exclusive slot only once resolved.
    const bool ReleaseExcl =
        T.needsExclusive() && Conc.getExclusiveTask() == &T &&
        (AlwaysReleaseExcl || T.St == Task::State::Resolved);
    if (ReleaseExcl) {
      Conc.setExclusiveTask(nullptr);
    }
    std::function<bool()> Wrapped;
    if (ReleaseExcl) {
      Wrapped = [Cond = std::move(Ready), &Conc]() {
        return Conc.getExclusiveTask() == nullptr && Cond();
      };
    } else {
      Wrapped = std::move(Ready);
    }
    // Fast path: an event-wait whose condition already holds resolves here.
    if (FastPath && Wrapped && Wrapped()) {
      if (ReleaseExcl) {
        Conc.setExclusiveTask(&T);
      }
      return ResumeReason::Normal;
    }
    EXPECTED_TRY(auto Reason, parkCurrent(std::move(Wrapped), Cancellable, &T));
    if (ReleaseExcl && Reason != ResumeReason::Abort) {
      Conc.setExclusiveTask(&T);
    }
    return Reason;
  }

  /// Deliver or queue a cancellation request for T.
  void requestCancellation(Task &T) noexcept {
    // A cancellable suspension point wakes with the Cancelled signal.
    if (T.St == Task::State::Initial) {
      T.St = Task::State::CancelDelivered;
      if (T.Thread != nullptr) {
        resumeParked(T.Thread, ResumeReason::Cancelled);
      }
      return;
    }
    if (T.St != Task::State::Started) {
      return;
    }
    if (T.Thread != nullptr && T.Thread->Cancellable) {
      T.St = Task::State::CancelDelivered;
      if (resumeParked(T.Thread, ResumeReason::Cancelled)) {
        return;
      }
      T.St = Task::State::PendingCancel;
      return;
    }
    T.St = Task::State::PendingCancel;
  }
  /// @}

  /// \name Trap propagation and teardown.
  /// @{
  /// Record the first trap and poison the instance tree containing Inst.
  void noteTrap(ErrCode Err,
                const Runtime::Instance::ComponentInstance *Inst) noexcept {
    if (Err.getEnum() == ErrCode::Value::ComponentAsyncAborted) {
      return;
    }
    if (!Trap.has_value()) {
      Trap = Err;
    }
    if (Inst != nullptr) {
      Inst->getRoot()->concurrency().setPoisoned();
    }
  }
  const std::optional<ErrCode> &trapLatch() const noexcept { return Trap; }

  /// Set while teardown() unwinds; an unwinding thread must not touch Inst.
  bool aborting() const noexcept { return Aborting; }

  /// Whether a parked, still-resumable thread outlives its host call.
  bool hasParkedThreads() const noexcept {
    for (const TaskThread *V : Waiting) {
      if (!V->Finished) {
        return true;
      }
    }
    return false;
  }

  /// Abort and join every live task thread (after a trap or at destruction).
  void teardown() noexcept {
    // Entered on the embedder thread with every live thread parked.
    if (!Trap.has_value()) {
      Trap = ErrCode(ErrCode::Value::ComponentAsyncAborted);
    }
    Aborting = true;
    Waiting.clear();
    // Abort every running thread; index-based, since unwinding can grow it.
    bool Progress = true;
    while (Progress) {
      Progress = false;
      for (size_t I = 0; I < Threads.size(); ++I) {
        TaskThread *V = Threads[I].get();
        if (V != nullptr && !V->Finished) {
          resumeThread(V, ResumeReason::Abort);
          Progress = true;
        }
      }
    }
    Threads.clear();
    Tasks.clear();
    EmbedderTasks.clear();
    // No instance may call back into a manager that has nothing left to abort.
    for (const auto *Inst : Adopted) {
      Inst->concurrency().clearDestroyHook();
    }
    Adopted.clear();
    Trap.reset();
    Aborting = false;
  }

  /// Whether V sits on the waiting list.
  bool isParked(const TaskThread *V) const noexcept {
    return std::find(Waiting.begin(), Waiting.end(), V) != Waiting.end();
  }

  /// Enter one component invoke on the embedder thread.
  void enterInvoke() noexcept { InvokeDepth += 1; }

  /// Leave one component invoke; the remaining depth, 0 at the outermost.
  uint32_t leaveInvoke() noexcept { return --InvokeDepth; }
  /// @}

private:
  /// Remove V from the waiting list and resume it; false when not parked.
  bool resumeParked(TaskThread *V, ResumeReason Reason) noexcept {
    auto It = std::find(Waiting.begin(), Waiting.end(), V);
    if (It == Waiting.end()) {
      return false;
    }
    Waiting.erase(It);
    V->ReadyFn = nullptr;
    resumeThread(V, Reason);
    return true;
  }

  /// Park the current thread until Ready() holds; Pin restricts the pump.
  Expect<ResumeReason> parkCurrent(std::function<bool()> Ready,
                                   bool Cancellable,
                                   const Task *Pin = nullptr) noexcept {
    using namespace std::literals;
    // During teardown a resumed task thread must unwind rather than re-park.
    if (Aborting) {
      return ResumeReason::Abort;
    }
    TaskThread *Self = Current;
    if (Self == nullptr) {
      // Embedder thread cannot park: pump other threads until ready.
      if (!Ready) {
        ThreadContext *Ctx = currentCtx();
        if (Ctx == nullptr) {
          spdlog::error(ErrCode::Value::ComponentAsyncDeadlock);
          spdlog::error("    deadlock detected: event loop cannot make further "
                        "progress"sv);
          return Unexpect(ErrCode::Value::ComponentAsyncDeadlock);
        }
        Ctx->Suspended = true;
        EXPECTED_TRY(pumpUntil([Ctx]() { return !Ctx->Suspended; }, Pin));
        return ResumeReason::Normal;
      }
      EXPECTED_TRY(pumpUntil(Ready, Pin));
      return ResumeReason::Normal;
    }
    Self->ReadyFn = std::move(Ready);
    Self->Cancellable = Cancellable;
    Waiting.push_back(Self);
    HandoffSemaphore *B = Self->Back;
    Self->Back = nullptr;
    B->release();
    Self->Run.acquire();
    return Self->Arg;
  }

  /// Whether V may take the stack while Pin's non-async-typed call runs.
  bool canResumeUnder(const Task &Pin, const TaskThread &V) const noexcept {
    // Resuming a thread of another instance would be unexpected reentrance.
    if (V.Owner == nullptr || V.Owner->Opts.Inst != Pin.Opts.Inst) {
      return false;
    }
    // A task needing the stack exclusively cannot run under the pin.
    return V.ContextStack.empty() ||
           V.ContextStack.back() != &V.Owner->Implicit ||
           !V.Owner->needsExclusive();
  }

  /// Abort and join the threads Inst owns; runs from its destructor.
  void
  abortThreadsOf(const Runtime::Instance::ComponentInstance *Inst) noexcept {
    Adopted.erase(std::remove(Adopted.begin(), Adopted.end(), Inst),
                  Adopted.end());
    auto Owned = [Inst](const TaskThread *V) {
      return V != nullptr && V->Owner != nullptr && V->Owner->Opts.Inst == Inst;
    };
    // A task thread resumed with Abort must not park again while this runs.
    const bool WasAborting = Aborting;
    Aborting = true;
    bool Progress = true;
    while (Progress) {
      Progress = false;
      for (size_t I = 0; I < Threads.size(); ++I) {
        TaskThread *V = Threads[I].get();
        if (V != nullptr && !V->Finished && Owned(V)) {
          Waiting.erase(std::remove(Waiting.begin(), Waiting.end(), V),
                        Waiting.end());
          resumeThread(V, ResumeReason::Abort);
          Progress = true;
        }
      }
    }
    Aborting = WasAborting;
    Threads.erase(
        std::remove_if(Threads.begin(), Threads.end(),
                       [&Owned](const std::unique_ptr<TaskThread> &V) {
                         return Owned(V.get());
                       }),
        Threads.end());
  }

  /// \name Data of task manager.
  /// @{
  /// Depth of the component invokes on the embedder thread.
  uint32_t InvokeDepth = 0;
  /// Threads parked on the waiting list.
  std::vector<TaskThread *> Waiting;
  TaskThread *Current = nullptr;
  std::vector<Task *> EmbedderTasks;
  std::vector<ThreadContext *> EmbedderContexts;
  std::vector<std::unique_ptr<Task>> Tasks;
  std::vector<std::unique_ptr<TaskThread>> Threads;
  std::vector<std::unique_ptr<ThreadContext>> SpawnContexts;
  /// Instances whose destructor hook points here; teardown clears them.
  std::vector<const Runtime::Instance::ComponentInstance *> Adopted;
  std::optional<ErrCode> Trap;
  // Set while teardown() unwinds; a thread resumed with Abort must not park.
  bool Aborting = false;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
