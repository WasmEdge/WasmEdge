// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/gc/allocator.h - GC memory allocator ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of allocator class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "gc/coherent_slot.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#define WASMEDGE_GC_DISABLE_SANITIZER
#else
#define WASMEDGE_GC_DISABLE_SANITIZER                                          \
  [[gnu::no_sanitize_address, gnu::no_sanitize_thread]]
#endif

namespace WasmEdge {

namespace Runtime {
namespace Instance {
class GlobalInstance;
class TableInstance;
class ElementInstance;
class ExceptionInstance;
} // namespace Instance
} // namespace Runtime

namespace GC {

class Controller;

enum class GCPhase : uint8_t {
  Idle,
  MarkRootStart,
  MarkGrayStart,
  SweepStart,
  SweepEnd,
};

struct PhaseObserver {
  virtual void onPhase(GCPhase) noexcept = 0;
  virtual ~PhaseObserver() = default;
};

class Allocator {
public:
  /// Mark colours. `Marked0`/`Marked1` are the two *parity* encodings of
  /// "traced": which one currently means Black is decided by `BlackParity`, so
  /// the sweep's black/white role swap is a single parity flip rather than
  /// moving every survivor between two hash sets. Black is
  /// `Marked(BlackParity)`; White is `Marked(1 - BlackParity)`.
  static constexpr uint8_t ColorMarked0 = 0;
  static constexpr uint8_t ColorMarked1 = 1;
  /// On (or about to be on) the gray work list. Never reinterpreted by a parity
  /// flip, so an object allocated during Idle/Sweeping stays gray across the
  /// cycle boundary and is traced by the next marking phase.
  static constexpr uint8_t ColorGray = 2;

  /// One managed object's GC metadata, immediately preceding its payload.
  /// `sizeof` must stay a multiple of 16 so the payload (RawData, whose
  /// ValVariant members are SIMD-aligned) starts 16-aligned, and `alignof` must
  /// stay 16 for the over-aligned block allocation -- see the static_assert in
  /// runtime/instance/gc.h.
  struct alignas(16) Header {
    /// Total block size (header + payload), as charged to `Used`.
    uint32_t Size = 0;
    /// Mark colour, written by mutators (the write barrier's shade) and by
    /// collector workers (the post-trace blacken) concurrently.
    std::atomic<uint8_t> Color{ColorGray};
    /// Intrusive link for the all-object list: sweep enumeration and teardown.
    /// An object is on this list for its whole life. Guarded by `AllMutex`.
    Header *AllNext = nullptr;
    /// Intrusive link for the gray work list, held only while the object is
    /// gray. Guarded by `GrayMutex`. Distinct from `AllNext` precisely because
    /// an object is on both lists at once while it waits to be traced.
    Header *GrayNext = nullptr;
  };

  Allocator();

  explicit Allocator(Controller &C);

  ~Allocator() noexcept;

  template <typename InitFunc>
  [[nodiscard]] void *allocate(InitFunc &&Init, uint32_t Size) noexcept {
    // Init runs inside this noexcept function; a throwing Init would terminate.
    static_assert(std::is_nothrow_invocable_v<InitFunc &, void *>,
                  "allocate()'s Init callback must be noexcept");
    // doAllocate and Header::Size are uint32_t; reject sizes where
    // sizeof(Header) + Size would wrap and under-allocate (Init then
    // overflows).
    if (unlikely(Size >
                 std::numeric_limits<uint32_t>::max() - sizeof(Header))) {
      return nullptr;
    }
    // Sum now fits uint32_t; narrow once (else MSVC /W4 flags the H->Size
    // assignment).
    const uint32_t Total = static_cast<uint32_t>(sizeof(Header) + Size);
    uint8_t *Pointer = doAllocate(Total);
    if (unlikely(!Pointer)) {
      return nullptr;
    }
    auto H = reinterpret_cast<Header *>(Pointer);
    new (H) Header(); // born gray (Header's default colour)
    H->Size = Total;
    // Publish into the membership index and the all-object list BEFORE Init, so
    // the block is sweep-visible and validatable from the moment it exists. A
    // concurrent sweep walking the list here reads only Size/Color/AllNext, all
    // set above, and skips it because it is gray -- Init's payload writes are
    // never observed half-done by the collector.
    //
    // This is the ONE index operation that may need to grow, and it is the one
    // with a failure channel: on growth failure the allocation fails (nullptr)
    // rather than throwing on this noexcept path. Every other index operation
    // -- the barrier's contains(), the sweep's erase() -- is allocation-free.
    {
      std::unique_lock<std::mutex> Locker(AllMutex);
      if (unlikely(!Index.insert(H))) {
        Locker.unlock();
        doDeallocate(Pointer, Total);
        return nullptr;
      }
      H->AllNext = AllHead;
      AllHead = H;
    }
    Init(reinterpret_cast<void *>(Pointer + sizeof(Header)));
    {
      std::unique_lock<std::mutex> Locker(GrayMutex);
      H->GrayNext = GrayHead;
      GrayHead = H;
      // During concurrent marking, wake a worker to scan this freshly-allocated
      // gray object before sweeping; its edges would otherwise stay unmarked.
      // Guarded to MarkingGray so idle-time allocations don't wake workers.
      if (CurrentGCState.load(std::memory_order_acquire) ==
          GCState::MarkingGray) {
        GrayNotEmptyCV.notify_one();
      }
    }

    return static_cast<void *>(Pointer + sizeof(Header));
  }

