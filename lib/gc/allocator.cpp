// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "gc/allocator.h"
#include "common/spdlog.h"
#include "gc/coherent_slot.h"
#include "gc/controller.h"
#include "runtime/instance/gc.h"
#include "runtime/instance/module.h"

#define FMT_CPP_LIB_FILESYSTEM 0
#include <fmt/std.h>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
// getStack() uses pthread stack APIs; include explicitly, not transitively.
#include <pthread.h>
#endif

// std::fill_n / std::find over the index table and the root registries.
#include <algorithm>
// setjmp spills callee-saved registers before conservative scanning
// (markNativeStackRoots).
#include <csetjmp>
// std::align_val_t / std::nothrow for over-aligned GC block allocation.
#include <new>

namespace {

// TSan annotations for conservative scanning's intentional races: the GC reads
// stack/heap/global data concurrently with execution. Safe -- conservatism
// over-retains a stale pointer, never UAF.
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define TSAN_ENABLED 1
#endif
#endif
#if defined(__SANITIZE_THREAD__)
#define TSAN_ENABLED 1
#endif

#ifdef TSAN_ENABLED
extern "C" {
void AnnotateIgnoreReadsBegin(const char *file, int line);
void AnnotateIgnoreReadsEnd(const char *file, int line);
}
#define TSAN_IGNORE_READS_BEGIN() AnnotateIgnoreReadsBegin(__FILE__, __LINE__)
#define TSAN_IGNORE_READS_END() AnnotateIgnoreReadsEnd(__FILE__, __LINE__)
#else
#define TSAN_IGNORE_READS_BEGIN()
#define TSAN_IGNORE_READS_END()
#endif

// hardware_concurrency() may return 0; clamp to 1, else no worker drives
// marking/sweeping and manualCollect() hangs forever.
uint32_t collectorThreadCount() noexcept {
  const uint32_t N = std::thread::hardware_concurrency();
  return N == 0 ? UINT32_C(1) : N;
}

} // namespace

using namespace std::literals;

