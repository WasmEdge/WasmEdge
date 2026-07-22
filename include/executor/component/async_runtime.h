// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/async_runtime.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The component-model async task runtime: tasks (one per export
/// activation), their execution vehicles (OS threads with strict hand-off,
/// mirroring the spec's definitional interpreter where exactly one thread
/// runs at any time), and the cooperative scheduler that resumes waiting
/// tasks until the driven call resolves.
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

class Executor;

/// Wake-up signal carried by a resume: normal progress, cancellation
/// delivery, or teardown after a trap elsewhere.
enum class ResumeSignal : uint8_t {
  Normal,
  Cancelled,
  Abort,
};

/// A binary semaphore for the strict hand-off between vehicles. The
/// notification happens under the lock: the released side may destroy this
/// semaphore as soon as its acquire returns, so the releaser must be done
/// touching the condition variable by then.
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

class ComponentTask;
class TaskVehicle;

/// Context-local record of one thread activation (spec `Thread`): the
/// context.get/set storage, the per-instance thread-table index, and the
/// vehicle driving it (implicit task threads and spawned threads alike).
struct ComponentThreadCtx {
  uint64_t Storage[2] = {0, 0};
  uint32_t Index = 0;
  bool Registered = false;
  TaskVehicle *Vehicle = nullptr;
  ComponentTask *Owner = nullptr;
};

/// The OS-thread execution vehicle of one async task. The thread parks on
/// its Run semaphore; `AsyncRuntime::resumeVehicle` hands control to it and
/// blocks until the vehicle blocks again or its body finishes.
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
  /// Set while parked on the scheduler's waiting list; a parked vehicle
  /// with no ready predicate is suspended (thread.suspend) and only wakes
  /// through thread.resume-later and friends.
  std::function<bool()> ReadyFn;
  bool Cancellable = false;
  /// The task this vehicle was created for.
  ComponentTask *Owner = nullptr;
  /// Tasks executing on this vehicle: the owner plus nested sync calls.
  std::vector<ComponentTask *> TaskStack;
  /// Thread-context records active on this vehicle, innermost last.
  std::vector<ComponentThreadCtx *> CtxStack;

private:
  std::thread Thread;
};

/// One export activation (spec `Task` + its implicit thread record).
class ComponentTask {
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
  /// Consume the component-level results; nullopt = cancelled.
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
  bool AlwaysTaskReturn = false;

  // Activation state.
  State St = State::Initial;
  uint32_t NumBorrows = 0;
  OnStartFn OnStart;
  OnResolveFn OnResolve;
  ComponentTask *CallerTask = nullptr;
  const Runtime::Instance::ComponentInstance *CallerInst = nullptr;
  /// Non-null while a dedicated vehicle drives this task; null for tasks
  /// nested synchronously on the caller's vehicle (or the embedder thread).
  TaskVehicle *Vehicle = nullptr;
  /// Implicit-thread record: context-local storage and the index in the
  /// instance's thread table while registered.
  ComponentThreadCtx Implicit;
  /// Latched error of this task's body (first error wins globally through
  /// the runtime's trap latch).
  std::optional<ErrCode> Failed;

  bool needsExclusive() const noexcept {
    // Spec Task.needs_exclusive: only meaningful for async-typed functions.
    return FTAsync && (!OptAsync || Callback != nullptr);
  }
  bool mayBlock() const noexcept { return FTAsync || St == State::Resolved; }
};

/// The scheduler: owns every task and vehicle created below one embedder
/// entry, runs the hand-off protocol, and surfaces the first trap.
class AsyncRuntime {
public:
  ~AsyncRuntime() noexcept { teardown(); }

  /// The task executing on the current vehicle (or the embedder thread).
  ComponentTask *currentTask() noexcept {
    auto &Stack = Current != nullptr ? Current->TaskStack : EmbedderTasks;
    return Stack.empty() ? nullptr : Stack.back();
  }
  /// The innermost thread-context record (context.get/set, thread.index).
  ComponentThreadCtx *currentCtx() noexcept {
    auto &Stack = Current != nullptr ? Current->CtxStack : EmbedderCtxs;
    return Stack.empty() ? nullptr : Stack.back();
  }
  TaskVehicle *currentVehicle() noexcept { return Current; }

  void pushNestedTask(ComponentTask *T) noexcept {
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
  ComponentThreadCtx *newSpawnCtx() noexcept {
    SpawnCtxs.push_back(std::make_unique<ComponentThreadCtx>());
    return SpawnCtxs.back().get();
  }

  /// Allocate a task owned by the runtime.
  ComponentTask *newTask() noexcept {
    Tasks.push_back(std::make_unique<ComponentTask>());
    return Tasks.back().get();
  }

  /// Create a vehicle for T and hand it Body (not yet resumed).
  TaskVehicle *newVehicle(ComponentTask *T,
                          std::function<void(ResumeSignal)> Body) noexcept;

  /// Create a vehicle for a spawned thread of T (thread.new-indirect):
  /// starts suspended on the waiting list until resumed.
  TaskVehicle *newSpawnVehicle(ComponentTask *T, ComponentThreadCtx *Ctx,
                               std::function<void(ResumeSignal)> Body) noexcept;

  /// Hand control to V until it blocks or finishes. Returns the trap latch
  /// state so callers can propagate promptly.
  void resumeVehicle(TaskVehicle *V, ResumeSignal Sig) noexcept;

  /// Remove V from the waiting list and resume it with Sig. Returns false
  /// when V is not parked.
  bool resumeParked(TaskVehicle *V, ResumeSignal Sig) noexcept;

  /// Park the current vehicle until Ready() holds (the pump resumes it).
  /// Returns the wake-up signal (Cancelled only when Cancellable). On the
  /// embedder thread this pumps instead (deadlock traps surface here).
  Expect<ResumeSignal> parkCurrent(std::function<bool()> Ready,
                                   bool Cancellable) noexcept;

  /// Resume ready parked vehicles until Done() holds. Traps with
  /// ComponentAsyncDeadlock when nothing is resumable first.
  Expect<void> pumpUntil(const std::function<bool()> &Done) noexcept;

  /// Record the first trap and poison the instance tree containing Inst.
  void noteTrap(ErrCode Err,
                const Runtime::Instance::ComponentInstance *Inst) noexcept;
  const std::optional<ErrCode> &trapLatch() const noexcept { return Trap; }

  /// Abort and join every live vehicle (after a trap or at destruction).
  void teardown() noexcept;

  /// Depth counter of component invokes on the embedder thread; the
  /// outermost exit clears finished bookkeeping.
  uint32_t InvokeDepth = 0;

  /// Vehicles parked on the waiting list (spec store.waiting).
  std::vector<TaskVehicle *> Waiting;

private:
  TaskVehicle *Current = nullptr;
  std::vector<ComponentTask *> EmbedderTasks;
  std::vector<ComponentThreadCtx *> EmbedderCtxs;
  std::vector<std::unique_ptr<ComponentTask>> Tasks;
  std::vector<std::unique_ptr<TaskVehicle>> Vehicles;
  std::vector<std::unique_ptr<ComponentThreadCtx>> SpawnCtxs;
  std::optional<ErrCode> Trap;
  // Set while teardown() unwinds aborted vehicles: a vehicle resumed with
  // Abort must not re-park (which would reference a stale hand-off).
  bool Aborting = false;
};

} // namespace Executor
} // namespace WasmEdge
