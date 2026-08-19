// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/async_runtime.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The component-model async task runtime: tasks, their OS-thread execution
/// vehicles with strict hand-off, and the cooperative scheduler.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "common/types.h"
#include "runtime/instance/component/component.h"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {

class Executor;

/// Wake-up signal carried by a resume: normal progress, cancellation
/// delivery, or teardown after a trap elsewhere.
enum class ResumeSignal : uint8_t {
  Normal,
  Cancelled,
  Abort,
};

/// A binary semaphore for the strict hand-off between vehicles. It notifies
/// under the lock, because the released side can destroy it right away.
class HandoffSem {
public:
  void release() noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mtx);
      Value = true;
      Cond.notify_one();
    } catch (...) {
      // A destroyed peer semaphore during teardown: nothing left to signal.
    }
  }
  void acquire() noexcept {
    try {
      std::unique_lock<std::mutex> Lock(Mtx);
      Cond.wait(Lock, [this]() { return Value; });
      Value = false;
    } catch (...) {
    }
  }

private:
  std::mutex Mtx;
  std::condition_variable Cond;
  bool Value = false;
};

class Task;
class TaskVehicle;

/// Context-local record of one thread activation, implicit or spawned:
/// context storage, thread-table index, and driving vehicle.
struct ThreadCtx {
  uint64_t Storage[2] = {0, 0};
  uint32_t Index = 0;
  bool Registered = false;
  /// Set while a vehicle-less (embedder-driven) activation is suspended.
  /// The thread that resumes it clears the flag.
  bool Suspended = false;
  TaskVehicle *Vehicle = nullptr;
  Task *Owner = nullptr;
};

/// The OS-thread execution vehicle of one async task. It parks on its Run
/// semaphore, and `AsyncRuntime::resumeVehicle` hands control to it.
class TaskVehicle {
public:
  TaskVehicle() = default;
  ~TaskVehicle() noexcept;

  /// Start the OS thread executing Body on first resume.
  void launch(std::function<void(ResumeSignal)> Body) noexcept;

  HandoffSem Run;
  HandoffSem *Back = nullptr;
  ResumeSignal Arg = ResumeSignal::Normal;
  bool Finished = false;
  /// Set while the vehicle parks on the waiting list. Without it the vehicle
  /// is suspended and wakes only through thread.resume-later.
  std::function<bool()> ReadyFn;
  bool Cancellable = false;
  /// The task this vehicle was created for.
  Task *Owner = nullptr;
  /// Tasks executing on this vehicle: the owner plus nested sync calls.
  std::vector<Task *> TaskStack;
  /// Thread-context records active on this vehicle, innermost last.
  std::vector<ThreadCtx *> CtxStack;

private:
  std::thread Thread;
};

/// One export activation and its implicit thread record.
class Task {
public:
  enum class State : uint8_t {
    Initial,
    Started,
    PendingCancel,
    CancelDelivered,
    Resolved,
  };

  /// Produce the component-level arguments (caller side).
  using OnStartFn = std::function<Expect<std::vector<ComponentValVariant>>()>;
  /// Consume the component-level results. A nullopt means cancelled.
  using OnResolveFn = std::function<Expect<void>(
      std::optional<std::vector<ComponentValVariant>>)>;

  // Callee function shape.
  const AST::Component::FuncType *FT = nullptr;
  bool FTAsync = false;
  const Runtime::Instance::ComponentInstance *Inst = nullptr;
  Runtime::Instance::FunctionInstance *Core = nullptr;
  Runtime::Instance::MemoryInstance *Mem = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  Runtime::Instance::FunctionInstance *PostReturn = nullptr;
  Runtime::Instance::FunctionInstance *Callback = nullptr;
  StringEncoding Enc = StringEncoding::UTF8;
  bool OptAsync = false;

  // Activation state.
  State St = State::Initial;
  uint32_t NumBorrows = 0;
  OnStartFn OnStart;
  OnResolveFn OnResolve;
  Task *CallerTask = nullptr;
  const Runtime::Instance::ComponentInstance *CallerInst = nullptr;
  /// Non-null while a dedicated vehicle drives this task. It is null for a
  /// task nested synchronously on the caller or on the embedder thread.
  TaskVehicle *Vehicle = nullptr;
  /// Implicit-thread record: context-local storage and the index in the
  /// instance's thread table while registered.
  ThreadCtx Implicit;
  /// Latched error of this task's body (first error wins globally through
  /// the runtime's trap latch).
  std::optional<ErrCode> Failed;

