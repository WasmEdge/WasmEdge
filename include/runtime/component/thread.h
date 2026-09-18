// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/thread.h - Thread definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the thread of the component model: one suspendable
/// execution flow on a stack of its own or on the stack of its synchronous
/// caller. It parks and resumes; its task rules when it may.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class Task;

/// One suspendable execution flow with a derived state. Only its task may
/// park it, and only the executor may resume it.
class Thread {
public:
  /// Why the thread was woken. A never-resumed thread carries Abort, and a
  /// thread woken with Abort refuses every later park.
  enum class Reason : uint8_t {
    Normal,
    Cancelled,
    Abort,
  };

  /// Where a thread handing the stack over leaves itself.
  enum class Next : uint8_t {
    Suspended,
    Waiting,
  };

  /// What a thread of its own runs.
  using Body = std::function<void()>;

  /// The hand-off between two parties: a binary semaphore that notifies
  /// under its lock, because the released side may destroy it right away.
  class Signal {
  public:
    /// Release the semaphore and wake the acquiring side.
    void release() noexcept {
      try {
        std::lock_guard<std::mutex> Lock(Mutex);
        Value = true;
        CondVar.notify_one();
      } catch (...) {
        // A destroyed peer during teardown: nothing left to signal.
      }
    }
    /// Acquire the semaphore, waiting until it is released.
    void acquire() noexcept {
      try {
        std::unique_lock<std::mutex> Lock(Mutex);
        CondVar.wait(Lock, [this]() { return Value; });
        Value = false;
      } catch (...) {
        // A destroyed peer during teardown: nothing left to wait for.
      }
    }
    /// Acquire, or give up at Until: false when that time passed.
    bool acquireUntil(std::chrono::steady_clock::time_point Until) noexcept {
      try {
        std::unique_lock<std::mutex> Lock(Mutex);
        if (!CondVar.wait_until(Lock, Until, [this]() { return Value; })) {
          return false;
        }
        Value = false;
        return true;
      } catch (...) {
        return false;
      }
    }

  private:
    std::mutex Mutex;
    std::condition_variable CondVar;
    bool Value = false;
  };

  /// A thread on a stack of its own: it runs BodyFunc once resumed and parks
  /// before that.
  Thread(Task &T, Body BodyFunc) noexcept
      : Owner(T), OwnedCarrier(std::make_unique<Carrier>()),
        Stack(*OwnedCarrier) {
    Stack.push(*this);
    Stack.start([this, BodyFunc = std::move(BodyFunc)]() {
      // A stack released to unwind never runs its body.
      if (Wake != Reason::Abort) {
        BodyFunc();
      }
    });
  }
  /// A thread sharing the stack of Outer: the synchronous callee running
  /// above it, which is the only one on that stack that may park.
  Thread(Task &T, Thread &Outer) noexcept
      : Owner(T), Wake(Reason::Normal), Stack(Outer.Stack) {
    Stack.push(*this);
  }
  Thread(const Thread &) = delete;
  Thread &operator=(const Thread &) = delete;

  Task &getOwner() const noexcept { return Owner; }
  /// The innermost thread sharing the stack with this one; itself unless a
  /// synchronous callee runs above it.
  Thread *getInnermost() const noexcept { return Stack.getTop(); }
  /// Whether it gave its stack up for good.
  bool isEnded() const noexcept { return Ended || Stack.isFinished(); }
  /// Whether it runs on a stack of its own rather than on its caller's.
  bool isRoot() const noexcept { return static_cast<bool>(OwnedCarrier); }

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

  /// Whether the suspension point it parks on delivers a cancellation.
  bool isCancellable() const noexcept { return Cancellable; }

  /// \name Scheduler hints of a park.
  /// @{
  const std::optional<std::chrono::steady_clock::time_point> &
  getDeadline() const noexcept {
    return Deadline;
  }
  void setDeadline(
      std::optional<std::chrono::steady_clock::time_point> Until) noexcept {
    Deadline = Until;
  }
  bool isExternal() const noexcept { return External; }
  void setExternal(bool IsExternal) noexcept { External = IsExternal; }
  uint32_t getWaitingSet() const noexcept { return WaitingSet; }
  void setWaitingSet(uint32_t Idx) noexcept { WaitingSet = Idx; }
  /// @}

  /// \name The derived states.
  /// @{
  bool isRunning() const noexcept {
    return Stack.isCheckedOut() && Stack.getTop() == this;
  }
  bool isSuspended() const noexcept { return !isRunning() && !ReadyFunc; }
  bool isWaiting() const noexcept {
    return !isRunning() && static_cast<bool>(ReadyFunc);
  }
  bool isReady() const noexcept { return isWaiting() && ReadyFunc(); }
  /// @}

  /// \name The transitions the executor drives.
  /// @{
  /// Hand the stack to this thread until the threads on it park or end, and
  /// return the thread they named to run next, if any.
  Thread *onResume(Reason WakeReason) noexcept {
    ReadyFunc = nullptr;
    if (Stack.isFinished()) {
      return nullptr;
    }
    Wake = WakeReason;
    return Stack.resume();
  }

  /// A suspended thread becomes ready without handing the stack over;
  /// `thread.resume-later` in the spec.
  void onReady() noexcept {
    ReadyFunc = []() { return true; };
  }

