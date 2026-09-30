// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCShadowTest.cpp - GC shadow root tests ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for GC roots of compiled code: the
/// shadow-root chain, its truncation on a fault, the shadow spill
/// around calls, and the safepoint poll in compiled loops.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

// A hardware trap escapes a compiled frame by longjmp-ing out of the fault
// handler, which needs unwind information that the generated code registers
// only on some platforms -- the same gap
// AOTCrossModule.CompiledFramelessTrapAttributedToCallee skips for. On Windows
// it is fatal rather than merely unattributable: the unwind fails inside the
// vectored exception handler and RtlRaiseStatus turns it into a noncontinuable
// exception. Ruled out as causes there: the DbgHelp stack capture in emitFault
// and the shadow-head restore store -- disabling either one still crashes.
bool compiledFramesUnwindable() noexcept {
#if defined(_WIN32)
  return false;
#else
  return true;
#endif
}

// The stable shadow-head cell must keep a fixed address across Entries-vector
// relocation. Registering many threads forces the vector to reallocate
// repeatedly; a head pointer taken before must still equal the head looked up
// after, with an unchanged incarnation.
TEST(GCThread, ShadowHeadStableAcrossEntryRelocation) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S0;
  auto Reg0 = Ctrl.registerStack(S0);
  GC::Controller::ShadowHead *H0 = Ctrl.currentShadowHead();
  ASSERT_NE(H0, nullptr);
  const uint64_t Inc0 = H0->IncId;

  constexpr int K = 64; // grow Entries from 1 to 65 -> multiple reallocations
  std::atomic<int> Registered{0};
  std::atomic<bool> Release{false};
  std::vector<std::thread> Threads;
  Threads.reserve(K);
  for (int I = 0; I < K; ++I) {
    Threads.emplace_back([&] {
      std::vector<ValVariant> S;
      auto Reg = Ctrl.registerStack(S);
      Registered.fetch_add(1);
      while (!Release.load()) {
        std::this_thread::yield();
      }
    });
  }
  while (Registered.load() < K) {
    std::this_thread::yield();
  }

  // After all K registrations (Entries surely reallocated), the original
  // thread's head cell is unchanged in identity and incarnation.
  GC::Controller::ShadowHead *H1 = Ctrl.currentShadowHead();
  EXPECT_EQ(H1, H0);
  EXPECT_EQ(H0->IncId, Inc0);

  Release.store(true);
  for (auto &T : Threads) {
    T.join();
  }
}

// An abnormal fault must reset the thread's shadow-root head to
// its boundary value before the longjmp, so no scanner walks a ShadowFrame in
// the compiled stack the fault is about to unwind. Because the shadow chain is
// a strict stack parallel to the compiled call stack, one store to the boundary
// value truncates the whole abandoned suffix. Models the real path with a real
// Fault (setjmp) + emitFault (longjmp); the published frame lives in a scope
// the longjmp abandons, so after recovery the head must equal the boundary
// (nullptr), not the abandoned frame.
TEST(GCThread, FaultTruncatesShadowHeadOnAbnormalUnwind) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
  ASSERT_NE(H, nullptr);
  ASSERT_EQ(H->Head.load(std::memory_order_relaxed), nullptr); // boundary/Prev0

  Fault F;
  // Arm exactly as helper.cpp does at the compiled-call boundary, before any
  // frame is published: record the head cell + its boundary value.
  F.armShadowRestore(reinterpret_cast<std::atomic<void *> *>(&H->Head),
                     H->Head.load(std::memory_order_relaxed));

  volatile int Reached = 0;
  if (PREPARE_FAULT(F) == 0) {
    // "compiled frame": publish a shadow frame whose storage is in this scope,
    // then trap. The longjmp abandons the scope without running the pop.
    ValVariant Slots[1] = {};
    GC::Controller::ShadowFrame Frame;
    Frame.Prev = H->Head.load(std::memory_order_relaxed);
    Frame.Count = 1;
    Frame.Slots = Slots;
    H->Head.store(&Frame, std::memory_order_release);
    ASSERT_EQ(H->Head.load(std::memory_order_acquire), &Frame); // published
    Reached = 1;
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds); // longjmp back
    FAIL() << "emitFault must not return";
  } else {
    EXPECT_EQ(Reached, 1);
    // Truncated to the boundary: the head no longer references the abandoned
    // frame that lived in the now-unwound scope.
    EXPECT_EQ(H->Head.load(std::memory_order_acquire), nullptr);
  }
}

// Truncating the head protects a walk that starts after the fault, not one
// already in flight: a remote scanner that loaded the old head still reads the
// frames the longjmp is about to abandon, and the recovery code then pushes
// new C++ frames over them. emitFault must therefore wait (after the
// truncation store, before the longjmp) until no walker holds the chain. The
// test holds a real remote walk open with the shadow-walk hook while the
// walked thread faults, and requires the fault to recover only after the walk
// was released.
TEST(GCThread, FaultWaitsForInFlightShadowWalk) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;

  std::atomic<bool> FramePublished{false};
  std::atomic<bool> WalkStarted{false};
  std::atomic<bool> LetWalkFinish{false};
  std::atomic<bool> WalkReleased{false};
  std::atomic<int> RecoveredAfterWalkRelease{-1};

  Ctrl.setShadowWalkHook([&] {
    WalkStarted.store(true);
    while (!LetWalkFinish.load()) {
      std::this_thread::yield();
    }
    WalkReleased.store(true);
  });

  std::thread Mutator([&] {
    auto Reg = Ctrl.registerStack(S);
    GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
    ASSERT_NE(H, nullptr);
    // A host call in progress: the remote collection scans this thread's
    // chain in place instead of waiting for an ack.
    GC::Controller::NativeScope Native(Ctrl);
    Fault F;
    F.armShadowRestore(reinterpret_cast<std::atomic<void *> *>(&H->Head),
                       H->Head.load(std::memory_order_relaxed), &H->Walkers);
    if (PREPARE_FAULT(F) == 0) {
      ValVariant Slots[1] = {};
      GC::Controller::ShadowFrame Frame;
      Frame.Prev = H->Head.load(std::memory_order_relaxed);
      Frame.Count = 1;
      Frame.Slots = Slots;
      H->Head.store(&Frame, std::memory_order_release);
      FramePublished.store(true);
      while (!WalkStarted.load()) {
        std::this_thread::yield();
      }
      // The walker holds the chain: this must not longjmp until it is done.
      Fault::emitFault(ErrCode::Value::MemoryOutOfBounds);
      FAIL() << "emitFault must not return";
    } else {
      RecoveredAfterWalkRelease.store(WalkReleased.load() ? 1 : 0);
      EXPECT_EQ(H->Head.load(std::memory_order_acquire), nullptr);
    }
  });

  while (!FramePublished.load()) {
    std::this_thread::yield();
  }
  std::thread Collector([&] { EXPECT_TRUE(Ctrl.collect(true, false)); });
  while (!WalkStarted.load()) {
    std::this_thread::yield();
  }
  // Give a non-waiting emitFault time to longjmp while the walk is still held.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  LetWalkFinish.store(true);

  Mutator.join();
  Collector.join();
  Ctrl.setShadowWalkHook(nullptr);
  EXPECT_EQ(RecoveredAfterWalkRelease.load(), 1);
}

