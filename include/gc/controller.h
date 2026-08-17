// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "gc/allocator.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace WasmEdge {
namespace GC {

/// Permanent, executor-owned owner of the GC concurrency protocol. It owns
/// the Allocator, the mutator stack registry, the admission gate, safe
/// points, and the teardown drain. The Allocator owns only heap
/// allocation/marking/sweeping.
class Controller {
public:
  Controller() : Alloc(*this) {}
  ~Controller() noexcept;

  Controller(const Controller &) = delete;
  Controller(Controller &&) = delete;
  Controller &operator=(const Controller &) = delete;
  Controller &operator=(Controller &&) = delete;

  Allocator &getAllocator() noexcept { return Alloc; }
  const Allocator &getAllocator() const noexcept { return Alloc; }

  // Obs is a non-owning raw pointer; the caller must keep it alive for as long
  // as this Controller (and therefore its Allocator) may run collections --
  // for an Executor-owned Controller, that is the whole Executor lifetime.
  void setPhaseObserver(PhaseObserver *Obs) noexcept {
    Alloc.setPhaseObserver(Obs);
  }

  // Execution state of a registered mutator thread, tracked so the coordinator
  // knows which threads can reach a safe point (and must ack the handshake) and
  // which have stable stacks it scans in place.
  enum class MutatorState : uint8_t {
    Running,       // executing guest code: MUST reach a safe point and ack.
    NativeRunning, // inside a host callback: guest stacks stable, scan in
                   // place.
    Blocked,       // blocked on a runtime CV (atomic.wait): stacks stable too.
  };

  // RAII registration for one value stack (one StackManager). Deregisters on
  // destruction. Reentrant: nested StackManagers on one OS thread each hold
  // their own Registration; the registry groups them per thread. A reentrant
  // registration (guest re-entered from a host callback, entry already present)
  // records the entry's prior state and restores it on reset -- see
  // registerStack.
  class Registration {
  public:
    Registration() noexcept = default;
    Registration(Controller *C, std::vector<ValVariant> *S) noexcept
        : Ctrl(C), Stack(S) {}
    Registration(Registration &&O) noexcept
        : Ctrl(O.Ctrl), Stack(O.Stack), HasSavedState(O.HasSavedState),
          SavedState(O.SavedState) {
      O.Ctrl = nullptr;
      O.Stack = nullptr;
      O.HasSavedState = false;
    }
    Registration &operator=(Registration &&O) noexcept {
      if (this != &O) {
        reset();
        Ctrl = O.Ctrl;
        Stack = O.Stack;
        HasSavedState = O.HasSavedState;
        SavedState = O.SavedState;
        O.Ctrl = nullptr;
        O.Stack = nullptr;
        O.HasSavedState = false;
      }
      return *this;
    }
    Registration(const Registration &) = delete;
    Registration &operator=(const Registration &) = delete;
    ~Registration() noexcept { reset(); }

  private:
    friend class Controller;
    void reset() noexcept;
    Controller *Ctrl = nullptr;
    std::vector<ValVariant> *Stack = nullptr;
    // Set for a reentrant registration: the entry's state before this nested
    // guest stack made it Running, restored when this registration is reset.
    bool HasSavedState = false;
    MutatorState SavedState = MutatorState::Running;
  };

  Registration registerStack(std::vector<ValVariant> &Stack) noexcept;

  // --- AOT shadow roots (phase-1 prototype) ---------------------------------
  // A managed ref that AOT/native code holds only in a register or a native
  // stack slot is invisible to the precise value-stack scan. Before a call or a
  // transition to a remotely-scannable state, generated code spills such refs
  // into a ShadowFrame whose Slots point at the compiled frame's own spill
  // area; the collector walks the chain for NativeRunning/Blocked threads
  // (scanNonRunningRoots) and for the coordinator's own frames (selfScanInto).
  // Slots is caller-owned storage; Count is how many leading slots are live.
  struct ShadowFrame {
    ShadowFrame *Prev = nullptr;
    uint32_t Count = 0;
    const ValVariant *Slots = nullptr;
  };

  // Stable-address anchor for one thread's shadow-frame chain. Heap-allocated
  // and owned by that thread's Entry via unique_ptr, so its address is FIXED
  // for the registration lifetime even as the Entries vector relocates on
  // emplace_back/erase. Generated code and Fault checkpoints retain a pointer
  // to THIS cell, never into the Entry (whose address is valid only while
  // RegistryMtx is held). IncId stamps the registration lifetime so a pointer
  // that outlives deregistration + thread-id reuse is detectably stale.
  struct ShadowHead {
    std::atomic<ShadowFrame *> Head{nullptr};
    uint64_t IncId = 0;
  };

