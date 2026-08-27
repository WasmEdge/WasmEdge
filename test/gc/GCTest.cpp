// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCTest.cpp - GC tests ----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for GC (garbage collection) correctness in both
/// single-threaded and multi-threaded environments.
///
//===----------------------------------------------------------------------===//

#include "runtime/instance/gc.h"
#include "api/internal/managed_ref_getter.h"
#include "ast/type.h"
#include "common/spdlog.h"
#include "gc/allocator.h"
#include "gc/coherent_slot.h"
#include "gc/controller.h"
#include "runtime/instance/exception.h"
#include "runtime/instance/global.h"
#include "runtime/instance/table.h"
#include "runtime/instance/tag.h"
#include "runtime/stackmgr.h"
#include "system/fault.h"
#include "vm/vm.h"

// Capability gate test: manual Loader/Compiler/JIT path so a module's
// compile-time GC capability can differ from the executor that runs it.
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"

#ifdef WASMEDGE_USE_LLVM
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"
#endif

#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace WasmEdge {
namespace Executor {
// Test-only bridge to the private Executor::runTableGrowOp, befriended in
// executor.h. Lets GCThread.InterpreterGrowInitializerSurvivesCollect drive the
// interpreter table.grow handler directly so the scoped-root pin it adds around
// the grow window is exercised (and provably load-bearing) without a full guest
// module.
Expect<void> gcTestRunTableGrowOp(Executor &Exe,
                                  Runtime::StackManager &StackMgr,
                                  Runtime::Instance::TableInstance &TabInst) noexcept {
  return Exe.runTableGrowOp(StackMgr, TabInst);
}
} // namespace Executor
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
} // namespace GC
} // namespace WasmEdge

namespace {

// Tests that need real compiled code. Without the AOT/JIT compiler the module
// stays interpreted, so there is no native frame, no shadow spill and no
// safepoint poll to exercise -- skip rather than fail.
#ifdef WASMEDGE_USE_LLVM
#define SKIP_WITHOUT_COMPILER() ((void)0)
#else
#define SKIP_WITHOUT_COMPILER() GTEST_SKIP() << "requires the AOT/JIT compiler"
#endif

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

using namespace WasmEdge;

// --- Host functions for GC testing ---

class Collect : public Runtime::HostFunction<Collect> {
public:
  Expect<void> body(const Runtime::CallingFrame &CF) {
    // Unlike the single-threaded ExecutorTest.cpp copy, this host function is
    // also wired into GCThread's concurrent tests (e.g. ConcurrentAllocation),
    // where multiple threads race to CAS Idle->MarkingRoot; manualCollect()
    // legitimately returns false for the losers, so its result is not
    // asserted here.
    CF.getExecutor()->getAllocator().manualCollect();
    return {};
  }
};

class Record : public Runtime::HostFunction<Record> {
public:
  Expect<void> body(const Runtime::CallingFrame &CF) {
    std::lock_guard<std::mutex> Lock(Mutex);
    MemoryUsageLog.push_back(CF.getExecutor()->getAllocator().getMemoryUsage());
    return {};
  }
  Span<const uint64_t> getLog() const noexcept { return MemoryUsageLog; }

private:
  mutable std::mutex Mutex;
  std::vector<uint64_t> MemoryUsageLog;
};

class Check : public Runtime::HostFunction<Check> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t Value) {
    Values.push_back(Value);
    return {};
  }
  Span<const uint32_t> getValues() const noexcept { return Values; }

private:
  std::vector<uint32_t> Values;
};

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

// Module with collect + record
class GCRecModule : public Runtime::Instance::ModuleInstance {
public:
  GCRecModule() : ModuleInstance("gc") {
    addHostFunc("coll", std::make_unique<Collect>());
    auto RP = std::make_unique<Record>();
    R = RP.get();
    addHostFunc("rec", std::move(RP));
  }
  Span<const uint64_t> getLog() const noexcept { return R->getLog(); }

private:
  Record *R = nullptr;
};

// Module with collect + record + check
class GCFullModule : public Runtime::Instance::ModuleInstance {
public:
  GCFullModule() : ModuleInstance("gc") {
    addHostFunc("coll", std::make_unique<Collect>());
    auto RP = std::make_unique<Record>();
    R = RP.get();
    addHostFunc("rec", std::move(RP));
    auto CP = std::make_unique<Check>();
    C = CP.get();
    addHostFunc("check", std::move(CP));
  }
  Span<const uint64_t> getLog() const noexcept { return R->getLog(); }
  Span<const uint32_t> getValues() const noexcept { return C->getValues(); }

private:
  Record *R = nullptr;
  Check *C = nullptr;
};

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

// Test 6: Concurrent allocation - loop allocating arrays
const std::array<WasmEdge::Byte, 195> ConcurrentAllocWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5e,
    0x7f, 0x01, 0x60, 0x00, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x02, 0x14,
    0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x00, 0x01, 0x02,
    0x67, 0x63, 0x03, 0x72, 0x65, 0x63, 0x00, 0x01, 0x03, 0x02, 0x01, 0x02,
    0x07, 0x0e, 0x01, 0x0a, 0x61, 0x6c, 0x6c, 0x6f, 0x63, 0x5f, 0x6c, 0x6f,
    0x6f, 0x70, 0x00, 0x02, 0x0a, 0x34, 0x01, 0x32, 0x02, 0x01, 0x7f, 0x01,
    0x63, 0x00, 0x41, 0x00, 0x21, 0x02, 0x02, 0x40, 0x03, 0x40, 0x20, 0x02,
    0x20, 0x00, 0x4f, 0x0d, 0x01, 0x41, 0x00, 0x20, 0x01, 0xfb, 0x06, 0x00,
    0x21, 0x03, 0x20, 0x02, 0x41, 0x01, 0x6a, 0x21, 0x02, 0x0c, 0x00, 0x0b,
    0x0b, 0xd0, 0x00, 0x21, 0x03, 0x10, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x4b,
    0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0c, 0x02, 0x00, 0x04, 0x63, 0x6f,
    0x6c, 0x6c, 0x01, 0x03, 0x72, 0x65, 0x63, 0x02, 0x18, 0x01, 0x02, 0x04,
    0x00, 0x05, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x01, 0x04, 0x73, 0x69, 0x7a,
    0x65, 0x02, 0x01, 0x69, 0x03, 0x03, 0x74, 0x6d, 0x70, 0x03, 0x10, 0x01,
    0x02, 0x02, 0x00, 0x05, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x01, 0x04, 0x6c,
    0x6f, 0x6f, 0x70, 0x04, 0x0a, 0x02, 0x00, 0x03, 0x61, 0x72, 0x72, 0x01,
    0x02, 0x66, 0x6e};

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

// Test 8: Shared references via global - multiple mutators sharing ONE VM /
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

// Functions that RETURN gc refs to the host (for host-root retention tests).
//   (type $s (struct (field (mut i32))))
//   (type $a (array (mut i32)))
//   (func (export "make") (param i32) (result (ref $s))
//     (struct.new $s (local.get 0)))
//   (func (export "make_arr") (param i32) (result (ref $a))
//     (array.new_default $a (local.get 0)))
//   (func (export "drop_return_i31") (result i31ref)
//     (struct.new $s (i32.const 0)) drop (ref.i31 (i32.const 7)))
const std::array<WasmEdge::Byte, 127> HostRetentionWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x18, 0x05, 0x5f,
    0x01, 0x7f, 0x01, 0x5e, 0x7f, 0x01, 0x60, 0x01, 0x7f, 0x01, 0x64, 0x00,
    0x60, 0x01, 0x7f, 0x01, 0x64, 0x01, 0x60, 0x00, 0x01, 0x6c, 0x03, 0x04,
    0x03, 0x02, 0x03, 0x04, 0x07, 0x25, 0x03, 0x04, 0x6d, 0x61, 0x6b, 0x65,
    0x00, 0x00, 0x08, 0x6d, 0x61, 0x6b, 0x65, 0x5f, 0x61, 0x72, 0x72, 0x00,
    0x01, 0x0f, 0x64, 0x72, 0x6f, 0x70, 0x5f, 0x72, 0x65, 0x74, 0x75, 0x72,
    0x6e, 0x5f, 0x69, 0x33, 0x31, 0x00, 0x02, 0x0a, 0x1e, 0x03, 0x07, 0x00,
    0x20, 0x00, 0xfb, 0x00, 0x00, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xfb, 0x07,
    0x01, 0x0b, 0x0c, 0x00, 0x41, 0x00, 0xfb, 0x00, 0x00, 0x1a, 0x41, 0x07,
    0xfb, 0x1c, 0x0b, 0x00, 0x0e, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x04, 0x07,
    0x02, 0x00, 0x01, 0x73, 0x01, 0x01, 0x61};

// Returns an EXTERNALIZED gc struct to the host (regression for the host-root
// retention of externalized refs):
//   (func (export "make_ext") (param i32) (result externref)
//     (extern.convert_any (struct.new $s (local.get 0))))
const std::array<WasmEdge::Byte, 64> MakeExtWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02,
    0x5f, 0x01, 0x7f, 0x01, 0x60, 0x01, 0x7f, 0x01, 0x6f, 0x03, 0x02,
    0x01, 0x01, 0x07, 0x0c, 0x01, 0x08, 0x6d, 0x61, 0x6b, 0x65, 0x5f,
    0x65, 0x78, 0x74, 0x00, 0x00, 0x0a, 0x0b, 0x01, 0x09, 0x00, 0x20,
    0x00, 0xfb, 0x00, 0x00, 0xfb, 0x1b, 0x0b, 0x00, 0x0b, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

// A PASSIVE element segment whose init expr allocates a gc struct (regression
// for element-segment refs being scanned as roots). After two collections the
// segment holds the only reference; the test materializes it via array.new_elem
// and reads the field, garbage if the segment was not scanned.
//   (type $s (struct (field i32))) (type $a (array (ref null $s)))
//   (elem $e (ref null $s) (item (struct.new $s (i32.const 42))))
//   (func (export "test") coll; coll;
//     check (struct.get $s 0 (ref.cast (ref $s)
//       (array.get $a (array.new_elem $a $e 0 1) 0))))
const std::array<WasmEdge::Byte, 164> ElemRootWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x04, 0x5f,
    0x01, 0x7f, 0x00, 0x5e, 0x63, 0x00, 0x00, 0x60, 0x00, 0x00, 0x60, 0x01,
    0x7f, 0x00, 0x02, 0x16, 0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c,
    0x6c, 0x00, 0x02, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b,
    0x00, 0x03, 0x03, 0x02, 0x01, 0x02, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65,
    0x73, 0x74, 0x00, 0x02, 0x09, 0x0b, 0x01, 0x05, 0x63, 0x00, 0x01, 0x41,
    0x2a, 0xfb, 0x00, 0x00, 0x0b, 0x0a, 0x25, 0x01, 0x23, 0x01, 0x01, 0x63,
    0x01, 0x10, 0x00, 0x10, 0x00, 0x41, 0x00, 0x41, 0x01, 0xfb, 0x0a, 0x01,
    0x00, 0x21, 0x00, 0x20, 0x00, 0x41, 0x00, 0xfb, 0x0b, 0x01, 0xfb, 0x16,
    0x00, 0xfb, 0x02, 0x00, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x2e, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x01, 0x0e, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c, 0x6c,
    0x01, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x02, 0x08, 0x01, 0x02, 0x01,
    0x00, 0x03, 0x61, 0x72, 0x72, 0x04, 0x07, 0x02, 0x00, 0x01, 0x73, 0x01,
    0x01, 0x61, 0x08, 0x04, 0x01, 0x00, 0x01, 0x65};

// Exports a GLOBAL and a TABLE each holding a genuinely CONCRETE (ref $s)
// managed struct reference, produced by a constant struct.new initializer.
// Unlike every other GC-suite fixture (which hand-stamps an abstract StructRef
// ref, reads an i31 slot, or reads an already-normalized invoke result), the
// slot RefVariant here is stamped ValType(TypeCode::Ref, <type index>) by
// Executor::structNew (lib/executor/engine/refInstr.cpp ~512) -- the concrete
// form that Executor::expandGCRefType must normalize to structref. Used by
// GC.ExpandGCRefTypeNormalizesConcreteSlotRef.
//   (type $s (struct (field i32)))
//   (global (export "g") (ref $s) (struct.new $s (i32.const 42)))
//   (table (export "t") 1 1 (ref $s) (struct.new $s (i32.const 7)))
const std::array<WasmEdge::Byte, 67> ConcreteGlobalTableWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x5f,
    0x01, 0x7f, 0x00, 0x04, 0x0e, 0x01, 0x40, 0x00, 0x64, 0x00, 0x01, 0x01,
    0x01, 0x41, 0x07, 0xfb, 0x00, 0x00, 0x0b, 0x06, 0x0a, 0x01, 0x64, 0x00,
    0x00, 0x41, 0x2a, 0xfb, 0x00, 0x00, 0x0b, 0x07, 0x09, 0x02, 0x01, 0x67,
    0x03, 0x00, 0x01, 0x74, 0x01, 0x00, 0x00, 0x0b, 0x04, 0x6e, 0x61, 0x6d,
    0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

// Allocate one zero-child GC object directly through the allocator, wrap it as
// a RefVariant, and exercise retain/collect/release. Zero-Length RawData so the
// collector's child-scan reads no garbage children.
TEST(GC, AllocatorHostRootsRetainRelease) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;

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

  Alloc.retainResult(Ref);

  EXPECT_TRUE(Alloc.manualCollect()); // new object: gray -> black -> white
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
  EXPECT_TRUE(Alloc.manualCollect()); // retained: re-grayed, survives the sweep
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);

  Alloc.releaseRef(Ref);
  EXPECT_TRUE(Alloc.manualCollect()); // unrooted now -> swept
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

TEST(GC, ReleaseAllRefsDoesNotClearScopedRoots) {
  // C2 regression: scoped BoundaryRoots must survive a public releaseAllRefs()
  // (WasmEdge_{VM,Executor}ReleaseAllRefs) -- they live in a separate store the
  // host-facing release-all cannot consume, and are still scanned as roots.
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;
  Alloc.setManualGC(true);

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

  {
    GC::BoundaryRoots BR(Alloc);
    BR.pin(Ref); // scoped root only

    // A newly-allocated object is born gray and gets traced on its very
    // first collect regardless of rootedness (see
    // GC.AllocatorHostRootsRetainRelease), so this first collect is not yet
    // a meaningful check of the scoped root -- it just clears that grace
    // period before the real assertion below.
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u);

    // Host-facing release-all must NOT drop the scoped root.
    Alloc.releaseAllRefs();

    // Forced collection: a scoped root that leaked into HostRoots (pre-fix)
    // would have been cleared by releaseAllRefs above and the object swept
    // here.
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u); // survived: still a scoped root

    // BR goes out of scope -> releaseScopedRef drops the last root.
  }

  // Now unrooted: the object is reclaimed.
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

TEST(GC, ExecutorRetainAndRelease) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto Res = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(42)},
                        {ValType(TypeCode::I32)});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  const RefVariant Ref = (*Res)[0].first.get<RefVariant>();
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // struct allocated

  EXPECT_TRUE(Alloc.manualCollect()); // new -> white
  EXPECT_TRUE(Alloc.manualCollect()); // retained by HostRoots -> survives
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  VM.getExecutor().releaseRef(Ref);
  EXPECT_TRUE(Alloc.manualCollect()); // unrooted -> reclaimed
  EXPECT_EQ(Alloc.getMemoryUsage(), Before);
}

