// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/aot/AOTcoreTest.cpp - Wasm test suites --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suites: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "common/defines.h"
#include "common/executable.h"
#include "common/spdlog.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"
// GCInstance::RawData, for deriving the expected GC byte totals from the real
// allocation layout rather than hard-coding them (AOTGC.MemoryUsage).
#include "runtime/instance/gc.h"

// lib/llvm internals: Data::DataContext and LLVM::Module::unwrap().
#include "data.h"

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

// Parameterized testing class.
class NativeCoreTest : public testing::TestWithParam<std::string> {};
class CustomWasmCoreTest : public testing::TestWithParam<std::string> {};
class JITCoreTest : public testing::TestWithParam<std::string> {};
class LazyJITCoreTest : public testing::TestWithParam<std::string> {};

// A compiled cross-module call must run the callee with its own context and
// restore the caller's on return. run_ref computes
// call_ref(read) * 10 + memory.grow(0) = 187*10 + 1 = 1871 with contexts kept
// separate (callee global 187, caller's 1-page memory); a leaked callee context
// reads its 5-page memory and gives 1875. memory.grow is the probe because it
// resolves memory live, unlike inline accesses snapshotted on function entry.
//   callee: (memory 5) (global $g (mut i32) 187)
//           (func (export "read") (result i32) (global.get $g))
//   caller: (import "callee" "read" (func $read (result i32)))
//           (memory 1) (global $g (mut i32) 170)
//           (table 1 funcref) (elem (i32.const 0) func $read)
//           (func (export "run_ref") (result i32)
//             (i32.add (i32.mul (call_ref $t (ref.func $read)) (i32.const 10))
//                      (memory.grow (i32.const 0))))
//           run_direct and run_indirect are the same with (call $read) and
//           (call_indirect (type $t) (i32.const 0)).
const std::array<WasmEdge::Byte, 70> CrossModuleCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x05,
    0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xbb, 0x01, 0x0b, 0x07, 0x08, 0x01,
    0x04, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00,
    0x23, 0x00, 0x0b, 0x00, 0x11, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x04, 0x04,
    0x01, 0x00, 0x01, 0x74, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

const std::array<WasmEdge::Byte, 183> CrossModuleCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x0f, 0x01, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
    0x65, 0x04, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x03, 0x04, 0x03, 0x00,
    0x00, 0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01, 0x05, 0x03, 0x01, 0x00,
    0x01, 0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xaa, 0x01, 0x0b, 0x07, 0x27,
    0x03, 0x07, 0x72, 0x75, 0x6e, 0x5f, 0x72, 0x65, 0x66, 0x00, 0x01, 0x0a,
    0x72, 0x75, 0x6e, 0x5f, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x00, 0x02,
    0x0c, 0x72, 0x75, 0x6e, 0x5f, 0x69, 0x6e, 0x64, 0x69, 0x72, 0x65, 0x63,
    0x74, 0x00, 0x03, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x00,
    0x0a, 0x2d, 0x03, 0x0e, 0x00, 0xd2, 0x00, 0x14, 0x00, 0x41, 0x0a, 0x6c,
    0x41, 0x00, 0x40, 0x00, 0x6a, 0x0b, 0x0c, 0x00, 0x10, 0x00, 0x41, 0x0a,
    0x6c, 0x41, 0x00, 0x40, 0x00, 0x6a, 0x0b, 0x0f, 0x00, 0x41, 0x00, 0x11,
    0x00, 0x00, 0x41, 0x0a, 0x6c, 0x41, 0x00, 0x40, 0x00, 0x6a, 0x0b, 0x00,
    0x1a, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07, 0x01, 0x00, 0x04, 0x72,
    0x65, 0x61, 0x64, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74, 0x07, 0x04, 0x01,
    0x00, 0x01, 0x67};

// Parse, validate, compile, and attach compiled symbols. The Loader gets the
// executor's intrinsics table so loadExecutable can patch the code's intrinsics
// global; without it, intrinsic calls jump through a null table.
std::shared_ptr<AST::Module> compileToJIT(const Configure &Conf,
                                          Span<const Byte> Bytes) {
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

// Parse and validate a module without compiling it, so its functions run under
// the interpreter. Exercises interpreterStackTrace directly.
std::shared_ptr<AST::Module> loadModule(const Configure &Conf,
                                        Span<const Byte> Bytes) {
  Loader::Loader LoaderEngine(Conf);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(Bytes);
  if (!ModOrErr) {
    return nullptr;
  }
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  if (!ValidatorEngine.validate(*Mod)) {
    return nullptr;
  }
  return Mod;
}

// Both modules are compiled, then registered and instantiated in one store
// under one executor, so the result is not an artifact of cross-VM sharing.
TEST(AOTCrossModule, CompiledCallUsesCalleeContext) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);

  // Declared before the instances that reference their compiled code.
  auto CalleeMod = compileToJIT(Conf, CrossModuleCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  // CalleeInst is declared first so the dependent caller tears down before it.
  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);
  const auto *ReadFn = CalleeInst->findFuncExports("read");
  ASSERT_NE(ReadFn, nullptr);
  ASSERT_TRUE(ReadFn->isCompiledFunction());

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  // call_ref of a cross-module funcref must read the callee's global.
  const auto *RunRef = CallerInst->findFuncExports("run_ref");
  ASSERT_NE(RunRef, nullptr);
  ASSERT_TRUE(RunRef->isCompiledFunction());
  auto RRef = ExecEngine.invoke(RunRef, {}, {});
  ASSERT_TRUE(RRef);
  ASSERT_EQ(RRef->size(), 1u);
  EXPECT_EQ((*RRef)[0].first.get<uint32_t>(), 1871u)
      << "call_ref did not keep the two modules' contexts apart";

  const auto *RunDirect = CallerInst->findFuncExports("run_direct");
  ASSERT_NE(RunDirect, nullptr);
  ASSERT_TRUE(RunDirect->isCompiledFunction());
  auto RDir = ExecEngine.invoke(RunDirect, {}, {});
  ASSERT_TRUE(RDir);
  ASSERT_EQ(RDir->size(), 1u);
  EXPECT_EQ((*RDir)[0].first.get<uint32_t>(), 1871u)
      << "direct call did not keep the two modules' contexts apart";

  // call_indirect resolves through proxyTableGetFuncSymbol, which must mediate
  // as well.
  const auto *RunInd = CallerInst->findFuncExports("run_indirect");
  ASSERT_NE(RunInd, nullptr);
  ASSERT_TRUE(RunInd->isCompiledFunction());
  auto RInd = ExecEngine.invoke(RunInd, {}, {});
  ASSERT_TRUE(RInd);
  ASSERT_EQ(RInd->size(), 1u);
  EXPECT_EQ((*RInd)[0].first.get<uint32_t>(), 1871u)
      << "call_indirect did not keep the two modules' contexts apart";
}

// A cross-module return_call_ref must run each side with its own state. The
// kCallRef proxy nests a proxy and an enterFunction frame (~1 KiB) per hop, so
// the recursion stays well within a 1 MiB thread stack. This covers context
// handling only; constant-space cross-module tail calls are not implemented.
//   callee: (global $g (mut i32) 187) (global $peer (export "peer")
//           (mut (ref null $t)) (ref.null $t))
//           (func $f (export "f") (param i32) (result i32)
//             if (global.get $g) != 187 -> 1
//             if param == 0 -> 0
//             return_call_ref $t (param - 1) (global.get $peer))
//   caller: (import "callee" "f") (import "callee" "peer")
//           (global $g (mut i32) 170)
//           (func $a (param i32) (result i32)
//             if (global.get $g) != 170 -> 2
//             if param == 0 -> 0
//             return_call_ref $t (param - 1) (ref.func $f))
//           (func (export "run") (param i32) (result i32)
//             global.set $peer (ref.func $a)
//             return_call_ref $t (param) (ref.func $f))
const std::array<WasmEdge::Byte, 116> CrossModuleTailCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x06, 0x0d, 0x02, 0x7f,
    0x01, 0x41, 0xbb, 0x01, 0x0b, 0x63, 0x00, 0x01, 0xd0, 0x00, 0x0b, 0x07,
    0x0c, 0x02, 0x04, 0x70, 0x65, 0x65, 0x72, 0x03, 0x01, 0x01, 0x66, 0x00,
    0x00, 0x0a, 0x22, 0x01, 0x20, 0x00, 0x23, 0x00, 0x41, 0xbb, 0x01, 0x47,
    0x04, 0x40, 0x41, 0x01, 0x0f, 0x0b, 0x20, 0x00, 0x45, 0x04, 0x40, 0x41,
    0x00, 0x0f, 0x0b, 0x20, 0x00, 0x41, 0x01, 0x6b, 0x23, 0x01, 0x15, 0x00,
    0x0b, 0x00, 0x1d, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x04, 0x01, 0x00,
    0x01, 0x66, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74, 0x07, 0x0a, 0x02, 0x00,
    0x01, 0x67, 0x01, 0x04, 0x70, 0x65, 0x65, 0x72};