  uint64_t getMemoryUsage() const noexcept {
    return Used.load(std::memory_order_relaxed);
  }

  // Back-reference to the owning Controller (the GC concurrency coordinator), or
  // nullptr for a standalone Allocator with no controller -- a bare unit-test
  // allocator, or a C-API table created without an executor. TableInstance::
  // growTable reaches the exclusive-op arbiter and the setup handshake through
  // this: a null controller means there is no mutator registry to stop, so
  // growth keeps the plain (non-STW) path.
  Controller *getController() noexcept { return Ctrl; }
  const Controller *getController() const noexcept { return Ctrl; }

  void setManualGC(bool Enable) noexcept {
    EnableManualGC.store(Enable, std::memory_order_release);
  }

  void setPhaseObserver(PhaseObserver *Obs) noexcept {
    PhaseObs.store(Obs, std::memory_order_release);
  }

  // Test-only hook: invoked by a collector worker after it pops a gray object
  // and BEFORE it shades that object's children, i.e. while the object is
  // popped-but-untraced and ActiveTracers has been incremented. Lets a test
  // pause a worker mid-trace to prove termination waits for in-flight tracers
  // (GC.TerminationWaitsForInFlightTracer). Set while the heap is Idle (no
  // worker is popping) so the store does not race a worker's read.
  void setTracerPauseHook(std::function<void()> Fn) noexcept {
    TracerPauseHook = std::move(Fn);
  }

  // Test-only hook: invoked by the sweep owner at the top of
  // runSweepAndSwap() -- GCState is Sweeping, STW #2 has ended (mutators are
  // released), and NO allocator lock is held -- so a test can hold the sweep
  // open and perform a deterministic allocation DURING the sweep phase. (The
  // SweepStart PhaseObserver cannot serve this: it fires under GrayMutex,
  // which allocate() must take.) Set while the heap is Idle, like
  // setTracerPauseHook.
  void setSweepPauseHook(std::function<void()> Fn) noexcept {
    SweepPauseHook = std::move(Fn);
  }

  // Test-only hook: invoked by a collector worker that has decided to terminate
  // (empty Gray under MarkingGray) AFTER it releases GrayMutex and BEFORE it
  // CASes for termination ownership, holding no allocator lock. Lets a test
  // stall a would-be owner at exactly the preemption window that produces a
  // stale ownership win, so the phase re-check that rejects it
  // (GCThread.StaleTerminationWinRejected) is exercised deterministically. Set
  // while the heap is Idle, like setTracerPauseHook.
  void setPreTerminationCASHook(std::function<void()> Fn) noexcept {
    PreTerminationCASHook = std::move(Fn);
  }

