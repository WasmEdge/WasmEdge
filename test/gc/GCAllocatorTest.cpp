// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCAllocatorTest.cpp - GC allocator tests ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for the GC allocator and the collector:
/// allocation under heap pressure, write barriers, marking, sweeping
/// and tracer termination.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace WasmEdge {
namespace GC {
// Test-only bridge to the private Allocator::markGrayRoot, befriended in
// allocator.h. Lets GC.RootShadeQuietDuringSweeping drive the root-scan shading
// path directly; no public API reaches markGray() (writeBarrier() gates itself
// on the phase, so it cannot exercise the gate inside markGray()).
struct RootShadeTestSeam {
  static void shade(Allocator &A, const ValVariant &V) noexcept {
    A.markGrayRoot(V);
  }
};
// Test-only bridge to the private Allocator::allocateUnderPressure, befriended
// in allocator.h. allocate() enters it only after its own reservation was
// rejected, so "room already on the heap when the slow path starts" (what a
// concurrent cycle produces) is reachable from a test only by calling it
// directly.
struct PressureTestSeam {
  static uint8_t *allocateUnderPressure(Allocator &A, uint32_t N) noexcept {
    return A.allocateUnderPressure(N);
  }
  // The raw reservation never went through allocate()'s registration, so the
  // allocator's teardown does not know it: hand it back the same way.
  static void release(Allocator &A, uint8_t *P, uint32_t N) noexcept {
    A.doDeallocate(P, N);
  }
};
} // namespace GC
} // namespace WasmEdge

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

// --- WASM modules (compiled from WAT with wasm-tools) ---

// Test 1: Struct allocation and GC
// Allocates a struct, verifies memory usage, drops it, verifies collection
const std::array<WasmEdge::Byte, 114> StructGCWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02, 0x5f,
    0x02, 0x7f, 0x00, 0x7e, 0x00, 0x60, 0x00, 0x00, 0x02, 0x14, 0x02, 0x02,
    0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x01, 0x02, 0x67, 0x63,
    0x03, 0x72, 0x65, 0x63, 0x00, 0x01, 0x03, 0x02, 0x01, 0x01, 0x07, 0x08,
    0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x02, 0x0a, 0x19, 0x01, 0x17,
    0x00, 0x10, 0x01, 0x41, 0x2a, 0x42, 0xe4, 0x00, 0xfb, 0x00, 0x00, 0x10,
    0x01, 0x10, 0x00, 0x10, 0x01, 0x1a, 0x10, 0x00, 0x10, 0x01, 0x0b, 0x00,
    0x1d, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0c, 0x02, 0x00, 0x04, 0x63,
    0x6f, 0x6c, 0x6c, 0x01, 0x03, 0x72, 0x65, 0x63, 0x04, 0x08, 0x02, 0x00,
    0x01, 0x73, 0x01, 0x02, 0x66, 0x6e};

// Test 2: Nested references - outer struct contains ref to inner struct.
// Sequence: rec; (alloc inner+outer); rec; coll; coll; rec; drop; coll; rec.
// Exercises heap->heap child-edge tracing (see the assertion below).
const std::array<WasmEdge::Byte, 130> NestedRefWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x5f,
    0x01, 0x7f, 0x00, 0x5f, 0x01, 0x64, 0x00, 0x00, 0x60, 0x00, 0x00, 0x02,
    0x14, 0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x02,
    0x02, 0x67, 0x63, 0x03, 0x72, 0x65, 0x63, 0x00, 0x02, 0x03, 0x02, 0x01,
    0x02, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x02, 0x0a,
    0x1b, 0x01, 0x19, 0x00, 0x10, 0x01, 0x41, 0x2a, 0xfb, 0x00, 0x00, 0xfb,
    0x00, 0x01, 0x10, 0x01, 0x10, 0x00, 0x10, 0x00, 0x10, 0x01, 0x1a, 0x10,
    0x00, 0x10, 0x01, 0x0b, 0x00, 0x28, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01,
    0x0c, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x01, 0x03, 0x72, 0x65,
    0x63, 0x04, 0x13, 0x03, 0x00, 0x05, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x01,
    0x05, 0x6f, 0x75, 0x74, 0x65, 0x72, 0x02, 0x02, 0x66, 0x6e};

// Test 3: Data survives GC - struct.get/set correctness across GC cycles
const std::array<WasmEdge::Byte, 156> DataSurvivesGCWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5f,
    0x01, 0x7f, 0x01, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x16,
    0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x02, 0x03, 0x02,
    0x01, 0x01, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x02,
    0x0a, 0x2b, 0x01, 0x29, 0x01, 0x01, 0x64, 0x00, 0x41, 0x2a, 0xfb, 0x00,
    0x00, 0x21, 0x00, 0x10, 0x00, 0x20, 0x00, 0xfb, 0x02, 0x00, 0x00, 0x10,
    0x01, 0x20, 0x00, 0x41, 0xe3, 0x00, 0xfb, 0x05, 0x00, 0x00, 0x10, 0x00,
    0x20, 0x00, 0xfb, 0x02, 0x00, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x31, 0x04,
    0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0e, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c,
    0x6c, 0x01, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x02, 0x08, 0x01, 0x02,
    0x01, 0x00, 0x03, 0x72, 0x65, 0x66, 0x04, 0x10, 0x03, 0x00, 0x01, 0x73,
    0x01, 0x02, 0x66, 0x6e, 0x02, 0x06, 0x66, 0x6e, 0x5f, 0x69, 0x33, 0x32};