// Load-bearing check: Executor::expandGCRefType
// (include/executor/executor.h) must normalize a genuinely CONCRETE
// (ref $t) slot ref into its abstract composite kind (structref) so the
// producer-bearing retained C-API getters
// (WasmEdge_GlobalInstanceGetValueRetained /
// WasmEdge_TableInstanceGetDataRetained) recognize it as GC-retainable and
// pin it before handing it to the host. ValType::isGCRefType() is FALSE for a
// concrete type-index ref, so if expandGCRefType ever returned the concrete
// type unchanged the getter would SKIP retention and hand back a dangling
// reference -- a use-after-free (the exact E4 hazard). Every other GC-suite
// test hand-stamps an already-abstract StructRef, reads an i31 (abstract)
// slot, or reads an invoke result (Executor::invoke already normalizes
// returned refs to abstract), so none falsifies the concrete->abstract
// transform. This one does: it instantiates a module whose exported global
// and table hold concrete (ref $s) refs produced by a constant struct.new,
// reads each slot exactly as the getter does (getValue()/getRefAddr()),
// asserts the ref is genuinely concrete (NOT abstract, NOT already GC-ref),
// then asserts expandGCRefType turns it into a GC-ref.
TEST(GC, ExpandGCRefTypeNormalizesConcreteSlotRef) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(ConcreteGlobalTableWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const auto *ModInst = VM.getActiveModule();
  ASSERT_NE(ModInst, nullptr);
  auto &Exec = VM.getExecutor();

  // ---- Concrete ref in an exported GLOBAL slot (populated by struct.new) ----
  {
    // findGlobalExports is exactly what WasmEdge_ModuleInstanceFindGlobal (the
    // C-API path feeding the retained getter) uses to reach the slot.
    const auto *Glob = ModInst->findGlobalExports("g");
    ASSERT_NE(Glob, nullptr);
    const RefVariant SlotRef = Glob->getValue().get<RefVariant>();
    ASSERT_FALSE(SlotRef.isNull());

    // The slot ref is genuinely CONCRETE: a type-index heap type, not one of
    // the abstract heap types. If this fails the fixture is wrong (the slot
    // was normalized on the way in).
    EXPECT_FALSE(SlotRef.getType().isAbsHeapType());
    // A concrete type-index ref is NOT by-value releasable: isGCRefType()
    // (structref/arrayref only) is what the getter keys retention off of.
    EXPECT_FALSE(SlotRef.getType().isGCRefType());
    // The load-bearing normalization: concrete (ref $s) -> abstract structref,
    // which IS a GC-ref, so the getter retains it (no UAF).
    EXPECT_TRUE(Exec.expandGCRefType(SlotRef).isGCRefType());
  }

  // ---- Concrete ref in an exported TABLE slot (populated by struct.new) ----
  {
    const auto *Tab = ModInst->findTableExports("t");
    ASSERT_NE(Tab, nullptr);
    auto AddrRes = Tab->getRefAddr(0); // exactly what the getter reads
    ASSERT_TRUE(AddrRes);
    const RefVariant SlotRef = *AddrRes;
    ASSERT_FALSE(SlotRef.isNull());

    EXPECT_FALSE(SlotRef.getType().isAbsHeapType());
    EXPECT_FALSE(SlotRef.getType().isGCRefType());
    EXPECT_TRUE(Exec.expandGCRefType(SlotRef).isGCRefType());
  }
}

// ==========================================================================
// Single-threaded GC tests
// ==========================================================================

TEST(GC, StructAllocAndCollect) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(StructGCWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // autoCollect off: tests assert exact usage, so only explicit coll runs.
  VM.getExecutor().getAllocator().setManualGC(true);
  ASSERT_TRUE(VM.execute("test"));
  auto Log = GCMod.getLog();

  ASSERT_EQ(Log.size(), 4);
  EXPECT_EQ(Log[0], 0); // before allocation
  EXPECT_GT(Log[1], 0); // after struct.new
  EXPECT_GT(Log[2], 0); // survives: born-gray makes the first cycle keep it
  EXPECT_EQ(Log[3], 0); // after drop + GC, collected
}

TEST(GC, NestedReferences) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(NestedRefWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // autoCollect off: tests assert exact usage, so only explicit coll runs.
  VM.getExecutor().getAllocator().setManualGC(true);
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);

  // Host modules must outlive the VM that terminates them; declare first.
  auto GCMod = std::make_unique<Runtime::Instance::ModuleInstance>("gc");
  GCMod->addHostFunc("coll", std::make_unique<Collect>());
  auto CP = std::make_unique<Check>();
  auto *C = CP.get();
  GCMod->addHostFunc("check", std::move(CP));

  VM::VM VM(Conf);
  VM.registerModule(*GCMod);

  ASSERT_TRUE(VM.loadWasm(DataSurvivesGCWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // autoCollect off: tests assert exact usage, so only explicit coll runs.
  VM.getExecutor().getAllocator().setManualGC(true);
  ASSERT_TRUE(VM.execute("test"));
  auto Values = C->getValues();

  ASSERT_EQ(Values.size(), 2);
  EXPECT_EQ(Values[0], 42); // value survives first GC
  EXPECT_EQ(Values[1], 99); // mutated value survives second GC
}

TEST(GC, ArrayOperations) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  GCFullModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ArrayOpsWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // autoCollect off: tests assert exact usage, so only explicit coll runs.
  VM.getExecutor().getAllocator().setManualGC(true);
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(AllocPressureWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // autoCollect off: tests assert exact usage, so only explicit coll runs.
  VM.getExecutor().getAllocator().setManualGC(true);
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
  // Deterministic fresh-object-during-sweep coverage (previously only
  // incidental in MultiMutatorAllocationStress): an object allocated while
  // the collector is INSIDE the sweep phase must not be freed by that sweep.
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
// live ref during the concurrent-mark window must shade the OLD ref, or the
// referenced object can be swept even though it was reachable at this cycle's
// snapshot. Built directly against GC::Controller/Allocator rather than a
// hand-assembled WASM module -- this build environment has no WAT compiler.
//
// A first design rooted a "Holder" struct (whose field points at "Inner") on
// a registered stack and hooked MarkGrayStart to null the field, expecting to
// race the collector's own root-scan against the store. That is NOT
// deterministic: selfScanInto() shades Holder (and wakes a worker via
// GrayNotEmptyCV) *before* MarkGrayStart even fires, so a worker always traced
// Holder->Inner first in practice -- verified by temporarily gating
// writeBarrier() to no-op on MarkingGray, which still passed every time
// (confirming the test was vacuous). This version removes the race instead of
// trying to win it: Inner is NEVER reachable via any root, so nothing but our
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
  // Executor::structSet performs on a guest struct.set: shade the OLD field
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
        // Mirror Executor::structSet: shade the OLD ref before clobbering it.
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

// Regression test: the ROOT-SCAN shading path must be quiet during Sweeping
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
// set. Because the sweep partitions the all-object list WITHOUT holding
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

// --- Allocation-free marking metadata (C2) --------------------------------
//
// The colour/gray/all-object representation carries no allocating container:
// colour is a byte in the Header, the gray work list and the all-object list
// are intrusive links, and membership is an open-addressed index whose ONLY
// growing operation lives on allocate()'s failure-returning path. The three
// tests below pin the parts of that representation that have no analogue in
// the old set-based code.

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
// interpreted, so it must survive an ODD number of cycles as well as an even
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
  S.emplace_back(Live); // the ONLY root
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

// A conservative candidate that points INTO a live object's payload -- aligned,
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
  // barrier, with the barrier ARMED (MarkRootStart fires during MarkingRoot).
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
  // R3 primitive: the fallible teardown entries (C API delete functions) need
  // to ask "is the calling thread inside one of THIS controller's own
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
  // while a handshake is already in flight must NOT be admitted to Running until
  // the handshake releases. Before the admission gate, registerStack() returned
  // immediately, letting a late joiner mutate roots during the coordinator's
  // writer-free snapshot window (or enqueue work after a terminal check).
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
    // Record BEFORE signalling: a correct gate admits only with StopFlag
    // observed clear, so a buggy fall-through -- admitted while the handshake
    // is still in flight, flag still raised -- is caught deterministically by
    // the post-join assert below, with no timing window.
    StopSeenAtAdmission.store(Ctrl.stopRequested());
    Admitted.store(true);
    (void)Reg;
  });

  // Deterministic observation instead of a sleep: a correctly-gated joiner
  // PARKS (ParkedMutators goes nonzero); a buggy non-blocking registration
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

TEST(GC, TeardownDrainsSynchronousMutator) {
  // The teardown drain must wait for a synchronous registered stack on another
  // thread -- which holds NO launch lease and is not parked -- before it
  // returns, so destruction never frees state a synchronous execute()/invoke()
  // is still using. Before the RegisteredStacks counter the drain waited only on
  // leases + parked mutators and would return while another thread still ran.
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
    // on reads zero HERE -- always, no timing. A drain that skips the
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

TEST(GC, SpinningGuestReachesSafepoint) {
  // Reuses ConcurrentAllocWasm's alloc_loop export: a real back-edge loop
  // (the interpreter's driving loop polls at the top of the back edge),
  // run with a large iteration count so the coordinator's handshake overlaps
  // its execution.
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);

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

TEST(GC, TeardownDrainsLiveAsync) {
  // Regression: destroying a VM while a detached
  // async invocation is still running must NOT leave the worker touching a freed
  // Executor/Controller. Without the launch lease + drain the detached thread
  // outlives the VM and use-after-frees (caught by ASan on the Linux gate); with
  // it, ~Controller::beginClosing() blocks until the worker unwinds.
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
  // teardown deterministically race a genuinely-executing async task rather than
  // guessing at timing, and closes the "worker not yet started" escape where the
  // drain would be a trivial no-op.
  auto &Ctrl = VM->getExecutor().getController();
  while (Ctrl.debugActiveStackCount() == 0) {
    std::this_thread::yield();
  }

  // Deletion-ordered handle contract (R1): the Async handle co-owns the
  // launch lease, so destroying the VM on THIS thread while the handle is
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
  // R1 regression (deletion-ordered handle contract): a successful async
  // result can carry a GC-managed reference retained into the handle's
  // shared_future. The launch lease is therefore co-owned by the worker
  // lambda AND the Async handle, released only at the LAST owner's
  // destruction -- so ~VM's teardown drain cannot free the allocator while a
  // live handle still exposes references into that heap. Before the fix the
  // lease died at set_value and the reads below were use-after-free (the
  // Linux ASan gate is the enforcing signal; this also documents that the
  // handle must be released before the VM on the same thread -- the drain
  // deliberately waits on it).
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

  // Destroy the VM on ANOTHER thread while this thread still holds the
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
  // handle is alive, so the destructor CANNOT have finished. A lease
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

TEST(GC, AsyncInvokeRejectsNonDeferrableModule) {
  // R4: asyncInvoke must refuse a target whose defining module has
  // non-deferrable storage (embedder-constructed -- here a stack host
  // module). Its ModulePin could never DEFER the module's destruction; a
  // caller destroying the module mid-worker would abort (checked builds) or
  // UAF (release). A runtime-instantiated module is terminate()-managed and
  // stays accepted.
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  GCRecModule GCMod; // stack storage: NOT deferrable
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto *HostFn = GCMod.findFuncExports("coll");
  ASSERT_NE(HostFn, nullptr);
  auto A = VM.getExecutor().asyncInvoke(HostFn, {}, {});
  EXPECT_FALSE(A.valid()); // refused up front: pin could not defer

  // The runtime-instantiated active module IS deferrable; same call form.
  auto *GuestFn = VM.getActiveModule()->findFuncExports("alloc_loop");
  ASSERT_NE(GuestFn, nullptr);
  std::vector<ValVariant> P{UINT32_C(1), UINT32_C(1)};
  std::vector<ValType> PT{ValType(TypeCode::I32), ValType(TypeCode::I32)};
  auto B = VM.getExecutor().asyncInvoke(GuestFn, P, PT);
  ASSERT_TRUE(B.valid());
  EXPECT_TRUE(B.get());
}

TEST(GC, AsyncInvokeRejectsModulelessTarget) {
  // H4: asyncInvoke must refuse a target with a null defining module (a
  // standalone/independent host function). Nothing can pin such a
  // FunctionInstance, so a caller destroying it mid-worker would UAF. Reject
  // up front (invalid Async). Synchronous invoke of the same func is
  // unaffected (covered elsewhere).
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  auto &Exec = VM.getExecutor();

  // A standalone host function instance: null module (CompositeBase() ctor).
  auto HostFunc = std::make_unique<Collect>();
  Runtime::Instance::FunctionInstance StandaloneFn(std::move(HostFunc));
  ASSERT_EQ(StandaloneFn.getModule(), nullptr);

  auto A = Exec.asyncInvoke(&StandaloneFn, {}, {});
  EXPECT_FALSE(A.valid()); // refused: a null-module target cannot be pinned
}

// ==========================================================================
// Multi-threaded GC tests
// ==========================================================================

TEST(GCThread, TableCanHoldManagedResolvedAtConstruction) {
  // The can-hold-managed bit is resolved ONCE, at
  // ModuleInstance::addTable's call site -- the one place that has
  // both the table type and the defining module's type list -- and then just
  // read back by canHoldManaged(). All four ref types under test here are
  // abstract heap types, so AST::TypeMatcher::refTypeCanHoldGCObject never
  // needs to consult the type list; a module that defines no types is enough
  // to exercise the addTable resolution path end-to-end. Deterministic,
  // single-threaded (no concurrent GC activity is needed to exercise
  // construction-time resolution).
  class CanHoldManagedTestModule : public Runtime::Instance::ModuleInstance {
  public:
    explicit CanHoldManagedTestModule(GC::Allocator &Alloc)
        : ModuleInstance("chm") {
      addTable(Alloc, AST::TableType(TypeCode::AnyRef, 0, 1));
      addTable(Alloc, AST::TableType(TypeCode::StructRef, 0, 1));
      addTable(Alloc, AST::TableType(TypeCode::FuncRef, 0, 1));
      addTable(Alloc, AST::TableType(TypeCode::NullFuncRef, 0, 1));
    }
    uint32_t tableCount() const noexcept { return getTableNum(); }
    bool canHold(uint32_t Idx) const noexcept {
      return unsafeGetTable(Idx)->canHoldManaged();
    }
  };

  GC::Allocator Alloc;
  CanHoldManagedTestModule Mod(Alloc);
  ASSERT_EQ(Mod.tableCount(), 4u);
  EXPECT_TRUE(Mod.canHold(0)) << "anyref should be GC-managed";
  EXPECT_TRUE(Mod.canHold(1)) << "structref should be GC-managed";
  EXPECT_FALSE(Mod.canHold(2)) << "funcref should not be GC-managed";
  EXPECT_FALSE(Mod.canHold(3)) << "nullfuncref should not be GC-managed";
}

TEST(GCThread, SetAllocatorRejectsForeignGrowableTable) {
  // TableInstance::setAllocator is the single, fallible, centralized
  // owner-attach check. A SECOND, FOREIGN controller may not attach
  // to a table that is EITHER managed-capable (hazard a: a ref it stores would
  // barrier against the wrong allocator and be swept while reachable) OR
  // growable (hazard b: a reallocating grow would free a buffer a peer reader
  // still holds). A fixed, non-managed table carries neither hazard and stays
  // freely shareable, keeping its first owner. A same-owner re-attach is an
  // idempotent success. Deterministic and single-threaded: the two hazards are
  // structural properties of the table, so no concurrent GC activity is needed
  // to exercise the rejection decision.
  GC::Allocator AllocA;
  GC::Allocator AllocB;

  // (i) Growable funcref table (max 4 > min 0): buffer-realloc UAF (hazard b),
  // rejected for a foreign controller even though funcref is NOT managed.
  {
    AST::TableType TType(TypeCode::FuncRef, 0, 4);
    Runtime::Instance::TableInstance Table(TType, /*CanHoldManaged=*/false);
    EXPECT_TRUE(Table.setAllocator(AllocA)); // first attach: claims ownership
    EXPECT_TRUE(Table.setAllocator(AllocA)); // same owner: idempotent success
    auto Foreign = Table.setAllocator(AllocB); // foreign + growable: reject
    EXPECT_FALSE(Foreign);
    EXPECT_EQ(Foreign.error(), ErrCode::Value::IncompatibleImportType);
  }

  // (ii) Managed-capable table (structref), FIXED size (max == min): rejected
  // for a foreign controller by hazard a regardless of the fixed size.
  {
    AST::TableType TType(TypeCode::StructRef, 1, 1);
    Runtime::Instance::TableInstance Table(TType, /*CanHoldManaged=*/true);
    EXPECT_TRUE(Table.setAllocator(AllocA));
    auto Foreign = Table.setAllocator(AllocB); // foreign + managed: reject
    EXPECT_FALSE(Foreign);
    EXPECT_EQ(Foreign.error(), ErrCode::Value::IncompatibleImportType);
  }

  // (iii) Fixed funcref table (hasMax && max == min), not managed: neither
  // hazard applies, so a foreign attach is ALLOWED (keeps the first owner).
  {
    AST::TableType TType(TypeCode::FuncRef, 2, 2);
    Runtime::Instance::TableInstance Table(TType, /*CanHoldManaged=*/false);
    EXPECT_TRUE(Table.setAllocator(AllocA));
    EXPECT_TRUE(Table.setAllocator(AllocB)); // foreign but freely shareable
  }
}

// A prebuilt module whose owned tables we can pre-attach and inspect.
// addHostTable adds an owned, initially-UNATTACHED table; the executor's
// registerModule walk is what attaches it.
class RegisterWalkModule : public Runtime::Instance::ModuleInstance {
public:
  explicit RegisterWalkModule(std::string_view Name) : ModuleInstance(Name) {}
  Runtime::Instance::TableInstance *
  addOwnedTable(std::unique_ptr<Runtime::Instance::TableInstance> Tab) {
    auto *Raw = Tab.get();
    addHostTable("t" + std::to_string(Count++), std::move(Tab));
    return Raw;
  }

private:
  uint32_t Count = 0;
};

TEST(GCThread, RegisterModuleRejectsForeignGrowableTable) {
  // registerModule walks a prebuilt module's owned tables/globals and attaches
  // them to the registering executor's allocator, validating ownership. A
  // managed/growable table already owned by ANOTHER controller is
  // rejected; the walk is transactional (preflight-all-then-commit), so a
  // rejection reverses every attach THIS registration made and does not publish
  // the module. A module with only fixed non-managed tables registers into any
  // number of executors (freely shared). Deterministic and single-threaded: the
  // two hazards are structural properties of the table.
  Configure Conf;
  Conf.addProposal(Proposal::GC);

  // --- Negative + rollback ------------------------------------------------
  {
    // Executor A is the GC executor we register INTO; controller B already owns
    // the module's second table.
    WasmEdge::Executor::Executor ExecA(Conf);
    Runtime::StoreManager StoreA;
    GC::Controller CtrlB;

    RegisterWalkModule Mod("walk-neg");
    // Table 1: managed, fixed, initially unattached -> the walk NEWLY claims it
    // for A. This is the attach the rollback must reverse.
    auto *Tab1 = Mod.addOwnedTable(
        std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::StructRef, 1, 1), /*CanHoldManaged=*/true));
    // Table 2: growable, already owned by foreign controller B -> the walk's
    // attach to A is rejected (hazard b), aborting the whole registration.
    auto *Tab2 = Mod.addOwnedTable(
        std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::FuncRef, 0, 4), /*CanHoldManaged=*/false));
    ASSERT_TRUE(Tab2->setAllocator(CtrlB.getAllocator()));

    // Preconditions: Tab1 not yet controller-owned; Tab2 owned by B (foreign to
    // A).
    ASSERT_FALSE(Tab1->isManagedByController());
    ASSERT_TRUE(Tab2->hasForeignAllocator(ExecA.getAllocator()));

    auto Res = ExecA.registerModule(StoreA, Mod);
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::IncompatibleImportType);

    // Rollback proof: Tab1 (managed) must be cleanly UN-attached again, so it is
    // no longer controller-owned AND is freshly claimable by any controller. If
    // rollback had failed, Tab1 would still be owned by A and this fresh claim
    // by B would be a rejected foreign-managed attach.
    EXPECT_FALSE(Tab1->isManagedByController())
        << "rollback must un-attach the table this registration newly claimed";
    EXPECT_TRUE(Tab1->setAllocator(CtrlB.getAllocator()))
        << "the rolled-back table must be cleanly unattached (claimable)";
    // Tab2 must still be owned by its first controller B (never moved to A).
    EXPECT_TRUE(Tab2->hasForeignAllocator(ExecA.getAllocator()))
        << "the foreign-owned table must stay with its first controller";
    EXPECT_FALSE(Tab2->hasForeignAllocator(CtrlB.getAllocator()));
    // Module not published.
    EXPECT_EQ(StoreA.findModule("walk-neg"), nullptr);
  }

  // --- Positive: fixed non-managed table freely shared across executors ---
  {
    WasmEdge::Executor::Executor ExecA(Conf);
    WasmEdge::Executor::Executor ExecB(Conf);
    Runtime::StoreManager StoreA;
    Runtime::StoreManager StoreB;

    RegisterWalkModule Mod("walk-pos");
    Mod.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
        AST::TableType(TypeCode::FuncRef, 2, 2), /*CanHoldManaged=*/false));

    // First registration claims the fixed funcref table for A; the second is a
    // foreign-but-freely-shareable attach that keeps A and still succeeds.
    EXPECT_TRUE(ExecA.registerModule(StoreA, Mod));
    EXPECT_TRUE(ExecB.registerModule(StoreB, Mod));
    EXPECT_NE(StoreA.findModule("walk-pos"), nullptr);
    EXPECT_NE(StoreB.findModule("walk-pos"), nullptr);
  }
}

