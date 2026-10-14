// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCConcurrencyTest.cpp - GC concurrency tests -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for concurrent mutators: shared
/// references, table and stack growth against a collection, exclusive
/// operations, and coherent reference slots.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace WasmEdge {
namespace Executor {
// Test-only bridge to the private Executor::runTableGrowOp, befriended in
// executor.h. Lets GCThread.InterpreterGrowInitializerSurvivesCollect drive the
// interpreter table.grow handler directly so the scoped-root pin it adds around
// the grow window is exercised (and provably load-bearing) without a full guest
// module.
Expect<void>
gcTestRunTableGrowOp(Executor &Exe, Runtime::StackManager &StackMgr,
                     Runtime::Instance::TableInstance &TabInst) noexcept {
  return Exe.runTableGrowOp(StackMgr, TabInst);
}
} // namespace Executor
} // namespace WasmEdge

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

// Thread-safe check for concurrent tests
class ThreadSafeCheck : public Runtime::HostFunction<ThreadSafeCheck> {
public:
  explicit ThreadSafeCheck(uint32_t Expected) : ExpectedValue(Expected) {}
  Expect<void> body(const Runtime::CallingFrame &, uint32_t Value) {
    if (Value != ExpectedValue) {
      FailCount.fetch_add(1, std::memory_order_relaxed);
    }
    CallCount.fetch_add(1, std::memory_order_relaxed);
    return {};
  }
  uint64_t getFailCount() const noexcept {
    return FailCount.load(std::memory_order_relaxed);
  }
  uint64_t getCallCount() const noexcept {
    return CallCount.load(std::memory_order_relaxed);
  }

private:
  uint32_t ExpectedValue;
  std::atomic<uint64_t> FailCount{0};
  std::atomic<uint64_t> CallCount{0};
};

// Test 7: GC during concurrent execution - alloc struct, GC in loop, verify
const std::array<WasmEdge::Byte, 203> GCDuringConcurrentWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5f,
    0x01, 0x7f, 0x01, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x16,
    0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x02, 0x03, 0x02,
    0x01, 0x02, 0x07, 0x14, 0x01, 0x10, 0x61, 0x6c, 0x6c, 0x6f, 0x63, 0x5f,
    0x61, 0x6e, 0x64, 0x5f, 0x76, 0x65, 0x72, 0x69, 0x66, 0x79, 0x00, 0x02,
    0x0a, 0x35, 0x01, 0x33, 0x02, 0x01, 0x64, 0x00, 0x01, 0x7f, 0x20, 0x00,
    0xfb, 0x00, 0x00, 0x21, 0x01, 0x41, 0x00, 0x21, 0x02, 0x02, 0x40, 0x03,
    0x40, 0x20, 0x02, 0x41, 0xe4, 0x00, 0x4f, 0x0d, 0x01, 0x10, 0x00, 0x20,
    0x01, 0xfb, 0x02, 0x00, 0x00, 0x10, 0x01, 0x20, 0x02, 0x41, 0x01, 0x6a,
    0x21, 0x02, 0x0c, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x4a, 0x04, 0x6e, 0x61,
    0x6d, 0x65, 0x01, 0x0e, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x01,
    0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x02, 0x0f, 0x01, 0x02, 0x03, 0x00,
    0x02, 0x69, 0x64, 0x01, 0x03, 0x72, 0x65, 0x66, 0x02, 0x01, 0x69, 0x03,
    0x10, 0x01, 0x02, 0x02, 0x00, 0x05, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x01,
    0x04, 0x6c, 0x6f, 0x6f, 0x70, 0x04, 0x10, 0x03, 0x00, 0x01, 0x73, 0x01,
    0x02, 0x66, 0x6e, 0x02, 0x06, 0x66, 0x6e, 0x5f, 0x69, 0x33, 0x32};

// Test 8: Shared references via global - multiple mutators sharing one VM /
// module / allocator write and verify a shared GC array through a module
// global.
//
//   (type $arr (array (mut i32)))                      ;; type 0
//   (import "gc" "coll" (func $coll))                  ;; func 0
//   (import "gc" "check" (func $check (param i32)))    ;; func 1, unused
//   (global $shared (mut (ref null $arr)) (ref.null $arr))
//   (func (export "init")                              ;; func 2
//     (global.set $shared (array.new $arr (i32.const 0) (i32.const 4))))
//   (func (export "write_and_verify") (param $idx i32) (param $val i32)
//     (local $i i32)                                   ;; func 3
//     (loop (bounded 50 iterations)
//       ;; array[idx] = val
//       (array.set $arr (ref.as_non_null (global.get $shared))
//                  (local.get $idx) (local.get $val))
//       (call $coll)                     ;; force a GC between write and read
//       ;; if array[idx] != val -> trap (real, self-verifying assertion; a
//       ;; torn/stale read or a collected-out-from-under array traps and the
//       ;; async result becomes an error). No host check -> no shared,
//       ;; non-thread-safe Check::body race under the one shared module.
//       (if (i32.ne (array.get $arr (ref.as_non_null (global.get $shared))
//                               (local.get $idx))
//                   (local.get $val))
//         (then unreachable))))
const std::array<WasmEdge::Byte, 256> SharedRefsWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x04, 0x5e,
    0x7f, 0x01, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x02, 0x7f,
    0x7f, 0x00, 0x02, 0x16, 0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c,
    0x6c, 0x00, 0x01, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b,
    0x00, 0x02, 0x03, 0x03, 0x02, 0x01, 0x03, 0x06, 0x07, 0x01, 0x63, 0x00,
    0x01, 0xd0, 0x00, 0x0b, 0x07, 0x1b, 0x02, 0x04, 0x69, 0x6e, 0x69, 0x74,
    0x00, 0x02, 0x10, 0x77, 0x72, 0x69, 0x74, 0x65, 0x5f, 0x61, 0x6e, 0x64,
    0x5f, 0x76, 0x65, 0x72, 0x69, 0x66, 0x79, 0x00, 0x03, 0x0a, 0x47, 0x02,
    0x0b, 0x00, 0x41, 0x00, 0x41, 0x04, 0xfb, 0x06, 0x00, 0x24, 0x00, 0x0b,
    0x39, 0x01, 0x01, 0x7f, 0x41, 0x00, 0x21, 0x02, 0x02, 0x40, 0x03, 0x40,
    0x20, 0x02, 0x41, 0x32, 0x4f, 0x0d, 0x01, 0x23, 0x00, 0xd4, 0x20, 0x00,
    0x20, 0x01, 0xfb, 0x0e, 0x00, 0x10, 0x00, 0x23, 0x00, 0xd4, 0x20, 0x00,
    0xfb, 0x0b, 0x00, 0x20, 0x01, 0x47, 0x04, 0x40, 0x00, 0x0b, 0x20, 0x02,
    0x41, 0x01, 0x6a, 0x21, 0x02, 0x0c, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x58,
    0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0e, 0x02, 0x00, 0x04, 0x63, 0x6f,
    0x6c, 0x6c, 0x01, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x02, 0x10, 0x01,
    0x03, 0x03, 0x00, 0x03, 0x69, 0x64, 0x78, 0x01, 0x03, 0x76, 0x61, 0x6c,
    0x02, 0x01, 0x69, 0x03, 0x10, 0x01, 0x03, 0x02, 0x00, 0x05, 0x62, 0x72,
    0x65, 0x61, 0x6b, 0x01, 0x04, 0x6c, 0x6f, 0x6f, 0x70, 0x04, 0x12, 0x03,
    0x00, 0x03, 0x61, 0x72, 0x72, 0x01, 0x02, 0x66, 0x6e, 0x02, 0x06, 0x66,
    0x6e, 0x5f, 0x69, 0x33, 0x32, 0x07, 0x09, 0x01, 0x00, 0x06, 0x73, 0x68,
    0x61, 0x72, 0x65, 0x64};

