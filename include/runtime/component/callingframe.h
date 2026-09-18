// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/callingframe.h - Calling Frame ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the calling frame of a host component function: the
/// task it runs as, the thread it runs on, the executor driving them, and
/// the suspension helpers that park the host task the way a guest built-in
/// would.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "runtime/component/task.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

namespace WasmEdge {

namespace Executor {
class ComponentExecutor;
} // namespace Executor

namespace Runtime {
namespace Component {

class CallingFrame {
public:
  CallingFrame(Task &Current, Thread &Running,
               Executor::ComponentExecutor &CompExec) noexcept
      : HostTask(Current), CurThread(Running), Exec(CompExec) {}

  /// Get the host task of this activation.
  Task &getTask() const noexcept { return HostTask; }

  /// Get the executor driving this activation; the ends of streams and
  /// futures the host owns are operated through it.
  Executor::ComponentExecutor &getExecutor() const noexcept { return Exec; }

  /// Get the host instance owning the function.
  const Instance::ComponentInstance *getInstance() const noexcept {
    return HostTask.getInstance();
  }

  /// True once a cancellation was delivered at a suspension point below; the
  /// task is resolved as cancelled and the value of the body is discarded.
  bool isCancelled() const noexcept { return Cancelled; }

  /// Park until Ready() holds. Only what the scheduler runs may flip it; an
  /// operation on another OS thread parks with runBlocking() instead.
  Expect<void> waitUntil(std::function<bool()> Ready) noexcept {
    return checkWake(park(std::move(Ready)));
  }

  /// Park for Duration.
  Expect<void> sleepFor(std::chrono::nanoseconds Duration) noexcept {
    const auto Deadline = std::chrono::steady_clock::now() + Duration;
    CurThread.setDeadline(Deadline);
    const Thread::Reason Wake = park(
        [Deadline]() { return std::chrono::steady_clock::now() >= Deadline; });
    CurThread.setDeadline(std::nullopt);
    return checkWake(Wake);
  }

  /// Run a blocking OS call on a helper thread and park until it returns. Op
  /// owns everything it touches: it finishes even after a cancellation.
  Expect<void> runBlocking(std::function<void()> Op) noexcept {
    auto Done = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<Thread::Signal> WakeUp = HostTask.getWakeUp();
    try {
      std::thread([Op = std::move(Op), Done, WakeUp]() {
        Op();
        Done->store(true);
        if (WakeUp) {
          WakeUp->release();
        }
      }).detach();
    } catch (...) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    CurThread.setExternal(true);
    const Thread::Reason Wake = park([Done]() { return Done->load(); });
    CurThread.setExternal(false);
    return checkWake(Wake);
  }

  /// Start a detached host task of the same instance on a stack of its own.
  /// The scheduler starts it at its next turn; it ends when Body returns.
  void spawn(Task::SpawnBody Body) noexcept {
    HostTask.postSpawn(std::move(Body));
  }

private:
  /// Park the thread this host task runs on at a cancellable suspension
  /// point of the task.
  Thread::Reason park(std::function<bool()> Ready) noexcept {
    return HostTask.wait(CurThread, std::move(Ready), /*Cancellable=*/true);
  }

  /// An abort unwinds the body; a cancellation resolves the task.
  Expect<void> checkWake(Thread::Reason Wake) noexcept {
    if (Wake == Thread::Reason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    if (Wake == Thread::Reason::Cancelled) {
      Cancelled = true;
      return HostTask.onCancel();
    }
    return {};
  }

  /// \name Data of calling frame.
  /// @{
  Task &HostTask;
  Thread &CurThread;
  Executor::ComponentExecutor &Exec;
  bool Cancelled = false;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
