// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#include "gc/controller.h"
#include "common/spdlog.h"
#include <algorithm>

namespace WasmEdge {
namespace GC {

Controller::~Controller() noexcept {
  // Full teardown drain. beginClosing() publishes Closing, wakes every parked /
  // blocked mutator, and then WAITS until no launch lease is outstanding and no
  // mutator is still inside gcSafepoint()'s park -- so the protocol state
  // (HandshakeMtx/ReleaseCV/DrainCV) and the Allocator (both members destroyed
  // after this body runs) are still alive for any detached async mutator that
  // has yet to unwind. Controller is the last-declared Executor member, so this
  // drain completes before any Executor state the async thread uses is
  // destroyed.
  beginClosing();
}

Controller::Lease Controller::acquireLease() noexcept {
  // Serialize admission against the teardown drain under DrainMtx, and refuse
  // once Closing is published. Without this, acquireLease could increment the
  // count after beginClosing()'s drain already observed zero leases and
  // returned, detaching a worker onto a Controller/Allocator about to be
  // destroyed.
  //
  // Airtight because the drain reads OutstandingLeases under this same
  // DrainMtx: if this call reads Closing == false it must have taken DrainMtx
  // before the drain's passing check (Closing is published before the drain can
  // ever pass), so its increment is visible to that check and the drain waits
  // for the release. If it takes DrainMtx after the drain started, it observes
  // Closing and returns an empty (unadmitted) lease.
  std::lock_guard<std::mutex> L(DrainMtx);
  if (Closing.load(std::memory_order_acquire)) {
    return Lease(); // admission refused: empty lease, valid() == false
  }
  OutstandingLeases.fetch_add(1, std::memory_order_acq_rel);
  return Lease(this);
}

Controller::HandleLease Controller::acquireHandleLease() noexcept {
  // Refuse once Closing: no new async handle may be published against a target
  // that teardown is concurrently destroying (mirrors acquireLease).
  std::lock_guard<std::mutex> L(DrainMtx);
  if (Closing.load(std::memory_order_acquire)) {
    return HandleLease();
  }
  HandleLeases.fetch_add(1, std::memory_order_acq_rel);
  return HandleLease(this);
}

void Controller::HandleLease::release() noexcept {
  if (Ctrl == nullptr) {
    return;
  }
  Ctrl->HandleLeases.fetch_sub(1, std::memory_order_acq_rel);
  Ctrl = nullptr;
}

void Controller::Lease::release() noexcept {
  if (Ctrl == nullptr) {
    return;
  }
  // Take DrainMtx BEFORE the drain-satisfying decrement so the decrement and
  // the notify are atomic w.r.t. the drain's predicate-check-under-wait: a
  // spurious wakeup can no longer let beginClosing() observe zero and destroy
  // DrainMtx/DrainCV between our fetch_sub and our notify (a use-after-free of
  // the drain's mutex and condition variable).
  std::lock_guard<std::mutex> L(Ctrl->DrainMtx);
  if (Ctrl->OutstandingLeases.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    Ctrl->DrainCV.notify_all();
    Ctrl->drainReleaseFired();
  }
  Ctrl = nullptr;
}

void Controller::registerBlockedWait(BlockedWaitToken *T) noexcept {
  std::lock_guard<std::mutex> L(BlockedMtx);
  BlockedWaiters.push_back(T);
}

void Controller::deregisterBlockedWait(BlockedWaitToken *T) noexcept {
  std::lock_guard<std::mutex> L(BlockedMtx);
  auto It = std::find(BlockedWaiters.begin(), BlockedWaiters.end(), T);
  if (It != BlockedWaiters.end()) {
    BlockedWaiters.erase(It);
  }
}

void Controller::wakeAllBlocked() noexcept {
  std::lock_guard<std::mutex> L(BlockedMtx);
  for (auto *T : BlockedWaiters) {
    // Set the wake flag and notify under the waiter's own mutex, exactly as
    // MemoryInstance::notifyAllWaiters does: the waiter checks Flag/Closing
    // while holding this mutex just before entering wait(), so taking it here
    // closes the lost-wakeup window (we either block until the waiter is parked
    // and then wake it, or run entirely before its pre-wait check, which then
    // sees Flag). Lock order BlockedMtx -> Waiter.Mutex is safe: the waiter
    // never acquires BlockedMtx while holding its Waiter.Mutex
    // (register/deregister run outside the wait loop).
    std::unique_lock<std::mutex> WL;
    if (T->Mtx != nullptr) {
      WL = std::unique_lock<std::mutex>(*T->Mtx);
    }
    if (T->Flag != nullptr) {
      T->Flag->store(true, std::memory_order_release);
    }
    if (T->CV != nullptr) {
      T->CV->notify_all();
    }
  }
}

void Controller::beginClosing() noexcept {
  // Publish Closing first: new parks in gcSafepoint() bail immediately and
  // waitForAcks() abandons its spin. Idempotent: ~Controller is the only
  // caller, but a redundant store is harmless.
  Closing.store(true, std::memory_order_release);

  // Wake mutators blocked on a runtime CV (atomic.wait) so their invocation can
  // unwind and release its launch lease.
  wakeAllBlocked();

  // Wake any mutator parked in gcSafepoint(): Closing is already a wake
  // condition of that park. Publish under HandshakeMtx so a mutator between its
  // predicate check and wait() cannot miss the wakeup.
  {
    std::lock_guard<std::mutex> HL(HandshakeMtx);
  }
  ReleaseCV.notify_all();

  // Wake any waiter parked in beginExclusiveOp() on TokenCV: its wait predicate
  // is Closing-aware, so it re-evaluates, dequeues its ticket, restores its
  // prior registry state, and returns false -- releasing the registered stack
  // the drain below waits on. Without this the abandon-on-Closing branch is
  // dead code and a parked exclusive-op loser would wedge teardown forever.
  // Publish under ExclusiveMtx (the same publish-under-lock discipline as the
  // ReleaseCV wake) so a waiter between its predicate check and wait() cannot
  // miss it. ExclusiveMtx is the OUTERMOST lock, so take it here where no other
  // controller mutex is held (the HandshakeMtx guard above has been released).
  {
    std::lock_guard<std::mutex> EL(ExclusiveMtx);
  }
  TokenCV.notify_all();

  // Wake the allocator-side waits that have no other teardown escape: the
  // coordinator parked in Allocator::waitForCycleComplete() (GCCV) and any
  // collector worker parked on an empty Gray queue (GrayNotEmptyCV). Both
  // re-evaluate their predicates -- now Closing-aware -- and abandon the
  // in-flight cycle WITHOUT sweeping, so the coordinator returns from collect()
  // and releases its launch lease, letting this drain make progress.
  Alloc.wakeForTeardown();

  // Self-teardown detection: if the calling thread is itself a registered
  // mutator of this controller -- a reentrant host callback that deletes its
  // own VM -- it can never deregister (it is mid-callback, ON the very stack we
  // would drain), so waiting for RegisteredStacks to reach zero would
  // self-deadlock. This is a fundamental misuse: the destructor cannot return
  // an error and cannot defer, so it CANNOT be made safe here -- the callback's
  // continued use of the freed executor/allocator after destruction is
  // undefined behaviour. Diagnose loudly and skip the drain rather than hang;
  // the wakes above already released every OTHER waiter. A higher layer (the C
  // API / embedder) must prevent deleting a VM from within its own host
  // callback.
  {
    std::lock_guard<std::mutex> RL(RegistryMtx);
    if (findEntry(std::this_thread::get_id()) != nullptr) {
      spdlog::error(
          "GC: a VM/executor is being destroyed from within its own running "
          "host callback (self-teardown). This is unsupported and leaves the "
          "callback using freed state (undefined behaviour). Destroy the VM "
          "only after all its invocations have returned. The C API rejects "
          "this (WasmEdge_VMDelete / WasmEdge_ExecutorDelete return without "
          "deleting); a C++ embedder must not destroy the VM from its own "
          "callback.");
      return;
    }
  }

  // Deletion-ordered handle contract: an undeleted Async handle co-owns its
  // launch lease, so the drain below waits for it. The fallible C-API delete
  // entries reject this case up front; a C++ embedder destroying the VM/
  // Executor while still holding a handle (declared before the VM, or simply
  // not yet released) reaches here and would otherwise block with no
  // diagnostic at all. Say what we are waiting for, then wait.
  if (const uint32_t Handles = HandleLeases.load(std::memory_order_acquire);
      Handles != 0) {
    spdlog::error(
        "GC: {} async handle(s) of this VM/executor are still alive; its "
        "teardown waits until every WasmEdge::Async handle it produced is "
        "destroyed. Release the handles before destroying the VM/executor "
        "(a handle declared before its VM in the same scope is destroyed "
        "after it and deadlocks here).",
        Handles);
  }

  // Drain: block until no launch lease is outstanding, no mutator is still
  // inside gcSafepoint()'s park, AND no thread still holds a registered stack.
  std::unique_lock<std::mutex> L(DrainMtx);
  DrainCV.wait(L, [this] {
    // All three conditions. Leases alone are not enough on two counts: (1) a
    // mutator awake but still INSIDE ReleaseCV.wait() would be blocked on a
    // condition_variable we are about to destroy (UB) -- ParkedMutators guards
    // that; (2) a synchronous execute()/invoke() on another thread holds a
    // registered stack but NO launch lease -- RegisteredStacks guards that, so
    // the drain does not complete while another thread is still running guest
    // code against this store/executor.
    return OutstandingLeases.load(std::memory_order_acquire) == 0 &&
           ParkedMutators.load(std::memory_order_acquire) == 0 &&
           RegisteredStacks.load(std::memory_order_acquire) == 0;
  });
}

Controller::Entry *Controller::findEntry(std::thread::id Tid) noexcept {
  for (auto &E : Entries) {
    if (E.Tid == Tid) {
      return &E;
    }
  }
  return nullptr;
}

Controller::ShadowHead *Controller::currentShadowHead() noexcept {
  std::lock_guard<std::mutex> L(RegistryMtx);
  if (auto *E = findEntry(std::this_thread::get_id())) {
    // .get() on the unique_ptr yields the heap cell's stable address; it stays
    // valid for this registration lifetime even as Entries relocates.
    return E->Shadow.get();
  }
  return nullptr;
}

Controller::Registration
Controller::registerStack(std::vector<ValVariant> &Stack) noexcept {
  const std::thread::id Tid = std::this_thread::get_id();
  bool Reentrant = false;
  MutatorState Saved = MutatorState::Running;
  {
    std::lock_guard<std::mutex> L(RegistryMtx);
    Entry *E = findEntry(Tid);
    if (E == nullptr) {
      E = &Entries.emplace_back();
      E->Tid = Tid;
      E->IncId = NextIncId++;
      E->State = MutatorState::Running;
      // Stable-address shadow-head cell for AOT root spills. Its
      // IncId mirrors the entry's so a retained pointer surviving this
      // registration's teardown + thread-id reuse is detectably stale.
      E->Shadow = std::make_unique<ShadowHead>();
      E->Shadow->IncId = E->IncId;
      // A brand-new entry starts un-acked (AckedGen 0).
    } else {
      // Reentrant guest: a nested StackManager created from within a host
      // callback. The entry is currently NativeRunning (the outer NativeScope
      // set it). The nested guest must run as Running so a concurrent handshake
      // WAITS for its ack instead of scanning its live, actively-mutating stack
      // in place (a data race and a possibly missed root). Record the prior
      // state; reset() restores it when the nested guest unwinds to the host.
      Reentrant = true;
      Saved = E->State;
    }
    // Cache this thread's auxiliary root vector (the executor's
    // pending-exception payload). Re-evaluated on a reentrant registration
    // too: same thread, so the provider yields the same stable address, and
    // this keeps the slot correct even if a nested registration is the first
    // one made after a provider was installed.
    if (AuxProvider != nullptr) {
      E->AuxRoots = AuxProvider();
    }
    E->Stacks.push_back(&Stack);
    ++E->RefCount;
    RegisteredStacks.fetch_add(1, std::memory_order_acq_rel);
  }
  // Admission gate. A thread joining (new entry) or re-entering guest execution
  // (reentrant, NativeRunning -> Running) while a handshake is in flight must
  // acknowledge and park before it is admitted to Running. Registration itself
  // is the admission boundary: a late arrival added AFTER the coordinator's
  // waitForAcks() already passed would otherwise mutate roots during the
  // writer-free snapshot window, or enqueue marking work after STW #2's
  // terminal check. admitToRunning() self-scans, acks, and parks until the flag
  // clears, then sets this entry Running with StopFlag observed clear (no
  // Running-but-unacked window a concurrent coordinator could skip). Fast no-op
  // when no handshake is in flight.
  admitToRunning();
  Registration R(this, &Stack);
  if (Reentrant) {
    R.HasSavedState = true;
    R.SavedState = Saved;
  }
  return R;
}

void Controller::Registration::reset() noexcept {
  if (Ctrl == nullptr) {
    return;
  }
  Controller *const C = Ctrl;
  // No Closing special case: the teardown drain in beginClosing() waits for
  // RegisteredStacks to reach zero before ~Controller destroys Entries, so the
  // registry is alive for as long as ANY registration exists -- including one
  // that unwinds after Closing was published. Skipping the erase here would
  // leave this (about to be freed) stack pointer in E.Stacks and a nested
  // registration's saved state unrestored, and Closing does NOT stop a
  // collection already past beginCycle from running scanNonRunningRoots over
  // exactly those stacks. (The self-teardown misuse -- destroying the
  // controller from its own registered thread -- skips the drain and is
  // undefined behaviour either way; see beginClosing.)
  {
    std::lock_guard<std::mutex> L(C->RegistryMtx);
    auto &Es = C->Entries;
    // Locate the entry holding this stack rather than assuming the destroying
    // thread is the registering one: a Registration is movable, and a
    // misattributed decrement would strand an entry in the registry forever
    // (the admission gate would then wait on a dead thread's ack).
    for (auto It = Es.begin(); It != Es.end(); ++It) {
      auto &S = It->Stacks;
      auto SIt = std::find(S.begin(), S.end(), Stack);
      if (SIt == S.end()) {
        continue;
      }
      S.erase(SIt);
      if (--It->RefCount == 0) {
        // Retire the entry. Erasing it (rather than keeping a zero-refcount
        // shell) is what stops allAcked() blocking on a thread that no longer
        // runs Wasm; any future registration on this OS thread -- including a
        // recycled thread id -- creates a fresh entry with a new IncId.
        Es.erase(It);
      } else if (HasSavedState) {
        // Reentrant registration unwinding back to its host callback: restore
        // the state the entry had before this nested guest stack made it
        // Running (typically NativeRunning -- the outer host frame is still on
        // the stack). Running -> NativeRunning is the safe transition direction
        // (the nested guest has finished; its stack is now stable and this
        // stack is being removed in the same critical section), mirroring
        // NativeScope's constructor.
        It->State = SavedState;
      }
      break;
    }
  }
  // Decrement AFTER releasing RegistryMtx (this thread no longer touches
  // Entries), but inside DrainMtx so the decrement + notify are atomic w.r.t.
  // the drain's wait predicate; see Lease::release for the use-after-free this
  // prevents.
  {
    std::lock_guard<std::mutex> DL(C->DrainMtx);
    if (C->RegisteredStacks.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      C->DrainCV.notify_all();
    }
  }
  Ctrl = nullptr;
  Stack = nullptr;
}

uint64_t Controller::beginHandshake() noexcept {
  uint64_t Gen;
  {
    std::lock_guard<std::mutex> L(HandshakeMtx);
    Gen = ++CurrentGeneration;
  }
  // Publish the generation before raising the flag: a mutator that observes
  // StopFlag then reads CurrentGeneration under HandshakeMtx cannot see a stale
  // generation and acknowledge the wrong (already-released) handshake.
  StopFlag.store(true, std::memory_order_release);
  return Gen;
}

bool Controller::allAcked(uint64_t Gen) noexcept {
  std::lock_guard<std::mutex> L(RegistryMtx);
  for (const auto &E : Entries) {
    if (E.AckedGen < Gen) {
      return false;
    }
  }
  return true;
}

void Controller::endHandshake(uint64_t Gen) noexcept {
  {
    // Clear the flag under HandshakeMtx so it cannot be lowered between a
    // parked mutator's predicate check and its wait() -- that would be a lost
    // wakeup and the mutator would never leave the safe point.
    std::lock_guard<std::mutex> L(HandshakeMtx);
    // The coordinator drives one generation at a time: the gen we are ending
    // must be the current one. A mismatch would mean a second cycle raced this
    // one past the Idle->MarkingRoot CAS, which cannot happen.
    assuming(Gen == CurrentGeneration);
    StopFlag.store(false, std::memory_order_release);
  }
  ReleaseCV.notify_all();
}

void Controller::gcSafepoint() noexcept {
  // A closing controller must never park a mutator: ~Controller is about to
  // destroy HandshakeMtx/ReleaseCV, and destroying a condition_variable with a
  // waiter blocked on it is UB. A detached async mutator can still be here.
  if (unlikely(Closing.load(std::memory_order_acquire))) {
    return;
  }

  // Loop, not a single park: the coordinator may end this generation and open
  // the next one while we are scanning. Re-entering acknowledges the new
  // generation instead of escaping it.
  while (StopFlag.load(std::memory_order_acquire) &&
         !Closing.load(std::memory_order_acquire)) {
    uint64_t Gen;
    {
      // Only to read the generation. HandshakeMtx must NOT be held across the
      // self-scan: the scan takes RegistryMtx -> GrayMutex -> AllMutex, and
      // nesting a second lock order under HandshakeMtx would create a cycle
      // with the coordinator (which takes RegistryMtx in allAcked()).
      std::lock_guard<std::mutex> L(HandshakeMtx);
      Gen = CurrentGeneration;
    }

    // Shade our own roots gray before parking, so the collector never has to
    // touch this thread's stacks itself. Conservatively scan the native stack
    // too: a parked mutator may be AOT code holding a ref only in a register.
    selfScanInto(true);

    {
      std::lock_guard<std::mutex> RL(RegistryMtx);
      if (auto *E = findEntry(std::this_thread::get_id())) {
        E->AckedGen = Gen;
      }
      // No entry: this thread has no registered value stack (its native-stack
      // roots were still shaded above). It has nothing to acknowledge, and
      // allAcked() does not wait for it -- but it still parks below, so it
      // cannot mutate the heap during the handshake.
    }

    // Announce this park to the teardown drain BEFORE entering the wait, so
    // beginClosing() cannot observe ParkedMutators == 0 and destroy ReleaseCV
    // while we are still blocked on it. Decremented (with a DrainCV notify on
    // reaching zero) immediately after the wait returns.
    ParkedMutators.fetch_add(1, std::memory_order_acq_rel);
    {
      std::unique_lock<std::mutex> L(HandshakeMtx);
      ReleaseCV.wait(L, [this, Gen] {
        // Released, or a newer handshake replaced ours (which the outer loop
        // will then acknowledge in turn), or the controller is tearing down and
        // we must leave before HandshakeMtx/ReleaseCV are destroyed.
        return Closing.load(std::memory_order_acquire) ||
               !StopFlag.load(std::memory_order_acquire) ||
               CurrentGeneration != Gen;
      });
    }
    {
      std::lock_guard<std::mutex> DL(DrainMtx);
      if (ParkedMutators.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        DrainCV.notify_all();
      }
    }
  }
}

bool Controller::collectForPressure(bool ScanNative) noexcept {
  // Every wait below stays in the Running state: an AOT caller reaches here
  // from inside an allocation intrinsic with its live references in registers
  // / native slots, which only a self-scan (gcSafepoint's conservative native
  // scan) can find -- the remote scan a Blocked thread gets would miss them.
  // gcSafepoint() acks and parks for whichever handshake is in flight and is a
  // no-op otherwise, so both loops are plain polls between stop-the-worlds.
  for (;;) {
    if (Closing.load(std::memory_order_acquire)) {
      return false;
    }
    if (collect(/*Manual=*/true, ScanNative)) {
      return true; // our own cycle ran to completion (collect waits for it)
    }
    if (!Alloc.isIdle()) {
      // Lost to a cycle already in flight: wait for its sweep. The caller's
      // retry then sees whatever it reclaimed, and a second pass through here
      // runs a cycle of our own if that was not enough.
      while (!Alloc.isIdle() && !Closing.load(std::memory_order_acquire)) {
        gcSafepoint();
        std::this_thread::yield();
      }
      return !Closing.load(std::memory_order_acquire);
    }
    // Heap Idle yet collect() lost: the exclusive token is held by a grower
    // (table.grow's stop-the-world) or by a host reader window
    // (retainManagedRefFromSlot), neither of which frees a single byte.
    // Returning now would let the caller fail with reclaimable garbage still
    // on the heap. Both holders are short-lived, so poll until the token is
    // ours; the safepoint poll is also what acks a grower's handshake, which
    // may be waiting on exactly this thread.
    gcSafepoint();
    std::this_thread::yield();
  }
}

void Controller::selfScanInto(bool ScanNative) noexcept {
  {
    std::lock_guard<std::mutex> L(RegistryMtx);
    if (auto *E = findEntry(std::this_thread::get_id())) {
      for (auto *S : E->Stacks) {
        for (const auto &V : *S) {
          Alloc.markGrayRoot(V); // precise value-stack scan
        }
      }
      // AOT shadow roots on this thread's own frames (refs spilled to native
      // spill slots). Acquire-load pairs with ShadowScope's release publish.
      scanShadowChain(*E);
      // Auxiliary roots: a pending exception's payload, which this thread may
      // be propagating through its native frames right now (this self-scan can
      // run from a safe point reached mid-propagation).
      scanAuxRoots(*E);
    }
  }
  if (!ScanNative) {
    // Interpreter path: value stacks are complete; a conservative native scan
    // would only over-retain stale ref bytes in the deep collect() call chain.
    return;
  }
  // Conservative native scan, with RegistryMtx released: only the precise walk
  // above needs the registry, and markNativeStackRoots() shades many words.
  //
  // It anchors at the setjmp spill buffer inside its OWN frame and scans
  // upward, which covers this thread's entire C++ call chain (the interpreter
  // frames above us included). Do NOT bound it at a caller-supplied frame
  // address: the stack grows down, so the spill buffer sits BELOW any caller's
  // frame and such a bound would exclude it -- losing exactly the
  // callee-saved-register roots the spill exists to capture.
  Alloc.markNativeStackRoots();
}

void Controller::waitForAcks(uint64_t Gen) noexcept {
  const auto Self = std::this_thread::get_id();
  for (;;) {
    // Teardown escape. Once Closing is published a mutator that bails its safe
    // point (gcSafepoint's top-level `if (Closing) return`) never acks this
    // generation and stays Running and unacked until it deregisters -- so
    // without this escape the spin below might never observe
    // allAcked() and this wait would hang forever, wedging both the coordinator
    // (collect -> waitForAcks) and a collector worker driving STW #2
    // (requestTerminationStop -> waitForAcks), and in turn the teardown drain
    // that waits on their launch leases. Treat Closing as "all acked" and
    // abandon; the roots this generation would have scanned no longer matter
    // because teardown performs NO sweep (see the Closing gate in the collector
    // worker's terminate path) -- ~Allocator frees the whole heap wholesale
    // after the drain confirms every mutator guest has finished.
    if (Closing.load(std::memory_order_acquire)) {
      return;
    }
    bool All = true;
    {
      std::lock_guard<std::mutex> L(RegistryMtx);
      for (auto &E : Entries) {
        // Only a Running mutator can reach a safe point; NativeRunning/Blocked
        // entries are scanned in place by scanNonRunningRoots and count as
        // pre-acked. Waiting on them would hang forever. The coordinator
        // self-scans inline, so it never waits on itself.
        if (E.Tid != Self && E.State == MutatorState::Running &&
            E.AckedGen < Gen) {
          All = false;
          break;
        }
      }
    }
    if (All) {
      return;
    }
    std::this_thread::yield();
  }
}

void Controller::scanNonRunningRoots() noexcept {
  const auto Self = std::this_thread::get_id();
  std::lock_guard<std::mutex> L(RegistryMtx);
  for (auto &E : Entries) {
    if (E.Tid == Self || E.State == MutatorState::Running) {
      continue; // self-scanned, or parked and self-scanned at its safe point
    }
    // A NativeRunning/Blocked thread is not touching its value stacks, so
    // scanning them here (while Running mutators are quiesced) is safe and is
    // what makes skipping its ack sound. Holds RegistryMtx across
    // markGrayRoot->markGray (RegistryMtx -> GrayMutex -> AllMutex), the
    // documented lock order.
    for (auto *S : E.Stacks) {
      for (const auto &V : *S) {
        Alloc.markGrayRoot(V);
      }
    }
    // AOT shadow roots of the NativeRunning/Blocked thread: refs it spilled to
    // native-frame shadow slots before parking in its host call. Without this
    // an AOT ref held only in a register/native slot across a host call would
    // be swept while live -- the AOT analog of the value-stack scan above.
    scanShadowChain(E);
    // Auxiliary roots of the NativeRunning/Blocked thread: the payload of an
    // exception it is propagating out through the native frames. Those refs
    // were erased from its value stack when the pending record was set, so this
    // is their only root.
    scanAuxRoots(E);
  }
}

void Controller::scanAuxRoots(Entry &E) noexcept {
  if (E.AuxRoots == nullptr) {
    return;
  }
  // The roots' own leaf mutex, not RegistryMtx: the owning thread mutates the
  // payload under this mutex from whichever executor it is currently running
  // (the payload is per thread, shared across controllers). Lock order
  // RegistryMtx -> AuxRoots::Mtx -> GrayMutex (via markGrayRoot).
  std::lock_guard<std::mutex> L(E.AuxRoots->Mtx);
  for (const auto &V : E.AuxRoots->Vals) {
    Alloc.markGrayRoot(V);
  }
}

void Controller::scanShadowChain(Entry &E) noexcept {
  if (!E.Shadow) {
    return;
  }
  // Count this walk so a fault on the walked thread (Fault::emitFault) waits
  // for it before abandoning the frames we are about to read; see
  // ShadowHead::Walkers. Incremented BEFORE the Head load: a truncation that
  // lands after the load then sees us and waits, and one that lands before is
  // simply reflected in the (shorter) chain we walk. Both seq_cst: the fault
  // side does the mirror image (store Head, then load Walkers), and only a
  // single total order over all four operations guarantees that one side sees
  // the other's write -- with release/acquire each side's store could stay
  // buffered past its own later load and both would read stale values.
  // Only a REMOTE walk is counted (selfScanInto walks the caller's own chain,
  // and a thread cannot fault while it scans itself -- counting it would make
  // a later fault of its own spin on its own count).
  const bool Remote = E.Tid != std::this_thread::get_id();
  if (Remote) {
    E.Shadow->Walkers.fetch_add(1, std::memory_order_seq_cst);
  }
  auto *F = E.Shadow->Head.load(std::memory_order_seq_cst);
  if (Remote && ShadowWalkHook) {
    ShadowWalkHook();
  }
  for (; F != nullptr; F = F->Prev) {
    for (uint32_t I = 0; I < F->Count; ++I) {
      Alloc.markGrayRoot(F->Slots[I]);
    }
  }
  if (Remote) {
    E.Shadow->Walkers.fetch_sub(1, std::memory_order_acq_rel);
  }
}

void Controller::requestTerminationStop() noexcept {
  // Reuse the STW infrastructure from a worker thread. beginHandshake bumps the
  // generation and raises StopFlag; waitForAcks blocks until every OTHER
  // Running mutator has parked at a safe point (the worker is unregistered, and
  // the coordinator was marked Blocked before it parked in
  // waitForCycleComplete, so neither is waited on). On return no mutator write
  // barrier is in flight.
  TerminationGen = beginHandshake();
  waitForAcks(TerminationGen);
}

void Controller::endTerminationStop() noexcept {
  // Lower StopFlag and wake the mutators parked for STW #2.
  endHandshake(TerminationGen);
}

bool Controller::setSelfBlocked(MutatorState &Prev) noexcept {
  std::lock_guard<std::mutex> L(RegistryMtx);
  if (auto *E = findEntry(std::this_thread::get_id())) {
    Prev = E->State;
    E->State = MutatorState::Blocked;
    return true;
  }
  return false;
}

void Controller::admitToState(MutatorState Target) noexcept {
  // Admit the calling thread to Target without ever exposing a transient
  // Running-but-unacked state to the coordinator. Used by the coordinator
  // restoring itself after a cycle, by registerStack admitting a joining /
  // reentrant mutator, by beginExclusiveOp after a park, by ~NativeScope, and
  // by restoreStateAfterFault.
  //
  // Why this is needed (a TOCTOU): the coordinator covers a mutator's roots
  // for an in-flight generation in one of two mutually exclusive ways. It
  // WAITS for a Running thread's ack (waitForAcks), or it SCANS a
  // NativeRunning/Blocked thread's stable stacks in place
  // (scanNonRunningRoots). These two decisions are read at different times
  // with RegistryMtx dropped between them. If a thread leaving NativeRunning
  // naively set Running and was then preempted before parking, the
  // coordinator could skip it in waitForAcks (saw NativeRunning) and again in
  // scanNonRunningRoots (now Running): its roots for that generation would be
  // scanned by nobody (a use-after-free).
  //
  // Invariant that closes the window: the state changes ONLY with StopFlag
  // observed clear under RegistryMtx (the same lock the coordinator reads
  // State under), or during teardown, where gcSafepoint is inert. During a
  // generation's active handshake StopFlag stays set, so the coordinator
  // observes a stable prior state for that generation (for NativeRunning:
  // waitForAcks skips it and scanNonRunningRoots scans it, redundantly with
  // the gcSafepoint self-scan below, which markGray makes idempotent). A
  // handshake that begins AFTER the change finds a Running thread in the
  // Running set and waits for its ack.
  //
  // In the common case no handshake is in flight, so the loop runs once. If
  // one is (or a new one races in after gcSafepoint returns), gcSafepoint
  // self-scans, acks, and parks until it releases us; then we retry. No lock
  // is held across gcSafepoint, which takes RegistryMtx itself.
  for (;;) {
    gcSafepoint();
    std::lock_guard<std::mutex> L(RegistryMtx);
    if (!StopFlag.load(std::memory_order_acquire) ||
        Closing.load(std::memory_order_acquire)) {
      if (auto *E = findEntry(std::this_thread::get_id())) {
        E->State = Target;
      }
      return;
    }
  }
}

bool Controller::tryBeginExclusiveOp(ExclusiveState Want,
                                     uint64_t &OutGen) noexcept {
  std::lock_guard<std::mutex> L(ExclusiveMtx);
  // Non-blocking winner path only. A closing controller admits no new owner
  // (the arbiter is being torn down), and a non-Free token means someone else
  // owns it. Either way return false at once with NO FIFO ticket and NO park.
  // This is the collector's acquire: a losing collection is skippable (it
  // simply returns false, matching beginCycle's "already collecting" loser), so
  // it must not queue -- see the header for why queuing a collector trips
  // endExclusiveOp.
  if (Closing.load(std::memory_order_acquire)) {
    return false;
  }
  if (ExclState != ExclusiveState::Free) {
    return false;
  }
  // Winner: take the token and mint a fresh owner generation (mirrors the
  // winner path in beginExclusiveOp).
  ExclState = Want;
  OutGen = ++ExclOwnerGen;
  return true;
}

bool Controller::beginExclusiveOp(ExclusiveState Want,
                                  uint64_t &OutGen) noexcept {
  MutatorState Prev = MutatorState::Running;
  bool WasBlocked = false;
  bool Won = false;
  {
    std::unique_lock<std::mutex> L(ExclusiveMtx);
    // A closing controller admits no new owner: the arbiter is being torn down.
    if (Closing.load(std::memory_order_acquire)) {
      return false;
    }
    if (ExclState == ExclusiveState::Free) {
      // Winner: take the token and mint a fresh owner generation. Mirrors the
      // single-owner election in Allocator's TerminationOwner CAS, but
      // blocking.
      ExclState = Want;
      OutGen = ++ExclOwnerGen;
      return true;
    }
    // Loser protocol. (1) Freeze roots: an interpreter value
    // stack is already precise and, once we publish Blocked below, stable for
    // the collector to scan in place; AOT code has spilled its register-held
    // refs into the shadow region registered at frame entry. Nothing more to
    // emit here. (2) The State/Closing recheck happened under this same lock
    // above. (3) Publish the pre-ackable Blocked state under RegistryMtx while
    // still holding ExclusiveMtx (lock order ExclusiveMtx -> RegistryMtx), so a
    // handshake the current owner raises counts this thread as pre-acked
    // (waitForAcks skips Blocked) without waiting for an ack we can never
    // deliver while parked; then enqueue a FIFO ticket.
    //
    // ONLY GROWERS MAY QUEUE. Tickets carry no per-ticket Want, so the reserved
    // handoff in endExclusiveOp labels every granted ticket OwnedGrowing; a
    // queued COLLECTOR would therefore be handed an OwnedGrowing token and trip
    // endExclusiveOp's assuming(ExclState == Have) when it released
    // OwnedCollecting. Collectors avoid this by construction -- collect() takes
    // the non-blocking tryBeginExclusiveOp and simply skips the cycle on a
    // miss. Assert that invariant here so it is self-enforcing at the one site
    // that could violate it, instead of resting on a comment.
    assuming(Want == ExclusiveState::OwnedGrowing);
    WasBlocked = setSelfBlocked(Prev);
    const uint64_t Ticket = ExclNextTicket++;
    ExclGrowQueue.push_back(Ticket);
    // (4) Predicate-loop wait, lost-wakeup-safe: we enqueued under ExclusiveMtx
    // and wait() atomically releases it, so a handoff (which takes ExclusiveMtx
    // to grant + notify) cannot slip between the enqueue and the park.
    TokenCV.wait(L, [&] {
      return ExclGrantedTicket == Ticket ||
             Closing.load(std::memory_order_acquire);
    });
    if (ExclGrantedTicket == Ticket) {
      // Won the reserved handoff: the releaser already set ExclState and a
      // fresh ExclOwnerGen for us. Consume the grant.
      ExclGrantedTicket = 0;
      OutGen = ExclOwnerGen;
      Won = true;
    } else {
      // Woke to Closing: abandon. Remove our still-queued ticket so the arbiter
      // carries no phantom waiter into teardown.
      for (auto It = ExclGrowQueue.begin(); It != ExclGrowQueue.end(); ++It) {
        if (*It == Ticket) {
          ExclGrowQueue.erase(It);
          break;
        }
      }
    }
  }
  // (5) Pass handshake admission before returning to guest execution: a
  // handshake may have started while we were parked Blocked, and we must
  // acknowledge it before resuming as Running (or restore the prior state on an
  // abandon). admitToState is a no-op fast path when no handshake is in flight.
  if (WasBlocked) {
    admitToState(Prev);
  }
  return Won;
}

void Controller::endExclusiveOp(uint64_t OwnerGen,
                                ExclusiveState Have) noexcept {
  std::lock_guard<std::mutex> L(ExclusiveMtx);
  // Exactly-once release: the releasing owner must be the live owner. A
  // mismatch means a double release or a stale generation (mirrors
  // endHandshake's generation assert).
  assuming(ExclState == Have);
  assuming(ExclOwnerGen == OwnerGen);
  // Invalidate this owner's generation so a second endExclusiveOp with the same
  // OwnerGen can no longer match; this also mints the fresh generation the
  // reserved handoff below grants to the next owner.
  ++ExclOwnerGen;
  if (!ExclGrowQueue.empty()) {
    // Reserved handoff to the oldest queued waiter: keep the token non-Free so
    // a newcomer taking the winner path cannot jump ahead of the FIFO. Hand it
    // the ticket + the fresh generation; wake everyone (only the granted
    // ticket's predicate passes).
    const uint64_t Next = ExclGrowQueue.front();
    ExclGrowQueue.pop_front();
    ExclState = ExclusiveState::OwnedGrowing;
    ExclGrantedTicket = Next;
  } else {
    ExclState = ExclusiveState::Free;
  }
  TokenCV.notify_all();
}

bool Controller::collect(bool Manual, bool ScanNative) noexcept {
  Allocator &A = Alloc;
  // Arbitrate against a concurrent stop-the-world grow through the
  // per-controller exclusive-operation owner: a collection must own the token
  // before it may drive a handshake, so growth and collection can never both
  // stop the world at once. Use the NON-blocking acquire: a losing collection
  // is skippable, so it must not enqueue as a FIFO grow-ticket (the reserved
  // handoff relabels every granted ticket OwnedGrowing, which would later trip
  // endExclusiveOp's assuming(ExclState == Have) when this collector released
  // OwnedCollecting). On a miss just skip this cycle -- the same "a collection
  // loser simply returns false" semantics beginCycle already has.
  uint64_t OwnerGen = 0;
  if (!tryBeginExclusiveOp(ExclusiveState::OwnedCollecting, OwnerGen)) {
    return false;
  }
  // One cycle at a time: the Idle->MarkingRoot CAS (plus the auto admission
  // gate) lives in the allocator. Bail if a cycle is already in flight or the
  // gate says not now.
  if (!A.beginCycle(Manual)) {
    // We own the token but no cycle will run, so no sweep-completing worker
    // will ever release it for us: release it immediately
    // (coordinator-immediate path). Every other post-acquire return releases
    // exactly once too -- either here, or deferred to the sweep-completing
    // worker below.
    endExclusiveOp(OwnerGen, ExclusiveState::OwnedCollecting);
    return false;
  }
  // Publish the owner generation for the sweep-completing worker to release
  // across the STW #2 handoff (see the worker body in allocator.cpp). Stored
  // before wakeCollectors() below, so it is set before any worker can observe
  // MarkingGray and reach the release.
  A.CollectionOwnerGen.store(OwnerGen, std::memory_order_release);
  // STW #1: bump the generation and raise the stop flag, then wait for every
  // OTHER Running mutator to acknowledge by parking. The coordinator is one of
  // the mutators; it does not ack itself.
  const uint64_t Gen = beginHandshake();
  waitForAcks(Gen);
  // A NativeRunning/Blocked mutator never reaches a safe point and so never
  // self-scans; the coordinator scans its (stable) stacks in place. Without
  // this the roots of a thread inside a host call would be missed.
  scanNonRunningRoots();
  // Shared-root snapshot with all mutators quiesced: no concurrent writer.
  A.snapshotSharedRootsInto();
  // The coordinator self-scans its own roots. The native scan is driven by the
  // caller's ScanNative flag -- NOT by Manual, which is orthogonal (the
  // admission-gate bypass). AOT callers (the alloc proxies, and the AOT `coll`
  // host function via manualCollect(true)) pass ScanNative == true because a
  // ref may live only in a register/native-stack slot; the interpreter's
  // precise path passes false, so its exact-usage tests see no conservative
  // over-retention.
  selfScanInto(ScanNative);
  // Fire MarkGrayStart (before releasing the mutators, so the snapshot window
  // stays closed), then release the parked mutators. beginMarking does NOT
  // publish MarkingGray -- that is deferred to wakeCollectors below.
  A.beginMarking();
  endHandshake(Gen);
  // Publish MarkingGray and wake the workers only now that STW #1 has fully
  // ended: a worker that observes MarkingGray and drains Gray drives STW #2 as
  // a fresh handshake generation, which must not overlap STW #1's (endHandshake
  // above would otherwise clear the stop flag STW #2 raised, hanging its
  // waitForAcks). Because MarkingGray is published here (not in beginMarking),
  // a worker woken spuriously in the [endHandshake, wakeCollectors] gap still
  // sees MarkingRoot and re-parks -- it cannot reach STW #2 before this point.
  A.wakeCollectors();
  // Concurrent mark drain + STW #2 termination + sweep run on the workers.
  // Before parking (this thread is a registered Running mutator that can no
  // longer reach a safe point), mark ourselves Blocked so the worker-driven
  // STW #2 does not wait on us forever -- see setSelfBlocked. Capture the
  // prior state and restore it once the cycle completes (mirrors
  // NativeScope::Prev) rather than hardcoding Running: a collect() entered
  // while this entry was NativeRunning must return to NativeRunning, not
  // Running.
  MutatorState Prev = MutatorState::Running;
  const bool WasBlocked = setSelfBlocked(Prev);
  A.waitForCycleComplete();
  if (WasBlocked) {
    admitToState(Prev);
  }
  // The exclusive token is NOT released here: the full cycle holds it across
  // the STW #2 handoff, and the sweep-completing worker releases it via
  // endExclusiveOp(OwnerGen, OwnedCollecting) after runSweepAndSwap() (see
  // allocator.cpp). On a teardown abandonment (Closing) no worker sweeps, so
  // the token is intentionally left owned -- the controller is closing and
  // admits no new owner (tryBeginExclusiveOp/beginExclusiveOp both refuse while
  // Closing), so the leaked token can never wedge a later acquire.
  return true;
}

Controller::NativeScope::NativeScope(Controller &C) noexcept
    : Ctrl(&C), Prev(MutatorState::Running) {
  // Flip this thread to NativeRunning so the coordinator stops waiting on its
  // ack and instead scans its stable stacks in place. Save the prior state so
  // nested native calls unwind correctly. If no entry exists (thread with no
  // registered value stack), there is nothing to track.
  std::lock_guard<std::mutex> L(Ctrl->RegistryMtx);
  if (auto *E = Ctrl->findEntry(std::this_thread::get_id())) {
    Prev = E->State;
    E->State = MutatorState::NativeRunning;
  }
}

Controller::NativeScope::~NativeScope() noexcept {
  // Restore the pre-native state through the admission gate, so this thread
  // never shows a transient Running-but-unacked state to the coordinator; see
  // admitToState.
  Ctrl->admitToState(Prev);
}

void Controller::restoreStateAfterFault(MutatorState BoundaryState) noexcept {
  // Restoring TO a scannable state (NativeRunning/Blocked) -- e.g. the boundary
  // was a host->guest reentry, so the entry should stay NativeRunning: a plain
  // locked store. We are entering (not leaving) a scannable state, so there is
  // no waitForAcks/scanNonRunningRoots TOCTOU window to close.
  if (BoundaryState != MutatorState::Running) {
    std::lock_guard<std::mutex> L(RegistryMtx);
    if (auto *E = findEntry(std::this_thread::get_id())) {
      E->State = BoundaryState;
    }
    return;
  }
  // Restoring TO Running leaves a (possibly stranded) NativeRunning state, so
  // it goes through the admission gate like ~NativeScope. When the entry was
  // never stranded this is one no-op safepoint plus a Running->Running store.
  admitToRunning();
}

} // namespace GC
} // namespace WasmEdge