  // Test-only hook: invoked at the top of a controller-backed collection's
  // beginCycle() AFTER the collection has won the exclusive token but BEFORE the
  // Idle->MarkingRoot CAS -- so the token is held while the heap is still Idle
  // and no allocator lock is held. Lets a test park a grower behind the held
  // token and have it pop its table.grow initializer while the write barrier is
  // still quiet (Idle), so the grower's scoped-root pin -- not the pop-time
  // shade -- is what must carry that ref into this cycle's root snapshot
  // (GCThread.InterpreterGrowInitializerSurvivesCollect). Set while Idle, like
  // setTracerPauseHook. Empty in production.
  void setPreCycleHook(std::function<void()> Fn) noexcept {
    PreCycleHook = std::move(Fn);
  }

  // Test-only read of the stale-termination-win reject count; see
  // StaleTerminationRejects.
  uint64_t debugStaleTerminationRejects() const noexcept {
    return StaleTerminationRejects.load(std::memory_order_acquire);
  }

  // ScanNativeStack conservatively scans the current thread's native stack for
  // roots. Only AOT callers need it (a ref may live solely in a register/stack
  // slot); the interpreter keeps every live ref on a tracked value stack.
  void autoCollect(bool ScanNativeStack = false) noexcept;

  bool manualCollect(bool ScanNativeStack = false) noexcept;

  template <typename T> void writeBarrier(const T &Val) const noexcept {
    // Shade for MarkingRoot/MarkingGray, not just MarkingGray: the collector
    // can enter MarkingRoot/MarkingGray between this load and the caller's
    // store, so guarding on MarkingGray alone would let a store during root
    // marking escape shading (lost object). markGray no-ops non-White refs,
    // so shading during MarkingRoot is merely conservative. Quiet on
    // Sweeping too: the termination owner disables the barrier atomically
    // (the MarkingGray->Sweeping CAS) before sweep begins, so shading a
    // soon-to-be-freed White object mid-sweep would just be wasted work.
    const GCState St = CurrentGCState.load(std::memory_order_acquire);
    if (likely(St == GCState::Idle || St == GCState::Sweeping)) {
      return;
    }
    const_cast<Allocator *>(this)->doWriteBarrier(getPointer(Val));
  }

  template <typename T>
  void bulkWriteBarrier(Span<const T> Slots) const noexcept {
    // See writeBarrier: shade for MarkingRoot/MarkingGray to avoid losing
    // stores that race the MarkingRoot->MarkingGray transition; quiet on
    // Idle and Sweeping.
    const GCState St = CurrentGCState.load(std::memory_order_acquire);
    if (likely(St == GCState::Idle || St == GCState::Sweeping)) {
      return;
    }
    for (const auto &Val : Slots) {
      const_cast<Allocator *>(this)->doWriteBarrier(getPointer(Val));
    }
  }

  // Deletion-barrier variants for a LIVE managed slot that a concurrent mutator
  // may storeCoherent to. Unlike writeBarrier/bulkWriteBarrier -- whose
  // getPointer() does a plain 128-bit memcpy of the value, fine for a local or
  // a stable source -- these read only the pointer word, with a single relaxed
  // atomic load (loadPointerWordRelaxed), so shading the OLD reference of a
  // slot that is being overwritten never data-races the writer's 16-byte atomic
  // store. Use these for the pre-store old-value read of a struct field or
  // array element; keep writeBarrier for the new (local) value.
  void writeBarrierSlot(const ValVariant &Slot) const noexcept {
    const GCState St = CurrentGCState.load(std::memory_order_acquire);
    if (likely(St == GCState::Idle || St == GCState::Sweeping)) {
      return;
    }
    const_cast<Allocator *>(this)->doWriteBarrier(loadPointerWordRelaxed(Slot));
  }

  void bulkWriteBarrierSlots(Span<const ValVariant> Slots) const noexcept {
    const GCState St = CurrentGCState.load(std::memory_order_acquire);
    if (likely(St == GCState::Idle || St == GCState::Sweeping)) {
      return;
    }
    auto *Self = const_cast<Allocator *>(this);
    for (const auto &Slot : Slots) {
      Self->doWriteBarrier(loadPointerWordRelaxed(Slot));
    }
  }

  void addTable(Runtime::Instance::TableInstance &Table) noexcept;

  void removeTable(Runtime::Instance::TableInstance &Table) noexcept;

  void addGlobal(Runtime::Instance::GlobalInstance &Global) noexcept;

  void removeGlobal(Runtime::Instance::GlobalInstance &Global) noexcept;

