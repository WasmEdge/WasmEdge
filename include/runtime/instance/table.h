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
  // \p CanHoldManaged is the resolved, immutable "can this table hold
  // managed GC objects" bit. It cannot be computed here: a
  // concrete heap-type index is defining-module-relative, and the
  // constructor has no type list. Callers with a type list (addTable)
  // resolve it via AST::TypeMatcher::refTypeCanHoldGCObject; callers without
  // one (e.g. the standalone C API) resolve conservatively -- see the
  // comments at those call sites.
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

  // Attach the GC allocator that scans this table's roots and runs its write
  // barriers. This is the single, fallible, centralized owner-attach check: a
  // second, foreign controller is rejected when the table is unsafe to share
  // across allocators, namely when it is either
  //   (a) managed-capable (canHoldManaged()): a ref this foreign controller
  //       stores here barriers against the wrong allocator, so it is invisible
  //       to the owner's GC and can be swept while still reachable -- rejected
  //       regardless of size; OR
  //   (b) growable (!hasMax() || max > min): a reallocating growTable frees the
  //       Refs buffer a peer executor's reader may still hold -- use-after-free
  //       -- rejected regardless of element type.
  // A fixed, non-managed table (e.g. a fixed funcref table) carries neither
  // hazard and stays freely shareable: a foreign attach keeps the FIRST owner
  // and succeeds. A same-owner re-attach is an idempotent success. The claim of
  // an unattached table runs under a per-instance mutex so two controllers can
  // never race it into a torn owner: the loser blocks, then sees the winner's
  // commit and re-runs the foreign check above. A host table is rooted only
  // once imported; if created with a GC ref init value, that ref must be kept
  // reachable by the host until then.
  Expect<void> setAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    // The bool (whether this call newly claimed ownership) is unused here; only
    // the registerModule walk needs it -- see claimForRegister().
    auto Res = setAllocatorLocked(A);
    if (!Res) {
      return Unexpect(Res.error());
    }
    return {};
  }

  /// registerModule ownership walk: attach to \p A exactly as setAllocator,
  /// but report whether THIS call newly claimed ownership, so a
  /// preflight-then-commit registration can reverse exactly the attaches it
  /// made if a later table in the same module is rejected. Returns true when
  /// the table transitioned from unattached to \p A-owned; false for an
  /// idempotent same-owner re-attach or a foreign-but-freely-shareable attach
  /// (both leave the prior owner in place); and the unchanged two-hazard error
  /// for a rejected foreign attach.
  Expect<bool> claimForRegister(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    return setAllocatorLocked(A);
  }

  /// Reverse a claimForRegister()/setAllocator() ownership claim made by \p A,
  /// undoing the `Allocator = &A; addTable(*this)` in setAllocatorLocked. A
  /// call naming a non-owner (a foreign, freely-shareable attach that kept the
  /// first owner and registered nothing) is a no-op, so a registerModule
  /// rollback never disturbs a table it did not newly claim. Distinct from the
  /// private clearAllocator() teardown hook, which runs under A's HeapMutex and
  /// must NOT re-enter removeTable().
  void detachAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    if (Allocator == &A) {
      Allocator->removeTable(*this);
      Allocator = nullptr;
    }
  }

  /// True iff a GC controller owns this table AND its element type holds
  /// internal GC-managed objects (struct/array/any/eq/i31/exn or a concrete
  /// struct/array type index) -- i.e. a raw mutation would place or observe a
  /// managed object whose controller provenance the C-API cannot establish. The
  /// direct C-API mutators (WasmEdge_TableInstanceSetData /
  /// WasmEdge_TableInstanceGrow) refuse a raw mutation on such a table: the raw
  /// path cannot prove the caller's controller matches the owner, so a managed
  /// store/grow could barrier against, or stop-the-world on, the wrong
  /// allocator.
  ///
  /// externref is deliberately EXCLUDED even though canHoldManaged() is true
  /// for it (extern.convert_any can wrap a GC object): an externref table is
  /// the host-facing reference primitive, and storing a host reference through
  /// it is the standard C-API workflow, which must keep working. A standalone
  /// (unattached) table, an externref table, and a funcref table therefore all
  /// stay C-API-mutable exactly as before; only internal GC-ref tables are
  /// gated.
  bool isManagedByController() const noexcept {
    return Allocator != nullptr && Allocator->getController() != nullptr &&
           CanHoldManaged && !TabType.getRefType().isExternRefType();
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

  /// True if this table can ever grow (its live buffer may be reallocated):
  /// no declared max, or a max strictly above the current min. Computed from
  /// TabType and stable at attach time (the max is immutable post-construction
  /// and setAllocator runs before any grow), so it is safe to read here as the
  /// second cross-controller sharing hazard.
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
  /// When attached to a GC controller, table growth is an EXCLUSIVE,
  /// mutator-parking stop-the-world operation: the grower acquires the
  /// per-controller exclusive token (blocking, FIFO) so a grow and a collection
  /// can never both stop the world, then drives the setup handshake so every
  /// other Running mutator is parked at a safe point while the Refs buffer is
  /// swapped. That is what closes the interpreter concurrent-grow-vs-reader
  /// race (getRefAddr/setRefAddr read the Refs vector object a reallocating
  /// grow mutates). A standalone table (no controller) keeps the plain path --
  /// there is no mutator registry to stop.
  ///
  /// On success, if \p OldSize is non-null, the pre-grow size is written to it
  /// from inside the exclusive window: both callers push that size as the
  /// table.grow result, so it must be the size a serialized grow actually
  /// extended, not one read racily before the token was held.
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
    // Pre-token fast reject for a DEFINITELY-doomed grow, BEFORE acquiring the
    // exclusive token or driving any handshake: otherwise a guest
    // `(loop (drop (table.grow $t (ref.null) <huge>)) (br 0))` would force a
    // global stop-the-world every iteration (DoS amplification). MaxSizeCaped
    // is constant (the address-type ceiling, further capped by an explicit max)
    // and LiveSize is monotonically non-decreasing, so a grow observed over the
    // cap at the current size stays over it -- this NEVER false-rejects. Only
    // the pass->fail direction (a concurrent winner raised the min) still needs
    // the in-window re-validation, which growRefsLocked does. Read the size
    // from the atomic mirror (race-free against a grower's setLiveSize store);
    // the max / addr-type fields are immutable after construction.
    {
      uint64_t MaxSizeCaped = getMaxAddress(TabType.getLimit().getAddrType());
      if (TabType.getLimit().hasMax()) {
        MaxSizeCaped = std::min(TabType.getLimit().getMax(), MaxSizeCaped);
      }
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
    // Controller-backed: acquire the exclusive token (BLOCKING -- a losing
    // grower parks on a FIFO ticket and, on wake, OWNS the token). A false
    // return means the controller is Closing: report the defined grow failure.
    uint64_t Gen = 0;
    if (!Ctrl->beginExclusiveOp(
            GC::Controller::ExclusiveOwner::State::OwnedGrowing, Gen)) {
      return false;
    }
    // Winner (or a loser that woke owning the token) runs its STW on its own
    // Running thread: park every other Running mutator, then swap the buffer
    // under HeapMutex. growRefsLocked reads the CURRENT min, so a loser
    // re-validates the limit a prior winner raised. No root scan and
    // no setSelfBlocked (grow grays its broadcast ref via writeBarrier).
    bool Result = false;
    Ctrl->growStopTheWorld(*Allocator, [&]() noexcept {
      Result = growRefsLocked(Count, Val, OldSize);
    });
    Ctrl->endExclusiveOp(Gen,
                         GC::Controller::ExclusiveOwner::State::OwnedGrowing);
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

  /// Core of the fallible owner-attach check, assuming the caller holds
  /// AttachMutex. Returns true iff this call newly claimed ownership (was
  /// unattached, now owned by \p A); false for an idempotent same-owner
  /// re-attach or a foreign-but-freely-shareable attach; and the two-hazard
  /// error for a rejected foreign attach. Kept as the single source of the
  /// attach rule so setAllocator() and claimForRegister() cannot diverge.
  Expect<bool> setAllocatorLocked(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      // Idempotent: the same owner re-attaching (e.g. a regularly instantiated
      // table re-imported into another module owned by the same executor).
      return false;
    }
    if (hasForeignAllocator(A)) {
      // A different controller already owns this table. Reject the unsafe
      // cases; a fixed, non-managed table is freely shareable and keeps its
      // first owner.
      if (canHoldManaged() || isGrowable()) {
        return Unexpect(ErrCode::Value::IncompatibleImportType);
      }
      return false;
    }
    // Unattached: claim ownership under the lock.
    Allocator = &A;
    Allocator->addTable(*this);
    return true;
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
    }
  }

  /// Perform the checked resize/retire/publish of the Refs buffer.
  ///
  /// Assumes the caller holds the table-root lock (Allocator::HeapMutex) when
  /// Allocator != nullptr -- taken either by the grower STW
  /// (Controller::growStopTheWorld) or by growTable's standalone plain path.
  /// Reads the CURRENT min, so a grow-loser that woke owning the exclusive
  /// token re-validates the limit a prior winning grow may have raised. On
  /// success writes the pre-grow size to \p OldSize (if non-null) from inside
  /// the window. Returns false on an over-limit or allocation-failed grow,
  /// leaving Refs untouched.
  bool growRefsLocked(const uint64_t Count, const RefVariant &Val,
                      uint64_t *OldSize) noexcept {
    // growTable handles the no-op grow before ever calling this helper.
    assuming(Count != 0);
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
  // Serializes owner claims in setAllocator so two controllers can never race
  // an unattached table into a torn owner (double-register). The common
  // instantiation path is single-threaded; this only guards the rare contended
  // attach. The committed Allocator is still read unsynchronized on the hot
  // barrier/destructor paths, which run only after attachment is settled.
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