TEST(GCThread, ConcurrentAllocation) {
  // Multiple threads allocating GC objects simultaneously
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // 4 async tasks, each allocating 200 arrays of size 64.
  constexpr uint32_t NumThreads = 4;
  std::array<Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>,
             NumThreads>
      AsyncResults;

  for (uint32_t I = 0; I < NumThreads; ++I) {
    AsyncResults[I] = VM.asyncExecute(
        "alloc_loop",
        std::initializer_list<ValVariant>{UINT32_C(200), UINT32_C(64)},
        {ValType(TypeCode::I32), ValType(TypeCode::I32)});
  }

  for (uint32_t I = 0; I < NumThreads; ++I) {
    auto Result = AsyncResults[I].get();
    EXPECT_TRUE(Result) << "Thread " << I << " failed";
  }
}

TEST(GCThread, GCDuringConcurrentExecution) {
  Configure Conf = makeGCConf();

  constexpr uint32_t NumThreads = 4;

  // Each thread (own VM, own check module) allocates a struct holding its ID,
  // then loops 100x running GC and asserting the field still equals the ID.
  std::vector<std::thread> Threads;
  std::atomic<uint32_t> FailCount{0};

  for (uint32_t I = 0; I < NumThreads; ++I) {
    Threads.emplace_back([&, I]() {
      // Host modules must outlive the VM that terminates them; declare first.
      auto Mod = std::make_unique<Runtime::Instance::ModuleInstance>("gc");
      Mod->addHostFunc("coll", std::make_unique<Collect>());
      auto CP = std::make_unique<ThreadSafeCheck>(I);
      auto *C = CP.get();
      Mod->addHostFunc("check", std::move(CP));

      Configure TConf = makeGCConf();
      VM::VM TVM(TConf);
      TVM.registerModule(*Mod);

      ASSERT_TRUE(TVM.loadWasm(GCDuringConcurrentWasm));
      ASSERT_TRUE(TVM.validate());
      ASSERT_TRUE(TVM.instantiate());
      auto Result = TVM.execute(
          "alloc_and_verify",
          std::initializer_list<ValVariant>{static_cast<uint32_t>(I)},
          {ValType(TypeCode::I32)});
      if (!Result) {
        FailCount.fetch_add(1, std::memory_order_relaxed);
      }
      EXPECT_EQ(C->getFailCount(), 0)
          << "Thread " << I << " had " << C->getFailCount()
          << " data integrity failures out of " << C->getCallCount()
          << " checks";
    });
  }

  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_EQ(FailCount.load(), 0);
}

