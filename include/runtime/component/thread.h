// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/thread.h - Thread definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the thread of the component model.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "system/poll.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class Task;

/// One suspendable execution flow: its task parks it, the executor resumes it.
/// A root thread owns a stack that its synchronous callees nest on.
class Thread {
public:
  /// Why the thread was woken; after Aborted it parks no more.
  enum class WakeReason : uint8_t {
    Normal,
    Aborted,
  };

  /// What a root thread runs.
  using Body = std::function<void()>;

  /// A root thread, running BodyFunc once resumed.
  Thread(Task &T, Body BodyFunc) noexcept : Owner(T), Root(*this) {
    StackThreads.push_back(this);
    StackBody = [this, Run = std::move(BodyFunc)]() {
      // A stack released to unwind never runs its body.
      if (Wake != WakeReason::Aborted) {
        Run();
      }
    };
    try {
      OSThread = std::thread([this]() { run(); });
    } catch (...) {
      // A stack that never started: resuming it is a no-op.
      Status = ThreadState::Ended;
    }
  }
  /// A synchronous callee nesting on the stack of Outer, which it blocks.
  Thread(Task &T, Thread &Outer) noexcept
      : Owner(T), Root(Outer.Root), Wake(WakeReason::Normal),
        Status(ThreadState::Running) {
    Root.StackThreads.back()->Status = ThreadState::Blocked;
    Root.StackThreads.push_back(this);
  }
  /// A root thread still parked unwinds with an abort before it goes.
  ~Thread() noexcept {
    if (isRoot() && OSThread.joinable()) {
      if (Status != ThreadState::Ended) {
        for (Thread *Entry : StackThreads) {
          Entry->Wake = WakeReason::Aborted;
        }
        setHolder(StackHolder::Stack);
      }
      OSThread.join();
    }
  }
  Thread(const Thread &) = delete;
  Thread &operator=(const Thread &) = delete;

  Task &getOwner() const noexcept { return Owner; }
  /// Whether it runs on a stack of its own rather than on its caller's.
  bool isRoot() const noexcept { return &Root == this; }
  /// The root thread whose stack it runs on; itself for a root.
  Thread &getRoot() const noexcept { return Root; }
  /// The innermost thread nesting on the stack of this one.
  Thread *getInnermost() const noexcept {
    return Root.StackThreads.empty() ? nullptr : Root.StackThreads.back();
  }
  /// The threads nesting on the stack of this one, innermost last.
  Span<Thread *const> getStackThreads() const noexcept {
    return Root.StackThreads;
  }
  /// Whether a pump pinned to the stack of this one runs.
  bool isPinned() const noexcept { return Root.Pinned; }
  void setPinned(bool Flag) noexcept { Root.Pinned = Flag; }

  /// The index in the instance thread table while registered; 0 outside.
  uint32_t getIndex() const noexcept { return Index; }
  void setIndex(uint32_t Idx) noexcept { Index = Idx; }

  /// The context-local storage of `context.get` and `context.set`.
  uint64_t getStorage(uint32_t Slot) const noexcept {
    return Storage[Slot & UINT32_C(1)];
  }
  void setStorage(uint32_t Slot, uint64_t Val) noexcept {
    Storage[Slot & UINT32_C(1)] = Val;
  }

  /// \name The states.
  /// @{
  bool isRunning() const noexcept { return Status == ThreadState::Running; }
  /// A synchronous callee nests above it.
  bool isBlocked() const noexcept { return Status == ThreadState::Blocked; }
  /// Parked with no condition: only a named resume wakes it.
  bool isSuspended() const noexcept { return Status == ThreadState::Suspended; }
  /// Parked until its wait condition holds.
  bool isWaiting() const noexcept { return Status == ThreadState::Waiting; }
  bool isReady() const noexcept {
    if (Status != ThreadState::Waiting) {
      return false;
    }
    // A wait with no condition is a yield.
    if (!ReadyFunc && !Deadline.has_value() && Interests.empty()) {
      return true;
    }
    if (ReadyFunc && ReadyFunc()) {
      return true;
    }
    if (Deadline.has_value() && std::chrono::steady_clock::now() >= *Deadline) {
      return true;
    }
    return std::any_of(Interests.begin(), Interests.end(),
                       [](const PollEntry &Entry) { return Entry.Ready; });
  }
  bool isEnded() const noexcept { return Status == ThreadState::Ended; }
  /// @}

  /// \name The wait condition of a waiting thread.
  /// @{
  const std::optional<std::chrono::steady_clock::time_point> &
  getDeadline() const noexcept {
    return Deadline;
  }
  /// The OS handles it waits on.
  Span<const PollEntry> getInterests() const noexcept { return Interests; }
  /// The waitable set it waits on; 0 for none.
  uint32_t getWaitingSet() const noexcept { return WaitingSet; }
  /// @}