// Positive control for the truncation test: with the Fault not armed,
// emitFault's longjmp leaves the head pointing at the abandoned compiled frame
// -- exactly the dangling state truncation fixes, and what the recovery C++
// would then clobber. This asserts the dangling address (never dereferences
// it), so it stays memory-safe.
TEST(GCThread, FaultLeavesShadowHeadDanglingWithoutTruncation) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
  ASSERT_NE(H, nullptr);

  Fault F; // deliberately not armed -> no truncation
  void *volatile AbandonedFrame = nullptr;
  if (PREPARE_FAULT(F) == 0) {
    ValVariant Slots[1] = {};
    GC::Controller::ShadowFrame Frame;
    Frame.Prev = H->Head.load(std::memory_order_relaxed);
    Frame.Count = 1;
    Frame.Slots = Slots;
    H->Head.store(&Frame, std::memory_order_release);
    AbandonedFrame = &Frame;
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds);
    FAIL() << "emitFault must not return";
  } else {
    // Without truncation the head still points at the (now out-of-scope) frame.
    EXPECT_NE(H->Head.load(std::memory_order_acquire), nullptr);
    EXPECT_EQ(reinterpret_cast<void *>(H->Head.load(std::memory_order_acquire)),
              AbandonedFrame);
    // Repair the chain so Controller teardown does not observe the dangling
    // head.
    H->Head.store(nullptr, std::memory_order_release);
  }
}

// Nested compiled boundaries each truncate to their own boundary value. An
// inner fault resets the head to the frame the outer boundary published, not
// all the way to null -- proving per-boundary O(1) truncation composes across
// compiled->host->compiled reentry (localHandler is a stack of Faults;
// emitFault truncates the innermost).
TEST(GCThread, FaultTruncatesShadowHeadToNearestBoundary) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
  ASSERT_NE(H, nullptr);

  // Outer boundary publishes one frame that stays live across the inner fault.
  ValVariant OuterSlots[1] = {};
  GC::Controller::ShadowFrame OuterFrame;
  OuterFrame.Prev = H->Head.load(std::memory_order_relaxed);
  OuterFrame.Count = 1;
  OuterFrame.Slots = OuterSlots;
  H->Head.store(&OuterFrame, std::memory_order_release);

  Fault Inner;
  // Inner boundary value == &OuterFrame (what the outer boundary left).
  Inner.armShadowRestore(reinterpret_cast<std::atomic<void *> *>(&H->Head),
                         H->Head.load(std::memory_order_relaxed));

  volatile int Reached = 0;
  if (PREPARE_FAULT(Inner) == 0) {
    ValVariant InnerSlots[1] = {};
    GC::Controller::ShadowFrame InnerFrame;
    InnerFrame.Prev = H->Head.load(std::memory_order_relaxed);
    InnerFrame.Count = 1;
    InnerFrame.Slots = InnerSlots;
    H->Head.store(&InnerFrame, std::memory_order_release);
    Reached = 1;
    Fault::emitFault(ErrCode::Value::DivideByZero);
    FAIL() << "emitFault must not return";
  } else {
    EXPECT_EQ(Reached, 1);
    // Truncated to the outer boundary, not to null: the outer frame is intact.
    EXPECT_EQ(H->Head.load(std::memory_order_acquire), &OuterFrame);
  }
  // Clean up the outer frame so teardown scans an empty chain.
  H->Head.store(nullptr, std::memory_order_release);
}

// State restore: an abnormal fault's longjmp skips the NativeScope
// destructor of a host call it unwinds through, stranding the entry in
// NativeRunning (remotely scannable) though its boundary was Running.
// restoreStateAfterFault -- called from helper.cpp's post-longjmp recovery --
// must return it to the boundary state. Models the strand with a real
// setjmp/emitFault: NativeScope flips to NativeRunning, the longjmp skips its
// dtor, and the recovery restores Running. The pre-restore assertion is the
// built-in positive control: the entry is stranded until the fix runs.
TEST(GCThread, StateRestoreAfterFaultUnstrandsNativeRunning) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  ASSERT_EQ(Ctrl.currentMutatorState(), GC::Controller::MutatorState::Running);

  // Capture the boundary state as helper.cpp does before the compiled call.
  const GC::Controller::MutatorState Boundary = Ctrl.currentMutatorState();
  ASSERT_EQ(Boundary, GC::Controller::MutatorState::Running);

  Fault F; // unarmed: no shadow head to truncate here, only state to restore
  if (PREPARE_FAULT(F) == 0) {
    GC::Controller::NativeScope Native(Ctrl); // host call -> NativeRunning
    ASSERT_EQ(Ctrl.currentMutatorState(),
              GC::Controller::MutatorState::NativeRunning);
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds); // skips Native's dtor
    FAIL() << "emitFault must not return";
  } else {
#if defined(_WIN32)
    // The Windows CRT's longjmp unwinds the frame, so NativeScope's dtor does
    // run and the entry is never stranded. The strand is what the restore below
    // repairs on POSIX; here only that restore is meaningful.
    const auto AfterFault = Ctrl.currentMutatorState();
    EXPECT_TRUE(AfterFault == GC::Controller::MutatorState::NativeRunning ||
                AfterFault == GC::Controller::MutatorState::Running);
#else
    // Positive control: the skipped dtor left the entry stranded NativeRunning.
    EXPECT_EQ(Ctrl.currentMutatorState(),
              GC::Controller::MutatorState::NativeRunning);
#endif
    // The fix: recovery restores the boundary state.
    Ctrl.restoreStateAfterFault(Boundary);
    EXPECT_EQ(Ctrl.currentMutatorState(),
              GC::Controller::MutatorState::Running);
  }
}

// When the boundary was itself NativeRunning (a host->guest
// reentry -- the outer host call is still active), the fault recovery must
// restore to NativeRunning, not force Running, or the still-live outer native
// call would be mismarked (making the coordinator wait for an ack it can never
// deliver). No NativeScope is skipped in this shape, so restoring to the
// boundary is the correct no-op-preserving behavior.
TEST(GCThread, StateRestoreAfterFaultPreservesReentryNativeRunning) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  GC::Controller::NativeScope Native(Ctrl); // outer host -> NativeRunning
  const GC::Controller::MutatorState Boundary = Ctrl.currentMutatorState();
  ASSERT_EQ(Boundary, GC::Controller::MutatorState::NativeRunning);

  Ctrl.restoreStateAfterFault(Boundary);
  EXPECT_EQ(Ctrl.currentMutatorState(),
            GC::Controller::MutatorState::NativeRunning);
  // Native's dtor restores Running normally at scope end.
}

// AOT codegen thesis: a JIT-compiled function allocates a struct into a
// ref local, then calls a host function that triggers GC. With ScanNative=false
// the native-stack alloca is not conservatively scanned, so the struct survives
// only if the compiler-emitted shadow spill published the ref for the
// collector. (module (type $s (struct (field i32))) (import "host" "collect"
// (func))
//  (func (export "run") (result i32) (local $r (ref null $s))
//    (local.set $r (struct.new_default $s)) (call 0)
//    (i32.eqz (ref.is_null (local.get $r)))))
const std::array<WasmEdge::Byte, 106> ShadowSpillWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x02, 0x10,
    0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65,
    0x63, 0x74, 0x00, 0x01, 0x03, 0x02, 0x01, 0x02, 0x07, 0x07, 0x01, 0x03,
    0x72, 0x75, 0x6e, 0x00, 0x01, 0x0a, 0x12, 0x01, 0x10, 0x01, 0x01, 0x63,
    0x00, 0xfb, 0x01, 0x00, 0x21, 0x00, 0x10, 0x00, 0x20, 0x00, 0xd1, 0x45,
    0x0b, 0x00, 0x1f, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0a, 0x01, 0x00,
    0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x02, 0x06, 0x01, 0x01,
    0x01, 0x00, 0x01, 0x72, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

