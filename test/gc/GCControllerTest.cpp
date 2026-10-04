// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCControllerTest.cpp - GC controller tests -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for the GC controller: the mutator
/// registry, safepoints, the stop-the-world handshake, and the root
/// scan of native-running mutators and auxiliary roots.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

TEST(GC, RegistryReentrantAndRefcounted) {
  WasmEdge::GC::Controller Ctrl;
  std::vector<WasmEdge::ValVariant> Outer, Inner;
  {
    auto R1 = Ctrl.registerStack(Outer); // outer invocation
    {
      auto R2 = Ctrl.registerStack(Inner); // reentrant nested invocation
      size_t Count = 0;
      Ctrl.forEachStackRoot([&](const WasmEdge::ValVariant &) { ++Count; });
      // Both stacks are empty, so zero slots, but both must be enumerated
      // without crashing and the registry must hold two active stacks.
      EXPECT_EQ(Count, 0u);
      EXPECT_EQ(Ctrl.debugActiveStackCount(), 2u);
    }
    EXPECT_EQ(Ctrl.debugActiveStackCount(), 1u); // inner deregistered on scope
  }
  EXPECT_EQ(Ctrl.debugActiveStackCount(), 0u); // outer deregistered
}

TEST(GC, CurrentThreadRegisteredQuery) {
  // The fallible teardown entries (C API delete functions) need
  // to ask "is the calling thread inside one of this controller's own
  // invocations?" -- true exactly while the thread holds a registered value
  // stack, and scoped to this controller (a callback of VM1 deleting VM2 is
  // legal and must not be rejected).
  WasmEdge::GC::Controller Ctrl;
  EXPECT_FALSE(Ctrl.currentThreadRegistered());
  {
    std::vector<WasmEdge::ValVariant> S;
    auto Reg = Ctrl.registerStack(S);
    EXPECT_TRUE(Ctrl.currentThreadRegistered());
    // A different thread is not registered here.
    std::atomic<bool> Other{true};
    std::thread T([&] { Other.store(Ctrl.currentThreadRegistered()); });
    T.join();
    EXPECT_FALSE(Other.load());
  }
  EXPECT_FALSE(Ctrl.currentThreadRegistered());
}

TEST(GC, SafepointParksAndReleases) {
  WasmEdge::GC::Controller Ctrl;

  std::atomic<bool> MutatorReady{false};
  std::atomic<bool> MutatorParked{false};
  std::atomic<bool> Released{false};
  std::atomic<bool> PassedSafepoint{false};

  // Mutator thread: registers its own value stack (a StackManager always lives
  // on the thread that runs it), then reaches a safe point while a handshake is
  // in flight. It must not return from gcSafepoint() before the release.
  std::thread Mutator([&] {
    std::vector<WasmEdge::ValVariant> S;
    auto Reg = Ctrl.registerStack(S);
    MutatorReady.store(true);
    // Emulate the interpreter's hot poll.
    while (!Ctrl.stopRequested()) {
      std::this_thread::yield();
    }
    Ctrl.gcSafepoint(); // parks here until the coordinator releases the gen
    // Released is stored *before* endHandshake(), so observing it false here
    // would mean the safe point let the mutator run during the handshake.
    EXPECT_TRUE(Released.load());
    PassedSafepoint.store(true);
  });

  // This thread plays the coordinator.
  while (!MutatorReady.load()) {
    std::this_thread::yield();
  }
  const uint64_t Gen = Ctrl.debugBeginHandshake(); // request stop, bump gen
  while (!Ctrl.debugAllAcked(Gen)) {               // wait for the mutator's ack
    std::this_thread::yield();
  }
  MutatorParked.store(true);
  Released.store(true);
  Ctrl.debugEndHandshake(Gen); // release parked mutators

  Mutator.join();
  EXPECT_TRUE(MutatorParked.load());
  EXPECT_TRUE(PassedSafepoint.load());
}

