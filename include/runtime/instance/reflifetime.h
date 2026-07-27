// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/runtime/instance/reflifetime.h - RefLifetime def ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines RefLifetime, the intrusive lifetime counter used to decide
/// when a heap-allocated, dependency-shared instance may delete itself.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

/// Intrusive lifetime counter for a heap object shared by importers.
///
/// Packs an owner flag and a dependent count into one atomic so the deletion
/// decision is a single read-modify-write. Defaults to owner-held with zero
/// dependents, so an object that never calls releaseOwner() is never
/// self-deleted.
class RefLifetime {
public:
  /// Pin this object for one importer. The caller must keep the object alive
  /// across the call (via an existing pin or a store lock spanning lookup and
  /// pin).
  void addDependent() noexcept {
    Bits.fetch_add(DependentUnit, std::memory_order_relaxed);
  }

  /// Release one importer pin. Returns true iff the caller must delete the
  /// object now (last dependent gone after the owner already released). An
  /// over-release trips assuming() in debug and is a no-op in release.
  [[nodiscard]] bool releaseDependent() noexcept {
    uint64_t Prev = Bits.load(std::memory_order_acquire);
    while (true) {
      assuming((Prev & CountMask) != 0U);
      if ((Prev & CountMask) == 0U) {
        return false;
      }
      if (Bits.compare_exchange_weak(Prev, Prev - DependentUnit,
                                     std::memory_order_acq_rel,
                                     std::memory_order_relaxed)) {
        return Prev == DependentUnit;
      }
    }
  }

  /// Relinquish external ownership, opting a heap instance into deferred
  /// deletion. Idempotent: a second call is a no-op and never double-deletes.
  /// Returns true iff the caller must delete now, i.e. no dependents remain.
  [[nodiscard]] bool releaseOwner() noexcept {
    const uint64_t Prev = Bits.fetch_and(~OwnerBit, std::memory_order_acq_rel);
    return Prev == OwnerBit;
  }

  /// True iff at least one importer still pins this object.
  [[nodiscard]] bool hasDependents() const noexcept {
    return (Bits.load(std::memory_order_acquire) & CountMask) != 0U;
  }

private:
  static constexpr uint64_t OwnerBit = UINT64_C(1) << 32;
  static constexpr uint64_t CountMask = OwnerBit - 1U;
  static constexpr uint64_t DependentUnit = UINT64_C(1);

  std::atomic<uint64_t> Bits{OwnerBit};
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