  // Element segments holding GC refs are roots: a passive/declarative segment
  // can hold the only reference to a struct/array from its init expression,
  // which table.init/array.new_elem later copy into live tables/arrays.
  void addElem(Runtime::Instance::ElementInstance &Elem) noexcept;

  void removeElem(Runtime::Instance::ElementInstance &Elem) noexcept;

  // Exception payloads are roots: a struct/array ref in a tag payload may
  // survive only here once on-stack copies are consumed, and throw_ref
  // re-pushes the payload.
  void addException(Runtime::Instance::ExceptionInstance &Exception) noexcept;

  void
  removeException(Runtime::Instance::ExceptionInstance &Exception) noexcept;

  /// Host-root retention for GC references handed back to the host.
  ///
  /// retainResult pins a ref as a GC root. Refs match by pointer identity
  /// (ValType ignored) and retain by multiplicity: N retains need N releases.
  /// releaseRef removes one, releaseRefs one of each, releaseAllRefs all.
  void retainResult(const RefVariant &Ref) noexcept;

  WASMEDGE_EXPORT void releaseRef(const RefVariant &Ref) noexcept;

  void releaseRefs(Span<const RefVariant> Refs) noexcept {
    for (const auto &Ref : Refs) {
      releaseRef(Ref);
    }
  }

  WASMEDGE_EXPORT void releaseAllRefs() noexcept;

  /// Scoped boundary-root retention, distinct from the host-explicit
  /// retainResult/releaseRef ownership. Backs BoundaryRoots: refs pinned at an
  /// API/async boundary and released when the boundary scope unwinds. Matched
  /// by pointer identity, LIFO. These are scanned as GC roots but are NEVER
  /// consumed by releaseAllRefs() -- see design 2026-07-13:228-229.
  void retainScopedRef(const RefVariant &Ref) noexcept;
  void releaseScopedRef(const RefVariant &Ref) noexcept;

  /// Serialize a structural table-root mutation against the root scan, which
  /// reads every registered table's Refs vector under this same HeapMutex.
  /// growTable reallocates that vector; holding this lock across the resize
  /// stops the scan iterating a freed buffer. In-place slot writes
  /// (setRefs/fillRefs/setRefAddr) keep the buffer stable and don't need it --
  /// benign conservative-read races the scan tolerates.
  [[nodiscard]] std::unique_lock<std::mutex> lockTableRoots() noexcept {
    return std::unique_lock<std::mutex>(HeapMutex);
  }

  /// Retire a table's old Refs buffer instead of freeing it in a reallocating
  /// growTable (round-2 A3). A concurrent mutator reader (AOT/interpreter
  /// table.get/set holding the old DataPtr) may still be accessing that buffer,
  /// so freeing it there is a use-after-free. The retired buffers are freed at
  /// the next stop-the-world root scan (scanSharedRoots), when every mutator is
  /// parked at a safepoint or NativeRunning -- a table access is safepoint-free,
  /// so no acked mutator holds an in-flight pointer into a retired buffer. The
  /// caller MUST hold lockTableRoots() (growTable does), so this pushes without
  /// re-locking HeapMutex. Reserve BEFORE the move: `Old` is only a reference
  /// until push_back runs, so a bad_alloc in reserve leaves the caller's Refs
  /// untouched (not half-retired then freed -> the UAF this exists to avoid).
  void retireTableBufferLocked(std::vector<RefVariant> &&Old) {
    RetiredTableBuffers.reserve(RetiredTableBuffers.size() + 1);
    RetiredTableBuffers.push_back(std::move(Old));
  }

private:
  // The Controller drives the cooperative safe point, where a mutator shades
  // its own roots (Controller::selfScanInto): it needs markGrayRoot() and
  // markNativeStackRoots(). Friendship rather than new public methods keeps
  // marking off the Allocator's public surface -- nothing outside the GC may
  // shade an object by hand.
  friend class Controller;

  // Shade one value-stack slot. Thin wrapper so the Controller's precise
  // self-scan does not need getPointer()/markGray() individually.
  void markGrayRoot(const ValVariant &V) noexcept { markGray(getPointer(V)); }

  // Constructs the worker threads shared by both constructors; written once so
  // the default (standalone, Ctrl == nullptr) and controller-backed
  // constructors don't duplicate the spawn/rollback logic.
  void startWorkers();

