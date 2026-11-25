// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/typetable.h - Canonical type table ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the process-wide table of canonical defined types.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Runtime {

/// Append-only table that gives structurally equal rec groups equal ids.
class CanonicalTypeTable {
public:
  static constexpr uint32_t kDefaultCap = UINT32_C(1) << 24;
  /// Largest total field size of a struct type.
  static constexpr uint64_t kMaxFieldBytes = (UINT64_C(1) << 32) - 64;

  /// A table that holds at most `Cap` sub types. `Cap` is at most
  /// `kDefaultCap`. The constructor does not allocate.
  explicit CanonicalTypeTable(uint32_t Cap = kDefaultCap) noexcept;
  ~CanonicalTypeTable() noexcept;
  CanonicalTypeTable(const CanonicalTypeTable &) = delete;
  CanonicalTypeTable &operator=(const CanonicalTypeTable &) = delete;

  /// The table shared by the whole process. It is never destroyed.
  WASMEDGE_EXPORT static CanonicalTypeTable &getGlobal() noexcept;

  /// Use `Table` as the global table of this runtime copy. Fails if this copy
  /// already uses a different global table.
  WASMEDGE_EXPORT static bool attachGlobal(CanonicalTypeTable *Table) noexcept;

  /// Register a rec group and return the id of its first sub type.
  ///
  /// `Start` is the module type index of `Group[0]`, and `ModuleIds` holds the
  /// canonical ids of the module types before it. The ids of a group are
  /// consecutive.
  WASMEDGE_EXPORT Expect<uint32_t>
  registerRecGroup(Span<const AST::SubType *const> Group, uint32_t Start,
                   Span<const uint32_t> ModuleIds) noexcept;

  /// Number of registered sub types.
  uint32_t size() const noexcept {
    return Size.load(std::memory_order_acquire);
  }

  /// Get a registered sub type. Its type references are canonical ids.
  const AST::SubType &getType(uint32_t Id) const noexcept {
    assuming(Id < size());
    return Blocks[Id / kBlockSize][Id % kBlockSize];
  }

  /// Storage size in bytes of a field of `StorageType`.
  WASMEDGE_EXPORT static uint32_t
  getFieldSize(const ValType &StorageType) noexcept;

  /// Positions of the fields of the struct type `Id`, relative to the first
  /// field, then the total field size. Empty for other types.
  Span<const uint32_t> getFieldPositions(uint32_t Id) const noexcept {
    assuming(Id < size());
    return Layouts[Id / kBlockSize][Id % kBlockSize];
  }

  /// Check whether the type `Got` is `Exp` or one of its declared subtypes.
  bool isSubType(uint32_t Exp, uint32_t Got) const noexcept {
    while (Got != Exp) {
      const auto Supers = getType(Got).getSuperTypeIndices();
      if (Supers.empty()) {
        return false;
      }
      Got = Supers[0];
    }
    return true;
  }

private:
  static constexpr uint32_t kBlockSize = 4096;

  struct KeyHash {
    size_t operator()(const std::vector<uint64_t> &Key) const noexcept;
  };

  static constexpr uint32_t kMaxBlocks = kDefaultCap / kBlockSize;

  const uint32_t Cap;
  std::array<AST::SubType *, kMaxBlocks> Blocks = {};
  std::array<std::vector<uint32_t> *, kMaxBlocks> Layouts = {};
  std::atomic<uint32_t> Size = 0;
  std::mutex Mutex;
  /// Created on the first registration, because some standard libraries
  /// allocate in the constructor of an unordered map.
  std::optional<std::unordered_map<std::vector<uint64_t>, uint32_t, KeyHash>>
      Groups;
};

} // namespace Runtime
} // namespace WasmEdge