TEST(GCThread, RegisterModuleRollsBackClaimWhenPublicationFails) {
  // Transaction boundary: the ownership claim spans the WHOLE registration,
  // not just the attach walk. StoreManager::registerModule rejects a duplicate
  // module name AFTER the walk has already attached the module's roots, so that
  // rejection must reverse the attach too. A leaked claim is not memory-unsafe
  // (each instance unregisters itself in its destructor), but it would leave a
  // never-published module owned by -- and scanned by -- this executor, and it
  // would make a later registration of the SAME module into a DIFFERENT
  // executor fail the foreign-hazard check, turning a recoverable name conflict
  // into a permanent cross-executor rejection. Both registerModule overloads
  // are covered. Deterministic and single-threaded.
  Configure Conf;
  Conf.addProposal(Proposal::GC);

  // --- Default-name overload ----------------------------------------------
  {
    WasmEdge::Executor::Executor ExecA(Conf);
    WasmEdge::Executor::Executor ExecC(Conf);
    Runtime::StoreManager StoreA;
    Runtime::StoreManager StoreC;

    // Occupy the name in A's store so the publication below is rejected.
    RegisterWalkModule Occupier("dup-default");
    ASSERT_TRUE(ExecA.registerModule(StoreA, Occupier));
    ASSERT_EQ(StoreA.findModule("dup-default"), &Occupier);

    // The module whose registration fails at PUBLICATION (not at the walk). Its
    // managed table is unattached, so the walk NEWLY claims it -- exactly the
    // attach the failed publication must reverse.
    RegisterWalkModule Mod("dup-default");
    auto *Tab = Mod.addOwnedTable(
        std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::StructRef, 1, 1), /*CanHoldManaged=*/true));
    ASSERT_FALSE(Tab->isManagedByController());

    auto Res = ExecA.registerModule(StoreA, Mod);
    ASSERT_FALSE(Res);
    // Proves the failure came from publication, not from the ownership walk.
    EXPECT_EQ(Res.error(), ErrCode::Value::ModuleNameConflict);

    // Rollback proof 1: the newly-claimed table is un-attached again.
    EXPECT_FALSE(Tab->isManagedByController())
        << "a failed publication must reverse the attach the walk made";
    // Rollback proof 2 (the user-visible symptom): the module is still
    // registrable into a different GC executor. Without the rollback its
    // managed table would still be owned by A, so this would fail the
    // foreign-managed check with IncompatibleImportType.
    EXPECT_TRUE(ExecC.registerModule(StoreC, Mod))
        << "a rejected registration must not durably own the module's roots";
    EXPECT_EQ(StoreC.findModule("dup-default"), &Mod);
    // A's store still holds the original occupier, unchanged.
    EXPECT_EQ(StoreA.findModule("dup-default"), &Occupier);
  }

  // --- Alias-name overload -------------------------------------------------
  {
    WasmEdge::Executor::Executor ExecA(Conf);
    WasmEdge::Executor::Executor ExecC(Conf);
    Runtime::StoreManager StoreA;
    Runtime::StoreManager StoreC;

    RegisterWalkModule Occupier("occupier");
    ASSERT_TRUE(ExecA.registerModule(StoreA, Occupier, "dup-alias"));
    ASSERT_EQ(StoreA.findModule("dup-alias"), &Occupier);

    RegisterWalkModule Mod("other-name");
    auto *Tab = Mod.addOwnedTable(
        std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::StructRef, 1, 1), /*CanHoldManaged=*/true));

    auto Res = ExecA.registerModule(StoreA, Mod, "dup-alias");
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::ModuleNameConflict);

    EXPECT_FALSE(Tab->isManagedByController())
        << "a failed aliased publication must reverse the attach too";
    EXPECT_TRUE(ExecC.registerModule(StoreC, Mod, "fresh-alias"))
        << "a rejected aliased registration must not durably own the roots";
    EXPECT_EQ(StoreA.findModule("dup-alias"), &Occupier);
  }
}

TEST(GCThread, ControllerOwnedTableRejectsRawCApiMutation) {
  // The direct C-API table mutators (WasmEdge_TableInstanceSetData /
  // WasmEdge_TableInstanceGrow) refuse a raw mutation on a table a GC
  // controller manages -- the C-API cannot prove the caller's controller
  // matches the owner. The gate is TableInstance::isManagedByController(); the
  // C-API wrappers do exactly `if (isManagedByController()) reject; else
  // mutate`. The GC suite cannot link libwasmedge, so the gate decision AND the
  // still-open paths are exercised here directly against real TableInstances.
  // Deterministic and single-threaded.
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  const RefVariant Null(ValType(TypeCode::RefNull, TypeCode::NullFuncRef));

  // (a) Managed table attached to a controller-backed allocator: a raw C-API
  // setData/grow is rejected (provenance unprovable).
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::StructRef, 1, 4), /*CanHoldManaged=*/true);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    EXPECT_TRUE(Table.isManagedByController())
        << "managed + controller-attached -> raw C-API mutation rejected";
  }

  // (b) Non-managed table attached to the SAME controller-backed allocator: not
  // managed, so the raw C-API path stays open (this is the existing
  // instantiated externref-table C-API behavior, kept working).
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::FuncRef, 2, 2), /*CanHoldManaged=*/false);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    EXPECT_FALSE(Table.isManagedByController())
        << "attached but non-managed -> C-API mutation still allowed";
    EXPECT_TRUE(Table.setRefAddr(0, Null)); // allowed path really mutates
  }

  // (b2) externref table attached to the same controller: canHoldManaged() is
  // true (extern.convert_any can wrap a GC object), but externref is the host-
  // facing reference primitive -- the standard C-API workflow stores host
  // references through it, so the gate must NOT fire and it stays mutable.
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::ExternRef, 1, 4), /*CanHoldManaged=*/true);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    EXPECT_FALSE(Table.isManagedByController())
        << "externref table stays C-API-mutable despite canHoldManaged()";
    EXPECT_TRUE(Table.setRefAddr(0, Null));
  }

  // (c) Standalone, unattached table (as WasmEdge_TableInstanceCreate builds,
  // even a managed one): no controller owns it, so it mutates freely as before.
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::StructRef, 1, 4), /*CanHoldManaged=*/true);
    EXPECT_FALSE(Table.isManagedByController())
        << "standalone table -> C-API mutation allowed";
    EXPECT_TRUE(Table.growTable(1)); // allowed path really mutates
  }
}

TEST(GCThread, LegacyManagedRefGettersReturnSentinel) {
  // WasmEdge_GlobalInstanceGetValue / WasmEdge_TableInstanceGetData restrict a
  // managed-ref slot's legacy BORROWED getter to a typed-null sentinel plus a
  // warning, instead of handing out a live reference that is not in HostRoots
  // and could be reclaimed while the host still holds it. The gate is
  // `CanHoldManaged() && !RefType.isExternRefType()`
  // (isRestrictedManagedRefGetterType, shared with wasmedge.cpp via
  // include/api/internal/managed_ref_getter.h) and the sentinel is
  // `RefVariant(RefType)`. externref is excluded: it is the host-facing
  // reference primitive whose host-pointer round trip must keep working.
  //
  // The GC suite cannot link libwasmedge, so this test calls the REAL gate
  // predicate against real GlobalInstance/TableInstance objects rather than the
  // exported C symbols -- a mis-wiring in wasmedge.cpp would fail here too. It
  // also confirms the underlying getValue()/getRefAddr() is unchanged and still
  // returns the live borrowed value, and that the sentinel is null and distinct
  // from it. APIUnitTest.cpp drives the exported symbols for the non-managed
  // paths the C-API can construct. Deterministic and single-threaded.

  // A live (non-null) i31 ref: i31 needs no heap allocation (RefI31Op simply
  // tags a small integer as a pointer -- see Executor::runRefI31Op), so it
  // exercises the "managed ref" path without a GC allocator/controller.
  const ValType I31Ty(TypeCode::I31Ref);
  const RefVariant LiveI31(
      I31Ty, reinterpret_cast<void *>(static_cast<uintptr_t>(0x80000001U)));
  ASSERT_FALSE(LiveI31.isNull());
  // Arbitrary non-null host address for the externref cases below (must be a
  // mutable object -- RefVariant's pointer-typed constructor forwards it
  // as-is, so a `const` source would not bind to `void *`).
  int HostDummy = 0;
  void *const HostPtr = &HostDummy;

  // (a) Managed-ref global (i31ref, CanHoldManaged=true, not externref): the
  // gate fires -- WasmEdge_GlobalInstanceGetValue must substitute the
  // sentinel instead of this live value.
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(I31Ty, ValMut::Var), /*CanHoldManagedIn=*/true,
        ValVariant(LiveI31));
    EXPECT_TRUE(Glob.canHoldManaged());
    EXPECT_FALSE(Glob.getGlobalType().getValType().isExternRefType());
    // Assert against the REAL shared predicate (the exact function
    // lib/api/wasmedge.cpp calls), not a hand-rederived copy of the boolean.
    EXPECT_TRUE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "gate condition: canHoldManaged() && !isExternRefType() -> "
           "restricted";
    // getValue() itself is left untouched and still returns the live,
    // non-null borrowed ref -- exactly what the restricted C-API getter must
    // NOT hand back.
    EXPECT_FALSE(Glob.getValue().get<RefVariant>().isNull());
    const RefVariant Sentinel(Glob.getGlobalType().getValType());
    EXPECT_TRUE(Sentinel.isNull())
        << "the documented sentinel is a null ref of the slot's type";
    EXPECT_NE(Glob.getValue().get<RefVariant>().getPtr<void>(),
              Sentinel.getPtr<void>())
        << "the restricted getter's sentinel must differ from the live "
           "borrowed value it replaces";
  }

  // (b) funcref global (CanHoldManaged=false): the gate does not fire --
  // unaffected, returns the value as before.
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::FuncRef), ValMut::Var),
        /*CanHoldManagedIn=*/false);
    EXPECT_FALSE(Glob.canHoldManaged());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "gate condition false -> funcref getter unaffected";
  }

  // (c) externref global: canHoldManaged() is TRUE (extern.convert_any can
  // wrap a GC object), but the scoping decision excludes externref from the
  // sentinel restriction so the standard host-pointer round trip keeps
  // working -- the gate must NOT fire despite CanHoldManaged().
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::ExternRef), ValMut::Var),
        /*CanHoldManagedIn=*/true, ValVariant(RefVariant(HostPtr)));
    EXPECT_TRUE(Glob.canHoldManaged());
    EXPECT_TRUE(Glob.getGlobalType().getValType().isExternRefType());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "gate condition: canHoldManaged() && !isExternRefType() -> false "
           "-> externref getter unaffected (externref exclusion)";
    EXPECT_EQ(Glob.getValue().get<RefVariant>().getPtr<void>(), HostPtr)
        << "externref getter must still return the live host pointer, not "
           "a sentinel";
  }

  // (d) i32 (non-ref) global: never managed, unaffected.
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::I32), ValMut::Var),
        /*CanHoldManagedIn=*/false, ValVariant(UINT32_C(42)));
    EXPECT_FALSE(Glob.canHoldManaged());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()));
    EXPECT_EQ(Glob.getValue().get<uint32_t>(), 42U);
  }

  // (e) Managed-ref table (i31ref element, CanHoldManaged=true): same gate,
  // mirrored for WasmEdge_TableInstanceGetData / TableInstance::getRefAddr.
  {
    Runtime::Instance::TableInstance Table(AST::TableType(I31Ty, 1, 1),
                                           /*CanHoldManaged=*/true);
    ASSERT_TRUE(Table.setRefAddr(0, LiveI31));
    EXPECT_TRUE(Table.canHoldManaged());
    EXPECT_FALSE(Table.getTableType().getRefType().isExternRefType());
    EXPECT_TRUE(isRestrictedManagedRefGetterType(
        Table.canHoldManaged(), Table.getTableType().getRefType()))
        << "gate condition: canHoldManaged() && !isExternRefType() -> "
           "restricted";
    auto Res = Table.getRefAddr(0);
    ASSERT_TRUE(Res);
    EXPECT_FALSE(Res->isNull())
        << "getRefAddr() itself is unchanged and still returns the live "
           "borrowed ref";
    const RefVariant Sentinel(Table.getTableType().getRefType());
    EXPECT_TRUE(Sentinel.isNull());
    EXPECT_NE(Res->getPtr<void>(), Sentinel.getPtr<void>());
  }

  // (f) externref table: canHoldManaged() true, but excluded -- unaffected
  // (this is the shipped tab-ext workflow exercised in APIUnitTest.cpp's
  // WasmEdge_TableInstanceSetData/GetData externref round trip).
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::ExternRef, 1, 1), /*CanHoldManaged=*/true);
    ASSERT_TRUE(Table.setRefAddr(0, RefVariant(HostPtr)));
    EXPECT_TRUE(Table.canHoldManaged());
    EXPECT_TRUE(Table.getTableType().getRefType().isExternRefType());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Table.canHoldManaged(), Table.getTableType().getRefType()))
        << "gate condition false -> externref table getter unaffected";
    auto Res = Table.getRefAddr(0);
    ASSERT_TRUE(Res);
    EXPECT_EQ(Res->getPtr<void>(), HostPtr);
  }

  // (g) funcref table: never managed, unaffected.
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::FuncRef, 1, 1), /*CanHoldManaged=*/false);
    EXPECT_FALSE(Table.canHoldManaged());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Table.canHoldManaged(), Table.getTableType().getRefType()));
  }

  // (h) NON-NULLABLE managed-ref global (i31ref, not nullable,
  // CanHoldManaged=true): the gate still fires -- this documents that the
  // typed-null sentinel is a DELIBERATE choice for this case, not an
  // oversight. A non-nullable managed-ref slot is a real, constructible
  // instance (GlobalInstance/TableInstance require
  // isNullableRefType() || !Val.isNull() at construction, so it must be
  // seeded with a live, non-null value), yet there is no valid *non-null*
  // sentinel we could synthesize -- we cannot fabricate a live managed
  // object out of thin air. Typed-null is therefore the only representable
  // sentinel even though it is not a valid value of the slot's own
  // (non-nullable) type; callers must treat it purely as an
  // error-indicator, never round-trip it back into a constructor. See
  // genManagedRefGetterSentinel's doc-comment in wasmedge.cpp and spec
  // section 4.4.
  {
    const ValType NonNullI31Ty(TypeCode::Ref, TypeCode::I31Ref);
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(NonNullI31Ty, ValMut::Var), /*CanHoldManagedIn=*/true,
        ValVariant(LiveI31));
    EXPECT_TRUE(Glob.canHoldManaged());
    EXPECT_FALSE(Glob.getGlobalType().getValType().isNullableRefType());
    EXPECT_TRUE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "the gate fires for a non-nullable managed slot exactly as it "
           "does for a nullable one -- the sentinel is a documented error "
           "indicator, not intended to be a round-trippable non-null value";
  }
}