  bool needsExclusive() const noexcept {
    // Exclusivity is only meaningful for async-typed functions.
    return FTAsync && (!OptAsync || Callback != nullptr);
  }
  /// Whether blocking is unconditionally allowed. A synchronous, unresolved
  /// task may still block when another thread can run: see
  /// `AsyncRuntime::mayBlock`.
  bool mayBlockAlways() const noexcept {
    return FTAsync || St == State::Resolved;
  }
};

/// The scheduler: owns every task and vehicle created below one embedder
/// entry, runs the hand-off protocol, and surfaces the first trap.
class AsyncRuntime {
public:
  explicit AsyncRuntime(Executor &E) noexcept : Exec(E) {}
  ~AsyncRuntime() noexcept { teardown(); }

  /// \name The task lifecycle.
  /// @{
  /// canon lift: build and start the task for a lifted component function.
  Expect<Task *>
  liftCall(const Runtime::Instance::Component::FunctionInstance *FuncInst,
           Task::OnStartFn OnStart, Task::OnResolveFn OnResolve,
           Task *CallerTask) noexcept;
  /// task.return / task.cancel / thread-exit bookkeeping.
  Expect<void> taskReturn(Task &T,
                          std::vector<ComponentValVariant> Results) noexcept;
  Expect<void> taskCancel(Task &T) noexcept;
  Expect<void> taskExit(Task &T) noexcept;
  /// Whether T may block here. An async-typed or already-resolved task always
  /// may; a synchronous one may only when another thread of its own instance
  /// tree is ready to run. Whether that thread may take the stack is then the
  /// event loop's decision, which reports a deadlock when none may.
  bool mayBlock(const Task &T) const noexcept;
  /// Cancellable suspension point. `AlwaysReleaseExcl` releases the exclusive
  /// slot around the park, and `FastPath` resolves an event-wait in place.
  Expect<ResumeSignal> taskWait(Task &T, std::function<bool()> Ready,
                                bool Cancellable,
                                bool AlwaysReleaseExcl = false,
                                bool FastPath = false) noexcept;
  /// The canon-lift task body (all four lift shapes).
  Expect<void> runTaskBody(Task &T) noexcept;
  /// Run a resource destructor as an implicit sync task of its instance.
  Expect<void>
  resourceDtorCall(const Runtime::Instance::ComponentInstance *Impl,
                   Runtime::Instance::FunctionInstance *Dtor,
                   uint64_t Rep) noexcept;
  /// Deliver or queue a cancellation request for T.
  void requestCancellation(Task &T) noexcept;
  /// @}

  /// The task executing on the current vehicle (or the embedder thread).
  Task *currentTask() noexcept {
    auto &Stack = Current != nullptr ? Current->TaskStack : EmbedderTasks;
    return Stack.empty() ? nullptr : Stack.back();
  }
  /// The innermost thread-context record (context.get/set, thread.index).
  ThreadCtx *currentCtx() noexcept {
    auto &Stack = Current != nullptr ? Current->CtxStack : EmbedderCtxs;
    return Stack.empty() ? nullptr : Stack.back();
  }
  TaskVehicle *currentVehicle() noexcept { return Current; }

  void pushNestedTask(Task *T) noexcept {
    if (Current != nullptr) {
      Current->TaskStack.push_back(T);
      Current->CtxStack.push_back(&T->Implicit);
    } else {
      EmbedderTasks.push_back(T);
      EmbedderCtxs.push_back(&T->Implicit);
    }
  }
  void popNestedTask() noexcept {
    auto &Stack = Current != nullptr ? Current->TaskStack : EmbedderTasks;
    auto &Ctxs = Current != nullptr ? Current->CtxStack : EmbedderCtxs;
    if (!Stack.empty()) {
      Stack.pop_back();
    }
    if (!Ctxs.empty()) {
      Ctxs.pop_back();
    }
  }

