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
/// task it runs as, the task manager scheduling it, and the suspension
/// helpers that park the host task the way a guest built-in would.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "runtime/component/hostreactor.h"
#include "runtime/component/task.h"
#include "runtime/component/taskmgr.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class CallingFrame {
public:
  CallingFrame(TaskManager &Manager, Task &Current) noexcept
      : TaskMgr(Manager), CurTask(Current) {}

  /// Get the task manager scheduling this activation.
  TaskManager &getTaskManager() const noexcept { return TaskMgr; }

  /// Get the host task of this activation.
  Task &getTask() const noexcept { return CurTask; }

  /// Get the host instance owning the function.
  const Instance::ComponentInstance *getInstance() const noexcept {
    return CurTask.Opts.Inst;
  }

  /// True once a cancellation was delivered at a suspension point below. The
  /// task is then resolved as cancelled, and the body returns; its value is
  /// discarded.
  bool isCancelled() const noexcept { return Cancelled; }

  /// Park until Ready() holds. The host thread that flips the condition
  /// calls HostReactor::notify() afterwards.
  Expect<void> waitUntil(std::function<bool()> Ready) noexcept {
    auto &Reactor = TaskMgr.getReactor();
    Reactor.addWaiter();
    auto ReasonOrErr =
        TaskMgr.taskWait(CurTask, std::move(Ready), /*Cancellable=*/true);
    Reactor.removeWaiter();
    return settle(std::move(ReasonOrErr));
  }

  /// Park for Duration on the reactor clock.
  Expect<void> sleepFor(std::chrono::nanoseconds Duration) noexcept {
    const auto Deadline = HostReactor::Clock::now() + Duration;
    auto &Reactor = TaskMgr.getReactor();
    const uint64_t Token = Reactor.addDeadline(Deadline);
    auto ReasonOrErr = TaskMgr.taskWait(
        CurTask, [Deadline]() { return HostReactor::Clock::now() >= Deadline; },
        /*Cancellable=*/true);
    Reactor.removeDeadline(Token);
    return settle(std::move(ReasonOrErr));
  }

  /// Run an OS call that may block on a helper thread and park until it
  /// returns. Op owns everything it touches: after a cancellation or an
  /// abort it still finishes on its own.
  Expect<void> blocking(std::function<void()> Op) noexcept {
    auto Done = std::make_shared<std::atomic<bool>>(false);
    auto Reactor = TaskMgr.getReactorHandle();
    Reactor->addWaiter();
    try {
      std::thread([Op = std::move(Op), Done, Reactor]() {
        Op();
        Done->store(true);
        Reactor->notify();
      }).detach();
    } catch (...) {
      Reactor->removeWaiter();
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto ReasonOrErr = TaskMgr.taskWait(
        CurTask, [Done]() { return Done->load(); }, /*Cancellable=*/true);
    Reactor->removeWaiter();
    return settle(std::move(ReasonOrErr));
  }

  /// Start a detached host task of the same instance on its own task thread.
  /// The scheduler runs it at its next turn; it ends when Body returns.
  void spawn(std::function<Expect<void>(CallingFrame &)> Body) noexcept {
    Task *T = TaskMgr.newTask();
    T->Opts.Inst = CurTask.Opts.Inst;
    T->Opts.Async = true;
    T->IsFuncTypeAsync = true;
    T->Detached = true;
    T->Status = Task::State::Started;
    TaskManager *M = &TaskMgr;
    TaskMgr.newThread(T, [M, T, Body = std::move(Body)](ResumeReason Reason) {
      if (Reason == ResumeReason::Abort) {
        return;
      }
      CallingFrame Frame(*M, *T);
      auto Res = Body(Frame);
      T->Status = Task::State::Resolved;
      if (!Res && Res.error() != ErrCode::Value::ComponentAsyncAborted) {
        M->noteTrap(Res.error(), T->Opts.Inst);
      }
    });
    TaskMgr.parkReadyThread(T->Thread);
  }

private:
  /// An abort unwinds the body; a cancellation resolves the task.
  Expect<void> settle(Expect<ResumeReason> ReasonOrErr) noexcept {
    EXPECTED_TRY(auto Reason, ReasonOrErr);
    if (Reason == ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    if (Reason == ResumeReason::Cancelled) {
      Cancelled = true;
      return TaskMgr.taskCancel(CurTask);
    }
    return {};
  }

  /// \name Data of calling frame.
  /// @{
  TaskManager &TaskMgr;
  Task &CurTask;
  bool Cancelled = false;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