  // Mark every registered root gray (value stacks, tables, globals, element
  // segments, exception payloads, host roots, and -- when ScanNativeStack --
  // the current thread's native stack). Used only by the standalone-allocator
  // legacy path (Ctrl == nullptr); the coordinator-driven path scans stacks via
  // the handshake and shared roots via snapshotSharedRootsInto().
  void markRoots(bool ScanNativeStack) noexcept;

  // Shared (non-stack) roots: table InitValue+Refs, globals, element segments,
  // exception payloads, host roots -- each under its own mutex. Factored out so
  // markRoots() and snapshotSharedRootsInto() cannot diverge and drop a root
  // type. Does NOT notify a phase.
  void scanSharedRoots() noexcept;

  // --- Coordinator (Controller::collect) primitives -------------------------
  // These split the old inline autoCollect/manualCollect into steps the
  // Controller drives across the setup handshake. All are Controller-only
  // (friend); nothing outside the GC drives a cycle by hand.

  // Admission gate + Idle->MarkingRoot CAS. For an auto cycle (Manual == false)
  // it also honours the manual-GC toggle and the NextGC schedule. Returns false
  // if a cycle must not start (already collecting / manual-gated / not due).
  bool beginCycle(bool Manual) noexcept;

  // Shared-root snapshot taken while all mutators are quiesced (globals,
  // tables, element segments, exceptions, host roots). Value stacks are NOT
  // scanned here
  // -- the handshake scans them (self-scan of parked Running mutators +
  // scanNonRunningRoots for the rest + the coordinator's own selfScanInto).
  // Fires MarkRootStart.
  void snapshotSharedRootsInto() noexcept;

  // Fire MarkGrayStart. Does NOT publish MarkingGray and does NOT wake the
  // workers -- both are deferred to wakeCollectors(), which the coordinator
  // calls only after STW #1 has ended (endHandshake). Keeping MarkingGray
  // unobservable to workers until then means a worker woken (even spuriously)
  // cannot drive STW #2's handshake while STW #1's is still open.
  void beginMarking() noexcept;

  // Publish MarkingGray (under GrayMutex) and wake the collector workers to
  // begin draining Gray. Called by the coordinator after endHandshake(Gen) ends
  // STW #1. Storing the state under GrayMutex before notifying pairs with the
  // workers' predicate check under the same lock (no lost wakeup).
  void wakeCollectors() noexcept;

  // Block until the workers have swept and returned the state to Idle -- or
  // until teardown (Closing/Stop) abandons the cycle.
  void waitForCycleComplete() noexcept;

  // Teardown wake: re-notify GCCV and GrayNotEmptyCV (each under its mutex) so
  // the coordinator in waitForCycleComplete() and any parked worker re-evaluate
  // their Closing-aware predicates and abandon the in-flight cycle. Called by
  // Controller::beginClosing() after it publishes Closing.
  void wakeForTeardown() noexcept;

  // Old inline manualCollect path for the standalone allocator (Ctrl ==
  // nullptr, no registry/coordinator): markRoots on this thread, publish, wait.
  bool legacyManualCollect(bool ScanNativeStack) noexcept;

  // Spill callee-saved registers onto the native stack, then mark every word of
  // the active stack as a conservative root. Must run in a frame that outlives
  // the synchronous root scan; see the definition for why the spill cannot live
  // inside getStack().
  void markNativeStackRoots() noexcept;

  // Active native stack span [Frame, stack base). Frame must point into a live
  // frame (typically a local's address in the caller) so the scan covers the
  // spilled registers and every caller frame up to the base.
  static Span<uint8_t *const> getStack(void *Frame) noexcept;

  [[nodiscard]] uint8_t *doAllocate(uint32_t N) noexcept;

