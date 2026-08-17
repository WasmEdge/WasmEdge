// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/state.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The mutable runtime state of a component instance: the unified handles
/// table and the concurrency state.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/component/async.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {
class Task;
struct ThreadCtx;
} // namespace Component
} // namespace Executor

namespace Runtime {
namespace Instance {
namespace Component {

struct ResourceTypeRT;

/// The canonical `handles` table of one component instance. Every handle kind
/// shares one index space, which is what the specification defines.
class HandleTable {
public:
  // A resource entry of the canonical `handles` table.
  struct HandleSlot {
    const ResourceTypeRT *RT = nullptr;
    uint64_t Rep = 0;
    bool Own = true;
    uint32_t Lends = 0;
    // The task that received this borrow, for its borrow accounting. It is
    // null for own and host-boundary handles.
    Executor::Component::Task *BorrowScope = nullptr;
  };

  // One slot of the unified `handles` table, where every handle kind shares
  // an index space.
  using TableSlot =
      std::variant<std::monostate, HandleSlot, std::shared_ptr<WaitableObj>,
                   std::unique_ptr<WaitableSetObj>, std::string>;

  // Unified canonical `handles` table: index 0 unused, LIFO slot reuse. It
  // mutates through the const instance pointers of the canonical contexts.
  uint32_t slotAdd(TableSlot &&Slot) noexcept {
    if (Handles.empty()) {
      Handles.emplace_back(); // slot 0 stays dead
    }
    if (!FreeSlots.empty()) {
      const uint32_t Idx = FreeSlots.back();
      FreeSlots.pop_back();
      Handles[Idx] = std::move(Slot);
      return Idx;
    }
    Handles.push_back(std::move(Slot));
    return static_cast<uint32_t>(Handles.size() - 1);
  }
  bool slotLive(uint32_t Idx) noexcept {
    return Idx != 0 && Idx < Handles.size() &&
           !std::holds_alternative<std::monostate>(Handles[Idx]);
  }
  void slotFree(uint32_t Idx) noexcept {
    Handles[Idx] = std::monostate{};
    FreeSlots.push_back(Idx);
  }

  // Resource entries.
  uint32_t
  handleAdd(const ResourceTypeRT *RT, uint64_t Rep, bool Own,
            Executor::Component::Task *BorrowScope = nullptr) noexcept {
    return slotAdd(TableSlot{HandleSlot{RT, Rep, Own, 0, BorrowScope}});
  }
  HandleSlot *handleGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<HandleSlot>(&Handles[Idx]);
  }
  std::optional<HandleSlot> handleRemove(uint32_t Idx) noexcept {
    auto *Slot = handleGet(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    HandleSlot Out = *Slot;
    slotFree(Idx);
    return Out;
  }

  // Waitable entries (subtasks and stream/future copy ends).
  uint32_t waitableAdd(std::shared_ptr<WaitableObj> W) noexcept {
    return slotAdd(TableSlot{std::move(W)});
  }
  WaitableObj *waitableGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::shared_ptr<WaitableObj>>(&Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::shared_ptr<WaitableObj> waitableRemove(uint32_t Idx) noexcept {
    if (auto *W = std::get_if<std::shared_ptr<WaitableObj>>(
            slotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      slotFree(Idx);
      return Out;
    }
    return nullptr;
  }

  // Waitable-set entries.
  uint32_t waitableSetAdd() noexcept {
    return slotAdd(TableSlot{std::make_unique<WaitableSetObj>()});
  }
  WaitableSetObj *waitableSetGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::unique_ptr<WaitableSetObj>>(&Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::unique_ptr<WaitableSetObj> waitableSetRemove(uint32_t Idx) noexcept {
    if (auto *W = std::get_if<std::unique_ptr<WaitableSetObj>>(
            slotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      slotFree(Idx);
      return Out;
    }
    return nullptr;
  }

  // Error-context entries.
  uint32_t errorContextAdd(std::string Msg) noexcept {
    return slotAdd(TableSlot{std::move(Msg)});
  }
  std::string *errorContextGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::string>(&Handles[Idx]);
  }
  bool errorContextRemove(uint32_t Idx) noexcept {
    if (errorContextGet(Idx) == nullptr) {
      return false;
    }
    slotFree(Idx);
    return true;
  }

private:
  // Index 0 stays dead, and a freed slot returns through the LIFO list.
  std::vector<TableSlot> Handles;
  std::vector<uint32_t> FreeSlots;
};

/// The concurrency state of one component instance. MayLeave marks the
/// no-leave regions, and Poisoned marks a trapped instance tree.
class ConcurrencyState {
public:
  /// Installed by the executor. A thread belongs to the instance rather than
  /// to the call that spawned it, so the instance aborts and joins its
  /// threads when it goes away.
  std::function<void()> OnDestroy;
  bool MayLeave = true;
  int64_t Backpressure = 0;
  uint32_t NumWaitingToEnter = 0;
  Executor::Component::Task *ExclusiveTask = nullptr;
  bool Poisoned = false;
  std::vector<Executor::Component::ThreadCtx *> Threads;
  std::vector<uint32_t> ThreadFree;

  // The per-instance thread table: every thread activation registers here,
  // and thread.index reads it.
  uint32_t threadAdd(Executor::Component::ThreadCtx *T) noexcept {
    auto &St = *this;
    if (St.Threads.empty()) {
      St.Threads.push_back(nullptr); // slot 0 stays dead
    }
    if (!St.ThreadFree.empty()) {
      const uint32_t Idx = St.ThreadFree.back();
      St.ThreadFree.pop_back();
      St.Threads[Idx] = T;
      return Idx;
    }
    St.Threads.push_back(T);
    return static_cast<uint32_t>(St.Threads.size() - 1);
  }
  void threadRemove(uint32_t Idx) noexcept {
    if (Idx != 0 && Idx < Threads.size()) {
      Threads[Idx] = nullptr;
      ThreadFree.push_back(Idx);
    }
  }
  Executor::Component::ThreadCtx *threadGet(uint32_t Idx) noexcept {
    return Idx != 0 && Idx < Threads.size() ? Threads[Idx] : nullptr;
  }

  /// True while the instance is executing or still instantiating.
  bool entered() const noexcept { return Entered; }

  /// Marks the instance as entered for the scope. It replaces every
  /// hand-managed set and clear pair, so no error path can leak the flag.
  class EnterGuard {
  public:
    explicit EnterGuard(ConcurrencyState &S) noexcept : St(S) {
      St.Entered = true;
    }
    ~EnterGuard() noexcept { St.Entered = false; }
    EnterGuard(const EnterGuard &) = delete;
    EnterGuard &operator=(const EnterGuard &) = delete;

  private:
    ConcurrencyState &St;
  };

  /// Clears the flag for the scope. A start function is part of its own
  /// instantiation, so it enters the instance under construction.
  class LeaveGuard {
  public:
    explicit LeaveGuard(ConcurrencyState &S) noexcept : St(S) {
      St.Entered = false;
    }
    ~LeaveGuard() noexcept { St.Entered = true; }
    LeaveGuard(const LeaveGuard &) = delete;
    LeaveGuard &operator=(const LeaveGuard &) = delete;

  private:
    ConcurrencyState &St;
  };

private:
  bool Entered = false;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