  // Return the calling thread's stable shadow-head cell, or nullptr if the
  // thread holds no registered stack. The returned pointer stays valid for the
  // whole registration lifetime regardless of Entries relocation.
  ShadowHead *currentShadowHead() noexcept;

  // --- Auxiliary thread roots -----------------------------------------------
  // Managed refs a thread holds outside every registered value stack AND
  // outside the AOT shadow chain. Today that is exactly the executor's
  // pending-exception payload: throwException/proxyThrow copy the payload off
  // the value stack and then erase those slots, and the throwing frame's shadow
  // frame is popped as that frame unwinds -- so for the whole propagation
  // across the native frames nothing else roots those refs.
  //
  // The shadow chain cannot carry them: its nodes are popped strictly LIFO by
  // the frames that pushed them, and a pending exception outlives the frame
  // that raised it. Hence a separate per-thread slot, scanned wherever the
  // registered stacks are.
  //
  // A provider is a plain function pointer returning the CALLING thread's
  // aux-root vector (a thread_local in the executor, so its address is fixed
  // for the thread's lifetime). registerStack invokes it on the joining thread
  // and caches the result in that thread's Entry, so no publication is needed
  // on the throw path and no future StackManager site can forget to opt in.
  // Set once by the owning Executor before any mutator exists; left null by the
  // bare Controller of the unit tests, which simply contributes no aux roots.
  //
  // The vector's CONTENTS may only be mutated under lockStackRoots() (every
  // scan below holds RegistryMtx), so a scan never iterates a buffer being
  // reallocated or emptied.
  using AuxRootProvider = const std::vector<ValVariant> *(*)() noexcept;
  void setAuxRootProvider(AuxRootProvider P) noexcept { AuxProvider = P; }

  // Stable address of the stop flag, threaded into the compiled ExecCtx so
  // generated code polls it inline at loop back-edges (see stopRequested()). The
  // controller outlives every compiled entry that reads it.
  std::atomic<bool> *stopFlagPtr() noexcept { return &StopFlag; }

  // RAII push/pop of a shadow frame onto a thread's chain. Constructed while
  // the owner thread is Running (chain mutation is only legal in a
  // non-scannable state); the publish uses release so a remote scanner's
  // acquire load observes a fully-initialized frame. Pops on destruction.
  // Models exactly what generated code emits (a linked push/pop of a
  // stack-allocated node); a test or the codegen backend supplies the storage.
  class ShadowScope {
  public:
    ShadowScope(ShadowHead *H, ShadowFrame *F, uint32_t Count,
                const ValVariant *Slots) noexcept
        : Head(H), Frame(F) {
      Frame->Count = Count;
      Frame->Slots = Slots;
      Frame->Prev = Head->Head.load(std::memory_order_relaxed);
      Head->Head.store(Frame, std::memory_order_release);
    }
    ~ShadowScope() noexcept {
      Head->Head.store(Frame->Prev, std::memory_order_release);
    }
    ShadowScope(const ShadowScope &) = delete;
    ShadowScope &operator=(const ShadowScope &) = delete;

  private:
    ShadowHead *Head;
    ShadowFrame *Frame;
  };

  // RAII: marks the calling thread NativeRunning for the duration of a host
  // call, so the coordinator scans its (stable) value stacks in place instead
  // of waiting for an ack native code can never deliver. On destruction it
  // restores Running and performs a re-entry gcSafepoint() -- a handshake may
  // have started while the thread was native, and it must acknowledge it before
  // resuming guest mutation. Non-copyable.
  class NativeScope {
  public:
    explicit NativeScope(Controller &C) noexcept;
    ~NativeScope() noexcept;
    NativeScope(const NativeScope &) = delete;
    NativeScope &operator=(const NativeScope &) = delete;

  private:
    Controller *Ctrl;
    // Prior state, restored on exit. Save/restore (rather than unconditionally
    // returning to Running) keeps nested host->guest->host calls correct: the
    // inner scope restores NativeRunning, so the still-active outer native call
    // is not mismarked Running -- which would make the coordinator wait for an
    // ack this native-stuck thread can never deliver (a hang).
    MutatorState Prev;
  };

  // Coordinator entry point: run one collection cycle, driving the two-STW
  // setup handshake (STW #1) -- bump the generation, wait for every other
  // Running mutator to park/ack, snapshot the roots while everyone is quiesced,
  // publish MarkingGray, release, then wait for the workers to complete the
  // cycle. Manual == true bypasses the auto admission gate (schedule / manual
  // toggle). ScanNative requests a conservative native-stack scan of the
  // coordinator's own roots (the AOT execution model keeps operand-stack roots
  // natively); Manual and ScanNative are orthogonal -- an AOT-manual collect
  // passes (true, true). Returns false if no cycle started (already collecting
  // / gated / not due).
  bool collect(bool Manual, bool ScanNative) noexcept;