std::atomic<uint64_t> ShadowSpillUsageAfter{0};

// Host "collect": runs two collections with ScanNative=false. Born-gray keeps
// the struct through cycle 1 regardless; cycle 2 sweeps it unless a real root
// (the shadow spill) re-grays it. Records surviving usage for the assertion.
class ShadowCollectHost : public Runtime::HostFunction<ShadowCollectHost> {
public:
  Expect<void> body(const Runtime::CallingFrame &CF) {
    auto &Alloc = CF.getExecutor()->getAllocator();
    Alloc.manualCollect(false);
    Alloc.manualCollect(false);
    ShadowSpillUsageAfter.store(Alloc.getMemoryUsage(),
                                std::memory_order_relaxed);
    return {};
  }
};

TEST(GCThread, ShadowSpillKeepsRefLocalAcrossCompiledCall) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  // Declare the host module before the VM so it is destroyed after the VM's
  // instantiated module, which depends on it for the "collect" import. The
  // reverse order trips ~ModuleInstance's !hasDependents() assert
  // (Debug/UBSan).
  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<ShadowCollectHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));

  ASSERT_TRUE(VM.loadWasm(ShadowSpillWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  // Guard against a silent interpreter fallback (unsafeLoadJITExecutable logs
  // "use interpreter mode instead" and continues on JIT failure). If "run" is
  // not compiled, this test would prove nothing about codegen -- fail loudly.
  const auto *RunMod = VM.getActiveModule();
  ASSERT_NE(RunMod, nullptr);
  const auto *RunFn = RunMod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  ShadowSpillUsageAfter.store(0, std::memory_order_relaxed);
  auto Res = VM.execute("run");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  // The struct was reachable only through the codegen shadow spill during the
  // 2nd collect; surviving usage proves the compiler-emitted spill was scanned.
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u);
}

// Operand-stack coverage: the struct is held only on run's operand stack
// across the host call -- no local, no param. In compiled code that is an LLVM
// SSA value (a register or a native spill slot), so the collector can see it
// only if the compiler spills the ref-typed operand-stack entries into the
// shadow frame at the call site, alongside the ref locals. ScanNative=false in
// the host's collect keeps the native stack out of the picture, exactly as in
// ShadowSpillKeepsRefLocalAcrossCompiledCall.
// (module (type $s (struct (field i32))) (import "host" "collect" (func))
//  (func (export "run") (result i32)
//    (struct.new_default $s) (call 0) (ref.is_null) (i32.eqz)))
const std::array<WasmEdge::Byte, 79> OperandSpillWasm{
    0,   97,  115, 109, 1,  0,   0,  0,   1,   12, 3,   95,  1,   127, 0,  96,
    0,   0,   96,  0,   1,  127, 2,  16,  1,   4,  104, 111, 115, 116, 7,  99,
    111, 108, 108, 101, 99, 116, 0,  1,   3,   2,  1,   2,   7,   7,   1,  3,
    114, 117, 110, 0,   1,  10,  11, 1,   9,   0,  251, 1,   0,   16,  0,  209,
    69,  11,  0,   11,  4,  110, 97, 109, 101, 4,  4,   1,   0,   1,   115};

TEST(GCThread, ShadowSpillKeepsOperandStackRefAcrossCompiledCall) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<ShadowCollectHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(OperandSpillWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *RunMod = VM.getActiveModule();
  ASSERT_NE(RunMod, nullptr);
  const auto *RunFn = RunMod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  ShadowSpillUsageAfter.store(0, std::memory_order_relaxed);
  auto Res = VM.execute("run");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), UINT32_C(1));
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u)
      << "a ref held only on the compiled operand stack across a host call "
         "was swept";
}

// Host "prep" for the two blocking-intrinsic tests below: ages the structs on
// the guest's operand stack out of their newborn grace (one cycle), records the
// heap usage they account for, reports that it did, then holds the guest in
// the host call until the test's collector owns the exclusive token -- so the
// guest's next instruction runs against a collection that is about to scan it
// remotely. Each test owns one SpillPrepSync on its stack and hands it to its
// own SpillPrepHost, so no handshake state is shared between the two tests.
struct SpillPrepSync {
  std::atomic<bool> PrepDone{false};
  std::atomic<bool> TokenHeld{false};
  std::atomic<uint64_t> UsageAfterPrep{0};
};

class SpillPrepHost : public Runtime::HostFunction<SpillPrepHost> {
public:
  explicit SpillPrepHost(SpillPrepSync &S) : Sync(S) {}
  Expect<void> body(const Runtime::CallingFrame &CF) {
    auto &Alloc = CF.getExecutor()->getAllocator();
    Alloc.manualCollect(false);
    Sync.UsageAfterPrep.store(Alloc.getMemoryUsage());
    Sync.PrepDone.store(true);
    while (!Sync.TokenHeld.load()) {
      std::this_thread::yield();
    }
    return {};
  }

private:
  SpillPrepSync &Sync;
};

// table.grow is an exclusive stop-the-world operation: behind a running
// collection the compiled proxy parks this thread Blocked, and the collection
// then scans it remotely -- shadow chain only. Two references are live across
// the grow and neither is on any scanned stack: one below the grow's operands
// (must be spilled at the grow site) and the grow's init value itself, which
// compiled code passes to the proxy by value in a register/native slot (the
// proxy must pin it, as the interpreter's runTableGrowOp does). The collector
// is held at the pre-cycle hook (token owned, heap Idle) until the guest has
// queued behind it, so the cycle's root scan deterministically sees the guest
// parked in the grow. Both structs are past their newborn grace (prep ran a
// cycle), so survival can only come from rooting.
// (module (type $s (struct (field i32))) (import "host" "prep" (func))
//  (table $t 1 10 (ref null $s))
//  (func (export "run") (result i32 i32)
//    (struct.new_default $s) (struct.new_default $s) (call 0)
//    (i32.const 1) (table.grow $t) (drop)
//    (ref.is_null) (i32.eqz)
//    (ref.is_null (table.get $t (i32.const 1))) (i32.eqz)))
const std::array<WasmEdge::Byte, 106> GrowSpillWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   13,  3,  95,  1,   127,
    0,   96,  0,   0,   96,  0,   2,   127, 127, 2,   13, 1,   4,   104,
    111, 115, 116, 4,   112, 114, 101, 112, 0,   1,   3,  2,   1,   2,
    4,   6,   1,   99,  0,   1,   1,   10,  7,   7,   1,  3,   114, 117,
    110, 0,   1,   10,  26,  1,   24,  0,   251, 1,   0,  251, 1,   0,
    16,  0,   65,  1,   252, 15,  0,   26,  209, 69,  65, 1,   37,  0,
    209, 69,  11,  0,   17,  4,   110, 97,  109, 101, 4,  4,   1,   0,
    1,   115, 5,   4,   1,   0,   1,   116};

