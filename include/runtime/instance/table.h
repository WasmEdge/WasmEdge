// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/table.h - Table Instance definition ---------===//
//
// Part of the SSVM Project.
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
#include "common/log.h"
#include "common/span.h"
#include "common/types.h"
#include "common/value.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class TableInstance {
public:
  TableInstance() = delete;
  TableInstance(const RefType &Ref, const AST::Limit &Lim)
      : Type(Ref), HasMaxSize(Lim.hasMax()), MaxSize(Lim.getMax()),
        Refs(Lim.getMin(), genNullRef(Ref)) {}
  virtual ~TableInstance() = default;

  /// Getter of reference type.
  RefType getReferenceType() const noexcept { return Type; }

  /// Get size of table.refs
  uint32_t getSize() const noexcept { return Refs.size(); }

  /// Getter of limit definition.
  bool getHasMax() const noexcept { return HasMaxSize; }

  /// Getter of limit definition.
  uint32_t getMin() const noexcept { return Refs.size(); }

  /// Getter of limit definition.
  uint32_t getMax() const noexcept { return MaxSize; }

  /// Check is out of bound.
  bool checkAccessBound(uint32_t Offset, uint32_t Length) const noexcept {
    const uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    return AccessLen <= Refs.size();
  }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return ((Refs.size() > 0) ? (Refs.size() - 1) : 0);
  }

  /// Grow table with initialization value.
  bool growTable(const uint32_t Count, const RefVariant Val) {
    uint32_t MaxSizeCaped = std::numeric_limits<uint32_t>::max();
    if (HasMaxSize) {
      MaxSizeCaped = std::min(MaxSize, MaxSizeCaped);
    }
    if (Count > MaxSizeCaped - Refs.size()) {
      return false;
    }
    Refs.resize(Refs.size() + Count);
    std::fill_n(Refs.end() - Count, Count, Val);
    return true;
  }

  /// Get slice of Refs[Offset : Offset + Length - 1]
  Expect<Span<const RefVariant>> getRefs(const uint32_t Offset,
                                         const uint32_t Length) const noexcept {
    /// Check access boundary.
    if (!checkAccessBound(Offset, Length)) {
      LOG(ERROR) << ErrCode::TableOutOfBounds;
      LOG(ERROR) << ErrInfo::InfoBoundary(Offset, Length, getBoundIdx());
      return Unexpect(ErrCode::TableOutOfBounds);
    }
    return Span<const RefVariant>(Refs.begin() + Offset, Length);
  }

  /// Replace the Refs[Offset :] by Slice[Start : Start + Legnth - 1]
  Expect<void> setRefs(Span<const RefVariant> Slice, const uint32_t Offset,
                       const uint32_t Start, const uint32_t Length) {
    /// Check access boundary.
    if (!checkAccessBound(Offset, Length)) {
      LOG(ERROR) << ErrCode::TableOutOfBounds;
      LOG(ERROR) << ErrInfo::InfoBoundary(Offset, Length, getBoundIdx());
      return Unexpect(ErrCode::TableOutOfBounds);
    }

    /// Check input data validation.
    if (Start + Length > Slice.size()) {
      LOG(ERROR) << ErrCode::TableOutOfBounds;
      LOG(ERROR) << ErrInfo::InfoBoundary(Start, Length, Slice.size() - 1);
      return Unexpect(ErrCode::TableOutOfBounds);
    }

    /// Copy references.
    std::copy_n(Slice.begin() + Start, Length, Refs.begin() + Offset);
    return {};
  }

  /// Fill the Refs[Offset : Offset + Length - 1] by Val.
  Expect<void> fillRefs(const RefVariant Val, const uint32_t Offset,
                        const uint32_t Length) {
    /// Check access boundary.
    if (!checkAccessBound(Offset, Length)) {
      LOG(ERROR) << ErrCode::TableOutOfBounds;
      LOG(ERROR) << ErrInfo::InfoBoundary(Offset, Length, getBoundIdx());
      return Unexpect(ErrCode::TableOutOfBounds);
    }

    /// Fill references.
    std::fill_n(Refs.begin() + Offset, Length, Val);
    return {};
  }

  /// Get the elem address.
  Expect<RefVariant> getRefAddr(const uint32_t Idx) const noexcept {
    if (Idx >= Refs.size()) {
      LOG(ERROR) << ErrCode::TableOutOfBounds;
      LOG(ERROR) << ErrInfo::InfoBoundary(Idx, 1, getBoundIdx());
      return Unexpect(ErrCode::TableOutOfBounds);
    }
    return Refs[Idx];
  }

  /// Set the elem address.
  Expect<void> setRefAddr(const uint32_t Idx, const RefVariant Val) {
    if (Idx >= Refs.size()) {
      LOG(ERROR) << ErrCode::TableOutOfBounds;
      LOG(ERROR) << ErrInfo::InfoBoundary(Idx, 1, getBoundIdx());
      return Unexpect(ErrCode::TableOutOfBounds);
    }
    Refs[Idx] = Val;
    return {};
  }

private:
  /// \name Data of table instance.
  /// @{
  const RefType Type;
  const bool HasMaxSize;
  const uint32_t MaxSize;
  std::vector<RefVariant> Refs;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
