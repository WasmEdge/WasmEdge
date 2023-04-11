// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include "ast/type.h"
#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/log.h"

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
      : TabType(TType), Refs(TType.getLimit().getMin(), UnknownRef()) {}

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
  bool growTable(uint32_t Count, RefVariant Val) noexcept {
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
    return growTable(Count, UnknownRef());
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
    return Span<const RefVariant>(Refs.begin() + Offset, Length);
  }

  /// Replace the Refs[Offset :] by Slice[Start : Start + Length - 1]
  Expect<void> setRefs(Span<const RefVariant> Slice, uint32_t Offset,
                       uint32_t Start, uint32_t Length) noexcept {
    // Check the accessing boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    // Check the input data validation.
    if (Start + Length > Slice.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(
          Start, Length,
          std::max(static_cast<uint32_t>(Slice.size()), UINT32_C(1)) -
              UINT32_C(1)));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }

    // Copy the references.
    std::copy_n(Slice.begin() + Start, Length, Refs.begin() + Offset);
    return {};
  }

  /// Fill the Refs[Offset : Offset + Length - 1] by Val.
  Expect<void> fillRefs(const RefVariant Val, uint32_t Offset,
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
  Expect<void> setRefAddr(uint32_t Idx, RefVariant Val) {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getBoundIdx()));
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
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