const std::array<WasmEdge::Byte, 160> CrossModuleTailCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x02, 0x1c, 0x02, 0x06, 0x63, 0x61, 0x6c, 0x6c,
    0x65, 0x65, 0x01, 0x66, 0x00, 0x00, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
    0x65, 0x04, 0x70, 0x65, 0x65, 0x72, 0x03, 0x63, 0x00, 0x01, 0x03, 0x03,
    0x02, 0x00, 0x00, 0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xaa, 0x01, 0x0b,
    0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02, 0x09, 0x06, 0x01,
    0x03, 0x00, 0x02, 0x01, 0x00, 0x0a, 0x2f, 0x02, 0x20, 0x00, 0x23, 0x01,
    0x41, 0xaa, 0x01, 0x47, 0x04, 0x40, 0x41, 0x02, 0x0f, 0x0b, 0x20, 0x00,
    0x45, 0x04, 0x40, 0x41, 0x00, 0x0f, 0x0b, 0x20, 0x00, 0x41, 0x01, 0x6b,
    0xd2, 0x00, 0x15, 0x00, 0x0b, 0x0c, 0x00, 0xd2, 0x01, 0x24, 0x00, 0x20,
    0x00, 0xd2, 0x00, 0x15, 0x00, 0x0b, 0x00, 0x20, 0x04, 0x6e, 0x61, 0x6d,
    0x65, 0x01, 0x07, 0x02, 0x00, 0x01, 0x66, 0x01, 0x01, 0x61, 0x04, 0x04,
    0x01, 0x00, 0x01, 0x74, 0x07, 0x0a, 0x02, 0x00, 0x04, 0x70, 0x65, 0x65,
    0x72, 0x01, 0x01, 0x67};

TEST(AOTCrossModule, CompiledTailCallUsesCalleeContext) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::TailCall);

  auto CalleeMod = compileToJIT(Conf, CrossModuleTailCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleTailCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());

  const std::array<ValVariant, 1> Args{ValVariant(uint32_t(8U))};
  const std::array<ValType, 1> ArgTypes{ValType(TypeCode::I32)};
  auto R = ExecEngine.invoke(Run, Args, ArgTypes);
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 0u)
      << "a cross-module tail call ran with the wrong module's globals";
}

// Lazy JIT resolves calls through its own per-function symbol cache, so cover
// the same reads under RunMode::LazyJIT.
TEST(AOTCrossModule, LazyJITCallUsesCalleeContext) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  VM::VM VM(Conf);

  ASSERT_TRUE(VM.registerModule("callee"sv, CrossModuleCalleeWasm));
  ASSERT_TRUE(VM.loadWasm(CrossModuleCallerWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  for (const auto &Name : {"run_direct"sv, "run_ref"sv, "run_indirect"sv}) {
    auto R = VM.execute(Name);
    ASSERT_TRUE(R) << Name;
    ASSERT_EQ(R->size(), 1u) << Name;
    EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1871u)
        << Name << " did not keep the two modules' contexts apart";
  }
}

const std::array<WasmEdge::Byte, 67> CrossModuleNestCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x05,
    0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xbb, 0x01, 0x0b, 0x07, 0x05, 0x01,
    0x01, 0x63, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x23, 0x00, 0x0b,
    0x00, 0x11, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01,
    0x74, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

const std::array<WasmEdge::Byte, 115> CrossModuleNestCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x0c, 0x01, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
    0x65, 0x01, 0x63, 0x00, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00, 0x05, 0x03,
    0x01, 0x00, 0x01, 0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xaa, 0x01, 0x0b,
    0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02, 0x09, 0x05, 0x01,
    0x03, 0x00, 0x01, 0x00, 0x0a, 0x15, 0x02, 0x06, 0x00, 0xd2, 0x00, 0x15,
    0x00, 0x0b, 0x0c, 0x00, 0x10, 0x01, 0x41, 0x0a, 0x6c, 0x41, 0x00, 0x40,
    0x00, 0x6a, 0x0b, 0x00, 0x1a, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07,
    0x02, 0x00, 0x01, 0x63, 0x01, 0x01, 0x62, 0x04, 0x04, 0x01, 0x00, 0x01,
    0x74, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

// A tail call must not leave the caller's caller running with the target's
// module. Here A.a calls A.b through the same-module fast path, which pushes no
// frame, and A.b tail-calls into B; when B returns, control lands back in A.a,
// which must still resolve its own memory.
TEST(AOTCrossModule, TailCallLeavesCallerContextIntact) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::TailCall);

  auto CalleeMod = compileToJIT(Conf, CrossModuleNestCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleNestCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;
  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);
  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());
  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1871u)
      << "the caller resumed with the tail-call target's module";
}

// A cross-module return_call_indirect must hand the callee its own module. This
// covers compileReturnIndirectCallOp, the last cross-module tail site (call_ref
// and call_indirect are covered above). The probe combines the callee's global
// and memory.grow(0), both in the callee's body: the callee context gives
// 187*10 + 5 = 1875; a leaked caller context reads 170 and a 1-page memory for
// 1701.
//   callee: (memory 5) (global $g (mut i32) 187)
//           (func (export "probe") (result i32)
//             (i32.add (i32.mul (global.get $g) (i32.const 10))
//                      (memory.grow (i32.const 0))))
//   caller: (import "callee" "probe" (func $probe (result i32)))
//           (table 1 funcref) (elem (i32.const 0) func $probe)
//           (memory 1) (global $g (mut i32) 170)
//           (func (export "run") (result i32)
//             (return_call_indirect (type $t) (i32.const 0)))
const std::array<WasmEdge::Byte, 89> CrossModuleRetIndirectCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x05,
    0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xbb, 0x01, 0x0b, 0x07, 0x09, 0x01,
    0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x0a, 0x0e, 0x01, 0x0c,
    0x00, 0x23, 0x00, 0x41, 0x0a, 0x6c, 0x41, 0x00, 0x40, 0x00, 0x6a, 0x0b,
    0x00, 0x1b, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05,
    0x70, 0x72, 0x6f, 0x62, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74, 0x07,
    0x04, 0x01, 0x00, 0x01, 0x67};

const std::array<WasmEdge::Byte, 120> CrossModuleRetIndirectCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
    0x65, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x03, 0x02, 0x01,
    0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01,
    0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xaa, 0x01, 0x0b, 0x07, 0x07, 0x01,
    0x03, 0x72, 0x75, 0x6e, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00,
    0x0b, 0x01, 0x00, 0x0a, 0x09, 0x01, 0x07, 0x00, 0x41, 0x00, 0x13, 0x00,
    0x00, 0x0b, 0x00, 0x20, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0d, 0x02,
    0x00, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x01, 0x03, 0x72, 0x75, 0x6e,
    0x04, 0x04, 0x01, 0x00, 0x01, 0x74, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

TEST(AOTCrossModule, CompiledReturnCallIndirectUsesCalleeContext) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::TailCall);

  auto CalleeMod = compileToJIT(Conf, CrossModuleRetIndirectCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleRetIndirectCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());
  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1875u)
      << "a cross-module return_call_indirect did not run with the callee's "
         "module";
}

// Gas metering is one thread-local counter shared by the whole call chain, so
// it must keep charging once a compiled call crosses into another module. Run 1
// measures the total cost of one cross-module run_direct invocation; run 2 sets
// the limit one unit below that and must fail with CostLimitExceeded, proving
// the callee's instructions charge against the same budget as the caller's.
TEST(AOTCrossModule, GasMeteringSpansCrossModuleCall) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.getStatisticsConfigure().setCostMeasuring(true);

  auto CalleeMod = compileToJIT(Conf, CrossModuleCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  uint64_t FullCost = 0;
  {
    Statistics::Statistics Stat;
    Executor::Executor ExecEngine(Conf, &Stat);
    Runtime::StoreManager Store;

    auto CalleeInstOrErr =
        ExecEngine.registerModule(Store, *CalleeMod, "callee");
    ASSERT_TRUE(CalleeInstOrErr);
    auto CalleeInst = std::move(*CalleeInstOrErr);
    auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
    ASSERT_TRUE(CallerInstOrErr);
    auto CallerInst = std::move(*CallerInstOrErr);

    const auto *RunDirect = CallerInst->findFuncExports("run_direct");
    ASSERT_NE(RunDirect, nullptr);
    auto R = ExecEngine.invoke(RunDirect, {}, {});
    ASSERT_TRUE(R);
    ASSERT_EQ(R->size(), 1u);
    EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1871u);

    FullCost = Stat.getTotalCost();
    EXPECT_GT(FullCost, 0u)
        << "gas metering did not run across the cross-module call";
  }

  // Executor::Executor() re-seeds the cost limit from its Configure, so set the
  // second run's limit here rather than on the Statistics constructor.
  Conf.getStatisticsConfigure().setCostLimit(FullCost - 1);
  {
    Statistics::Statistics Stat;
    Executor::Executor ExecEngine(Conf, &Stat);
    Runtime::StoreManager Store;

    auto CalleeInstOrErr =
        ExecEngine.registerModule(Store, *CalleeMod, "callee");
    ASSERT_TRUE(CalleeInstOrErr);
    auto CalleeInst = std::move(*CalleeInstOrErr);
    auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
    ASSERT_TRUE(CallerInstOrErr);
    auto CallerInst = std::move(*CallerInstOrErr);

    const auto *RunDirect = CallerInst->findFuncExports("run_direct");
    ASSERT_NE(RunDirect, nullptr);
    auto R = ExecEngine.invoke(RunDirect, {}, {});
    ASSERT_FALSE(R) << "a cost limit one below the measured cross-module "
                       "total should have been enforced";
    EXPECT_EQ(R.error(), ErrCode::Value::CostLimitExceeded);
  }
}