// Multiple mutators sharing one VM / module /
// allocator. Each async task runs on its own operand stack (Executor::invoke
// builds a per-invocation StackManager registered independently with the
// Controller), but all four share the single module instance's global array
// and the one allocator. init() creates the shared 4-element array once; then
// four concurrent write_and_verify calls each write their own index, attempt a
// collection between write and read (the host coll racing the other mutators
// for the Idle->MarkingRoot CAS, so a given call may lose and collect nothing),
// and trap on any mismatch. Across the four racing threads collections do run;
// a torn global ref, a UAF on the shared array, or the array being collected
// out from under a mutator makes the guest trap and the async result an error
// -> EXPECT_TRUE fails. This test races collection against shared-reference
// access; it does not assert a specific write/collect/read interleaving (a
// phase-gated determinism test is a follow-up). All mutators share one VM,
// module instance and heap.
TEST(GCThread, SharedReferencesAcrossThreads) {
  Configure Conf = makeGCConf();
  // Host module must outlive the VM that terminates it; declare first.
  GCFullModule GCMod;
  VM::VM VM(Conf); // One VM shared by all mutators
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(SharedRefsWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  ASSERT_TRUE(VM.execute("init")); // create the shared global array once

  constexpr uint32_t NumThreads = 4;
  std::array<Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>,
             NumThreads>
      Results;
  for (uint32_t I = 0; I < NumThreads; ++I) {
    Results[I] = VM.asyncExecute(
        "write_and_verify", std::initializer_list<ValVariant>{I, (I + 1) * 10},
        {ValType(TypeCode::I32), ValType(TypeCode::I32)});
  }
  for (uint32_t I = 0; I < NumThreads; ++I) {
    auto R = Results[I].get();
    EXPECT_TRUE(R) << "Thread " << I << " failed";
  }
}

// Multi-mutator allocation / GC stress on one
// allocator. Eight concurrent mutators on one VM each run a bounded loop
// allocating 500 GC arrays of size 64 (alloc_loop is deterministic and cannot
// hang), then call gc.coll once at the end. Together with the allocator's own
// auto-collection and the concurrent registered stacks this exercises the
// allocate/mark/sweep paths across mutators; the run must be UAF- and race-free
// under ASan/TSan. NOTE: phase-gated determinism for allocate-during-sweep
// lives in GC.AllocateDuringSweepSurvives; this stress adds the
// multi-mutator scheduling dimension on top.
TEST(GCThread, MultiMutatorAllocationStress) {
  Configure Conf = makeGCConf();
  // Host module must outlive the VM that terminates it; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  constexpr uint32_t NumThreads = 8;
  std::array<Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>,
             NumThreads>
      Results;
  for (uint32_t I = 0; I < NumThreads; ++I) {
    Results[I] = VM.asyncExecute(
        "alloc_loop",
        std::initializer_list<ValVariant>{UINT32_C(500), UINT32_C(64)},
        {ValType(TypeCode::I32), ValType(TypeCode::I32)});
  }
  for (auto &R : Results) {
    EXPECT_TRUE(R.get());
  }
}

TEST(GCThread, GrowTableDuringCollect) {
  // Regression for the table-grow vs root-scan race: growTable reallocates the
  // Refs vector the collector scans as roots. A grower thread races the main
  // thread's collections; without serialization the scan reads a freed buffer.
  // Null funcrefs only, so this isolates the realloc race (TSan-clean = pass).
  GC::Allocator Alloc;
  AST::TableType TType(ValType(TypeCode::FuncRef), 0, 100000);
  // funcref: not GC-managed.
  Runtime::Instance::TableInstance Table(TType, false);
  Table.setAllocator(Alloc);

  std::atomic<bool> Stop{false};
  std::atomic<uint32_t> Grown{0};
  std::atomic<bool> GrowerDone{false};
  std::thread Grower([&]() {
    for (uint32_t I = 0; I < 1000; ++I) {
      if (!Table.growTable(1)) {
        break;
      }
      Grown.fetch_add(1, std::memory_order_relaxed);
      if (Stop.load(std::memory_order_relaxed)) {
        break;
      }
    }
    GrowerDone.store(true, std::memory_order_release);
  });

  for (uint32_t I = 0; I < 1000; ++I) {
    Alloc.manualCollect();
  }
  // Checking Stop only after a grow keeps the race under test alive even when
  // the grower is scheduled late: on a fast host the collect loop can finish
  // before the thread runs at all, which used to end with an ungrown table.
  while (Grown.load(std::memory_order_relaxed) == 0 &&
         !GrowerDone.load(std::memory_order_acquire)) {
    Alloc.manualCollect();
  }
  Stop.store(true, std::memory_order_relaxed);
  Grower.join();

  EXPECT_GE(Table.getSize(), 1u);
}

TEST(GCThread, GrowStackDuringCollect) {
  // Regression for the value-stack realloc vs root-scan race: pushing past the
  // reserved capacity reallocates the GC-registered ValueStack the collector
  // scans. A pusher thread races the main thread's collections; without the
  // grow-only lock the scan reads a freed buffer (TSan-clean = pass).
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  std::atomic<bool> Stop{false};
  std::thread Pusher([&]() {
    Runtime::StackManager StackMgr(Ctrl);
    // Cross the 2048-entry reserve so push_back reallocates the registered
    // buffer while the collector may be iterating it.
    for (uint32_t I = 0; I < 2500u && !Stop.load(std::memory_order_relaxed);
         ++I) {
      StackMgr.push(ValVariant(UINT32_C(0)));
    }
  });

  for (uint32_t I = 0; I < 200u; ++I) {
    Alloc.manualCollect();
  }
  Stop.store(true, std::memory_order_relaxed);
  Pusher.join();
  SUCCEED();
}

TEST(GCThread, StaleTerminationWinRejected) {
  // Regression for the stale termination-owner win. A worker that decides to
  // terminate (empty Gray under MarkingGray) but is preempted before the
  // ownership CAS can win late -- after the in-cycle owner already swept and
  // left MarkingGray. Without the phase re-check it would fire STW #2 for a
  // finished cycle, ++CurrentGeneration and raise StopFlag while the next
  // coordinator's STW #1 is open, tripping endHandshake's generation assert
  // (the GrowStackDuringCollect flake). The guard must instead release
  // ownership and re-park without driving STW #2.
  if (std::thread::hardware_concurrency() < 2) {
    GTEST_SKIP() << "needs >=2 collector workers to stage the stale-CAS win";
  }
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  Alloc.setManualGC(true);

  // Pin exactly one would-be owner at the pre-CAS preemption window; let every
  // other worker through so one of them actually terminates the cycle.
  std::atomic<bool> Pinned{false};
  std::atomic<bool> Release{false};
  Alloc.setPreTerminationCASHook([&]() {
    bool Expected = false;
    if (Pinned.compare_exchange_strong(Expected, true)) {
      while (!Release.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
    }
  });

  // Cycle 1: a non-pinned worker terminates and sweeps it to Idle while the
  // pinned worker still holds its (about-to-be-stale) Terminate decision. A
  // worker sweeps only after passing the hook, which requires Pinned already
  // set, so the pin is observable once the cycle returns.
  EXPECT_TRUE(Alloc.manualCollect());
  ASSERT_TRUE(Pinned.load(std::memory_order_acquire));

  // Baseline after the cycle: on a many-core box the same two-owners-in-turn
  // race can fire the guard organically during the cycle, so the count is not
  // deterministically zero -- only the released worker's rejection below is.
  // No collection runs between here and the release, so nothing else advances
  // the count until our staged stale winner does.
  const uint64_t Before = Alloc.debugStaleTerminationRejects();

  // Release the pinned worker: it now wins ownership in the Idle phase, and the
  // guard must reject it -- advancing the count -- rather than drive a stray
  // STW #2. The hook is left installed but inert (Pinned is set, so its CAS
  // fails and it returns immediately); rewriting it here would race a worker
  // still reading it.
  Release.store(true, std::memory_order_release);
  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (Alloc.debugStaleTerminationRejects() == Before &&
         std::chrono::steady_clock::now() < Deadline) {
    std::this_thread::yield();
  }
  EXPECT_GT(Alloc.debugStaleTerminationRejects(), Before);

  // The rejected worker must have cleanly re-parked: a fresh cycle still
  // completes, proving the allocator is healthy after the stale win.
  EXPECT_TRUE(Alloc.manualCollect());
}

TEST(GCThread, ExclusiveOpSingleOwnerUnderContention) {
  // The per-controller exclusive-operation owner must admit at most one owner
  // at a time under contention. N registered mutators each loop acquiring the
  // token (OwnedGrowing), briefly hold it while asserting no other thread holds
  // it, then release it through the reserved FIFO handoff. A shared high-water
  // mark proves the single-owner invariant (max concurrent owners == 1); a hard
  // watchdog turns a deadlock (a bug wedging the handoff) into a failure +
  // hard-exit rather than an indefinite hang.
  GC::Controller Ctrl;

  constexpr uint32_t NumThreads = 8;
  constexpr uint32_t Iterations = 2000;

  std::atomic<int> Inside{0};    // owners currently in the critical section
  std::atomic<int> MaxInside{0}; // high-water mark of Inside (must stay 1)
  std::atomic<uint32_t> Done{0}; // workers that finished their loop
  std::atomic<bool> Failed{false};

  auto Worker = [&]() {
    // Register a value stack so the loser protocol's Blocked publish +
    // admission exercise the real RegistryMtx path (setSelfBlocked /
    // admitToState).
    Runtime::StackManager StackMgr(Ctrl);
    for (uint32_t I = 0; I < Iterations; ++I) {
      uint64_t Gen = 0;
      if (!Ctrl.beginExclusiveOp(GC::Controller::ExclusiveState::OwnedGrowing,
                                 Gen)) {
        // beginExclusiveOp only returns false while Closing, which never
        // happens in this test -- treat it as a failure.
        Failed.store(true, std::memory_order_relaxed);
        break;
      }
      const int Now = Inside.fetch_add(1, std::memory_order_acq_rel) + 1;
      if (Now > 1) {
        Failed.store(true, std::memory_order_relaxed);
      }
      int Prev = MaxInside.load(std::memory_order_relaxed);
      while (Now > Prev && !MaxInside.compare_exchange_weak(
                               Prev, Now, std::memory_order_relaxed)) {
      }
      // Widen the exclusion-violation detection window: if the single-owner
      // invariant ever broke, this yield makes the overlap far likelier to be
      // observed by a racing thread's Now > 1 check above.
      std::this_thread::yield();
      Inside.fetch_sub(1, std::memory_order_acq_rel);
      Ctrl.endExclusiveOp(Gen, GC::Controller::ExclusiveState::OwnedGrowing);
    }
    Done.fetch_add(1, std::memory_order_acq_rel);
  };

  std::vector<std::thread> Threads;
  for (uint32_t I = 0; I < NumThreads; ++I) {
    Threads.emplace_back(Worker);
  }

  // Hard watchdog: a deadlock becomes a failure + hard-exit, never a hang. Do
  // not join the wedged threads on timeout -- _Exit skips the blocking joins.
  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while (Done.load(std::memory_order_acquire) < NumThreads) {
    if (std::chrono::steady_clock::now() > Deadline) {
      ADD_FAILURE() << "exclusive-op owner deadlocked: "
                    << Done.load(std::memory_order_acquire) << "/" << NumThreads
                    << " workers finished";
      std::fflush(stderr);
      std::_Exit(2);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_FALSE(Failed.load(std::memory_order_relaxed));
  EXPECT_EQ(MaxInside.load(std::memory_order_relaxed), 1);
}

TEST(GCThread, GrowBlocksDuringCollectAndViceVersa) {
  // Controller::collect() owns the exclusive-operation token for the
  // whole cycle, so a stop-the-world grow and a collection can never both stop
  // the world at once. Drive a real collection that holds the token (a worker
  // pinned inside its sweep hook -- token still held, Idle not yet published,
  // endExclusiveOp not yet called), then have a separate "grower" thread call
  // beginExclusiveOp(OwnedGrowing) directly (growTable is not wired to the
  // arbiter here). The grower must park -- not acquire -- until the
  // collection releases the token, then acquire through the reserved FIFO
  // handoff. Proven deterministically with the queued-waiter counter, never
  // sleeps; a background watchdog turns a deadlock into a failure + hard-exit
  // (never an indefinite hang), and _Exit skips the wedged joins below.
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  Alloc.setManualGC(true); // no auto cycle may race our single collect()

  // Pin the sweep-completing worker inside runSweepAndSwap: at this point the
  // collection still owns the token (endExclusiveOp runs only after the sweep
  // returns), Idle is not yet published, and the cycle cannot complete -- so
  // the collection holds the token for as long as we hold the hook.
  std::atomic<bool> AtSweep{false};
  std::atomic<bool> ReleaseSweep{false};
  std::atomic<bool> HookFired{false};
  Alloc.setSweepPauseHook([&]() noexcept {
    if (HookFired.exchange(true)) {
      return; // gate only the first sweep
    }
    AtSweep.store(true, std::memory_order_release);
    while (!ReleaseSweep.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  // Background watchdog: a deadlock (grower never released, or sweep never
  // reached) becomes a failure + hard-exit rather than an indefinite hang.
  std::atomic<bool> TestDone{false};
  std::thread Watchdog([&]() {
    const auto WD = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!TestDone.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > WD) {
        ADD_FAILURE() << "grow<->collect arbitration deadlocked";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  // Collector thread: one real collection. It blocks in collect() until the
  // cycle completes, which cannot happen while the sweep hook is pinned -- so
  // the collection holds the exclusive token the entire time below.
  std::thread Collector([&]() { EXPECT_TRUE(Alloc.manualCollect()); });

  // Wait until the collection reaches the pinned sweep: the token is now held
  // (OwnedCollecting) and stays held until we set ReleaseSweep.
  while (!AtSweep.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  // Grower thread: attempt to acquire the token for a (simulated) grow. It
  // loses (token is OwnedCollecting), publishes Blocked, enqueues a FIFO
  // ticket, and parks on TokenCV -- it must not proceed until the collection
  // releases.
  std::atomic<bool> GrowerAcquired{false};
  std::thread Grower([&]() {
    // Register a stack so the loser path exercises the real Blocked publish +
    // admission (setSelfBlocked / admitToState), matching a real grower.
    Runtime::StackManager StackMgr(Ctrl);
    uint64_t Gen = 0;
    ASSERT_TRUE(Ctrl.beginExclusiveOp(
        GC::Controller::ExclusiveState::OwnedGrowing, Gen));
    GrowerAcquired.store(true, std::memory_order_release);
    Ctrl.endExclusiveOp(Gen, GC::Controller::ExclusiveState::OwnedGrowing);
  });

  // Deterministically wait until the grower is queued (parked on TokenCV): once
  // its ticket is enqueued it is a committed loser that cannot proceed until
  // the reserved handoff grants it the token.
  while (Ctrl.debugExclusiveWaiters() == 0) {
    std::this_thread::yield();
  }

  // The collection still holds the token, so the grower must not have acquired.
  EXPECT_FALSE(GrowerAcquired.load(std::memory_order_acquire));
  EXPECT_EQ(Ctrl.debugExclusiveWaiters(), 1u);

  // Release the pinned sweep: the sweep-completing worker finishes the cycle
  // and calls endExclusiveOp(OwnedCollecting), handing the token to the queued
  // grower. Only now may the grower acquire.
  ReleaseSweep.store(true, std::memory_order_release);

  Collector.join();
  Grower.join();

  // The grower acquired -- but only after the collection released the token.
  EXPECT_TRUE(GrowerAcquired.load(std::memory_order_acquire));

  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
  Alloc.setSweepPauseHook(nullptr);
}

TEST(GC, CoherentSlotPreservesExternalizedRefUnderTrace) {
  // A managed slot holds a 128-bit RefVariant: Raw[0] is the type tag (with the
  // Externalize bit), Raw[1] is the object pointer. While the marker reads such
  // a slot, a mutator may struct.set/array.set the same slot. Two independent
  // word accesses could fabricate a torn (type, pointer) pair -- e.g. the
  // externalized type of one value glued to the pointer of the other -- which
  // the runtime would then dereference. The coherent accessors must make every
  // full-reference read return a whole prior/next value, never a mixed pair.
  //
  // On Windows (no TSan) this verifies the coherence logic: a reader running
  // the marker path plus loadCoherent must never observe a pair that is neither
  // of the two whole values the writer alternates. (The data-race cleanliness
  // is the separate Linux TSan gate.)
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;

  auto MakeGCStruct = [&](uint32_t V) noexcept -> RawData * {
    return static_cast<RawData *>(Alloc.allocate(
        [&](void *Pointer) noexcept {
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 1;
          new (&Raw->data()[0]) ValVariant(V);
        },
        static_cast<uint32_t>(sizeof(RawData) + sizeof(ValVariant))));
  };
  RawData *ObjA = MakeGCStruct(1);
  RawData *ObjB = MakeGCStruct(2);
  ASSERT_NE(ObjA, nullptr);
  ASSERT_NE(ObjB, nullptr);

  // ValA: internalized concrete struct ref (Externalize bit clear), ptr = ObjA.
  RefVariant RefA(ValType(TypeCode::Ref, TypeCode::StructRef), ObjA);
  // ValB: externalized ref (Externalize bit set), ptr = ObjB.
  // extern.convert_any folds a gc ref to externref with the Externalize bit
  // set; emulate that here.
  RefVariant RefB(ValType(TypeCode::ExternRef), ObjB);
  RefB.getType().setExternalized();
  const ValVariant ValA(RefA);
  const ValVariant ValB(RefB);

  auto RawOf = [](const ValVariant &V) noexcept {
    std::array<uint64_t, 2> R;
    std::memcpy(R.data(), &V, sizeof(R));
    return R;
  };
  const auto RawA = RawOf(ValA);
  const auto RawB = RawOf(ValB);
  // Torn-pair detectability requires both words to differ: otherwise a torn
  // (typeA,ptrB)/(typeB,ptrA) could coincide with a whole value and hide.
  ASSERT_NE(RawA[0], RawB[0]) << "type words must differ";
  ASSERT_NE(RawA[1], RawB[1]) << "pointer words must differ";
  ASSERT_TRUE(RefA.getType().isExternalized() == false);
  ASSERT_TRUE(RefB.getType().isExternalized() == true);

  alignas(16) ValVariant Slot(ValA);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(&Slot) % 16, 0u);

  std::atomic<bool> Stop{false};
  std::atomic<uint64_t> TornCount{0};
  std::atomic<uint64_t> BadPtrCount{0};
  std::atomic<uint64_t> ReadCount{0};

  std::thread Reader([&] {
    while (!Stop.load(std::memory_order_relaxed)) {
      // Marker path: single relaxed atomic load of the pointer word. It must
      // always be one of the two object pointers (never a shredded word).
      uint8_t *P = GC::loadPointerWordRelaxed(Slot);
      if (P != reinterpret_cast<uint8_t *>(ObjA) &&
          P != reinterpret_cast<uint8_t *>(ObjB)) {
        BadPtrCount.fetch_add(1, std::memory_order_relaxed);
      }
      // Full-reference coherent read: must equal a whole prior/next value.
      ValVariant Got = GC::loadCoherent(Slot);
      const auto R = RawOf(Got);
      const bool IsA = (R == RawA);
      const bool IsB = (R == RawB);
      if (!IsA && !IsB) {
        TornCount.fetch_add(1, std::memory_order_relaxed);
      }
      ReadCount.fetch_add(1, std::memory_order_relaxed);
    }
  });

  std::thread Writer([&] {
    // Hold the writes until the reader has demonstrably started (>= 1 read):
    // otherwise all writes can complete before the reader is ever scheduled
    // and the reader-ran assertion below turns flaky.
    while (ReadCount.load(std::memory_order_relaxed) == 0) {
      std::this_thread::yield();
    }
    for (uint64_t I = 0; I < 3000000U; ++I) {
      GC::storeCoherent(Slot, (I & 1U) ? ValB : ValA);
    }
    Stop.store(true, std::memory_order_relaxed);
  });

  Writer.join();
  Stop.store(true, std::memory_order_relaxed);
  Reader.join();

  EXPECT_GT(ReadCount.load(), 0u) << "reader never ran";
  EXPECT_EQ(TornCount.load(), 0u)
      << "loadCoherent observed a torn (type,pointer) pair";
  EXPECT_EQ(BadPtrCount.load(), 0u)
      << "marker pointer-word read observed a shredded pointer";

  // Whole value preserved; Externalize bit stays consistent with the pointer.
  const ValVariant Final = GC::loadCoherent(Slot);
  const auto RF = RawOf(Final);
  ASSERT_TRUE(RF == RawA || RF == RawB);
  const RefVariant &FinalRef = Final.get<RefVariant>();
  if (RF == RawB) {
    EXPECT_TRUE(FinalRef.getType().isExternalized());
    EXPECT_EQ(FinalRef.getPtr<void>(), static_cast<void *>(ObjB));
  } else {
    EXPECT_FALSE(FinalRef.getType().isExternalized());
    EXPECT_EQ(FinalRef.getPtr<void>(), static_cast<void *>(ObjA));
  }
}

// Two mutators sharing one reference-typed global. In the multi-mutator model
// async invocations share the same GlobalInstance, so one thread's compiled
// global.set of a struct ref runs concurrently with another thread's global.get
// of the same 128-bit (type, pointer) slot. With the coherent store and load
// (kCoherentRefStore/kCoherentRefLoad) the shared-slot access is race-free and
// can never form a torn pair; the earlier codegen (a bare 128-bit load/store)
// tears the pair and TSan flags the data race. (The marker is
// not the racing reader here: mutators park at the STW root snapshot before
// scanSharedRoots runs, so global tearing is genuinely mutator-vs-mutator.)
// Validated race-clean under TSan; the negative control (plain store/load)
// reports the race.
//   (global $g (mut (ref null $s)) ...)
//   (func (export "set") (param $n) <loop: global.set $g = struct.new>)
//   (func (export "get") (param $n) (result i32) <loop: acc +=
//   !ref.is_null(global.get $g)>)
const std::array<WasmEdge::Byte, 172> RefGlobalConcurrentWasm{
    0,   97,  115, 109, 1,   0,   0,   0,  1,   14,  3,   95,  1,   127, 0,
    96,  1,   127, 0,   96,  1,   127, 1,  127, 3,   3,   2,   1,   2,   6,
    7,   1,   99,  0,   1,   208, 0,   11, 7,   13,  2,   3,   115, 101, 116,
    0,   0,   3,   103, 101, 116, 0,   1,  10,  61,  2,   26,  1,   1,   127,
    3,   64,  251, 1,   0,   36,  0,   32, 1,   65,  1,   106, 33,  1,   32,
    1,   32,  0,   73,  13,  0,   11,  11, 32,  1,   2,   127, 3,   64,  32,
    2,   35,  0,   209, 69,  106, 33,  2,  32,  1,   65,  1,   106, 33,  1,
    32,  1,   32,  0,   73,  13,  0,   11, 32,  2,   11,  0,   54,  4,   110,
    97,  109, 101, 2,   22,  2,   0,   2,  0,   1,   110, 1,   1,   105, 1,
    3,   0,   1,   110, 1,   1,   105, 2,  3,   97,  99,  99,  3,   11,  2,
    0,   1,   0,   1,   108, 1,   1,   0,  1,   108, 4,   4,   1,   0,   1,
    115, 7,   4,   1,   0,   1,   103};

TEST(GCThread, CoherentRefGlobalConcurrentAccess) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(RefGlobalConcurrentWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  ASSERT_NE(Mod->findFuncExports("set"), nullptr);
  ASSERT_NE(Mod->findFuncExports("get"), nullptr);
  ASSERT_TRUE(Mod->findFuncExports("set")->isCompiledFunction());
  ASSERT_TRUE(Mod->findFuncExports("get")->isCompiledFunction());

  auto Setter = VM.asyncExecute(
      "set", std::initializer_list<ValVariant>{UINT32_C(300000)},
      {ValType(TypeCode::I32)});
  auto Getter = VM.asyncExecute(
      "get", std::initializer_list<ValVariant>{UINT32_C(300000)},
      {ValType(TypeCode::I32)});
  auto RSet = Setter.get();
  auto RGet = Getter.get();
  EXPECT_TRUE(RSet);
  EXPECT_TRUE(RGet);
}

// The table analog of CoherentRefGlobalConcurrentAccess. Two async mutators
// share one table element -- one compiled `set` loop (table.set element 0 =
// struct.new), one compiled `get` loop (ref.is_null(table.get 0)).
// Table elements are always managed refs; the compiled coherent store/load
// (kCoherentRefStore / kCoherentRefLoad, routed through TableInstance's
// coherent accessors in the interpreter) make the shared 128-bit slot access
// race-free. A bare 128-bit load/store tears the (type, pointer) pair and TSan
// flags the race. Validated race-clean under TSan; the plain-access control
// reports it.
//   (table $t 1 (ref null $s))
//   (func (export "set") (param $n) <loop: table.set $t 0 = struct.new>)
//   (func (export "get") (param $n) (result i32) <loop: acc +=
//   !ref.is_null(table.get $t 0)>)
const std::array<WasmEdge::Byte, 174> RefTableConcurrentWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   14, 3,   95,  1,   127, 0,
    96,  1,   127, 0,   96,  1,   127, 1,   127, 3,  3,   2,   1,   2,   4,
    5,   1,   99,  0,   0,   1,   7,   13,  2,   3,  115, 101, 116, 0,   0,
    3,   103, 101, 116, 0,   1,   10,  65,  2,   28, 1,   1,   127, 3,   64,
    65,  0,   251, 1,   0,   38,  0,   32,  1,   65, 1,   106, 33,  1,   32,
    1,   32,  0,   73,  13,  0,   11,  11,  34,  1,  2,   127, 3,   64,  32,
    2,   65,  0,   37,  0,   209, 69,  106, 33,  2,  32,  1,   65,  1,   106,
    33,  1,   32,  1,   32,  0,   73,  13,  0,   11, 32,  2,   11,  0,   54,
    4,   110, 97,  109, 101, 2,   22,  2,   0,   2,  0,   1,   110, 1,   1,
    105, 1,   3,   0,   1,   110, 1,   1,   105, 2,  3,   97,  99,  99,  3,
    11,  2,   0,   1,   0,   1,   108, 1,   1,   0,  1,   108, 4,   4,   1,
    0,   1,   115, 5,   4,   1,   0,   1,   116};

TEST(GCThread, CoherentRefTableConcurrentAccess) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(RefTableConcurrentWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  ASSERT_NE(Mod->findFuncExports("set"), nullptr);
  ASSERT_NE(Mod->findFuncExports("get"), nullptr);
  ASSERT_TRUE(Mod->findFuncExports("set")->isCompiledFunction());
  ASSERT_TRUE(Mod->findFuncExports("get")->isCompiledFunction());

  auto Setter = VM.asyncExecute(
      "set", std::initializer_list<ValVariant>{UINT32_C(300000)},
      {ValType(TypeCode::I32)});
  auto Getter = VM.asyncExecute(
      "get", std::initializer_list<ValVariant>{UINT32_C(300000)},
      {ValType(TypeCode::I32)});
  auto RSet = Setter.get();
  auto RGet = Getter.get();
  EXPECT_TRUE(RSet);
  EXPECT_TRUE(RGet);
}

// A reallocating table.grow must not free the old Refs buffer
// while a concurrent mutator reader still holds the old DataPtr -- a UAF. Two
// async mutators share table $t: one repeatedly table.grow (which reallocates
// the backing buffer), the other repeatedly table.get element 0 (reading
// through the table's DataPtr). A collector thread runs concurrent collects,
// which free the buffers grow retired -- at a stop-the-world root scan, when
// every mutator is parked so none holds an in-flight pointer into a retired
// buffer. Without the fix, grow's realloc frees the buffer out from under the
// reader (ASan reports heap-use-after-free); with it (retire + free-at-STW) the
// read always hits live memory.
//   (table $t 1 (ref null $s))
//   (func (export "grow") (param $n) <loop: table.grow $t (ref.null) 1>)
//   (func (export "read") (param $n) (result i32) <loop: acc +=
//   !ref.is_null(table.get $t 0)>)
const std::array<WasmEdge::Byte, 177> TableGrowReadWasm{
    0,   97, 115, 109, 1,   0,   0,   0,   1,   14,  3,   95,  1,   127, 0,
    96,  1,  127, 0,   96,  1,   127, 1,   127, 3,   3,   2,   1,   2,   4,
    5,   1,  99,  0,   0,   1,   7,   15,  2,   4,   103, 114, 111, 119, 0,
    0,   4,  114, 101, 97,  100, 0,   1,   10,  66,  2,   29,  1,   1,   127,
    3,   64, 208, 0,   65,  1,   252, 15,  0,   26,  32,  1,   65,  1,   106,
    33,  1,  32,  1,   32,  0,   73,  13,  0,   11,  11,  34,  1,   2,   127,
    3,   64, 32,  2,   65,  0,   37,  0,   209, 69,  106, 33,  2,   32,  1,
    65,  1,  106, 33,  1,   32,  1,   32,  0,   73,  13,  0,   11,  32,  2,
    11,  0,  54,  4,   110, 97,  109, 101, 2,   22,  2,   0,   2,   0,   1,
    110, 1,  1,   105, 1,   3,   0,   1,   110, 1,   1,   105, 2,   3,   97,
    99,  99, 3,   11,  2,   0,   1,   0,   1,   108, 1,   1,   0,   1,   108,
    4,   4,  1,   0,   1,   115, 5,   4,   1,   0,   1,   116};

TEST(GCThread, GrowTableRetiresBufferUnderConcurrentReaders) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(TableGrowReadWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  ASSERT_NE(Mod->findFuncExports("grow"), nullptr);
  ASSERT_NE(Mod->findFuncExports("read"), nullptr);
  ASSERT_TRUE(Mod->findFuncExports("grow")->isCompiledFunction());
  ASSERT_TRUE(Mod->findFuncExports("read")->isCompiledFunction());

  std::atomic<bool> Done{false};
  std::thread Collector([&] {
    auto &Ctrl = VM.getController();
    while (!Done.load(std::memory_order_relaxed)) {
      Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    }
  });

  auto Grower = VM.asyncExecute(
      "grow", std::initializer_list<ValVariant>{UINT32_C(20000)},
      {ValType(TypeCode::I32)});
  auto Reader = VM.asyncExecute(
      "read", std::initializer_list<ValVariant>{UINT32_C(20000)},
      {ValType(TypeCode::I32)});
  auto RGrow = Grower.get();
  auto RRead = Reader.get();
  Done.store(true, std::memory_order_relaxed);
  Collector.join();
  EXPECT_TRUE(RGrow);
  EXPECT_TRUE(RRead);
}

// An interpreter table.grow reallocates the Refs vector object that a
// concurrent interpreter table.get/table.set reads (getRefAddr/setRefAddr read
// Refs.size()/Refs[Idx], i.e. the vector control block grow reassigns with
// `Refs = std::move(New)`). Making the AOT DataPtr load atomic left the
// interpreter path, which reads the vector object directly, with a residual
// data race. Making table growth an exclusive mutator-parking STW
// closes it: while the grower swaps the buffer every other Running mutator is
// parked at a safe point, so no reader is inside getRefAddr/setRefAddr. Two
// async interpreter mutators share one table -- a grow loop vs a get/set loop
// -- and must both finish race-clean (TSan) and use-after-free-clean (ASan).
//   (table $t 1 100000 funcref)
//   (func (export "grow") (param $n)  <loop: drop (table.grow $t (ref.null)
//   1)>) (func (export "getset") (param $n) (result i32)
//     <loop: acc += ref.is_null(table.get $t 0); table.set $t 0 (ref.null)>)
const std::array<WasmEdge::Byte, 198> TableGrowGetSetWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   10,  2,   96,  1,   127, 0,
    96,  1,   127, 1,   127, 3,   3,   2,   0,   1,   4,   7,   1,   112, 1,
    1,   160, 141, 6,   7,   17,  2,   4,   103, 114, 111, 119, 0,   0,   6,
    103, 101, 116, 115, 101, 116, 0,   1,   10,  81,  2,   34,  1,   1,   127,
    2,   64,  3,   64,  32,  1,   32,  0,   79,  13,  1,   208, 112, 65,  1,
    252, 15,  0,   26,  32,  1,   65,  1,   106, 33,  1,   12,  0,   11,  11,
    11,  44,  1,   2,   127, 2,   64,  3,   64,  32,  1,   32,  0,   79,  13,
    1,   32,  2,   65,  0,   37,  0,   209, 106, 33,  2,   65,  0,   208, 112,
    38,  0,   32,  1,   65,  1,   106, 33,  1,   12,  0,   11,  11,  32,  2,
    11,  0,   60,  4,   110, 97,  109, 101, 2,   22,  2,   0,   2,   0,   1,
    110, 1,   1,   105, 1,   3,   0,   1,   110, 1,   1,   105, 2,   3,   97,
    99,  99,  3,   23,  2,   0,   2,   0,   4,   100, 111, 110, 101, 1,   1,
    108, 1,   2,   0,   4,   100, 111, 110, 101, 1,   1,   108, 5,   4,   1,
    0,   1,   116};

TEST(GCThread, InterpreterGrowExcludesConcurrentReader) {
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  // Interpreter mode (default): the grow and get/set loops run through the
  // interpreter engine, which polls the GC safe point before each instruction.
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(TableGrowGetSetWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  ASSERT_NE(Mod->findFuncExports("grow"), nullptr);
  ASSERT_NE(Mod->findFuncExports("getset"), nullptr);
  // Confirm we exercise the interpreter path (the atomic-DataPtr path is
  // AOT-only; this test covers the interpreter's direct Refs-vector reads).
  ASSERT_FALSE(Mod->findFuncExports("grow")->isCompiledFunction());
  ASSERT_FALSE(Mod->findFuncExports("getset")->isCompiledFunction());

  // Background watchdog: a wedged handshake (grower's waitForAcks never
  // satisfied or a reader that never parks) becomes a failure + hard-exit,
  // never a hang.
  std::atomic<bool> TestDone{false};
  std::thread Watchdog([&]() {
    const auto WD = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!TestDone.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > WD) {
        ADD_FAILURE() << "interpreter grow<->reader deadlocked";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  auto Grower = VM.asyncExecute(
      "grow", std::initializer_list<ValVariant>{UINT32_C(20000)},
      {ValType(TypeCode::I32)});
  auto Reader = VM.asyncExecute(
      "getset", std::initializer_list<ValVariant>{UINT32_C(20000)},
      {ValType(TypeCode::I32)});
  auto RGrow = Grower.get();
  auto RRead = Reader.get();
  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
  EXPECT_TRUE(RGrow);
  EXPECT_TRUE(RRead);
}

TEST(GCThread, GrowCollectArbitration) {
  // Two growers + one collector contend for the per-controller exclusive
  // token. Growers use the blocking acquire (they must eventually grow), so a
  // losing grower parks on a FIFO ticket and, on wake, owns the token and runs
  // its own STW; the collector uses the non-blocking acquire and simply skips a
  // cycle when a grow holds the token. The invariants: (1) no deadlock (a hard
  // watchdog + _Exit turns a wedge into a failure), (2) the table's max limit
  // is respected exactly -- every grow reads the current min inside the token,
  // so a loser re-validates the limit a prior winner raised and total
  // successful grows == max (never over-grows), (3) collections keep completing
  // throughout.
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  Alloc.setManualGC(true); // only our explicit collect() cycles run

  constexpr uint64_t MaxGrow = 1000;
  AST::TableType TType(ValType(TypeCode::FuncRef), 0, MaxGrow);
  // funcref: not GC-managed.
  Runtime::Instance::TableInstance Table(TType, false);
  Table.setAllocator(Alloc);

  std::atomic<uint32_t> DoneGrowers{0};
  std::atomic<uint64_t> TotalSucceeded{0};
  std::atomic<bool> OverGrew{false};

  auto GrowWorker = [&]() {
    // A registered value stack makes this a real cooperative Running mutator:
    // the loop polls the safe point (like the interpreter engine) so a peer
    // grower's in-flight handshake is acknowledged instead of stalled.
    Runtime::StackManager StackMgr(Ctrl);
    for (uint64_t I = 0; I < MaxGrow; ++I) {
      if (Ctrl.stopRequested()) {
        Ctrl.gcSafepoint();
      }
      if (Table.growTable(1)) {
        TotalSucceeded.fetch_add(1, std::memory_order_acq_rel);
      }
      if (Table.getSize() > MaxGrow) {
        OverGrew.store(true, std::memory_order_relaxed);
      }
    }
    DoneGrowers.fetch_add(1, std::memory_order_acq_rel);
  };

  std::atomic<bool> StopCollector{false};
  std::atomic<uint64_t> Collections{0};
  std::thread Collector([&]() {
    while (!StopCollector.load(std::memory_order_acquire)) {
      if (Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false)) {
        Collections.fetch_add(1, std::memory_order_relaxed);
      }
      std::this_thread::yield();
    }
  });

  std::thread GrowerA(GrowWorker);
  std::thread GrowerB(GrowWorker);

  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(60);
  while (DoneGrowers.load(std::memory_order_acquire) < 2) {
    if (std::chrono::steady_clock::now() > Deadline) {
      ADD_FAILURE() << "grow<->collect arbitration deadlocked: "
                    << DoneGrowers.load(std::memory_order_acquire)
                    << "/2 growers finished";
      std::fflush(stderr);
      std::_Exit(2);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  GrowerA.join();
  GrowerB.join();
  StopCollector.store(true, std::memory_order_release);
  Collector.join();

  // The limit was re-validated by every grow: total successes == max, the table
  // ends exactly at max, and no grow ever pushed it past max.
  EXPECT_FALSE(OverGrew.load(std::memory_order_relaxed));
  EXPECT_EQ(TotalSucceeded.load(std::memory_order_acquire), MaxGrow);
  EXPECT_EQ(Table.getSize(), MaxGrow);
}

TEST(GCThread, GrowOverLimitDoesNotStopTheWorld) {
  // A definitely-doomed (over-limit) grow is rejected before the
  // exclusive token / handshake, so a guest `table.grow <huge>` loop cannot
  // force a global stop-the-world every iteration. Prove it by holding a
  // collection's token open (pinned sweep, as
  // GrowBlocksDuringCollectAndViceVersa does): a token-taking grow would park
  // behind the collection, but an over-limit grow must still return the failure
  // value promptly, never queueing for the token.
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  Alloc.setManualGC(true);

  AST::TableType TType(ValType(TypeCode::FuncRef), 1, 4); // min 1, max 4
  // funcref: not GC-managed.
  Runtime::Instance::TableInstance Table(TType, false);
  Table.setAllocator(Alloc);

  std::atomic<bool> AtSweep{false};
  std::atomic<bool> ReleaseSweep{false};
  std::atomic<bool> HookFired{false};
  Alloc.setSweepPauseHook([&]() noexcept {
    if (HookFired.exchange(true)) {
      return;
    }
    AtSweep.store(true, std::memory_order_release);
    while (!ReleaseSweep.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  // Watchdog: if the over-limit grow ever blocks on the token (regression),
  // this turns the hang into a failure + hard-exit rather than an indefinite
  // wait.
  std::atomic<bool> TestDone{false};
  std::thread Watchdog([&]() {
    const auto WD = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!TestDone.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > WD) {
        ADD_FAILURE() << "over-limit grow blocked on the exclusive token (STW)";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  std::thread Collector([&]() { EXPECT_TRUE(Alloc.manualCollect()); });
  while (!AtSweep.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  // The collection now holds the exclusive token (OwnedCollecting) and keeps it
  // until we release the sweep. An over-limit grow (1 + 1000 >> max 4) must be
  // rejected pre-token: same failure value, size unchanged, never queued.
  const bool Res = Table.growTable(1000);
  EXPECT_FALSE(Res);
  EXPECT_EQ(Table.getSize(), 1u);
  EXPECT_EQ(Ctrl.debugExclusiveWaiters(), 0u);

  ReleaseSweep.store(true, std::memory_order_release);
  Collector.join();
  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
  Alloc.setSweepPauseHook(nullptr);
}

// The interpreter table.grow initializer must be rooted across the grow
// window. runTableGrowOp pops the init ref into a bare local before growTable
// acquires the exclusive token, and a grow-loser can park (Blocked)
// through an entire concurrent collection before it owns the token. During that
// park the popped ref is on no GC-scanned stack and in no root set, so a
// collection whose root snapshot runs while we are parked would sweep it and
// the grow would broadcast a dangling ref into the new slots. The fix pins it
// as a scoped boundary root before growTable, released after.
//
// Determinism is delicate because StackManager::pop already shades a ref when a
// collection is mid-mark: to make the pin (not the pop-time shade) the sole
// savior, the grower must pop while the write barrier is still quiet (heap
// Idle). setPreCycleHook holds the collection after it wins the exclusive token
// but before the Idle->MarkingRoot CAS; the grower pops+pins+parks in that
// quiet window, then the released collection's stop-the-world snapshot -- gated
// behind the grower's Blocked ack -- scans the scoped roots and must find the
// pinned ref. Remove the pin (tableInstr.cpp) and this test sweeps the init
// object: an ASan heap-use-after-free on the sentinel read below, or memory
// usage 0.
TEST(GCThread, InterpreterGrowInitializerSurvivesCollect) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  WasmEdge::Executor::Executor Exe(Conf);
  GC::Controller &Ctrl = Exe.getController();
  GC::Allocator &Alloc = Exe.getAllocator();
  Alloc.setManualGC(true); // only our explicit collect() cycles run

  // Nullable abstract struct-ref table, grown by 1 with a live struct init ref.
  AST::TableType TType(ValType(TypeCode::RefNull, TypeCode::StructRef), 0, 10);
  // structref: GC-managed.
  Runtime::Instance::TableInstance Table(TType, true);
  Table.setAllocator(Alloc);

  auto MakeSentinelStruct = [&]() noexcept -> RefVariant {
    void *P = Alloc.allocate(
        [](void *Pointer) noexcept {
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = nullptr; // leaf: no managed children to trace
          Raw->TypeIdx = 0;
          Raw->Length = 1;
          // Zero-extended numeric: the tracer's pointer-word read rejects it,
          // so this is a leaf, and the value doubles as a survival sentinel.
          new (&Raw->data()[0]) ValVariant(UINT32_C(0xC0FFEE));
        },
        static_cast<uint32_t>(sizeof(RawData) + sizeof(ValVariant)));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };

  std::atomic<bool> ArmPreCycle{false};
  std::atomic<bool> TokenHeldIdle{false};
  std::atomic<bool> ReleaseCycle{false};
  std::atomic<bool> GraceCleared{false};
  std::atomic<bool> GoGrow{false};
  RawData *CapturedRaw = nullptr;

  // Hold the armed collection between winning the token and the
  // Idle->MarkingRoot CAS: token held, barrier still quiet. The first
  // (grace-clearing) collection runs with ArmPreCycle == false, so this is a
  // no-op there.
  Alloc.setPreCycleHook([&]() noexcept {
    if (!ArmPreCycle.load(std::memory_order_acquire)) {
      return;
    }
    TokenHeldIdle.store(true, std::memory_order_release);
    while (!ReleaseCycle.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  // Watchdog: a wedged handshake / token handoff becomes a failure + hard-exit.
  std::atomic<bool> TestDone{false};
  std::thread Watchdog([&]() {
    const auto WD = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!TestDone.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > WD) {
        ADD_FAILURE() << "interpreter grow-initializer rooting deadlocked";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  std::thread Grow([&]() {
    Runtime::StackManager StackMgr(Ctrl);
    RefVariant StructRef = MakeSentinelStruct();
    CapturedRaw = StructRef.getPtr<RawData>();
    // Operand-stack layout runTableGrowOp expects: init ref below, N on top.
    StackMgr.push(StructRef);
    StackMgr.push(ValVariant(UINT32_C(1)));
    // First collection: the struct is rooted on this stack, so it survives and
    // its born-gray grace period is cleared -- a later unrooted cycle would now
    // sweep it. This runs with ArmPreCycle == false (hook is a no-op).
    EXPECT_TRUE(Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false));
    GraceCleared.store(true, std::memory_order_release);
    // Wait until the armed collection holds the token in the Idle window.
    while (!GoGrow.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    // Drive the real interpreter grow: it pops the init ref (barrier quiet --
    // heap Idle), pins it, then parks Blocked behind the collection's token
    // until the collection completes and the FIFO handoff grants the token
    // here.
    EXPECT_TRUE(WasmEdge::Executor::gcTestRunTableGrowOp(Exe, StackMgr, Table));
  });

  // Wait for the grace-clearing collection to finish.
  while (!GraceCleared.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  // Arm the hook, then start the collection that will run during the grow's
  // park.
  ArmPreCycle.store(true, std::memory_order_release);
  std::thread Collector([&]() { EXPECT_TRUE(Alloc.manualCollect()); });
  // Wait until the collection holds the token with the heap still Idle.
  while (!TokenHeldIdle.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  // Release the grower: it pops (quiet barrier), pins, and parks behind the
  // token.
  GoGrow.store(true, std::memory_order_release);
  // Deterministically wait until the grower is queued behind the exclusive
  // token.
  while (Ctrl.debugExclusiveWaiters() == 0) {
    std::this_thread::yield();
  }
  // Now let the collection run its snapshot + mark + sweep. Its root snapshot
  // must find the pinned init ref in the scoped roots; without the pin the
  // struct is White (off-stack, un-shaded) and is swept here.
  ReleaseCycle.store(true, std::memory_order_release);

  Grow.join();
  Collector.join();
  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
  Alloc.setPreCycleHook(nullptr);

  // The grow published slot 0 with the init ref, which survived the concurrent
  // collection. Reading the sentinel field traps under ASan
  // (heap-use-after-free) if the object was swept; the live-byte count is the
  // plain-build signal.
  auto Slot = Table.getRefAddr(0);
  ASSERT_TRUE(Slot);
  ASSERT_FALSE(Slot->isNull());
  RawData *Raw = Slot->getPtr<RawData>();
  EXPECT_EQ(Raw, CapturedRaw);
  EXPECT_EQ(Raw->data()[0].get<uint32_t>(), UINT32_C(0xC0FFEE));
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
}

} // namespace