  template <typename Fn> void forEachStackRoot(Fn &&F) noexcept {
    std::lock_guard<std::mutex> L(RegistryMtx);
    for (const auto &E : Entries) {
      for (auto *S : E.Stacks) {
        for (const auto &V : *S) {
          F(V);
        }
      }
      // AOT shadow roots (refs spilled by compiled code around calls). Covered
      // here too so the markRoots/manualCollect entry point sees them, not just
      // the collect() coordinator path (selfScanInto/scanNonRunningRoots).
      if (E.Shadow) {
        for (auto *Fr = E.Shadow->Head.load(std::memory_order_acquire);
             Fr != nullptr; Fr = Fr->Prev) {
          for (uint32_t I = 0; I < Fr->Count; ++I) {
            F(Fr->Slots[I]);
          }
        }
      }
      // Auxiliary roots (the executor's pending-exception payload). Empty
      // whenever no exception is propagating, so this costs nothing on the
      // normal path.
      if (E.AuxRoots) {
        for (const auto &V : *E.AuxRoots) {
          F(V);
        }
      }
    }
  }

  [[nodiscard]] std::unique_lock<std::mutex> lockStackRoots() noexcept {
    return std::unique_lock<std::mutex>(RegistryMtx);
  }

  // Total number of registered value stacks across every thread (not the number
  // of thread entries): a reentrant thread with two nested StackManagers counts
  // two.
  size_t debugActiveStackCount() noexcept {
    std::lock_guard<std::mutex> L(RegistryMtx);
    size_t N = 0;
    for (const auto &E : Entries) {
      N += E.Stacks.size();
    }
    return N;
  }

  // Test-only counter reads for deterministic assertions (no timing windows).
  // ParkedMutators counts threads inside gcSafepoint()'s park (this includes
  // a joiner held in registerStack's admission gate, which parks via
  // gcSafepoint); RegisteredStacks counts live stack registrations.
  uint32_t debugParkedMutators() noexcept {
    return ParkedMutators.load(std::memory_order_acquire);
  }
  uint32_t debugRegisteredStacks() noexcept {
    return RegisteredStacks.load(std::memory_order_acquire);
  }

  // Test-only read of the calling thread's cached auxiliary root vector; null
  // if the thread holds no registered stack or no provider is installed. Lets a
  // test prove the executor's pending-exception payload really reached the
  // registry without reaching into Executor's private thread_locals.
  const std::vector<ValVariant> *debugAuxRoots() noexcept {
    std::lock_guard<std::mutex> L(RegistryMtx);
    if (auto *E = findEntry(std::this_thread::get_id())) {
      return E->AuxRoots;
    }
    return nullptr;
  }

  // Test-only read of the calling thread's registry State (Running /
  // NativeRunning / Blocked). Returns Running if the thread has no entry.
  MutatorState debugSelfState() noexcept {
    std::lock_guard<std::mutex> L(RegistryMtx);
    if (auto *E = findEntry(std::this_thread::get_id())) {
      return E->State;
    }
    return MutatorState::Running;
  }

  // Capture the calling thread's registry State at a compiled-call boundary so
  // the abnormal-fault recovery (restoreStateAfterFault) can undo any
  // NativeScope transition a longjmp later skips. Returns Running if the thread
  // has no entry. Read under RegistryMtx (State is coordinator-visible), so call
  // it only from a normal (non-signal) context such as the enterFunction
  // compiled boundary -- which is low frequency (interpreter->compiled entry),
  // not the compiled->compiled hot path.
  MutatorState currentMutatorState() noexcept {
    std::lock_guard<std::mutex> L(RegistryMtx);
    if (auto *E = findEntry(std::this_thread::get_id())) {
      return E->State;
    }
    return MutatorState::Running;
  }

  // Restore this thread's registry State to BoundaryState after an abnormal
  // fault (longjmp) skipped one or more NativeScope destructors, which can
  // strand the entry in NativeRunning (remotely scannable) when the boundary was
  // Running. Runs in the normal (non-signal) recovery context in helper.cpp, so
  // taking RegistryMtx and parking are legal here (unlike the signal-safe head
  // truncation in emitFault). Restoring TO a scannable state
  // (NativeRunning/Blocked) is a plain locked store (no window). Restoring TO
  // Running uses the same StopFlag-gated transition NativeScope::~NativeScope
  // uses, so it never exposes a transient Running-but-unacked state to the
  // coordinator (the TOCTOU window between waitForAcks and scanNonRunningRoots).
  void restoreStateAfterFault(MutatorState BoundaryState) noexcept;

