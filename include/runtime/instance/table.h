// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ModuleInstance;

class TableInstance {
public:
  TableInstance() = delete;
  TableInstance(const AST::TableType &TType) noexcept
      : TableInstance(TType, RefVariant(TType.getRefType())) {
    // The reference type should be nullable because there is no initial ref.
    // This constructor only handles abstract heap types correctly for null
    // refs. For concrete type indices, the caller should use the two-arg
    // constructor with a properly initialized RefVariant.
    assuming(TType.getRefType().isNullableRefType());
    assuming(TType.getRefType().isAbsHeapType());
  }
  /// The table size stays 0 if the allocation of the initial size failed.
  TableInstance(const AST::TableType &TType, const RefVariant &InitVal) noexcept
      : TabType(TType), InitValue(InitVal), LiveSize(0) {
    // If the reference type is not nullable, the initial reference is required.
    assuming(TType.getRefType().isNullableRefType() || !InitVal.isNull());
    TabType.getLimit().setMin(0);
    growRefs(TType.getLimit().getMin(), InitVal);
  }
  /// Constructor for a table instance defined by a module.
  TableInstance(const ModuleInstance *Mod, const AST::TableType &TType,
                const RefVariant &InitVal) noexcept
      : TableInstance(TType, InitVal) {
    ModInst = Mod;
  }
  TableInstance(const TableInstance &) = delete;
  TableInstance &operator=(const TableInstance &) = delete;
  ~TableInstance() noexcept { std::free(DataPtr); }

  /// Getter for the defining module instance, if any.
  const ModuleInstance *getModule() const noexcept { return ModInst; }

  /// Get size of table.refs
  uint64_t getSize() const noexcept {
    // The table size is bound to the limit in the table type.
    return TabType.getLimit().getMin();
  }

  /// Get a stable pointer to the live size field for compiled code.
  const uint64_t *getSizePtr() const noexcept { return &LiveSize; }

  /// Get the stable reference to the live element buffer for compiled code.
  RefVariant *const &getDataPtr() const noexcept { return DataPtr; }
  RefVariant *&getDataPtr() noexcept { return DataPtr; }

  /// Getter for table type.
  const AST::TableType &getTableType() const noexcept { return TabType; }

  /// Check whether access is out of bounds.
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
    return growRefs(Count, Val);
  }
  bool growTable(const uint64_t Count) noexcept {
    return growTable(Count, InitValue);
  }

  /// Shrink table.
  bool shrinkTable(const uint64_t Count) noexcept {
    if (Count > getSize()) {
      return false;
    }
    setLiveSize(getSize() - Count);
    return true;
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
    return Span<const RefVariant>(DataPtr + Offset, Length);
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

    // Copy the references. The slice may be from the same table instance, so
    // use memmove semantics for the possible overlapping case.
    if (likely(Length > 0)) {
      std::memmove(DataPtr + Dst, Slice.data() + Src,
                   Length * sizeof(RefVariant));
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
    std::fill_n(DataPtr + Offset, Length, Val);
    return {};
  }

  /// Get the elem address.
  Expect<RefVariant> getRefAddr(const uint64_t Idx) const noexcept {
    if (Idx >= getSize()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    return DataPtr[Idx];
  }

  /// Set the elem address.
  Expect<void> setRefAddr(const uint64_t Idx, const RefVariant &Val) {
    if (Idx >= getSize()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    DataPtr[Idx] = Val;
    return {};
  }

private:
  /// Append references. Fail without changes if the allocation failed.
  bool growRefs(const uint64_t Count, const RefVariant &Val) noexcept {
    static_assert(std::is_trivially_copyable_v<RefVariant>);
    const uint64_t MaxSize =
        std::numeric_limits<size_t>::max() / sizeof(RefVariant);
    const uint64_t Size = getSize();
    if (Count > MaxSize - Size) {
      return false;
    }
    const uint64_t NewSize = Size + Count;
    if (NewSize > Capacity) {
      // Grow geometrically, and fall back to the exact size.
      const uint64_t NewCap =
          std::max(NewSize, std::min(MaxSize, Capacity + Capacity / 2));
      if (!reserveRefs(NewCap) &&
          (NewCap == NewSize || !reserveRefs(NewSize))) {
        return false;
      }
    }
    std::uninitialized_fill_n(DataPtr + Size, Count, Val);
    setLiveSize(NewSize);
    return true;
  }

  /// Reallocate the buffer. Fail without changes if the allocation failed.
  bool reserveRefs(const uint64_t NewCap) noexcept {
    auto *NewPtr = static_cast<RefVariant *>(std::realloc(
        DataPtr, static_cast<size_t>(NewCap) * sizeof(RefVariant)));
    if (NewPtr == nullptr) {
      return false;
    }
    DataPtr = NewPtr;
    Capacity = NewCap;
    return true;
  }

  /// Update the size in the limit and its live mirror synchronously.
  void setLiveSize(const uint64_t Size) noexcept {
    TabType.getLimit().setMin(Size);
    LiveSize = Size;
  }

  /// \name Data of table instance.
  /// @{
  const ModuleInstance *ModInst = nullptr;
  AST::TableType TabType;
  RefVariant InitValue;
  RefVariant *DataPtr = nullptr;
  uint64_t Capacity = 0;
  uint64_t LiveSize;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
