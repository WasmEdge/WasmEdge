// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCTeardownTest.cpp - GC teardown tests -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for VM and controller teardown while
/// synchronous or asynchronous mutators run, for the lifetime of
/// async handles, and for calls that start after closing begins.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

TEST(GC, TeardownDrainsSynchronousMutator) {
  // The teardown drain must wait for a synchronous registered stack on another
  // thread -- which holds no launch lease and is not parked -- before it
  // returns, so destruction never frees state a synchronous execute()/invoke()
  // is still using. Before the RegisteredStacks counter the drain waited only
  // on leases + parked mutators and would return while another thread still
  // ran.
  WasmEdge::GC::Controller Ctrl;

  std::atomic<bool> Registered{false};
  std::atomic<bool> ReleaseIt{false};
  std::atomic<uint32_t> StacksAtDrainReturn{UINT32_MAX};

  std::thread Mutator([&] {
    std::vector<WasmEdge::ValVariant> S;
    auto Reg = Ctrl.registerStack(S); // holds a registered stack, no lease
    Registered.store(true);
    while (!ReleaseIt.load()) {
      std::this_thread::yield();
    }
    // Reg is destroyed here (deregisters), which lets the drain reach zero.
  });

  while (!Registered.load()) {
    std::this_thread::yield();
  }
  std::thread Closer([&] {
    Ctrl.beginClosing(); // must block until RegisteredStacks == 0
    // Deterministic postcondition (replaces a sleep + poll): a correct drain
    // returns only after the mutator deregistered, so the counter it waited
    // on reads zero here -- always, no timing. A drain that skips the
    // RegisteredStacks condition returns while the mutator still holds its
    // registration (ReleaseIt is not yet set at that point, see below) and
    // records a nonzero count.
    StacksAtDrainReturn.store(Ctrl.debugRegisteredStacks());
  });

  // Release the mutator only after the closer has published Closing, so a
  // buggy early drain-return necessarily happens while the registration is
  // still held.
  while (!Ctrl.isClosing()) {
    std::this_thread::yield();
  }
  ReleaseIt.store(true);
  Mutator.join();
  Closer.join();
  EXPECT_EQ(StacksAtDrainReturn.load(), 0u);
}

TEST(GC, NestedDeregistrationAfterClosingStillUpdatesRegistry) {
  // A nested (reentrant) registration that unwinds after Closing was published
  // must still erase its stack pointer from the registry and restore the
  // entry's saved state. The drain in beginClosing() keeps Entries alive while
  // any registration is held, so touching it here is safe -- and skipping it
  // leaves a dangling std::vector* in E.Stacks that a collection past
  // beginCycle (which Closing does not stop) dereferences in
  // scanNonRunningRoots, plus a stale Running state on an entry that is really
  // NativeRunning.
  WasmEdge::GC::Controller Ctrl;

  std::atomic<bool> Registered{false};
  std::atomic<bool> CloseStarted{false};
  std::atomic<size_t> StacksAfterNested{SIZE_MAX};
  std::atomic<WasmEdge::GC::Controller::MutatorState> StateAfterNested{
      WasmEdge::GC::Controller::MutatorState::Running};

  std::thread Mutator([&] {
    std::vector<WasmEdge::ValVariant> Outer;
    auto OuterReg = Ctrl.registerStack(Outer);
    {
      // Host callback (NativeRunning) that re-enters the guest with a nested
      // stack: the nested registration records NativeRunning as its saved
      // state and runs the entry as Running.
      WasmEdge::GC::Controller::NativeScope Native(Ctrl);
      {
        std::vector<WasmEdge::ValVariant> Nested;
        auto NestedReg = Ctrl.registerStack(Nested);
        Registered.store(true);
        while (!CloseStarted.load()) {
          std::this_thread::yield();
        }
        // NestedReg is destroyed here, after Closing was published.
      }
      StacksAfterNested.store(Ctrl.debugActiveStackCount());
      StateAfterNested.store(Ctrl.currentMutatorState());
    }
    // OuterReg is destroyed here, which lets the drain reach zero.
  });

  while (!Registered.load()) {
    std::this_thread::yield();
  }
  std::thread Closer([&] { Ctrl.beginClosing(); });
  while (!Ctrl.isClosing()) {
    std::this_thread::yield();
  }
  CloseStarted.store(true);
  Mutator.join();
  Closer.join();

  EXPECT_EQ(StacksAfterNested.load(), 1u);
  EXPECT_EQ(StateAfterNested.load(),
            WasmEdge::GC::Controller::MutatorState::NativeRunning);
}

