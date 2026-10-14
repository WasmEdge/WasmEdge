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
#include "gc/controller.h"

#include <algorithm>
#include <atomic>
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
  // \p CanHoldManagedIn tells whether this table can hold managed GC objects.
  // The caller resolves it, because a concrete heap-type index is relative to
  // the defining module and the constructor has no type list.
  TableInstance(const AST::TableType &TType, bool CanHoldManagedIn) noexcept
      : TabType(TType),
        Refs(TType.getLimit().getMin(), RefVariant(TType.getRefType())),
        InitValue(RefVariant(TType.getRefType())),
        LiveSize(TType.getLimit().getMin()), CanHoldManaged(CanHoldManagedIn) {
    // The reference type should be nullable because there is no initial ref.
    // This constructor only handles abstract heap types correctly for null
    // refs. For concrete type indices, the caller should use the two-arg
    // constructor with a properly initialized RefVariant.
    assuming(TType.getRefType().isNullableRefType());
    assuming(TType.getRefType().isAbsHeapType());
    DataPtr.store(Refs.data(), std::memory_order_release);
  }
  TableInstance(const AST::TableType &TType, const RefVariant &InitVal,
                bool CanHoldManagedIn) noexcept
      : TabType(TType), Refs(TType.getLimit().getMin(), InitVal),
        InitValue(InitVal), LiveSize(TType.getLimit().getMin()),
        CanHoldManaged(CanHoldManagedIn) {
    // If the reference type is not nullable, the initial reference is required.
    assuming(TType.getRefType().isNullableRefType() || !InitVal.isNull());
    DataPtr.store(Refs.data(), std::memory_order_release);
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

  /// Attach the GC allocator that scans this table's roots and runs its write
  /// barriers. A same-owner attach is an idempotent success. When a different
  /// allocator already owns the table, the attach fails with
  /// IncompatibleImportType if the table is unsafe to share:
  ///   - canHoldManaged(): a ref that the other allocator stores here is not
  ///     visible to the owner's collector, which can sweep it while reachable.
  ///   - isGrowable(): a reallocating grow frees the buffer that a reader on
  ///     the other executor can still hold.
  /// Otherwise (a fixed, non-managed table) the attach succeeds and the first
  /// owner stays. AttachMutex serializes the claim of an unattached table. A
  /// host table is rooted only when it is imported, so the host must keep a GC
  /// ref init value reachable until then.
  Expect<void> setAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    return setAllocatorLocked(A);
  }

  /// Attach as setAllocator() does, for the registerModule ownership walk.
  /// Returns true when \p A owns the table after this call: the call took a
  /// hold (see AttachRefs), which the walk releases with detachAllocator() if
  /// the registration fails. Returns false when another allocator keeps a
  /// shareable table (no hold taken).
  Expect<bool> claimForRegister(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    EXPECTED_TRY(setAllocatorLocked(A));
    return Allocator == &A;
  }

  /// Release one claimForRegister() hold made by \p A. Only the last hold
  /// unregisters the table from \p A, so a failed registration never detaches
  /// a table that another registration or an instantiation uses. No-op if
  /// \p A is not the owner. Unlike clearAllocator(), this calls removeTable().
  void detachAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    if (Allocator != &A) {
      return;
    }
    assuming(AttachRefs > 0);
    if (--AttachRefs == 0) {
      Allocator->removeTable(*this);
      Allocator = nullptr;
    }
  }

  /// True if a GC controller owns this table and its element type can hold an
  /// internal GC-managed object. The C API mutators
  /// (WasmEdge_TableInstanceSetData / WasmEdge_TableInstanceGrow) reject such a
  /// table, because they cannot prove that the caller uses the owner's
  /// controller. externref tables are excluded even though they can wrap a GC
  /// object (extern.convert_any): they are the host-facing reference type and
  /// must stay mutable through the C API.
  bool isManagedByController() const noexcept {
    return Allocator != nullptr && Allocator->getController() != nullptr &&
           CanHoldManaged && !TabType.getRefType().isExternRefType();
  }

  /// True if attached to an allocator other than A. Import uses this to reject
  /// a table that another executor owns (see setAllocator for the rule).
  bool hasForeignAllocator(const GC::Allocator &A) const noexcept {
    return Allocator != nullptr && Allocator != &A;
  }

  /// Get size of table.refs
  uint64_t getSize() const noexcept {
    // The table size is bound to the limit in the table type.
    return TabType.getLimit().getMin();
  }

  /// Get a stable pointer to the live size field for compiled code.
  const uint64_t *getSizePtr() const noexcept {
    // atomic<uint64_t> is lock-free and layout-compatible with uint64_t; the
    // AOT reader loads this cell atomically (context.h getTableSize).
    return reinterpret_cast<const uint64_t *>(&LiveSize);
  }

  /// Get the stable reference to the live element buffer for compiled code.
  /// Address of the atomic base-pointer cell, threaded into the compiled
  /// ExecCtx (module.h TableRefPtrs) so generated code loads the current buffer
  /// base. `std::atomic<RefVariant*>` is lock-free and layout-compatible with
  /// `RefVariant*`, so the AOT reader treats the cell as a `RefVariant*` and
  /// loads it atomically (context.h getTable).
  RefVariant **getDataPtrAddr() noexcept {
    return reinterpret_cast<RefVariant **>(&DataPtr);
  }

  /// Getter for table type.
  const AST::TableType &getTableType() const noexcept { return TabType; }

  /// True if this table's element type can hold a GC-managed (struct/array)
  /// heap object. Resolved once at construction -- not recomputable from
  /// TabType alone, since a concrete heap-type index is defining-module-
  /// relative.
  bool canHoldManaged() const noexcept { return CanHoldManaged; }

  /// True if this table can grow (its buffer can be reallocated): no declared
  /// max, or a max above the current min.
  bool isGrowable() const noexcept {
    const AST::Limit &Lim = TabType.getLimit();
    return !Lim.hasMax() || Lim.getMax() > Lim.getMin();
  }

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
  ///
  /// When attached to a GC controller, a grow is an exclusive stop-the-world
  /// operation: the grower takes the controller's exclusive token (blocking,
  /// FIFO), so a grow and a collection never stop the world together, and
  /// parks every other running mutator while the Refs buffer is swapped. This
  /// does not park a host thread (a NativeRunning host function or a direct
  /// C API caller), so the embedder must not overlap such reads with a guest
  /// table.grow. A table without a controller uses the plain path.
  ///
  /// On success, if \p OldSize is non-null, it receives the pre-grow size read
  /// inside the exclusive window, which the callers push as the table.grow
  /// result.
  bool growTable(const uint64_t Count, const RefVariant &Val,
                 uint64_t *OldSize = nullptr) noexcept {
    if (Count == 0) {
      // No-op grow: report the current size and return without stopping the
      // world. The LiveSize atomic mirror is race-free against a concurrent
      // grower's setLiveSize store, so this needs neither the token nor a
      // handshake -- there is no buffer swap to exclude readers from.
      if (OldSize) {
        *OldSize = LiveSize.load(std::memory_order_acquire);
      }
      return true;
    }
    // Reject a grow that must fail before taking the token, so a guest loop of
    // oversized table.grow calls cannot force a stop-the-world each time. The
    // cap is constant and LiveSize only increases, so this never rejects a
    // grow that could succeed. growRefsLocked re-checks inside the window.
    {
      const uint64_t MaxSizeCaped = getMaxSizeCaped();
      const uint64_t Size = LiveSize.load(std::memory_order_acquire);
      if (Size > MaxSizeCaped || Count > MaxSizeCaped - Size) {
        return false;
      }
    }
    GC::Controller *const Ctrl =
        Allocator ? Allocator->getController() : nullptr;
    if (Ctrl == nullptr) {
      // Standalone table (no controller, so no registered mutators to park):
      // keep the plain path. Still take the table-root lock if attached to a
      // bare allocator so a concurrent root scan never iterates a freed buffer.
      std::optional<std::unique_lock<std::mutex>> RootLock;
      if (Allocator) {
        RootLock = Allocator->lockTableRoots();
      }
      return growRefsLocked(Count, Val, OldSize);
    }
    // Take the exclusive token (blocking). A false return means the controller
    // is closing, which is a grow failure.
    uint64_t Gen = 0;
    if (!Ctrl->beginExclusiveOp(GC::Controller::ExclusiveState::OwnedGrowing,
                                Gen)) {
      return false;
    }
    // Park every other running mutator, then swap the buffer under TableMutex.
    // growRefsLocked reads the current min, so it re-checks a limit that an
    // earlier grow can have raised.
    bool Result = false;
    Ctrl->growStopTheWorld(*Allocator, [&]() noexcept {
      Result = growRefsLocked(Count, Val, OldSize);
    });
    Ctrl->endExclusiveOp(Gen, GC::Controller::ExclusiveState::OwnedGrowing);
    return Result;
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
    // preserve memmove overlap semantics.
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
      // Per-element coherent copy: each slot may be read by the marker or a
      // concurrent coherent reader, so a bulk memmove would tear. Pick the
      // direction from the actual memory positions so an overlapping same-table
      // copy reads each source slot before it can be overwritten (memmove
      // semantics); for distinct tables there is no overlap.
      RefVariant *DstPtr = Refs.data() + Dst;
      const RefVariant *SrcPtr = Slice.data() + Src;
      if (DstPtr <= SrcPtr) {
        for (uint64_t I = 0; I < Length; ++I) {
          GC::storeCoherent(DstPtr[I], GC::loadCoherent(SrcPtr[I]));
        }
      } else {
        for (uint64_t I = Length; I-- > 0;) {
          GC::storeCoherent(DstPtr[I], GC::loadCoherent(SrcPtr[I]));
        }
      }
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

    // Per-element coherent store: each slot may be read by the marker or a
    // concurrent coherent reader, so a bulk fill_n (two independent word stores
    // per slot) would let them observe a torn pair.
    for (uint64_t I = 0; I < Length; ++I) {
      GC::storeCoherent(Refs[Offset + I], Val);
    }
    return {};
  }

  /// Get the elem address.
  Expect<RefVariant> getRefAddr(const uint64_t Idx) const noexcept {
    if (Idx >= Refs.size()) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Idx, 1, getSize()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    // Coherent read of the 128-bit ref element: a concurrent setRefAddr / bulk
    // store on another mutator can never hand back a torn (type, pointer) pair.
    return GC::loadCoherent(Refs[Idx]);
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
    // Publish the new (type, pointer) pair atomically so the marker's relaxed
    // pointer-word load and a concurrent coherent reader never observe a torn
    // slot.
    GC::storeCoherent(Refs[Idx], Val);
    return {};
  }

