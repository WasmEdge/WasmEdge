// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/table.h - Table Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the table instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/segment.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "gc/allocator.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class TableInstance {
public:
  TableInstance() = delete;
  TableInstance(const AST::TableType &TType) noexcept
      : TabType(TType), InitValue(RefVariant(TType.getRefType())),
        Refs(TType.getLimit().getMin(), RefVariant(TType.getRefType())) {
    // The reftype should a nullable reference because of no init ref.
    assuming(TType.getRefType().isNullableRefType());
  }
  TableInstance(const AST::TableType &TType, const RefVariant &InitVal) noexcept
      : TabType(TType), InitValue(InitVal),
        Refs(TType.getLimit().getMin(), InitVal) {
    // If the reftype is not a nullable reference, the init ref is required.
    assuming(TType.getRefType().isNullableRefType() || !InitVal.isNull());
  }

  ~TableInstance() noexcept {
    if (Allocator) {
      Allocator->removeTable(*this);
    }
  }

  void setAllocator(GC::Allocator &A) noexcept {
    assuming(Allocator == nullptr);
    Allocator = &A;
    Allocator->addTable(*this);
  }

  /// Get size of table.refs
  uint32_t getSize() const noexcept {
    // The table size is binded with the limit in table type.
    return TabType.getLimit().getMin();
  }

  /// Getter of table type.
  const AST::TableType &getTableType() const noexcept { return TabType; }

  /// Check is out of bound.
  bool checkAccessBound(uint32_t Offset, uint32_t Length) const noexcept {
    const uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    return AccessLen <= Refs.size();
  }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return std::max(static_cast<uint32_t>(Refs.size()), UINT32_C(1)) -
           UINT32_C(1);
  }

  /// Grow table with initialization value.
  bool growTable(uint32_t Count, const RefVariant &Val) noexcept {
    uint32_t MaxSizeCaped = std::numeric_limits<uint32_t>::max();
    uint32_t Min = TabType.getLimit().getMin();
    uint32_t Max = TabType.getLimit().getMax();
    if (TabType.getLimit().hasMax()) {
      MaxSizeCaped = std::min(Max, MaxSizeCaped);
    }
    if (Count > MaxSizeCaped - Refs.size()) {
      return false;
    }
    Refs.resize(Refs.size() + Count);
    std::fill_n(Refs.end() - Count, Count, Val);
    TabType.getLimit().setMin(Min + Count);
    return true;
  }
  bool growTable(uint32_t Count) noexcept {
    return growTable(Count, InitValue);
  }

  /// Get slice of Refs[Offset : Offset + Length - 1]
  Expect<Span<const RefVariant>> getRefs(uint32_t Offset,
                                         uint32_t Length) const noexcept {
    // Check the accessing boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    return Span<const RefVariant>(Refs).subspan(Offset, Length);
  }

  /// Replace the Refs[Dst : Dst + Slice.size()] by Slice
  Expect<void> setRefs(Span<const RefVariant> Slice, uint32_t Dst) noexcept {
    const uint32_t Length = static_cast<uint32_t>(Slice.size());
    // Check the accessing boundary.
    if (!checkAccessBound(Dst, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Dst, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    auto Write = Span<RefVariant>(Refs).subspan(Dst, Slice.size());

    // Copy the references.
    if (Write.begin() <= Slice.begin() || Write.begin() >= Slice.end()) {
      std::copy(Slice.begin(), Slice.end(), Write.begin());
    } else {
      std::copy_backward(Slice.begin(), Slice.end(), Write.end());
    }
    return {};
  }

  /// Fill the Refs[Offset : Offset + Length - 1] by Val.
  Expect<void> fillRefs(const RefVariant &Val, uint32_t Offset,
                        uint32_t Length) noexcept {
    // Check the accessing boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    // Fill the references.
    std::fill_n(Refs.begin() + Offset, Length, Val);
    return {};
  }

  /// Get the elem address.
  Expect<RefVariant> getRefAddr(uint32_t Idx) const noexcept {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getBoundIdx()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    return Refs[Idx];
  }

  /// Set the elem address.
  Expect<void> setRefAddr(uint32_t Idx, const RefVariant &Val) {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getBoundIdx()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    Refs[Idx] = Val;
    return {};
  }

private:
  friend class GC::Allocator;
  void clearAllocator(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      Allocator = nullptr;
    }
  }

  /// \name Data of table instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  AST::TableType TabType;
  RefVariant InitValue;
  std::vector<RefVariant> Refs;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