// One compiled image can back many instances. The shared module is registered
// twice under different names, each with its own mutated global; a third module
// call_refs "read" on both. If context selection ever keyed on the (identical)
// compiled function pointer instead of the calling FunctionInstance, it fails.
//   shared: (global $g (export "g") (mut i32) 0)
//           (func (export "read") (result i32) (global.get $g))
//   caller: (import "inst_a" "read") (import "inst_b" "read")
//           (func (export "run") (result i32)
//             (i32.add (i32.mul (call_ref $t (ref.func $ra)) (i32.const 1000))
//                      (call_ref $t (ref.func $rb))))
const std::array<WasmEdge::Byte, 71> CrossModuleSharedInstWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x06, 0x06, 0x01, 0x7f, 0x01,
    0x41, 0x00, 0x0b, 0x07, 0x0c, 0x02, 0x01, 0x67, 0x03, 0x00, 0x04, 0x72,
    0x65, 0x61, 0x64, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x23, 0x00,
    0x0b, 0x00, 0x14, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07, 0x01, 0x00,
    0x04, 0x72, 0x65, 0x61, 0x64, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

const std::array<WasmEdge::Byte, 115> CrossModuleSharedInstCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x1d, 0x02, 0x06, 0x69, 0x6e, 0x73, 0x74, 0x5f,
    0x61, 0x04, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x06, 0x69, 0x6e, 0x73,
    0x74, 0x5f, 0x62, 0x04, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x03, 0x02,
    0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02, 0x09,
    0x06, 0x01, 0x03, 0x00, 0x02, 0x00, 0x01, 0x0a, 0x11, 0x01, 0x0f, 0x00,
    0xd2, 0x00, 0x14, 0x00, 0x41, 0xe8, 0x07, 0x6c, 0xd2, 0x01, 0x14, 0x00,
    0x6a, 0x0b, 0x00, 0x1b, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0e, 0x03,
    0x00, 0x02, 0x72, 0x61, 0x01, 0x02, 0x72, 0x62, 0x02, 0x03, 0x72, 0x75,
    0x6e, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74};

TEST(AOTCrossModule, SameCompiledModuleTwoInstancesKeepOwnState) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);

  auto SharedMod = compileToJIT(Conf, CrossModuleSharedInstWasm);
  ASSERT_NE(SharedMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleSharedInstCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto InstAOrErr = ExecEngine.registerModule(Store, *SharedMod, "inst_a");
  ASSERT_TRUE(InstAOrErr);
  auto InstA = std::move(*InstAOrErr);
  auto InstBOrErr = ExecEngine.registerModule(Store, *SharedMod, "inst_b");
  ASSERT_TRUE(InstBOrErr);
  auto InstB = std::move(*InstBOrErr);
  ASSERT_NE(InstA.get(), InstB.get())
      << "registering the same compiled module twice must produce two "
         "distinct instances";

  auto *GlobalA = InstA->findGlobalExports("g");
  ASSERT_NE(GlobalA, nullptr);
  GlobalA->setValue(ValVariant(uint32_t(111U)));
  auto *GlobalB = InstB->findGlobalExports("g");
  ASSERT_NE(GlobalB, nullptr);
  GlobalB->setValue(ValVariant(uint32_t(222U)));

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());
  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 111222u)
      << "the two instances of the identical compiled module did not keep "
         "their own global state apart";
}

// A host function reached from a compiled cross-module callee must see the
// callee's own CallingFrame so CallingFrame::getMemoryByIndex() resolves the
// callee's memory. The callee is entered via call_ref (a native call that never
// touches StackManager), then calls its host import; the explicit CallerModInst
// threaded through Executor::enterFunction keeps that host call's module right.
class CrossModuleReadMem0 : public Runtime::HostFunction<CrossModuleReadMem0> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame) {
    auto *MemInst = Frame.getMemoryByIndex(0);
    if (MemInst == nullptr) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    const auto *Ptr = MemInst->getPointer<const uint32_t *>(0);
    if (Ptr == nullptr) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    return *Ptr;
  }
};

class CrossModuleHostModule : public Runtime::Instance::ModuleInstance {
public:
  CrossModuleHostModule() : ModuleInstance("host") {
    addHostFunc("read0", std::make_unique<CrossModuleReadMem0>());
  }
};

//   callee: (import "host" "read0" (func $read0 (result i32)))
//           (memory 1) (data (i32.const 0) "\22\22\22\00")
//           (func (export "probe") (result i32) (call $read0))
//   caller: (import "callee" "probe" (func $probe (result i32)))
//           (memory 1) (data (i32.const 0) "\11\11\11\00")
//           (func (export "run") (result i32)
//             (call_ref $t (ref.func $probe)))
const std::array<WasmEdge::Byte, 101> CrossModuleHostCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x0e, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x05,
    0x72, 0x65, 0x61, 0x64, 0x30, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05,
    0x03, 0x01, 0x00, 0x01, 0x07, 0x09, 0x01, 0x05, 0x70, 0x72, 0x6f, 0x62,
    0x65, 0x00, 0x01, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x0b,
    0x0a, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x04, 0x22, 0x22, 0x22, 0x00, 0x00,
    0x1c, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0f, 0x02, 0x00, 0x05, 0x72,
    0x65, 0x61, 0x64, 0x30, 0x01, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x04,
    0x04, 0x01, 0x00, 0x01, 0x74};

const std::array<WasmEdge::Byte, 108> CrossModuleHostCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
    0x65, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x03, 0x02, 0x01,
    0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75,
    0x6e, 0x00, 0x01, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x08,
    0x01, 0x06, 0x00, 0xd2, 0x00, 0x14, 0x00, 0x0b, 0x0b, 0x0a, 0x01, 0x00,
    0x41, 0x00, 0x0b, 0x04, 0x11, 0x11, 0x11, 0x00, 0x00, 0x1a, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x01, 0x0d, 0x02, 0x00, 0x05, 0x70, 0x72, 0x6f, 0x62,
    0x65, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74};

TEST(AOTCrossModule, HostFunctionSeesCalleeMemory) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);

  auto CalleeMod = compileToJIT(Conf, CrossModuleHostCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleHostCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  CrossModuleHostModule HostMod;
  ASSERT_TRUE(ExecEngine.registerModule(Store, HostMod));

  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());
  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 0x00222222U)
      << "the host function saw the caller's memory instead of the callee's";
}

// A cross-module struct.new_default must resolve its type index against the
// callee's own type section and own the result under the callee's module. The
// modules number types differently: callee $S is at index 1, while caller index
// 1 is a differently-shaped struct ($Collide), so a wrongly-bound context reads
// a valid but wrong type instead of crashing. $S's second field is a concrete
// (ref null $T), the only default-init field kind that reaches toBottomType.
//   callee: (type $T (struct (field i32)))
//           (type $S (struct (field $tag (mut i32)) (field $next (ref null
//           $T)))) (func (export "make") (result (ref $S)) (struct.new_default
//           $S))
//   caller: (type $Collide (struct (field (mut i32)) (field (mut i32))))
//           (type $S_caller (struct (field (mut i32)) (field (ref null
//           $T_caller)))) (import "callee" "make" (func $make (result (ref
//           $S_caller)))) (table 1 funcref) (elem (i32.const 0) func $make)
//           (func (export "run") (result (ref $S_caller))
//             (call_indirect (type $MkTy) (i32.const 0)))
const std::array<WasmEdge::Byte, 97> CrossModuleGCCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x11, 0x03,
    0x5f, 0x01, 0x7f, 0x00, 0x5f, 0x02, 0x7f, 0x01, 0x63, 0x00, 0x00,
    0x60, 0x00, 0x01, 0x64, 0x01, 0x03, 0x02, 0x01, 0x02, 0x07, 0x08,
    0x01, 0x04, 0x6d, 0x61, 0x6b, 0x65, 0x00, 0x00, 0x0a, 0x07, 0x01,
    0x05, 0x00, 0xfb, 0x01, 0x01, 0x0b, 0x00, 0x2d, 0x04, 0x6e, 0x61,
    0x6d, 0x65, 0x01, 0x07, 0x01, 0x00, 0x04, 0x6d, 0x61, 0x6b, 0x65,
    0x04, 0x0d, 0x03, 0x00, 0x01, 0x54, 0x01, 0x01, 0x53, 0x02, 0x04,
    0x4d, 0x6b, 0x54, 0x79, 0x0a, 0x0e, 0x01, 0x01, 0x02, 0x00, 0x03,
    0x74, 0x61, 0x67, 0x01, 0x04, 0x6e, 0x65, 0x78, 0x74};