TEST(GC, AdmissionGateBlocksLateJoiner) {
  // Registration is the admission boundary: a thread that registers its stack
  // while a handshake is already in flight must not be admitted to Running
  // until the handshake releases. Before the admission gate, registerStack()
  // returned immediately, letting a late joiner mutate roots during the
  // coordinator's writer-free snapshot window (or enqueue work after a terminal
  // check).
  WasmEdge::GC::Controller Ctrl;

  // Begin a handshake with no mutators registered yet: StopFlag is raised.
  const uint64_t Gen = Ctrl.debugBeginHandshake();

  std::atomic<bool> Admitted{false};
  std::atomic<bool> StopSeenAtAdmission{true};
  std::thread Joiner([&] {
    std::vector<WasmEdge::ValVariant> S;
    // registerStack() must block in the admission gate (self-scan + ack +
    // park) until the handshake ends; Admitted is reached only after it
    // returns.
    auto Reg = Ctrl.registerStack(S);
    // Record before signalling: a correct gate admits only with StopFlag
    // observed clear, so a buggy fall-through -- admitted while the handshake
    // is still in flight, flag still raised -- is caught deterministically by
    // the post-join assert below, with no timing window.
    StopSeenAtAdmission.store(Ctrl.stopRequested());
    Admitted.store(true);
    (void)Reg;
  });

  // Deterministic observation instead of a sleep: a correctly-gated joiner
  // parks (ParkedMutators goes nonzero); a buggy non-blocking registration
  // sets Admitted instead. Either way this loop terminates, and the asserts
  // distinguish the two.
  while (Ctrl.debugParkedMutators() == 0 && !Admitted.load()) {
    std::this_thread::yield();
  }
  EXPECT_FALSE(Admitted.load()); // parked behind the active handshake

  Ctrl.debugEndHandshake(Gen); // release the gate
  Joiner.join();
  EXPECT_TRUE(Admitted.load());
  // The gate must have released (StopFlag lowered) before admission.
  EXPECT_FALSE(StopSeenAtAdmission.load());
}

TEST(GC, SpinningGuestReachesSafepoint) {
  // Reuses ConcurrentAllocWasm's alloc_loop export: a real back-edge loop
  // (the interpreter's driving loop polls at the top of the back edge),
  // run with a large iteration count so the coordinator's handshake overlaps
  // its execution.
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, GCMod, ConcurrentAllocWasm));

  auto &Ctrl = VM.getExecutor().getController();
  std::atomic<bool> HandshakeDone{false};
  // Load-bearing: Executor::invoke() registers a fresh StackManager for the
  // duration of the call and deregisters it on return, so allAcked() would
  // also (vacuously) return true once the guest has already finished and
  // deregistered -- with an empty registry, "all" entries trivially satisfy
  // the check. Observing the registered stack count still nonzero at the
  // moment the ack is seen proves the ack came from a genuine safe-point
  // park while the guest was still running, not from the registry emptying
  // out after natural completion.
  std::atomic<bool> AckedWhileGuestStillRunning{false};

  std::thread Coordinator([&] {
    while (Ctrl.debugActiveStackCount() == 0) {
      std::this_thread::yield();
    }
    const uint64_t Gen = Ctrl.debugBeginHandshake();
    while (!Ctrl.debugAllAcked(Gen)) {
      std::this_thread::yield();
    }
    AckedWhileGuestStillRunning.store(Ctrl.debugActiveStackCount() > 0);
    Ctrl.debugEndHandshake(Gen);
    HandshakeDone.store(true);
  });

  auto Result = VM.execute(
      "alloc_loop",
      std::initializer_list<ValVariant>{UINT32_C(300000), UINT32_C(4)},
      {ValType(TypeCode::I32), ValType(TypeCode::I32)});
  ASSERT_TRUE(Result); // completes only because it parked and was released

  Coordinator.join();
  EXPECT_TRUE(HandshakeDone.load());
  EXPECT_TRUE(AckedWhileGuestStillRunning.load());
}