TEST(GCThread, ShadowSpillKeepsOperandStackRefAcrossCompiledTableGrow) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  SpillPrepSync Sync;
  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("prep", std::make_unique<SpillPrepHost>(Sync));

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(GrowSpillWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  auto &Ctrl = VM.getController();
  Alloc.setManualGC(true);

  const auto *RunMod = VM.getActiveModule();
  ASSERT_NE(RunMod, nullptr);
  const auto *RunFn = RunMod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  std::atomic<bool> Go{false};

  auto Run = VM.asyncExecute("run");
  while (!Sync.PrepDone.load()) {
    std::this_thread::yield();
  }
  // Both structs are now past their newborn grace (prep's cycle has
  // completed, so the heap is Idle and the hook can be installed without it
  // firing for that cycle). Own the token, let the guest leave prep and queue
  // behind it in table.grow, then run the cycle.
  Alloc.setPreCycleHook([&] {
    Sync.TokenHeld.store(true);
    while (!Go.load()) {
      std::this_thread::yield();
    }
  });
  std::thread Collector([&] { EXPECT_TRUE(Ctrl.collect(true, false)); });
  while (Ctrl.debugExclusiveWaiters() == 0) {
    std::this_thread::yield();
  }
  Go.store(true);
  Collector.join();
  auto Res = Run.get();
  Alloc.setPreCycleHook(nullptr);

  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 2u);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), UINT32_C(1));
  EXPECT_EQ((*Res)[1].first.get<uint32_t>(), UINT32_C(1));
  // Both objects must have survived the collection the grow parked behind:
  // the operand below the grow (spilled at the grow site) and the init value
  // (pinned by the proxy). Nothing else was allocated after prep.
  EXPECT_EQ(Alloc.getMemoryUsage(), Sync.UsageAfterPrep.load())
      << "a ref live across the compiled table.grow was swept";
}

// memory.atomic.wait blocks this thread (Blocked via NativeScope); a collection
// that runs meanwhile scans it remotely -- shadow chain only -- so a ref live
// on the operand stack across the wait must be spilled at the wait site. The
// guest signals just before it blocks; the collector's handshake then either
// finds it already Blocked or waits until it enters the wait (entering a
// scannable state is what stands in for its ack), so the cycle's root scan
// deterministically covers the parked guest.
// (module (type $s (struct (field i32))) (import "host" "prep" (func))
//  (memory (export "mem") 1 1 shared)
//  (func (export "run") (result i32)
//    (struct.new_default $s) (call 0)
//    (i32.atomic.store (i32.const 4) (i32.const 1))
//    (drop (memory.atomic.wait32 (i32.const 0) (i32.const 0) (i64.const -1)))
//    (ref.is_null) (i32.eqz))
//  (func (export "wake") (drop (memory.atomic.notify (i32.const 0) (i32.const
//  1)))))
const std::array<WasmEdge::Byte, 127> WaitSpillWasm{
    0,   97,  115, 109, 1,   0,  0,   0,   1,   12,  3,  95,  1,   127, 0,
    96,  0,   0,   96,  0,   1,  127, 2,   13,  1,   4,  104, 111, 115, 116,
    4,   112, 114, 101, 112, 0,  1,   3,   3,   2,   2,  1,   5,   4,   1,
    3,   1,   1,   7,   20,  3,  3,   109, 101, 109, 2,  0,   3,   114, 117,
    110, 0,   1,   4,   119, 97, 107, 101, 0,   2,   10, 42,  2,   28,  0,
    251, 1,   0,   16,  0,   65, 4,   65,  1,   254, 23, 2,   0,   65,  0,
    65,  0,   66,  127, 254, 1,  2,   0,   26,  209, 69, 11,  11,  0,   65,
    0,   65,  1,   254, 0,   2,  0,   26,  11,  0,   11, 4,   110, 97,  109,
    101, 4,   4,   1,   0,   1,  115};

TEST(GCThread, ShadowSpillKeepsOperandStackRefAcrossCompiledAtomicWait) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::Threads);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  SpillPrepSync Sync;
  Sync.TokenHeld.store(true); // prep need not hold the guest here
  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("prep", std::make_unique<SpillPrepHost>(Sync));

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(WaitSpillWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  auto &Ctrl = VM.getController();
  Alloc.setManualGC(true);

  const auto *RunMod = VM.getActiveModule();
  ASSERT_NE(RunMod, nullptr);
  const auto *RunFn = RunMod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());
  auto *Mem = RunMod->findMemoryExports("mem");
  ASSERT_NE(Mem, nullptr);
  auto *Started =
      reinterpret_cast<std::atomic<uint32_t> *>(Mem->getDataPtr() + 4);

  auto Run = VM.asyncExecute("run");
  while (Started->load(std::memory_order_acquire) == 0) {
    std::this_thread::yield();
  }
  // Cycle 2: the guest is in (or entering) the wait; its operand-stack ref is
  // past its newborn grace and reachable only through the wait-site spill.
  EXPECT_TRUE(Ctrl.collect(true, false));
  ASSERT_TRUE(VM.execute("wake"));
  auto Res = Run.get();

  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), UINT32_C(1));
  EXPECT_GT(Alloc.getMemoryUsage(), 0u)
      << "the ref held on the compiled operand stack across atomic.wait was "
         "swept by the collection that ran while it blocked";
}

// Ref-param coverage: run passes a freshly-allocated struct directly to $hold
// (no local -- the value is only on run's operand stack, which is not spilled),
// and $hold holds it as a ref param across a GC. The struct survives only if
// the compiler spills ref params (not just locals). (module (type $s (struct
// (field i32))) (import "host" "collect" (func))
//  (func $hold (param (ref null $s)) (call 0))
//  (func (export "run") (call $hold (struct.new_default $s))))
const std::array<WasmEdge::Byte, 110> ParamSpillWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03,
    0x5f, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x01, 0x63, 0x00,
    0x00, 0x02, 0x10, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x07, 0x63,
    0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x00, 0x01, 0x03, 0x03, 0x02,
    0x02, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02,
    0x0a, 0x0e, 0x02, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x07, 0x00, 0xfb,
    0x01, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x25, 0x04, 0x6e, 0x61, 0x6d,
    0x65, 0x01, 0x10, 0x02, 0x00, 0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65,
    0x63, 0x74, 0x01, 0x04, 0x68, 0x6f, 0x6c, 0x64, 0x02, 0x06, 0x01,
    0x01, 0x01, 0x00, 0x01, 0x72, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

TEST(GCThread, ShadowSpillKeepsRefParamAcrossCompiledCall) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<ShadowCollectHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(ParamSpillWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  ShadowSpillUsageAfter.store(0, std::memory_order_relaxed);
  auto R = VM.execute("run");
  ASSERT_TRUE(R);
  // Reachable only via $hold's ref-param shadow spill during the 2nd collect.
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u);
}

// call_ref coverage: a ref local held across a call_ref to a host function
// (which routes through the slow kCallRef proxy -> NativeRunning). Survival
// proves the call_ref emission site spills, not just direct calls.
// (module (type $s (struct (field i32))) (type $ft (func))
//  (import "host" "collect" (func $collect (type $ft))) (elem declare func
//  $collect) (func (export "run") (local $r (ref null $s))
//    (local.set $r (struct.new_default $s)) (call_ref $ft (ref.func
//    $collect))))
const std::array<WasmEdge::Byte, 111> CallRefWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x10, 0x01, 0x04, 0x68, 0x6f,
    0x73, 0x74, 0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x00, 0x01,
    0x03, 0x02, 0x01, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00,
    0x01, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x10, 0x01, 0x0e,
    0x01, 0x01, 0x63, 0x00, 0xfb, 0x01, 0x00, 0x21, 0x00, 0xd2, 0x00, 0x14,
    0x01, 0x0b, 0x00, 0x23, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0a, 0x01,
    0x00, 0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x02, 0x06, 0x01,
    0x01, 0x01, 0x00, 0x01, 0x72, 0x04, 0x08, 0x02, 0x00, 0x01, 0x73, 0x01,
    0x02, 0x66, 0x74};