const std::array<WasmEdge::Byte, 160> CrossModuleGCCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1a, 0x05, 0x60,
    0x00, 0x00, 0x5f, 0x02, 0x7f, 0x01, 0x7f, 0x01, 0x5f, 0x01, 0x7f, 0x00,
    0x5f, 0x02, 0x7f, 0x01, 0x63, 0x02, 0x00, 0x60, 0x00, 0x01, 0x64, 0x03,
    0x02, 0x0f, 0x01, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65, 0x65, 0x04, 0x6d,
    0x61, 0x6b, 0x65, 0x00, 0x04, 0x03, 0x02, 0x01, 0x04, 0x04, 0x04, 0x01,
    0x70, 0x00, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01,
    0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x00, 0x0a, 0x09, 0x01,
    0x07, 0x00, 0x41, 0x00, 0x11, 0x04, 0x00, 0x0b, 0x00, 0x42, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x01, 0x0c, 0x02, 0x00, 0x04, 0x6d, 0x61, 0x6b, 0x65,
    0x01, 0x03, 0x72, 0x75, 0x6e, 0x04, 0x2d, 0x05, 0x00, 0x07, 0x41, 0x46,
    0x69, 0x6c, 0x6c, 0x65, 0x72, 0x01, 0x07, 0x43, 0x6f, 0x6c, 0x6c, 0x69,
    0x64, 0x65, 0x02, 0x08, 0x54, 0x5f, 0x63, 0x61, 0x6c, 0x6c, 0x65, 0x72,
    0x03, 0x08, 0x53, 0x5f, 0x63, 0x61, 0x6c, 0x6c, 0x65, 0x72, 0x04, 0x04,
    0x4d, 0x6b, 0x54, 0x79};

TEST(AOTCrossModule, CrossModuleStructNewOwnedByCallee) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::GC);

  auto CalleeMod = compileToJIT(Conf, CrossModuleGCCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleGCCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());
  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);

  // Struct payloads live in the GC heap: the ref points at GCInstance::RawData,
  // whose leading ModInst names the owning module.
  const auto *Raw = (*R)[0]
                        .first.get<RefVariant>()
                        .getPtr<const Runtime::Instance::GCInstance::RawData>();
  ASSERT_NE(Raw, nullptr);
  EXPECT_EQ(Raw->ModInst, CalleeInst.get())
      << "the struct created by the callee's struct.new_default is not "
         "owned by the callee module";
  EXPECT_NE(Raw->ModInst, CallerInst.get())
      << "the struct is owned by the caller instead of the callee";
  ASSERT_EQ(Raw->Length, 2u);
  EXPECT_EQ(Raw->data()[0].get<uint32_t>(), 0u);
  EXPECT_TRUE(Raw->data()[1].get<RefVariant>().isNull())
      << "the default-initialized concrete reference field did not resolve "
         "to a null bottom type through the callee's own type section";
}

// A compiled throw must resolve its tag in the throwing module's tag space. The
// two modules disagree on what tag index 1 means: it is $b1 in the callee but
// the caller's own $a1, so a throw resolved against the caller is caught by
// catch_all (2) instead of the imported $b1 (1). call_ref is the probe because
// it reaches the callee natively, without an interpreter frame to carry the
// module.
//   callee: (tag $b0) (tag $b1) (export "b1" (tag $b1))
//           (func (export "throw1") (throw $b1))
//   caller: (import "callee" "b1" (tag $ib1))
//           (import "callee" "throw1" (func $throw1))
//           (tag $a1)
//           (func (export "run") (result i32)
//             (block $on_b1 (block $on_any
//               (try_table (catch $ib1 $on_b1) (catch_all $on_any)
//                 (call_ref $v (ref.func $throw1)))
//               (return (i32.const 0)))
//               (return (i32.const 2)))
//             (i32.const 1))
const std::array<WasmEdge::Byte, 85> CrossModuleThrowCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
    0x60, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x0d, 0x05, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x07, 0x0f, 0x02, 0x06, 0x74, 0x68, 0x72, 0x6f,
    0x77, 0x31, 0x00, 0x00, 0x02, 0x62, 0x31, 0x04, 0x01, 0x0a, 0x06,
    0x01, 0x04, 0x00, 0x08, 0x01, 0x0b, 0x00, 0x21, 0x04, 0x6e, 0x61,
    0x6d, 0x65, 0x01, 0x09, 0x01, 0x00, 0x06, 0x74, 0x68, 0x72, 0x6f,
    0x77, 0x31, 0x04, 0x04, 0x01, 0x00, 0x01, 0x76, 0x0b, 0x09, 0x02,
    0x00, 0x02, 0x62, 0x30, 0x01, 0x02, 0x62, 0x31};

const std::array<WasmEdge::Byte, 164> CrossModuleThrowCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
    0x00, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x02, 0x1e, 0x02, 0x06, 0x63, 0x61,
    0x6c, 0x6c, 0x65, 0x65, 0x02, 0x62, 0x31, 0x04, 0x00, 0x00, 0x06, 0x63,
    0x61, 0x6c, 0x6c, 0x65, 0x65, 0x06, 0x74, 0x68, 0x72, 0x6f, 0x77, 0x31,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x01, 0x0d, 0x03, 0x01, 0x00, 0x00, 0x07,
    0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01, 0x09, 0x05, 0x01, 0x03,
    0x00, 0x01, 0x00, 0x0a, 0x1f, 0x01, 0x1d, 0x00, 0x02, 0x40, 0x02, 0x40,
    0x1f, 0x40, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0xd2, 0x00, 0x14, 0x00,
    0x0b, 0x41, 0x00, 0x0f, 0x0b, 0x41, 0x02, 0x0f, 0x0b, 0x41, 0x01, 0x0b,
    0x00, 0x36, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x09, 0x01, 0x00, 0x06,
    0x74, 0x68, 0x72, 0x6f, 0x77, 0x31, 0x03, 0x12, 0x01, 0x01, 0x02, 0x00,
    0x05, 0x6f, 0x6e, 0x5f, 0x62, 0x31, 0x01, 0x06, 0x6f, 0x6e, 0x5f, 0x61,
    0x6e, 0x79, 0x04, 0x04, 0x01, 0x00, 0x01, 0x76, 0x0b, 0x0a, 0x02, 0x00,
    0x03, 0x69, 0x62, 0x31, 0x01, 0x02, 0x61, 0x31};

TEST(AOTCrossModule, CompiledThrowUsesCalleeTagSpace) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::ExceptionHandling);

  auto CalleeMod = compileToJIT(Conf, CrossModuleThrowCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CrossModuleThrowCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());
  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 1u)
      << "the callee's throw did not resolve its tag in the callee's module";
}

//   callee (trapper2): (func $g (result i32) unreachable)
//                       (func $f (export "f") (result i32) (call $g))
const std::array<WasmEdge::Byte, 55> InterpTrapCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x00, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00, 0x00, 0x07, 0x05,
    0x01, 0x01, 0x66, 0x00, 0x01, 0x0a, 0x0a, 0x02, 0x03, 0x00, 0x00,
    0x0b, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x0e, 0x04, 0x6e, 0x61,
    0x6d, 0x65, 0x01, 0x07, 0x02, 0x00, 0x01, 0x67, 0x01, 0x01, 0x66};

//   caller: (import "trapper2" "f" (func $f (result i32)))
//           (func (export "run") (result i32) (call $f))
const std::array<WasmEdge::Byte, 65> InterpTrapCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x00, 0x01, 0x7f, 0x02, 0x0e, 0x01, 0x08, 0x74, 0x72, 0x61,
    0x70, 0x70, 0x65, 0x72, 0x32, 0x01, 0x66, 0x00, 0x00, 0x03, 0x02,
    0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01,
    0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x0b, 0x04,
    0x6e, 0x61, 0x6d, 0x65, 0x01, 0x04, 0x01, 0x00, 0x01, 0x66};

// A cross-module interpreter stack must attribute each frame to its own module.
// run() (caller) calls f() (callee) calls g(), which traps; the caller's frame
// must resolve against the caller module, not the top (callee) module.
TEST(AOTCrossModule, InterpreterTrapAttributesCallerModule) {
  Configure Conf;

  auto CalleeMod = loadModule(Conf, InterpTrapCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = loadModule(Conf, InterpTrapCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr =
      ExecEngine.registerModule(Store, *CalleeMod, "trapper2");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);

  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_FALSE(R);

  auto Trace = Executor::Executor::getRecordedStackTrace();
  bool SawCaller = false;
  bool SawCallee = false;
  for (const auto &E : Trace) {
    if (E.Module == CallerInst.get()) {
      SawCaller = true;
    }
    if (E.Module == CalleeInst.get()) {
      SawCallee = true;
    }
  }
  EXPECT_TRUE(SawCaller) << "cross-module interpreter frame was not attributed "
                            "to the caller module";
  EXPECT_TRUE(SawCallee);
}

//   callee (trapper): (func (export "boom") (result i32) unreachable)
const std::array<WasmEdge::Byte, 36> TrapCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x08, 0x01, 0x04, 0x62,
    0x6f, 0x6f, 0x6d, 0x00, 0x00, 0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b};

//   caller: (import "trapper" "boom" (func $boom (result i32)))
//           (table 1 funcref) (elem (i32.const 0) $boom)
//           (func (export "run") (result i32)
//             (call_indirect (type $t) (i32.const 0)))
const std::array<WasmEdge::Byte, 94> TrapCallerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x07, 0x74, 0x72, 0x61, 0x70, 0x70,
    0x65, 0x72, 0x04, 0x62, 0x6f, 0x6f, 0x6d, 0x00, 0x00, 0x03, 0x02, 0x01,
    0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72,
    0x75, 0x6e, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01,
    0x00, 0x0a, 0x09, 0x01, 0x07, 0x00, 0x41, 0x00, 0x11, 0x00, 0x00, 0x0b,
    0x00, 0x14, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07, 0x01, 0x00, 0x04,
    0x62, 0x6f, 0x6f, 0x6d, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74};