  // Test-only hook: invoked by the final drain-satisfying releaser (the
  // decrement that brings OutstandingLeases/RegisteredStacks/ParkedMutators to
  // zero) WHILE holding DrainMtx, immediately after DrainCV.notify_all() and
  // before the lock is released. Lets a test prove the notify + return happen
  // inside the critical section (GCThread.DrainDecrementUnderDrainMtx). Empty
  // in production.
  void setDrainReleaseHook(std::function<void()> Fn) noexcept {
    DrainReleaseHook = std::move(Fn);
  }

  // --- Admission gate / cooperative safe points -----------------------------

  // Hot poll for the interpreter. Relaxed: a stale false only delays
  // this mutator's arrival at the safe point by one poll; the gcSafepoint()
  // fast path re-reads with acquire before doing anything observable.
  bool stopRequested() const noexcept {
    return StopFlag.load(std::memory_order_relaxed);
  }

  // Cooperative safe point. If a handshake is in flight, self-scan this
  // thread's roots (registered value stacks precisely, native stack
  // conservatively), acknowledge the current generation, and park until the
  // coordinator releases it. No-op otherwise.
  void gcSafepoint() noexcept;

  // Test-only hooks driving a bare handshake without a full collect() cycle;
  // the real coordinator (collect()) uses the private primitives directly.
  uint64_t debugBeginHandshake() noexcept { return beginHandshake(); }
  bool debugAllAcked(uint64_t Gen) noexcept { return allAcked(Gen); }
  void debugEndHandshake(uint64_t Gen) noexcept { endHandshake(Gen); }

  // --- Exclusive-operation owner (grow <-> collect arbiter) -----------------
  // A per-controller single-owner arbiter. A stop-the-world operation (collection,
  // wired in E1.1; table growth, wired in E1.2) must own the token before it may
  // drive a handshake, so at most one such operation is ever in flight -- growth
  // and collection can never both stop the world at once. collect() takes it
  // non-blocking (tryBeginExclusiveOp -- a losing collection is skippable) and
  // growTable blocking (beginExclusiveOp -- a grower must eventually grow).
  // State transitions and the FIFO waiter queue are guarded by ExclusiveMtx,
  // which is OUTERMOST in the lock order (ExclusiveMtx -> RegistryMtx ->
  // HandshakeMtx). The election mirrors Allocator::TerminationOwner's
  // single-owner CAS, generalized to a blocking, starvation-free acquire.
  struct ExclusiveOwner {
    enum class State { Free, OwnedCollecting, OwnedGrowing };
  };

  // Acquire the exclusive token for a Want operation. On the winner path (token
  // Free and not Closing) the caller becomes the owner immediately, its fresh
  // owner generation is written to OutGen, and this returns true. On the loser
  // path the caller freezes its roots, publishes a pre-ackable Blocked state
  // under RegistryMtx (so a handshake the current owner raises counts it without
  // waiting), enqueues a FIFO ticket, and parks on TokenCV until the reserved
  // handoff grants it the token (returns true, OutGen set) or the controller
  // starts Closing (returns false). Exactly-once release is via endExclusiveOp
  // with the OutGen this returned. noexcept: on the mutator hot path.
  bool beginExclusiveOp(ExclusiveOwner::State Want, uint64_t &OutGen) noexcept;

  // Non-blocking acquire of the exclusive token for a Want operation. If the
  // token is Free and the controller is not Closing, take it immediately (mint a
  // fresh owner generation into OutGen, return true); otherwise return false at
  // once WITHOUT queuing or parking. Used by the collector (Controller::collect):
  // a losing collection is skippable, so it must NOT enqueue as a FIFO
  // grow-ticket -- the reserved handoff relabels every granted ticket
  // OwnedGrowing (tickets carry no per-ticket Want), which would trip
  // endExclusiveOp's assuming(ExclState == Have) when the collector later
  // released OwnedCollecting. Grows (E1.2) still use the blocking
  // beginExclusiveOp. Release via endExclusiveOp with the OutGen this returned.
  bool tryBeginExclusiveOp(ExclusiveOwner::State Want,
                           uint64_t &OutGen) noexcept;

  // Release the exclusive token previously acquired with owner generation
  // OwnerGen and state Have. If a grower is queued the token is handed to the
  // oldest ticket (FIFO reserved handoff -- never returned to Free in between,
  // so a newcomer cannot jump the queue); otherwise the token goes Free. Wakes
  // every parked waiter; only the granted ticket proceeds.
  void endExclusiveOp(uint64_t OwnerGen, ExclusiveOwner::State Have) noexcept;