TEST(GCThread, RetainedManagedRefGetterSurvivesCollection) {
  // The producer-bearing RETAINED getters
  // (WasmEdge_GlobalInstanceGetValueRetained /
  // WasmEdge_TableInstanceGetDataRetained, lib/api/wasmedge.cpp) are the safe
  // alternative to the legacy borrowed getters: they root a managed
  // struct/array reference handed back to the host by pinning it through
  // Executor::getAllocator().retainResult(ref) -- the EXACT call
  // Executor::invoke makes for a returned GC ref -- so a concurrent collection
  // cannot reclaim it until the host releases it (WasmEdge_ExecutorReleaseRef
  // -> Executor::releaseRef -> Allocator::releaseRef).
  //
  // The GC suite cannot link libwasmedge (see
  // GCThread.ControllerOwnedTableRejectsRawCApiMutation), so it cannot call
  // the exported symbols, and the C-API has no public force-collect. So this
  // test proves the SURVIVES-A-REAL-COLLECTION property the getter relies on
  // by driving the exact underlying calls the getter makes: read the live ref
  // from a real attached GlobalInstance / TableInstance slot
  // (getValue / getRefAddr), retainResult it, DROP the slot's own reference,
  // then force a full collection -- and assert the object survives purely
  // because of the retention, then becomes collectable after releaseRef.
  // test/api/APIUnitTest.cpp
  // (APICoreTest.RetainedManagedRefGettersRootAndRelease) separately drives
  // the exported C symbols for foreign rejection, the retain->release round
  // trip, and non-managed passthrough. Documented split, dictated by what each
  // suite can construct. Deterministic and single-threaded.
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  WasmEdge::Executor::Executor Exec(Conf);
  // Executor::getAllocator() forwards to its controller's allocator -- the
  // same object the retained getters reach via
  // fromExecutorCxt(Cxt)->getAllocator().
  GC::Allocator &Alloc = Exec.getAllocator();
  Alloc.setManualGC(true); // only explicit manualCollect() runs (no flaky auto)

  // Allocate one zero-child GC struct directly through the allocator (Length 0
  // so the collector's child-scan reads no garbage). Mirrors
  // GC.AllocatorHostRootsRetainRelease.
  auto AllocStruct = [&]() noexcept -> RefVariant {
    void *P = Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };
  // A null struct ref to overwrite the slot with (the "drop the source root"
  // step). setValue/setRefAddr's write barrier is a no-op while the collector
  // is Idle (Allocator::writeBarrier), which it always is here between the
  // synchronous manualCollect() calls -- so overwriting does NOT shade the old
  // struct and cannot mask the retention we are testing.
  const RefVariant NullStruct(ValType(TypeCode::RefNull, TypeCode::StructRef));

  // ---- Managed GLOBAL slot ----
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                        ValMut::Var),
        /*CanHoldManagedIn=*/true, ValVariant(AllocStruct()));
    ASSERT_TRUE(Glob.setAllocator(Alloc)); // attach: the slot is now a GC root

    // Read the live ref exactly as the getter does, then retain it exactly as
    // the getter does.
    const RefVariant Ref = Glob.getValue().get<RefVariant>();
    ASSERT_FALSE(Ref.isNull());
    Alloc.retainResult(Ref);

    // Clear the newborn grace period: a just-allocated object survives its very
    // first collect regardless of rootedness (see
    // GC.AllocatorHostRootsRetainRelease). It is still rooted by BOTH the
    // global slot and the retention here.
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u);

    // Drop the slot's reference: the retention is now the ONLY root.
    Glob.setValue(ValVariant(NullStruct));
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u)
        << "the retained managed ref must survive a collection after the "
           "global slot that produced it is cleared";

    // Release the retention -> unrooted -> reclaimed on the next collection.
    Alloc.releaseRef(Ref);
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_EQ(Alloc.getMemoryUsage(), 0u)
        << "after the release path's Allocator::releaseRef, the ref is "
           "collectable";
  }

  // ---- Managed TABLE slot ----
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(ValType(TypeCode::RefNull, TypeCode::StructRef), 1, 1),
        /*CanHoldManaged=*/true);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    ASSERT_TRUE(Table.setRefAddr(0, AllocStruct()));

    auto Res = Table.getRefAddr(0); // exactly what the getter reads
    ASSERT_TRUE(Res);
    const RefVariant Ref = *Res;
    ASSERT_FALSE(Ref.isNull());
    Alloc.retainResult(Ref);

    EXPECT_TRUE(Alloc.manualCollect()); // clear the newborn grace period
    EXPECT_GT(Alloc.getMemoryUsage(), 0u);

    ASSERT_TRUE(Table.setRefAddr(0, NullStruct)); // drop the slot's root
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u)
        << "the retained managed table ref must survive a collection after "
           "the table slot that produced it is cleared";

    Alloc.releaseRef(Ref);
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
  }
}

TEST(GCThread, ConcurrentAllocation) {
  // Multiple threads allocating GC objects simultaneously
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);

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

      Configure TConf;
      TConf.addProposal(Proposal::GC);
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

// Multiple mutators sharing ONE VM / module /
// allocator. Each async task runs on its own operand stack (Executor::invoke
// builds a per-invocation StackManager registered independently with the
// Controller), but all four share the single module instance's global array
// and the one allocator. init() creates the shared 4-element array once; then
// four concurrent write_and_verify calls each write their own index, ATTEMPT a
// collection between write and read (the host coll racing the other mutators
// for the Idle->MarkingRoot CAS, so a given call may lose and collect nothing),
// and TRAP on any mismatch. Across the four racing threads collections do run;
// a torn global ref, a UAF on the shared array, or the array being collected
// out from under a mutator makes the guest trap and the async result an error
// -> EXPECT_TRUE fails. This test races collection against shared-reference
// access; it does not assert a specific write/collect/read interleaving (a
// phase-gated determinism test is a follow-up). It is the truly-shared
// strengthening of the old per-thread-VM test.
TEST(GCThread, SharedReferencesAcrossThreads) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host module must outlive the VM that terminates it; declare first.
  GCFullModule GCMod;
  VM::VM VM(Conf); // ONE VM shared by all mutators
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

// Regression for the teardown-vs-handshake deadlock: tearing down the GC
// controller while an in-flight collection still has a Running-but-unacked
// mutator must NOT hang. The coordinator spins in waitForAcks for that ack; a
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

  // Wait until both mutators are registered and C has raised the stop flag, i.e.
  // C is spinning in waitForAcks on M -- the stable in-flight-handshake state.
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

  // Teardown on its own thread: beginClosing() publishes Closing and then drains
  // until every lease is released -- exactly what ~Controller does at ~VM.
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

  // Release M: it drops its (lingering) Registration and its lease. Now only the
  // coordinator's lease is outstanding -- so whether the drain completes turns
  // entirely on C escaping waitForAcks, which is precisely the fix under test.
  ReleaseM.store(true, std::memory_order_release);

  // Hard watchdog: a deadlock becomes a FAILURE + hard-exit, never a hang. Do
  // NOT join the wedged threads or touch the half-torn Controller on timeout;
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
  // C1 regression: the drain-satisfying decrement + notify must happen while
  // holding DrainMtx, and must be complete before the releaser drops the lock.
  // If they are, the teardown drain in beginClosing() provably cannot return
  // while the final releaser is still inside its DrainMtx critical section --
  // so ~Controller cannot free DrainMtx/DrainCV under the releaser (the UAF).
  // The enforcing signal for the raw UAF is the neihu1 TSan/ASan gate; this
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
  // hook WHILE holding DrainMtx (after notify_all), and blocks there.
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

  // The releaser holds DrainMtx; the drain CANNOT reacquire it to return.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(ClosingDone.load(std::memory_order_acquire));

  // Let the releaser drop DrainMtx; the drain now wakes, sees count 0, returns.
  HookRelease.store(true, std::memory_order_release);
  Releaser.join();
  Teardown.join();
  EXPECT_TRUE(ClosingDone.load(std::memory_order_acquire));
  Ctrl.reset();
}

// Multi-mutator allocation / GC stress on one
// allocator. Eight concurrent mutators on ONE VM each run a bounded loop
// allocating 500 GC arrays of size 64 (alloc_loop is deterministic and cannot
// hang), then call gc.coll once at the end. Together with the allocator's own
// auto-collection and the concurrent registered stacks this exercises the
// allocate/mark/sweep paths across mutators; the run must be UAF- and race-free
// under ASan/TSan. NOTE: phase-gated determinism for allocate-during-sweep
// lives in GC.AllocateDuringSweepSurvives; this stress adds the
// multi-mutator scheduling dimension on top.
TEST(GCThread, MultiMutatorAllocationStress) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
  // Checking Stop only AFTER a grow keeps the race under test alive even when
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
  // ownership CAS can win LATE -- after the in-cycle owner already swept and
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

  // Baseline AFTER the cycle: on a many-core box the same two-owners-in-turn
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
  // The per-controller exclusive-operation owner must admit at most ONE owner at
  // a time under contention. N registered mutators each loop acquiring the token
  // (OwnedGrowing), briefly hold it while asserting no other thread holds it,
  // then release it through the reserved FIFO handoff. A shared high-water mark
  // proves the single-owner invariant (max concurrent owners == 1); a hard
  // watchdog turns a deadlock (a bug wedging the handoff) into a FAILURE +
  // hard-exit rather than an indefinite hang.
  GC::Controller Ctrl;

  constexpr uint32_t NumThreads = 8;
  constexpr uint32_t Iterations = 2000;

  std::atomic<int> Inside{0};    // owners currently in the critical section
  std::atomic<int> MaxInside{0}; // high-water mark of Inside (must stay 1)
  std::atomic<uint32_t> Done{0}; // workers that finished their loop
  std::atomic<bool> Failed{false};

  auto Worker = [&]() {
    // Register a value stack so the loser protocol's Blocked publish + admission
    // exercise the real RegistryMtx path (setSelfBlocked / admitToState).
    Runtime::StackManager StackMgr(Ctrl);
    for (uint32_t I = 0; I < Iterations; ++I) {
      uint64_t Gen = 0;
      if (!Ctrl.beginExclusiveOp(
              GC::Controller::ExclusiveOwner::State::OwnedGrowing, Gen)) {
        // beginExclusiveOp only returns false while Closing, which never happens
        // in this test -- treat it as a failure.
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
      Ctrl.endExclusiveOp(
          Gen, GC::Controller::ExclusiveOwner::State::OwnedGrowing);
    }
    Done.fetch_add(1, std::memory_order_acq_rel);
  };

  std::vector<std::thread> Threads;
  for (uint32_t I = 0; I < NumThreads; ++I) {
    Threads.emplace_back(Worker);
  }

  // Hard watchdog: a deadlock becomes a FAILURE + hard-exit, never a hang. Do
  // NOT join the wedged threads on timeout -- _Exit skips the blocking joins.
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
  // WHOLE cycle, so a stop-the-world grow and a collection can never both stop
  // the world at once. Drive a REAL collection that holds the token (a worker
  // pinned inside its sweep hook -- token still held, Idle not yet published,
  // endExclusiveOp not yet called), then have a separate "grower" thread call
  // beginExclusiveOp(OwnedGrowing) directly (growTable is not wired to the
  // arbiter here). The grower must PARK -- not acquire -- until the
  // collection releases the token, then acquire through the reserved FIFO
  // handoff. Proven deterministically with the queued-waiter counter, never
  // sleeps; a background watchdog turns a deadlock into a FAILURE + hard-exit
  // (never an indefinite hang), and _Exit skips the wedged joins below.
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  Alloc.setManualGC(true); // no auto cycle may race our single collect()

  // Pin the sweep-completing worker inside runSweepAndSwap: at this point the
  // collection still owns the token (endExclusiveOp runs only AFTER the sweep
  // returns), Idle is not yet published, and the cycle cannot complete -- so the
  // collection HOLDS the token for as long as we hold the hook.
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
  // reached) becomes a FAILURE + hard-exit rather than an indefinite hang.
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

  // Grower thread: attempt to acquire the token for a (simulated) grow. It loses
  // (token is OwnedCollecting), publishes Blocked, enqueues a FIFO ticket, and
  // parks on TokenCV -- it must not proceed until the collection releases.
  std::atomic<bool> GrowerAcquired{false};
  std::thread Grower([&]() {
    // Register a stack so the loser path exercises the real Blocked publish +
    // admission (setSelfBlocked / admitToState), matching a real grower.
    Runtime::StackManager StackMgr(Ctrl);
    uint64_t Gen = 0;
    ASSERT_TRUE(Ctrl.beginExclusiveOp(
        GC::Controller::ExclusiveOwner::State::OwnedGrowing, Gen));
    GrowerAcquired.store(true, std::memory_order_release);
    Ctrl.endExclusiveOp(Gen,
                        GC::Controller::ExclusiveOwner::State::OwnedGrowing);
  });

  // Deterministically wait until the grower is queued (parked on TokenCV): once
  // its ticket is enqueued it is a committed loser that cannot proceed until the
  // reserved handoff grants it the token.
  while (Ctrl.debugExclusiveWaiters() == 0) {
    std::this_thread::yield();
  }

  // The collection still holds the token, so the grower MUST NOT have acquired.
  EXPECT_FALSE(GrowerAcquired.load(std::memory_order_acquire));
  EXPECT_EQ(Ctrl.debugExclusiveWaiters(), 1u);

  // Release the pinned sweep: the sweep-completing worker finishes the cycle and
  // calls endExclusiveOp(OwnedCollecting), handing the token to the queued
  // grower. Only now may the grower acquire.
  ReleaseSweep.store(true, std::memory_order_release);

  Collector.join();
  Grower.join();

  // The grower acquired -- but only AFTER the collection released the token.
  EXPECT_TRUE(GrowerAcquired.load(std::memory_order_acquire));

  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
  Alloc.setSweepPauseHook(nullptr);
}

