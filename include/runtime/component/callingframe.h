// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/callingframe.h - Calling Frame ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the calling frame of a host component function.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "runtime/component/task.h"
#include "runtime/instance/component/component.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

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
      : HostTask(Current), CurThread(Running), Exec(CompExec),
        HostInst(Current.getInstance()) {}

  /// Get the host task of this activation.
  Task &getTask() const noexcept { return HostTask; }

  /// Get the executor driving this activation.
  Executor::ComponentExecutor &getExecutor() const noexcept { return Exec; }

  /// Get the host instance owning the function; it stays after an abort.
  Instance::ComponentInstance *getInstance() const noexcept { return HostInst; }

  /// True once a cancellation was delivered; the value of the body is
  /// discarded.
  bool isCancelled() const noexcept { return Cancelled; }

  /// Park until Ready() holds.
  Expect<void> waitUntil(std::function<bool()> Ready) noexcept {
    return checkWake(
        HostTask.waitAny(CurThread, std::move(Ready), std::nullopt, {}));
  }

  /// Park for Duration.
  Expect<void> sleepFor(std::chrono::nanoseconds Duration) noexcept {
    return waitAny(nullptr, std::chrono::steady_clock::now() + Duration, {});
  }

  /// Park until the OS handle Handle is ready for reading.
  Expect<void> waitReadable(uint64_t Handle) noexcept {
    return waitAny(nullptr, std::nullopt, {PollEntry{Handle, false, false}});
  }

  /// Park until the OS handle Handle is ready for writing.
  Expect<void> waitWritable(uint64_t Handle) noexcept {
    return waitAny(nullptr, std::nullopt, {PollEntry{Handle, true, false}});
  }

  /// Park until Ready() holds, Until passes or an entry of Entries is ready.
  Expect<void>
  waitAny(std::function<bool()> Ready,
          std::optional<std::chrono::steady_clock::time_point> Until,
          std::vector<PollEntry> Entries) noexcept {
    return checkWake(HostTask.waitAny(CurThread, std::move(Ready), Until,
                                      std::move(Entries)));
  }

  /// Start a detached host task of the same instance at the next turn.
  void spawn(Task::SpawnBody Body) noexcept {
    HostTask.postSpawn(std::move(Body));
  }

private:
  /// An abort unwinds the body; a pending cancellation is delivered here and
  /// resolves the task.
  Expect<void> checkWake(Thread::WakeReason Wake) noexcept {
    if (Wake == Thread::WakeReason::Aborted) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    if (HostTask.onCancelDelivery()) {
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
  Instance::ComponentInstance *HostInst;
  bool Cancelled = false;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
