// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/task.h - Task Records definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the activation records of the component model async
/// model: one task per export activation, the OS thread carrying it, and the
/// hand-off semaphore between two such threads. The scheduler driving them is
/// Runtime::Component::TaskManager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/types.h"
#include "runtime/component/canonopt.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace WasmEdge {
namespace Runtime {

namespace Instance {
class ComponentInstance;
class FunctionInstance;
} // namespace Instance

namespace Component {

/// Why a resumed thread was woken: normal progress, cancellation
/// delivery, or teardown after a trap elsewhere.
enum class ResumeReason : uint8_t {
  Normal,
  Cancelled,
  Abort,
};

/// A binary semaphore for the strict hand-off between task threads. It notifies
/// under the lock, because the released side can destroy it right away.
class HandoffSemaphore {
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
class TaskThread;

/// Context-local record of one thread activation, implicit or spawned:
/// context storage, thread-table index, and driving task thread.
struct ThreadContext {
  uint64_t Storage[2] = {0, 0};
  uint32_t Index = 0;
  bool Registered = false;
  /// Set while an activation with no task thread (embedder-driven) is
  /// suspended.
  /// The thread that resumes it clears the flag.
  bool Suspended = false;
  TaskThread *Thread = nullptr;
};

/// The OS thread carrying one async task. It parks on its Run
/// semaphore, and `TaskManager::resumeThread` hands control to it.
class TaskThread {
public:
  TaskThread() = default;
  ~TaskThread() noexcept;

  /// Start the OS thread executing Body on first resume.
  void launch(std::function<void(ResumeReason)> Body) noexcept;

  HandoffSemaphore Run;
  HandoffSemaphore *Back = nullptr;
  ResumeReason Arg = ResumeReason::Normal;
  bool Finished = false;
  /// Set while the task thread parks on the waiting list. Without it the
  /// thread is suspended and wakes only through thread.resume-later.
  std::function<bool()> ReadyFn;
  bool Cancellable = false;
  /// The task this task thread was created for.
  Task *Owner = nullptr;
  /// Tasks executing on this task thread: the owner plus nested sync calls.
  std::vector<Task *> TaskStack;
  /// Thread-context records active on this task thread, innermost last.
  std::vector<ThreadContext *> ContextStack;

private:
  std::thread Thread;
};

inline TaskThread::~TaskThread() noexcept {
  if (Thread.joinable()) {
    if (!Finished) {
      // Never-resumed task thread: release the parked thread so its body can
      // observe the abort and finish.
      Arg = ResumeReason::Abort;
      Run.release();
    }
    Thread.join();
  }
}

inline void
TaskThread::launch(std::function<void(ResumeReason)> Body) noexcept {
  Thread = std::thread([this, Fn = std::move(Body)]() {
    Run.acquire();
    try {
      Fn(Arg);
    } catch (...) {
      // During teardown, a HandoffSemaphore operation on an already-destroyed
      // peer can throw. The body then has nothing left to do.
    }
    Finished = true;
    if (Back != nullptr) {
      HandoffSemaphore *B = Back;
      Back = nullptr;
      B->release();
    }
  });
}

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
  using OnStartCallback =
      std::function<Expect<std::vector<ComponentValVariant>>()>;
  /// Consume the component-level results. A nullopt means cancelled.
  using OnResolveCallback = std::function<Expect<void>(
      std::optional<std::vector<ComponentValVariant>>)>;

  // Callee function shape.
  const AST::Component::FuncType *FT = nullptr;
  bool FTAsync = false;
  Runtime::Instance::FunctionInstance *Core = nullptr;
  /// The canonical options of the `canon lift` this activation runs.
  CanonOptions Opts;

  // Activation state.
  State St = State::Initial;
  uint32_t NumBorrows = 0;
  OnStartCallback OnStart;
  OnResolveCallback OnResolve;
  Task *CallerTask = nullptr;
  const Runtime::Instance::ComponentInstance *CallerInst = nullptr;
  /// Non-null while a dedicated task thread drives this task. It is null for a
  /// task nested synchronously on the caller or on the embedder thread.
  TaskThread *Thread = nullptr;
  /// Implicit-thread record: context-local storage and the index in the
  /// instance's thread table while registered.
  ThreadContext Implicit;
  /// Latched error of this task's body (first error wins globally through
  /// the runtime's trap latch).
  std::optional<ErrCode> Failed;

  bool needsExclusive() const noexcept {
    // Exclusivity is only meaningful for async-typed functions.
    return FTAsync && (!Opts.Async || Opts.Callback != nullptr);
  }
  /// Whether blocking is unconditionally allowed. A synchronous, unresolved
  /// task may still block when another thread can run: see
  /// `TaskManager::mayBlock`.
  bool mayBlockAlways() const noexcept {
    return FTAsync || St == State::Resolved;
  }
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