TEST(GCThread, ShadowSpillKeepsRefLocalAcrossCompiledCallRef) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<ShadowCollectHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(CallRefWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  ShadowSpillUsageAfter.store(0, std::memory_order_relaxed);
  auto R = VM.execute("run");
  ASSERT_TRUE(R);
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u);
}

// call_indirect coverage: a ref local held across a call_indirect to a host
// function (in a table). Survival proves the call_indirect emission site
// spills. (module (type $s (struct (field i32))) (type $ft (func))
//  (import "host" "collect" (func $collect (type $ft)))
//  (table 1 funcref) (elem (i32.const 0) func $collect)
//  (func (export "run") (local $r (ref null $s))
//    (local.set $r (struct.new_default $s))
//    (call_indirect (type $ft) (i32.const 0))))
const std::array<WasmEdge::Byte, 120> CallIndirectWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x10, 0x01, 0x04, 0x68, 0x6f,
    0x73, 0x74, 0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x00, 0x01,
    0x03, 0x02, 0x01, 0x01, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01, 0x07, 0x07,
    0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41,
    0x00, 0x0b, 0x01, 0x00, 0x0a, 0x11, 0x01, 0x0f, 0x01, 0x01, 0x63, 0x00,
    0xfb, 0x01, 0x00, 0x21, 0x00, 0x41, 0x00, 0x11, 0x01, 0x00, 0x0b, 0x00,
    0x23, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0a, 0x01, 0x00, 0x07, 0x63,
    0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x02, 0x06, 0x01, 0x01, 0x01, 0x00,
    0x01, 0x72, 0x04, 0x08, 0x02, 0x00, 0x01, 0x73, 0x01, 0x02, 0x66, 0x74};

TEST(GCThread, ShadowSpillKeepsRefLocalAcrossCompiledCallIndirect) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<ShadowCollectHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(CallIndirectWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  ShadowSpillUsageAfter.store(0, std::memory_order_relaxed);
  auto R = VM.execute("run");
  ASSERT_TRUE(R);
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u);
}

// Positive control for the JIT ThreadSanitizer instrumentation.
// (module (memory (export "mem") 1)
//  (func (export "spin") (param $n i32) (local $i i32)
//    (loop $l (i32.store (i32.const 0) (local.get $i))
//      (local.set $i (i32.add (local.get $i) (i32.const 1)))
//      (br_if $l (i32.lt_u (local.get $i) (local.get $n))))))
const std::array<WasmEdge::Byte, 98> SpinStoreWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x01, 0x7f, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01,
    0x00, 0x01, 0x07, 0x0e, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00,
    0x04, 0x73, 0x70, 0x69, 0x6e, 0x00, 0x00, 0x0a, 0x1e, 0x01, 0x1c,
    0x01, 0x01, 0x7f, 0x03, 0x40, 0x41, 0x00, 0x20, 0x01, 0x36, 0x02,
    0x00, 0x20, 0x01, 0x41, 0x01, 0x6a, 0x21, 0x01, 0x20, 0x01, 0x20,
    0x00, 0x49, 0x0d, 0x00, 0x0b, 0x0b, 0x00, 0x18, 0x04, 0x6e, 0x61,
    0x6d, 0x65, 0x02, 0x09, 0x01, 0x00, 0x02, 0x00, 0x01, 0x6e, 0x01,
    0x01, 0x69, 0x03, 0x06, 0x01, 0x00, 0x01, 0x00, 0x01, 0x6c};

// A compiled function writes wasm linear memory[0] in a loop (an instrumented
// JIT store) while a C++ thread reads the same byte (instrumented by
// -fsanitize=thread). This is an intentional data race, disabled so it never
// runs in normal suites. Run it from a -fsanitize=thread build with
// --gtest_also_run_disabled_tests: TSan must report a race, which is only
// possible if the JIT store itself is instrumented -- without JIT
// instrumentation TSan sees only the C++ read and reports nothing. This is the
// positive control proving the JIT ThreadSanitizer pass is effective, not
// silently inert.
TEST(GCThread, DISABLED_JITTsanRacePositiveControl) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(SpinStoreWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *SpinFn = Mod->findFuncExports("spin");
  ASSERT_NE(SpinFn, nullptr);
  ASSERT_TRUE(SpinFn->isCompiledFunction()); // the racy store must be JIT'd
  auto *Mem = Mod->findMemoryExports("mem");
  ASSERT_NE(Mem, nullptr);
  volatile uint8_t *Base = Mem->getDataPtr();

  std::atomic<bool> Stop{false};
  std::thread Reader([&] {
    while (!Stop.load(std::memory_order_relaxed)) {
      volatile uint8_t X = Base[0]; // racy read of mem[0]
      (void)X;
    }
  });
  auto R =
      VM.execute("spin", std::initializer_list<ValVariant>{UINT32_C(50000000)},
                 {ValType(TypeCode::I32)});
  Stop.store(true, std::memory_order_relaxed);
  Reader.join();
  EXPECT_TRUE(R);
}

// Multi-mutator protocol test for the codegen shadow spill.
// (module (type $s (struct (field i32))) (import "host" "tick" (func))
//  (func (export "run") (param $n i32) (local $i i32) (local $r (ref null $s))
//    (loop $l (local.set $r (struct.new_default $s)) (call 0)
//      (local.set $i (i32.add (local.get $i) (i32.const 1)))
//      (br_if $l (i32.lt_u (local.get $i) (local.get $n))))))
const std::array<WasmEdge::Byte, 129> MTShadowWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x0d,
    0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x04, 0x74, 0x69, 0x63, 0x6b, 0x00,
    0x01, 0x03, 0x02, 0x01, 0x02, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e,
    0x00, 0x01, 0x0a, 0x21, 0x01, 0x1f, 0x02, 0x01, 0x7f, 0x01, 0x63, 0x00,
    0x03, 0x40, 0xfb, 0x01, 0x00, 0x21, 0x02, 0x10, 0x00, 0x20, 0x01, 0x41,
    0x01, 0x6a, 0x21, 0x01, 0x20, 0x01, 0x20, 0x00, 0x49, 0x0d, 0x00, 0x0b,
    0x0b, 0x00, 0x2a, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07, 0x01, 0x00,
    0x04, 0x74, 0x69, 0x63, 0x6b, 0x02, 0x0c, 0x01, 0x01, 0x03, 0x00, 0x01,
    0x6e, 0x01, 0x01, 0x69, 0x02, 0x01, 0x72, 0x03, 0x06, 0x01, 0x01, 0x01,
    0x00, 0x01, 0x6c, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

class TickHost : public Runtime::HostFunction<TickHost> {
public:
  Expect<void> body(const Runtime::CallingFrame &) { return {}; }
};