  // Run a grower's stop-the-world under an ALREADY-ACQUIRED OwnedGrowing token
  // (the caller has won beginExclusiveOp). Drives the SAME setup handshake
  // collect()'s STW #1 uses -- beginHandshake() bumps the generation and raises
  // the stop flag; waitForAcks() blocks until every OTHER Running mutator has
  // parked at a safe point (the grower runs on its own Running thread and is
  // self-excluded via E.Tid != Self) -- then runs Resize with the table-root
  // lock held (Allocator::HeapMutex, innermost), then endHandshake() lowers the
  // flag and releases the parked mutators. This is what closes the interpreter
  // concurrent-grow-vs-reader race: while Resize swaps the Refs buffer every
  // reader is parked, so none is inside getRefAddr/setRefAddr touching the Refs
  // vector object. NO root scan and NO setSelfBlocked -- the grower grays its
  // broadcast ref through the write barrier and never parks itself. Lock order
  // ExclusiveMtx (released by the caller's beginExclusiveOp before this runs) ->
  // HandshakeMtx -> HeapMutex. Reuses the exact primitives collect() uses; it
  // does NOT reimplement the handshake.
  template <typename ResizeFn>
  void growStopTheWorld(Allocator &A, ResizeFn &&Resize) noexcept {
    const uint64_t Gen = beginHandshake();
    waitForAcks(Gen);
    {
      // HeapMutex is innermost: a concurrent root scan reads every table's Refs
      // vector under it, so holding it across the swap stops the scan iterating a
      // freed buffer (a collection cannot actually run here -- the grower holds
      // the exclusive token -- but keep the lock discipline the plain path uses).
      auto RootLock = A.lockTableRoots();
      Resize();
    }
    endHandshake(Gen);
  }

  // Test-only: number of exclusive-op losers currently queued (each parked on
  // TokenCV) awaiting the reserved FIFO handoff. Read under ExclusiveMtx. Lets a
  // test prove a grower parked behind an in-flight collection without a timing
  // window (GCThread.GrowBlocksDuringCollectAndViceVersa).
  uint32_t debugExclusiveWaiters() noexcept {
    std::lock_guard<std::mutex> L(ExclusiveMtx);
    return static_cast<uint32_t>(ExclGrowQueue.size());
  }

  // --- Teardown drain / launch lease -----------------------------------------

  // RAII launch lease. Held for the entire duration of a detached async
  // invocation (VM::asyncExecute / Executor::asyncInvoke): acquired
  // synchronously before the worker thread is detached and released only after
  // the invocation returns. ~Controller (via beginClosing) waits for every
  // outstanding lease to be released before it destroys the protocol state, so
  // a detached async mutator can never touch a freed Executor/Controller.
  class Lease {
  public:
    Lease() noexcept = default;
    explicit Lease(Controller *C) noexcept : Ctrl(C) {}
    Lease(Lease &&O) noexcept : Ctrl(O.Ctrl) { O.Ctrl = nullptr; }
    Lease &operator=(Lease &&O) noexcept {
      if (this != &O) {
        release();
        Ctrl = O.Ctrl;
        O.Ctrl = nullptr;
      }
      return *this;
    }
    Lease(const Lease &) = delete;
    Lease &operator=(const Lease &) = delete;
    ~Lease() noexcept { release(); }

    // True for a lease that was admitted (holds a live count). An empty lease
    // -- default-constructed, moved-from, or returned by acquireLease() when
    // the controller is already closing -- is not admitted; the async launch
    // must not detach a worker against a closing target.
    bool valid() const noexcept { return Ctrl != nullptr; }

  private:
    void release() noexcept;
    Controller *Ctrl = nullptr;
  };
  Lease acquireLease() noexcept;

  // RAII token representing an undeleted async HANDLE that co-owns a result
  // lease. Held ONLY by the Async handle (not the worker) -- separate from the
  // launch Lease. Its count is NOT part of the teardown drain; instead the
  // fallible C-API delete entries query outstandingHandleLeases() and REJECT a
  // VM/Executor delete while any async handle is still undeleted (the
  // deletion-ordered contract: the handle must be deleted before its producer).
  // This converts the same-thread handle-before-producer deadlock into a
  // detected, logged, fail-fast rejection.
  class HandleLease {
  public:
    HandleLease() noexcept = default;
    explicit HandleLease(Controller *C) noexcept : Ctrl(C) {}
    HandleLease(HandleLease &&O) noexcept : Ctrl(O.Ctrl) { O.Ctrl = nullptr; }
    HandleLease &operator=(HandleLease &&O) noexcept {
      if (this != &O) {
        release();
        Ctrl = O.Ctrl;
        O.Ctrl = nullptr;
      }
      return *this;
    }
    HandleLease(const HandleLease &) = delete;
    HandleLease &operator=(const HandleLease &) = delete;
    ~HandleLease() noexcept { release(); }
    bool valid() const noexcept { return Ctrl != nullptr; }

  private:
    void release() noexcept;
    Controller *Ctrl = nullptr;
  };
  HandleLease acquireHandleLease() noexcept;

  // Number of undeleted async handles that co-own a result lease. Read by the
  // fallible C-API delete entries to reject a handle-before-producer deletion.
  uint32_t outstandingHandleLeases() const noexcept {
    return HandleLeases.load(std::memory_order_acquire);
  }