TEST(GC, SetupHandshakeSnapshotsWithMutatorsParked) {
  WasmEdge::GC::Controller Ctrl;
  // Register two mutator stacks on two threads; one spins hitting safepoints.
  std::vector<WasmEdge::ValVariant> S1;
  auto R1 = Ctrl.registerStack(S1);
  std::atomic<bool> Stop{false};
  std::atomic<bool> InSnapshot{false};
  std::atomic<bool> MutatorRanDuringSnapshot{false};
  // Non-vacuity gate: without it the mutator might not have entered its spin
  // loop before collect() completes, so MutatorRanDuringSnapshot would stay
  // false regardless of correctness. The coordinator waits on this before
  // starting collect(), so the mutator is provably looping (and thus would set
  // MutatorRanDuringSnapshot if it failed to park during the snapshot window).
  std::atomic<bool> MutatorEnteredLoop{false};

  std::thread Mutator([&] {
    std::vector<WasmEdge::ValVariant> S2;
    auto R2 = Ctrl.registerStack(S2);

    while (!Stop.load()) {
      MutatorEnteredLoop.store(true);
      if (InSnapshot.load()) {
        MutatorRanDuringSnapshot.store(true); // must not happen while parked
      }
      Ctrl.gcSafepoint();
    }
  });

  // Hook: set InSnapshot around the shared-root snapshot via the phase
  // observer.
  struct Obs : WasmEdge::GC::PhaseObserver {
    std::atomic<bool> *In;
    void onPhase(WasmEdge::GC::GCPhase P) noexcept override {
      if (P == WasmEdge::GC::GCPhase::MarkRootStart)
        In->store(true);
      if (P == WasmEdge::GC::GCPhase::MarkGrayStart)
        In->store(false);
    }
  } O;
  O.In = &InSnapshot;
  Ctrl.setPhaseObserver(&O);

  // Deterministic: do not snapshot until the mutator is actively looping.
  while (!MutatorEnteredLoop.load()) {
    std::this_thread::yield();
  }
  EXPECT_TRUE(Ctrl.collect(true, false));
  Stop.store(true);
  Mutator.join();
  EXPECT_FALSE(MutatorRanDuringSnapshot.load());
}

TEST(GCThread, CollectDoesNotHangOnNativeRunningMutator) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  WasmEdge::GC::Controller Ctrl;
  auto &Alloc = Ctrl.getAllocator();
  std::vector<WasmEdge::ValVariant> S;

  std::atomic<bool> InNative{false};
  std::atomic<bool> LetNativeFinish{false};
  std::atomic<bool> Collected{false};

  // Mutator: registers a stack, roots a freshly-allocated GC object only on
  // that registered stack (nowhere else), then enters a "host call"
  // (NativeScope) and stays there. It never reaches a safe point, so the
  // coordinator must (a) not wait on it (no hang) and (b) still scan its stable
  // stack in place (scanNonRunningRoots) -- otherwise the rooted object is
  // swept while live.
  std::thread Mutator([&] {
    auto Reg = Ctrl.registerStack(S);
    void *P = Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
    ASSERT_NE(P, nullptr);
    RefVariant Ref(ValType(TypeCode::Ref, TypeCode::StructRef),
                   static_cast<RawData *>(P));
    S.emplace_back(Ref); // the only root for this object

    WasmEdge::GC::Controller::NativeScope Native(Ctrl);
    InNative.store(true);
    while (!LetNativeFinish.load()) {
      std::this_thread::yield();
    }
  });

  while (!InNative.load()) {
    std::this_thread::yield();
  }

  const uint64_t UsageWithObject = Alloc.getMemoryUsage();
  ASSERT_GT(UsageWithObject, 0u); // the object is allocated and live

  // Coordinator: must complete both cycles without hanging on the native
  // thread. Two cycles are needed to prove root scanning: the object is
  // born-gray so cycle 1 keeps it regardless; only a live root re-grays it
  // through cycle 2 (post-swap it is white otherwise). If scanNonRunningRoots
  // were a no-op the object would be swept in cycle 2 and usage would drop to
  // 0 -- the survival assertion below then goes red.
  std::thread Collector([&] {
    EXPECT_TRUE(Ctrl.collect(true, false));
    EXPECT_TRUE(Ctrl.collect(true, false));
    Collected.store(true);
  });

  Collector.join(); // hangs here if waitForAcks waits on NativeRunning
  EXPECT_TRUE(Collected.load());

  // The object was rooted only on the native-running thread's registered stack;
  // its survival proves scanNonRunningRoots scanned that stack in place.
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject);

  LetNativeFinish.store(true);
  Mutator.join();
}