  void doDeallocate(uint8_t *P, uint32_t Size) noexcept;

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const ValVariant &Val) noexcept {
    // The managed pointer is the high 64-bit word: RefVariant (and ValVariant's
    // RefVariant slot) is 128 bits, type tag in the low word, object pointer in
    // the high. Keep in sync with RefVariant in common/types.h -- a layout
    // change would silently make the GC miss every root. memcpy the bits out
    // rather than type-pun through an std::array reference (strict aliasing).
    std::array<uint64_t, 2> Raw;
    static_assert(sizeof(Val) >= sizeof(Raw));
    std::memcpy(Raw.data(), &Val, sizeof(Raw));
    return reinterpret_cast<uint8_t *>(Raw[1]);
  }

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(const RefVariant &Ref) noexcept {
    std::array<uint64_t, 2> Raw;
    static_assert(sizeof(Ref) >= sizeof(Raw));
    std::memcpy(Raw.data(), &Ref, sizeof(Raw));
    return reinterpret_cast<uint8_t *>(Raw[1]);
  }

  WASMEDGE_GC_DISABLE_SANITIZER
  static uint8_t *getPointer(uint8_t *const &Ptr) noexcept { return Ptr; }

  void doWriteBarrier(uint8_t *Target) noexcept;

  // --- Allocation-membership index ------------------------------------------
  // Maps an allocated block's Header* to "is a live managed object". The
  // conservative root scan must decide whether an arbitrary aligned word (a
  // native-stack slot, a table entry) points at a real object BEFORE touching
  // its header -- reading Color at a bogus address could fault or read
  // unrelated memory -- so this index, not the header, is the validity oracle.
  // It is the exact replacement for the old White hash-set membership test,
  // which did the same job as a side effect of erase().
  //
  // Open addressing, linear probing, tombstoned erase. insert() is the only
  // operation that may allocate, and it runs only from allocate(), which HAS a
  // failure channel: a failed growth fails that allocation (nullptr) instead of
  // throwing on a noexcept path. contains() (write barrier / root scan) and
  // erase() (sweep) are strictly allocation-free -- the point of the class.
  //
  // Address reuse is handled by ordering, not by generation stamps: sweep
  // erases a dead object from the index while holding AllMutex and frees its
  // block only after releasing it, so an address can be recycled by a later
  // ::operator new and re-inserted with no stale entry in between.
  class ObjectIndex {
  public:
    ObjectIndex() noexcept = default;
    ObjectIndex(const ObjectIndex &) = delete;
    ObjectIndex &operator=(const ObjectIndex &) = delete;
    ~ObjectIndex() noexcept { ::operator delete(Slots); }

    // Insert H. Returns false only when the table had to grow and the growth
    // allocation failed; the caller must then fail its allocation.
    [[nodiscard]] bool insert(Header *H) noexcept;
    // Membership test. Touches only the table, never H.
    bool contains(const Header *H) const noexcept;
    // Remove H if present. Allocation-free (leaves a tombstone).
    void erase(const Header *H) noexcept;
    size_t size() const noexcept { return Count; }

  private:
    // Not 16-aligned, so it can never collide with a real Header address.
    static Header *tombstone() noexcept {
      return reinterpret_cast<Header *>(alignof(Header) / 2 + 1);
    }
    // Murmur3 finalizer over the block index (pointers are 16-aligned, so the
    // low 4 bits carry nothing). Spreads into the low bits, which is what the
    // power-of-two mask consumes.
    static size_t mix(const Header *H) noexcept {
      uint64_t X = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(H)) >> 4;
      X ^= X >> 33;
      X *= UINT64_C(0xFF51AFD7ED558CCD);
      X ^= X >> 33;
      X *= UINT64_C(0xC4CEB9FE1A85EC53);
      X ^= X >> 33;
      return static_cast<size_t>(X);
    }
    // Rebuild into a fresh table of NewCap slots (power of two). Drops
    // tombstones. Returns false if the table allocation failed, leaving the
    // index untouched.
    [[nodiscard]] bool rehash(size_t NewCap) noexcept;
    // Grow (or, when the pressure is only tombstones, rehash in place at the
    // same capacity to reclaim them).
    [[nodiscard]] bool grow() noexcept;

    Header **Slots =
        nullptr;      // Cap entries; nullptr = empty, tombstone() = dead
    size_t Cap = 0;   // power of two, or 0 before the first insert
    size_t Count = 0; // live entries
    size_t Tombs = 0; // tombstones
  };

  void markGray(uint8_t *Pointer) noexcept;

