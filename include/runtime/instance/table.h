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
      : TabType(TType),
        Refs(TType.getLimit().getMin(), RefVariant(TType.getRefType())),
        InitValue(RefVariant(TType.getRefType())) {
    // The reftype should a nullable reference because of no init ref.
    assuming(TType.getRefType().isNullableRefType());
  }
  TableInstance(const AST::TableType &TType, const RefVariant &InitVal) noexcept
      : TabType(TType), Refs(TType.getLimit().getMin(), InitVal),
        InitValue(InitVal) {
    // If the reftype is not a nullable reference, the init ref is required.
    assuming(TType.getRefType().isNullableRefType() || !InitVal.isNull());
  }

  /// Get size of table.refs
  uint64_t getSize() const noexcept {
    // The table size is binded with the limit in table type.
    return TabType.getLimit().getMin();
  }

  /// Getter of table type.
  const AST::TableType &getTableType() const noexcept { return TabType; }

  /// Check is out of bound.
  bool checkAccessBound(const uint64_t Offset,
                        const uint64_t Length) const noexcept {
    // Due to applying the Memory64 proposal, we should avoid the overflow issue
    // of the following code:
    //   return Offset + Length <= Limit;
    const uint64_t Limit = TabType.getLimit().getMin();
    return std::numeric_limits<uint64_t>::max() - Offset >= Length &&
           Offset + Length <= Limit;
  }

  /// Grow table with initialization value.
  bool growTable(const uint64_t Count, const RefVariant &Val) noexcept {
    if (Count == 0) {
      return true;
    }
    uint64_t MaxSizeCaped = getMaxAddress(TabType.getLimit().getAddrType());
    const uint64_t Min = TabType.getLimit().getMin();
    assuming(MaxSizeCaped >= Min);
    if (TabType.getLimit().hasMax()) {
      const uint64_t Max = TabType.getLimit().getMax();
      MaxSizeCaped = std::min(Max, MaxSizeCaped);
    }
    if (Count > MaxSizeCaped - Min) {
      return false;
    }
    Refs.resize(Refs.size() + Count);
    std::fill_n(Refs.end() - static_cast<std::ptrdiff_t>(Count), Count, Val);
    TabType.getLimit().setMin(Min + Count);
    return true;
  }
  bool growTable(const uint64_t Count) noexcept {
    return growTable(Count, InitValue);
  }

  /// Get slice of Refs[Offset : Offset + Length - 1]
  Expect<Span<const RefVariant>> getRefs(const uint64_t Offset,
                                         const uint64_t Length) const noexcept {
    // Check the accessing boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    return Span<const RefVariant>(
        Refs.begin() + static_cast<std::ptrdiff_t>(Offset), Length);
  }

  /// Replace the Refs[Dst :] by Slice[Src : Src + Length)
  Expect<void> setRefs(Span<const RefVariant> Slice, const uint64_t Dst,
                       const uint64_t Src, const uint64_t Length) noexcept {
    // Check the accessing boundary.
    if (!checkAccessBound(Dst, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Dst, Length, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    // Check the input data validation.
    if (std::numeric_limits<uint64_t>::max() - Src < Length ||
        Src + Length > Slice.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Src, Length, Slice.size()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    // Copy the references.
    if (Dst <= Src) {
      std::copy(Slice.begin() + Src, Slice.begin() + Src + Length,
                Refs.begin() + static_cast<std::ptrdiff_t>(Dst));
    } else {
      std::copy(std::make_reverse_iterator(Slice.begin() + Src + Length),
                std::make_reverse_iterator(Slice.begin() + Src),
                std::make_reverse_iterator(
                    Refs.begin() + static_cast<std::ptrdiff_t>(Dst + Length)));
    }
    return {};
  }

  /// Fill the Refs[Offset : Offset + Length - 1] by Val.
  Expect<void> fillRefs(const RefVariant &Val, const uint64_t Offset,
                        const uint64_t Length) noexcept {
    // Check the accessing boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    // Fill the references.
    std::fill_n(Refs.begin() + static_cast<std::ptrdiff_t>(Offset), Length,
                Val);
    return {};
  }

  /// Get the elem address.
  Expect<RefVariant> getRefAddr(const uint64_t Idx) const noexcept {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    return Refs[Idx];
  }

  /// Set the elem address.
  Expect<void> setRefAddr(const uint64_t Idx, const RefVariant &Val) {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    Refs[Idx] = Val;
    return {};
  }

private:
  /// \name Data of table instance.
  /// @{
  AST::TableType TabType;
  std::vector<RefVariant> Refs;
  RefVariant InitValue;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