private:
  friend class GC::Allocator;

  /// The owner-attach rule of setAllocator(). The caller holds AttachMutex.
  Expect<void> setAllocatorLocked(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      // The same owner attaches again (e.g. a table re-imported into another
      // module of the same executor).
      ++AttachRefs;
      return {};
    }
    if (hasForeignAllocator(A)) {
      // A fixed, non-managed table is shareable and keeps its first owner.
      if (canHoldManaged() || isGrowable()) {
        return Unexpect(ErrCode::Value::IncompatibleImportType);
      }
      return {};
    }
    Allocator = &A;
    ++AttachRefs;
    Allocator->addTable(*this);
    return {};
  }

  /// The largest size the table can reach: the address-type ceiling, capped by
  /// the declared max. Constant after construction.
  uint64_t getMaxSizeCaped() const noexcept {
    uint64_t MaxSizeCaped = getMaxAddress(TabType.getLimit().getAddrType());
    if (TabType.getLimit().hasMax()) {
      MaxSizeCaped = std::min(TabType.getLimit().getMax(), MaxSizeCaped);
    }
    return MaxSizeCaped;
  }

  /// Detach this table from the allocator during allocator teardown.
  ///
  /// The Allocator pointer is read unsynchronized on the destructor and barrier
  /// paths, so table and allocator teardown must be single-threaded w.r.t. each
  /// other: either ~TableInstance removes the registration before ~Allocator,
  /// or ~Allocator calls clearAllocator() under its heap lock first.
  void clearAllocator(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      Allocator = nullptr;
      AttachRefs = 0;
    }
  }

  /// Perform the checked resize/retire/publish of the Refs buffer.
  ///
  /// The caller holds the table-root lock (Allocator::TableMutex) when
  /// Allocator != nullptr. Reads the current min, so a grow that waited for the
  /// token re-checks the limit. On success writes the pre-grow size to
  /// \p OldSize (if non-null). Returns false on an over-limit grow or a failed
  /// allocation, and leaves Refs unchanged.
  bool growRefsLocked(const uint64_t Count, const RefVariant &Val,
                      uint64_t *OldSize) noexcept {
    // growTable handles the no-op grow before ever calling this helper.
    assuming(Count != 0);
    const uint64_t Min = TabType.getLimit().getMin();
    assuming(getMaxAddress(TabType.getLimit().getAddrType()) >= Min);
    const uint64_t MaxSizeCaped = getMaxSizeCaped();
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
    // Guard the try/catch so this header compiles in -fno-exceptions TUs (e.g.
    // lib/llvm includes it transitively; growTable is unreached there). All
    // three macros are needed: MSVC sets only _CPPUNWIND, so guarding on
    // __EXCEPTIONS alone would drop the catch and terminate on a guest grow.
    const size_t NewSize = Refs.size() + static_cast<size_t>(Count);
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try {
#endif
      if (Allocator && NewSize > Refs.capacity()) {
        // A reallocating resize would free the old Refs buffer while a
        // concurrent mutator reader (AOT/interpreter table.get/set holding the
        // old DataPtr) is still using it -- a use-after-free. Build a fresh
        // buffer and RETIRE the old one instead; the collector frees retired
        // buffers at the next stop-the-world root scan, when no mutator holds
        // an in-flight pointer into it. Reserve geometrically so repeated grows
        // do not retire a buffer on every call. All throwing work happens
        // before the old buffer is handed over, so a failure leaves Refs
        // intact.
        std::vector<RefVariant> New;
        New.reserve(std::max(NewSize, Refs.size() + Refs.size() / 2 + 1));
        New.assign(Refs.begin(), Refs.end());
        New.resize(NewSize, Val);
        Allocator->retireTableBufferLocked(std::move(Refs));
        Refs = std::move(New);
      } else {
        // In-place (capacity already suffices) or a standalone table (no
        // allocator, no concurrent readers): extend without moving the buffer,
        // so no reader is left pointing at freed storage. resize(n, Val) also
        // value-initializes the new slots to Val.
        Refs.resize(NewSize, Val);
      }
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
    } catch (...) {
      return false;
    }
#endif
    // Report the pre-grow size from inside the exclusive window before
    // publishing the new size below.
    if (OldSize) {
      *OldSize = Min;
    }
    // New slots join the roots after the root snapshot; shade the broadcast
    // reference so a concurrent collection keeps it alive.
    if (Allocator) {
      Allocator->writeBarrier(Val);
    }
    DataPtr.store(Refs.data(), std::memory_order_release);
    setLiveSize(NewSize);
    return true;
  }

  /// Update the size in the limit and its live mirror synchronously.
  void setLiveSize(const uint64_t Size) noexcept {
    TabType.getLimit().setMin(Size);
    LiveSize.store(Size, std::memory_order_release);
  }

  /// \name Data of table instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  // Number of holds that keep this table attached to Allocator, written under
  // AttachMutex. A setAllocator() hold (instantiation, import) is permanent. A
  // claimForRegister() hold is released by detachAllocator() if its
  // registration fails. Only the last hold detaches the table.
  uint64_t AttachRefs = 0;
  // Serializes owner claims so two allocators never both claim an unattached
  // table. Allocator is read without the lock on the barrier and destructor
  // paths, which run only after the attachment is settled.
  std::mutex AttachMutex;
  AST::TableType TabType;
  std::vector<RefVariant> Refs;
  RefVariant InitValue;
  // Atomic so a concurrent AOT reader's load (context.h getTable) does not
  // data-race a reallocating growTable's republish of the base pointer. The
  // pointer read is a whole value either way; atomicity satisfies the memory
  // model (and TSan). Layout-compatible with a plain RefVariant* (lock-free).
  std::atomic<RefVariant *> DataPtr{nullptr};
  // Atomic so a concurrent AOT reader's bounds-check size load (context.h
  // getTableSize) does not data-race growTable's setLiveSize. Lock-free and
  // layout-compatible with a plain uint64_t.
  std::atomic<uint64_t> LiveSize;
  // Resolved once at construction; see canHoldManaged().
  const bool CanHoldManaged;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