  // Reclaim every White object, flip the Black/White parity, fire SweepEnd, and
  // publish GCState::Idle. Called by the elected termination owner AFTER it has
  // published GCState::Sweeping under STW #2 (which quiets the write barrier
  // and stops workers dequeuing), so no markGray runs concurrently and the
  // free needs no GrayMutex. Does NOT perform the MarkingGray->Sweeping
  // transition (the owner does that under GrayMutex) or fire SweepStart.
  void runSweepAndSwap() noexcept;

  void notifyPhase(GCPhase P) noexcept {
    if (auto *Obs = PhaseObs.load(std::memory_order_acquire)) {
      Obs->onPhase(P);
    }
  }

  enum class GCState : uint8_t {
    Idle,
    MarkingRoot,
    MarkingGray,
    Sweeping,
  };

  std::atomic<bool> Stop = false;
  std::atomic<bool> EnableManualGC = false;
  std::atomic<GCState> CurrentGCState = GCState::Idle;
  // In-flight tracer accounting. Incremented under GrayMutex right after a
  // worker pops a gray object (before releasing the lock) and decremented after
  // that object's children are shaded and it is inserted into Black. An empty
  // Gray queue is NOT quiescence: a worker may hold a popped-but-untraced
  // object whose only-reachable-through-it children are still White.
  // Termination requires Gray.empty() && ActiveTracers == 0 observed under
  // GrayMutex.
  std::atomic<uint32_t> ActiveTracers = 0;
  // Single-owner election for STW #2 termination: a worker CASes 0->1 to become
  // the termination owner; the loser workers keep tracing/parking. Reset to 0
  // after the owner sweeps (or aborts termination on a failed re-check).
  std::atomic<uint32_t> TerminationOwner = 0;
  // Exclusive-operation owner generation of the in-flight Controller-backed
  // collection (E1.1). Controller::collect() stamps it -- after winning
  // tryBeginExclusiveOp(OwnedCollecting) and before wakeCollectors() -- so it is
  // set before any worker can observe MarkingGray. The sweep-completing worker
  // reads it BEFORE runSweepAndSwap() publishes Idle (after which a fresh
  // collect() could overwrite it) and calls Ctrl->endExclusiveOp(gen,
  // OwnedCollecting), releasing the token across the STW #2 handoff. Zero and
  // unused for a standalone Allocator (Ctrl == nullptr, legacyManualCollect),
  // which never takes the token.
  std::atomic<uint64_t> CollectionOwnerGen = 0;
  // Diagnostic count of stale termination-owner wins rejected by the phase
  // re-check in the worker loop: a worker that set Terminate under GrayMutex
  // but was preempted before the ownership CAS can win LATE, after the in-cycle
  // owner already swept and left MarkingGray. Firing STW #2 then would open a
  // second handshake generation overlapping the next coordinator's STW #1. Also
  // read by tests to assert the guard fired (debugStaleTerminationRejects).
  std::atomic<uint64_t> StaleTerminationRejects = 0;
  std::atomic<uint64_t> Threshold = uint64_t{1024} * 1024 * 1024; // 1 GiB
  std::atomic<uint64_t> Used = 0;
  std::atomic<PhaseObserver *> PhaseObs = nullptr;
  // Test-only mid-trace pause hook; see setTracerPauseHook. Empty in
  // production.
  std::function<void()> TracerPauseHook;
  // Test-only sweep-entry pause hook; see setSweepPauseHook. Empty in
  // production.
  std::function<void()> SweepPauseHook;
  // Test-only pause hook; see setPreTerminationCASHook. Empty in production.
  std::function<void()> PreTerminationCASHook;
  // Test-only token-held/heap-Idle pause hook; see setPreCycleHook. Empty in
  // production.
  std::function<void()> PreCycleHook;

  // Non-owning back-reference to the owning Controller, which holds the
  // mutator stack registry markRoots delegates to. Null for a standalone
  // Allocator (no controller, no registered stacks -- e.g. the GC unit tests
  // that construct a bare Allocator).
  Controller *Ctrl = nullptr;

  std::mutex HeapMutex{};
  std::vector<Runtime::Instance::TableInstance *> Heaps;
  // Old table Refs buffers retired by a reallocating growTable (round-2 A3),
  // freed at the next stop-the-world root scan. Guarded by HeapMutex.
  std::vector<std::vector<RefVariant>> RetiredTableBuffers;
  std::mutex GlobalMutex{};
  std::vector<Runtime::Instance::GlobalInstance *> Globals;