namespace WasmEdge {
namespace GC {

Allocator::Allocator() { startWorkers(); }

Allocator::Allocator(Controller &C) : Ctrl(&C) { startWorkers(); }

void Allocator::startWorkers() {
  const uint32_t Workers = collectorThreadCount();
  Collectors.reserve(Workers);
  // Worker body, defined once so the spawn loop can wrap it in try/catch.
  auto WorkerBody = [this] {
    while (Stop.load(std::memory_order_acquire) == false) {
      Header *H = nullptr;
      bool Terminate = false;
      {
        std::unique_lock<std::mutex> Locker(GrayMutex);
        // Park until there is marking work, a termination opportunity, or Stop.
        GrayNotEmptyCV.wait(Locker, [this] {
          if (Stop.load(std::memory_order_acquire)) {
            return true;
          }
          if (Ctrl != nullptr && Ctrl->isClosing()) {
            // Teardown in progress: abandon marking/termination and park until
            // ~Allocator sets Stop. Returning false here (rather than waking to
            // spin on a termination we would immediately decline) keeps the
            // workers quiescent through the drain -- no sweep runs during
            // Closing, and ~Allocator reclaims the whole heap wholesale.
            return false;
          }
          if (CurrentGCState.load(std::memory_order_acquire) !=
              GCState::MarkingGray) {
            // No active marking phase (Idle/Sweeping): park until a cycle
            // publishes MarkingGray (beginMarking notifies) or Stop. Born-gray
            // objects allocated while Idle stay queued until then.
            return false;
          }
          if (GrayHead != nullptr) {
            return true; // a gray object to trace
          }
          // Empty gray during marking is a termination candidate only when no
          // tracer is mid-trace (ActiveTracers == 0) and no owner is already
          // driving termination (TerminationOwner == 0). Otherwise keep
          // parking: the in-flight tracer may still shade new gray objects.
          return ActiveTracers.load(std::memory_order_acquire) == 0 &&
                 TerminationOwner.load(std::memory_order_acquire) == 0;
        });
        if (Stop.load(std::memory_order_acquire)) {
          break;
        }
        if (CurrentGCState.load(std::memory_order_acquire) !=
            GCState::MarkingGray) {
          // Woken for a non-marking state (e.g. the owner published Idle after
          // sweeping); loop to re-park.
          continue;
        }
        if (GrayHead != nullptr) {
          H = GrayHead;
          GrayHead = H->GrayNext;
          H->GrayNext = nullptr;
          // Increment BEFORE releasing GrayMutex: a termination owner that
          // holds GrayMutex and observes Gray.empty() then also observes
          // ActiveTracers > 0, so this popped-but-untraced object (and its
          // still-White children) cannot be missed by the quiescence check.
          ActiveTracers.fetch_add(1, std::memory_order_acq_rel);
        } else {
          Terminate = true;
        }
      }

      if (Terminate) {
        // Test hook: stall here -- GrayMutex released, no lock held, ownership
        // not yet contended -- to stage the stale-win preemption window.
        if (PreTerminationCASHook) {
          PreTerminationCASHook();
        }
        // Elect a single termination owner; loser workers re-park.
        uint32_t Expected = 0;
        if (!TerminationOwner.compare_exchange_strong(
                Expected, 1, std::memory_order_acq_rel)) {
          continue;
        }
        // Stale-win guard: we set Terminate under GrayMutex while MarkingGray,
        // but may have been preempted before winning ownership above. In that
        // gap the in-cycle owner can have swept and left MarkingGray (to
        // Sweeping/Idle), and a fresh cycle may even be underway. Re-check the
        // phase now that we hold ownership. Only the termination owner leaves
        // MarkingGray, and we are it, so this reading is stable until we act on
        // it. If we are no longer in MarkingGray this ownership is stale:
        // firing requestTerminationStop() would ++CurrentGeneration and raise
        // StopFlag for a cycle that is over, opening a second handshake
        // generation that overlaps the next coordinator's STW #1 and trips
        // endHandshake's generation assert (GCThread.GrowStackDuringCollect).
        // Release ownership and re-park without driving STW #2; the live cycle
        // drives its own termination.
        bool StillMarking;
        {
          std::lock_guard<std::mutex> RL(GrayMutex);
          StillMarking = CurrentGCState.load(std::memory_order_acquire) ==
                         GCState::MarkingGray;
          if (!StillMarking) {
            // Release under GrayMutex so a worker that observed our (now stale)
            // ownership in its wait predicate cannot miss the notify below.
            TerminationOwner.store(0, std::memory_order_release);
          }
        }
        if (!StillMarking) {
          StaleTerminationRejects.fetch_add(1, std::memory_order_relaxed);
          GrayNotEmptyCV.notify_all();
          continue;
        }
        // Teardown safety: NO sweep may run during Closing. A mutator that
        // bailed its safe point (gcSafepoint's `if (Closing) return`) keeps
        // running its guest and may still read/write heap objects; its roots
        // were never scanned for this cycle, so the White set can contain live
        // objects. runSweepAndSwap() frees White, so sweeping now would be a
        // use-after-free -- strictly worse than the hang this fix removes.
        // Release ownership without sweeping and re-park (the predicate above
        // then keeps us parked until Stop); ~Allocator reclaims the whole heap
        // wholesale after the drain confirms every guest has finished.
        if (Ctrl != nullptr && Ctrl->isClosing()) {
          TerminationOwner.store(0, std::memory_order_release);
          continue;
        }
        // STW #2: stop every mutator at a safe point so no write barrier is in
        // flight, then re-check quiescence with mutators parked. STW #2 stops
        // MUTATORS, not the other collector workers, so a worker may still be
        // tracing when we re-check. Standalone allocator (Ctrl == nullptr) has
        // no mutator registry: the ActiveTracers check alone gives quiescence.
        if (Ctrl != nullptr) {
          Ctrl->requestTerminationStop();
        }
        bool Done = false;
        {
          std::unique_lock<std::mutex> L2(GrayMutex);
          // The `!isClosing()` term re-checks teardown atomically with the
          // single-sweep CAS, closing the race where Closing was published
          // after the early bail above. Closing is monotonic, so a false
          // reading here proves Closing stayed false throughout this cycle's
          // STW #1 root scan: every Running mutator acked and was scanned,
          // hence White is genuine garbage and the sweep is safe. A true
          // reading skips the CAS (Done stays false) and falls through to the
          // not-quiescent path, which releases ownership and re-parks without
          // sweeping.
          if (GrayHead == nullptr &&
              ActiveTracers.load(std::memory_order_acquire) == 0 &&
              !(Ctrl != nullptr && Ctrl->isClosing())) {
            // Gate the sweep on the phase transition, not just marking
            // quiescence. Quiescence (empty Gray, no tracer) proves *marking*
            // finished; it does NOT prove a sweep for THIS phase hasn't already
            // run. Two workers can both observe Gray.empty() && MarkingGray and
            // each win a fresh TerminationOwner CAS in turn (owner A resets it
            // to 0 after sweeping). Without a phase gate, owner B re-checks
            // quiescence while A has already swept and published Idle -- empty
            // Gray in Idle is trivially quiescent -- and sweeps a SECOND time,
            // freeing the live set A's Black/White role-swap just relabelled
            // White (heap-use-after-free of the marked-live objects).
            //
            // The CAS enforces the single-sweep invariant: admit exactly
            // one MarkingGray -> Sweeping transition per marking phase. A
            // failed CAS means another owner already drove this phase's
            // MarkingGray -> Sweeping -> Idle transition (state is now Idle or
            // Sweeping), so this worker is NOT Done and must NOT sweep.
            //
            // ExpectedState is a fresh local per attempt:
            // compare_exchange_strong overwrites it with the observed value on
            // failure, so it must not be reused as MarkingGray across attempts.
            //
            // Close the barrier BEFORE sweep, still stopped: the CAS publishes
            // Sweeping so writeBarrier no-ops and no worker dequeues. Publish
            // Sweeping (the CAS, under GrayMutex) BEFORE firing SweepStart so a
            // SweepStart observer that calls writeBarrier sees Sweeping and
            // no-ops instead of re-locking the non-recursive GrayMutex we
            // hold here (self-deadlock).
            GCState ExpectedState = GCState::MarkingGray;
            if (CurrentGCState.compare_exchange_strong(
                    ExpectedState, GCState::Sweeping,
                    std::memory_order_acq_rel)) {
              Done = true;
              notifyPhase(GCPhase::SweepStart);
            }
          }
        }
        if (Ctrl != nullptr) {
          Ctrl->endTerminationStop();
        }
        if (Done) {
          // Capture the collection's exclusive-owner generation BEFORE Idle is
          // published (E1.1): once Idle is visible a fresh
          // Controller::collect() can win beginCycle and overwrite
          // CollectionOwnerGen, so reading it later could hand endExclusiveOp
          // the next cycle's generation. (In practice the still-held token
          // blocks that fresh collect()'s tryBeginExclusiveOp, but read early
          // regardless -- the local is unconditionally correct.)
          const uint64_t OwnerGen =
              CollectionOwnerGen.load(std::memory_order_acquire);
          runSweepAndSwap(); // frees white, flips parity, SweepEnd -- not Idle
          TerminationOwner.store(0, std::memory_order_release);
          // Release the exclusive-operation token this collection held for the
          // WHOLE cycle (E1.1). The sweep-completing worker -- not the
          // coordinator -- releases it, handing the token across the STW #2
          // boundary to any queued grower (endExclusiveOp's reserved FIFO
          // handoff). Only a Controller-backed cycle takes the token; a
          // standalone Allocator (Ctrl == nullptr, legacyManualCollect) never
          // did, so guard on Ctrl. This is the sole worker path that releases:
          // the stale-termination, teardown-Closing, and not-quiescent branches
          // all reset TerminationOwner WITHOUT sweeping and must NOT release
          // (either another worker completes this cycle's sweep and releases,
          // or teardown intentionally leaves the token owned).
          //
          // Released BEFORE Idle is published, not after. Controller::collect()
          // returns as soon as waitForCycleComplete() observes Idle -- and its
          // wait predicate is checked BEFORE it ever blocks, so a cycle that
          // finishes while the coordinator is still on its way to
          // waitForCycleComplete() never waits at all and no notify is
          // involved. Publishing Idle first therefore lets the coordinator
          // return with this cycle's token still owned, and a back-to-back
          // collect() -- exactly what the VM-level release tests do -- loses
          // its tryBeginExclusiveOp to a collection that has already finished
          // and returns false. Making Idle the LAST thing published means
          // "collect() returned" implies both the token is released and the
          // heap is Idle, so the next acquire cannot be spuriously refused.
          if (Ctrl != nullptr) {
            Ctrl->endExclusiveOp(
                OwnerGen, Controller::ExclusiveOwner::State::OwnedCollecting);
          }
          CurrentGCState.store(GCState::Idle, std::memory_order_release);
          // Wake the coordinator parked in waitForCycleComplete().
          {
            std::unique_lock<std::mutex> GLocker(GCMutex);
          }
          GCCV.notify_all();
        } else {
          // Not quiescent: a mutator barrier shaded a new gray (or a tracer is
          // still running). Release ownership and wake workers to resume
          // tracing -- do NOT sweep.
          TerminationOwner.store(0, std::memory_order_release);
          GrayNotEmptyCV.notify_all();
        }
        continue;
      }

      // Test hook: pause mid-trace (after pop / ActiveTracers increment, before
      // shading children) so a test can prove termination waits for us.
      if (TracerPauseHook) {
        TracerPauseHook();
      }
      assuming(H->Color.load(std::memory_order_relaxed) == ColorGray);
      // Mark H's children gray
      const auto *Raw =
          reinterpret_cast<const Runtime::Instance::GCInstance::RawData *>(
              reinterpret_cast<uint8_t *>(H) + sizeof(Header));
      Span<const ValVariant> Pointers(Raw->data(), Raw->Length);
      for (size_t I = 0; I < Pointers.size(); ++I) {
        // Read only the pointer word, via a single relaxed atomic load, so this
        // concurrent scan does not data-race a mutator's coherent struct.set/
        // array.set store to the same 128-bit slot. The marker needs no
        // coherent (type,pointer) pair -- markGray only consumes the pointer.
        markGray(GC::loadPointerWordRelaxed(Pointers[I]));
      }
      // Blacken: publish Marked(BlackParity). This single store replaces the
      // old "clear IsGray, insert into the Black set" pair -- there is no set
      // to insert into, and the sweep's parity flip is what will later
      // reinterpret this same colour as White. Reading BlackParity here cannot
      // race the flip: only the sweep flips it, and the sweep runs only after
      // termination observed ActiveTracers == 0, which this tracer is holding
      // above 0 until the fetch_sub below.
      H->Color.store(BlackParity.load(std::memory_order_acquire),
                     std::memory_order_release);
      // Drop the in-flight count AFTER the blacken so a termination owner
      // never observes ActiveTracers == 0 with H neither gray nor black. If
      // we were the last tracer, wake a worker to attempt termination (Gray may
      // now be empty and quiescent).
      if (ActiveTracers.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        GrayNotEmptyCV.notify_all();
      }
    }
  };
  // Spawn workers. If a std::thread ctor throws (OS refuses a thread), the
  // already-created joinable threads would std::terminate at destruction; stop
  // and join them, then rethrow.
// MSVC signals exceptions via _CPPUNWIND, not __EXCEPTIONS; guarding on the
// latter alone would drop this cleanup on MSVC.
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    for (uint32_t I = 0; I < Workers; ++I) {
      Collectors.emplace_back(WorkerBody);
    }
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch (...) {
    {
      std::unique_lock<std::mutex> Locker(GrayMutex);
      Stop.store(true, std::memory_order_release);
    }
    GrayNotEmptyCV.notify_all();
    GCCV.notify_all();
    for (auto &Collector : Collectors) {
      if (Collector.joinable()) {
        Collector.join();
      }
    }
    throw;
  }
#endif
}

void Allocator::runSweepAndSwap() noexcept {
  // Test hook: hold the sweep open (state == Sweeping, no locks held) so a
  // test can allocate deterministically during the sweep phase.
  if (SweepPauseHook) {
    SweepPauseHook();
  }
  // Reclaim every White object. The termination owner has already published
  // GCState::Sweeping under STW #2 (barrier quiet, workers not dequeuing) and
  // all mutators were parked while the barrier closed, so nothing calls
  // markGray concurrently here and the free needs no GrayMutex.
  const std::chrono::steady_clock::time_point Start =
      std::chrono::steady_clock::now();
  const uint8_t Parity = BlackParity.load(std::memory_order_relaxed);
  const uint8_t WhiteColor = static_cast<uint8_t>(1U - Parity);

  // Detach the whole all-object list and partition it OUTSIDE AllMutex.
  // Allocations concurrent with the sweep (the phase is Sweeping, mutators are
  // released -- see setSweepPauseHook) push onto the now-empty AllHead and are
  // untouched by this walk; the survivors are spliced back in front of them at
  // the end. This is what keeps the sweep from blocking allocation for its
  // whole duration, which a single lock held across the walk would do.
  Header *List = nullptr;
  {
    std::unique_lock<std::mutex> Locker(AllMutex);
    List = AllHead;
    AllHead = nullptr;
  }
  Header *Survivors = nullptr;
  Header *SurvivorTail = nullptr;
  Header *Dead = nullptr;
  size_t DeadCount = 0;
  for (Header *H = List; H != nullptr;) {
    Header *Next = H->AllNext;
    if (H->Color.load(std::memory_order_relaxed) == WhiteColor) {
      H->AllNext = Dead;
      Dead = H;
      ++DeadCount;
    } else {
      // Gray (allocated during Idle/Sweeping, or still queued) and Black
      // (traced this cycle) both survive.
      if (SurvivorTail == nullptr) {
        SurvivorTail = H;
      }
      H->AllNext = Survivors;
      Survivors = H;
    }
    H = Next;
  }
  {
    // Un-index the dead and re-attach the survivors in ONE critical section.
    // Erasing before the blocks are freed (below, with AllMutex released) is
    // what makes address reuse safe: a validating markGray either sees the
    // entry and holds AllMutex -- so this erase, and therefore the free, has
    // not happened yet -- or does not see it at all.
    std::unique_lock<std::mutex> Locker(AllMutex);
    for (Header *H = Dead; H != nullptr; H = H->AllNext) {
      Index.erase(H);
    }
    if (SurvivorTail != nullptr) {
      SurvivorTail->AllNext = AllHead;
      AllHead = Survivors;
    }
  }
  spdlog::debug("worker sweep W:{}"sv, DeadCount);
  uint64_t Freed = 0;
  for (Header *H = Dead; H != nullptr;) {
    Header *Next = H->AllNext;
    Freed += H->Size;
    doDeallocate(reinterpret_cast<uint8_t *>(H), H->Size);
    H = Next;
  }
  if (Freed > 0) {
    const auto Duration = std::chrono::steady_clock::now() - Start;
    const auto Milli =
        std::chrono::duration_cast<std::chrono::milliseconds>(Duration);
    spdlog::debug("GC: Freed {} bytes in {} ms"sv, Freed, Milli.count());
  }

  // Swap the black and white roles as a single parity flip: every survivor is
  // Marked(Parity), which the new parity reinterprets as White (candidate
  // garbage for the next cycle), and Black becomes empty because every
  // Marked(1 - Parity) object was just freed. Exactly the old set-pointer swap,
  // without touching a single object.
  spdlog::debug("swap parity {} -> {}"sv, Parity, WhiteColor);
  BlackParity.store(WhiteColor, std::memory_order_release);
  notifyPhase(GCPhase::SweepEnd);
  // Deliberately does NOT publish GCState::Idle -- the caller does, only after
  // releasing the exclusive-operation token. See the Done branch of the worker
  // loop for why that ordering is load-bearing.
}

Allocator::~Allocator() noexcept {
  // Publish Stop under each CV's mutex so a worker that checked its predicate
  // but hasn't parked can't miss the wakeup and hang join().
  {
    std::unique_lock<std::mutex> Locker(GrayMutex);
    Stop.store(true, std::memory_order_release);
  }
  GrayNotEmptyCV.notify_all();
  {
    std::unique_lock<std::mutex> Locker(GCMutex);
  }
  GCCV.notify_all();
  {
    std::unique_lock<std::mutex> Locker(HeapMutex);
    for (auto T : Heaps) {
      T->clearAllocator(*this);
    }
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    for (auto G : Globals) {
      G->clearAllocator(*this);
    }
  }
  {
    std::unique_lock<std::mutex> Locker(ElemMutex);
    for (auto E : Elems) {
      E->clearAllocator(*this);
    }
  }
  {
    std::unique_lock<std::mutex> Locker(ExceptionMutex);
    for (auto Ex : Exceptions) {
      Ex->clearAllocator(*this);
    }
  }
  for (auto &Collector : Collectors) {
    Collector.join();
  }
  // Reclaim the whole heap wholesale. One walk of the all-object list covers
  // every colour -- gray, black and white alike -- where the old teardown had
  // to drain three separate containers and could only find an object that was
  // in one of them.
  for (Header *H = AllHead; H != nullptr;) {
    Header *Next = H->AllNext;
    doDeallocate(reinterpret_cast<uint8_t *>(H), H->Size);
    H = Next;
  }
  AllHead = nullptr;
}

void Allocator::markRoots(bool ScanNativeStack) noexcept {
  notifyPhase(GCPhase::MarkRootStart);
  // SINGLE-MUTATOR PATH. This is reached only from legacyManualCollect(), which
  // manualCollect() selects only when Ctrl == nullptr -- a standalone allocator
  // with no mutator registry and no coordinator (the bare-Allocator unit tests).
  // Its root set is therefore whatever this one thread can see: the native scan
  // below covers the calling thread alone (markNativeStackRoots uses
  // current-thread APIs), and marking is not stop-the-world. A second mutator
  // running AOT code with a ref only in a register/stack slot would be missed --
  // which is sound here precisely because a standalone allocator has no way to
  // acquire one.
  //
  // The same single-mutator assumption underlies the write barriers on this
  // path: each shades around a store while gated on a non-Idle state that only
  // the storing thread leaves Idle. One mutator cannot scan roots between a
  // shade and its store; a *second* mutator flipping the state mid-store could,
  // scanning past the not-yet-stored value while the shade was a no-op -- again
  // unreachable without a registry, not a separate barrier-ordering bug.
  //
  // The Controller-backed path (Controller::collect) does NOT come through here
  // and carries none of these limitations: root scanning is handshake-gated, so
  // it cannot complete while any mutator is still Running, and every mutator
  // covers its own roots at its ack -- gcSafepoint() calls selfScanInto(true),
  // whose conservative native scan captures refs held only in that thread's
  // registers or native frames. NativeRunning/Blocked entries do not ack and are
  // scanned remotely instead (scanNonRunningRoots), which is why they must keep
  // their refs on the value stack or in a published shadow frame.
  if (ScanNativeStack) {
    markNativeStackRoots();
  }
  // Value-stack roots now live on the Controller's registry.
  if (Ctrl != nullptr) {
    TSAN_IGNORE_READS_BEGIN();
    Ctrl->forEachStackRoot(
        [this](const ValVariant &Val) { markGray(getPointer(Val)); });
    TSAN_IGNORE_READS_END();
  }
  scanSharedRoots();
}

void Allocator::scanSharedRoots() noexcept {
  {
    std::unique_lock<std::mutex> Locker(HeapMutex);
    // Round-2 A2: read each table element (and InitValue) with a single relaxed
    // atomic pointer-word load, which pairs race-free with a concurrent coherent
    // setRefAddr / fillRefs / setRefs -- so no TSAN_IGNORE_READS is needed here.
    for (auto T : Heaps) {
      // InitValue holds a GC ref for growTable's slot fill but isn't in Refs;
      // scan it explicitly, else a one-arg grow broadcasts a dangling ref into
      // new slots (use-after-free).
      markGray(loadPointerWordRelaxed(T->InitValue));
      for (const auto &Ref : T->Refs) {
        markGray(loadPointerWordRelaxed(Ref));
      }
    }
    // Round-2 A3: free the table buffers a reallocating growTable retired since
    // the last scan. This runs under the stop-the-world root snapshot with every
    // mutator parked at a safepoint or NativeRunning, so no mutator holds an
    // in-flight pointer into a retired buffer (a table access is safepoint-free).
    // Their live refs already migrated into the current Refs buffers scanned
    // above, so dropping them loses no root.
    RetiredTableBuffers.clear();
  }
  {
    std::unique_lock<std::mutex> Locker(GlobalMutex);
    // Round-2 A2: read each ref global's pointer word with a single relaxed
    // atomic load, which pairs race-free with a concurrent coherent setValue --
    // so no TSAN_IGNORE_READS is needed here. Numeric globals hold no managed
    // reference and are skipped (their plain store must not race an atomic read).
    for (auto G : Globals) {
      if (G->getGlobalType().getValType().isRefType()) {
        markGray(loadPointerWordRelaxed(G->Value));
      }
    }
  }
  // Snapshot the four "category" root sets (Elems, Exceptions, HostRoots,
  // ScopedRoots) under their respective mutexes, releasing each mutex before
  // shading. Holding a category mutex across markGray() nested
  // categoryMutex -> GrayMutex -> set-mutex, and because these per-Allocator
  // mutexes are stack-local, sequential single-threaded tests reuse the same
  // stack addresses; TSan then stitches a lock-order-inversion cycle across
  // unrelated tests (a false positive, but it fails the gate). Snapshotting the
  // pointers first removes the nesting entirely without changing marking
  // semantics.
  std::vector<uint8_t *> Roots;
  {
    std::unique_lock<std::mutex> Locker(ElemMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto E : Elems) {
      for (const auto &Ref : E->getRefs()) {
        Roots.push_back(getPointer(Ref));
      }
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(ExceptionMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (auto Ex : Exceptions) {
      for (const auto &Val : Ex->getPayload()) {
        Roots.push_back(getPointer(Val));
      }
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(HostRootsMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (const auto &Ptr : HostRoots) {
      Roots.push_back(Ptr);
    }
    TSAN_IGNORE_READS_END();
  }
  {
    std::unique_lock<std::mutex> Locker(ScopedRootsMutex);
    TSAN_IGNORE_READS_BEGIN();
    for (const auto &Ptr : ScopedRoots) {
      Roots.push_back(Ptr);
    }
    TSAN_IGNORE_READS_END();
  }
  // Shade every snapshotted root with NO category mutex held, so markGray's
  // GrayMutex/set-mutex acquisition never nests under a category lock. The
  // snapshot is consistent per category; marking a briefly-stale set is safe
  // (a concurrently-added object is born gray; a concurrently-removed one is at
  // worst conservatively retained one cycle).
  for (auto *Ptr : Roots) {
    markGray(Ptr);
  }
}

bool Allocator::beginCycle(bool Manual) noexcept {
  // Teardown: never START a new cycle once Closing. A fresh collection begun
  // mid-teardown would only have to be abandoned (its handshake would race the
  // drain and its sweep is gated off anyway), so refuse it up front and let the
  // caller unwind.
  if (Ctrl != nullptr && Ctrl->isClosing()) {
    return false;
  }
  // Auto cycles honour the manual-GC toggle and the schedule; a manual cycle
  // (Manual == true, i.e. Controller::collect(true)) bypasses both and runs
  // whenever the heap is Idle.
  if (!Manual) {
    if (unlikely(EnableManualGC.load(std::memory_order_acquire) == true)) {
      return false;
    }
    if (std::chrono::steady_clock::now() <
        NextGC.load(std::memory_order_relaxed)) {
      return false;
    }
  }
  // Test-only: the collection already owns the exclusive token here, but the
  // heap is still Idle (the CAS below is what arms the write barrier). Fired
  // before the CAS so a test can drive a grower's pop-and-park in this quiet
  // window; empty in production. See setPreCycleHook.
  if (PreCycleHook) {
    PreCycleHook();
  }
  GCState State = GCState::Idle;
  if (!CurrentGCState.compare_exchange_strong(State, GCState::MarkingRoot,
                                              std::memory_order_acq_rel)) {
    return false;
  }
  NextGC.store(std::chrono::steady_clock::now() + std::chrono::seconds(1),
               std::memory_order_release);
  return true;
}

void Allocator::snapshotSharedRootsInto() noexcept {
  // Fire MarkRootStart here (not before the handshake) so the observer window
  // opens with every Running mutator already parked -- the snapshot is taken
  // with no concurrent writer. Value stacks are scanned by the handshake, not
  // here.
  notifyPhase(GCPhase::MarkRootStart);
  scanSharedRoots();
}

void Allocator::beginMarking() noexcept {
  // Fires MarkGrayStart only. Deliberately does NOT publish MarkingGray and
  // does NOT wake the workers -- both are deferred to wakeCollectors(), which
  // the coordinator calls only AFTER endHandshake ends STW #1.
  //
  // Why MarkingGray must not be published here (deterministic STW #1/STW #2
  // separation): a worker that observes MarkingGray with Gray empty and
  // quiescent drives STW #2 as a FRESH handshake generation. If it could
  // observe MarkingGray while STW #1's handshake is still open, STW #2's
  // beginHandshake would ++CurrentGeneration before the coordinator runs
  // endHandshake(Gen) -- tripping endHandshake's assuming(Gen ==
  // CurrentGeneration) and clearing the stop flag STW #2 just raised, hanging
  // STW #2's waitForAcks. std::condition_variable permits SPURIOUS wakeups, so
  // merely deferring the NOTIFY is not enough: MarkingGray itself must be
  // unobservable to workers until STW #1 has fully retired. Deferring the state
  // STORE to wakeCollectors() (run under GrayMutex, after endHandshake) means a
  // worker woken spuriously in the [beginMarking, wakeCollectors] window
  // re-checks its predicate, still sees MarkingRoot, and re-parks -- it cannot
  // reach STW #2 early no matter how it woke.
  //
  // Fire MarkGrayStart here (before the mutators are released in endHandshake)
  // so the snapshot window stays closed for phase observers
  // (GC.SetupHandshakeSnapshotsWithMutatorsParked). Mutators resuming in the
  // [endHandshake, wakeCollectors] gap still shade correctly: the state is
  // MarkingRoot throughout that gap and writeBarrier is armed for BOTH
  // MarkingRoot and MarkingGray (only Idle/Sweeping are quiet).
  notifyPhase(GCPhase::MarkGrayStart);
}

void Allocator::wakeCollectors() noexcept {
  // Publish MarkingGray and wake the workers, now that STW #1 has fully ended
  // (endHandshake returned). Publishing MarkingGray ONLY here (not in
  // beginMarking) is what deterministically closes the STW #1/STW #2 collision:
  // a worker that wakes before this point -- spuriously or otherwise -- cannot
  // observe MarkingGray (state is still MarkingRoot), so its predicate is
  // unsatisfied and it re-parks; it cannot reach STW #2's beginHandshake until
  // STW #1's endHandshake(Gen) has completed.
  //
  // Store MarkingGray UNDER GrayMutex, then notify: parked workers re-read
  // CurrentGCState under GrayMutex in their wait predicate, so setting the
  // state under the same lock before notifying makes the wakeup impossible to
  // lose (the standard state-under-lock / notify-after idiom).
  {
    std::unique_lock<std::mutex> Locker(GrayMutex);
    CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
  }
  GrayNotEmptyCV.notify_all();
}

void Allocator::waitForCycleComplete() noexcept {
  std::unique_lock<std::mutex> Locker(GCMutex);
  GCCV.wait(Locker, [this] {
    // Wake on teardown as well as cycle completion. During Closing the workers
    // abandon the cycle WITHOUT sweeping (so Idle is never republished), and on
    // Stop they exit outright; either way the coordinator must stop waiting
    // here so it can return from collect() and release its launch lease. Idle
    // stays the sole wake reason on the normal path (Closing/Stop both false).
    return CurrentGCState.load(std::memory_order_acquire) == GCState::Idle ||
           Stop.load(std::memory_order_acquire) ||
           (Ctrl != nullptr && Ctrl->isClosing());
  });
}

void Allocator::wakeForTeardown() noexcept {
  // Called by Controller::beginClosing() after it publishes Closing. Re-notify
  // both allocator CVs under their own mutexes so a coordinator parked in
  // waitForCycleComplete() and any worker parked on an empty Gray queue
  // re-evaluate their (now Closing-aware) predicates and abandon the cycle.
  // Taking each mutex before notifying closes the lost-wakeup window against a
  // waiter sitting between its predicate check and wait().
  {
    std::unique_lock<std::mutex> Locker(GCMutex);
  }
  GCCV.notify_all();
  {
    std::unique_lock<std::mutex> Locker(GrayMutex);
  }
  GrayNotEmptyCV.notify_all();
}

bool Allocator::legacyManualCollect(bool ScanNativeStack) noexcept {
  GCState State = GCState::Idle;
  if (!CurrentGCState.compare_exchange_strong(State, GCState::MarkingRoot,
                                              std::memory_order_acq_rel)) {
    return false;
  }
  NextGC.store(std::chrono::steady_clock::now() + std::chrono::seconds(1),
               std::memory_order_release);
  markRoots(ScanNativeStack);
  // Publish MarkingGray under GrayMutex and wake the workers (see
  // beginMarking); fire MarkGrayStart outside the lock so an observer's barrier
  // can't deadlock.
  {
    std::unique_lock<std::mutex> Locker(GrayMutex);
    CurrentGCState.store(GCState::MarkingGray, std::memory_order_release);
  }
  notifyPhase(GCPhase::MarkGrayStart);
  GrayNotEmptyCV.notify_all();
  // Block until the workers terminate the cycle and publish Idle.
  std::unique_lock<std::mutex> Locker(GCMutex);
  GCCV.wait(Locker, [this] {
    return CurrentGCState.load(std::memory_order_acquire) == GCState::Idle;
  });
  return true;
}

bool Allocator::manualCollect(bool ScanNativeStack) noexcept {
  // Controller-backed: route through the coordinator-driven setup handshake
  // (STW #1), threading the real native-scan request through (the AOT `coll`
  // host function calls manualCollect(true) and needs it). Standalone
  // (Ctrl == nullptr, the bare-Allocator unit tests): the old inline path,
  // which needs no registry or coordinator.
  return Ctrl ? Ctrl->collect(true, ScanNativeStack)
              : legacyManualCollect(ScanNativeStack);
}

void Allocator::autoCollect(bool ScanNativeStack) noexcept {
  if (Ctrl) {
    Ctrl->collect(false, ScanNativeStack);
  }
}

void Allocator::doWriteBarrier(uint8_t *Target) noexcept { markGray(Target); }

bool Allocator::ObjectIndex::contains(const Header *H) const noexcept {
  if (Cap == 0) {
    return false;
  }
  const size_t Mask = Cap - 1;
  size_t I = mix(H) & Mask;
  // Bounded by Cap: a table that is all entries and tombstones (no empty slot)
  // would otherwise spin forever. insert()'s load factor keeps at least half
  // the slots empty, so this bound is never actually reached.
  for (size_t N = 0; N != Cap; ++N) {
    Header *S = Slots[I];
    if (S == nullptr) {
      return false; // end of the probe chain
    }
    if (S == H) {
      return true;
    }
    I = (I + 1) & Mask;
  }
  return false;
}

void Allocator::ObjectIndex::erase(const Header *H) noexcept {
  if (Cap == 0) {
    return;
  }
  const size_t Mask = Cap - 1;
  size_t I = mix(H) & Mask;
  for (size_t N = 0; N != Cap; ++N) {
    Header *S = Slots[I];
    if (S == nullptr) {
      return;
    }
    if (S == H) {
      // Tombstone rather than clear: clearing would truncate the probe chain of
      // any entry that collided with this slot, making it unfindable. The
      // tombstones are reclaimed by the next rehash, which only insert()
      // drives.
      Slots[I] = tombstone();
      --Count;
      ++Tombs;
      return;
    }
    I = (I + 1) & Mask;
  }
}

bool Allocator::ObjectIndex::insert(Header *H) noexcept {
  // Keep the table at most half full COUNTING tombstones: that is what bounds
  // the probe chains and guarantees the loop below finds an empty slot.
  if ((Count + Tombs + 1) * 2 > Cap && !grow()) {
    return false;
  }
  const size_t Mask = Cap - 1;
  size_t I = mix(H) & Mask;
  size_t FirstTomb = Cap; // Cap == "no tombstone seen yet"
  for (;;) {
    Header *S = Slots[I];
    if (S == nullptr) {
      // Prefer the earliest tombstone on this chain so the chain shortens.
      if (FirstTomb != Cap) {
        Slots[FirstTomb] = H;
        --Tombs;
      } else {
        Slots[I] = H;
      }
      ++Count;
      return true;
    }
    if (S == tombstone()) {
      if (FirstTomb == Cap) {
        FirstTomb = I;
      }
    } else if (S == H) {
      return true; // already indexed; allocate() never re-inserts, but be total
    }
    I = (I + 1) & Mask;
  }
}

bool Allocator::ObjectIndex::grow() noexcept {
  // Live entries past a quarter of capacity means we genuinely need more room;
  // otherwise the pressure is tombstones, and a same-capacity rehash reclaims
  // them without doubling the table.
  const size_t NewCap =
      (Cap == 0) ? 1024 : (((Count + 1) * 4 > Cap) ? Cap * 2 : Cap);
  return rehash(NewCap);
}

bool Allocator::ObjectIndex::rehash(size_t NewCap) noexcept {
  auto **NewSlots = static_cast<Header **>(
      ::operator new(NewCap * sizeof(Header *), std::nothrow));
  if (NewSlots == nullptr) {
    // The only failure path. It surfaces as allocate() returning nullptr, i.e.
    // a normal out-of-memory for the guest -- never an exception on a noexcept
    // path. The index is left untouched and stays usable.
    return false;
  }
  std::fill_n(NewSlots, NewCap, nullptr);
  const size_t NewMask = NewCap - 1;
  for (size_t I = 0; I < Cap; ++I) {
    Header *S = Slots[I];
    if (S == nullptr || S == tombstone()) {
      continue;
    }
    size_t J = mix(S) & NewMask;
    while (NewSlots[J] != nullptr) {
      J = (J + 1) & NewMask;
    }
    NewSlots[J] = S;
  }
  ::operator delete(Slots);
  Slots = NewSlots;
  Cap = NewCap;
  Tombs = 0;
  return true;
}

[[nodiscard]] uint8_t *Allocator::doAllocate(uint32_t N) noexcept {
  // Reserve N via CAS, rejecting when Old + N exceeds Threshold. A plain
  // fetch_add tests pre-increment Used and lets one large N overshoot; summing
  // first keeps a hard bound. Old (uint64_t) + N (uint32_t) can't wrap.
  const uint64_t Limit = Threshold.load(std::memory_order_relaxed);
  uint64_t Old = Used.load(std::memory_order_relaxed);
  do {
    if (Old + N > Limit) {
      return nullptr;
    }
  } while (!Used.compare_exchange_weak(Old, Old + N, std::memory_order_acq_rel,
                                       std::memory_order_relaxed));
  // Over-align to the header (16): RawData embeds ValVariant whose SIMD members
  // need 16-byte alignment, but malloc guarantees only max_align_t (8 on some
  // 32-bit ABIs) -- misaligned payload is UB / SIGBUS on vector loads.
  uint8_t *P = static_cast<uint8_t *>(
      ::operator new(N, std::align_val_t{alignof(Header)}, std::nothrow));
  spdlog::debug("{} allocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  if (unlikely(P == nullptr)) {
    // Failed: release the reserved bytes.
    Used.fetch_sub(N, std::memory_order_acq_rel);
  }
  return P;
}

void Allocator::doDeallocate(uint8_t *P, uint32_t N) noexcept {
  spdlog::debug("{} deallocate({}) {}"sv, std::this_thread::get_id(), N,
                fmt::ptr(P));
  Used.fetch_sub(N, std::memory_order_acq_rel);
  ::operator delete(P, std::align_val_t{alignof(Header)});
}

void Allocator::addTable(Runtime::Instance::TableInstance &Table) noexcept {
  std::unique_lock<std::mutex> Locker(HeapMutex);
  Heaps.emplace_back(&Table);
}

void Allocator::removeTable(Runtime::Instance::TableInstance &Table) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HeapMutex);
  auto It = std::find(Heaps.begin(), Heaps.end(), &Table);
  if (It != Heaps.end()) {
    Heaps.erase(It);
  }
}

void Allocator::addGlobal(Runtime::Instance::GlobalInstance &Global) noexcept {
  std::unique_lock<std::mutex> Locker(GlobalMutex);
  Globals.emplace_back(&Global);
}

void Allocator::removeGlobal(
    Runtime::Instance::GlobalInstance &Global) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(GlobalMutex);
  auto It = std::find(Globals.begin(), Globals.end(), &Global);
  if (It != Globals.end()) {
    Globals.erase(It);
  }
}

void Allocator::addElem(Runtime::Instance::ElementInstance &Elem) noexcept {
  std::unique_lock<std::mutex> Locker(ElemMutex);
  Elems.emplace_back(&Elem);
}

void Allocator::removeElem(Runtime::Instance::ElementInstance &Elem) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(ElemMutex);
  auto It = std::find(Elems.begin(), Elems.end(), &Elem);
  if (It != Elems.end()) {
    Elems.erase(It);
  }
}

void Allocator::addException(
    Runtime::Instance::ExceptionInstance &Exception) noexcept {
  std::unique_lock<std::mutex> Locker(ExceptionMutex);
  Exceptions.emplace_back(&Exception);
}

void Allocator::removeException(
    Runtime::Instance::ExceptionInstance &Exception) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(ExceptionMutex);
  auto It = std::find(Exceptions.begin(), Exceptions.end(), &Exception);
  if (It != Exceptions.end()) {
    Exceptions.erase(It);
  }
}

void Allocator::retainResult(const RefVariant &Ref) noexcept {
  // Like releaseRef/releaseAllRefs: skip during teardown so we don't append to
  // a host-root bag about to be destroyed.
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HostRootsMutex);
  HostRoots.emplace_back(Ref.getPtr<uint8_t>());
}