// Mutator (this thread): JIT-compiled code allocating a struct and publishing a
// shadow frame each iteration, held across a host tick() (a NativeRunning
// window). Collector (spawned thread): repeatedly runs the STW collect,
// scanning the mutator's shadow chain while it is NativeRunning. Verifies the
// full protocol end-to-end; in a -fsanitize=thread build (which also
// instruments the JIT code) it also checks the codegen publish store and the
// scanner's read are properly synchronized (via RegistryMtx) -- expected clean.
TEST(GCThread, ShadowSpillProtocolUnderConcurrentCollect) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("tick", std::make_unique<TickHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(MTShadowWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction());

  std::atomic<bool> Done{false};
  std::thread Collector([&] {
    auto &Ctrl = VM.getController();
    while (!Done.load(std::memory_order_relaxed)) {
      Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    }
  });

  auto R = VM.execute("run", std::initializer_list<ValVariant>{UINT32_C(20000)},
                      {ValType(TypeCode::I32)});
  Done.store(true, std::memory_order_relaxed);
  Collector.join();
  EXPECT_TRUE(R);
}

// A GC cooperative safepoint poll at loop back-edges lets a
// concurrent collection's stop-the-world complete against a compute-only
// compiled loop. `run` signals started (mem[4]) then spins in a pure loop -- a
// single shared-memory atomic load, no host call, so the loop-back-edge poll is
// the only possible yield point -- until mem[0]!=0. The collector thread waits
// until the mutator is spinning, runs one STW collect (which completes only if
// the spinning mutator reaches a safepoint and acks), and only then sets mem[0]
// to release the loop. Without the poll this deadlocks: the collect waits for
// an ack that never comes, and the release is gated on the collect returning.
// With it, the mutator parks+acks, the collect completes, the loop is released,
// and run returns. Shared memory + atomic accesses keep the signals race-free
// (TSan-clean); the yield point is genuinely the codegen poll, not a host call.
// (module (memory (export "mem") 1 1 shared)
//  (func (export "run")
//    (i32.atomic.store (i32.const 4) (i32.const 1))   ;; signal started
//    (loop $l (br_if $l (i32.eqz (i32.atomic.load (i32.const 0)))))))
const std::array<WasmEdge::Byte, 80> ComputeSpinWasm{
    0,   97, 115, 109, 1,   0,  0,   0,   1,   4,  1,  96, 0,   0,   3,   2,
    1,   0,  5,   4,   1,   3,  1,   1,   7,   13, 2,  3,  109, 101, 109, 2,
    0,   3,  114, 117, 110, 0,  0,   10,  24,  1,  22, 0,  65,  4,   65,  1,
    254, 23, 2,   0,   3,   64, 65,  0,   254, 16, 2,  0,  69,  13,  0,   11,
    11,  0,  13,  4,   110, 97, 109, 101, 3,   6,  1,  0,  1,   0,   1,   108};

TEST(GCThread, CompiledLoopYieldsToConcurrentCollect) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::Threads);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(ComputeSpinWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction()); // the spinning loop must be JIT'd
  auto *Mem = Mod->findMemoryExports("mem");
  ASSERT_NE(Mem, nullptr);
  uint8_t *Base = Mem->getDataPtr();
  auto *Release = reinterpret_cast<std::atomic<uint32_t> *>(Base);
  auto *Started = reinterpret_cast<std::atomic<uint32_t> *>(Base + 4);

  std::thread Collector([&] {
    // Wait until the mutator is actually spinning in the compiled loop.
    while (Started->load(std::memory_order_acquire) == 0) {
      std::this_thread::yield();
    }
    // This STW collect can complete only if the spinning mutator reaches the
    // loop-back-edge safepoint and acks -- there is no host call to yield at.
    VM.getController().collect(/*Manual=*/true, /*ScanNative=*/false);
    // The collect returned, so the mutator yielded. Release the loop.
    Release->store(1, std::memory_order_release);
  });

  auto R = VM.execute("run"); // spins until Release != 0; returns only if freed
  Collector.join();
  EXPECT_TRUE(R);
}

// End-to-end survival under a genuinely remote collection. Unlike
// ShadowSpillKeepsRefLocalAcrossCompiledCall (which self-drives the collect
// from inside the host call, on the mutator's own thread) and unlike
// ShadowSpillProtocolUnderConcurrentCollect (a concurrent collector, but only a
// no-crash assertion), this runs the two STW collects on a separate collector
// thread while the JIT mutator is parked NativeRunning inside the host call,
// with its ref local live only in the codegen-published shadow frame -- then
// asserts the struct survived. This is the real multi-mutator proof: the remote
// scan (scanNonRunningRoots) must walk a compiled thread's shadow chain across
// a true thread boundary. Barrier-coordinated (park -> collect x2 -> release)
// so the collects provably straddle the spill window; deterministic, not
// timing-based.
namespace {
std::atomic<bool> RemoteMutatorParked{false};
std::atomic<bool> RemoteCollectDone{false};
std::atomic<uint64_t> RemoteShadowUsageAfter{0};
} // namespace

// Bound to ShadowSpillWasm's "collect" import: signals the collector that the
// mutator is parked (shadow frame already published around this call, entry now
// NativeRunning) and blocks until the collector finishes both cycles.
class RemoteParkHost : public Runtime::HostFunction<RemoteParkHost> {
public:
  Expect<void> body(const Runtime::CallingFrame &) {
    RemoteMutatorParked.store(true, std::memory_order_release);
    while (!RemoteCollectDone.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    return {};
  }
};

TEST(GCThread, ShadowSpillSurvivesConcurrentRemoteCollect) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  RemoteMutatorParked.store(false, std::memory_order_relaxed);
  RemoteCollectDone.store(false, std::memory_order_relaxed);
  RemoteShadowUsageAfter.store(0, std::memory_order_relaxed);

  // HostMod declared before the VM so it outlives the module that imports it.
  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<RemoteParkHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(ShadowSpillWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction()); // else this proves nothing

  std::thread Collector([&] {
    while (!RemoteMutatorParked.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    auto &Ctrl = VM.getController();
    // Born-gray keeps the struct through cycle 1 regardless; cycle 2 sweeps it
    // unless the remote scan of the parked mutator's shadow chain re-grayed it.
    // Both cycles run on this (separate) thread while the mutator stays parked.
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    RemoteShadowUsageAfter.store(
        VM.getExecutor().getAllocator().getMemoryUsage(),
        std::memory_order_relaxed);
    RemoteCollectDone.store(true, std::memory_order_release);
  });

  auto R = VM.execute("run");
  Collector.join();
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  // run returns i32.eqz(ref.is_null($r)) == 1: $r still non-null after the
  // call.
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1u);
  // The struct was reachable only via the codegen shadow spill during the 2nd
  // collect -- run on a separate thread while this mutator was parked
  // NativeRunning. Surviving usage proves the remote shadow scan works.
  EXPECT_GT(RemoteShadowUsageAfter.load(std::memory_order_relaxed), 0u);
}