// Test 4: Array operations - array.new, get, set, len with GC interleaved
const std::array<WasmEdge::Byte, 189> ArrayOpsWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x03, 0x5e,
    0x7f, 0x01, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x1f, 0x03,
    0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x01, 0x02, 0x67,
    0x63, 0x03, 0x72, 0x65, 0x63, 0x00, 0x01, 0x02, 0x67, 0x63, 0x05, 0x63,
    0x68, 0x65, 0x63, 0x6b, 0x00, 0x02, 0x03, 0x02, 0x01, 0x01, 0x07, 0x08,
    0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x03, 0x0a, 0x3f, 0x01, 0x3d,
    0x01, 0x01, 0x64, 0x00, 0x10, 0x01, 0x41, 0x07, 0x41, 0x0a, 0xfb, 0x06,
    0x00, 0x21, 0x00, 0x10, 0x01, 0x20, 0x00, 0xfb, 0x0f, 0x10, 0x02, 0x20,
    0x00, 0x41, 0x03, 0x41, 0x2a, 0xfb, 0x0e, 0x00, 0x10, 0x00, 0x20, 0x00,
    0x41, 0x00, 0xfb, 0x0b, 0x00, 0x10, 0x02, 0x20, 0x00, 0x41, 0x03, 0xfb,
    0x0b, 0x00, 0x10, 0x02, 0x20, 0x00, 0xfb, 0x0f, 0x10, 0x02, 0x10, 0x01,
    0x0b, 0x00, 0x36, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x13, 0x03, 0x00,
    0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x01, 0x03, 0x72, 0x65, 0x63, 0x02, 0x05,
    0x63, 0x68, 0x65, 0x63, 0x6b, 0x02, 0x06, 0x01, 0x03, 0x01, 0x00, 0x01,
    0x61, 0x04, 0x12, 0x03, 0x00, 0x03, 0x61, 0x72, 0x72, 0x01, 0x02, 0x66,
    0x6e, 0x02, 0x06, 0x66, 0x6e, 0x5f, 0x69, 0x33, 0x32};

// Test 5: Large allocation pressure - 500 arrays of size 64, dropped
// immediately
const std::array<WasmEdge::Byte, 164> AllocPressureWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x02, 0x5e,
    0x7f, 0x01, 0x60, 0x00, 0x00, 0x02, 0x14, 0x02, 0x02, 0x67, 0x63, 0x04,
    0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x01, 0x02, 0x67, 0x63, 0x03, 0x72, 0x65,
    0x63, 0x00, 0x01, 0x03, 0x02, 0x01, 0x01, 0x07, 0x08, 0x01, 0x04, 0x74,
    0x65, 0x73, 0x74, 0x00, 0x02, 0x0a, 0x32, 0x01, 0x30, 0x01, 0x01, 0x7f,
    0x10, 0x01, 0x41, 0x00, 0x21, 0x00, 0x02, 0x40, 0x03, 0x40, 0x20, 0x00,
    0x41, 0xf4, 0x03, 0x4f, 0x0d, 0x01, 0x41, 0x00, 0x41, 0xc0, 0x00, 0xfb,
    0x06, 0x00, 0x1a, 0x20, 0x00, 0x41, 0x01, 0x6a, 0x21, 0x00, 0x0c, 0x00,
    0x0b, 0x0b, 0x10, 0x01, 0x10, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x39, 0x04,
    0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0c, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c,
    0x6c, 0x01, 0x03, 0x72, 0x65, 0x63, 0x02, 0x06, 0x01, 0x02, 0x01, 0x00,
    0x01, 0x69, 0x03, 0x10, 0x01, 0x02, 0x02, 0x00, 0x05, 0x62, 0x72, 0x65,
    0x61, 0x6b, 0x01, 0x04, 0x6c, 0x6f, 0x6f, 0x70, 0x04, 0x0a, 0x02, 0x00,
    0x03, 0x61, 0x72, 0x72, 0x01, 0x02, 0x66, 0x6e};

// Reaching the heap limit must not trap while reclaimable garbage exists: the
// allocator's only automatic trigger is a 1-second schedule, so a guest that
// allocates and drops more than the limit within that second must not hit
// GCAllocationFailed with a live set of almost nothing. On a limit rejection
// the allocator runs a pressure collection and retries. Newborn objects
// survive their first cycle (born gray), so the pressure path may need two
// cycles -- the test's garbage is all newborn.
TEST(GC, AllocationAtHeapLimitCollectsAndRetries) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  GC::Allocator &Alloc = Exec.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Exec.getController().registerStack(S);

  auto AllocOne = [&]() noexcept -> void * {
    return Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
  };
  // Fill the heap exactly to a limit of four objects with unrooted garbage.
  // Done in a separate frame so no stale pointer to the garbage sits in a
  // frame the pressure collection's conservative native scan walks.
  const uint64_t One =
                     [&] {
                       EXPECT_NE(AllocOne(), nullptr);
                       return Alloc.getMemoryUsage();
                     }(),
                 Limit = 4 * One;
  ASSERT_GT(One, 0u);
  Alloc.setHeapLimit(Limit);
  [&] {
    for (int I = 0; I < 3; ++I) {
      ASSERT_NE(AllocOne(), nullptr);
    }
  }();
  ASSERT_EQ(Alloc.getMemoryUsage(), Limit);

  // The fifth allocation exceeds the limit: it must succeed by collecting the
  // (unrooted) garbage first rather than fail.
  void *P = AllocOne();
  EXPECT_NE(P, nullptr) << "allocation at the heap limit must collect garbage "
                           "and retry, not fail";
  EXPECT_LE(Alloc.getMemoryUsage(), Limit);
  EXPECT_LT(Alloc.getMemoryUsage(), Limit)
      << "the pressure collection reclaimed nothing";
}

// allocateUnderPressure short-circuits when it sees room on the heap: another
// thread's cycle made some, or the system allocator failed. The reservation
// CAS that follows still races every other mutator, so the room can be gone
// again by the time it runs. A lost race there is a limit rejection like any
// other and must be answered with a collection, not with the trap the slow
// path exists to avoid.
TEST(GC, PressureRetryLostRaceCollects) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  GC::Allocator &Alloc = Exec.getAllocator();

  auto AllocOne = [&]() noexcept -> void * {
    return Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
  };
  // Three objects of unrooted garbage, one slot of room left at the limit.
  // Allocated in a separate frame so no stale pointer sits where the pressure
  // cycle's conservative native scan would find it.
  const uint64_t One =
                     [&] {
                       EXPECT_NE(AllocOne(), nullptr);
                       return Alloc.getMemoryUsage();
                     }(),
                 Limit = 4 * One;
  ASSERT_GT(One, 0u);
  [&] {
    for (int I = 0; I < 2; ++I) {
      ASSERT_NE(AllocOne(), nullptr);
    }
  }();
  ASSERT_EQ(Alloc.getMemoryUsage(), 3 * One);
  Alloc.setHeapLimit(Limit);

  // The moment the slow path has seen the room and is about to reserve it,
  // another allocation takes it (a fourth unrooted object).
  bool Raced = false;
  Alloc.setPressureRetryHook([&]() noexcept {
    if (!Raced) {
      Raced = true;
      [&] { EXPECT_NE(AllocOne(), nullptr); }();
    }
  });
  // Header + payload, exactly what allocate() would ask for one object.
  const uint32_t Total = static_cast<uint32_t>(One);
  uint8_t *P = GC::PressureTestSeam::allocateUnderPressure(Alloc, Total);
  Alloc.setPressureRetryHook(nullptr);
  ASSERT_TRUE(Raced);
  EXPECT_NE(P, nullptr) << "a lost reservation race at the heap limit was "
                           "reported as allocation failure";
  // The three old objects were reclaimable: the retry collected them.
  EXPECT_LT(Alloc.getMemoryUsage(), Limit)
      << "the lost race did not lead to a collection";
  if (P != nullptr) {
    GC::PressureTestSeam::release(Alloc, P, Total);
  }
}

