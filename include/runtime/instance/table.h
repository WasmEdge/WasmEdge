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
#include "gc/allocator.h"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>
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
        InitValue(RefVariant(TType.getRefType())),
        LiveSize(TType.getLimit().getMin()) {
    // The reference type should be nullable because there is no initial ref.
    // This constructor only handles abstract heap types correctly for null
    // refs. For concrete type indices, the caller should use the two-arg
    // constructor with a properly initialized RefVariant.
    assuming(TType.getRefType().isNullableRefType());
    assuming(TType.getRefType().isAbsHeapType());
    DataPtr = Refs.data();
  }
  TableInstance(const AST::TableType &TType, const RefVariant &InitVal) noexcept
      : TabType(TType), Refs(TType.getLimit().getMin(), InitVal),
        InitValue(InitVal), LiveSize(TType.getLimit().getMin()) {
    // If the reference type is not nullable, the initial reference is required.
    assuming(TType.getRefType().isNullableRefType() || !InitVal.isNull());
    DataPtr = Refs.data();
  }

  ~TableInstance() noexcept {
    if (Allocator) {
      Allocator->removeTable(*this);
    }
  }

  // Rooted by address: a copy/move would leave the new object unregistered and
  // its destructor's removeTable pointing at an address never stored.
  TableInstance(const TableInstance &) = delete;
  TableInstance(TableInstance &&) = delete;
  TableInstance &operator=(const TableInstance &) = delete;
  TableInstance &operator=(TableInstance &&) = delete;

  // Attach the GC allocator that scans this table's roots and runs its write
  // barriers. Attach-once: first allocator wins, so a regularly instantiated
  // table must not re-register, and a table shared across two executors keeps
  // the first -- the second's refs go unscanned (cross-executor GC limitation).
  // A host table is rooted only once imported; if created with a GC ref init
  // value, that ref must be kept reachable by the host until then.
  void setAllocator(GC::Allocator &A) noexcept {
    if (Allocator != nullptr) {
      return;
    }
    Allocator = &A;
    Allocator->addTable(*this);
  }

  /// True if attached to an allocator other than A. Import uses this to reject
  /// sharing a GC ref-typed table across executors (see setAllocator); interim
  /// guard until cross-allocator roots exist.
  bool hasForeignAllocator(const GC::Allocator &A) const noexcept {
    return Allocator != nullptr && Allocator != &A;
  }

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
    // growTable is noexcept, so an over-large resize() throw would terminate
    // (guest DoS via huge table.grow on a no-max table; the uint64 sum could
    // also wrap into resize on 32-bit hosts). Reject past max_size and treat an
    // allocation failure as a failed grow.
    if (Count > Refs.max_size() - Refs.size()) {
      return false;
    }
    // Refs is a GC root set; resize() may reallocate and free the old buffer,
    // so hold the heap lock across resize/fill so a concurrent root scan never
    // iterates freed storage. In-place slot writes need no such lock.
    std::optional<std::unique_lock<std::mutex>> RootLock;
    if (Allocator) {
      RootLock = Allocator->lockTableRoots();
    }
    // Guard the try/catch so this header compiles in -fno-exceptions TUs (e.g.
    // lib/llvm includes it transitively; growTable is unreached there). All
    // three macros are needed: MSVC sets only _CPPUNWIND, so guarding on
    // __EXCEPTIONS alone would drop the catch and terminate on a guest grow.
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try {
      Refs.resize(Refs.size() + Count);
    } catch (...) {
      return false;
    }
#else
    Refs.resize(Refs.size() + Count);
#endif
    std::fill_n(Refs.end() - static_cast<std::ptrdiff_t>(Count), Count, Val);
    // New slots join the roots after the root snapshot; shade the broadcast
    // reference so a concurrent collection keeps it alive.
    if (Allocator) {
      Allocator->writeBarrier(Val);
    }
    DataPtr = Refs.data();
    setLiveSize(Min + Count);
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
    if (Length == 0) {
      // Empty result (e.g. zero-length table.copy/init read). Forming
      // Refs.data() + Offset is well-defined in C++17 but UBSan's
      // pointer-overflow check flags it, so return an empty span instead.
      return Span<const RefVariant>{};
    }
    return Span<const RefVariant>(Refs).subspan(Offset, Length);
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
      // Table slots are GC roots; shade the overwritten and the newly stored
      // refs so a concurrent collection does not reclaim a still-reachable
      // object. Shade before the copy: afterwards the old refs are gone.
      if (Allocator) {
        Allocator->bulkWriteBarrier(
            Span<const RefVariant>(Refs.data() + Dst, Length));
        Allocator->bulkWriteBarrier(
            Span<const RefVariant>(Slice.data() + Src, Length));
      }
      std::memmove(Refs.data() + Dst, Slice.data() + Src,
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
    if (Length == 0) {
      // No slots to fill; skip the fill_n below (UBSan's pointer-overflow check
      // flags Refs.begin() + Offset though it is well-defined in C++17).
      return {};
    }

    // Table slots are GC roots; shade the overwritten range and the fill value
    // so a concurrent collection does not miss a reachable object.
    if (Allocator) {
      Allocator->bulkWriteBarrier(
          Span<const RefVariant>(Refs).subspan(Offset, Length));
      Allocator->writeBarrier(Val);
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
  Expect<void> setRefAddr(const uint64_t Idx, const RefVariant &Val) noexcept {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    // Table slots are GC roots; shade the overwritten and newly stored ref so
    // a concurrent collection does not miss a reachable object.
    if (Allocator) {
      Allocator->writeBarrier(Refs[Idx]);
      Allocator->writeBarrier(Val);
    }
    Refs[Idx] = Val;
    return {};
  }

private:
  friend class GC::Allocator;
  /// Detach this table from the allocator during allocator teardown.
  ///
  /// The Allocator pointer is read unsynchronized on the destructor and barrier
  /// paths, so table and allocator teardown must be single-threaded w.r.t. each
  /// other: either ~TableInstance removes the registration before ~Allocator,
  /// or ~Allocator calls clearAllocator() under its heap lock first.
  void clearAllocator(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      Allocator = nullptr;
    }
  }

  /// Update the size in the limit and its live mirror synchronously.
  void setLiveSize(const uint64_t Size) noexcept {
    TabType.getLimit().setMin(Size);
    LiveSize = Size;
  }

  /// \name Data of table instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  AST::TableType TabType;
  std::vector<RefVariant> Refs;
  RefVariant InitValue;
  RefVariant *DataPtr = nullptr;
  uint64_t LiveSize;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