  std::mutex ElemMutex{};
  std::vector<Runtime::Instance::ElementInstance *> Elems;

  std::mutex ExceptionMutex{};
  std::vector<Runtime::Instance::ExceptionInstance *> Exceptions;

  std::mutex HostRootsMutex{};
  std::vector<uint8_t *> HostRoots;

  std::mutex ScopedRootsMutex{};
  std::vector<uint8_t *> ScopedRoots;

  // All-object list + membership index. Lock order is GrayMutex -> AllMutex
  // (markGray is the only path that holds both); allocate() takes them
  // sequentially, never nested, and sweep takes AllMutex alone.
  std::mutex AllMutex{};
  Header *AllHead = nullptr; // guarded by AllMutex
  ObjectIndex Index;         // guarded by AllMutex

  // Which Marked colour currently means Black. Flipped once per sweep, which
  // relabels every survivor White for the next cycle -- the parity form of the
  // old Black/White set-pointer swap.
  std::atomic<uint8_t> BlackParity = ColorMarked0;

  std::mutex GrayMutex{};
  std::condition_variable GrayNotEmptyCV;
  Header *GrayHead = nullptr; // intrusive LIFO work list, guarded by GrayMutex

  std::mutex GCMutex{};
  std::condition_variable GCCV;

  std::atomic<std::chrono::steady_clock::time_point> NextGC =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);

  std::vector<std::thread> Collectors;
};

/// RAII scope that pins managed references as GC host roots for as long as they
/// are NOT reachable from any GC-scanned value stack -- detached async call
/// parameters between launch and the worker pushing them onto its stack, or
/// host return values between production and transfer to the value stack.
/// Backed by the allocator's scoped-root store (retainScopedRef/
/// releaseScopedRef, matched by pointer identity), which the host-facing
/// releaseAllRefs() cannot consume. Every pinned reference is released when
/// the scope is destroyed. Move-only.
class BoundaryRoots {
public:
  BoundaryRoots() noexcept = default;
  explicit BoundaryRoots(Allocator &A) noexcept : Alloc(&A) {}
  BoundaryRoots(BoundaryRoots &&O) noexcept
      : Alloc(O.Alloc), Roots(std::move(O.Roots)) {
    O.Alloc = nullptr;
  }
  BoundaryRoots &operator=(BoundaryRoots &&O) noexcept {
    if (this != &O) {
      release();
      Alloc = O.Alloc;
      Roots = std::move(O.Roots);
      O.Alloc = nullptr;
    }
    return *this;
  }
  BoundaryRoots(const BoundaryRoots &) = delete;
  BoundaryRoots &operator=(const BoundaryRoots &) = delete;
  ~BoundaryRoots() noexcept { release(); }

  /// Pin a reference: retain it as a host root now, release it when this scope
  /// ends. No-op for a null reference (nothing to keep alive).
  void pin(const RefVariant &Ref) noexcept {
    if (Alloc == nullptr || Ref.isNull()) {
      return;
    }
    Alloc->retainScopedRef(Ref);
    Roots.push_back(Ref);
  }

  /// Release every pinned reference now (also run by the destructor).
  void release() noexcept {
    if (Alloc != nullptr) {
      for (const auto &R : Roots) {
        Alloc->releaseScopedRef(R);
      }
    }
    Roots.clear();
  }

private:
  Allocator *Alloc = nullptr;
  std::vector<RefVariant> Roots;
};

// Pin the managed (non-null reference) entries of an index-aligned parameter
// list into a BoundaryRoots at an async launch, so they stay rooted through the
// window before the worker moves them onto its registered stack. The same
// BoundaryRoots -- carried into the worker as the Async's keep-alive --
// releases them when the invocation completes, so the retain and the release
// form one balanced RAII pair spanning the launch and the worker.
inline void pinParamRootsInto(BoundaryRoots &BR, Span<const ValVariant> Params,
                              Span<const ValType> ParamTypes) noexcept {
  for (size_t I = 0; I < ParamTypes.size() && I < Params.size(); ++I) {
    if (ParamTypes[I].isRefType()) {
      BR.pin(Params[I].get<RefVariant>());
    }
  }
}

} // namespace GC
} // namespace WasmEdge