// The pressure collection takes the exclusive token non-blocking. Losing it
// to an in-flight collection is fine (waiting for the heap to go Idle is as
// good as a cycle), but losing it to a grower or a host reader window is not:
// the heap is Idle the whole time, so the wait returns at once and the
// allocation fails with reclaimable garbage still on the heap. The pressure
// path must keep polling (acking whatever handshake the holder raises) until
// it can run a cycle of its own.
//
// Shape 1: a grower owns the token and is parked in its stop-the-world
// handshake waiting for this thread's ack, so the poll loop must also be the
// ack (the reviewed code never reached a safepoint on this path).
TEST(GCThread, PressureCollectionWaitsForGrower) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  GC::Controller &Ctrl = Exec.getController();
  GC::Allocator &Alloc = Exec.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);

  auto AllocOne = [&]() noexcept -> void * {
    return Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
  };
  const uint64_t One =
                     [&] {
                       EXPECT_NE(AllocOne(), nullptr);
                       return Alloc.getMemoryUsage();
                     }(),
                 Limit = 4 * One;
  ASSERT_GT(One, 0u);
  Alloc.setHeapLimit(Limit);
  [&] {
    for (int I = 0; I < 3; ++I) {
      ASSERT_NE(AllocOne(), nullptr);
    }
  }();
  ASSERT_EQ(Alloc.getMemoryUsage(), Limit);

  std::atomic<bool> TestDone{false};
  std::thread Watchdog([&]() {
    const auto WD = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!TestDone.load(std::memory_order_acquire)) {
      if (std::chrono::steady_clock::now() > WD) {
        ADD_FAILURE() << "pressure collection vs grower deadlocked";
        std::fflush(stderr);
        std::_Exit(2);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  AST::TableType TType(ValType(TypeCode::RefNull, TypeCode::StructRef), 0, 10);
  Runtime::Instance::TableInstance Table(TType, true);
  ASSERT_TRUE(Table.setAllocator(Alloc));
  std::thread Grower([&]() {
    std::vector<ValVariant> GS;
    auto GReg = Ctrl.registerStack(GS);
    EXPECT_TRUE(Table.growTable(
        1, RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef))));
  });
  // The grower owns the token once its handshake is up; it now waits for our
  // ack, which we withhold until the allocation below has to deal with it.
  while (!Ctrl.stopRequested()) {
    std::this_thread::yield();
  }

  void *P = AllocOne();
  EXPECT_NE(P, nullptr) << "allocation at the heap limit gave up while a "
                           "grower held the exclusive token";
  EXPECT_LT(Alloc.getMemoryUsage(), Limit)
      << "the pressure collection reclaimed nothing";
  // If the allocation gave up without acking, let the grower finish.
  Ctrl.gcSafepoint();
  Grower.join();
  EXPECT_EQ(Table.getSize(), 1u);
  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
}

// Shape 2: an unregistered host thread holds the token with no handshake at
// all (the retained-getter window), so nothing but time frees it. The
// allocation must wait it out instead of failing.
TEST(GCThread, PressureCollectionWaitsForTokenHolder) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  GC::Controller &Ctrl = Exec.getController();
  GC::Allocator &Alloc = Exec.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);

  auto AllocOne = [&]() noexcept -> void * {
    return Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
  };
  const uint64_t One =
                     [&] {
                       EXPECT_NE(AllocOne(), nullptr);
                       return Alloc.getMemoryUsage();
                     }(),
                 Limit = 4 * One;
  ASSERT_GT(One, 0u);
  Alloc.setHeapLimit(Limit);
  [&] {
    for (int I = 0; I < 3; ++I) {
      ASSERT_NE(AllocOne(), nullptr);
    }
  }();
  ASSERT_EQ(Alloc.getMemoryUsage(), Limit);

  std::atomic<bool> Held{false};
  std::atomic<bool> Release{false};
  std::thread Holder([&]() {
    uint64_t Gen = 0;
    const bool Won = Ctrl.beginExclusiveOp(
        GC::Controller::ExclusiveState::OwnedGrowing, Gen);
    EXPECT_TRUE(Won);
    Held.store(true, std::memory_order_release);
    while (!Release.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    if (Won) {
      Ctrl.endExclusiveOp(Gen, GC::Controller::ExclusiveState::OwnedGrowing);
    }
  });
  while (!Held.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  // Free the token only after the allocation has had time to hit the
  // contention; the reviewed code returned nullptr inside that window.
  std::thread Timer([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Release.store(true, std::memory_order_release);
  });

  void *P = AllocOne();
  EXPECT_NE(P, nullptr) << "allocation at the heap limit gave up while a host "
                           "thread held the exclusive token";
  EXPECT_LT(Alloc.getMemoryUsage(), Limit)
      << "the pressure collection reclaimed nothing";
  Timer.join();
  Holder.join();
}

// ==========================================================================
// Single-threaded GC tests
// ==========================================================================

TEST(GC, StructAllocAndCollect) {
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, GCMod, StructGCWasm));
  ASSERT_TRUE(VM.execute("test"));
  auto Log = GCMod.getLog();

  ASSERT_EQ(Log.size(), 4);
  EXPECT_EQ(Log[0], 0); // before allocation
  EXPECT_GT(Log[1], 0); // after struct.new
  EXPECT_GT(Log[2], 0); // survives: born-gray makes the first cycle keep it
  EXPECT_EQ(Log[3], 0); // after drop + GC, collected
}