TEST(GC, VMReleaseAllRefsArray) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  // Allocate two arrays (length 4) and keep both refs alive.
  auto R1 =
      VM.execute("make_arr", std::initializer_list<ValVariant>{UINT32_C(4)},
                 {ValType(TypeCode::I32)});
  auto R2 =
      VM.execute("make_arr", std::initializer_list<ValVariant>{UINT32_C(4)},
                 {ValType(TypeCode::I32)});
  ASSERT_TRUE(R1);
  ASSERT_TRUE(R2);
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // both retained survive

  VM.releaseAllRefs();
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before); // both reclaimed
}

TEST(GC, VMReleaseRefsBatch) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto R1 = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(1)},
                       {ValType(TypeCode::I32)});
  auto R2 = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(2)},
                       {ValType(TypeCode::I32)});
  ASSERT_TRUE(R1);
  ASSERT_TRUE(R2);
  ASSERT_EQ(R1->size(), 1u);
  ASSERT_EQ(R2->size(), 1u);
  std::array<RefVariant, 2> Refs{(*R1)[0].first.get<RefVariant>(),
                                 (*R2)[0].first.get<RefVariant>()};

  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  VM.releaseRefs(Refs);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before);
}

TEST(GC, NonGCResultNotRetained) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  // Returns an i31 (non-heap) after allocating and dropping a struct; neither
  // the i31 nor the dropped struct should be retained.
  auto Res = VM.execute("drop_return_i31");
  ASSERT_TRUE(Res);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(),
            Before); // dropped struct reclaimed, i31 not retained
}

TEST(GC, ComponentValueReleaseOverload) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto Res = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(42)},
                        {ValType(TypeCode::I32)});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  // Wrap the same retained ValVariant in a component value and release via the
  // component overload (mirrors what executeComponent hands the host).
  ComponentValVariant CV{(*Res)[0].first};
  VM.releaseRef(CV);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before);
}

TEST(GC, ExternalizedRefRetained) {
  // An externalized gc struct (extern.convert_any) returned to the host must be
  // retained as a host root. It is typed externref but still points to a
  // collectible object; before the fix it escaped the struct/array retain check
  // because the type was folded to externref first.
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(MakeExtWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto Res =
      VM.execute("make_ext", std::initializer_list<ValVariant>{UINT32_C(42)},
                 {ValType(TypeCode::I32)});
  ASSERT_TRUE(Res);
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // externalized struct allocated

  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // retained across collections

  VM.releaseAllRefs();
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before); // reclaimed once released
}

TEST(GC, CoherentSlotPreservesExternalizedRefUnderTrace) {
  // A managed slot holds a 128-bit RefVariant: Raw[0] is the type tag (with the
  // Externalize bit), Raw[1] is the object pointer. While the marker reads such
  // a slot, a mutator may struct.set/array.set the same slot. Two independent
  // word accesses could fabricate a torn (type, pointer) pair -- e.g. the
  // externalized type of one value glued to the pointer of the other -- which
  // the runtime would then dereference. The coherent accessors must make every
  // full-reference read return a WHOLE prior/next value, never a mixed pair.
  //
  // On Windows (no TSan) this verifies the coherence LOGIC: a reader running the
  // marker path plus loadCoherent must NEVER observe a pair that is neither of
  // the two whole values the writer alternates. (The data-race cleanliness is
  // the separate Linux TSan gate.)
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
  // ValB: externalized ref (Externalize bit set), ptr = ObjB. extern.convert_any
  // folds a gc ref to externref with the Externalize bit set; emulate that here.
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
  // Torn-pair detectability requires BOTH words to differ: otherwise a torn
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
      // Full-reference coherent read: must equal a WHOLE prior/next value.
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

TEST(GC, ElemSegmentRoot) {
  // A passive element segment holds the only reference to a struct created in
  // its init expression; it must be scanned as a GC root or the struct is swept
  // and array.new_elem later reads a dangling pointer.
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Host modules must outlive the VM that terminates them; declare first.
  auto GCMod = std::make_unique<Runtime::Instance::ModuleInstance>("gc");
  GCMod->addHostFunc("coll", std::make_unique<Collect>());
  auto CP = std::make_unique<Check>();
  auto *C = CP.get();
  GCMod->addHostFunc("check", std::move(CP));
  VM::VM VM(Conf);
  VM.registerModule(*GCMod);

  ASSERT_TRUE(VM.loadWasm(ElemRootWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);
  ASSERT_TRUE(VM.execute("test"));

  auto Values = C->getValues();
  ASSERT_EQ(Values.size(), 1u);
  // 42 == the struct field read back after two collections; garbage/crash if
  // the element segment was not scanned and the struct was reclaimed.
  EXPECT_EQ(Values[0], 42u);
}

TEST(GC, ExceptionPayloadRoot) {
  // A struct reference captured in an exception payload survives only via the
  // ExceptionInstance once the on-stack copies are consumed; the payload must
  // be scanned as a GC root (throw_ref re-pushes it). Construct the instance
  // directly to isolate the root-scanning behavior.
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;
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

  AST::TagType DummyTag;
  Runtime::Instance::TagInstance Tag(DummyTag, nullptr);
  std::vector<ValVariant> Payload;
  Payload.emplace_back(Ref);
  Runtime::Instance::ExceptionInstance Exc(&Tag, std::move(Payload));
  Exc.setAllocator(Alloc);

  EXPECT_TRUE(Alloc.manualCollect()); // payload scanned -> struct survives
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
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
  // starting collect(), so the mutator is provably looping (and thus WOULD set
  // MutatorRanDuringSnapshot if it failed to park during the snapshot window).
  std::atomic<bool> MutatorEnteredLoop{false};

  std::thread Mutator([&] {
    std::vector<WasmEdge::ValVariant> S2;
    auto R2 = Ctrl.registerStack(S2);

    while (!Stop.load()) {
      MutatorEnteredLoop.store(true);
      if (InSnapshot.load()) {
        MutatorRanDuringSnapshot.store(true); // must NOT happen while parked
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

  // Mutator: registers a stack, roots a freshly-allocated GC object ONLY on
  // that registered stack (nowhere else), then enters a "host call"
  // (NativeScope) and stays there. It never reaches a safe point, so the
  // coordinator must (a) NOT wait on it (no hang) AND (b) still scan its stable
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
    S.emplace_back(Ref); // the ONLY root for this object

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
  // 0 -- the survival assertion below then goes RED.
  std::thread Collector([&] {
    EXPECT_TRUE(Ctrl.collect(true, false));
    EXPECT_TRUE(Ctrl.collect(true, false));
    Collected.store(true);
  });

  Collector.join(); // hangs here if waitForAcks waits on NativeRunning
  EXPECT_TRUE(Collected.load());

  // The object was rooted ONLY on the native-running thread's registered stack;
  // its survival proves scanNonRunningRoots scanned that stack in place.
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject);

  LetNativeFinish.store(true);
  Mutator.join();
}

// AOT shadow-root prototype: a managed ref held ONLY in a native-frame
// shadow slot (never on the value stack) of a NativeRunning thread must survive
// a remote collection -- proving scanNonRunningRoots walks the shadow chain.
// This is the AOT analog of CollectDoesNotHangOnNativeRunningMutator (which
// roots on the value stack). If scanShadowChain were a no-op the object is
// swept in cycle 2 and the survival assertion goes RED.
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

    // The ONLY root for this object: a shadow slot, NOT the value stack S.
    // S stays empty -- so survival can only come from the shadow chain.
    GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
    ASSERT_NE(H, nullptr);
    std::vector<ValVariant> Slots;
    Slots.emplace_back(Ref);
    GC::Controller::ShadowFrame Frame;
    // Push while Running (chain mutation is only legal in a non-scannable
    // state), then transition to NativeRunning. Destruction order (reverse)
    // pops the frame only AFTER NativeScope restores Running.
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
// shape (a thread_local vector of ValVariant whose address is fixed for the
// thread's lifetime), so the aux-root provider contract is exercised exactly as
// Executor::pendingExnRoots implements it.
thread_local std::vector<ValVariant> TestAuxRoots;
const std::vector<ValVariant> *testAuxRootProvider() noexcept {
  return &TestAuxRoots;
}
} // namespace

// A managed ref held ONLY in a thread's auxiliary root vector must survive a
// remote collection while that thread sits in a host call. This is the shape of
// a propagating exception's payload: throwException copies it off the value
// stack and then erases those slots, so the aux roots are its only root until a
// handler re-pushes it. NativeRunning is what makes the assertion falsifiable
// -- a remote scan performs no conservative native scan of that thread, so
// without scanAuxRoots in scanNonRunningRoots the object is swept in cycle 2
// and the survival assertion goes RED.
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
    // The ONLY root for this object: the aux vector, NOT the value stack S,
    // which stays empty.
    TestAuxRoots.emplace_back(Ref);

    GC::Controller::NativeScope Native(Ctrl);
    InNative.store(true);
    while (!LetNativeFinish.load()) {
      std::this_thread::yield();
    }
    TestAuxRoots.clear();
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

// The coordinator's OWN aux roots must be scanned by its self-scan. Driving
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
  TestAuxRoots.emplace_back(RefVariant(
      ValType(TypeCode::Ref, TypeCode::StructRef), static_cast<RawData *>(P)));

  const uint64_t UsageWithObject = Alloc.getMemoryUsage();
  ASSERT_GT(UsageWithObject, 0u);
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject);

  TestAuxRoots.clear();
  // Dropped from the aux roots, the object is now unreachable and must go.
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_LT(Alloc.getMemoryUsage(), UsageWithObject);
}

// Wiring: the executor installs its pending-exception payload as the aux-root
// provider, so every mutator stack it registers publishes THAT thread's payload
// to the registry. This is what roots the payload's managed refs while an
// exception propagates out through the native frames -- the values are erased
// from the value stack the moment the pending record is set.
TEST(GC, PendingExceptionPayloadIsRegisteredAuxRoot) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

// The stable shadow-head cell must keep a FIXED address across Entries-vector
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
// its boundary value BEFORE the longjmp, so no scanner walks a ShadowFrame in
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
  // Arm exactly as helper.cpp does at the compiled-call boundary, BEFORE any
  // frame is published: record the head cell + its boundary value.
  F.armShadowRestore(reinterpret_cast<std::atomic<void *> *>(&H->Head),
                     H->Head.load(std::memory_order_relaxed));

  volatile int Reached = 0;
  if (PREPARE_FAULT(F) == 0) {
    // "compiled frame": publish a shadow frame whose storage is in THIS scope,
    // then trap. The longjmp abandons the scope without running the pop.
    ValVariant Slots[1] = {};
    GC::Controller::ShadowFrame Frame;
    Frame.Prev = H->Head.load(std::memory_order_relaxed);
    Frame.Count = 1;
    Frame.Slots = Slots;
    H->Head.store(&Frame, std::memory_order_release);
    ASSERT_EQ(H->Head.load(std::memory_order_acquire), &Frame); // published
    Reached = 1;
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds);        // longjmp back
    FAIL() << "emitFault must not return";
  } else {
    EXPECT_EQ(Reached, 1);
    // Truncated to the boundary: the head no longer references the abandoned
    // frame that lived in the now-unwound scope.
    EXPECT_EQ(H->Head.load(std::memory_order_acquire), nullptr);
  }
}

// Positive control for the truncation test: with the Fault NOT armed,
// emitFault's longjmp leaves the head pointing at the abandoned compiled frame
// -- exactly the dangling state truncation fixes, and what the recovery C++
// would then clobber. This asserts the dangling ADDRESS (never dereferences
// it), so it stays memory-safe.
TEST(GCThread, FaultLeavesShadowHeadDanglingWithoutTruncation) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  GC::Controller::ShadowHead *H = Ctrl.currentShadowHead();
  ASSERT_NE(H, nullptr);

  Fault F; // deliberately NOT armed -> no truncation
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
    // Repair the chain so Controller teardown does not observe the dangling head.
    H->Head.store(nullptr, std::memory_order_release);
  }
}

// Nested compiled boundaries each truncate to THEIR OWN boundary value. An
// inner fault resets the head to the frame the OUTER boundary published, not
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
    // Truncated to the OUTER boundary, not to null: the outer frame is intact.
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
// built-in positive control: the entry IS stranded until the fix runs.
TEST(GCThread, StateRestoreAfterFaultUnstrandsNativeRunning) {
  GC::Controller Ctrl;
  std::vector<ValVariant> S;
  auto Reg = Ctrl.registerStack(S);
  ASSERT_EQ(Ctrl.debugSelfState(), GC::Controller::MutatorState::Running);

  // Capture the boundary state as helper.cpp does before the compiled call.
  const GC::Controller::MutatorState Boundary = Ctrl.currentMutatorState();
  ASSERT_EQ(Boundary, GC::Controller::MutatorState::Running);

  Fault F; // unarmed: no shadow head to truncate here, only state to restore
  if (PREPARE_FAULT(F) == 0) {
    GC::Controller::NativeScope Native(Ctrl); // host call -> NativeRunning
    ASSERT_EQ(Ctrl.debugSelfState(),
              GC::Controller::MutatorState::NativeRunning);
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds); // skips Native's dtor
    FAIL() << "emitFault must not return";
  } else {
#if defined(_WIN32)
    // The Windows CRT's longjmp UNWINDS the frame, so NativeScope's dtor does
    // run and the entry is never stranded. The strand is what the restore below
    // repairs on POSIX; here only that restore is meaningful.
    const auto AfterFault = Ctrl.debugSelfState();
    EXPECT_TRUE(AfterFault == GC::Controller::MutatorState::NativeRunning ||
                AfterFault == GC::Controller::MutatorState::Running);
#else
    // Positive control: the skipped dtor left the entry stranded NativeRunning.
    EXPECT_EQ(Ctrl.debugSelfState(),
              GC::Controller::MutatorState::NativeRunning);
#endif
    // The fix: recovery restores the boundary state.
    Ctrl.restoreStateAfterFault(Boundary);
    EXPECT_EQ(Ctrl.debugSelfState(), GC::Controller::MutatorState::Running);
  }
}

// When the boundary was itself NativeRunning (a host->guest
// reentry -- the outer host call is still active), the fault recovery must
// restore TO NativeRunning, not force Running, or the still-live outer native
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
  EXPECT_EQ(Ctrl.debugSelfState(), GC::Controller::MutatorState::NativeRunning);
  // Native's dtor restores Running normally at scope end.
}

// AOT codegen thesis: a JIT-compiled function allocates a struct into a
// ref LOCAL, then calls a host function that triggers GC. With ScanNative=false
// the native-stack alloca is NOT conservatively scanned, so the struct survives
// ONLY if the compiler-emitted shadow spill published the ref for the
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);

  // Declare the host module BEFORE the VM so it is destroyed AFTER the VM's
  // instantiated module, which depends on it for the "collect" import. The
  // reverse order trips ~ModuleInstance's !hasDependents() assert (Debug/UBSan).
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
  // The struct was reachable ONLY through the codegen shadow spill during the
  // 2nd collect; surviving usage proves the compiler-emitted spill was scanned.
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u);
}

// Ref-PARAM coverage: run passes a freshly-allocated struct DIRECTLY to $hold
// (no local -- the value is only on run's operand stack, which is not spilled),
// and $hold holds it as a ref PARAM across a GC. The struct survives only if the
// compiler spills ref params (not just locals).
// (module (type $s (struct (field i32))) (import "host" "collect" (func))
//  (func $hold (param (ref null $s)) (call 0))
//  (func (export "run") (call $hold (struct.new_default $s))))
const std::array<WasmEdge::Byte, 110> ParamSpillWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x01, 0x63, 0x00, 0x00, 0x02,
    0x10, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x07, 0x63, 0x6f, 0x6c, 0x6c,
    0x65, 0x63, 0x74, 0x00, 0x01, 0x03, 0x03, 0x02, 0x02, 0x01, 0x07, 0x07,
    0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02, 0x0a, 0x0e, 0x02, 0x04, 0x00,
    0x10, 0x00, 0x0b, 0x07, 0x00, 0xfb, 0x01, 0x00, 0x10, 0x01, 0x0b, 0x00,
    0x25, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x10, 0x02, 0x00, 0x07, 0x63,
    0x6f, 0x6c, 0x6c, 0x65, 0x63, 0x74, 0x01, 0x04, 0x68, 0x6f, 0x6c, 0x64,
    0x02, 0x06, 0x01, 0x01, 0x01, 0x00, 0x01, 0x72, 0x04, 0x04, 0x01, 0x00,
    0x01, 0x73};