  /// Register a spawned-thread context owned by the runtime.
  ThreadCtx *newSpawnCtx() noexcept {
    SpawnCtxs.push_back(std::make_unique<ThreadCtx>());
    return SpawnCtxs.back().get();
  }

  /// Allocate a task owned by the runtime.
  Task *newTask() noexcept {
    Tasks.push_back(std::make_unique<Task>());
    return Tasks.back().get();
  }

  /// Create a vehicle for T and hand it Body (not yet resumed).
  TaskVehicle *newVehicle(Task *T,
                          std::function<void(ResumeSignal)> Body) noexcept;

  /// Create a vehicle for a spawned thread of T (thread.new-indirect):
  /// starts suspended on the waiting list until resumed.
  TaskVehicle *newSpawnVehicle(Task *T, ThreadCtx *Ctx,
                               std::function<void(ResumeSignal)> Body) noexcept;

  /// Hand control to V until it blocks or finishes. Returns the trap latch
  /// state so callers can propagate promptly.
  void resumeVehicle(TaskVehicle *V, ResumeSignal Sig) noexcept;

  /// Remove V from the waiting list and resume it with Sig. Returns false
  /// when V is not parked.
  bool resumeParked(TaskVehicle *V, ResumeSignal Sig) noexcept;

  /// Park the current vehicle until Ready() holds and return the wake-up
  /// signal. On the embedder thread this pumps, so deadlocks trap here. Pin
  /// is the blocking task, which restricts what the pump may resume.
  Expect<ResumeSignal> parkCurrent(std::function<bool()> Ready,
                                   bool Cancellable,
                                   const Task *Pin = nullptr) noexcept;

  /// Resume ready parked vehicles until Done() holds. Traps with
  /// ComponentAsyncDeadlock when nothing is resumable first.
  Expect<void> pumpUntil(const std::function<bool()> &Done,
                         const Task *Pin = nullptr) noexcept;

  /// Whether V may take the stack while the non-async-typed call of Pin is in
  /// progress. Only threads of Pin's own instance qualify, and never the
  /// implicit thread of a task that needs the stack exclusively.
  bool canResumeUnder(const Task &Pin, const TaskVehicle &V) const noexcept;

  /// Record the first trap and poison the instance tree containing Inst.
  void noteTrap(ErrCode Err,
                const Runtime::Instance::ComponentInstance *Inst) noexcept;
  const std::optional<ErrCode> &trapLatch() const noexcept { return Trap; }

  /// Set while teardown() unwinds the aborted vehicles. A vehicle unwinding
  /// then must not touch its component instance, which can already be gone.
  bool aborting() const noexcept { return Aborting; }

  /// Abort and join every live vehicle (after a trap or at destruction).
  void teardown() noexcept;

  /// Abort and join the threads Inst still owns. It runs from the instance
  /// destructor, so a thread never outlives the instance it belongs to.
  void
  abortThreadsOf(const Runtime::Instance::ComponentInstance *Inst) noexcept;

  /// Whether a thread of some instance is parked and still resumable. Such a
  /// thread outlives the host call that created it.
  bool hasParkedThreads() const noexcept {
    for (const TaskVehicle *V : Waiting) {
      if (!V->Finished) {
        return true;
      }
    }
    return false;
  }

  /// Depth counter of the component invokes on the embedder thread. The
  /// outermost exit clears the finished bookkeeping.
  uint32_t InvokeDepth = 0;

  /// Vehicles parked on the waiting list.
  std::vector<TaskVehicle *> Waiting;

private:
  Executor &Exec;
  TaskVehicle *Current = nullptr;
  std::vector<Task *> EmbedderTasks;
  std::vector<ThreadCtx *> EmbedderCtxs;
  std::vector<std::unique_ptr<Task>> Tasks;
  std::vector<std::unique_ptr<TaskVehicle>> Vehicles;
  std::vector<std::unique_ptr<ThreadCtx>> SpawnCtxs;
  /// The instances whose destructor hook points back here. Teardown clears
  /// them, so a hook never outlives this runtime.
  std::vector<const Runtime::Instance::ComponentInstance *> Adopted;
  std::optional<ErrCode> Trap;
  // Set while teardown() unwinds aborted vehicles. A vehicle resumed with
  // Abort must not park again, because a stale hand-off is referenced then.
  bool Aborting = false;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