  // Begin controller shutdown: reject new parks/collections (Closing), wake
  // every blocked/parked mutator, then drain -- block until no launch lease is
  // outstanding and no mutator is still inside gcSafepoint()'s park. Called
  // from ~Controller (and safe to call more than once). Idempotent Closing
  // store.
  void beginClosing() noexcept;

  // Non-owning wake handle for a mutator blocked on a runtime condition
  // variable (atomic.wait). wakeAllBlocked() sets *Flag and notifies *CV so the
  // waiter leaves the CV during shutdown, letting its invocation unwind (and
  // release the launch lease) instead of blocking teardown forever.
  struct BlockedWaitToken {
    std::condition_variable *CV = nullptr;
    std::atomic<bool> *Flag = nullptr;
    // The mutex guarding the waiter's condition variable. wakeAllBlocked()
    // takes it before setting Flag/notifying, mirroring the waiter's own
    // pre-wait check under the same mutex, so a wake can never be lost in the
    // window between that check and the wait().
    std::mutex *Mtx = nullptr;
  };
  void registerBlockedWait(BlockedWaitToken *T) noexcept;
  void deregisterBlockedWait(BlockedWaitToken *T) noexcept;
  void wakeAllBlocked() noexcept;

  // Fast, lock-free query of the closing state for the interpreter's blocked
  // waits (atomic.wait): a closing controller must abandon the wait so the
  // invocation can unwind and release its launch lease.
  bool isClosing() const noexcept {
    return Closing.load(std::memory_order_acquire);
  }

  // True iff the calling thread currently holds at least one registered value
  // stack on this controller -- i.e. it is executing inside an invocation of
  // this executor (typically a host callback). Used by the fallible teardown
  // entries (the C API delete functions) to REJECT a self-teardown: deleting
  // the VM/executor whose own callback frame the thread is standing on would
  // free state under that running stack. beginClosing() can only diagnose
  // this (a destructor has no rejection channel); the API layer checks first
  // and refuses.
  bool currentThreadRegistered() noexcept {
    std::lock_guard<std::mutex> L(RegistryMtx);
    return findEntry(std::this_thread::get_id()) != nullptr;
  }

private:
  friend class Registration;
  // The Allocator's collector worker drives STW #2 termination through
  // requestTerminationStop()/endTerminationStop() (private protocol below).
  friend class Allocator;

  // One entry per OS thread that currently has at least one registered value
  // stack. Grouping per thread (rather than a flat stack list) is what lets the
  // admission gate track a *thread's* acknowledgement of a generation.
  struct Entry {
    std::thread::id Tid;
    // Incarnation: the OS may recycle a thread id after a thread exits, so Tid
    // alone does not identify a registration lifetime. A retired entry is
    // erased and any future entry -- even on the same recycled id -- gets a
    // fresh IncId, so a snapshot taken by the collector can tell them apart.
    uint64_t IncId = 0;
    // Number of live Registrations on this thread (nested StackManagers).
    uint32_t RefCount = 0;
    std::vector<std::vector<ValVariant> *> Stacks;
    // Last generation this thread acknowledged at a safe point. Guarded by
    // RegistryMtx (not atomic: Entries is a vector, and every reader/writer
    // already holds the registry lock).
    uint64_t AckedGen = 0;
    // Execution state. Guarded by RegistryMtx. A thread starts Running; the
    // NativeScope flips it to NativeRunning around a host call. Only Running
    // entries are waited on by the handshake (waitForAcks); NativeRunning and
    // Blocked entries have stable stacks the coordinator scans in place
    // (scanNonRunningRoots), so they count as pre-acked.
    MutatorState State = MutatorState::Running;
    // Stable-address anchor for this thread's AOT shadow-frame chain (phase-1
    // prototype). Heap-allocated so its address survives Entries relocation --
    // the Entry itself moves, this pointee does not. Created on first
    // registration of the thread; destroyed with the Entry.
    std::unique_ptr<ShadowHead> Shadow;
    // This thread's auxiliary root vector (the executor's pending-exception
    // payload), cached from AuxProvider at registration. Non-owning: the
    // pointee is a thread_local of the executor, which outlives every
    // registration on that thread. Null when no provider is set.
    const std::vector<ValVariant> *AuxRoots = nullptr;
  };

  // Caller must hold RegistryMtx. The returned pointer is only valid while that
  // lock is held (Entries may reallocate).
  Entry *findEntry(std::thread::id Tid) noexcept;