TEST(GC, NestedReferences) {
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, GCMod, NestedRefWasm));
  ASSERT_TRUE(VM.execute("test"));
  auto Log = GCMod.getLog();

  ASSERT_EQ(Log.size(), 4);
  EXPECT_EQ(Log[0], 0); // before allocation
  EXPECT_GT(Log[1], 0); // both inner and outer allocated
  // Two collections, outer still on stack: first keeps both (born-gray); second
  // reclaims the inner unless the collector follows outer's child edge. Equal
  // usage proves heap->heap tracing keeps the inner (reachable only via outer).
  EXPECT_EQ(Log[2], Log[1]);
  EXPECT_EQ(Log[3], 0); // after drop + GC, both collected
}

TEST(GC, DataSurvivesGC) {
  Configure Conf = makeGCConf();

  // Host modules must outlive the VM that terminates them; declare first.
  auto GCMod = std::make_unique<Runtime::Instance::ModuleInstance>("gc");
  GCMod->addHostFunc("coll", std::make_unique<Collect>());
  auto CP = std::make_unique<Check>();
  auto *C = CP.get();
  GCMod->addHostFunc("check", std::move(CP));

  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, *GCMod, DataSurvivesGCWasm));
  ASSERT_TRUE(VM.execute("test"));
  auto Values = C->getValues();

  ASSERT_EQ(Values.size(), 2);
  EXPECT_EQ(Values[0], 42); // value survives first GC
  EXPECT_EQ(Values[1], 99); // mutated value survives second GC
}

TEST(GC, ArrayOperations) {
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCFullModule GCMod;
  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, GCMod, ArrayOpsWasm));
  ASSERT_TRUE(VM.execute("test"));

  auto Log = GCMod.getLog();
  auto Values = GCMod.getValues();

  // Memory log: [0]=before alloc, [1]=after alloc, [2]=still alive
  ASSERT_EQ(Log.size(), 3);
  EXPECT_EQ(Log[0], 0);
  EXPECT_GT(Log[1], 0);
  EXPECT_GT(Log[2], 0);

  // Check values: len=10, arr[0]=7, arr[3]=42, len=10
  ASSERT_EQ(Values.size(), 4);
  EXPECT_EQ(Values[0], 10); // array.len
  EXPECT_EQ(Values[1], 7);  // array.get [0] (explicit array.new fill operand)
  EXPECT_EQ(Values[2], 42); // array.get [3] (set value survives GC)
  EXPECT_EQ(Values[3], 10); // array.len again
}

TEST(GC, AllocationPressure) {
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, GCMod, AllocPressureWasm));
  ASSERT_TRUE(VM.execute("test"));
  auto Log = GCMod.getLog();

  ASSERT_EQ(Log.size(), 3);
  EXPECT_EQ(Log[0], 0); // before loop

  // 500 live arrays (~516 KiB), comfortably under the threshold.
  EXPECT_GT(Log[1], 0);

  // Log[2] not asserted zero: host manualCollect() never scans the native
  // stack, and born-gray keeps the just-allocated arrays alive this cycle.
  // EXPECT_EQ(Log[2], 0); // not asserted: born-gray retention on first cycle
}

namespace {
struct RecordingPhaseObserver : WasmEdge::GC::PhaseObserver {
  std::mutex Mtx;
  std::vector<WasmEdge::GC::GCPhase> Seen;
  void onPhase(WasmEdge::GC::GCPhase P) noexcept override {
    std::lock_guard<std::mutex> L(Mtx);
    Seen.push_back(P);
  }
};
} // namespace

TEST(GC, PhaseObserverSeesFullCycle) {
  using WasmEdge::GC::GCPhase;
  WasmEdge::GC::Allocator Alloc;
  RecordingPhaseObserver Obs;
  Alloc.setPhaseObserver(&Obs);
  ASSERT_TRUE(Alloc.manualCollect());
  std::lock_guard<std::mutex> L(Obs.Mtx);
  // A full cycle must report root-start, gray-start, sweep-start, sweep-end.
  ASSERT_GE(Obs.Seen.size(), 4u);
  EXPECT_EQ(Obs.Seen.front(), GCPhase::MarkRootStart);
  EXPECT_EQ(Obs.Seen.back(), GCPhase::SweepEnd);
}

