// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/concurrencymgr.h - Concurrency -==//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Concurrency
/// Manager: the concurrency state and the thread table of one component
/// instance.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {

namespace Component {
class Task;
struct ThreadContext;
} // namespace Component

namespace Instance {
namespace Component {

/// The concurrency state of one component instance. MayLeave marks the
/// no-leave regions, and Poisoned marks a trapped instance tree.
class ConcurrencyManager {
public:
  /// Getter and setter for the no-leave region flag.
  bool mayLeave() const noexcept { return MayLeave; }
  void setMayLeave(bool V) noexcept { MayLeave = V; }

  /// Getter for the backpressure counter.
  int64_t getBackpressure() const noexcept { return Backpressure; }

  /// Raise the backpressure counter; false on overflow.
  [[nodiscard]] bool incBackpressure() noexcept {
    Backpressure += 1;
    return Backpressure != (INT64_C(1) << 16);
  }

  /// Lower the backpressure counter; false on underflow.
  [[nodiscard]] bool decBackpressure() noexcept {
    Backpressure -= 1;
    return Backpressure >= 0;
  }

  /// Getter and counters for the tasks blocked at the entry gate.
  uint32_t getNumWaitingToEnter() const noexcept { return NumWaitingToEnter; }
  void incWaitingToEnter() noexcept { NumWaitingToEnter += 1; }
  void decWaitingToEnter() noexcept { NumWaitingToEnter -= 1; }

  /// Getter and setter for the task holding the instance exclusively.
  Runtime::Component::Task *getExclusiveTask() const noexcept {
    return ExclusiveTask;
  }
  void setExclusiveTask(Runtime::Component::Task *T) noexcept {
    ExclusiveTask = T;
  }

  /// A trapped instance tree rejects every further entry.
  bool isPoisoned() const noexcept { return Poisoned; }
  void setPoisoned() noexcept { Poisoned = true; }

  /// The destroy hook, installed by the task manager. A thread belongs to the
  /// instance rather than to the call that spawned it, so the instance aborts
  /// and joins its threads when it goes away.
  bool hasDestroyHook() const noexcept { return static_cast<bool>(OnDestroy); }
  void setDestroyHook(std::function<void()> Hook) noexcept {
    OnDestroy = std::move(Hook);
  }
  void clearDestroyHook() noexcept { OnDestroy = nullptr; }
  void runDestroyHook() const noexcept {
    if (OnDestroy) {
      OnDestroy();
    }
  }

  /// The per-instance thread table: every thread activation registers here,
  /// and thread.index reads it.
  uint32_t threadAdd(Runtime::Component::ThreadContext *T) noexcept {
    if (Threads.empty()) {
      Threads.push_back(nullptr); // slot 0 stays dead
    }
    if (!ThreadFree.empty()) {
      const uint32_t Idx = ThreadFree.back();
      ThreadFree.pop_back();
      Threads[Idx] = T;
      return Idx;
    }
    Threads.push_back(T);
    return static_cast<uint32_t>(Threads.size() - 1);
  }
  void threadRemove(uint32_t Idx) noexcept {
    if (Idx != 0 && Idx < Threads.size()) {
      Threads[Idx] = nullptr;
      ThreadFree.push_back(Idx);
    }
  }
  Runtime::Component::ThreadContext *threadGet(uint32_t Idx) noexcept {
    return Idx != 0 && Idx < Threads.size() ? Threads[Idx] : nullptr;
  }

  /// True while the instance is executing or still instantiating.
  bool entered() const noexcept { return Entered; }

  /// Sets the entered flag for the scope and restores the previous value on
  /// exit, so no error path can leak it. Instantiation enters the instance
  /// under construction, and its start function leaves it again.
  class EnteredGuard {
  public:
    EnteredGuard(ConcurrencyManager &S, bool V) noexcept
        : St(S), Saved(S.Entered) {
      St.Entered = V;
    }
    ~EnteredGuard() noexcept { St.Entered = Saved; }
    EnteredGuard(const EnteredGuard &) = delete;
    EnteredGuard &operator=(const EnteredGuard &) = delete;

  private:
    ConcurrencyManager &St;
    bool Saved;
  };

private:
  /// \name Data of concurrency manager.
  /// @{
  std::function<void()> OnDestroy;
  bool MayLeave = true;
  int64_t Backpressure = 0;
  uint32_t NumWaitingToEnter = 0;
  Runtime::Component::Task *ExclusiveTask = nullptr;
  bool Poisoned = false;
  bool Entered = false;
  std::vector<Runtime::Component::ThreadContext *> Threads;
  std::vector<uint32_t> ThreadFree;
  /// @}
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