  // Scan the calling thread's roots: value stacks precisely, and -- only when
  // ScanNative -- the native stack conservatively. ScanNative is the AOT knob:
  // an interpreter thread keeps every ref on a tracked value stack, so a native
  // scan there only over-retains stale ref bytes left in the deep C++ call
  // chain (breaking exact-usage tests); AOT code may hold a ref solely in a
  // register/stack slot and needs it.
  void selfScanInto(bool ScanNative) noexcept;
  uint64_t beginHandshake() noexcept;       // bump generation, raise stop
  bool allAcked(uint64_t Gen) noexcept;     // every entry acked >= Gen
  void endHandshake(uint64_t Gen) noexcept; // lower stop, wake parked mutators

  // Spin until every OTHER Running mutator has acked >= Gen. NativeRunning and
  // Blocked entries (and the coordinator itself) are skipped: they never reach
  // a safe point, so waiting on them would hang forever -- their roots are
  // scanned by scanNonRunningRoots instead.
  void waitForAcks(uint64_t Gen) noexcept;

  // Called by the coordinator after waitForAcks, while all Running mutators are
  // parked: gray the (stable) value stacks of every NativeRunning/Blocked entry
  // in place. This is what makes skipping their ack sound -- their roots are
  // still scanned, just by the coordinator rather than by themselves.
  void scanNonRunningRoots() noexcept;

  // Gray every live slot of every ShadowFrame on an entry's stable shadow
  // chain (AOT root spills). Caller holds RegistryMtx. Acquire-load of the head
  // pairs with ShadowScope's release publish so a partially-built frame is
  // never observed.
  void scanShadowChain(Entry &E) noexcept;

  // Gray every slot of an entry's auxiliary root vector (the executor's
  // pending-exception payload). Caller holds RegistryMtx, which is also what
  // the mutator takes (lockStackRoots) to mutate the vector -- so this never
  // races a reallocation. No-op for an entry with no provider-supplied vector.
  void scanAuxRoots(Entry &E) noexcept;

  // --- STW #2 termination (driven by the Allocator's elected collector worker)
  // The worker is NOT a registered mutator, so it never self-acks and
  // waitForAcks naturally excludes it. requestTerminationStop() bumps the
  // generation, raises the stop flag, and waits for every OTHER Running mutator
  // to park at a safe point -- so no write barrier is in flight when the owner
  // closes the barrier (publishes GCState::Sweeping) and re-checks quiescence.
  // endTerminationStop() lowers the flag and releases the parked mutators.
  // No root scan is performed (unlike STW #1): the roots were already marked in
  // STW #1, and this handshake exists only to quiesce the barrier before sweep.
  void requestTerminationStop() noexcept;
  void endTerminationStop() noexcept;
  // Generation of the in-flight STW #2, written by requestTerminationStop() and
  // read by endTerminationStop(). Only the single elected owner touches it
  // (TerminationOwner CAS serializes ownership), and both calls run on that one
  // worker thread, sequenced -- so a plain member is race-free.
  uint64_t TerminationGen = 0;

  // Mark / restore the calling coordinator thread's own registry State around
  // waitForCycleComplete(). The coordinator parks on a CV there while still a
  // registered Running mutator; STW #2's waitForAcks would wait on it forever
  // (it never reaches a safe point) -- a hang. Marking it Blocked excludes it
  // from waitForAcks; this is sound because STW #2 performs NO root scan (its
  // roots were marked in STW #1) and a coordinator blocked on a CV executes no
  // guest code, so it cannot be mid-barrier. setSelfBlocked captures the
  // coordinator's PRIOR state into Prev so collect() can restore it (mirrors
  // NativeScope::Prev) instead of hardcoding Running, and returns false if the
  // coordinator has no registry entry (bare-thread tests), leaving Prev
  // untouched -- in which case there is nothing to restore.
  bool setSelfBlocked(MutatorState &Prev) noexcept;
  // Admit the calling thread to a target State through the admission gate:
  // park/ack while a handshake is in flight, then set this thread's entry to
  // Target with StopFlag observed clear. Used by the coordinator (restore after
  // a cycle) and by registerStack (admit a joining or reentrant mutator).
  // admitToRunning() is the Target == Running wrapper.
  void admitToState(MutatorState Target) noexcept;
  void admitToRunning() noexcept { admitToState(MutatorState::Running); }

  // Set at the start of ~Controller: once closing, Registration::reset() must
  // not touch Entries/RegistryMtx, which are about to be destroyed. A detached
  // async mutator thread is never joined by ~Allocator, so its StackManager's
  // Registration can outlive them. Mirrors the Stop guard the Allocator's
  // remove*() siblings still carry. Declared FIRST so it is destroyed LAST.
  // The full teardown drain (OutstandingLeases / ParkedMutators / DrainCV,
  // below) closes this window completely.
  std::atomic<bool> Closing{false};