TEST(GC, AllocateDuringSweepSurvives) {
  // Deterministic fresh-object-during-sweep coverage (the concurrent
  // MultiMutatorAllocationStress reaches this only by chance): an object
  // allocated while
  // the collector is inside the sweep phase must not be freed by that sweep.
  // Fresh objects are pushed to Gray, never into the White set the sweep
  // frees, so the object survives; the Linux ASan gate turns a regression
  // into a hard UAF, and the sentinel readback guards the non-sanitizer
  // build.
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;
  Alloc.setManualGC(true); // no auto cycle may race the gated sweep

  std::atomic<bool> InSweep{false};
  std::atomic<bool> AllocDone{false};
  std::atomic<bool> HookFired{false};
  Alloc.setSweepPauseHook([&]() noexcept {
    if (HookFired.exchange(true)) {
      return; // gate only the first sweep
    }
    InSweep.store(true, std::memory_order_release);
    while (!AllocDone.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  RawData *Fresh = nullptr;
  std::thread Mut([&] {
    while (!InSweep.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    // The sweep owner is parked inside runSweepAndSwap: this allocation is
    // strictly concurrent with the sweep phase.
    Fresh = static_cast<RawData *>(Alloc.allocate(
        [](void *P) noexcept {
          auto *Raw = new (P) RawData;
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 1;
          new (&Raw->data()[0]) ValVariant(UINT32_C(0xC0FFEE));
        },
        static_cast<uint32_t>(sizeof(RawData) + sizeof(ValVariant))));
    AllocDone.store(true, std::memory_order_release);
  });

  ASSERT_TRUE(Alloc.manualCollect()); // full cycle; its sweep is gated above
  Mut.join();
  ASSERT_TRUE(HookFired.load());
  ASSERT_NE(Fresh, nullptr);
  // A swept (freed) object trips ASan here; the sentinel also guards plain
  // builds.
  EXPECT_EQ(Fresh->data()[0].get<uint32_t>(), UINT32_C(0xC0FFEE));
  Alloc.setSweepPauseHook(nullptr);
}

TEST(GC, ControllerOwnsAllocator) {
  WasmEdge::GC::Controller Ctrl;
  // The controller hands out a stable allocator reference usable exactly like
  // a standalone allocator.
  WasmEdge::GC::Allocator &Alloc = Ctrl.getAllocator();
  RecordingPhaseObserver Obs;
  Ctrl.setPhaseObserver(&Obs);
  ASSERT_TRUE(Alloc.manualCollect());
  std::lock_guard<std::mutex> L(Obs.Mtx);
  EXPECT_FALSE(Obs.Seen.empty());
}

// Deterministic Yuasa-deletion-barrier regression: a store that overwrites a
// live ref during the concurrent-mark window must shade the old ref, or the
// referenced object can be swept even though it was reachable at this cycle's
// snapshot. Built directly against GC::Controller/Allocator rather than a
// hand-assembled WASM module -- this build environment has no WAT compiler.
//
// A first design rooted a "Holder" struct (whose field points at "Inner") on
// a registered stack and hooked MarkGrayStart to null the field, expecting to
// race the collector's own root-scan against the store. That is not
// deterministic: selfScanInto() shades Holder (and wakes a worker via
// GrayNotEmptyCV) *before* MarkGrayStart even fires, so a worker always traced
// Holder->Inner first in practice -- verified by temporarily gating
// writeBarrier() to no-op on MarkingGray, which still passed every time
// (confirming the test was vacuous). This version removes the race instead of
// trying to win it: Inner is never reachable via any root, so nothing but our
// own explicit call can shade it. The MarkGrayStart callback runs
// synchronously on the coordinator thread, strictly before endHandshake()
// wakes any parked worker and strictly before GCCV.notify_all() wakes the
// worker that would otherwise attempt Sweep() -- so the barrier call below is
// provably the only thing that can save Inner this cycle.
TEST(GC, BarrierShadesOverwrittenRefDuringMarking) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();

  auto MakeGCStruct = [&](std::vector<ValVariant> Init) noexcept -> RefVariant {
    void *P = Alloc.allocate(
        [&](void *Pointer) noexcept {
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = static_cast<uint32_t>(Init.size());
          for (size_t I = 0; I < Init.size(); ++I) {
            new (&Raw->data()[I]) ValVariant(Init[I]);
          }
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant)));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };

  // Inner: a leaf struct with one sentinel field. Never placed on any
  // registered stack, global, table, element segment, or exception payload --
  // no root-scan will ever discover it.
  RefVariant InnerRef = MakeGCStruct({ValVariant(UINT32_C(4242))});
  // Holder: an ordinary (also unrooted) C++-side struct whose field holds the
  // "old value" a guest struct.set(Holder, 0, ref.null) would overwrite. Its
  // own reachability is irrelevant to this test; only Inner's survival is.
  RefVariant HolderRef = MakeGCStruct({ValVariant(InnerRef)});
  RawData *HolderRaw = HolderRef.getPtr<RawData>();

  // Cycle 1: promote Inner (and Holder) out of born-gray protection (new
  // objects start gray regardless of reachability) into the White pool, i.e.
  // "new -> white" per GC.AllocatorHostRootsRetainRelease's comment. Neither
  // is rooted, so ordinarily a second cycle would sweep both.
  ASSERT_TRUE(Ctrl.collect(true, false));

  // Cycle 2: intercept MarkGrayStart -- the instant the barrier arms for this
  // cycle, before any worker can act on it -- and perform the same operation
  // Executor::structSet performs on a guest struct.set: shade the old field
  // value (InnerRef) via Allocator::writeBarrier(), then overwrite the field
  // with ref.null. Since Inner has no other root, its survival past this
  // cycle depends entirely on that one writeBarrier() call.
  struct ShadeObserver : GC::PhaseObserver {
    GC::Allocator *Alloc = nullptr;
    RawData *HolderRaw = nullptr;
    bool Fired = false;
    void onPhase(GC::GCPhase P) noexcept override {
      if (P == GC::GCPhase::MarkGrayStart && !Fired) {
        Fired = true;
        ValVariant &Field = HolderRaw->data()[0];
        // Mirror Executor::structSet: shade the old ref before clobbering it.
        Alloc->writeBarrier(Field);
        Field = ValVariant(
            RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)));
      }
    }
  } Obs;
  Obs.Alloc = &Alloc;
  Obs.HolderRaw = HolderRaw;
  Ctrl.setPhaseObserver(&Obs);

  const uint64_t UsageBeforeCycle2 = Alloc.getMemoryUsage(); // Inner + Holder
  ASSERT_TRUE(Ctrl.collect(true, false));
  Ctrl.setPhaseObserver(nullptr);
  ASSERT_TRUE(Obs.Fired);

  // Holder is unrooted too, so it is reclaimed regardless (nothing shades
  // it). getMemoryUsage(), not a direct field read, is the survival signal:
  // doDeallocate() is a plain `operator delete` with no poisoning, so reading
  // Inner's fields back after an (incorrect) sweep can appear to "survive" a
  // use-after-free purely because nothing has reallocated that memory yet --
  // exactly what a first version of this test did, passing even with
  // writeBarrier() temporarily gated to no-op on MarkingGray. The live byte
  // count is the reliable signal: it drops to 0 only once Inner's block is
  // actually freed.
  const uint64_t UsageAfterCycle2 = Alloc.getMemoryUsage();
  EXPECT_GT(UsageAfterCycle2, 0u);                // Inner survived
  EXPECT_LT(UsageAfterCycle2, UsageBeforeCycle2); // Holder was reclaimed

  // Cycle 3: nothing roots Inner anymore (Holder's field was nulled, and
  // Holder itself is already gone), so this is not a permanent leak/over-
  // retention -- Inner is correctly reclaimed once truly unreachable.
  ASSERT_TRUE(Ctrl.collect(true, false));
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

// Regression test: writeBarrier() (and by extension bulkWriteBarrier())
// must be a no-op while CurrentGCState == Sweeping, not just Idle. Before the
// fix, a barrier call firing during the sweep window could re-shade a White
// object gray, rescuing it from the White-free loop it should have been
// swept by -- a "resurrection" that leaks memory the collector believed it
// reclaimed. GCPhase::SweepStart fires synchronously on the sweeping worker,
// strictly before the White-free loop (lib/gc/allocator.cpp), so a
// writeBarrier() call from that callback is a deterministic probe of the
// gate: if the gate is quiet, Target is freed this cycle regardless; if the
// gate is (incorrectly) armed, Target survives.
TEST(GC, BarrierQuietDuringSweeping) {
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();

  auto MakeGCStruct = [&](std::vector<ValVariant> Init) noexcept -> RefVariant {
    using RawData = Runtime::Instance::GCInstance::RawData;
    void *P = Alloc.allocate(
        [&](void *Pointer) noexcept {
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = static_cast<uint32_t>(Init.size());
          for (size_t I = 0; I < Init.size(); ++I) {
            new (&Raw->data()[I]) ValVariant(Init[I]);
          }
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant)));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };

  // Target: an unrooted leaf struct, never placed on any registered stack,
  // global, table, etc. -- nothing but our own explicit writeBarrier() call
  // below can shade it.
  RefVariant TargetRef = MakeGCStruct({ValVariant(UINT32_C(4242))});

  // Cycle 1: promote Target out of born-gray protection ("new -> white").
  ASSERT_TRUE(Ctrl.collect(true, false));

  // Cycle 2: intercept SweepStart -- synchronously on the sweeping worker,
  // strictly before the White-free loop -- and call writeBarrier() directly
  // on the unrooted Target. If the Sweeping-quiet gate holds, this call is a
  // no-op and Target is freed by the loop that follows. If the gate were
  // missing, the call would shade Target gray and
  // rescue it from this sweep.
  struct SweepObserver : GC::PhaseObserver {
    GC::Allocator *Alloc = nullptr;
    RefVariant *Target = nullptr;
    bool Fired = false;
    void onPhase(GC::GCPhase P) noexcept override {
      if (P == GC::GCPhase::SweepStart && !Fired) {
        Fired = true;
        Alloc->writeBarrier(*Target);
      }
    }
  } Obs;
  Obs.Alloc = &Alloc;
  Obs.Target = &TargetRef;
  Ctrl.setPhaseObserver(&Obs);

  ASSERT_TRUE(Ctrl.collect(true, false));
  Ctrl.setPhaseObserver(nullptr);
  ASSERT_TRUE(Obs.Fired);

  // Target was unrooted going into this cycle: if the barrier fired during
  // Sweeping had no effect (as it must), Target is
  // reclaimed and live usage drops to 0.
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

// Regression test: the root-scan shading path must be quiet during Sweeping
// too. writeBarrier() gates itself on the phase (BarrierQuietDuringSweeping
// above), but markGray() -- reached from Controller::gcSafepoint ->
// selfScanInto -> markNativeStackRoots, which is not a barrier and never
// consulted the phase -- did not, so the sweep's "nothing calls markGray
// concurrently here" precondition was an assumption rather than an invariant.
//
// A mutator returning from a host call is NativeRunning, and STW #2
// deliberately never waits for that state (waitForAcks skips it, because
// scanNonRunningRoots already scanned its stacks in place). Such a mutator can
// therefore reach gcSafepoint() on a stop flag that has not come down yet and
// run its conservative self-scan while the sweep is already freeing the White
// set. Because the sweep partitions the all-object list without holding
// AllMutex, a shade landing between that unlocked colour read and the
// Index.erase() that follows wins the White -> Gray CAS on an object the sweep
// has already condemned and pushes it onto the gray work list -- the sweep
// then frees it, leaving a dangling pointer that the next cycle's tracer pops.
// Observed as a use-after-free that trips the pop-is-gray assertion in
// GCThread.ShadowSpillProtocolUnderConcurrentCollect on assert-enabled builds.
//
// Probed through setSweepPauseHook rather than the SweepStart phase observer
// BarrierQuietDuringSweeping uses: notifyPhase(SweepStart) fires while the
// terminating worker still holds GrayMutex, so an observer that shades would
// deadlock on that non-recursive lock before ever reaching the gate (the
// existing test only gets away with it because writeBarrier() returns before
// touching GrayMutex). The sweep pause hook runs at the top of
// runSweepAndSwap() with the phase already Sweeping and no locks held --
// exactly the state a stale-stop-flag self-scan finds. Conservative scans are
// how this reaches a doomed object in production: a stale native-stack word
// keeps naming a block that is already garbage.
TEST(GC, RootShadeQuietDuringSweeping) {
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();

  auto MakeGCStruct = [&](std::vector<ValVariant> Init) noexcept -> RefVariant {
    using RawData = Runtime::Instance::GCInstance::RawData;
    void *P = Alloc.allocate(
        [&](void *Pointer) noexcept {
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = static_cast<uint32_t>(Init.size());
          for (size_t I = 0; I < Init.size(); ++I) {
            new (&Raw->data()[I]) ValVariant(Init[I]);
          }
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant)));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };

  // Unrooted leaf struct: nothing but our own explicit markGrayRoot() call can
  // shade it.
  RefVariant TargetRef = MakeGCStruct({ValVariant(UINT32_C(4242))});

  // Cycle 1: promote Target out of born-gray protection ("new -> white").
  ASSERT_TRUE(Ctrl.collect(true, false));

  // Cycle 2: shade Target from inside the paused sweep via markGrayRoot() --
  // the entry point every root scan (selfScanInto, scanNonRunningRoots,
  // scanShadowChain, scanAuxRoots, markNativeStackRoots) funnels through.
  std::atomic<bool> Fired{false};
  Alloc.setSweepPauseHook([&]() noexcept {
    if (Fired.exchange(true)) {
      return; // gate only the first sweep
    }
    const ValVariant V(TargetRef);
    GC::RootShadeTestSeam::shade(Alloc, V);
  });

  ASSERT_TRUE(Ctrl.collect(true, false));
  Alloc.setSweepPauseHook(nullptr);
  ASSERT_TRUE(Fired.load());

  // A shade that lands during Sweeping must not resurrect condemned garbage:
  // Target stays reclaimed and live usage drops to 0. Before the fix the CAS
  // succeeded, Target went gray and survived, and -- worse in the concurrent
  // case -- it was left on the gray work list after being freed.
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

// --- Allocation-free marking metadata -------------------------------------
//
// The colour/gray/all-object representation carries no allocating container:
// colour is a byte in the Header, the gray work list and the all-object list
// are intrusive links, and membership is an open-addressed index whose only
// growing operation lives on allocate()'s failure-returning path. The three
// tests below pin the colour parity flip, the rejection of interior pointers by
// the membership index, and the growth and tombstone reclamation of that
// index.

namespace {
// Allocate one leaf GC object with `Len` payload slots, initialised to a known
// pattern so a test can prove the collector never wrote into the payload.
RefVariant makeLeafObject(GC::Allocator &Alloc, uint32_t Len,
                          uint32_t Fill) noexcept {
  using RawData = Runtime::Instance::GCInstance::RawData;
  void *P = Alloc.allocate(
      [&](void *Pointer) noexcept {
        auto *Raw = new (Pointer) RawData;
        Raw->ModInst = nullptr;
        Raw->TypeIdx = 0;
        Raw->Length = Len;
        for (uint32_t I = 0; I < Len; ++I) {
          new (&Raw->data()[I]) ValVariant(Fill + I);
        }
      },
      static_cast<uint32_t>(sizeof(RawData) +
                            static_cast<size_t>(Len) * sizeof(ValVariant)));
  return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                    static_cast<RawData *>(P));
}
} // namespace