WASMEDGE_EXPORT void Allocator::releaseRef(const RefVariant &Ref) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HostRootsMutex);
  // Scan from the back: retainResult appends, so LIFO matches quickly. Remove
  // via swap-with-back + pop_back -- HostRoots is an unordered bag matched by
  // pointer identity, so order and which instance don't matter.
  auto It =
      std::find(HostRoots.rbegin(), HostRoots.rend(), Ref.getPtr<uint8_t>());
  if (It != HostRoots.rend()) {
    *It = HostRoots.back();
    HostRoots.pop_back();
  }
}

WASMEDGE_EXPORT void Allocator::releaseAllRefs() noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(HostRootsMutex);
  HostRoots.clear();
}

void Allocator::retainScopedRef(const RefVariant &Ref) noexcept {
  // Like retainResult: skip during teardown so we don't append to a root bag
  // about to be destroyed.
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(ScopedRootsMutex);
  ScopedRoots.emplace_back(Ref.getPtr<uint8_t>());
}

void Allocator::releaseScopedRef(const RefVariant &Ref) noexcept {
  if (Stop.load(std::memory_order_acquire) == true) {
    return;
  }
  std::unique_lock<std::mutex> Locker(ScopedRootsMutex);
  // Scan from the back: retainScopedRef appends, so LIFO matches quickly.
  // Remove via swap-with-back + pop_back -- ScopedRoots is an unordered bag
  // matched by pointer identity, so order and which instance don't matter.
  auto It = std::find(ScopedRoots.rbegin(), ScopedRoots.rend(),
                      Ref.getPtr<uint8_t>());
  if (It != ScopedRoots.rend()) {
    *It = ScopedRoots.back();
    ScopedRoots.pop_back();
  }
}