// A trap inside a frameless cross-module compiled callee must be attributed to
// the callee's module. The caller's run() call_indirects into the callee's
// boom(), which traps. The only top-frame module is the caller, so resolving
// the callee's frame needs the whole live store enumerated.
TEST(AOTCrossModule, CompiledFramelessTrapAttributedToCallee) {
  Configure Conf;
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::FunctionReferences);

  auto CalleeMod = compileToJIT(Conf, TrapCalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, TrapCallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr =
      ExecEngine.registerModule(Store, *CalleeMod, "trapper");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);
  const auto *Boom = CalleeInst->findFuncExports("boom");
  ASSERT_NE(Boom, nullptr);
  ASSERT_TRUE(Boom->isCompiledFunction());

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *Run = CallerInst->findFuncExports("run");
  ASSERT_NE(Run, nullptr);
  ASSERT_TRUE(Run->isCompiledFunction());

  auto R = ExecEngine.invoke(Run, {}, {});
  ASSERT_FALSE(R);

  auto Trace = Executor::Executor::getRecordedStackTrace();
  if (Trace.empty()) {
    // Walking a compiled frame needs unwind information that the generated
    // code registers only on some platforms; without it there is nothing to
    // attribute.
    GTEST_SKIP() << "no compiled frame was walkable on this platform";
  }
  bool SawCallee = false;
  for (const auto &E : Trace) {
    if (E.Module == CalleeInst.get()) {
      SawCallee = true;
    }
  }
  EXPECT_TRUE(SawCallee)
      << "frameless cross-module trap was not attributed to the callee module";
}

TEST_P(NativeCoreTest, TestSuites) {
  auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  // Native AOT spec test: explicitly opt into RunMode::AOT so the runtime
  // load step uses the produced .so as AOT, instead of falling back to
  // interpreter under the new default mode.
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    WasmEdge::Configure Conf;
    TestContext(const WasmEdge::Configure &C) : VM(C), Conf(C) {
      VM.registerModule(SpecTestMod);
    }
    Expect<std::string> compile(const std::string &FileName) {
      WasmEdge::Configure CopyConf = Conf;
      WasmEdge::Loader::Loader Loader(Conf);
      WasmEdge::Validator::Validator ValidatorEngine(Conf);
      CopyConf.getCompilerConfigure().setOutputFormat(
          CompilerConfigure::OutputFormat::Native);
      CopyConf.getCompilerConfigure().setOptimizationLevel(
          WasmEdge::CompilerConfigure::OptimizationLevel::O0);
      CopyConf.getCompilerConfigure().setDumpIR(true);
      WasmEdge::LLVM::Compiler Compiler(CopyConf);
      WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
      auto Path = std::filesystem::u8path(FileName);
      Path.replace_extension(std::filesystem::u8path(WASMEDGE_LIB_EXTENSION));
      const auto SOPath = Path.u8string();
      std::vector<WasmEdge::Byte> Data;
      std::unique_ptr<WasmEdge::AST::Module> Module;
      return Loader.loadFile(FileName)
          .and_then([&](auto Result) noexcept {
            Data = std::move(Result);
            return Loader.parseModule(Data);
          })
          .and_then([&](auto Result) noexcept {
            Module = std::move(Result);
            return ValidatorEngine.validate(*Module);
          })
          .and_then([&]() noexcept { return Compiler.compile(*Module); })
          .and_then([&](auto Result) noexcept {
            return CodeGen.codegen(Data, std::move(Result), SOPath);
          })
          .and_then([&]() noexcept { return Expect<std::string>{SOPath}; });
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          Ctx->VM.registerModule(AliasName, *ModInst);
        }
      }
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onModule = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
        [&VM, &ModName](const std::string &SOFileName) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFileName);
          } else {
            return VM.loadWasm(SOFileName)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
        [&VM](const std::string &SOFileName) -> Expect<SpecTest::WasmUnit> {
          Loader::Loader &Loader = VM.getLoader();
          Validator::Validator &Validator = VM.getValidator();
          EXPECTED_TRY(auto ASTMod, Loader.parseModule(SOFileName));
          EXPECTED_TRY(Validator.validate(*ASTMod.get()));
          return ASTMod;
        });
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in the VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

TEST_P(CustomWasmCoreTest, TestSuites) {
  auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  // Universal-WASM AOT spec test: produced files are .aot.wasm (universal
  // WASM with an AOT custom section). Opt into RunMode::AOT so the
  // runtime load step actually loads the AOT section.
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    WasmEdge::Configure Conf;
    TestContext(const WasmEdge::Configure &C) : VM(C), Conf(C) {
      VM.registerModule(SpecTestMod);
    }
    Expect<std::string> compile(const std::string &FileName) {
      WasmEdge::Configure CopyConf = Conf;
      WasmEdge::Loader::Loader Loader(Conf);
      WasmEdge::Validator::Validator ValidatorEngine(Conf);
      CopyConf.getCompilerConfigure().setOptimizationLevel(
          WasmEdge::CompilerConfigure::OptimizationLevel::O0);
      CopyConf.getCompilerConfigure().setDumpIR(true);
      WasmEdge::LLVM::Compiler Compiler(CopyConf);
      WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
      auto Path = std::filesystem::u8path(FileName);
      Path.replace_extension(std::filesystem::u8path(".aot.wasm"));
      const auto SOPath = Path.u8string();
      std::vector<WasmEdge::Byte> Data;
      std::unique_ptr<WasmEdge::AST::Module> Module;
      return Loader.loadFile(FileName)
          .and_then([&](auto Result) noexcept {
            Data = std::move(Result);
            return Loader.parseModule(Data);
          })
          .and_then([&](auto Result) noexcept {
            Module = std::move(Result);
            return ValidatorEngine.validate(*Module);
          })
          .and_then([&]() noexcept { return Compiler.compile(*Module); })
          .and_then([&](auto Result) noexcept {
            return CodeGen.codegen(Data, std::move(Result), SOPath);
          })
          .and_then([&]() noexcept { return Expect<std::string>{SOPath}; });
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          Ctx->VM.registerModule(AliasName, *ModInst);
        }
      }
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onModule = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
        [&VM, &ModName](const std::string &SOFileName) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFileName);
          } else {
            return VM.loadWasm(SOFileName)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
        [&VM](const std::string &SOFileName) -> Expect<SpecTest::WasmUnit> {
          Loader::Loader &Loader = VM.getLoader();
          Validator::Validator &Validator = VM.getValidator();
          EXPECTED_TRY(auto ASTMod, Loader.parseModule(SOFileName));
          EXPECTED_TRY(Validator.validate(*ASTMod.get()));
          return ASTMod;
        });
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in the VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

TEST_P(JITCoreTest, TestSuites) {
  auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::JIT);
  Conf.getCompilerConfigure().setOptimizationLevel(
      WasmEdge::CompilerConfigure::OptimizationLevel::O0);
  Conf.getCompilerConfigure().setDumpIR(true);
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    TestContext(const WasmEdge::Configure &C) : VM(C) {
      VM.registerModule(SpecTestMod);
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          Ctx->VM.registerModule(AliasName, *ModInst);
        }
      }
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onModule = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      return VM.registerModule(ModName, FileName);
    } else {
      return VM.loadWasm(FileName)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName).and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    Loader::Loader &Loader = VM.getLoader();
    Validator::Validator &Validator = VM.getValidator();
    EXPECTED_TRY(auto ASTMod, Loader.parseModule(FileName));
    EXPECTED_TRY(Validator.validate(*ASTMod.get()));
    return ASTMod;
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in the VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

TEST_P(LazyJITCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    TestContext(const WasmEdge::Configure &C) : VM(C) {
      VM.registerModule(SpecTestMod);
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    WasmEdge::Configure CopyConf = ConfRef;
    CopyConf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
    CopyConf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O0);
    auto *Ctx = new TestContext(CopyConf);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          Ctx->VM.registerModule(AliasName, *ModInst);
        }
      }
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onModule = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      return VM.registerModule(ModName, FileName);
    } else {
      return VM.loadWasm(FileName)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName).and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    Loader::Loader &Loader = VM.getLoader();
    Validator::Validator &Validator = VM.getValidator();
    EXPECTED_TRY(auto ASTMod, Loader.parseModule(FileName));
    EXPECTED_TRY(Validator.validate(*ASTMod.get()));
    return ASTMod;
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(
    TestUnit, NativeCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CustomWasmCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, JITCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::JIT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, LazyJITCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::JIT)));

std::array<WasmEdge::Byte, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