TEST(GC, TeardownDrainsLiveAsync) {
  // Regression: destroying a VM while a detached
  // async invocation is still running must not leave the worker touching a
  // freed Executor/Controller. Without the launch lease + drain the detached
  // thread outlives the VM and use-after-frees (caught by ASan on the Linux
  // gate); with it, ~Controller::beginClosing() blocks until the worker
  // unwinds.
  //
  // The guest runs a real, bounded back-edge loop (ConcurrentAllocWasm's
  // alloc_loop) with a large iteration count: long enough to still be executing
  // when the VM is destroyed a few microseconds later, yet bounded so the test
  // can never hang even if the drain machinery regresses -- the worker always
  // terminates on its own, and the drain simply waits for that.
  auto Conf = std::make_unique<Configure>();
  Conf->addProposal(Proposal::GC);

  // Host module (gc.coll / gc.rec) must outlive the async task. Declared before
  // the VM: the drain guarantees the worker finishes before VM.reset() returns,
  // hence before GCMod is destroyed at end of scope.
  GCRecModule GCMod;

  auto VM = std::make_unique<VM::VM>(*Conf);
  VM->registerModule(GCMod);
  ASSERT_TRUE(VM->loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  auto A = VM->asyncExecute(
      "alloc_loop",
      std::initializer_list<ValVariant>{UINT32_C(300000), UINT32_C(4)},
      {ValType(TypeCode::I32), ValType(TypeCode::I32)});

  // Wait until the worker has actually entered the guest (registered its value
  // stack with the controller) before destroying the VM: this makes the
  // teardown deterministically race a genuinely-executing async task rather
  // than guessing at timing, and closes the "worker not yet started" escape
  // where the drain would be a trivial no-op.
  auto &Ctrl = VM->getExecutor().getController();
  while (Ctrl.debugActiveStackCount() == 0) {
    std::this_thread::yield();
  }

  // Deletion-ordered handle contract: the Async handle co-owns the
  // launch lease, so destroying the VM on this thread while the handle is
  // alive would make the drain wait on our own handle (a deliberate deadlock
  // that surfaces the ordering misuse). Drop the handle first --
  // fire-and-forget. The worker lambda's co-owned holder ref keeps the lease
  // held for the whole invocation, so the drain below still waits for the
  // running worker: the regression under test is unchanged.
  A = {};

  // Destroy the VM while the async task is running; teardown must drain, not
  // UAF.
  VM.reset(); // ~VM -> beginClosing() drains the worker's co-owned lease

  SUCCEED();
}

TEST(GC, AsyncResultLeaseHeldUntilHandleRelease) {
  // Deletion-ordered handle contract: a successful async
  // result can carry a GC-managed reference retained into the handle's
  // shared_future. The launch lease is therefore co-owned by the worker
  // lambda and the Async handle, released only at the last owner's
  // destruction -- so ~VM's teardown drain cannot free the allocator while a
  // live handle still exposes references into that heap. If the lease died
  // at set_value, the reads below would be use-after-free (an ASan build is
  // the enforcing signal; this also documents that the
  // handle must be released before the VM on the same thread -- the drain
  // deliberately waits on it).
  Configure Conf = makeGCConf();
  auto VM = std::make_unique<VM::VM>(Conf);
  ASSERT_TRUE(VM->loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  auto A = VM->asyncExecute("make_arr",
                            std::initializer_list<ValVariant>{UINT32_C(4)},
                            {ValType(TypeCode::I32)});
  ASSERT_TRUE(A.valid());
  {
    auto Res = A.get(); // worker completed; result holds a retained ref
    ASSERT_TRUE(Res);
    ASSERT_EQ(Res->size(), 1u);
  }

  // Destroy the VM on another thread while this thread still holds the
  // handle. Gated on ResetEntered so the enforcing check below cannot pass
  // before the destroyer enters teardown.
  std::atomic<bool> ResetEntered{false};
  std::atomic<bool> DtorDone{false};
  std::thread Destroyer([&] {
    ResetEntered.store(true, std::memory_order_release);
    VM.reset();
    DtorDone.store(true, std::memory_order_release);
  });

  // Wait until the destroyer has entered teardown before asserting the
  // drain is still blocked.
  while (!ResetEntered.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  // Repeatedly read the managed reference the result carries. The reads
  // only compare pointer bits (Ref.isNull()) -- they never dereference the
  // GC heap -- so they merely keep the window open across many iterations.
  // The enforcing signal is the DtorDone check below.
  for (int I = 0; I < 1000; ++I) {
    auto Again = A.get();
    ASSERT_TRUE(Again);
    const auto &Ref = (*Again)[0].first.get<RefVariant>();
    EXPECT_FALSE(Ref.isNull());
  }

  // Enforcing assertion: the destroyer has entered VM.reset() (ResetEntered)
  // but the co-owned lease keeps the drain from completing while this
  // handle is alive, so the destructor cannot have finished. A lease
  // wrongly released at set_value lets VM.reset() finish and trips this.
  // (Fully hook-deterministic gating on the drain being entered is a
  // possible future hardening; the ResetEntered latch removes the
  // vacuous-pass window.)
  EXPECT_FALSE(DtorDone.load(std::memory_order_acquire));

  // Release the handle: the last lease owner drops, the drain unblocks, and
  // the destructor completes.
  A = {};
  Destroyer.join();
  EXPECT_TRUE(DtorDone.load(std::memory_order_acquire));
}

// The deletion-ordered handle contract makes ~VM wait on a live Async handle
// (see AsyncResultLeaseHeldUntilHandleRelease). On the C++ path that wait has
// no fallible entry to reject the misuse, so a handle declared before its VM
// -- legal upstream -- turns into a silent hang. The teardown must at least
// say so: beginClosing logs the outstanding handle count before it parks in
// the drain.
TEST(GC, TeardownLogsOutstandingAsyncHandles) {
  Configure Conf = makeGCConf();
  auto VM = std::make_unique<VM::VM>(Conf);
  ASSERT_TRUE(VM->loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  auto A = VM->asyncExecute("make_arr",
                            std::initializer_list<ValVariant>{UINT32_C(4)},
                            {ValType(TypeCode::I32)});
  ASSERT_TRUE(A.valid());
  ASSERT_TRUE(A.get()); // worker finished; only the handle holds the lease

  std::mutex LogMtx;
  std::string Captured;
  std::atomic<bool> Logged{false};
  Log::setLoggingCallback([&](const spdlog::details::log_msg &Msg) {
    std::lock_guard<std::mutex> L(LogMtx);
    Captured.append(Msg.payload.data(), Msg.payload.size());
    Captured.push_back('\n');
    if (Captured.find("async handle") != std::string::npos) {
      Logged.store(true, std::memory_order_release);
    }
  });

  // Destroy on another thread: the drain blocks on our live handle, and the
  // diagnostic must land before that block (we wait for it, not for a timer).
  std::thread Destroyer([&] { VM.reset(); });
  while (!Logged.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  A = {}; // release the handle: the drain unblocks
  Destroyer.join();
  Log::setLoggingCallback(nullptr);

  std::lock_guard<std::mutex> L(LogMtx);
  EXPECT_NE(Captured.find("1 async handle"), std::string::npos) << Captured;
}

TEST(GC, AsyncInvokeRejectsNonDeferrableModule) {
  // asyncInvoke must refuse a target whose defining module has
  // non-deferrable storage (embedder-constructed -- here a stack host
  // module). Its ModulePin could never defer the module's destruction; a
  // caller destroying the module mid-worker would abort (checked builds) or
  // UAF (release). A runtime-instantiated module is terminate()-managed and
  // stays accepted.
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod; // stack storage: not deferrable
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto *HostFn = GCMod.findFuncExports("coll");
  ASSERT_NE(HostFn, nullptr);
  auto A = VM.getExecutor().asyncInvoke(HostFn, {}, {});
  EXPECT_FALSE(A.valid()); // refused up front: pin could not defer

  // The runtime-instantiated active module is deferrable; same call form.
  auto *GuestFn = VM.getActiveModule()->findFuncExports("alloc_loop");
  ASSERT_NE(GuestFn, nullptr);
  std::vector<ValVariant> P{UINT32_C(1), UINT32_C(1)};
  std::vector<ValType> PT{ValType(TypeCode::I32), ValType(TypeCode::I32)};
  auto B = VM.getExecutor().asyncInvoke(GuestFn, P, PT);
  ASSERT_TRUE(B.valid());
  EXPECT_TRUE(B.get());
}

TEST(GC, AsyncInvokeRejectsModulelessTarget) {
  // asyncInvoke must refuse a target with a null defining module (a
  // standalone/independent host function). Nothing can pin such a
  // FunctionInstance, so a caller destroying it mid-worker would UAF. Reject
  // up front (invalid Async). Synchronous invoke of the same func is
  // unaffected (covered elsewhere).
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  auto &Exec = VM.getExecutor();

  // A standalone host function instance: null module (CompositeBase() ctor).
  auto HostFunc = std::make_unique<Collect>();
  Runtime::Instance::FunctionInstance StandaloneFn(std::move(HostFunc));
  ASSERT_EQ(StandaloneFn.getModule(), nullptr);

  auto A = Exec.asyncInvoke(&StandaloneFn, {}, {});
  EXPECT_FALSE(A.valid()); // refused: a null-module target cannot be pinned
}

// Regression for the teardown-vs-handshake deadlock: tearing down the GC
// controller while an in-flight collection still has a Running-but-unacked
// mutator must not hang. The coordinator spins in waitForAcks for that ack; a
// mutator that bailed its safe point on Closing never delivers it, so pre-fix
// the coordinator never returns from collect(), never releases its launch
// lease, and beginClosing()'s drain hangs.
//
// Modelled at the Controller level -- the machinery an async VM invocation uses
// (a launch lease + a registered value stack) -- driven directly so the
// interleaving is fixed rather than raced:
//
//   * Mutator M: leased and registered (Running), never acknowledges. When
//     released it drops its Registration (which lingers during Closing) and
//     its lease.
//   * Coordinator C: leased and registered, then calls the real collect(),
//     whose STW #1 waitForAcks blocks on M's missing ack.
//   * Teardown T: calls beginClosing(), which publishes Closing and drains
//     until every lease is released.
//
// A VM-level asyncExecute reproduction is unreliable: after bailing its safe
// point the mutator re-enters the host call and flips NativeRunning, a window
// in which waitForAcks skips it and the coordinator escapes even without the
// fix. Post-fix, waitForAcks abandons on Closing, C finishes collect() and
// releases its lease, and beginClosing drains cleanly.
TEST(GCThread, TeardownRacingInFlightHandshakeDoesNotDeadlock) {
  auto Ctrl = std::make_unique<GC::Controller>();

  std::atomic<bool> MRegistered{false};
  std::atomic<bool> CRegistered{false};
  std::atomic<bool> ReleaseM{false};

  // Mutator M: a leased, registered Running mutator that never acknowledges the
  // handshake -- exactly the thread that bailed its safe point on Closing. Held
  // Running-unacked until ReleaseM, then it drops its Registration and lease.
  std::thread MThread([&]() {
    auto Lease = Ctrl->acquireLease();
    std::vector<ValVariant> Stack;
    auto Reg = Ctrl->registerStack(Stack);
    MRegistered.store(true, std::memory_order_release);
    while (!ReleaseM.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    // Reg's destructor runs Registration::reset(): under Closing it leaves the
    // entry lingering Running-unacked (the precondition of the deadlock), then
    // Lease's destructor releases the launch lease.
  });

  // Coordinator C: a leased, registered mutator that drives a real collection.
  // Its STW #1 waitForAcks blocks on M's missing ack.
  std::thread CThread([&]() {
    auto Lease = Ctrl->acquireLease();
    std::vector<ValVariant> Stack;
    auto Reg = Ctrl->registerStack(Stack);
    CRegistered.store(true, std::memory_order_release);
    while (!MRegistered.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    Ctrl->collect(/*Manual=*/true, /*ScanNative=*/false);
    // Lease/Reg released here once collect() returns (only after the fix lets
    // waitForAcks abandon on Closing).
  });

  // Wait until both mutators are registered and C has raised the stop flag,
  // i.e. C is spinning in waitForAcks on M -- the stable in-flight-handshake
  // state.
  while (!CRegistered.load(std::memory_order_acquire) ||
         !MRegistered.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  {
    const auto Deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!Ctrl->stopRequested()) {
      if (std::chrono::steady_clock::now() > Deadline) {
        ADD_FAILURE() << "coordinator never entered the handshake";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::yield();
    }
  }

  // Teardown on its own thread: beginClosing() publishes Closing and then
  // drains until every lease is released -- exactly what ~Controller does at
  // ~VM.
  std::atomic<bool> ClosingDone{false};
  std::thread Teardown([&]() {
    Ctrl->beginClosing();
    ClosingDone.store(true, std::memory_order_release);
  });

  // Wait for Closing to be published (safe: the drain cannot progress while M
  // and C still hold their leases).
  {
    const auto Deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!Ctrl->isClosing()) {
      if (std::chrono::steady_clock::now() > Deadline) {
        ADD_FAILURE() << "beginClosing did not publish Closing within 10s";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::yield();
    }
  }

  // Release M: it drops its (lingering) Registration and its lease. Now only
  // the coordinator's lease is outstanding -- so whether the drain completes
  // turns entirely on C escaping waitForAcks, which is precisely the fix under
  // test.
  ReleaseM.store(true, std::memory_order_release);

  // Hard watchdog: a deadlock becomes a failure + hard-exit, never a hang. Do
  // not join the wedged threads or touch the half-torn Controller on timeout;
  // _Exit skips destructors (including the blocking joins below).
  {
    const auto Deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!ClosingDone.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > Deadline) {
        ADD_FAILURE() << "teardown deadlocked: waitForAcks did not abandon on "
                         "Closing (coordinator lease never released)";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  Teardown.join();
  CThread.join();
  MThread.join();
  Ctrl.reset(); // ~Controller: idempotent beginClosing, then joins GC workers
  SUCCEED();
}

TEST(GCThread, DrainDecrementUnderDrainMtx) {
  // The drain-satisfying decrement + notify must happen while
  // holding DrainMtx, and must be complete before the releaser drops the lock.
  // If they are, the teardown drain in beginClosing() provably cannot return
  // while the final releaser is still inside its DrainMtx critical section --
  // so ~Controller cannot free DrainMtx/DrainCV under the releaser (the UAF).
  // The enforcing signal for the raw UAF is a TSan/ASan build; this
  // test locks in the "notify + return happen under DrainMtx" invariant.
  auto Ctrl = std::make_unique<GC::Controller>();
  auto Lease = Ctrl->acquireLease();
  ASSERT_TRUE(Lease.valid());

  std::atomic<bool> HookEntered{false};
  std::atomic<bool> HookRelease{false};
  Ctrl->setDrainReleaseHook([&]() noexcept {
    HookEntered.store(true, std::memory_order_release);
    while (!HookRelease.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  std::atomic<bool> ClosingDone{false};
  std::thread Teardown([&]() {
    Ctrl->beginClosing(); // drain waits on the one outstanding lease
    ClosingDone.store(true, std::memory_order_release);
  });

  // Wait until Closing is published; the drain is now parked on the lease.
  {
    const auto Deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!Ctrl->isClosing()) {
      if (std::chrono::steady_clock::now() > Deadline) {
        ADD_FAILURE() << "beginClosing did not publish Closing";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::yield();
    }
  }

  // Release the final lease on another thread. Its decrement-to-zero fires the
  // hook while holding DrainMtx (after notify_all), and blocks there.
  std::thread Releaser([&]() { auto Local = std::move(Lease); });

  // Wait for the releaser to be paused mid-release (holding DrainMtx).
  {
    const auto Deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!HookEntered.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > Deadline) {
        ADD_FAILURE() << "final lease release did not reach the drain hook";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::yield();
    }
  }

  // The releaser holds DrainMtx; the drain cannot reacquire it to return.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(ClosingDone.load(std::memory_order_acquire));

  // Let the releaser drop DrainMtx; the drain now wakes, sees count 0, returns.
  HookRelease.store(true, std::memory_order_release);
  Releaser.join();
  Teardown.join();
  EXPECT_TRUE(ClosingDone.load(std::memory_order_acquire));
  Ctrl.reset();
}

TEST(GC, ClosingRefusesNewSynchronousExecute) {
  // Registration TOCTOU: once the controller begins closing,
  // a new synchronous public call must be refused with a clean error at the
  // boundary -- never allowed to register a fresh stack behind a teardown
  // drain that may already have observed RegisteredStacks == 0 and returned.
  // registerStack itself has no error channel (StackManager's ctor is
  // noexcept), so the refusal lives at lease acquisition, which is serialized
  // with the drain under DrainMtx.
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto *GuestFn = VM.getActiveModule()->findFuncExports("alloc_loop");
  ASSERT_NE(GuestFn, nullptr);

  // Nothing is outstanding, so beginClosing() drains immediately and returns;
  // the controller is now Closing while the VM object is still alive --
  // exactly the state a racing teardown publishes before its drain.
  VM.getExecutor().getController().beginClosing();

  // VM-level public boundary.
  auto Res = VM.execute(
      "alloc_loop", std::initializer_list<ValVariant>{UINT32_C(1), UINT32_C(1)},
      {ValType(TypeCode::I32), ValType(TypeCode::I32)});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::Interrupted);

  // Executor-level public boundary (direct embedder API).
  std::vector<ValVariant> P{UINT32_C(1), UINT32_C(1)};
  std::vector<ValType> PT{ValType(TypeCode::I32), ValType(TypeCode::I32)};
  auto Res2 = VM.getExecutor().invoke(GuestFn, P, PT);
  ASSERT_FALSE(Res2);
  EXPECT_EQ(Res2.error(), ErrCode::Value::Interrupted);
}

} // namespace