void Allocator::markGray(uint8_t *Pointer) noexcept {
  if (const auto Address = reinterpret_cast<uintptr_t>(Pointer);
      Address <= sizeof(Header) || Address % alignof(Header) != 0) {
    return;
  }
  Header *H = reinterpret_cast<Header *>(Pointer - sizeof(Header));
  // Hold GrayMutex across BOTH the shade and the work-list push so the shade is
  // atomic w.r.t. a worker draining toward termination: otherwise H is briefly
  // coloured gray but on no list, and the in-flight-tracer quiescence check
  // could miss it, freeing H's still-white children (reachable only through H)
  // and leaving dangling pointers when H is later traced (use-after-free). A
  // sweep cannot interpose either -- NOT because sweep holds GrayMutex (it does
  // not), but because the STW #2 barrier-close publishes GCState::Sweeping
  // while every mutator is parked, which quiets writeBarrier (Idle/Sweeping are
  // no-ops). So no markGray runs at all during sweep.
  std::unique_lock<std::mutex> GrayLocker(GrayMutex);
  bool Shaded = false;
  {
    // Pointer is a CONSERVATIVE candidate -- a native-stack word, a table slot
    // -- so H may not be an object at all and reading H->Color could fault or
    // read unrelated memory. Validate membership first; the index is the oracle
    // the White set's erase() used to be.
    //
    // AllMutex is held across the validation AND the colour transition, which
    // is what pins H: the sweep un-indexes a dead object under this same lock
    // and frees its block only after releasing it, so an entry found here
    // cannot be freed, nor its address recycled, before the CAS below runs.
    // Lock order is GrayMutex -> AllMutex; this is the only path that holds
    // both.
    std::unique_lock<std::mutex> AllLocker(AllMutex);
    if (!Index.contains(H)) {
      return;
    }
    // Shade White -> Gray with one CAS. This replaces the old erase-from-White
    // test: winning the transition is what makes the shade exactly-once, so
    // only the winner pushes. A failed CAS means H is already gray or black --
    // nothing to do.
    uint8_t Expected =
        static_cast<uint8_t>(1U - BlackParity.load(std::memory_order_acquire));
    Shaded = H->Color.compare_exchange_strong(Expected, ColorGray,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire);
  }
  if (Shaded) {
    H->GrayNext = GrayHead;
    GrayHead = H;
    // Wake a worker blocked on an empty gray list so concurrently-shaded
    // objects are marked this cycle. Guarded to MarkingGray: the root scan
    // shades before that state is published and the driver wakes workers on the
    // transition, so notifying during root marking is only spurious.
    if (CurrentGCState.load(std::memory_order_acquire) ==
        GCState::MarkingGray) {
      GrayNotEmptyCV.notify_one();
    }
  }
}