TEST(AsyncRunWsmFile, NativeInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncExecute, NativeInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

// The OS maps an AOT shared library once per process, so every thread that
// loads the same file patches the one intrinsics slot inside it. All of them
// publish the same process-wide table, which leaves the loads succeeding and
// the value correct either way -- only a race detector can see the defect, so
// this fails under ThreadSanitizer if loadExecutable patches the slot with a
// plain store.
TEST(AOTSharedLibrary, ConcurrentLoadPatchesIntrinsics) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path =
      std::filesystem::temp_directory_path() /
      std::filesystem::u8path("AOTIntrinsicsPatch" WASMEDGE_LIB_EXTENSION);
  auto Module = Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(ValidatorEngine.validate(**Module));
  auto Data = Compiler.compile(**Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));

  constexpr size_t ThreadCount = 4;
  std::array<bool, ThreadCount> Loaded{};
  std::atomic<size_t> Ready{0};
  std::atomic<bool> Go{false};
  std::vector<std::thread> Threads;
  Threads.reserve(ThreadCount);
  for (size_t I = 0; I < ThreadCount; ++I) {
    Threads.emplace_back([&, I]() {
      // Everything that can be done before the load is done before the
      // barrier, so the threads reach loadExecutable together.
      WasmEdge::VM::VM VM(Conf);
      Ready.fetch_add(1, std::memory_order_release);
      while (!Go.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      Loaded[I] = static_cast<bool>(VM.loadWasm(Path));
      VM.cleanup();
    });
  }
  while (Ready.load(std::memory_order_acquire) != ThreadCount) {
    std::this_thread::yield();
  }
  Go.store(true, std::memory_order_release);
  for (auto &Thread : Threads) {
    Thread.join();
  }

  for (size_t I = 0; I < ThreadCount; ++I) {
    EXPECT_TRUE(Loaded[I]) << "thread " << I << " failed to load the library";
  }
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncRunWsmFile, CustomWasmInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Wasm);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest.aot.wasm");
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncExecute, CustomWasmInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Wasm);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest.aot.wasm");
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(Configure, ConfigureTest) {
  {
    WasmEdge::Configure Conf;
    WasmEdge::LLVM::Compiler Compiler(Conf);
    auto Result = Compiler.checkConfigure();
    EXPECT_TRUE(Result);
  }
  {
    WasmEdge::Configure Conf;
    Conf.addProposal(Proposal::Annotations);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    auto Result = Compiler.checkConfigure();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::InvalidAOTConfigure);
  }
}

// Test for f32x4.max NaN handling (Issue #4257)
// This test verifies that f32x4.max correctly returns the RHS NaN when both
// operands are NaN, as per the WebAssembly SIMD spec.
//
// WAT source for SIMDNaNTestWasm:
// (module
//   (func (export "test_f32x4_max_nan") (result v128)
//     ;; LHS: v128.const with NaN values (0x7fc00001 in each lane)
//     v128.const i32x4 0x7fc00001 0x7fc00001 0x7fc00001 0x7fc00001
//     ;; RHS: v128.const with NaN values (0x7fc00000 in each lane)
//     v128.const i32x4 0x7fc00000 0x7fc00000 0x7fc00000 0x7fc00000
//     ;; f32x4.max should return RHS NaN (0x7fc00000) per spec
//     f32x4.max
//   )
// )
TEST(SIMDNaN, F32x4MaxNaNHandling) {
  std::array<WasmEdge::Byte, 88> SIMDNaNTestWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
      0x60, 0x00, 0x01, 0x7b, 0x03, 0x02, 0x01, 0x00, 0x07, 0x16, 0x01,
      0x12, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x66, 0x33, 0x32, 0x78, 0x34,
      0x5f, 0x6d, 0x61, 0x78, 0x5f, 0x6e, 0x61, 0x6e, 0x00, 0x00, 0x0a,
      0x2b, 0x01, 0x29, 0x00, 0xfd, 0x0c, 0x01, 0x00, 0xc0, 0x7f, 0x01,
      0x00, 0xc0, 0x7f, 0x01, 0x00, 0xc0, 0x7f, 0x01, 0x00, 0xc0, 0x7f,
      0xfd, 0x0c, 0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00, 0xc0, 0x7f, 0x00,
      0x00, 0xc0, 0x7f, 0x00, 0x00, 0xc0, 0x7f, 0xfd, 0xe9, 0x01, 0x0b};

  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);

  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("SIMDNaNTest" WASMEDGE_LIB_EXTENSION);

  auto Module = *Loader.parseModule(SIMDNaNTestWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(SIMDNaNTestWasm, std::move(*Data), Path));

  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto Result = VM.execute("test_f32x4_max_nan");
  ASSERT_TRUE(Result);
  ASSERT_EQ((*Result).size(), 1U);

  auto ResultVal = (*Result)[0].first.get<WasmEdge::uint128_t>();

  uint32_t Lanes[4];
  std::copy_n(reinterpret_cast<const uint32_t *>(&ResultVal), 4, Lanes);

  // Per SIMD spec, f32x4.max with two NaN inputs should return RHS NaN
  const uint32_t ExpectedNaN = 0x7fc00000;
  EXPECT_EQ(Lanes[0], ExpectedNaN)
      << "Lane 0: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[0];
  EXPECT_EQ(Lanes[1], ExpectedNaN)
      << "Lane 1: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[1];
  EXPECT_EQ(Lanes[2], ExpectedNaN)
      << "Lane 2: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[2];
  EXPECT_EQ(Lanes[3], ExpectedNaN)
      << "Lane 3: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[3];

  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AOTMemory64, BoundsCheck) {
  // A full 64-bit index escapes the guard page that memory32 relies on, so
  // check that a far out-of-bounds memory64 access traps rather than reaching
  // host memory.
  //
  // (module
  //   (memory $m1 (export "mem1") i64 1)
  //   (func (export "peek") (param i64) (result i64)
  //     (i64.load $m1 (local.get 0)))
  //   (func (export "poke") (param i64 i64)
  //     (i64.store $m1 (local.get 0) (local.get 1))))
  std::array<WasmEdge::Byte, 90> Memory64Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x02, 0x60,
      0x01, 0x7e, 0x01, 0x7e, 0x60, 0x02, 0x7e, 0x7e, 0x00, 0x03, 0x03, 0x02,
      0x00, 0x01, 0x05, 0x03, 0x01, 0x04, 0x01, 0x07, 0x16, 0x03, 0x04, 0x6d,
      0x65, 0x6d, 0x31, 0x02, 0x00, 0x04, 0x70, 0x65, 0x65, 0x6b, 0x00, 0x00,
      0x04, 0x70, 0x6f, 0x6b, 0x65, 0x00, 0x01, 0x0a, 0x13, 0x02, 0x07, 0x00,
      0x20, 0x00, 0x29, 0x03, 0x00, 0x0b, 0x09, 0x00, 0x20, 0x00, 0x20, 0x01,
      0x37, 0x03, 0x00, 0x0b, 0x00, 0x0c, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x06,
      0x05, 0x01, 0x00, 0x02, 0x6d, 0x31};

  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);

  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTMemory64Test" WASMEDGE_LIB_EXTENSION);

  auto Module = *Loader.parseModule(Memory64Wasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(Memory64Wasm, std::move(*Data), Path));

  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const std::vector<WasmEdge::ValType> I64{
      WasmEdge::ValType(WasmEdge::TypeCode::I64)};
  const std::vector<WasmEdge::ValType> I64x2{
      WasmEdge::ValType(WasmEdge::TypeCode::I64),
      WasmEdge::ValType(WasmEdge::TypeCode::I64)};

  // In-bounds store then load round-trips.
  ASSERT_TRUE(VM.execute("poke",
                         {WasmEdge::ValVariant(UINT64_C(8)),
                          WasmEdge::ValVariant(UINT64_C(0x1234))},
                         I64x2));
  auto InBounds = VM.execute("peek", {WasmEdge::ValVariant(UINT64_C(8))}, I64);
  ASSERT_TRUE(InBounds);
  EXPECT_EQ((*InBounds)[0].first.get<uint64_t>(), UINT64_C(0x1234));

  // Aim the index at an in-process heap buffer: a fixed huge constant would
  // only fault into unmapped space, which proves nothing.
  const auto *ModInst = VM.getActiveModule();
  ASSERT_NE(ModInst, nullptr);
  auto *MemInst = ModInst->findMemoryExports("mem1");
  ASSERT_NE(MemInst, nullptr);
  const uint8_t *Base = MemInst->getDataPtr();
  auto Secret = std::make_unique<uint64_t>(UINT64_C(0xDEADBEEFCAFEBABE));
  const uint64_t Off =
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Secret.get()) -
                            reinterpret_cast<uintptr_t>(Base));
  auto OobLoad = VM.execute("peek", {WasmEdge::ValVariant(Off)}, I64);
  ASSERT_FALSE(OobLoad);
  EXPECT_EQ(OobLoad.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  EXPECT_EQ(*Secret, UINT64_C(0xDEADBEEFCAFEBABE));

  auto OobStore = VM.execute(
      "poke", {WasmEdge::ValVariant(Off), WasmEdge::ValVariant(UINT64_C(0))},
      I64x2);
  ASSERT_FALSE(OobStore);
  EXPECT_EQ(OobStore.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  EXPECT_EQ(*Secret, UINT64_C(0xDEADBEEFCAFEBABE));

  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

// Owns a uniquely named file under the system temporary directory and removes
// it when the scope exits, including when an assertion returns early.
class ScopedTempFile {
public:
  explicit ScopedTempFile(std::string_view Stem,
                          std::string_view Extension = ""sv)
      : Path(
            std::filesystem::temp_directory_path() /
            std::filesystem::u8path(std::string(Stem) + "-" +
                                    std::to_string(std::hash<std::thread::id>{}(
                                        std::this_thread::get_id())) +
                                    std::string(Extension))) {}
  ScopedTempFile(const ScopedTempFile &) = delete;
  ScopedTempFile &operator=(const ScopedTempFile &) = delete;
  ~ScopedTempFile() noexcept {
    std::error_code EC;
    std::filesystem::remove(Path, EC);
  }

  const std::filesystem::path &get() const noexcept { return Path; }

private:
  std::filesystem::path Path;
};

TEST(NativeRunMode, LoadSharedLibraryRequiresAOTMode) {
  std::array<WasmEdge::Byte, 34> NativeRunModeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x2a, 0x0b};

  const ScopedTempFile Artifact("NativeRunModeTest", WASMEDGE_LIB_EXTENSION);
  const auto &Path = Artifact.get();

  {
    WasmEdge::Configure Conf;
    Conf.getCompilerConfigure().setOutputFormat(
        CompilerConfigure::OutputFormat::Native);
    Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);

    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);

    auto Module = Loader.parseModule(NativeRunModeWasm);
    ASSERT_TRUE(Module);
    ASSERT_TRUE(ValidatorEngine.validate(**Module));
    auto Data = Compiler.compile(**Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(NativeRunModeWasm, std::move(*Data), Path));
  }

  for (const auto Mode : {WasmEdge::RunMode::Interpreter,
                          WasmEdge::RunMode::JIT, WasmEdge::RunMode::LazyJIT}) {
    WasmEdge::Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(Mode);
    WasmEdge::VM::VM VM(Conf);
    auto Res = VM.loadWasm(Path);
    EXPECT_FALSE(Res);
    if (!Res) {
      EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::MalformedMagic);
    }
    VM.cleanup();
  }

  {
    WasmEdge::Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
    WasmEdge::VM::VM VM(Conf);
    EXPECT_TRUE(VM.loadWasm(Path));
    VM.cleanup();
  }
}