  /// \name The transitions the executor drives.
  /// @{
  /// Run this thread until it parks or ends; the thread named to run next.
  Thread *onResume(WakeReason NewWake) noexcept {
    if (Root.Status == ThreadState::Ended) {
      return nullptr;
    }
    clearWait();
    Status = ThreadState::Running;
    Wake = NewWake;
    return Root.resumeStack();
  }
  /// A parked thread becomes ready whatever it waited for
  /// (`thread.resume-later`, a cancellation of a task at its entry gate).
  void onReady() noexcept {
    clearWait();
    Status = ThreadState::Waiting;
  }
  /// The OS handle at Idx of its interests turned ready.
  void onIoReady(size_t Idx) noexcept { Interests[Idx].Ready = true; }
  /// A synchronous callee returned: it leaves the stack, and its caller runs.
  void onEnd() noexcept {
    if (!isRoot()) {
      Root.StackThreads.pop_back();
      Status = ThreadState::Ended;
      Root.StackThreads.back()->Status = ThreadState::Running;
    }
  }
  /// @}

protected:
  /// The task rules every suspension of the threads running for it.
  friend class Task;

  /// \name The transitions the task drives.
  /// @{
  /// Park with no condition: only a named resume wakes it.
  WakeReason onSuspend() noexcept {
    return park(ThreadState::Suspended, nullptr);
  }
  /// Park until Ready() holds, Until passes or an entry of Entries is ready;
  /// with none of them it is a yield. SetIdx is the waitable set waited on.
  WakeReason onWait(std::function<bool()> Ready,
                    std::optional<std::chrono::steady_clock::time_point> Until,
                    std::vector<PollEntry> Entries, uint32_t SetIdx) noexcept {
    ReadyFunc = std::move(Ready);
    Deadline = Until;
    Interests = std::move(Entries);
    WaitingSet = SetIdx;
    return park(ThreadState::Waiting, nullptr);
  }
  /// Park and name Other as the thread to run next; Other must be suspended.
  WakeReason onSwitch(Thread &Other, bool IsYield) noexcept {
    return park(IsYield ? ThreadState::Waiting : ThreadState::Suspended,
                &Other);
  }
  /// Hand over to Other only if it is ready, else park, ready again when
  /// IsYield.
  WakeReason onPromote(Thread &Other, bool IsYield) noexcept {
    if (Other.isReady()) {
      return onSwitch(Other, IsYield);
    }
    if (IsYield) {
      return onWait(nullptr, std::nullopt, {}, 0);
    }
    return onSuspend();
  }
  /// @}

private:
  /// The states of a thread.
  enum class ThreadState : uint8_t {
    Suspended,
    Waiting,
    Running,
    Blocked,
    Ended,
  };

  /// Who holds the stack of a root thread: its resumer, or its OS thread.
  enum class StackHolder : uint8_t {
    Resumer,
    Stack,
  };

  /// Hand the stack back as To and wait for the next resume.
  WakeReason park(ThreadState To, Thread *SwitchTo) noexcept {
    // A thread woken to unwind must not park again.
    if (Wake == WakeReason::Aborted) {
      clearWait();
      return WakeReason::Aborted;
    }
    Status = To;
    Root.parkStack(SwitchTo);
    return Wake;
  }
  void clearWait() noexcept {
    ReadyFunc = nullptr;
    Deadline.reset();
    Interests.clear();
    WaitingSet = 0;
  }

  /// \name The stack of a root thread.
  /// @{
  /// Run the stack until it parks or ends; the thread named to run next.
  Thread *resumeStack() noexcept {
    NextThread = nullptr;
    setHolder(StackHolder::Stack);
    waitHolder(StackHolder::Resumer);
    return NextThread;
  }
  /// Hand the stack back, naming the thread to run next, and wait.
  void parkStack(Thread *SwitchTo) noexcept {
    NextThread = SwitchTo;
    setHolder(StackHolder::Resumer);
    waitHolder(StackHolder::Stack);
  }
  /// Hand the stack to Next; notified under the lock, as the woken side may
  /// destroy this thread at once.
  void setHolder(StackHolder Next) noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    Holder = Next;
    CondVar.notify_one();
  }
  /// Wait until the stack is handed to Mine.
  void waitHolder(StackHolder Mine) noexcept {
    std::unique_lock<std::mutex> Lock(Mutex);
    CondVar.wait(Lock, [this, Mine]() { return Holder == Mine; });
  }
  /// The body of the OS thread.
  void run() noexcept {
    waitHolder(StackHolder::Stack);
    try {
      StackBody();
    } catch (...) {
      // Nothing may escape the OS thread.
    }
    Status = ThreadState::Ended;
    NextThread = nullptr;
    setHolder(StackHolder::Resumer);
  }
  /// @}

  /// \name Data of thread.
  /// @{
  Task &Owner;
  /// The root thread whose stack this one runs on; itself for a root.
  Thread &Root;
  WakeReason Wake = WakeReason::Aborted;
  ThreadState Status = ThreadState::Suspended;
  uint32_t Index = 0;
  uint64_t Storage[2] = {0, 0};
  /// The wait condition.
  std::function<bool()> ReadyFunc;
  std::optional<std::chrono::steady_clock::time_point> Deadline;
  std::vector<PollEntry> Interests;
  uint32_t WaitingSet = 0;
  /// The stack of a root thread; unused on a nested one.
  std::vector<Thread *> StackThreads;
  bool Pinned = false;
  std::mutex Mutex;
  std::condition_variable CondVar;
  StackHolder Holder = StackHolder::Resumer;
  Thread *NextThread = nullptr;
  Body StackBody;
  std::thread OSThread;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
