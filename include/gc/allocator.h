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
#include <algorithm>
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
  /// the sweep's black/white role swap is a single parity flip that touches no
  /// object. Black is `Marked(BlackParity)`; White is
  /// `Marked(1 - BlackParity)`.
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

  /// Process-monotonic identity of this allocator: drawn from a 64-bit
  /// counter, so no later allocator repeats it within any realistic process
  /// lifetime (the counter wraps after 2^64 constructions), not even one built
  /// at the same address. Ownership stamps that must outlive this allocator
  /// (ModuleInstance::getGCRootsOwnerId) carry the id, not the address, so a
  /// stale stamp cannot match a successor built in the same storage.
  uint64_t getId() const noexcept { return Id; }

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
      // Heap limit reached (or the system allocator failed): reclaim garbage
      // now and retry before failing the guest. Out of line -- the hot path
      // above stays a CAS.
      Pointer = allocateUnderPressure(Total);
      if (!Pointer) {
        return nullptr;
      }
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

  /// True while no collection cycle is in flight (the heap is Idle).
  bool isIdle() const noexcept {
    return CurrentGCState.load(std::memory_order_acquire) == GCState::Idle;
  }

  /// Heap limit: the byte total (headers included) past which an allocation
  /// first tries a pressure collection and, if that reclaims too little, fails
  /// (GCAllocationFailed in the guest). Defaults to 1 GiB.
  uint64_t getHeapLimit() const noexcept {
    return Threshold.load(std::memory_order_relaxed);
  }
  void setHeapLimit(uint64_t Bytes) noexcept {
    Threshold.store(Bytes, std::memory_order_relaxed);
  }

  // Back-reference to the owning Controller (the GC concurrency coordinator),
  // or nullptr for a standalone Allocator with no controller -- a bare
  // unit-test allocator, or a C-API table created without an executor.
  // TableInstance:: growTable reaches the exclusive-op arbiter and the setup
  // handshake through this: a null controller means there is no mutator
  // registry to stop, so growth keeps the plain (non-STW) path.
  Controller *getController() noexcept { return Ctrl; }
  const Controller *getController() const noexcept { return Ctrl; }

  void setManualGC(bool Enable) noexcept {
    EnableManualGC.store(Enable, std::memory_order_release);
  }

  void setPhaseObserver(PhaseObserver *Obs) noexcept {
    PhaseObs.store(Obs, std::memory_order_release);
  }

  // Test-only pause hooks; see TestHooks for where each one fires. Set them
  // only while the heap is Idle, so the store does not race a worker's read.
  void setTracerPauseHook(std::function<void()> Fn) noexcept {
    Hooks.TracerPause = std::move(Fn);
  }
  void setSweepPauseHook(std::function<void()> Fn) noexcept {
    Hooks.SweepPause = std::move(Fn);
  }
  void setPreTerminationCASHook(std::function<void()> Fn) noexcept {
    Hooks.PreTerminationCAS = std::move(Fn);
  }
  void setPreCycleHook(std::function<void()> Fn) noexcept {
    Hooks.PreCycle = std::move(Fn);
  }
  void setPressureRetryHook(std::function<void()> Fn) noexcept {
    Hooks.PressureRetry = std::move(Fn);
  }

  // Test-only read of the stale-termination-win reject count; see
  // StaleTerminationRejects.
  uint64_t debugStaleTerminationRejects() const noexcept {
    return StaleTerminationRejects.load(std::memory_order_acquire);
  }

  // Test-only, synchronized query: is \p Global in this allocator's root set
  // (attached by setAllocator / claimForRegister)? Read under GlobalMutex, so
  // a test may poll it while another thread attaches. Unlike the pause hooks
  // it needs no idle executor, and it adds no state to an Allocator: it is a
  // read of the root vector the collector already keeps.
  bool
  debugHasGlobalRoot(const Runtime::Instance::GlobalInstance &Global) noexcept {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    return std::find(Globals.begin(), Globals.end(), &Global) != Globals.end();
  }

  // ScanNativeStack conservatively scans the current thread's native stack for
  // roots. Only AOT callers need it (a ref may live solely in a register/stack
  // slot); the interpreter keeps every live ref on a tracked value stack.
  void autoCollect(bool ScanNativeStack = false) noexcept;

  bool manualCollect(bool ScanNativeStack = false) noexcept;

  template <typename T> void writeBarrier(const T &Val) const noexcept {
    if (likely(!barrierArmed())) {
      return;
    }
    const_cast<Allocator *>(this)->doWriteBarrier(getPointer(Val));
  }

  template <typename T>
  void bulkWriteBarrier(Span<const T> Slots) const noexcept {
    if (likely(!barrierArmed())) {
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
    if (likely(!barrierArmed())) {
      return;
    }
    const_cast<Allocator *>(this)->doWriteBarrier(loadPointerWordRelaxed(Slot));
  }

  void bulkWriteBarrierSlots(Span<const ValVariant> Slots) const noexcept {
    if (likely(!barrierArmed())) {
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
  /// consumed by releaseAllRefs(): a host call to releaseAllRefs() must not
  /// unpin a reference that a live boundary scope still holds.
  void retainScopedRef(const RefVariant &Ref) noexcept;
  void releaseScopedRef(const RefVariant &Ref) noexcept;

  /// Serialize a structural table-root mutation against the root scan, which
  /// reads every registered table's Refs vector under this same TableMutex.
  /// growTable reallocates that vector; holding this lock across the resize
  /// stops the scan iterating a freed buffer. In-place slot writes
  /// (setRefs/fillRefs/setRefAddr) keep the buffer stable and don't need it --
  /// benign conservative-read races the scan tolerates.
  [[nodiscard]] std::unique_lock<std::mutex> lockTableRoots() noexcept {
    return std::unique_lock<std::mutex>(TableMutex);
  }

  /// Retire a table's old Refs buffer instead of freeing it in a reallocating
  /// growTable. A concurrent mutator reader (AOT/interpreter table.get/set
  /// holding the old DataPtr) may still be accessing that buffer, so freeing it
  /// there is a use-after-free. The retired buffers are freed at the next
  /// stop-the-world root scan (scanSharedRoots), when every mutator is parked
  /// at a safepoint or NativeRunning -- a table access is safepoint-free, so no
  /// acked mutator holds an in-flight pointer into a retired buffer. The caller
  /// MUST hold lockTableRoots() (growTable does), so this pushes without
  /// re-locking TableMutex. Reserve BEFORE the move: `Old` is only a reference
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

  // Test seam: drives the root-scan shading path by hand to prove markGray()
  // is quiet outside a marking phase. There is no public route to it --
  // writeBarrier() gates itself, so it cannot reach the gate inside markGray()
  // -- and widening the shading API for a test would defeat the point of the
  // friendship above. Defined only in
  // test/gc/GCAllocatorTest.cpp.
  friend struct RootShadeTestSeam;
  // Test seam: enters the pressure slow path with room already on the heap
  // (the shape a concurrent cycle produces), which allocate() itself only
  // reaches through a race. Defined only in
  // test/gc/GCAllocatorTest.cpp.
  friend struct PressureTestSeam;

  // Shade one value-stack slot. Thin wrapper so the Controller's precise
  // self-scan does not need getPointer()/markGray() individually.
  void markGrayRoot(const ValVariant &V) noexcept { markGray(getPointer(V)); }

  // Spawn the collector worker threads for both constructors. If a spawn
  // fails, stop and join the threads already started, then rethrow.
  void startWorkers();

  // Body of one collector worker thread: park until there is marking work, a
  // termination opportunity, or Stop; then trace one gray object or attempt
  // termination. Returns on Stop.
  void workerLoop();

  // Termination attempt by a worker that saw an empty gray list under
  // MarkingGray with no tracer in flight: win TerminationOwner, run STW #2,
  // re-check quiescence and, if it holds, sweep via completeCycle(). Every
  // path that does not sweep releases TerminationOwner.
  void tryTerminate();

  // Tail of a successful termination: sweep, release TerminationOwner and the
  // collection's exclusive token, then publish Idle and wake the coordinator.
  void completeCycle();

  // Trace one popped gray object: shade its children, blacken it, and drop
  // the ActiveTracers count taken when it was popped.
  void traceObject(Header *H);

  // Mark every registered root gray (tables, globals, element segments,
  // exception payloads, host roots, and -- when ScanNativeStack -- the current
  // thread's native stack). Used only by legacyManualCollect() (Ctrl ==
  // nullptr, so there are no registered value stacks); the coordinator-driven
  // path scans stacks via the handshake and shared roots via
  // snapshotSharedRootsInto().
  void markRoots(bool ScanNativeStack) noexcept;

  // Shared (non-stack) roots: table InitValue+Refs, globals, element segments,
  // exception payloads, host roots -- each under its own mutex. Factored out so
  // markRoots() and snapshotSharedRootsInto() cannot diverge and drop a root
  // type. Does NOT notify a phase.
  void scanSharedRoots() noexcept;

  // --- Coordinator (Controller::collect) primitives -------------------------
  // The steps of one cycle that the Controller drives across the setup
  // handshake. All are Controller-only (friend); nothing outside the GC drives
  // a cycle by hand.

  // Admission gate + Idle->MarkingRoot CAS. For an auto cycle (Manual == false)
  // it also honours the manual-GC toggle and the NextGC schedule. Returns false
  // if a cycle must not start (already collecting / manual-gated / not due).
  bool beginCycle(bool Manual) noexcept;

  // Shared-root snapshot taken while all mutators are quiesced (globals,
  // tables, element segments, exceptions, host roots). Value stacks are NOT
  // scanned here; the handshake scans them (self-scan of parked Running
  // mutators, scanNonRunningRoots for the rest, and the coordinator's own
  // selfScanInto). Fires MarkRootStart.
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

  // Collection path for the standalone allocator (Ctrl == nullptr, no
  // registry/coordinator): markRoots on this thread, publish, wait.
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

  // Reserve N bytes against the heap limit and take them from the system
  // allocator. On nullptr, *LimitRejected (when given) says which of the two
  // refused: true for the limit (Used + N > Threshold at the reservation CAS),
  // false for the system allocator.
  [[nodiscard]] uint8_t *doAllocate(uint32_t N,
                                    bool *LimitRejected = nullptr) noexcept;
  // Slow path of allocate() after doAllocate rejected N bytes: while the
  // rejection is the heap limit (Used + N > Threshold) and a controller
  // exists, run a full collection through the coordinator (bypassing the
  // AutoGCInterval schedule and the manual-GC toggle -- the alternative is a
  // GCAllocationFailed trap with reclaimable garbage on the heap) and retry.
  // Every pass retries the reservation first and acts on ITS verdict, so a
  // limit rejection that comes from losing the race for room this thread had
  // just observed is answered by a collection like any other. Up to
  // MaxPressureCycles cycles: an object allocated since the previous cycle is
  // born gray and survives its first collection, so garbage created within the
  // last cycle is only reclaimable by the second. Returns nullptr when the
  // limit still cannot be met after those (live set genuinely at the limit),
  // when the system allocator fails, or for a standalone allocator with no
  // controller.
  [[nodiscard]] uint8_t *allocateUnderPressure(uint32_t N) noexcept;

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

  // Gate shared by the four write-barrier entry points: true while a store
  // must shade. Armed for MarkingRoot as well as MarkingGray, because the
  // collector can enter either state between this load and the caller's store;
  // gating on MarkingGray alone would let a store during root marking escape
  // shading (a lost object). markGray no-ops non-White refs, so shading during
  // MarkingRoot is merely conservative. Quiet on Idle and on Sweeping: the
  // termination owner closes the barrier with the MarkingGray -> Sweeping CAS
  // before the sweep begins, so shading a White object mid-sweep would only be
  // wasted work (markGray declines it anyway).
  bool barrierArmed() const noexcept {
    const GCState St = CurrentGCState.load(std::memory_order_acquire);
    return !(St == GCState::Idle || St == GCState::Sweeping);
  }

  // --- Allocation-membership index ------------------------------------------
  // Maps an allocated block's Header* to "is a live managed object". The
  // conservative root scan must decide whether an arbitrary aligned word (a
  // native-stack slot, a table entry) points at a real object BEFORE touching
  // its header -- reading Color at a bogus address could fault or read
  // unrelated memory -- so this index, not the header, is the validity oracle.
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

    // Capacity of the first table, allocated by the first insert().
    static constexpr size_t InitialCapacity = 1024;

    // Cap entries; nullptr = empty, tombstone() = dead.
    Header **Slots = nullptr;
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
  // that object's children are shaded and it is blackened. An empty gray list
  // is NOT quiescence: a worker may hold a popped-but-untraced object whose
  // only-reachable-through-it children are still White. Termination requires
  // GrayHead == nullptr && ActiveTracers == 0 observed under GrayMutex.
  std::atomic<uint32_t> ActiveTracers = 0;
  // Single-owner election for STW #2 termination: a worker CASes 0->1 to become
  // the termination owner; the loser workers keep tracing/parking. Reset to 0
  // after the owner sweeps (or aborts termination on a failed re-check).
  std::atomic<uint32_t> TerminationOwner = 0;
  // Exclusive-operation owner generation of the in-flight Controller-backed
  // collection. Controller::collect() stamps it after winning
  // tryBeginExclusiveOp(OwnedCollecting) and before wakeCollectors(), so it is
  // set before any worker can observe MarkingGray. The sweep-completing worker
  // reads it BEFORE runSweepAndSwap() publishes Idle (after which a fresh
  // collect() could overwrite it) and releases the token across the STW #2
  // handoff. Zero and unused for a standalone Allocator (Ctrl == nullptr),
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

  // Test-only pause points, all empty in production. Each one lets a test
  // hold a thread at an exact point of the protocol, with no allocator lock
  // held, to stage an interleaving deterministically:
  //   * TracerPause: a worker has popped a gray object (ActiveTracers is
  //     raised) but not yet shaded its children. Proves termination waits for
  //     an in-flight tracer.
  //   * SweepPause: top of runSweepAndSwap(). The state is Sweeping and STW #2
  //     has released the mutators, so a test can allocate during the sweep.
  //     (The SweepStart observer cannot do this: it fires under GrayMutex,
  //     which allocate() must take.)
  //   * PreTerminationCAS: a worker has decided to terminate but has not yet
  //     tried the TerminationOwner CAS: the preemption window that produces a
  //     stale ownership win.
  //   * PreCycle: a controller-backed collection holds the exclusive token but
  //     has not yet left Idle, so the write barrier is still quiet. A grower
  //     that pops its table.grow initializer here must keep that ref alive
  //     through its scoped-root pin, not through the barrier.
  //   * PressureRetry: before each doAllocate() retry in
  //     allocateUnderPressure(), so a test can take the room the caller just
  //     saw and prove a lost race is answered by a collection.
  struct TestHooks {
    std::function<void()> TracerPause;
    std::function<void()> SweepPause;
    std::function<void()> PreTerminationCAS;
    std::function<void()> PreCycle;
    std::function<void()> PressureRetry;
  };
  TestHooks Hooks;

  // Non-owning back-reference to the owning Controller, which holds the
  // mutator stack registry and drives controller-backed cycles. Null for a
  // standalone Allocator (no controller, no registered stacks -- e.g. the GC
  // unit tests that construct a bare Allocator).
  Controller *Ctrl = nullptr;
  /// See getId(). Drawn from a process-wide counter in the constructor.
  const uint64_t Id;

  std::mutex TableMutex{};
  std::vector<Runtime::Instance::TableInstance *> Tables;
  // Old table Refs buffers retired by a reallocating growTable, freed at the
  // next stop-the-world root scan. Guarded by TableMutex.
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
  // sequentially, never nested, and sweep takes AllMutex alone. See the
  // Controller class comment for the full lock order.
  std::mutex AllMutex{};
  Header *AllHead = nullptr; // guarded by AllMutex
  ObjectIndex Index;         // guarded by AllMutex

  // Which Marked colour currently means Black. Flipped once per sweep, which
  // relabels every survivor White for the next cycle.
  std::atomic<uint8_t> BlackParity = ColorMarked0;

  std::mutex GrayMutex{};
  std::condition_variable GrayNotEmptyCV;
  Header *GrayHead = nullptr; // intrusive LIFO work list, guarded by GrayMutex

  std::mutex GCMutex{};
  std::condition_variable GCCV;

  // Minimum time between two auto (non-manual) cycles; see beginCycle.
  static constexpr std::chrono::seconds AutoGCInterval{1};
  // Most collections allocateUnderPressure() runs for one allocation.
  static constexpr int MaxPressureCycles = 2;

  std::atomic<std::chrono::steady_clock::time_point> NextGC =
      std::chrono::steady_clock::now() + AutoGCInterval;

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