// The black/white role swap is a single parity flip on how Marked0/Marked1 are
// interpreted, so it must survive an odd number of cycles as well as an even
// one -- a flip applied in the wrong direction (or applied twice) would leave a
// live object interpreted as White and swept. Six cycles is three flips in each
// direction with the object rooted only on a registered value stack.
TEST(GC, SweepParityFlipSurvivesManyCycles) {
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);

  RefVariant Live = makeLeafObject(Alloc, 2, 0xABCD0000U);
  ASSERT_NE(Live.getPtr<void>(), nullptr);
  S.emplace_back(Live); // the only root
  const uint64_t Usage = Alloc.getMemoryUsage();
  ASSERT_GT(Usage, 0u);

  for (int Cycle = 0; Cycle < 6; ++Cycle) {
    ASSERT_TRUE(Ctrl.collect(true, false)) << "cycle " << Cycle;
    // Rooted throughout: usage must not move, on any parity.
    EXPECT_EQ(Alloc.getMemoryUsage(), Usage) << "cycle " << Cycle;
    // ...and the payload must still read back what we wrote, i.e. no collector
    // bookkeeping was written into the object body.
    const auto *Raw =
        Live.getPtr<const Runtime::Instance::GCInstance::RawData>();
    ASSERT_EQ(Raw->Length, 2u) << "cycle " << Cycle;
    EXPECT_EQ(Raw->data()[0].get<uint32_t>(), 0xABCD0000U) << "cycle " << Cycle;
    EXPECT_EQ(Raw->data()[1].get<uint32_t>(), 0xABCD0001U) << "cycle " << Cycle;
  }

  // Drop the root: two cycles (born-gray promotion, then sweep) reclaim it,
  // proving the object really was collectable all along and the survival above
  // was rooting, not a stuck colour.
  S.clear();
  ASSERT_TRUE(Ctrl.collect(true, false));
  ASSERT_TRUE(Ctrl.collect(true, false));
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