  // Lock order: RegistryMtx -> Allocator::GrayMutex -> Allocator::WhiteMutex
  // (the latter two via markGray). HandshakeMtx is a leaf and is NEVER held
  // together with any of them -- gcSafepoint drops it before self-scanning.
  std::mutex RegistryMtx;
  std::vector<Entry> Entries; // guarded by RegistryMtx
  uint64_t NextIncId = 1;     // guarded by RegistryMtx
  // Aux-root provider (see setAuxRootProvider). Written once by the owning
  // Executor's constructor, before any mutator thread can exist; read in
  // registerStack under RegistryMtx.
  AuxRootProvider AuxProvider = nullptr;

  std::atomic<bool> StopFlag{false};
  std::mutex HandshakeMtx;
  std::condition_variable ReleaseCV; // parked mutators wait here
  uint64_t CurrentGeneration = 0;    // guarded by HandshakeMtx

  // --- Exclusive-operation owner state (see beginExclusiveOp/endExclusiveOp).
  // ExclusiveMtx is OUTERMOST in the lock order: a loser takes RegistryMtx
  // (setSelfBlocked / admitToState) while still holding it, and admitToState in
  // turn may take HandshakeMtx via gcSafepoint, giving ExclusiveMtx ->
  // RegistryMtx -> HandshakeMtx. All fields below are guarded by ExclusiveMtx.
  std::mutex ExclusiveMtx;
  std::condition_variable TokenCV; // losing waiters park here
  ExclusiveOwner::State ExclState = ExclusiveOwner::State::Free;
  // Generation of the current owner; also the monotonic source bumped on every
  // acquire/handoff, so a stale endExclusiveOp cannot match a live owner.
  uint64_t ExclOwnerGen = 0;
  uint64_t ExclNextTicket = 1;    // next FIFO ticket to hand out
  uint64_t ExclGrantedTicket = 0; // ticket the reserved handoff granted (0=none)
  // GROWERS ONLY. Tickets carry no Want, so endExclusiveOp labels every handoff
  // OwnedGrowing; a queued COLLECTOR would be handed an OwnedGrowing token and
  // trip endExclusiveOp's assuming(ExclState==Have) on release. Collectors use
  // the non-blocking tryBeginExclusiveOp instead (a missed collection is
  // skippable), and beginExclusiveOp asserts Want==OwnedGrowing at the enqueue
  // site, so the invariant is enforced rather than merely documented. Wiring a
  // blocking collector here would require a per-ticket Want.
  std::deque<uint64_t> ExclGrowQueue; // FIFO of parked waiter tickets

  // --- Teardown drain state
  // --------------------------------------------------- Count of outstanding
  // launch leases (detached async invocations). The drain in beginClosing()
  // blocks until this reaches zero.
  std::atomic<uint32_t> OutstandingLeases{0};
  // Count of undeleted async handles co-owning a result lease. NOT part of the
  // teardown drain (that waits on OutstandingLeases); a detection counter for
  // the fallible C-API delete entries. See HandleLease.
  std::atomic<uint32_t> HandleLeases{0};
  // Count of mutators currently inside gcSafepoint()'s park (incremented before
  // ReleaseCV.wait(), decremented after it returns). The drain must reach zero
  // before ~Controller destroys HandshakeMtx/ReleaseCV: destroying a
  // condition_variable with a waiter still blocked on it is UB, and a detached
  // async mutator is exactly the thread that can still be parked at teardown.
  // This closes the residual window the `Closing` guard alone could not.
  std::atomic<uint32_t> ParkedMutators{0};
  // Number of live stack registrations across every thread (sum of all entries'
  // RefCounts), maintained as an atomic separate from the Entries vector so
  // reset() can account for a deregistration during teardown without touching
  // the vector (which may be under destruction). The drain blocks until this
  // reaches the count held by the teardown thread itself (0 for the normal
  // owner-thread ~VM), so destruction never frees state a synchronous
  // execute()/invoke() on another thread is still using -- those hold a
  // registered stack but no launch lease.
  std::atomic<uint32_t> RegisteredStacks{0};
  std::mutex DrainMtx;
  std::condition_variable DrainCV;

  // Test-only drain-release pause hook; see setDrainReleaseHook. Empty in
  // production. Read/called only under DrainMtx.
  std::function<void()> DrainReleaseHook;

  // Fire DrainReleaseHook if set. Called under DrainMtx by the drain-satisfying
  // release paths after notify_all.
  void drainReleaseFired() noexcept {
    if (DrainReleaseHook) {
      DrainReleaseHook();
    }
  }

  // Blocked-wait registry: every mutator blocked in a runtime condition
  // variable (atomic.wait) registers a wake token here so shutdown can notify
  // each one. Guarded by its own leaf mutex (never held with any other).
  std::mutex BlockedMtx;
  std::vector<BlockedWaitToken *> BlockedWaiters; // guarded by BlockedMtx

  Allocator Alloc;
};

} // namespace GC
} // namespace WasmEdge
