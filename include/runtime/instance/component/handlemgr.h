// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/handlemgr.h - Handle Manager --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Handle Manager,
/// which the specification calls the canonical `handles` table of a component
/// instance.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/component/resource.h"
#include "runtime/instance/component/waitable.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace Runtime {

namespace Component {
class Task;
} // namespace Component

namespace Instance {
namespace Component {

/// The canonical `handles` table of one component instance. Every handle kind
/// shares one index space, which is what the specification defines.
class HandleManager {
public:
  /// A resource entry of the canonical `handles` table.
  struct ResourceHandle {
    const ResourceTypeInstance *RT = nullptr;
    uint64_t Rep = 0;
    bool Own = true;
    uint32_t Lends = 0;
    /// The task that received this borrow, for its borrow accounting. It is
    /// null for own and host-boundary handles.
    Runtime::Component::Task *BorrowScope = nullptr;
  };

  /// Resource entries.
  uint32_t handleAdd(const ResourceTypeInstance *RT, uint64_t Rep, bool Own,
                     Runtime::Component::Task *BorrowScope = nullptr) noexcept {
    return slotAdd(HandleSlot{ResourceHandle{RT, Rep, Own, 0, BorrowScope}});
  }
  ResourceHandle *handleGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<ResourceHandle>(&Handles[Idx]);
  }
  std::optional<ResourceHandle> handleRemove(uint32_t Idx) noexcept {
    auto *Slot = handleGet(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    ResourceHandle Out = *Slot;
    slotFree(Idx);
    return Out;
  }

  /// Waitable entries (subtasks and stream or future transmit ends).
  uint32_t waitableAdd(std::shared_ptr<WaitableBase> W) noexcept {
    return slotAdd(HandleSlot{std::move(W)});
  }
  WaitableBase *waitableGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::shared_ptr<WaitableBase>>(&Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::shared_ptr<WaitableBase> waitableRemove(uint32_t Idx) noexcept {
    if (auto *W = std::get_if<std::shared_ptr<WaitableBase>>(
            slotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      slotFree(Idx);
      return Out;
    }
    return nullptr;
  }

  /// Waitable-set entries.
  uint32_t waitableSetAdd() noexcept {
    return slotAdd(HandleSlot{std::make_unique<WaitableSet>()});
  }
  WaitableSet *waitableSetGet(uint32_t Idx) noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::unique_ptr<WaitableSet>>(&Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::unique_ptr<WaitableSet> waitableSetRemove(uint32_t Idx) noexcept {
    if (auto *W = std::get_if<std::unique_ptr<WaitableSet>>(
            slotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      slotFree(Idx);
      return Out;
    }
    return nullptr;
  }

  /// Error-context entries.
  uint32_t errorContextAdd(std::string Msg) noexcept {
    return slotAdd(HandleSlot{std::move(Msg)});
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
  /// One slot of the unified `handles` table, where every handle kind shares
  /// an index space.
  using HandleSlot = std::variant<std::monostate, ResourceHandle,
                                  std::shared_ptr<WaitableBase>,
                                  std::unique_ptr<WaitableSet>, std::string>;

  /// Unified canonical `handles` table: index 0 unused, LIFO slot reuse. It
  /// mutates through the const instance pointers of the canonical contexts.
  uint32_t slotAdd(HandleSlot &&Slot) noexcept {
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

  /// \name Data of handle manager.
  /// @{
  /// Index 0 stays dead, and a freed slot returns through the LIFO list.
  std::vector<HandleSlot> Handles;
  std::vector<uint32_t> FreeSlots;
  /// @}
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