// A conservative candidate that points into a live object's payload -- aligned,
// plausible, and only sizeof(Header) away from a genuine header -- must be
// rejected by the membership index. Without the index the collector would treat
// `Candidate - sizeof(Header)` as a Header and CAS a colour byte into the
// middle of the object, silently corrupting the payload. The index is what
// makes that unreachable, so this test reads the payload back afterwards.
TEST(GC, InteriorPointerIsRejectedByMembershipIndex) {
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);

  // Big enough that several 16-byte-aligned interior offsets exist.
  constexpr uint32_t Len = 8;
  RefVariant Live = makeLeafObject(Alloc, Len, 0x11110000U);
  ASSERT_NE(Live.getPtr<void>(), nullptr);
  S.emplace_back(Live);
  const uint64_t Usage = Alloc.getMemoryUsage();
  ASSERT_GT(Usage, 0u);

  // Probe every 16-byte-aligned interior word of the payload through the write
  // barrier, with the barrier armed (MarkRootStart fires during MarkingRoot).
  // Each probe is a candidate whose implied header sits inside the object.
  struct Probe : GC::PhaseObserver {
    GC::Allocator *Alloc = nullptr;
    uint8_t *Base = nullptr;
    uint32_t Slots = 0;
    bool Fired = false;
    void onPhase(GC::GCPhase P) noexcept override {
      if (P != GC::GCPhase::MarkRootStart || Fired) {
        return;
      }
      Fired = true;
      for (uint32_t I = 1; I <= Slots; ++I) {
        uint8_t *Interior = Base + static_cast<size_t>(I) * 16U;
        RefVariant Fake(ValType(TypeCode::Ref, TypeCode::StructRef), Interior);
        Alloc->writeBarrier(Fake);
      }
    }
  } Obs;
  Obs.Alloc = &Alloc;
  Obs.Base = Live.getPtr<uint8_t>();
  Obs.Slots = Len;
  Ctrl.setPhaseObserver(&Obs);
  ASSERT_TRUE(Ctrl.collect(true, false));
  Ctrl.setPhaseObserver(nullptr);
  ASSERT_TRUE(Obs.Fired);

  // Nothing was allocated or freed by the probes, and -- the real assertion --
  // the payload still reads back exactly what was written.
  EXPECT_EQ(Alloc.getMemoryUsage(), Usage);
  const auto *Raw = Live.getPtr<const Runtime::Instance::GCInstance::RawData>();
  ASSERT_EQ(Raw->Length, Len);
  for (uint32_t I = 0; I < Len; ++I) {
    EXPECT_EQ(Raw->data()[I].get<uint32_t>(), 0x11110000U + I) << "slot " << I;
  }
}