// --- The consumed-argument window of a mediated call -------------------------
//
// (module
//  (type $s (struct (field i32)))
//  (type $f (func (param structref) (result i32)))
//  (import "host" "park" (func $park (type $f)))
//  (table 1 funcref) (elem (i32.const 0) func $park)
//  (func (export "run") (result i32)
//    (call_indirect (type $f) (struct.new $s (i32.const 42)) (i32.const 0))))
//
// The struct is allocated and immediately consumed as the call_indirect
// argument: it never occupies a local (so no shadow-frame slot covers it) and
// it reaches no value stack until the proxy pushes it. The callee is a host
// function, so proxyTableGetFuncSymbol returns nullptr on its cross-module gate
// and codegen takes the mediated IsNullBB path -- the window where the only
// copy of the ref lives in the packed native argument buffer.
const std::array<WasmEdge::Byte, 108> ConsumedArgWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   14,  3, 95,  1,   127,
    0,   96,  1,   107, 1,   127, 96,  0,   1,   127, 2, 13,  1,   4,
    104, 111, 115, 116, 4,   112, 97,  114, 107, 0,   1, 3,   2,   1,
    2,   4,   4,   1,   112, 0,   1,   7,   7,   1,   3, 114, 117, 110,
    0,   1,   9,   7,   1,   0,   65,  0,   11,  1,   0, 10,  14,  1,
    12,  0,   65,  42,  251, 0,   0,   65,  0,   17,  1, 0,   11,  0,
    23,  4,   110, 97,  109, 101, 1,   7,   1,   0,   4, 112, 97,  114,
    107, 4,   7,   2,   0,   1,   115, 1,   1,   102};

namespace {
std::atomic<bool> WindowEntered{false};
std::atomic<bool> WindowCollectorReady{false};
std::atomic<bool> WindowCollectFinished{false};
std::atomic<bool> WindowScanCompletedInsideWindow{false};
std::atomic<uint64_t> WindowUsageAfter{0};
} // namespace

// The mediated callee. HostFunction<T> cannot express a ref-typed parameter
// (ValTypeFromType covers numerics only), so the signature is built directly.
class ConsumedArgParkHost : public Runtime::HostFunctionBase {
public:
  ConsumedArgParkHost() : Runtime::HostFunctionBase(0) {
    auto &FT = DefType.getCompositeType().getFuncType();
    FT.getParamTypes().push_back(ValType(TypeCode::StructRef));
    FT.getReturnTypes().push_back(ValType(TypeCode::I32));
  }
  Expect<void> run(const Runtime::CallingFrame &CF, Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    // Reached only after the proxy pushed the argument onto the GC-rooted value
    // stack, so the window is already over here. Park (entry NativeRunning, and
    // therefore scanned remotely rather than waited on) until the collector has
    // run both cycles, then report what it left behind.
    while (!WindowCollectFinished.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    WindowUsageAfter.store(CF.getExecutor()->getAllocator().getMemoryUsage(),
                           std::memory_order_relaxed);
    Rets[0].emplace<uint32_t>(Args[0].get<RefVariant>().isNull() ? 0U : 1U);
    return {};
  }
};

// Can a ref consumed as a call argument -- popped off the compiler's operand
// stack into an SSA temporary, then packed into an untyped native buffer -- be
// missed by a collection before the proxy roots it?
//
// It cannot, and this pins down why: root scanning is handshake-gated. A thread
// in the window is Running (nothing on the stretch enters a NativeScope or
// setSelfBlocked), and Controller::waitForAcks blocks on every Running entry,
// so no remote root scan can complete while a mutator sits in the window. The
// test drives exactly that: a collector thread opens STW #1 while the mutator
// is held inside the window, and the mutator observes that the collection has
// not finished. Were the window ever made non-Running -- the regression this
// guards
// -- the scan would complete without covering the packed argument.
//
// checkLazyCompilation is the only runtime hook that runs inside the window
// (proxyCallIndirect calls it after resolving the callee and before
// StackMgr.push roots the arguments), which is what makes the timing exact.
TEST(GCThread, ConsumedArgWindowBlocksRemoteRootScan) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  WindowEntered.store(false, std::memory_order_relaxed);
  WindowCollectorReady.store(false, std::memory_order_relaxed);
  WindowCollectFinished.store(false, std::memory_order_relaxed);
  WindowScanCompletedInsideWindow.store(false, std::memory_order_relaxed);
  WindowUsageAfter.store(0, std::memory_order_relaxed);

  // HostMod declared before the VM so it outlives the module that imports it.
  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("park", std::make_unique<ConsumedArgParkHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(ConsumedArgWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  ASSERT_TRUE(RunFn->isCompiledFunction()); // else this proves nothing

  // Fires inside the window. Note proxyTableGetFuncSymbol returns nullptr on
  // its cross-module check before its own checkLazyCompilation, so this runs
  // exactly once, from proxyCallIndirect.
  VM.getExecutor().registerLazyCompilationCallback(
      [](const Runtime::Instance::FunctionInstance *) -> Expect<void> {
        WindowEntered.store(true, std::memory_order_release);
        while (!WindowCollectorReady.load(std::memory_order_acquire)) {
          std::this_thread::yield();
        }
        // The collector is now inside collect() with STW #1's stop flag raised.
        // This thread is Running and has not acked, so waitForAcks must still
        // be spinning. Waiting longer only makes the check stricter: a false
        // failure needs the collection to genuinely complete, which is the bug.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        WindowScanCompletedInsideWindow.store(
            WindowCollectFinished.load(std::memory_order_acquire),
            std::memory_order_relaxed);
        return {};
      });

  std::thread Collector([&] {
    while (!WindowEntered.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    auto &Ctrl = VM.getController();
    WindowCollectorReady.store(true, std::memory_order_release);
    // Born-gray carries the struct through cycle 1 regardless; cycle 2 sweeps
    // it unless a scan genuinely re-grayed it. ScanNative=false so this
    // thread's own conservative scan cannot mask a missing root on the
    // mutator's side.
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    WindowCollectFinished.store(true, std::memory_order_release);
  });

  auto R = VM.execute("run");
  Collector.join();
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  // The load-bearing assertion: the collection did not get through its root
  // scan while the mutator held a managed ref only in the packed argument
  // buffer.
  EXPECT_FALSE(WindowScanCompletedInsideWindow.load(std::memory_order_relaxed));
  // The argument arrived as a live, non-null ref and outlived both cycles.
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1u);
  EXPECT_GT(WindowUsageAfter.load(std::memory_order_relaxed), 0u);
}

// Per-call shadow-spill overhead microbenchmark (disabled; run explicitly with
// --gtest_also_run_disabled_tests). loop_ref has a funcref local live across a
// trivial $sink call in a tight loop, so codegen publishes+pops a shadow frame
// each iteration; loop_noref is identical but with an i32 local (no ref -> no
// spill). $sink stores to memory (an unavoidable side effect) so neither loop
// is elided and $sink is not a pure no-op. The within-build (ref - noref) delta
// is an estimate; the true isolation is this build (spill on) vs a build with
// FunctionCompiler::pushShadowFrame forced to early-return (spill off) run on
// the same loop_ref. Reports ns/call. (module
// (memory 1)
//  (func $sink (i32.store (i32.const 0) (i32.add (i32.load (i32.const 0))
//  (i32.const 1)))) (func (export "loop_ref")   (param $n i32) (local $i i32)
//  (local $r funcref) <loop calling $sink $n times>) (func (export
//  "loop_noref") (param $n i32) (local $i i32) (local $x i32)     <same loop>))
const std::array<WasmEdge::Byte, 179> BenchWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
    0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x03, 0x04, 0x03, 0x00, 0x01, 0x01,
    0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x19, 0x02, 0x08, 0x6c, 0x6f, 0x6f,
    0x70, 0x5f, 0x72, 0x65, 0x66, 0x00, 0x01, 0x0a, 0x6c, 0x6f, 0x6f, 0x70,
    0x5f, 0x6e, 0x6f, 0x72, 0x65, 0x66, 0x00, 0x02, 0x0a, 0x43, 0x03, 0x0f,
    0x00, 0x41, 0x00, 0x41, 0x00, 0x28, 0x02, 0x00, 0x41, 0x01, 0x6a, 0x36,
    0x02, 0x00, 0x0b, 0x19, 0x02, 0x01, 0x7f, 0x01, 0x70, 0x03, 0x40, 0x10,
    0x00, 0x20, 0x01, 0x41, 0x01, 0x6a, 0x21, 0x01, 0x20, 0x01, 0x20, 0x00,
    0x49, 0x0d, 0x00, 0x0b, 0x0b, 0x17, 0x01, 0x02, 0x7f, 0x03, 0x40, 0x10,
    0x00, 0x20, 0x01, 0x41, 0x01, 0x6a, 0x21, 0x01, 0x20, 0x01, 0x20, 0x00,
    0x49, 0x0d, 0x00, 0x0b, 0x0b, 0x00, 0x34, 0x04, 0x6e, 0x61, 0x6d, 0x65,
    0x01, 0x07, 0x01, 0x00, 0x04, 0x73, 0x69, 0x6e, 0x6b, 0x02, 0x17, 0x02,
    0x01, 0x03, 0x00, 0x01, 0x6e, 0x01, 0x01, 0x69, 0x02, 0x01, 0x72, 0x02,
    0x03, 0x00, 0x01, 0x6e, 0x01, 0x01, 0x69, 0x02, 0x01, 0x78, 0x03, 0x0b,
    0x02, 0x01, 0x01, 0x00, 0x01, 0x6c, 0x02, 0x01, 0x00, 0x01, 0x6c};

