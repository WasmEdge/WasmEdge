// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/hostreactor.h - Host Reactor definition ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Host Reactor: the
/// wake-up source of the host operations in flight, which the task manager
/// blocks on when no task thread is ready.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// Deadlines of sleeping host tasks plus notifications from host threads. A
/// host task parks with a ready predicate as a guest would; the reactor only
/// tells the scheduler when that predicate may have changed.
class HostReactor {
public:
  using Clock = std::chrono::steady_clock;

  /// Counter that every notification advances. Read it before scanning the
  /// parked threads and hand it to wait(), so a notification in between is
  /// not lost.
  uint64_t getGeneration() const noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mutex);
      return Generation;
    } catch (...) {
      return 0;
    }
  }

  /// Register a wake-up at Deadline; the token unregisters it.
  uint64_t addDeadline(Clock::time_point Deadline) noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mutex);
      const uint64_t Token = NextToken;
      NextToken += 1;
      Deadlines.emplace(Token, Deadline);
      Generation += 1;
      Cond.notify_all();
      return Token;
    } catch (...) {
      return 0;
    }
  }
  void removeDeadline(uint64_t Token) noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mutex);
      Deadlines.erase(Token);
    } catch (...) {
    }
  }

  /// A host operation that will call notify() from its own thread.
  void addWaiter() noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mutex);
      Waiters += 1;
    } catch (...) {
    }
  }
  void removeWaiter() noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mutex);
      if (Waiters > 0) {
        Waiters -= 1;
      }
    } catch (...) {
    }
  }

  /// Wake the scheduler; callable from any thread.
  void notify() noexcept {
    try {
      std::lock_guard<std::mutex> Lock(Mutex);
      Generation += 1;
      Cond.notify_all();
    } catch (...) {
    }
  }

  /// Block until a notification after Seen or until the earliest deadline.
  /// False when nothing is registered, so waiting could never end.
  bool wait(uint64_t Seen) noexcept {
    try {
      std::unique_lock<std::mutex> Lock(Mutex);
      if (Generation != Seen) {
        return true;
      }
      if (Deadlines.empty() && Waiters == 0) {
        return false;
      }
      auto Moved = [this, Seen]() { return Generation != Seen; };
      if (Deadlines.empty()) {
        Cond.wait(Lock, Moved);
        return true;
      }
      Clock::time_point Earliest = Deadlines.begin()->second;
      for (const auto &Entry : Deadlines) {
        Earliest = std::min(Earliest, Entry.second);
      }
      Cond.wait_until(Lock, Earliest, Moved);
      return true;
    } catch (...) {
      return false;
    }
  }

private:
  /// \name Data of host reactor.
  /// @{
  mutable std::mutex Mutex;
  std::condition_variable Cond;
  uint64_t Generation = 0;
  uint64_t NextToken = 1;
  std::map<uint64_t, Clock::time_point> Deadlines;
  uint32_t Waiters = 0;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