// AOT shadow-root scan: a managed ref held only in a native-frame
// shadow slot (never on the value stack) of a NativeRunning thread must survive
// a remote collection -- proving scanNonRunningRoots walks the shadow chain.
// This is the AOT analog of CollectDoesNotHangOnNativeRunningMutator (which
// roots on the value stack). If scanShadowChain were a no-op the object is
// swept in cycle 2 and the survival assertion goes red.
TEST(GCThread, ShadowRootScannedOnNativeRunningMutator) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Controller Ctrl;
  auto &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;

  std::atomic<bool> InNative{false};
  std::atomic<bool> LetNativeFinish{false};
  std::atomic<bool> Collected{false};

  std::thread Mutator([&] {
    auto Reg = Ctrl.registerStack(S);
    void *P = Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
    ASSERT_NE(P, nullptr);
    RefVariant Ref(ValType(TypeCode::Ref, TypeCode::StructRef),
                   static_cast<RawData *>(P));

    // The only root for this object: a shadow slot, not the value stack S.
    // S stays empty -- so survival can only come from the shadow chain.
    GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
    ASSERT_NE(H, nullptr);
    std::vector<ValVariant> Slots;
    Slots.emplace_back(Ref);
    GC::Controller::ShadowFrame Frame;
    // Push while Running (chain mutation is only legal in a non-scannable
    // state), then transition to NativeRunning. Destruction order (reverse)
    // pops the frame only after NativeScope restores Running.
    GC::Controller::ShadowScope SS(H, &Frame, 1, Slots.data());

    GC::Controller::NativeScope Native(Ctrl);
    InNative.store(true);
    while (!LetNativeFinish.load()) {
      std::this_thread::yield();
    }
  });

  while (!InNative.load()) {
    std::this_thread::yield();
  }

  const uint64_t UsageWithObject = Alloc.getMemoryUsage();
  ASSERT_GT(UsageWithObject, 0u);

  std::thread Collector([&] {
    EXPECT_TRUE(Ctrl.collect(true, false));
    EXPECT_TRUE(Ctrl.collect(true, false));
    Collected.store(true);
  });
  Collector.join();
  EXPECT_TRUE(Collected.load());

  // Survival proves the shadow slot on the native-running thread was scanned.
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject);

  LetNativeFinish.store(true);
  Mutator.join();
}

namespace {
// Stand-in for the executor's thread_local pending-exception payload: same
// shape (a thread_local AuxRoots whose address is fixed for the thread's
// lifetime), so the aux-root provider contract is exercised exactly as
// Executor::pendingExnRoots implements it.
thread_local GC::AuxRoots TestAuxRoots;
GC::AuxRoots *testAuxRootProvider() noexcept { return &TestAuxRoots; }
} // namespace

// A managed ref held only in a thread's auxiliary root vector must survive a
// remote collection while that thread sits in a host call. This is the shape of
// a propagating exception's payload: throwException copies it off the value
// stack and then erases those slots, so the aux roots are its only root until a
// handler re-pushes it. NativeRunning is what makes the assertion falsifiable
// -- a remote scan performs no conservative native scan of that thread, so
// without scanAuxRoots in scanNonRunningRoots the object is swept in cycle 2
// and the survival assertion goes red.
TEST(GCThread, AuxRootScannedOnNativeRunningMutator) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Controller Ctrl;
  Ctrl.setAuxRootProvider(&testAuxRootProvider);
  auto &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;

  std::atomic<bool> InNative{false};
  std::atomic<bool> LetNativeFinish{false};
  std::atomic<bool> Collected{false};

  std::thread Mutator([&] {
    auto Reg = Ctrl.registerStack(S);
    // The provider is consulted at registration, not on every publish.
    ASSERT_EQ(Ctrl.debugAuxRoots(), &TestAuxRoots);
    void *P = Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
    ASSERT_NE(P, nullptr);
    RefVariant Ref(ValType(TypeCode::Ref, TypeCode::StructRef),
                   static_cast<RawData *>(P));
    // The only root for this object: the aux vector, not the value stack S,
    // which stays empty.
    TestAuxRoots.Vals.emplace_back(Ref);

    GC::Controller::NativeScope Native(Ctrl);
    InNative.store(true);
    while (!LetNativeFinish.load()) {
      std::this_thread::yield();
    }
    TestAuxRoots.Vals.clear();
  });

  while (!InNative.load()) {
    std::this_thread::yield();
  }

  const uint64_t UsageWithObject = Alloc.getMemoryUsage();
  ASSERT_GT(UsageWithObject, 0u);

  std::thread Collector([&] {
    EXPECT_TRUE(Ctrl.collect(true, false));
    EXPECT_TRUE(Ctrl.collect(true, false));
    Collected.store(true);
  });
  Collector.join();
  EXPECT_TRUE(Collected.load());

  // Survival proves the aux roots of the native-running thread were scanned.
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject);

  LetNativeFinish.store(true);
  Mutator.join();
}