WASMEDGE_GC_DISABLE_SANITIZER
void Allocator::markNativeStackRoots() noexcept {
  // setjmp spills callee-saved registers into Buf in this frame; getStack(&Buf)
  // anchors the scanned span at Buf so the spill is covered, and the scan runs
  // while this frame is alive. Catches a GC ref an AOT caller keeps only in a
  // callee-saved register across an alloc intrinsic -- on no root set, it would
  // otherwise be swept and dangle. Caller-saved registers need no spill. The
  // spill can't live in getStack() -- its frame is popped before the scan.
  std::jmp_buf Buf;
  (void)setjmp(Buf);
  // Conservative scan: intentionally reads data execution threads may be
  // modifying. Suppress TSan.
  TSAN_IGNORE_READS_BEGIN();
  for (const auto &Val : getStack(&Buf)) {
    markGray(getPointer(Val));
  }
  TSAN_IGNORE_READS_END();
}

Span<uint8_t *const> Allocator::getStack(void *Frame) noexcept {
  // Frame is the low end of the active stack (an address in a live frame), so
  // the span covers the spill and every caller frame up to the base. The stack
  // grows down, so the base is the high address; the region below Frame holds
  // no roots and is excluded (scanning it wastes time, over-retains, and faults
  // untouched pages).
  uintptr_t StackEnd = reinterpret_cast<uintptr_t>(Frame);
#if WASMEDGE_OS_LINUX
  pthread_attr_t Attr;
  int Error = pthread_getattr_np(pthread_self(), &Attr);
  if (likely(!Error)) {
    void *StackBegin;
    size_t StackSize;
    Error = pthread_attr_getstack(&Attr, &StackBegin, &StackSize);
    pthread_attr_destroy(&Attr);
    if (likely(Error == 0)) {
      uintptr_t StackBase = reinterpret_cast<uintptr_t>(StackBegin);
      uintptr_t StackTop = StackBase + StackSize;
      // Reject a frame outside [StackBase, StackTop]: the span length is the
      // unsigned (StackTop - StackEnd), which would underflow to a near-
      // SIZE_MAX count and scan out of bounds. Fall through to the error path.
      if (likely(StackEnd >= StackBase && StackEnd <= StackTop)) {
        return Span<uint8_t *const>{
            reinterpret_cast<uint8_t *const *>(StackEnd),
            (StackTop - StackEnd) / sizeof(uint8_t *)};
      }
    }
  }
  // Could not read native stack bounds: an empty span means no native-stack
  // roots this cycle, so a ref held only by AOT code in a register/stack slot
  // could be swept. Should never happen for a live thread; surface it.
  spdlog::error("GC: failed to read native stack bounds; native-stack roots "
                "will not be scanned this cycle"sv);
  return {};

#elif WASMEDGE_OS_MACOS
  uintptr_t StackBegin =
      reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
  // Reject a base not above the captured frame: (StackBegin - StackEnd) is
  // unsigned and would otherwise underflow.
  if (unlikely(StackEnd > StackBegin)) {
    spdlog::error("GC: native stack base below current frame; native-stack "
                  "roots will not be scanned this cycle"sv);
    return {};
  }
  return Span<uint8_t *const>{reinterpret_cast<uint8_t *const *>(StackEnd),
                              (StackBegin - StackEnd) / sizeof(uint8_t *)};

#elif WASMEDGE_OS_WINDOWS
#if defined(_M_X64) || defined(_M_IX86)
  uintptr_t StackBegin = reinterpret_cast<uintptr_t>(
      reinterpret_cast<winapi::NT_TIB_ *>(winapi::NtCurrentTeb())->StackBase);
#elif defined(_M_ARM64)
  winapi::ULONG_PTR_ LowLimit, HighLimit;
  winapi::GetCurrentThreadStackLimits(&LowLimit, &HighLimit);
  uintptr_t StackBegin = reinterpret_cast<uintptr_t>(HighLimit);
#else
#error Unsupported architecture for Windows
#endif
  // Reject a base not above the captured frame: (StackBegin - StackEnd) is
  // unsigned and would otherwise underflow.
  if (unlikely(StackEnd > StackBegin)) {
    spdlog::error("GC: native stack base below current frame; native-stack "
                  "roots will not be scanned this cycle"sv);
    return {};
  }
  return Span<uint8_t *const>{reinterpret_cast<uint8_t *const *>(StackEnd),
                              (StackBegin - StackEnd) / sizeof(uint8_t *)};

#else
#error Unsupported architecture
#endif
}

} // namespace GC
} // namespace WasmEdge