class Collect : public WasmEdge::Runtime::HostFunction<Collect> {
public:
  Expect<void> body(const WasmEdge::Runtime::CallingFrame &CF) {
    auto &Allocator = CF.getExecutor()->getAllocator();
    Allocator.manualCollect(true);
    return {};
  }
};

class Record : public WasmEdge::Runtime::HostFunction<Record> {
public:
  Expect<void> body(const WasmEdge::Runtime::CallingFrame &CF) {
    MemoryUsageLog.push_back(CF.getExecutor()->getAllocator().getMemoryUsage());
    return {};
  }
  Span<const uint64_t> getLog() const noexcept { return MemoryUsageLog; }

private:
  std::vector<uint64_t> MemoryUsageLog;
};

class GCModule : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  GCModule() : ModuleInstance("gc") {
    addHostFunc("coll", std::make_unique<Collect>());
    auto RP = std::make_unique<Record>();
    R = RP.get();
    addHostFunc("rec", std::move(RP));
  }
  Span<const uint64_t> getLog() const noexcept { return R->getLog(); }

private:
  Record *R = nullptr;
};

TEST(AOTGC, MemoryUsage) {
  std::array<WasmEdge::Byte, 117> Wasm{
      0x00, 0x61, 0x73, 0x6d,       // wasm magic
      0x01, 0x00, 0x00, 0x00,       // module version
      0x01,                         // type section
      0x07,                         // section size
      0x02,                         // type count
      0x5e,                         // array type
      0x7f, 0x01,                   // i32 mutable
      0x60,                         // function type
      0x00, 0x00,                   // 0 arguments 0 results
      0x02,                         // import section
      0x14,                         // section size
      0x02,                         // import count
      0x02, 0x67, 0x63,             // "gc"
      0x04, 0x63, 0x6f, 0x6c, 0x6c, // "coll"
      0x00,                         // import kind: function
      0x01,                         // import type index: 1
      0x02, 0x67, 0x63,             // "gc"
      0x03, 0x72, 0x65, 0x63,       // "rec"
      0x00,                         // import kind: function
      0x01,                         // import type index: 1
      0x03,                         // function section
      0x02,                         // section size
      0x01,                         // function count
      0x01,                         // function type index: 1
      0x07,                         // export section
      0x06,                         // section size
      0x01,                         // export count
      0x02, 0x67, 0x63,             // export name "gc"
      0x00,                         // export kind: function
      0x02,                         // export index
      0x0a,                         // code section
      0x40,                         // section size
      0x01,                         // function count
      0x3e,                         // function size
      0x00,                         // local size
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x41, 0x04,                   // i32.const 4
      0xfb, 0x07, 0x00,             // array.new_default 0
      0x10, 0x01,                   // call 1 (gc.rec) Bytes4
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) Bytes4
      0x1a,                         // drop
      0x10, 0x01,                   // call 1 (gc.rec) Bytes4
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) <=Bytes4 (ranged: AOT)
      0x41, 0x08,                   // i32.const 8
      0xfb, 0x07, 0x00,             // array.new_default 0
      0x1a,                         // drop
      0x10, 0x01,                   // call 1 (gc.rec) >=Bytes8 (ranged: AOT)
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) >=Bytes8 (ranged: AOT)
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) <=Bytes4+8 (ranged: AOT)
      0x41, 0x0c,                   // i32.const 12
      0xfb, 0x07, 0x00,             // array.new_default 0
      0x10, 0x01,                   // call 1 (gc.rec) >=Bytes12 (ranged: AOT)
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) >=Bytes12 (ranged: AOT)
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) >=Bytes12 (ranged: AOT)
      0x1a,                         // drop
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) <=Bytes4+8+12 (ranged)
      0x0b,                         // end
  };

  const auto Path =
      std::filesystem::temp_directory_path() /
      std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::GC);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  {
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    auto Module = *Loader.parseModule(Wasm);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Path));
  }

  WasmEdge::VM::VM VM(Conf);
  GCModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // autoCollect off: only the wasm's explicit coll calls run, else a background
  // native-stack cycle between rec snapshots widens the range asserts.
  VM.getExecutor().getAllocator().setManualGC(true);
  ASSERT_TRUE(VM.execute("gc"));
  auto Result = GCMod.getLog();

  // allocate() charges sizeof(Header) + Size, where Size = sizeof(RawData) +
  // N * sizeof(ValVariant); derive the expected byte totals for the 4/8/12
  // element arrays from that formula instead of hard-coding them, so a change
  // to the GC header layout does not silently turn this into a false failure.
  constexpr auto GCBytes = [](uint32_t N) constexpr {
    return static_cast<uint32_t>(
        sizeof(WasmEdge::GC::Allocator::Header) +
        sizeof(WasmEdge::Runtime::Instance::GCInstance::RawData) +
        static_cast<size_t>(N) * sizeof(WasmEdge::ValVariant));
  };
  const uint32_t Bytes4 = GCBytes(4);
  const uint32_t Bytes8 = GCBytes(8);
  const uint32_t Bytes12 = GCBytes(12);

  ASSERT_EQ(Result.size(), 13);
  EXPECT_EQ(Result[0], 0);
  EXPECT_EQ(Result[1], 0);
  EXPECT_EQ(Result[2], Bytes4);
  EXPECT_EQ(Result[3], Bytes4);
  EXPECT_EQ(Result[4], Bytes4);
  // Only the 4-element array allocated so far, so usage cannot exceed it (upper
  // bound, not exact 0, since conservative stack scanning may keep it alive).
  EXPECT_LE(Result[5], Bytes4);
  EXPECT_GE(Result[6], Bytes8);
  EXPECT_GE(Result[7], Bytes8);
  // At most the 4- and 8-element arrays have been allocated by this point.
  EXPECT_LE(Result[8], Bytes4 + Bytes8);
  EXPECT_GE(Result[9], Bytes12);
  // A fresh object is born gray, so the FIRST collection after it is allocated
  // can never reclaim it -- that much is deterministic.
  EXPECT_GE(Result[10], Bytes12);
  // By the second collection this is only an upper bound. The array's sole
  // remaining use in the compiled code is the drop below, and a drop is not a
  // use, so the optimizer may keep it in no register and no stack slot; the
  // conservative native scan then finds nothing and the object is reclaimed.
  // That is precise collection, not a lost root -- LiveRefSurvivesCollect
  // covers the property that actually matters. Targets whose codegen happens to
  // leave the dead pointer in a stack slot retain it, which this also allows.
  EXPECT_LE(Result[11], Bytes12);
  // Final reading: at most the cumulative bytes of every array ever allocated.
  EXPECT_LE(Result[12], Bytes4 + Bytes8 + Bytes12);

  // Unload the AOT module before removing its shared library: on Windows the
  // DLL (file lock) is held until cleanup. Matches the other AOT tests.
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