TEST(GCThread, DISABLED_ShadowSpillOverheadBench) {
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(BenchWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RefFn = Mod->findFuncExports("loop_ref");
  const auto *NoRefFn = Mod->findFuncExports("loop_noref");
  ASSERT_NE(RefFn, nullptr);
  ASSERT_NE(NoRefFn, nullptr);
  ASSERT_TRUE(RefFn->isCompiledFunction());
  ASSERT_TRUE(NoRefFn->isCompiledFunction());

  const uint32_t N = 500000000u; // 5e8 calls per rep
  auto Bench = [&](const char *Fn) -> double {
    (void)VM.execute(Fn, std::initializer_list<ValVariant>{UINT32_C(1000000)},
                     {ValType(TypeCode::I32)}); // warm up
    double Best = 1e30;
    for (int Rep = 0; Rep < 5; ++Rep) {
      auto T0 = std::chrono::steady_clock::now();
      auto R = VM.execute(Fn, std::initializer_list<ValVariant>{N},
                          {ValType(TypeCode::I32)});
      auto T1 = std::chrono::steady_clock::now();
      EXPECT_TRUE(R);
      double Per =
          std::chrono::duration<double, std::nano>(T1 - T0).count() / N;
      if (Per < Best) {
        Best = Per;
      }
    }
    return Best;
  };

  double RefPer = Bench("loop_ref");
  double NoRefPer = Bench("loop_noref");
  std::printf("[BENCH] N=%u  loop_ref=%.4f ns/call  loop_noref=%.4f ns/call  "
              "spill(ref-noref)=%.4f ns/call\n",
              N, RefPer, NoRefPer, RefPer - NoRefPer);
}

// A real guard-page SIGSEGV inside JIT-compiled code, with a real
// codegen-published shadow frame live for a ref local held across the faulting
// call, must drive emitFault's signal-safe head truncation and leave the
// runtime consistent. $boom does a wasm OOB i32.store -- a hardware trap in JIT
// code (which is not sanitizer-instrumented, so the SIGSEGV reaches WasmEdge's
// own Fault handler, not ASan's). run holds $r across the call to $boom, so
// run's codegen publishes $r's shadow frame around it. Unlike the Fault* unit
// tests (hand-published frame), this exercises the real path end-to-end:
// helper.cpp arming + real codegen publish + real trap + emitFault truncation +
// longjmp recovery. Asserts the trap surfaces as MemoryOutOfBounds (no process
// crash) and the VM is reusable afterward (a clean function still runs),
// proving the unwind left arming/handler state balanced.
// (module (type $s (struct (field i32))) (import "host" "collect" (func))
//  (memory 1)
//  (func $boom (i32.store (i32.const 0xFFFF0000) (i32.const 1)))
//  (func (export "run") (result i32) (local $r (ref null $s))
//    (local.set $r (struct.new_default $s)) (call 0) (call $boom)
//    (i32.eqz (ref.is_null (local.get $r))))
//  (func (export "clean") (result i32) (i32.const 42)))
const std::array<WasmEdge::Byte, 146> CompiledTrapWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x02, 0x10,
    0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x07, 0x63, 0x6f, 0x6c, 0x6c, 0x65,
    0x63, 0x74, 0x00, 0x01, 0x03, 0x04, 0x03, 0x01, 0x02, 0x02, 0x05, 0x03,
    0x01, 0x00, 0x01, 0x07, 0x0f, 0x02, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02,
    0x05, 0x63, 0x6c, 0x65, 0x61, 0x6e, 0x00, 0x03, 0x0a, 0x25, 0x03, 0x0b,
    0x00, 0x41, 0x80, 0x80, 0x7c, 0x41, 0x01, 0x36, 0x02, 0x00, 0x0b, 0x12,
    0x01, 0x01, 0x63, 0x00, 0xfb, 0x01, 0x00, 0x21, 0x00, 0x10, 0x00, 0x10,
    0x01, 0x20, 0x00, 0xd1, 0x45, 0x0b, 0x04, 0x00, 0x41, 0x2a, 0x0b, 0x00,
    0x25, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x10, 0x02, 0x00, 0x07, 0x63,
    0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x01, 0x04, 0x62, 0x6f, 0x6f, 0x6d,
    0x02, 0x06, 0x01, 0x02, 0x01, 0x00, 0x01, 0x72, 0x04, 0x04, 0x01, 0x00,
    0x01, 0x73};

TEST(GCThread, RealCompiledTrapRunsTruncationAndSurvives) {
  SKIP_WITHOUT_COMPILER();
  if (!compiledFramesUnwindable()) {
    GTEST_SKIP() << "compiled frames are not unwindable on this platform";
  }
  Configure Conf = makeGCConf();
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("collect", std::make_unique<ShadowCollectHost>());

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(CompiledTrapWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

  const auto *Mod = VM.getActiveModule();
  ASSERT_NE(Mod, nullptr);
  const auto *RunFn = Mod->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  // The trap must fire in JIT code; a silent interpreter fallback would prove
  // nothing about the compiled fault path.
  ASSERT_TRUE(RunFn->isCompiledFunction());

  // The compiled OOB store traps: it must surface as MemoryOutOfBounds via the
  // Fault handler + longjmp, not crash the process.
  auto Res = VM.execute("run");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::MemoryOutOfBounds);

  // The runtime survived the longjmp + head truncation and is reusable: the
  // Fault handler count and shadow chain were left balanced.
  auto Clean = VM.execute("clean");
  ASSERT_TRUE(Clean);
  ASSERT_EQ(Clean->size(), 1u);
  EXPECT_EQ((*Clean)[0].first.get<uint32_t>(), 42u);
}

} // namespace
