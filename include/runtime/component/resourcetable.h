// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/resourcetable.h - Resource Table -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host-side table of one resource type: the
/// representation a handle carries is a slot of this table, which shares the
/// ownership of the host object behind it.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// Representations are one-based slot numbers, so a zero never names an
/// object. A freed slot is reused last-in first-out.
template <typename T> class ResourceTable {
public:
  uint64_t add(std::shared_ptr<T> Obj) noexcept {
    if (!Free.empty()) {
      const uint64_t Slot = Free.back();
      Free.pop_back();
      Slots[Slot] = std::move(Obj);
      return Slot + 1;
    }
    Slots.push_back(std::move(Obj));
    return static_cast<uint64_t>(Slots.size());
  }

  std::shared_ptr<T> get(uint64_t Rep) const noexcept {
    if (Rep == 0 || Rep > Slots.size()) {
      return nullptr;
    }
    return Slots[Rep - 1];
  }

  std::shared_ptr<T> remove(uint64_t Rep) noexcept {
    auto Out = get(Rep);
    if (Out) {
      Slots[Rep - 1].reset();
      Free.push_back(Rep - 1);
    }
    return Out;
  }

  /// Live objects, for teardown.
  std::size_t size() const noexcept { return Slots.size() - Free.size(); }

private:
  std::vector<std::shared_ptr<T>> Slots;
  std::vector<uint64_t> Free;
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