// The coordinator's own aux roots must be scanned by its self-scan. Driving
// collect() with ScanNative == false is what makes this falsifiable: no
// conservative native scan runs, so the locals holding the ref are invisible
// and survival can only come from scanAuxRoots in selfScanInto.
TEST(GC, AuxRootScannedByCoordinatorSelfScan) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Controller Ctrl;
  Ctrl.setAuxRootProvider(&testAuxRootProvider);
  auto &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  ASSERT_EQ(Ctrl.debugAuxRoots(), &TestAuxRoots);

  void *P = Alloc.allocate(
      [](void *Ptr) noexcept {
        auto *Raw = static_cast<RawData *>(Ptr);
        Raw->ModInst = nullptr;
        Raw->TypeIdx = 0;
        Raw->Length = 0;
      },
      sizeof(RawData));
  ASSERT_NE(P, nullptr);
  TestAuxRoots.Vals.emplace_back(RefVariant(
      ValType(TypeCode::Ref, TypeCode::StructRef), static_cast<RawData *>(P)));

  const uint64_t UsageWithObject = Alloc.getMemoryUsage();
  ASSERT_GT(UsageWithObject, 0u);
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject);

  TestAuxRoots.Vals.clear();
  // Dropped from the aux roots, the object is now unreachable and must go.
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_LT(Alloc.getMemoryUsage(), UsageWithObject);
}