TEST(GCThread, ShadowSpillKeepsRefParamAcrossCompiledCall) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
  // Reachable ONLY via $hold's ref-param shadow spill during the 2nd collect.
  EXPECT_GT(ShadowSpillUsageAfter.load(std::memory_order_relaxed), 0u);
}

// call_ref coverage: a ref local held across a call_ref to a host function
// (which routes through the slow kCallRef proxy -> NativeRunning). Survival
// proves the call_ref emission site spills, not just direct calls.
// (module (type $s (struct (field i32))) (type $ft (func))
//  (import "host" "collect" (func $collect (type $ft))) (elem declare func $collect)
//  (func (export "run") (local $r (ref null $s))
//    (local.set $r (struct.new_default $s)) (call_ref $ft (ref.func $collect))))
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
// function (in a table). Survival proves the call_indirect emission site spills.
// (module (type $s (struct (field i32))) (type $ft (func))
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

// Positive control for the WASMEDGE_JIT_TSAN JIT instrumentation.
// (module (memory (export "mem") 1)
//  (func (export "spin") (param $n i32) (local $i i32)
//    (loop $l (i32.store (i32.const 0) (local.get $i))
//      (local.set $i (i32.add (local.get $i) (i32.const 1)))
//      (br_if $l (i32.lt_u (local.get $i) (local.get $n))))))
const std::array<WasmEdge::Byte, 98> SpinStoreWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x01, 0x7f, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01,
    0x07, 0x0e, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x04, 0x73, 0x70,
    0x69, 0x6e, 0x00, 0x00, 0x0a, 0x1e, 0x01, 0x1c, 0x01, 0x01, 0x7f, 0x03,
    0x40, 0x41, 0x00, 0x20, 0x01, 0x36, 0x02, 0x00, 0x20, 0x01, 0x41, 0x01,
    0x6a, 0x21, 0x01, 0x20, 0x01, 0x20, 0x00, 0x49, 0x0d, 0x00, 0x0b, 0x0b,
    0x00, 0x18, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x02, 0x09, 0x01, 0x00, 0x02,
    0x00, 0x01, 0x6e, 0x01, 0x01, 0x69, 0x03, 0x06, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x6c};

// A compiled function writes wasm linear memory[0] in a loop (an instrumented
// JIT store) while a C++ thread reads the same byte (instrumented by
// -fsanitize=thread). This is an INTENTIONAL data race, DISABLED so it never
// runs in normal suites. Run explicitly under a TSan build with
// WASMEDGE_JIT_TSAN=1: TSan MUST report a race, which is only possible if the
// JIT store itself is instrumented -- without JIT instrumentation TSan sees
// only the C++ read and reports nothing. This is the positive control proving
// the JIT ThreadSanitizer pass is effective, not silently inert.
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
  auto R = VM.execute("spin",
                      std::initializer_list<ValVariant>{UINT32_C(50000000)},
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
// full protocol end-to-end; under an instrumented-JIT TSan build
// (WASMEDGE_JIT_TSAN=1) it also checks the codegen publish store and the
// scanner's read are properly synchronized (via RegistryMtx) -- expected clean.
TEST(GCThread, ShadowSpillProtocolUnderConcurrentCollect) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
// concurrent collection's stop-the-world complete against a COMPUTE-ONLY
// compiled loop. `run` signals started (mem[4]) then spins in a pure loop -- a
// single shared-memory atomic load, NO host call, so the loop-back-edge poll is
// the ONLY possible yield point -- until mem[0]!=0. The collector thread waits
// until the mutator is spinning, runs ONE STW collect (which completes only if
// the spinning mutator reaches a safepoint and acks), and ONLY THEN sets mem[0]
// to release the loop. Without the poll this DEADLOCKS: the collect waits for
// an ack that never comes, and the release is gated on the collect returning.
// With it, the mutator parks+acks, the collect completes, the loop is released,
// and run returns. Shared memory + atomic accesses keep the signals race-free
// (TSan-clean); the yield point is genuinely the codegen poll, not a host call.
// (module (memory (export "mem") 1 1 shared)
//  (func (export "run")
//    (i32.atomic.store (i32.const 4) (i32.const 1))   ;; signal started
//    (loop $l (br_if $l (i32.eqz (i32.atomic.load (i32.const 0)))))))
const std::array<WasmEdge::Byte, 80> ComputeSpinWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   4,   1,   96,  0,  0,   3,
    2,   1,   0,   5,   4,   1,   3,   1,   1,   7,   13,  2,   3,  109, 101,
    109, 2,   0,   3,   114, 117, 110, 0,   0,   10,  24,  1,   22, 0,   65,
    4,   65,  1,   254, 23,  2,   0,   3,   64,  65,  0,   254, 16, 2,   0,
    69,  13,  0,   11,  11,  0,   13,  4,   110, 97,  109, 101, 3,  6,   1,
    0,   1,   0,   1,   108};

TEST(GCThread, CompiledLoopYieldsToConcurrentCollect) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

// Two mutators sharing one reference-typed global. In the multi-mutator model
// async invocations share the same GlobalInstance, so one thread's compiled
// global.set of a struct ref runs concurrently with another thread's global.get
// of the same 128-bit (type, pointer) slot. With the coherent store and load
// (kCoherentRefStore/kCoherentRefLoad) the shared-slot access is race-free and
// can never form a torn pair; the earlier codegen (a bare 128-bit load/store)
// tears the pair and TSan flags the data race. (The marker is
// NOT the racing reader here: mutators park at the STW root snapshot before
// scanSharedRoots runs, so global tearing is genuinely mutator-vs-mutator.)
// Validated race-clean under TSan; the negative control (plain store/load)
// reports the race.
//   (global $g (mut (ref null $s)) ...)
//   (func (export "set") (param $n) <loop: global.set $g = struct.new>)
//   (func (export "get") (param $n) (result i32) <loop: acc +=
//   !ref.is_null(global.get $g)>)
const std::array<WasmEdge::Byte, 172> RefGlobalConcurrentWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   14,  3,   95,  1,   127, 0,
    96,  1,   127, 0,   96,  1,   127, 1,   127, 3,   3,   2,   1,   2,   6,
    7,   1,   99,  0,   1,   208, 0,   11,  7,   13,  2,   3,   115, 101, 116,
    0,   0,   3,   103, 101, 116, 0,   1,   10,  61,  2,   26,  1,   1,   127,
    3,   64,  251, 1,   0,   36,  0,   32,  1,   65,  1,   106, 33,  1,   32,
    1,   32,  0,   73,  13,  0,   11,  11,  32,  1,   2,   127, 3,   64,  32,
    2,   35,  0,   209, 69,  106, 33,  2,   32,  1,   65,  1,   106, 33,  1,
    32,  1,   32,  0,   73,  13,  0,   11,  32,  2,   11,  0,   54,  4,   110,
    97,  109, 101, 2,   22,  2,   0,   2,   0,   1,   110, 1,   1,   105, 1,
    3,   0,   1,   110, 1,   1,   105, 2,   3,   97,  99,  99,  3,   11,  2,
    0,   1,   0,   1,   108, 1,   1,   0,   1,   108, 4,   4,   1,   0,   1,
    115, 7,   4,   1,   0,   1,   103};

TEST(GCThread, CoherentRefGlobalConcurrentAccess) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

  auto Setter =
      VM.asyncExecute("set", std::initializer_list<ValVariant>{UINT32_C(300000)},
                      {ValType(TypeCode::I32)});
  auto Getter =
      VM.asyncExecute("get", std::initializer_list<ValVariant>{UINT32_C(300000)},
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
    0,   97,  115, 109, 1,   0,   0,   0,   1,   14,  3,   95,  1,   127, 0,
    96,  1,   127, 0,   96,  1,   127, 1,   127, 3,   3,   2,   1,   2,   4,
    5,   1,   99,  0,   0,   1,   7,   13,  2,   3,   115, 101, 116, 0,   0,
    3,   103, 101, 116, 0,   1,   10,  65,  2,   28,  1,   1,   127, 3,   64,
    65,  0,   251, 1,   0,   38,  0,   32,  1,   65,  1,   106, 33,  1,   32,
    1,   32,  0,   73,  13,  0,   11,  11,  34,  1,   2,   127, 3,   64,  32,
    2,   65,  0,   37,  0,   209, 69,  106, 33,  2,   32,  1,   65,  1,   106,
    33,  1,   32,  1,   32,  0,   73,  13,  0,   11,  32,  2,   11,  0,   54,
    4,   110, 97,  109, 101, 2,   22,  2,   0,   2,   0,   1,   110, 1,   1,
    105, 1,   3,   0,   1,   110, 1,   1,   105, 2,   3,   97,  99,  99,  3,
    11,  2,   0,   1,   0,   1,   108, 1,   1,   0,   1,   108, 4,   4,   1,
    0,   1,   115, 5,   4,   1,   0,   1,   116};

TEST(GCThread, CoherentRefTableConcurrentAccess) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

  auto Setter =
      VM.asyncExecute("set", std::initializer_list<ValVariant>{UINT32_C(300000)},
                      {ValType(TypeCode::I32)});
  auto Getter =
      VM.asyncExecute("get", std::initializer_list<ValVariant>{UINT32_C(300000)},
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
    0,   97,  115, 109, 1,   0,   0,   0,   1,   14,  3,   95,  1,   127, 0,
    96,  1,   127, 0,   96,  1,   127, 1,   127, 3,   3,   2,   1,   2,   4,
    5,   1,   99,  0,   0,   1,   7,   15,  2,   4,   103, 114, 111, 119, 0,
    0,   4,   114, 101, 97,  100, 0,   1,   10,  66,  2,   29,  1,   1,   127,
    3,   64,  208, 0,   65,  1,   252, 15,  0,   26,  32,  1,   65,  1,   106,
    33,  1,   32,  1,   32,  0,   73,  13,  0,   11,  11,  34,  1,   2,   127,
    3,   64,  32,  2,   65,  0,   37,  0,   209, 69,  106, 33,  2,   32,  1,
    65,  1,   106, 33,  1,   32,  1,   32,  0,   73,  13,  0,   11,  32,  2,
    11,  0,   54,  4,   110, 97,  109, 101, 2,   22,  2,   0,   2,   0,   1,
    110, 1,   1,   105, 1,   3,   0,   1,   110, 1,   1,   105, 2,   3,   97,
    99,  99,  3,   11,  2,   0,   1,   0,   1,   108, 1,   1,   0,   1,   108,
    4,   4,   1,   0,   1,   115, 5,   4,   1,   0,   1,   116};

TEST(GCThread, GrowTableRetiresBufferUnderConcurrentReaders) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

  auto Grower =
      VM.asyncExecute("grow", std::initializer_list<ValVariant>{UINT32_C(20000)},
                      {ValType(TypeCode::I32)});
  auto Reader =
      VM.asyncExecute("read", std::initializer_list<ValVariant>{UINT32_C(20000)},
                      {ValType(TypeCode::I32)});
  auto RGrow = Grower.get();
  auto RRead = Reader.get();
  Done.store(true, std::memory_order_relaxed);
  Collector.join();
  EXPECT_TRUE(RGrow);
  EXPECT_TRUE(RRead);
}

// An INTERPRETER table.grow reallocates the Refs vector object that a
// concurrent INTERPRETER table.get/table.set reads (getRefAddr/setRefAddr read
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
    0, 97, 115, 109, 1, 0, 0, 0, 1, 10, 2, 96, 1, 127, 0, 96,
    1, 127, 1, 127, 3, 3, 2, 0, 1, 4, 7, 1, 112, 1, 1, 160,
    141, 6, 7, 17, 2, 4, 103, 114, 111, 119, 0, 0, 6, 103, 101, 116,
    115, 101, 116, 0, 1, 10, 81, 2, 34, 1, 1, 127, 2, 64, 3, 64,
    32, 1, 32, 0, 79, 13, 1, 208, 112, 65, 1, 252, 15, 0, 26, 32,
    1, 65, 1, 106, 33, 1, 12, 0, 11, 11, 11, 44, 1, 2, 127, 2,
    64, 3, 64, 32, 1, 32, 0, 79, 13, 1, 32, 2, 65, 0, 37, 0,
    209, 106, 33, 2, 65, 0, 208, 112, 38, 0, 32, 1, 65, 1, 106, 33,
    1, 12, 0, 11, 11, 32, 2, 11, 0, 60, 4, 110, 97, 109, 101, 2,
    22, 2, 0, 2, 0, 1, 110, 1, 1, 105, 1, 3, 0, 1, 110, 1,
    1, 105, 2, 3, 97, 99, 99, 3, 23, 2, 0, 2, 0, 4, 100, 111,
    110, 101, 1, 1, 108, 1, 2, 0, 4, 100, 111, 110, 101, 1, 1, 108,
    5, 4, 1, 0, 1, 116};