// A reference the compiled code still USES after a collection must survive it.
// This is the companion to MemoryUsage's upper bounds: those cover values the
// optimizer is entitled to drop, this covers the one property the collector may
// never break. The array is read back after two collections, so it stays live
// in the compiled code throughout and the conservative native-stack scan has to
// find it.
//
// (module
//   (type $a (array (mut i32)))
//   (import "gc" "coll" (func $coll))
//   (import "gc" "rec" (func $rec))
//   (func (export "gc") (result i32)
//     i32.const 12
//     array.new_default $a
//     call $rec  call $coll  call $rec  call $coll  call $rec
//     i32.const 0
//     array.get $a))
TEST(AOTGC, LiveRefSurvivesCollect) {
  const std::array<WasmEdge::Byte, 108> Wasm{
      0,   97, 115, 109, 1,   0,  0,   0,   1,  11,  3,  94, 127, 1,   96,  0,
      0,   96, 0,   1,   127, 2,  20,  2,   2,  103, 99, 4,  99,  111, 108, 108,
      0,   1,  2,   103, 99,  3,  114, 101, 99, 0,   1,  3,  2,   1,   2,   7,
      6,   1,  2,   103, 99,  0,  2,   10,  24, 1,   22, 0,  65,  12,  251, 7,
      0,   16, 1,   16,  0,   16, 1,   16,  0,  16,  1,  65, 0,   251, 11,  0,
      11,  0,  25,  4,   110, 97, 109, 101, 1,  12,  2,  0,  4,   99,  111, 108,
      108, 1,  3,   114, 101, 99, 4,   4,   1,  0,   1,  97};

  const auto Path =
      std::filesystem::temp_directory_path() /
      std::filesystem::u8path("AOTliveRefTest" WASMEDGE_LIB_EXTENSION);
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::GC);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  {
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    auto Module = *Loader.parseModule(Wasm);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Path));
  }

  WasmEdge::VM::VM VM(Conf);
  GCModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.getExecutor().getAllocator().setManualGC(true);
  auto Res = VM.execute("gc");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  // Read back through the surviving array: zero-initialized, not freed memory.
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), UINT32_C(0));

  constexpr auto Bytes12 = static_cast<uint32_t>(
      sizeof(WasmEdge::GC::Allocator::Header) +
      sizeof(WasmEdge::Runtime::Instance::GCInstance::RawData) +
      UINT32_C(12) * sizeof(WasmEdge::ValVariant));
  auto Result = GCMod.getLog();
  ASSERT_EQ(Result.size(), 3);
  // Usage may never fall below the live array, at any point.
  EXPECT_GE(Result[0], Bytes12);
  EXPECT_GE(Result[1], Bytes12);
  EXPECT_GE(Result[2], Bytes12);

  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

// (module
//   (table 1 externref)
//   (func (export "f") (param externref)
//     i32.const 0
//     local.get 0
//     table.set 0))
const std::vector<uint8_t> TableSetWasm = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x01, 0x6F, 0x00, 0x03, 0x02, 0x01, 0x00, 0x04, 0x04, 0x01,
    0x6F, 0x00, 0x01, 0x07, 0x05, 0x01, 0x01, 0x66, 0x00, 0x00, 0x0A,
    0x0A, 0x01, 0x08, 0x00, 0x41, 0x00, 0x20, 0x00, 0x26, 0x00, 0x0B};

// Negative control. Same shape, but table.get: a read, which the collector
// does not barrier.
// (module
//   (table 1 externref)
//   (func (export "f") (result externref)
//     i32.const 0
//     table.get 0))
const std::vector<uint8_t> TableGetWasm = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x00, 0x01, 0x6F, 0x03, 0x02, 0x01, 0x00, 0x04, 0x04, 0x01,
    0x6F, 0x00, 0x01, 0x07, 0x05, 0x01, 0x01, 0x66, 0x00, 0x00, 0x0A,
    0x08, 0x01, 0x06, 0x00, 0x41, 0x00, 0x25, 0x00, 0x0B};

// Strip away any bitcasts, which older LLVM inserts and opaque pointers elide.
LLVMValueRef stripBitCasts(LLVMValueRef V) noexcept {
  while (LLVMIsAInstruction(V) != nullptr &&
         LLVMGetInstructionOpcode(V) == LLVMBitCast) {
    V = LLVMGetOperand(V, 0);
  }
  return V;
}

// Count the calls made through the intrinsics table at slot Which.
//
// CompileContext::getIntrinsic lowers an intrinsic call to a GEP into the
// intrinsics table, a load of that slot, and an indirect call of the loaded
// pointer. Walking that shape is exact: it cannot be fooled by an unrelated
// constant, and it survives any reformatting of the textual IR.
uint32_t countIntrinsicCalls(LLVMModuleRef M,
                             WasmEdge::Executable::Intrinsics Which) noexcept {
  const auto Want = static_cast<unsigned long long>(Which);
  // Take the pointer size from the module rather than the host, since the two
  // need not agree.
  const auto PointerSize = LLVMPointerSize(LLVMGetModuleDataLayout(M));
  uint32_t Count = 0;
  for (auto F = LLVMGetFirstFunction(M); F != nullptr;
       F = LLVMGetNextFunction(F)) {
    for (auto BB = LLVMGetFirstBasicBlock(F); BB != nullptr;
         BB = LLVMGetNextBasicBlock(BB)) {
      for (auto I = LLVMGetFirstInstruction(BB); I != nullptr;
           I = LLVMGetNextInstruction(I)) {
        if (LLVMGetInstructionOpcode(I) != LLVMCall) {
          continue;
        }
        // The callee must be a load of an intrinsics-table slot.
        auto Callee = stripBitCasts(LLVMGetCalledValue(I));
        if (LLVMIsAInstruction(Callee) == nullptr ||
            LLVMGetInstructionOpcode(Callee) != LLVMLoad) {
          continue;
        }
        auto Slot = stripBitCasts(LLVMGetOperand(Callee, 0));
        if (LLVMIsAInstruction(Slot) == nullptr ||
            LLVMGetInstructionOpcode(Slot) != LLVMGetElementPtr) {
          continue;
        }
        // The slot index is the GEP's last operand, and must be constant.
        const auto NumOperands =
            static_cast<unsigned>(LLVMGetNumOperands(Slot));
        auto Index = LLVMGetOperand(Slot, NumOperands - 1U);
        if (LLVMIsAConstantInt(Index) == nullptr) {
          continue;
        }
        // The table is indexed either by the array GEP as emitted, whose
        // operands are (table, 0, Index), or by the equivalent i8 byte offset
        // Index * pointer size that the optimizer canonicalizes it into.
        const auto Value = LLVMConstIntGetZExtValue(Index);
        if (NumOperands == 3) {
          Count += (Value == Want) ? 1U : 0U;
        } else if (NumOperands == 2) {
          Count += (Value == Want * PointerSize) ? 1U : 0U;
        }
      }
    }
  }
  return Count;
}

// Compile Wasm to an LLVM module and count its kWriteBarrier calls. Data owns
// the module, so it must outlive the walk. The result is returned via `Count`
// (an out-parameter, not the return value) so that ASSERT_TRUE can be used on
// a parse/compile failure instead of dereferencing an empty Expected: gtest's
// ASSERT_* macros expand to a bare `return;` and so require a void-returning
// function.
void countBarrierCalls(const std::vector<uint8_t> &Wasm,
                       WasmEdge::Executable::Intrinsics Intrinsic,
                       uint32_t &Count) {
  WasmEdge::Configure Conf;
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);

  auto Module = Loader.parseModule(Wasm);
  ASSERT_TRUE(Module);
  EXPECT_TRUE(ValidatorEngine.validate(**Module));
  auto Data = Compiler.compile(**Module);
  ASSERT_TRUE(Data);

  Count = countIntrinsicCalls(Data->extract().LLModule.unwrap(), Intrinsic);
}

TEST(AOTWriteBarrier, TableSetEmitsBarrier) {
  // Upstream lowers table.set to a bare store, which never reaches the barrier
  // in TableInstance::setRefs. This used to be covered by emitting two inlined
  // kWriteBarrier calls (shade the overwritten reference, then the new one).
  // The coherent-slot work replaced that with a single kCoherentRefStore, which
  // performs BOTH shades and then publishes the 128-bit (type, pointer) pair
  // atomically, so a concurrent marker or coherent reader cannot observe a torn
  // slot -- see Executor::proxyCoherentRefStore. Barrier coverage is therefore
  // unchanged; only the lowering moved into the helper.
  uint32_t Count = 0;
  ASSERT_NO_FATAL_FAILURE(countBarrierCalls(
      TableSetWasm, WasmEdge::Executable::Intrinsics::kCoherentRefStore,
      Count));
  EXPECT_EQ(Count, 1U);
}

TEST(AOTWriteBarrier, TableGetEmitsNoBarrier) {
  // table.get is a read, so it must lower to the load helper and never to the
  // storing one. Asserting both directions keeps the positive test above from
  // passing vacuously: a count of zero here has to mean "reads take the load
  // path", not "this intrinsic is never emitted anywhere".
  uint32_t StoreCount = 0;
  ASSERT_NO_FATAL_FAILURE(countBarrierCalls(
      TableGetWasm, WasmEdge::Executable::Intrinsics::kCoherentRefStore,
      StoreCount));
  EXPECT_EQ(StoreCount, 0U);

  uint32_t LoadCount = 0;
  ASSERT_NO_FATAL_FAILURE(countBarrierCalls(
      TableGetWasm, WasmEdge::Executable::Intrinsics::kCoherentRefLoad,
      LoadCount));
  EXPECT_EQ(LoadCount, 1U);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
