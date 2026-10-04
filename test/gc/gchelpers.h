// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/gchelpers.h - Shared GC test helpers -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host functions, host modules, WASM modules and
/// helper functions that more than one GC test file uses.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "api/internal/managed_ref_getter.h"
#include "ast/type.h"
#include "common/spdlog.h"
#include "gc/allocator.h"
#include "gc/coherent_slot.h"
#include "gc/controller.h"
#include "runtime/instance/exception.h"
#include "runtime/instance/gc.h"
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
#include <optional>
#include <thread>
#include <vector>

#include <fstream>
#include <string>

// Tests that need real compiled code. Without the AOT/JIT compiler the module
// stays interpreted, so there is no native frame, no shadow spill and no
// safepoint poll to exercise -- skip rather than fail.
#ifdef WASMEDGE_USE_LLVM
#define SKIP_WITHOUT_COMPILER() ((void)0)
#else
#define SKIP_WITHOUT_COMPILER() GTEST_SKIP() << "requires the AOT/JIT compiler"
#endif

namespace WasmEdge::GCTest {

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

// Functions that return gc refs to the host (for host-root retention tests).
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

#ifdef WASMEDGE_USE_LLVM

// (module
//   (type $s (struct (field i32)))
//   (global $g (export "g") (mut anyref) (ref.null any))
//   (func (export "set") (global.set $g (struct.new $s (i32.const 7))))
//   (func (export "get") (result i32)
//     (struct.get $s 0 (ref.cast (ref $s) (global.get $g)))))
const std::vector<uint8_t> ForeignRootsWasm = {
    0,   97,  115, 109, 1,   0,   0,  0,   1,   12,  3,  95,  1,   127,
    0,   96,  0,   0,   96,  0,   1,  127, 3,   3,   2,  1,   2,   6,
    6,   1,   110, 1,   208, 110, 11, 7,   17,  3,   1,  103, 3,   0,
    3,   115, 101, 116, 0,   0,   3,  103, 101, 116, 0,  1,   10,  23,
    2,   9,   0,   65,  7,   251, 0,  0,   36,  0,   11, 11,  0,   35,
    0,   251, 22,  0,   251, 2,   0,  0,   11,  0,   17, 4,   110, 97,
    109, 101, 4,   4,   1,   0,   1,  115, 7,   4,   1,  0,   1,   103};

// (module
//   (type $t (func))
//   (import "N" "set" (func $set (type $t)))
//   (table 1 1 funcref)
//   (elem (i32.const 0) $set)
//   (elem declare func $set)
//   (func (export "run") (call $set))
//   (func (export "run_ref") (call_ref $t (ref.func $set)))
//   (func (export "run_indirect") (call_indirect (type $t) (i32.const 0))))
const std::vector<uint8_t> CalleeImportWasm = {
    0,   97,  115, 109, 1,   0,   0,   0,   1,   4,   1,  96, 0,   0,   2,
    9,   1,   1,   78,  3,   115, 101, 116, 0,   0,   3,  4,  3,   0,   0,
    0,   4,   5,   1,   112, 1,   1,   1,   7,   32,  3,  3,  114, 117, 110,
    0,   1,   7,   114, 117, 110, 95,  114, 101, 102, 0,  2,  12,  114, 117,
    110, 95,  105, 110, 100, 105, 114, 101, 99,  116, 0,  3,  9,   11,  2,
    0,   65,  0,   11,  1,   0,   3,   0,   1,   0,   10, 21, 3,   4,   0,
    16,  0,   11,  6,   0,   210, 0,   20,  0,   11,  7,  0,  65,  0,   17,
    0,   0,   11,  0,   19,  4,   110, 97,  109, 101, 1,  6,  1,   0,   3,
    115, 101, 116, 4,   4,   1,   0,   1,   116};

#endif // WASMEDGE_USE_LLVM

// Return a configuration with the GC proposal enabled.
inline Configure makeGCConf() {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  return Conf;
}

// Register Host in VM, then load, validate and instantiate Wasm. Automatic
// collection is switched off, so only explicit collections run and a test can
// assert exact memory usage. Call it through ASSERT_NO_FATAL_FAILURE.
template <typename WasmT>
void setUpManualGCVM(VM::VM &VM, Runtime::Instance::ModuleInstance &Host,
                     const WasmT &Wasm) {
  VM.registerModule(Host);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);
}

} // namespace WasmEdge::GCTest
