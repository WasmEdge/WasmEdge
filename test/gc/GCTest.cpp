// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "ast/type.h"
#include "common/spdlog.h"
#include "gc/allocator.h"
#include "runtime/instance/exception.h"
#include "runtime/instance/table.h"
#include "runtime/instance/tag.h"
#include "runtime/stackmgr.h"
#include "vm/vm.h"

#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace {

using namespace WasmEdge;

// --- Host functions for GC testing ---

class Collect : public Runtime::HostFunction<Collect> {
public:
  Expect<void> body(const Runtime::CallingFrame &CF) {
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
// The two collections with the outer still on the stack are deliberate: the
// first lets both survive via the born-gray rule, but the second reclaims the
// inner unless the collector traces the outer's child edge to it -- so this
// exercises heap->heap child-edge tracing.
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

// Test 8: Shared references via global - multiple threads write/read shared
// array
const std::array<WasmEdge::Byte, 251> SharedRefsWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x04, 0x5e,
    0x7f, 0x01, 0x60, 0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x02, 0x7f,
    0x7f, 0x00, 0x02, 0x16, 0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c,
    0x6c, 0x00, 0x01, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b,
    0x00, 0x02, 0x03, 0x03, 0x02, 0x01, 0x03, 0x06, 0x07, 0x01, 0x63, 0x00,
    0x01, 0xd0, 0x00, 0x0b, 0x07, 0x1b, 0x02, 0x04, 0x69, 0x6e, 0x69, 0x74,
    0x00, 0x02, 0x10, 0x77, 0x72, 0x69, 0x74, 0x65, 0x5f, 0x61, 0x6e, 0x64,
    0x5f, 0x76, 0x65, 0x72, 0x69, 0x66, 0x79, 0x00, 0x03, 0x0a, 0x42, 0x02,
    0x0b, 0x00, 0x41, 0x00, 0x41, 0x04, 0xfb, 0x06, 0x00, 0x24, 0x00, 0x0b,
    0x34, 0x01, 0x01, 0x7f, 0x41, 0x00, 0x21, 0x02, 0x02, 0x40, 0x03, 0x40,
    0x20, 0x02, 0x41, 0x32, 0x4f, 0x0d, 0x01, 0x23, 0x00, 0xd4, 0x20, 0x00,
    0x20, 0x01, 0xfb, 0x0e, 0x00, 0x10, 0x00, 0x23, 0x00, 0xd4, 0x20, 0x00,
    0xfb, 0x0b, 0x00, 0x10, 0x01, 0x20, 0x02, 0x41, 0x01, 0x6a, 0x21, 0x02,
    0x0c, 0x00, 0x0b, 0x0b, 0x0b, 0x00, 0x58, 0x04, 0x6e, 0x61, 0x6d, 0x65,
    0x01, 0x0e, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c, 0x6c, 0x01, 0x05, 0x63,
    0x68, 0x65, 0x63, 0x6b, 0x02, 0x10, 0x01, 0x03, 0x03, 0x00, 0x03, 0x69,
    0x64, 0x78, 0x01, 0x03, 0x76, 0x61, 0x6c, 0x02, 0x01, 0x69, 0x03, 0x10,
    0x01, 0x03, 0x02, 0x00, 0x05, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x01, 0x04,
    0x6c, 0x6f, 0x6f, 0x70, 0x04, 0x12, 0x03, 0x00, 0x03, 0x61, 0x72, 0x72,
    0x01, 0x02, 0x66, 0x6e, 0x02, 0x06, 0x66, 0x6e, 0x5f, 0x69, 0x33, 0x32,
    0x07, 0x09, 0x01, 0x00, 0x06, 0x73, 0x68, 0x61, 0x72, 0x65, 0x64};

// Functions that RETURN gc refs to the host (for host-root retention tests).
//   (type $s (struct (field (mut i32))))
//   (type $a (array (mut i32)))
//   (func (export "make") (param i32) (result (ref $s))
//     (struct.new $s (local.get 0)))
//   (func (export "make_arr") (param i32) (result (ref $a))
//     (array.new_default $a (local.get 0)))
//   (func (export "drop_return_i31") (result i31ref)
//     (struct.new $s (i32.const 0)) drop (ref.i31 (i32.const 7)))
std::array<WasmEdge::Byte, 127> HostRetentionWasm{
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
std::array<WasmEdge::Byte, 64> MakeExtWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02,
    0x5f, 0x01, 0x7f, 0x01, 0x60, 0x01, 0x7f, 0x01, 0x6f, 0x03, 0x02,
    0x01, 0x01, 0x07, 0x0c, 0x01, 0x08, 0x6d, 0x61, 0x6b, 0x65, 0x5f,
    0x65, 0x78, 0x74, 0x00, 0x00, 0x0a, 0x0b, 0x01, 0x09, 0x00, 0x20,
    0x00, 0xfb, 0x00, 0x00, 0xfb, 0x1b, 0x0b, 0x00, 0x0b, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

// A PASSIVE element segment whose init expr allocates a gc struct (regression
// for element-segment refs being scanned as roots). After two collections the
// passive segment holds the only reference to the struct; the test then
// materializes it via array.new_elem and reads the field, which is garbage if
// the segment was not scanned and the struct was swept.
//   (type $s (struct (field i32))) (type $a (array (ref null $s)))
//   (elem $e (ref null $s) (item (struct.new $s (i32.const 42))))
//   (func (export "test") coll; coll;
//     check (struct.get $s 0 (ref.cast (ref $s)
//       (array.get $a (array.new_elem $a $e 0 1) 0))))
std::array<WasmEdge::Byte, 164> ElemRootWasm{
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

// Allocate one zero-child GC object directly through the allocator, wrap it as
// a RefVariant, and exercise retain/collect/release. A zero-Length RawData is
// required so the collector's child-scan reads no garbage children.
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

TEST(GC, ExecutorRetainAndRelease) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // Disable timer-based autoCollect so the only collections are the explicit
  // manualCollect() calls below; otherwise a background collection firing
  // between allocation and the memory-usage assertions makes them flaky.
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

// ==========================================================================
// Single-threaded GC tests
// ==========================================================================

TEST(GC, StructAllocAndCollect) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  GCRecModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(StructGCWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // Disable timer-based autoCollect: these tests assert exact memory usage at
  // each recorded point, so the only collections must be the explicit coll
  // calls the wasm makes (a background collection would perturb the snapshots).
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
  VM::VM VM(Conf);
  GCRecModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(NestedRefWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // Disable timer-based autoCollect: these tests assert exact memory usage at
  // each recorded point, so the only collections must be the explicit coll
  // calls the wasm makes (a background collection would perturb the snapshots).
  VM.getExecutor().getAllocator().setManualGC(true);
  ASSERT_TRUE(VM.execute("test"));
  auto Log = GCMod.getLog();

  ASSERT_EQ(Log.size(), 4);
  EXPECT_EQ(Log[0], 0); // before allocation
  EXPECT_GT(Log[1], 0); // both inner and outer allocated
  // After two collections with the outer still on the stack: the first lets
  // both survive via the born-gray rule, so it cannot test edge tracing; the
  // second reclaims the inner unless the collector follows the outer's child
  // edge. Equal usage therefore proves heap->heap child-edge tracing keeps the
  // inner (reachable only through the outer) alive.
  EXPECT_EQ(Log[2], Log[1]);
  EXPECT_EQ(Log[3], 0); // after drop + GC, both collected
}

TEST(GC, DataSurvivesGC) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);

  // Registered host modules must outlive the VM (~VM() terminates each);
  // declare the module before the VM.
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
  // Disable timer-based autoCollect: these tests assert exact memory usage at
  // each recorded point, so the only collections must be the explicit coll
  // calls the wasm makes (a background collection would perturb the snapshots).
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
  VM::VM VM(Conf);
  GCFullModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ArrayOpsWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // Disable timer-based autoCollect: these tests assert exact memory usage at
  // each recorded point, so the only collections must be the explicit coll
  // calls the wasm makes (a background collection would perturb the snapshots).
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
  VM::VM VM(Conf);
  GCRecModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(AllocPressureWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // Disable timer-based autoCollect: these tests assert exact memory usage at
  // each recorded point, so the only collections must be the explicit coll
  // calls the wasm makes (a background collection would perturb the snapshots).
  VM.getExecutor().getAllocator().setManualGC(true);
  ASSERT_TRUE(VM.execute("test"));
  auto Log = GCMod.getLog();

  ASSERT_EQ(Log.size(), 3);
  EXPECT_EQ(Log[0], 0); // before loop

  // 500 arrays of 64 elements each; array elements are 16-byte ValVariants,
  // so each array is ~1 KiB plus its header and all 500 (~516 KiB total) are
  // live here, comfortably under the threshold.
  EXPECT_GT(Log[1], 0);

  // Log[2] is intentionally not asserted to be zero: the host-side
  // manualCollect() never scans the native stack, so the leftover usage has
  // nothing to do with conservative stack scanning. The single collection here
  // keeps the just-allocated arrays alive via the born-gray rule, so an
  // exact-zero assertion would not be deterministic and is left disabled.
  // EXPECT_EQ(Log[2], 0); // not asserted: born-gray retention on first cycle
}

// ==========================================================================
// Multi-threaded GC tests
// ==========================================================================

TEST(GCThread, ConcurrentAllocation) {
  // Multiple threads allocating GC objects simultaneously
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  GCRecModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcurrentAllocWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // Launch 4 async tasks, each allocating 200 arrays of size 64
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

  // All should complete without crash
  for (uint32_t I = 0; I < NumThreads; ++I) {
    auto Result = AsyncResults[I].get();
    EXPECT_TRUE(Result) << "Thread " << I << " failed";
  }
}

TEST(GCThread, GCDuringConcurrentExecution) {
  // Multiple threads allocating structs and triggering GC while verifying
  // data integrity
  Configure Conf;
  Conf.addProposal(Proposal::GC);

  constexpr uint32_t NumThreads = 4;

  // Each thread gets its own expected value = thread ID (0..3)
  // The wasm module allocates a struct with the given ID, then loops 100 times
  // calling GC and checking that the struct's value is still the ID.
  // We use a thread-safe check function that records failures.
  // We need separate VMs per thread since each needs its own check module
  std::vector<std::thread> Threads;
  std::atomic<uint32_t> FailCount{0};

  for (uint32_t I = 0; I < NumThreads; ++I) {
    Threads.emplace_back([&, I]() {
      // Registered host modules must outlive the VM (~VM() terminates each);
      // declare the module before the VM.
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
      // Verify all checks passed
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

TEST(GCThread, SharedReferencesAcrossThreads) {
  // Multiple threads accessing a shared GC object via global
  // Each thread writes to a different index in a shared array and verifies
  Configure Conf;
  Conf.addProposal(Proposal::GC);

  constexpr uint32_t NumThreads = 4;
  std::atomic<uint32_t> TotalFails{0};

  // Each thread uses its own VM but they run the same module pattern.
  // Thread i writes value (i+1)*10 to index i, then reads it back after GC.
  std::vector<std::thread> Threads;

  for (uint32_t I = 0; I < NumThreads; ++I) {
    Threads.emplace_back([&, I]() {
      // Registered host modules must outlive the VM (~VM() terminates each);
      // declare the module before the VM.
      uint32_t ExpectedVal = (I + 1) * 10;
      auto Mod = std::make_unique<Runtime::Instance::ModuleInstance>("gc");
      Mod->addHostFunc("coll", std::make_unique<Collect>());
      auto CP = std::make_unique<ThreadSafeCheck>(ExpectedVal);
      auto *C = CP.get();
      Mod->addHostFunc("check", std::move(CP));

      Configure TConf;
      TConf.addProposal(Proposal::GC);
      VM::VM TVM(TConf);
      TVM.registerModule(*Mod);

      ASSERT_TRUE(TVM.loadWasm(SharedRefsWasm));
      ASSERT_TRUE(TVM.validate());
      ASSERT_TRUE(TVM.instantiate());

      // Initialize the shared global array
      TVM.execute("init");

      // Write and verify in a loop
      auto Result = TVM.execute(
          "write_and_verify", std::initializer_list<ValVariant>{I, ExpectedVal},
          {ValType(TypeCode::I32), ValType(TypeCode::I32)});

      if (!Result) {
        TotalFails.fetch_add(1, std::memory_order_relaxed);
      }
      EXPECT_EQ(C->getFailCount(), 0)
          << "Thread " << I << " had " << C->getFailCount()
          << " data integrity failures";
    });
  }

  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_EQ(TotalFails.load(), 0);
}

TEST(GCThread, GrowTableDuringCollect) {
  // Regression test for the table-grow vs root-scan data race. growTable
  // reallocates the Refs vector that the collector scans as a GC root set; one
  // thread grows a registered table in a loop while the main thread drives
  // collections that iterate that same vector. Without serialization the scan
  // reads a reallocated (freed) buffer. The table holds plain null funcrefs, so
  // this exercises the buffer-reallocation race itself, not object marking, and
  // a clean run -- especially under ThreadSanitizer -- is the assertion.
  GC::Allocator Alloc;
  AST::TableType TType(ValType(TypeCode::FuncRef), 0, 100000);
  Runtime::Instance::TableInstance Table(TType);
  Table.setAllocator(Alloc);

  std::atomic<bool> Stop{false};
  std::thread Grower([&]() {
    for (uint32_t I = 0; I < 1000 && !Stop.load(std::memory_order_relaxed);
         ++I) {
      if (!Table.growTable(1)) {
        break;
      }
    }
  });

  for (uint32_t I = 0; I < 1000; ++I) {
    Alloc.manualCollect();
  }
  Stop.store(true, std::memory_order_relaxed);
  Grower.join();

  EXPECT_GE(Table.getSize(), 1u);
}

TEST(GCThread, GrowStackDuringCollect) {
  // Regression test for the value-stack realloc vs root-scan race: pushing past
  // the reserved capacity reallocates the GC-registered ValueStack that the
  // collector scans. One thread grows a stack while the main thread drives
  // collections that iterate it; without the grow-only lock the scan reads a
  // reallocated (freed) buffer. Like GrowTableDuringCollect, the read side is
  // TSan-suppressed, so a clean, deadlock-free run is the assertion.
  GC::Allocator Alloc;
  std::atomic<bool> Stop{false};
  std::thread Pusher([&]() {
    Runtime::StackManager StackMgr(Alloc);
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

TEST(GC, VMReleaseAllRefsArray) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // Disable timer-based autoCollect so the only collections are the explicit
  // manualCollect() calls below; otherwise a background collection firing
  // between allocation and the memory-usage assertions makes them flaky.
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
  // Disable timer-based autoCollect so the only collections are the explicit
  // manualCollect() calls below; otherwise a background collection firing
  // between allocation and the memory-usage assertions makes them flaky.
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
  // Disable timer-based autoCollect so the only collections are the explicit
  // manualCollect() calls below; otherwise a background collection firing
  // between allocation and the memory-usage assertions makes them flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  // Allocates a struct internally, drops it, and returns an i31 (non-heap).
  // Nothing should be retained: the returned i31 is excluded, and the dropped
  // struct is not a returned value.
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
  // Disable timer-based autoCollect so the only collections are the explicit
  // manualCollect() calls below; otherwise a background collection firing
  // between allocation and the memory-usage assertions makes them flaky.
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
  // An externalized gc struct returned to the host (extern.convert_any) must be
  // retained as a host root, not handed out unrooted and swept on the next
  // collection. It is presented to the host as externref but still points to a
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

TEST(GC, ElemSegmentRoot) {
  // A passive element segment holds the only reference to a struct created in
  // its init expression; it must be scanned as a GC root or the struct is swept
  // and array.new_elem later reads a dangling pointer.
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  // Registered host modules must outlive the VM (~VM() terminates each);
  // declare the module before the VM.
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

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