TEST(GCThread, InterpreterGrowExcludesConcurrentReader) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

  // Background watchdog: a wedged handshake (grower's waitForAcks never satisfied
  // or a reader that never parks) becomes a FAILURE + hard-exit, never a hang.
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

  auto Grower =
      VM.asyncExecute("grow", std::initializer_list<ValVariant>{UINT32_C(20000)},
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
  // token. Growers use the BLOCKING acquire (they must eventually grow), so a
  // losing grower parks on a FIFO ticket and, on wake, OWNS the token and runs
  // its own STW; the collector uses the NON-blocking acquire and simply skips a
  // cycle when a grow holds the token. The invariants: (1) no deadlock (a hard
  // watchdog + _Exit turns a wedge into a failure), (2) the table's max limit
  // is respected exactly -- every grow reads the CURRENT min inside the token,
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
  // A definitely-doomed (over-limit) grow is rejected BEFORE the
  // exclusive token / handshake, so a guest `table.grow <huge>` loop cannot
  // force a global stop-the-world every iteration. Prove it by holding a
  // collection's token OPEN (pinned sweep, as
  // GrowBlocksDuringCollectAndViceVersa does): a token-taking grow would PARK
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

  // Watchdog: if the over-limit grow ever blocks on the token (regression), this
  // turns the hang into a FAILURE + hard-exit rather than an indefinite wait.
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
// window. runTableGrowOp pops the init ref into a bare local BEFORE growTable
// acquires the exclusive token, and a grow-loser can park (Blocked)
// through an entire concurrent collection before it owns the token. During that
// park the popped ref is on no GC-scanned stack and in no root set, so a
// collection whose root snapshot runs while we are parked would sweep it and
// the grow would broadcast a dangling ref into the new slots. The fix pins it
// as a scoped boundary root before growTable, released after.
//
// Determinism is delicate because StackManager::pop already shades a ref when a
// collection is mid-mark: to make the PIN (not the pop-time shade) the sole
// savior, the grower must pop while the write barrier is still quiet (heap
// Idle). setPreCycleHook holds the collection AFTER it wins the exclusive token
// but BEFORE the Idle->MarkingRoot CAS; the grower pops+pins+parks in that
// quiet window, then the released collection's stop-the-world snapshot -- gated
// behind the grower's Blocked ack -- scans the scoped roots and must find the
// pinned ref. Remove the pin (tableInstr.cpp) and this test sweeps the init
// object: an ASan heap-use-after-free on the sentinel read below, or memory
// usage 0.
TEST(GCThread, InterpreterGrowInitializerSurvivesCollect) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
          // Zero-extended numeric: the tracer's pointer-word read rejects it, so
          // this is a leaf, and the value doubles as a survival sentinel.
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

  // Hold the ARMED collection between winning the token and the Idle->MarkingRoot
  // CAS: token held, barrier still quiet. The first (grace-clearing) collection
  // runs with ArmPreCycle == false, so this is a no-op there.
  Alloc.setPreCycleHook([&]() noexcept {
    if (!ArmPreCycle.load(std::memory_order_acquire)) {
      return;
    }
    TokenHeldIdle.store(true, std::memory_order_release);
    while (!ReleaseCycle.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  // Watchdog: a wedged handshake / token handoff becomes a FAILURE + hard-exit.
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
    // heap Idle), pins it, then parks Blocked behind the collection's token until
    // the collection completes and the FIFO handoff grants the token here.
    EXPECT_TRUE(WasmEdge::Executor::gcTestRunTableGrowOp(Exe, StackMgr, Table));
  });

  // Wait for the grace-clearing collection to finish.
  while (!GraceCleared.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  // Arm the hook, then start the collection that will run during the grow's park.
  ArmPreCycle.store(true, std::memory_order_release);
  std::thread Collector([&]() { EXPECT_TRUE(Alloc.manualCollect()); });
  // Wait until the collection holds the token with the heap still Idle.
  while (!TokenHeldIdle.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  // Release the grower: it pops (quiet barrier), pins, and parks behind the token.
  GoGrow.store(true, std::memory_order_release);
  // Deterministically wait until the grower is queued behind the exclusive token.
  while (Ctrl.debugExclusiveWaiters() == 0) {
    std::this_thread::yield();
  }
  // Now let the collection run its snapshot + mark + sweep. Its root snapshot
  // must find the pinned init ref in the scoped roots; without the pin the struct
  // is White (off-stack, un-shaded) and is swept here.
  ReleaseCycle.store(true, std::memory_order_release);

  Grow.join();
  Collector.join();
  TestDone.store(true, std::memory_order_release);
  Watchdog.join();
  Alloc.setPreCycleHook(nullptr);

  // The grow published slot 0 with the init ref, which survived the concurrent
  // collection. Reading the sentinel field traps under ASan (heap-use-after-free)
  // if the object was swept; the live-byte count is the plain-build signal.
  auto Slot = Table.getRefAddr(0);
  ASSERT_TRUE(Slot);
  ASSERT_FALSE(Slot->isNull());
  RawData *Raw = Slot->getPtr<RawData>();
  EXPECT_EQ(Raw, CapturedRaw);
  EXPECT_EQ(Raw->data()[0].get<uint32_t>(), UINT32_C(0xC0FFEE));
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
}

// End-to-end SURVIVAL under a genuinely REMOTE collection. Unlike
// ShadowSpillKeepsRefLocalAcrossCompiledCall (which self-drives the collect from
// inside the host call, on the mutator's OWN thread) and unlike
// ShadowSpillProtocolUnderConcurrentCollect (a concurrent collector, but only a
// no-crash assertion), this runs the two STW collects on a SEPARATE collector
// thread while the JIT mutator is parked NativeRunning inside the host call, with
// its ref local live ONLY in the codegen-published shadow frame -- then asserts
// the struct SURVIVED. This is the real multi-mutator proof: the remote scan
// (scanNonRunningRoots) must walk a compiled thread's shadow chain across a true
// thread boundary. Barrier-coordinated (park -> collect x2 -> release) so the
// collects provably straddle the spill window; deterministic, not timing-based.
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
    // UNLESS the remote scan of the parked mutator's shadow chain re-grayed it.
    // Both cycles run on THIS (separate) thread while the mutator stays parked.
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
  // run returns i32.eqz(ref.is_null($r)) == 1: $r still non-null after the call.
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1u);
  // The struct was reachable ONLY via the codegen shadow spill during the 2nd
  // collect -- run on a SEPARATE thread while this mutator was parked
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
// The struct is allocated and IMMEDIATELY consumed as the call_indirect
// argument: it never occupies a local (so no shadow-frame slot covers it) and
// it reaches no value stack until the proxy pushes it. The callee is a host
// function, so proxyTableGetFuncSymbol returns nullptr on its cross-module gate
// and codegen takes the mediated IsNullBB path -- the window where the only
// copy of the ref lives in the packed native argument buffer.
const std::array<WasmEdge::Byte, 108> ConsumedArgWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   14,  3,   95,  1,   127,
    0,   96,  1,   107, 1,   127, 96,  0,   1,   127, 2,   13,  1,   4,
    104, 111, 115, 116, 4,   112, 97,  114, 107, 0,   1,   3,   2,   1,
    2,   4,   4,   1,   112, 0,   1,   7,   7,   1,   3,   114, 117, 110,
    0,   1,   9,   7,   1,   0,   65,  0,   11,  1,   0,   10,  14,  1,
    12,  0,   65,  42,  251, 0,   0,   65,  0,   17,  1,   0,   11,  0,
    23,  4,   110, 97,  109, 101, 1,   7,   1,   0,   4,   112, 97,  114,
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
    // Reached only AFTER the proxy pushed the argument onto the GC-rooted value
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
// It cannot, and this pins down why: root scanning is HANDSHAKE-GATED. A thread
// in the window is Running (nothing on the stretch enters a NativeScope or
// setSelfBlocked), and Controller::waitForAcks blocks on every Running entry,
// so no remote root scan can COMPLETE while a mutator sits in the window. The
// test drives exactly that: a collector thread opens STW #1 while the mutator
// is held inside the window, and the mutator observes that the collection has
// not finished. Were the window ever made non-Running -- the regression this
// guards
// -- the scan would complete without covering the packed argument.
//
// checkLazyCompilation is the only runtime hook that runs inside the window
// (proxyCallIndirect calls it after resolving the callee and BEFORE
// StackMgr.push roots the arguments), which is what makes the timing exact.
TEST(GCThread, ConsumedArgWindowBlocksRemoteRootScan) {
  SKIP_WITHOUT_COMPILER();
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

  // Fires inside the window. Note proxyTableGetFuncSymbol returns nullptr on its
  // cross-module check BEFORE its own checkLazyCompilation, so this runs exactly
  // once, from proxyCallIndirect.
  VM.getExecutor().registerLazyCompilationCallback(
      [](const Runtime::Instance::FunctionInstance *) -> Expect<void> {
        WindowEntered.store(true, std::memory_order_release);
        while (!WindowCollectorReady.load(std::memory_order_acquire)) {
          std::this_thread::yield();
        }
        // The collector is now inside collect() with STW #1's stop flag raised.
        // This thread is Running and has not acked, so waitForAcks must still be
        // spinning. Waiting longer only makes the check stricter: a false
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
    // Born-gray carries the struct through cycle 1 regardless; cycle 2 sweeps it
    // unless a scan genuinely re-grayed it. ScanNative=false so this thread's own
    // conservative scan cannot mask a missing root on the mutator's side.
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    WindowCollectFinished.store(true, std::memory_order_release);
  });

  auto R = VM.execute("run");
  Collector.join();
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  // The load-bearing assertion: the collection did NOT get through its root scan
  // while the mutator held a managed ref only in the packed argument buffer.
  EXPECT_FALSE(
      WindowScanCompletedInsideWindow.load(std::memory_order_relaxed));
  // The argument arrived as a live, non-null ref and outlived both cycles.
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1u);
  EXPECT_GT(WindowUsageAfter.load(std::memory_order_relaxed), 0u);
}

// Per-call shadow-spill OVERHEAD microbenchmark (DISABLED; run explicitly with
// --gtest_also_run_disabled_tests). loop_ref has a funcref LOCAL live across a
// trivial $sink call in a tight loop, so codegen publishes+pops a shadow frame
// each iteration; loop_noref is identical but with an i32 local (no ref -> no
// spill). $sink stores to memory (an unavoidable side effect) so neither loop is
// elided and $sink is not a pure no-op. The within-build (ref - noref) delta is
// an estimate; the TRUE isolation is this build (spill on) vs a build with
// FunctionCompiler::pushShadowFrame forced to early-return (spill off) run on the
// SAME loop_ref -- see the accompanying notes. Reports ns/call.
// (module (memory 1)
//  (func $sink (i32.store (i32.const 0) (i32.add (i32.load (i32.const 0)) (i32.const 1))))
//  (func (export "loop_ref")   (param $n i32) (local $i i32) (local $r funcref) <loop calling $sink $n times>)
//  (func (export "loop_noref") (param $n i32) (local $i i32) (local $x i32)     <same loop>))
const std::array<WasmEdge::Byte, 179> BenchWasm{
    0,   97,  115, 109, 1,   0,   0,   0,   1,   8,   2,   96,  0,   0,   96,
    1,   127, 0,   3,   4,   3,   0,   1,   1,   5,   3,   1,   0,   1,   7,
    25,  2,   8,   108, 111, 111, 112, 95,  114, 101, 102, 0,   1,   10,  108,
    111, 111, 112, 95,  110, 111, 114, 101, 102, 0,   2,   10,  67,  3,   15,
    0,   65,  0,   65,  0,   40,  2,   0,   65,  1,   106, 54,  2,   0,   11,
    25,  2,   1,   127, 1,   112, 3,   64,  16,  0,   32,  1,   65,  1,   106,
    33,  1,   32,  1,   32,  0,   73,  13,  0,   11,  11,  23,  1,   2,   127,
    3,   64,  16,  0,   32,  1,   65,  1,   106, 33,  1,   32,  1,   32,  0,
    73,  13,  0,   11,  11,  0,   52,  4,   110, 97,  109, 101, 1,   7,   1,
    0,   4,   115, 105, 110, 107, 2,   23,  2,   1,   3,   0,   1,   110, 1,
    1,   105, 2,   1,   114, 2,   3,   0,   1,   110, 1,   1,   105, 2,   1,
    120, 3,   11,  2,   1,   1,   0,   1,   108, 2,   1,   0,   1,   108};

TEST(GCThread, DISABLED_ShadowSpillOverheadBench) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

// A REAL guard-page SIGSEGV inside JIT-compiled code, with a real
// codegen-published shadow frame live for a ref local held across the faulting
// call, must drive emitFault's signal-safe head truncation and leave the
// runtime consistent. $boom does a wasm OOB i32.store -- a hardware trap in JIT
// code (which is NOT sanitizer-instrumented, so the SIGSEGV reaches WasmEdge's
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
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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

TEST(GCThread, CollectRestoresPriorCoordinatorState) {
  // H1 (latent-but-cheap): a collect() entered while the coordinator's entry is
  // NativeRunning must RESTORE NativeRunning afterwards, not hardcode Running.
  // Driven directly through the Controller (no production path reaches collect
  // while NativeRunning today). Single mutator: waitForAcks/scanNonRunningRoots
  // skip self, so the cycle completes deterministically.
  GC::Controller Ctrl;
  std::vector<ValVariant> Stack;
  auto Reg = Ctrl.registerStack(Stack);
  ASSERT_EQ(Ctrl.debugSelfState(), GC::Controller::MutatorState::Running);
  {
    GC::Controller::NativeScope Native(Ctrl); // entry -> NativeRunning
    ASSERT_EQ(Ctrl.debugSelfState(),
              GC::Controller::MutatorState::NativeRunning);
    Ctrl.collect(/*Manual=*/true, /*ScanNative=*/false);
    // Pre-fix: restored to Running (corrupt). Post-fix: still NativeRunning.
    EXPECT_EQ(Ctrl.debugSelfState(),
              GC::Controller::MutatorState::NativeRunning);
  }
  // NativeScope dtor restores Running.
  EXPECT_EQ(Ctrl.debugSelfState(), GC::Controller::MutatorState::Running);
}

// Regression: an empty gray queue is NOT quiescence. A collector worker
// pops a gray object and traces its children OUTSIDE GrayMutex; if termination
// concludes (sweeps) while a worker holds a popped-but-untraced object, a child
// reachable only through that object -- and still White -- is freed early. The
// ActiveTracers accounting closes this: termination requires
// Gray.empty() && ActiveTracers == 0 under GrayMutex. This test pauses a worker
// mid-trace (after pop, before shading children) via setTracerPauseHook and
// asserts the child survives.
//
// Determinism: parent is rooted on the collector thread's registered stack, so
// the STW #1 root scan re-grays it every cycle; child is reachable ONLY through
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

  // child: a leaf; in cycle 2 reachable ONLY through parent's field[0].
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

    // Cycle 2: root scan grays parent; a worker pops parent and pauses BEFORE
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
  // ActiveTracers accounting, termination CANNOT conclude while this tracer is
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
  // acknowledges BOTH STW #1 and STW #2 of every cycle.
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

TEST(GC, ClosingRefusesNewSynchronousExecute) {
  // R2 regression (registration TOCTOU): once the controller begins closing,
  // a NEW synchronous public call must be refused with a clean error at the
  // boundary -- never allowed to register a fresh stack behind a teardown
  // drain that may already have observed RegisteredStacks == 0 and returned.
  // registerStack itself has no error channel (StackManager's ctor is
  // noexcept), so the refusal lives at lease acquisition, which IS serialized
  // with the drain under DrainMtx.
  Configure Conf;
  Conf.addProposal(Proposal::GC);
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
      "alloc_loop",
      std::initializer_list<ValVariant>{UINT32_C(1), UINT32_C(1)},
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

#ifdef WASMEDGE_USE_LLVM

// --- Executor-enforced GC capability gate ---
//
// A GC-off compilation emits no cooperative safepoint poll and no shadow-root
// spill, so its native code is unsafe under a concurrent collector: it would
// never yield at a stop-the-world and would publish no native roots. The
// capability is recorded per module (AST::Module::getGCCompiled) and the
// Executor::instantiate function gate refuses to bind a non-capable module's
// compiled symbols under a GC-enabled executor, deopting the whole module to
// interpreter execution (which is cooperatively safe). This drives the gate's
// full decision matrix by compiling one ref-using module GC-off (via the manual
// Loader/Compiler/JIT path, so compile config and execute config differ) and
// instantiating it under executors with/without the GC proposal.

// (module (func (export "run") (result i32) (local funcref) i32.const 42))
const std::vector<uint8_t> RefLocalWasm = {
    0, 97, 115, 109, 1,   0,  0,  0,   1,  5,  1,   96, 0,  1,
    127, 3,  2,   1,   0,   7,  7,  1,   3,  114, 117, 110, 0,  0,
    10,  8,  1,   6,   1,   1,  112, 65, 42, 11};

// Manual parse -> validate -> compile -> JIT -> attach symbols under a chosen
// Configure, so the artifact's compile-time GC capability is independent of the
// executor that later runs it. Returns the module (carrying compiled symbols)
// or nullptr on any failure.
std::shared_ptr<AST::Module> compileWithConfig(const Configure &Conf,
                                               Span<const uint8_t> Bytes) {
  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(Bytes);
  if (!ModOrErr) {
    return nullptr;
  }
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  if (!ValidatorEngine.validate(*Mod)) {
    return nullptr;
  }
  LLVM::Compiler Compiler(Conf);
  if (!Compiler.checkConfigure()) {
    return nullptr;
  }
  auto Data = Compiler.compile(*Mod);
  if (!Data) {
    return nullptr;
  }
  LLVM::JIT JIT(Conf);
  auto Exec = JIT.load(std::move(*Data));
  if (!Exec) {
    return nullptr;
  }
  if (!LoaderEngine.loadExecutable(*Mod, std::move(*Exec))) {
    return nullptr;
  }
  return Mod;
}

// Compile the ref-using module GC-off (a non-capable artifact: symbols present,
// no safepoint poll / shadow spill), stamp its capability to GCCapable, then
// instantiate under an executor whose GC proposal is ExecutorGC. Returns whether
// "run" bound to native compiled code. Each call uses a FRESH module so compiled
// binding is independent across scenarios.
bool runIsCompiled(bool GCCapable, bool ExecutorGC) {
  // Configure() enables the full standard proposal set INCLUDING GC, so a
  // GC-off compile/executor is produced by removeProposal(GC), not by building
  // up from empty. Compiling with GC removed yields a genuinely non-capable
  // artifact (no safepoint poll / shadow spill emitted).
  Configure CompileConf;
  if (!GCCapable) {
    CompileConf.removeProposal(Proposal::GC);
  }
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  EXPECT_NE(Mod, nullptr);
  if (!Mod) {
    return false;
  }
  // On the VM path vm.cpp stamps this from the compile config; the manual path
  // sets it explicitly to match the GC-off vs GC-on compilation above.
  Mod->setGCCompiled(GCCapable);

  Configure ExecConf;
  if (!ExecutorGC) {
    ExecConf.removeProposal(Proposal::GC);
  }
  Executor::Executor Exec(ExecConf);
  Runtime::StoreManager Store;
  auto InstOrErr = Exec.instantiateModule(Store, *Mod);
  EXPECT_TRUE(InstOrErr);
  if (!InstOrErr) {
    return false;
  }
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  EXPECT_NE(RunFn, nullptr);
  return RunFn != nullptr && RunFn->isCompiledFunction();
}

TEST(GCThread, CapabilityGateDeoptsNonCapableModuleUnderGCExecutor) {
  // The gate fires in EXACTLY one quadrant of (module capability x executor GC):
  // a non-capable module under a GC-enabled executor deopts to the interpreter;
  // every other combination runs the compiled code natively.
  //
  // Baseline / capable + non-GC: proves the module compiles and native binding
  // is observable.
  EXPECT_TRUE(runIsCompiled(/*GCCapable=*/true, /*ExecutorGC=*/false));
  // THE GATE: non-capable + GC-enabled -> deopt to interpreter.
  EXPECT_FALSE(runIsCompiled(/*GCCapable=*/false, /*ExecutorGC=*/true));
  // Negative control: the deopt is gated on the executor's GC proposal, so a
  // non-capable module under a non-GC executor still runs native (no collector
  // to endanger it).
  EXPECT_TRUE(runIsCompiled(/*GCCapable=*/false, /*ExecutorGC=*/false));
  // Negative control: the deopt is gated on the module's capability, so a
  // capable module under a GC executor runs native.
  EXPECT_TRUE(runIsCompiled(/*GCCapable=*/true, /*ExecutorGC=*/true));

  // End-to-end: the deopted (interpreter) function is genuinely runnable and
  // returns the right value -- the fallback is real execution, not a stub.
  Configure CompileConf; // full set minus GC -> non-capable artifact
  CompileConf.removeProposal(Proposal::GC);
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  ASSERT_NE(Mod, nullptr);
  Mod->setGCCompiled(false); // non-capable
  Configure GCConf;          // default: GC enabled
  Executor::Executor Exec(GCConf);
  Runtime::StoreManager Store;
  auto InstOrErr = Exec.instantiateModule(Store, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  EXPECT_FALSE(RunFn->isCompiledFunction());
  auto R = Exec.invoke(RunFn, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), UINT32_C(42));
}

// Instantiate Wasm compiled under CompileGC using an executor whose GC proposal
// is InstantiateGC, then invoke the resulting function instance through a
// SECOND executor whose GC proposal is InvokeGC. This is the shape the
// instantiate gate cannot see: the module never passes through the invoking
// executor's instantiate, so only a per-call check can catch it. Returns the
// invocation result.
Expect<std::vector<std::pair<ValVariant, ValType>>>
invokeAcrossExecutors(bool CompileGC, bool InstantiateGC, bool InvokeGC,
                          bool *OutWasCompiled) {
  Configure CompileConf;
  if (!CompileGC) {
    CompileConf.removeProposal(Proposal::GC);
  }
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  EXPECT_NE(Mod, nullptr);
  Mod->setGCCompiled(CompileGC);

  Configure InstConf;
  if (!InstantiateGC) {
    InstConf.removeProposal(Proposal::GC);
  }
  Executor::Executor OwnerExec(InstConf);
  Runtime::StoreManager OwnerStore;
  auto InstOrErr = OwnerExec.instantiateModule(OwnerStore, *Mod);
  EXPECT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  EXPECT_NE(RunFn, nullptr);
  if (OutWasCompiled != nullptr) {
    *OutWasCompiled = RunFn->isCompiledFunction();
  }

  Configure InvokeConf;
  if (!InvokeGC) {
    InvokeConf.removeProposal(Proposal::GC);
  }
  Executor::Executor Foreign(InvokeConf);
  return Foreign.invoke(RunFn, {}, {});
}

TEST(GCThread, PerCallCapabilityCheckRefusesForeignNonCapableModule) {
  // The instantiate gate is a choke point for modules an executor instantiates
  // itself. registerModule and a direct Executor::invoke both bypass it: a
  // GC-off executor binds compiled code with no safepoint poll and no shadow
  // spill, and a GC-enabled executor can then be handed those very function
  // instances. enterFunction is the single funnel every call passes through, so
  // the backstop lives there.

  // THE BACKSTOP: compiled GC-off, instantiated by a GC-off executor (so the
  // instantiate deopt never fires and the code stays native), then invoked
  // through a GC-enabled executor.
  bool WasCompiled = false;
  auto Refused = invokeAcrossExecutors(
      /*CompileGC=*/false, /*InstantiateGC=*/false, /*InvokeGC=*/true,
      &WasCompiled);
  // Guard against a vacuous pass: if the owning executor had deopted this to
  // the interpreter there would be no native code to refuse, and the assertion
  // below would prove nothing.
  ASSERT_TRUE(WasCompiled);
  ASSERT_FALSE(Refused);
  EXPECT_EQ(Refused.error(), ErrCode::Value::IllegalGrammar);

  // Control: the refusal is the INVOKING executor's GC proposal, not something
  // broken about a cross-executor invocation. The same non-capable instance
  // called through a GC-off executor runs its compiled code and returns 42.
  auto Allowed = invokeAcrossExecutors(
      /*CompileGC=*/false, /*InstantiateGC=*/false, /*InvokeGC=*/false, nullptr);
  ASSERT_TRUE(Allowed);
  ASSERT_EQ(Allowed->size(), 1u);
  EXPECT_EQ((*Allowed)[0].first.get<uint32_t>(), UINT32_C(42));

  // Control: the refusal is keyed on CAPABILITY, not on the module being
  // foreign. A GC-capable module instantiated elsewhere still runs compiled
  // under a GC-enabled executor.
  bool CapableWasCompiled = false;
  auto Capable = invokeAcrossExecutors(
      /*CompileGC=*/true, /*InstantiateGC=*/false, /*InvokeGC=*/true,
      &CapableWasCompiled);
  ASSERT_TRUE(CapableWasCompiled);
  ASSERT_TRUE(Capable);
  ASSERT_EQ(Capable->size(), 1u);
  EXPECT_EQ((*Capable)[0].first.get<uint32_t>(), UINT32_C(42));
}

// --- Durable AOT capability bit + loader admission ---
//
// The gate is only as strong as the capability that reaches it. An artifact
// compiled in one process and loaded in another carries no compile Configure,
// so the capability has to survive inside the artifact. The compiler exports a
// "gc.capable" marker global only when GC codegen is on; for a universal WASM
// outputWasmLibrary records the marker's presence as a flag byte in the AOT
// section, which the loader reads back into AST::Module::getGCCompiled.
//
// Admission has to happen in the LOADER, not at instantiate: loading in AOT run
// mode skips parsing function bodies (Loader::loadSegment), so a module bound
// to non-capable native code has no instructions left to deopt to. The loader
// instead declines the bind and takes its interpreter fallback, which re-reads
// the skipped code section.

// AOT-compile the ref-using module to a universal WASM at Out with the GC
// proposal set to GCCapable, then load it back in AOT run mode with the GC
// proposal set to LoadWithGC. The reloaded module carries whatever capability
// the artifact itself recorded. Returns nullptr on any failure.
std::unique_ptr<AST::Module> aotRoundTrip(bool GCCapable, bool LoadWithGC,
                                          const std::filesystem::path &Out) {
  Configure CompileConf;
  if (!GCCapable) {
    CompileConf.removeProposal(Proposal::GC);
  }
  Loader::Loader LoaderEngine(CompileConf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(CompileConf);
  auto ModOrErr = LoaderEngine.parseModule(RefLocalWasm);
  if (!ModOrErr) {
    return nullptr;
  }
  if (!ValidatorEngine.validate(**ModOrErr)) {
    return nullptr;
  }
  LLVM::Compiler Compiler(CompileConf);
  if (!Compiler.checkConfigure()) {
    return nullptr;
  }
  auto Data = Compiler.compile(**ModOrErr);
  if (!Data) {
    return nullptr;
  }
  // Default OutputFormat is Wasm, so this emits a universal WASM carrying the
  // AOT section -- the path that serializes the capability flag byte.
  LLVM::CodeGen CodeGen(CompileConf);
  if (!CodeGen.codegen(RefLocalWasm, std::move(*Data), Out)) {
    return nullptr;
  }

  Configure LoadConf;
  if (!LoadWithGC) {
    LoadConf.removeProposal(Proposal::GC);
  }
  LoadConf.getRuntimeConfigure().setRunMode(RunMode::AOT);
  Loader::Loader Reloader(LoadConf, &Executor::Executor::Intrinsics);
  auto Reloaded = Reloader.parseModule(Out);
  if (!Reloaded) {
    return nullptr;
  }
  // A freshly loaded module has not passed validation, which instantiate
  // requires. Validating does not touch the attached symbols or the capability.
  Validator::Validator Revalidator(LoadConf);
  if (!Revalidator.validate(**Reloaded)) {
    return nullptr;
  }
  return std::move(*Reloaded);
}

// Instantiate under a GC-enabled executor and return whether "run" bound to
// native code; also asserts the function actually computes 42, so a deopt to an
// empty (body-stripped) function instance cannot pass as a successful fallback.
bool aotRunIsCompiled(AST::Module &Mod) {
  Configure GCConf; // default: GC enabled
  Executor::Executor Exec(GCConf);
  Runtime::StoreManager Store;
  auto InstOrErr = Exec.instantiateModule(Store, Mod);
  EXPECT_TRUE(InstOrErr);
  if (!InstOrErr) {
    return false;
  }
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  EXPECT_NE(RunFn, nullptr);
  if (RunFn == nullptr) {
    return false;
  }
  auto R = Exec.invoke(RunFn, {}, {});
  EXPECT_TRUE(R);
  if (!R) {
    return false;
  }
  EXPECT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), UINT32_C(42));
  return RunFn->isCompiledFunction();
}

TEST(GCThread, DurableAOTCapabilityBitSurvivesRoundTrip) {
  const auto Dir = std::filesystem::current_path();
  const auto CapablePath = Dir / "gc-capable.wasm";
  const auto OffPath = Dir / "gc-off.wasm";
  std::error_code EC;

  // Load with GC OFF so the loader always binds the native code: this isolates
  // "did the bit survive the round trip" from "what the loader does about it".
  {
    auto Capable = aotRoundTrip(/*GCCapable=*/true, /*LoadWithGC=*/false,
                                CapablePath);
    ASSERT_NE(Capable, nullptr);
    // getSymbol() confirms the AOT section really linked -- otherwise the
    // loader silently fell back and the capability below would be vacuous.
    EXPECT_TRUE(!!Capable->getSymbol());
    EXPECT_TRUE(Capable->getGCCompiled());

    auto NonCapable = aotRoundTrip(/*GCCapable=*/false, /*LoadWithGC=*/false,
                                   OffPath);
    ASSERT_NE(NonCapable, nullptr);
    EXPECT_TRUE(!!NonCapable->getSymbol());
    // THE DURABLE BIT: compiled GC-off in an earlier step, written to disk, and
    // still correctly reported as non-capable after a reload.
    EXPECT_FALSE(NonCapable->getGCCompiled());
  }

  // Load with GC ON: the loader must admit the capable artifact and decline the
  // non-capable one, and BOTH must still execute correctly.
  {
    auto Capable =
        aotRoundTrip(/*GCCapable=*/true, /*LoadWithGC=*/true, CapablePath);
    ASSERT_NE(Capable, nullptr);
    EXPECT_TRUE(!!Capable->getSymbol());
    EXPECT_TRUE(aotRunIsCompiled(*Capable));

    auto NonCapable =
        aotRoundTrip(/*GCCapable=*/false, /*LoadWithGC=*/true, OffPath);
    ASSERT_NE(NonCapable, nullptr);
    // ADMISSION: the loader refused to bind non-capable native code, so no
    // symbols are attached and the skipped function bodies were re-read.
    EXPECT_FALSE(!!NonCapable->getSymbol());
    // Runs interpreted -- and aotRunIsCompiled asserts it returns 42, which
    // is what proves the bodies came back rather than being empty.
    EXPECT_FALSE(aotRunIsCompiled(*NonCapable));
  }

  std::filesystem::remove(CapablePath, EC);
  std::filesystem::remove(OffPath, EC);
}

// --- Component core-module capability hole ---
//
// The component core-module instantiate path (component_module.cpp) used to
// call the function-section instantiate() without the capability argument,
// so it always took the `bool GCCompiled = true` default: DeoptForGC was
// always false and native symbols always bound, regardless of whether the
// core module was actually compiled GC-capable. The freshly built
// ModuleInstance was never setGCCompiled()-stamped either, so the per-call
// enterFunction backstop (helper.cpp) could not catch it after the fact.
// This test hand-assembles a minimal component AST embedding a GC-off
// compiled core module (mirroring the manual Loader/Compiler/JIT path used
// above) and instantiates it under a GC-enabled executor, matching the
// non-component gate exercised by CapabilityGateDeoptsNonCapableModuleUnder
// GCExecutor.
TEST(GCThread, ComponentCoreModuleCapabilityGated) {
  // Compile the ref-using core module GC-off: symbols present, but no
  // safepoint poll / shadow-root spill emitted.
  Configure CompileConf;
  CompileConf.removeProposal(Proposal::GC);
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  ASSERT_NE(Mod, nullptr);
  Mod->setGCCompiled(false);

  // Hand-assemble a component: a CoreModuleSection carrying the compiled
  // module, and a CoreInstanceSection that instantiates it with no imports.
  // This is the shape the production loader builds from a `.wasm` component
  // binary's `core:module` + `core:instance (instantiate ...)` sections; it
  // is built in-process here because there is no on-disk component fixture
  // wired to a compiled (as opposed to interpreted) core module.
  AST::Component::Component Comp;
  {
    AST::Component::CoreModuleSection ModSec;
    ModSec.getContent() = std::move(*Mod);
    Comp.getSections().emplace_back(std::move(ModSec));
  }
  {
    AST::Component::CoreInstanceSection InstSec;
    AST::Component::CoreInstance CoreInst;
    CoreInst.setInstantiateArgs(/*ModIdx=*/0, {});
    InstSec.getContent().push_back(std::move(CoreInst));
    Comp.getSections().emplace_back(std::move(InstSec));
  }

  Configure GCConf; // default: GC enabled
  Executor::Executor Exec(GCConf);
  Runtime::StoreManager Store;
  auto CompInstOrErr = Exec.instantiateComponent(Store, Comp);
  ASSERT_TRUE(CompInstOrErr);
  auto CompInst = std::move(*CompInstOrErr);

  const auto *CoreModInst = CompInst->getCoreModuleInstance(0);
  ASSERT_NE(CoreModInst, nullptr);
  // THE BACKSTOP STAMP: the freshly built component core-module instance
  // carries the same non-capability as the AST module it was built from, so
  // enterFunction covers it even for calls that bypass this instantiate.
  EXPECT_FALSE(CoreModInst->isGCCompiled());

  const auto *RunFn = CoreModInst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  // THE GATE: a non-capable core module under a GC-enabled executor must not
  // bind native compiled code.
  EXPECT_FALSE(RunFn->isCompiledFunction());

  // The deopted (interpreter) function is genuinely runnable and returns the
  // right value -- the fallback is real execution, not a stub.
  auto R = Exec.invoke(RunFn, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), UINT32_C(42));

  // Negative control: the deopt is gated on the EXECUTOR's GC proposal, not
  // unconditional. The same GC-off-compiled core module, embedded in a fresh
  // component and instantiated under a GC-DISABLED executor, still runs its
  // compiled code natively -- there is no collector to endanger it, so
  // nothing should be refused.
  auto Mod2 = compileWithConfig(CompileConf, RefLocalWasm);
  ASSERT_NE(Mod2, nullptr);
  Mod2->setGCCompiled(false);

  AST::Component::Component Comp2;
  {
    AST::Component::CoreModuleSection ModSec;
    ModSec.getContent() = std::move(*Mod2);
    Comp2.getSections().emplace_back(std::move(ModSec));
  }
  {
    AST::Component::CoreInstanceSection InstSec;
    AST::Component::CoreInstance CoreInst;
    CoreInst.setInstantiateArgs(/*ModIdx=*/0, {});
    InstSec.getContent().push_back(std::move(CoreInst));
    Comp2.getSections().emplace_back(std::move(InstSec));
  }

  Configure NonGCConf;
  NonGCConf.removeProposal(Proposal::GC);
  Executor::Executor NonGCExec(NonGCConf);
  Runtime::StoreManager Store2;
  auto CompInstOrErr2 = NonGCExec.instantiateComponent(Store2, Comp2);
  ASSERT_TRUE(CompInstOrErr2);
  auto CompInst2 = std::move(*CompInstOrErr2);
  const auto *CoreModInst2 = CompInst2->getCoreModuleInstance(0);
  ASSERT_NE(CoreModInst2, nullptr);
  const auto *RunFn2 = CoreModInst2->findFuncExports("run");
  ASSERT_NE(RunFn2, nullptr);
  EXPECT_TRUE(RunFn2->isCompiledFunction());
  auto R2 = NonGCExec.invoke(RunFn2, {}, {});
  ASSERT_TRUE(R2);
  ASSERT_EQ(R2->size(), 1u);
  EXPECT_EQ((*R2)[0].first.get<uint32_t>(), UINT32_C(42));
}

#endif // WASMEDGE_USE_LLVM

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