// The aux roots of a thread are one object shared by every controller that
// thread registers with (the executor's payload is a per-thread thread_local,
// not per-executor), so a remote scan must exclude the owner's mutation through
// the roots' own mutex, not through the scanning controller's RegistryMtx --
// the owner mutates it under a different controller's locks when it is a host
// callback re-entering a second VM. Modelled here: the mutator parks
// NativeRunning while holding AuxRoots::Mtx (a payload assignment in flight);
// a remote collection must not reach its root snapshot (MarkRootStart) until
// that hold is released.
TEST(GCThread, AuxRootScanExcludesOwnerMutation) {
  GC::Controller Ctrl;
  Ctrl.setAuxRootProvider(&testAuxRootProvider);
  auto &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;

  std::atomic<bool> InNative{false};
  std::atomic<bool> LetNativeFinish{false};
  std::atomic<bool> Released{false};
  // Records whether the owner had released its hold by the time the
  // collector snapshotted the roots (which runs after scanNonRunningRoots).
  struct OrderObserver : GC::PhaseObserver {
    std::atomic<bool> *Released;
    std::atomic<int> SawReleasedAtRootStart{-1};
    void onPhase(GC::GCPhase P) noexcept override {
      if (P == GC::GCPhase::MarkRootStart) {
        SawReleasedAtRootStart.store(Released->load() ? 1 : 0);
      }
    }
  } Obs;
  Obs.Released = &Released;
  Alloc.setPhaseObserver(&Obs);

  std::thread Mutator([&] {
    auto Reg = Ctrl.registerStack(S);
    GC::Controller::NativeScope Native(Ctrl);
    std::unique_lock<std::mutex> Hold(TestAuxRoots.Mtx);
    InNative.store(true);
    while (!LetNativeFinish.load()) {
      std::this_thread::yield();
    }
    Released.store(true);
    Hold.unlock();
  });
  while (!InNative.load()) {
    std::this_thread::yield();
  }

  std::atomic<bool> CycleBegun{false};
  Alloc.setPreCycleHook([&] { CycleBegun.store(true); });
  std::thread Collector([&] { EXPECT_TRUE(Ctrl.collect(true, false)); });
  // Let the collector pass beginCycle and reach the remote scan (the only
  // Running-state mutator to wait for is none: the owner is NativeRunning), so
  // a scan that ignores AuxRoots::Mtx snapshots the roots before the release
  // below and records SawReleasedAtRootStart == 0.
  while (!CycleBegun.load()) {
    std::this_thread::yield();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  LetNativeFinish.store(true);

  Collector.join();
  Mutator.join();
  Alloc.setPhaseObserver(nullptr);
  Alloc.setPreCycleHook(nullptr);
  EXPECT_EQ(Obs.SawReleasedAtRootStart.load(), 1);
}

// Wiring: the executor installs its pending-exception payload as the aux-root
// provider, so every mutator stack it registers publishes that thread's payload
// to the registry. This is what roots the payload's managed refs while an
// exception propagates out through the native frames -- the values are erased
// from the value stack the moment the pending record is set.
TEST(GC, PendingExceptionPayloadIsRegisteredAuxRoot) {
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  // No registered stack on this thread yet -> no entry, hence no aux roots.
  EXPECT_EQ(Exec.getController().debugAuxRoots(), nullptr);
  {
    Runtime::StackManager StackMgr(Exec.getController());
    EXPECT_EQ(Exec.getController().debugAuxRoots(),
              WasmEdge::Executor::Executor::pendingExnRoots());
    EXPECT_NE(Exec.getController().debugAuxRoots(), nullptr);
  }
  // The entry retires with the stack; nothing dangles onto the thread_local.
  EXPECT_EQ(Exec.getController().debugAuxRoots(), nullptr);
}

TEST(GCThread, CollectRestoresPriorCoordinatorState) {
  // A collect() entered while the coordinator's entry is
  // NativeRunning must restore NativeRunning afterwards, not hardcode Running.
  // Driven directly through the Controller (no production path reaches collect
  // while NativeRunning today). Single mutator: waitForAcks/scanNonRunningRoots
  // skip self, so the cycle completes deterministically.
  GC::Controller Ctrl;
  std::vector<ValVariant> Stack;
  auto Reg = Ctrl.registerStack(Stack);
  ASSERT_EQ(Ctrl.currentMutatorState(), GC::Controller::MutatorState::Running);
  {
    GC::Controller::NativeScope Native(Ctrl); // entry -> NativeRunning
    ASSERT_EQ(Ctrl.currentMutatorState(),
              GC::Controller::MutatorState::NativeRunning);
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    // Pre-fix: restored to Running (corrupt). Post-fix: still NativeRunning.
    EXPECT_EQ(Ctrl.currentMutatorState(),
              GC::Controller::MutatorState::NativeRunning);
  }
  // NativeScope dtor restores Running.
  EXPECT_EQ(Ctrl.currentMutatorState(), GC::Controller::MutatorState::Running);
}

// Regression: STW #2 is driven by a collector worker and, via
// waitForAcks, waits for every registered Running mutator to park at a safe
// point. The coordinator that called collect() is itself a registered Running
// mutator blocked in waitForCycleComplete -- it can never reach a safe point,
// so STW #2 would hang on it unless it is excluded (setSelfBlocked). A second
// registered mutator that keeps hitting safe points must also be handshaked and
// released by STW #2 without hanging. Runs several cycles to stress the path.
TEST(GCThread, TerminationStopDoesNotHangWithRegisteredMutators) {
  GC::Controller Ctrl;
  std::atomic<bool> StopMutator{false};
  std::atomic<bool> SecondReady{false};

  // Second mutator: registered + Running, repeatedly hitting safe points so it
  // acknowledges both STW #1 and STW #2 of every cycle.
  std::thread Second([&] {
    std::vector<ValVariant> S;
    auto Reg = Ctrl.registerStack(S);
    SecondReady.store(true);
    while (!StopMutator.load()) {
      Ctrl.gcSafepoint();
      std::this_thread::yield();
    }
  });
  while (!SecondReady.load()) {
    std::this_thread::yield();
  }

  std::atomic<bool> Done{false};
  std::thread Coordinator([&] {
    std::vector<ValVariant> S;
    auto Reg = Ctrl.registerStack(S); // coordinator is registered + Running
    for (int I = 0; I < 8; ++I) {
      EXPECT_TRUE(Ctrl.collect(true, false));
    }
    Done.store(true);
  });

  Coordinator.join(); // hangs here if STW #2 waits on the blocked coordinator
  EXPECT_TRUE(Done.load());
  StopMutator.store(true);
  Second.join();
}

} // namespace