  /// A synchronous callee returned: it leaves the shared stack.
  void onEnd() noexcept {
    if (!OwnedCarrier) {
      Stack.pop();
      Ended = true;
    }
  }
  /// @}

private:
  /// The task rules every suspension of the threads running for it.
  friend class Task;

  /// \name The transitions the task drives.
  /// @{
  /// Park without a readiness predicate: only a named resume wakes it.
  Reason onSuspend(bool IsCancellable) noexcept {
    return park(IsCancellable, nullptr);
  }

  /// Park until Ready() holds.
  Reason onWait(std::function<bool()> Ready, bool IsCancellable) noexcept {
    ReadyFunc = std::move(Ready);
    return park(IsCancellable, nullptr);
  }

  /// Park and name Other as the thread to run next; Other must be suspended.
  Reason onSwitch(Thread &Other, Next NextState, bool IsCancellable) noexcept {
    if (NextState == Next::Waiting) {
      ReadyFunc = []() { return true; };
    }
    return park(IsCancellable, &Other);
  }

  /// Hand over to Other only if it is ready, else park as NextState says.
  Reason onPromote(Thread &Other, Next NextState, bool IsCancellable) noexcept {
    if (Other.isReady()) {
      return onSwitch(Other, NextState, IsCancellable);
    }
    if (NextState == Next::Waiting) {
      return onWait([]() { return true; }, IsCancellable);
    }
    return onSuspend(IsCancellable);
  }
  /// @}

  /// Hand the stack back and wait for the next resume.
  Reason park(bool IsCancellable, Thread *SwitchTo) noexcept {
    // A thread woken to unwind must not park again.
    if (Wake == Reason::Abort) {
      ReadyFunc = nullptr;
      return Reason::Abort;
    }
    Cancellable = IsCancellable;
    Stack.park(SwitchTo);
    Cancellable = false;
    return Wake;
  }

  /// The stack of one execution flow: an OS thread and the hand-off with
  /// whoever resumes it. The threads sharing it form a nest, innermost last.
  class Carrier {
  public:
    Carrier() = default;
    ~Carrier() noexcept {
      if (OSThread.joinable()) {
        if (!Finished) {
          // Release the stack so its body unwinds: every thread on it wakes
          // with an abort and parks no more.
          for (Thread *Entry : Threads) {
            Entry->Wake = Reason::Abort;
          }
          Run.release();
        }
        OSThread.join();
      }
    }
    Carrier(const Carrier &) = delete;
    Carrier &operator=(const Carrier &) = delete;

    /// Start the OS thread on Func; it stops before the first resume.
    void start(Body Func) noexcept {
      BodyFunc = std::move(Func);
      try {
        OSThread = std::thread([this]() { run(); });
      } catch (...) {
        // A stack that never started has nothing to run; resuming it is a
        // no-op.
        Finished = true;
      }
    }

    /// Hand the stack over until it parks or ends, and return the thread it
    /// named to run next, if any.
    Thread *resume() noexcept {
      Signal Mine;
      Back = &Mine;
      NextThread = nullptr;
      CheckedOut = true;
      Run.release();
      Mine.acquire();
      return NextThread;
    }

    /// Hand the stack back to the last resumer and wait for the next resume,
    /// optionally naming the thread to run in the meantime.
    void park(Thread *SwitchTo) noexcept {
      assuming(Back != nullptr);
      NextThread = SwitchTo;
      CheckedOut = false;
      Signal *Resumer = Back;
      Back = nullptr;
      Resumer->release();
      Run.acquire();
    }

    /// Whether the stack is handed out: it or a stack it resumed executes.
    bool isCheckedOut() const noexcept { return CheckedOut; }
    /// Whether the body has returned.
    bool isFinished() const noexcept { return Finished; }

    /// \name The threads sharing this stack, innermost last.
    /// @{
    void push(Thread &Entry) noexcept { Threads.push_back(&Entry); }
    void pop() noexcept {
      if (!Threads.empty()) {
        Threads.pop_back();
      }
    }
    Thread *getTop() const noexcept {
      return Threads.empty() ? nullptr : Threads.back();
    }
    /// @}

  private:
    /// The body of the OS thread.
    void run() noexcept {
      Run.acquire();
      try {
        BodyFunc();
      } catch (...) {
        // During teardown, a hand-off on an already-destroyed peer semaphore
        // can throw. The body then has nothing left to do.
      }
      Finished = true;
      CheckedOut = false;
      NextThread = nullptr;
      if (Back != nullptr) {
        Signal *Resumer = Back;
        Back = nullptr;
        Resumer->release();
      }
    }

    /// \name Data of carrier.
    /// @{
    Signal Run;
    Signal *Back = nullptr;
    Thread *NextThread = nullptr;
    bool CheckedOut = false;
    bool Finished = false;
    std::vector<Thread *> Threads;
    Body BodyFunc;
    std::thread OSThread;
    /// @}
  };

  /// \name Data of thread.
  /// @{
  Task &Owner;
  std::function<bool()> ReadyFunc;
  Reason Wake = Reason::Abort;
  bool Cancellable = false;
  bool Ended = false;
  uint32_t Index = 0;
  uint64_t Storage[2] = {0, 0};
  std::optional<std::chrono::steady_clock::time_point> Deadline;
  bool External = false;
  uint32_t WaitingSet = 0;
  /// The stack of its own, if any; declared last, so its OS thread is joined
  /// before anything the body may still read goes away.
  std::unique_ptr<Carrier> OwnedCarrier;
  Carrier &Stack;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
