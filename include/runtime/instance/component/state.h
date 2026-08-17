// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/state.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The mutable runtime state of a component instance: the canonical handles
/// table and the concurrency state.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
namespace Component {

struct ResourceTypeRT;

/// The canonical `handles` table of one component instance.
class HandleTable {
public:
  // A resource entry of the canonical `handles` table.
  struct HandleSlot {
    const ResourceTypeRT *RT = nullptr;
    uint32_t Rep = 0;
    bool Own = true;
    uint32_t Lends = 0;
    bool Live = false;
  };

  // Index 0 stays dead, and a freed slot returns through the LIFO list. The
  // table mutates through the const instance pointers of the canonical
  // contexts.
  uint32_t handleAdd(const ResourceTypeRT *RT, uint32_t Rep,
                     bool Own) noexcept {
    if (Handles.empty()) {
      Handles.emplace_back(); // slot 0 stays dead
    }
    if (!FreeSlots.empty()) {
      const uint32_t Idx = FreeSlots.back();
      FreeSlots.pop_back();
      Handles[Idx] = {RT, Rep, Own, 0, true};
      return Idx;
    }
    Handles.push_back({RT, Rep, Own, 0, true});
    return static_cast<uint32_t>(Handles.size() - 1);
  }
  HandleSlot *handleGet(uint32_t Idx) noexcept {
    if (Idx == 0 || Idx >= Handles.size() || !Handles[Idx].Live) {
      return nullptr;
    }
    return &Handles[Idx];
  }
  std::optional<HandleSlot> handleRemove(uint32_t Idx) noexcept {
    auto *Slot = handleGet(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    HandleSlot Out = *Slot;
    Slot->Live = false;
    FreeSlots.push_back(Idx);
    return Out;
  }

private:
  std::vector<HandleSlot> Handles;
  std::vector<uint32_t> FreeSlots;
};

/// The concurrency state of one component instance.
class ConcurrencyState {
public:
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