// Exercise the membership index across its growth and tombstone-reclamation
// paths: allocate well past the initial 1024-slot capacity, collect (which
// tombstones every dead entry), then allocate the same population again so the
// next insert must rehash to reclaim those tombstones. Correctness is asserted
// on the outcome that the index exists to protect -- rooted objects survive,
// unrooted ones are reclaimed -- rather than on internal counters.
TEST(GC, ObjectIndexGrowsAndReclaimsTombstones) {
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);

  constexpr uint32_t Population = 3000; // > 1024 initial slots, forces growth
  constexpr uint32_t Keep = 64;

  for (int Round = 0; Round < 2; ++Round) {
    S.clear();
    std::vector<RefVariant> Rooted;
    for (uint32_t I = 0; I < Population; ++I) {
      RefVariant R = makeLeafObject(Alloc, 1, I);
      ASSERT_NE(R.getPtr<void>(), nullptr) << "round " << Round << " obj " << I;
      if (I < Keep) {
        S.emplace_back(R); // rooted on the registered value stack
        Rooted.push_back(R);
      }
    }
    const uint64_t Full = Alloc.getMemoryUsage();
    ASSERT_GT(Full, 0u);

    // Two cycles: the first promotes the whole population out of born-gray,
    // the second sweeps everything that is not rooted.
    ASSERT_TRUE(Ctrl.collect(true, false));
    ASSERT_TRUE(Ctrl.collect(true, false));
    EXPECT_LT(Alloc.getMemoryUsage(), Full) << "round " << Round;
    EXPECT_GT(Alloc.getMemoryUsage(), 0u) << "round " << Round;

    // Every kept object must still be intact -- a mis-sized or mis-probed index
    // would either lose one (freed while rooted) or corrupt it.
    for (uint32_t I = 0; I < Keep; ++I) {
      const auto *Raw =
          Rooted[I].getPtr<const Runtime::Instance::GCInstance::RawData>();
      ASSERT_EQ(Raw->Length, 1u) << "round " << Round << " obj " << I;
      EXPECT_EQ(Raw->data()[0].get<uint32_t>(), I)
          << "round " << Round << " obj " << I;
    }
  }

  // Drop everything: the heap must drain completely, so no entry was stranded
  // in the index (which would keep re-validating a freed address).
  S.clear();
  ASSERT_TRUE(Ctrl.collect(true, false));
  ASSERT_TRUE(Ctrl.collect(true, false));
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

// Regression: an empty gray queue is not quiescence. A collector worker
// pops a gray object and traces its children outside GrayMutex; if termination
// concludes (sweeps) while a worker holds a popped-but-untraced object, a child
// reachable only through that object -- and still White -- is freed early. The
// ActiveTracers accounting closes this: termination requires
// Gray.empty() && ActiveTracers == 0 under GrayMutex. This test pauses a worker
// mid-trace (after pop, before shading children) via setTracerPauseHook and
// asserts the child survives.
//
// Determinism: parent is rooted on the collector thread's registered stack, so
// the STW #1 root scan re-grays it every cycle; child is reachable only through
// parent's field, so nothing but the paused tracer's resumed child-shade can
// save it. This also exercises a related hazard: the collector thread is a
// registered Running mutator blocked in waitForCycleComplete when the worker
// drives STW #2, so setSelfBlocked must exclude it from waitForAcks or the
// cycle hangs.
TEST(GC, TerminationWaitsForInFlightTracer) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Controller Ctrl;
  GC::Allocator &A = Ctrl.getAllocator();

  auto MakeGCStruct = [&](std::vector<ValVariant> Init) noexcept -> RefVariant {
    void *P = A.allocate(
        [&](void *Pointer) noexcept {
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = static_cast<uint32_t>(Init.size());
          for (size_t I = 0; I < Init.size(); ++I) {
            new (&Raw->data()[I]) ValVariant(Init[I]);
          }
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant)));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };

  // child: a leaf; in cycle 2 reachable only through parent's field[0].
  RefVariant ChildRef = MakeGCStruct({ValVariant(UINT32_C(4242))});
  // parent: holds child in field 0.
  RefVariant ParentRef = MakeGCStruct({ValVariant(ChildRef)});

  std::atomic<bool> Paused{false}, Resume{false}, HookArmed{true};
  std::atomic<uint64_t> UsageBothLive{0};
  std::atomic<bool> SweptWhilePaused{false};

  std::thread Collector([&] {
    std::vector<ValVariant> S;
    auto Reg = Ctrl.registerStack(S);
    S.emplace_back(ValVariant(ParentRef)); // parent's only root

    // Cycle 1: promote both out of born-gray protection into the White pool.
    EXPECT_TRUE(Ctrl.collect(true, false));
    UsageBothLive.store(A.getMemoryUsage());
    EXPECT_GT(UsageBothLive.load(), 0u);

    // Arm the mid-trace pause for cycle 2 (workers are idle between cycles, so
    // this store does not race a worker's read).
    A.setTracerPauseHook([&] {
      if (HookArmed.exchange(false)) { // only the first pop (parent) pauses
        Paused.store(true);
        while (!Resume.load()) {
          std::this_thread::yield();
        }
      }
    });

    // Cycle 2: root scan grays parent; a worker pops parent and pauses before
    // shading child. Termination must wait for that in-flight tracer, or child
    // (White, reachable only via parent) is swept while we hold parent.
    EXPECT_TRUE(Ctrl.collect(true, false));
    A.setTracerPauseHook(nullptr);

    // Both survive: child was re-grayed by the resumed tracer, not swept early.
    EXPECT_EQ(A.getMemoryUsage(), UsageBothLive.load());
  });

  // Wait for the worker to pause mid-trace.
  while (!Paused.load()) {
    std::this_thread::yield();
  }
  // Hold the tracer paused and watch for an early sweep. With correct
  // ActiveTracers accounting, termination cannot conclude while this tracer is
  // in flight, so child (White) is never freed and usage stays at both-live. A
  // broken gate lets a worker terminate and sweep child now -- usage drops
  // below both-live within this window. The bounded wait exists only to give a
  // broken gate time to expose itself; the correct gate simply waits it out.
  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
  while (std::chrono::steady_clock::now() < Deadline) {
    if (A.getMemoryUsage() < UsageBothLive.load()) {
      SweptWhilePaused.store(true);
      break;
    }
    std::this_thread::yield();
  }
  // Release the tracer and let the cycle finish.
  Resume.store(true);
  Collector.join();
  EXPECT_FALSE(SweptWhilePaused.load())
      << "termination swept a White child while a tracer held its parent";
}

} // namespace
