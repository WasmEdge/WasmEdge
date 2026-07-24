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
#include "common/spdlog.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
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

// A compiled cross-module call must run the callee with its own state and give
// the caller its state back on return. Each caller function returns
// `callee_global * 10 + memory.grow(0)`: 1871 if both held, 1875 if the call
// rebound the context and kept it. The second term goes through memory.grow
// because that intrinsic resolves the memory live, while inline accesses read
// the context each compiled function snapshots on entry and so cannot see a
// stale rebind.
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

// Parse, validate, compile, and attach the compiled symbols. The Loader needs
// the executor's intrinsics table so loadExecutable patches the compiled code's
// intrinsics global; without it the generated intrinsic calls jump through a
// null table.
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

// Both modules are compiled up front, then registered and instantiated into one
// store owned by one executor, so the result cannot be an artifact of sharing
// modules across VMs.
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

// A cross-module return_call_ref has to run each side with its own state. It is
// mediated by the kCallRef proxy, which nests a proxy and an enterFunction
// frame per hop at roughly a kilobyte of native stack each, so the depth stays
// low enough to fit a 1 MiB thread stack many times over. This covers the
// contexts only; constant-space cross-module tail calls are not implemented.
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

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
