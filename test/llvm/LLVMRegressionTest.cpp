// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/llvm/LLVMRegressionTest.cpp - Regression tests ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains regression tests of the AOT and JIT compilation.
///
//===----------------------------------------------------------------------===//

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
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

// (module
//   (type $t (;0;) (func (result i32)))
//   (memory (;0;) 5)
//   (global $g (;0;) (mut i32) i32.const 187)
//   (export "read" (func 0))
//   (func (;0;) (type $t) (result i32)
//     global.get $g
//   )
// )
const std::array<WasmEdge::Byte, 70> CrossModuleCalleeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x05,
    0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xbb, 0x01, 0x0b, 0x07, 0x08, 0x01,
    0x04, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00,
    0x23, 0x00, 0x0b, 0x00, 0x11, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x04, 0x04,
    0x01, 0x00, 0x01, 0x74, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

// run_* compute read() * 10 + memory.grow(0): 1871 with separate contexts, 1875
// when the callee's 5-page memory leaks into the caller.
// (module
//   (type $t (;0;) (func (result i32)))
//   (import "callee" "read" (func $read (;0;) (type $t)))
//   (table (;0;) 1 funcref)
//   (memory (;0;) 1)
//   (global $g (;0;) (mut i32) i32.const 170)
//   (export "run_ref" (func 1))
//   (export "run_direct" (func 2))
//   (export "run_indirect" (func 3))
//   (elem (;0;) (i32.const 0) func $read)
//   (func (;1;) (type $t) (result i32)
//     ref.func $read
//     call_ref $t
//     i32.const 10
//     i32.mul
//     i32.const 0
//     memory.grow
//     i32.add
//   )
//   (func (;2;) (type $t) (result i32)
//     call $read
//     i32.const 10
//     i32.mul
//     i32.const 0
//     memory.grow
//     i32.add
//   )
//   (func (;3;) (type $t) (result i32)
//     i32.const 0
//     call_indirect (type $t)
//     i32.const 10
//     i32.mul
//     i32.const 0
//     memory.grow
//     i32.add
//   )
// )
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

// Parses, validates, and JIT-compiles a module; the Loader needs the executor's
// intrinsics table, or intrinsic calls jump through a null table.
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

// Parses and validates a module without compiling it, so it runs interpreted.
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

// A compiled cross-module call must run the callee with its own context and
// restore the caller's; both modules share one store and executor.
TEST(LLVMRegressionTest, CrossModuleCompiledCallUsesCalleeContext) {
  Configure Conf;

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

// A cross-module return_call_ref must run each side with its own globals.
TEST(LLVMRegressionTest, CrossModuleCompiledTailCallUsesCalleeContext) {
  // (module
  //   (type $t (;0;) (func (param i32) (result i32)))
  //   (global $g (;0;) (mut i32) i32.const 187)
  //   (global $peer (;1;) (mut (ref null $t)) ref.null $t)
  //   (export "peer" (global $peer))
  //   (export "f" (func $f))
  //   (func $f (;0;) (type $t) (param i32) (result i32)
  //     global.get $g
  //     i32.const 187
  //     i32.ne
  //     if ;; label = @1
  //       i32.const 1
  //       return
  //     end
  //     local.get 0
  //     i32.eqz
  //     if ;; label = @1
  //       i32.const 0
  //       return
  //     end
  //     local.get 0
  //     i32.const 1
  //     i32.sub
  //     global.get $peer
  //     return_call_ref $t
  //   )
  // )
  std::array<WasmEdge::Byte, 116> CalleeWasm{
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
  // (module
  //   (type $t (;0;) (func (param i32) (result i32)))
  //   (import "callee" "f" (func $f (;0;) (type $t)))
  //   (import "callee" "peer" (global $peer (;0;) (mut (ref null $t))))
  //   (global $g (;1;) (mut i32) i32.const 170)
  //   (export "run" (func 2))
  //   (elem (;0;) declare func $a $f)
  //   (func $a (;1;) (type $t) (param i32) (result i32)
  //     global.get $g
  //     i32.const 170
  //     i32.ne
  //     if ;; label = @1
  //       i32.const 2
  //       return
  //     end
  //     local.get 0
  //     i32.eqz
  //     if ;; label = @1
  //       i32.const 0
  //       return
  //     end
  //     local.get 0
  //     i32.const 1
  //     i32.sub
  //     ref.func $f
  //     return_call_ref $t
  //   )
  //   (func (;2;) (type $t) (param i32) (result i32)
  //     ref.func $a
  //     global.set $peer
  //     local.get 0
  //     ref.func $f
  //     return_call_ref $t
  //   )
  // )
  std::array<WasmEdge::Byte, 160> CallerWasm{
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

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

// Lazy JIT resolves calls through its own per-function symbol cache.
TEST(LLVMRegressionTest, CrossModuleLazyJITCallUsesCalleeContext) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
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

// A same-module call followed by a cross-module tail call must return to a
// caller that still resolves its own memory.
TEST(LLVMRegressionTest, CrossModuleTailCallLeavesCallerContextIntact) {
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (memory (;0;) 5)
  //   (global $g (;0;) (mut i32) i32.const 187)
  //   (export "c" (func 0))
  //   (func (;0;) (type $t) (result i32)
  //     global.get $g
  //   )
  // )
  std::array<WasmEdge::Byte, 67> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x05,
      0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xbb, 0x01, 0x0b, 0x07, 0x05, 0x01,
      0x01, 0x63, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x23, 0x00, 0x0b,
      0x00, 0x11, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01,
      0x74, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (import "callee" "c" (func $c (;0;) (type $t)))
  //   (memory (;0;) 1)
  //   (global $g (;0;) (mut i32) i32.const 170)
  //   (export "run" (func 2))
  //   (elem (;0;) declare func $c)
  //   (func $b (;1;) (type $t) (result i32)
  //     ref.func $c
  //     return_call_ref $t
  //   )
  //   (func (;2;) (type $t) (result i32)
  //     call $b
  //     i32.const 10
  //     i32.mul
  //     i32.const 0
  //     memory.grow
  //     i32.add
  //   )
  // )
  std::array<WasmEdge::Byte, 115> CallerWasm{
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

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

// A cross-module return_call_indirect must run the callee with its own module:
// 187*10 + 5 = 1875, a leaked caller context gives 1701.
TEST(LLVMRegressionTest,
     CrossModuleCompiledReturnCallIndirectUsesCalleeContext) {
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (memory (;0;) 5)
  //   (global $g (;0;) (mut i32) i32.const 187)
  //   (export "probe" (func $probe))
  //   (func $probe (;0;) (type $t) (result i32)
  //     global.get $g
  //     i32.const 10
  //     i32.mul
  //     i32.const 0
  //     memory.grow
  //     i32.add
  //   )
  // )
  std::array<WasmEdge::Byte, 89> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x05,
      0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xbb, 0x01, 0x0b, 0x07, 0x09, 0x01,
      0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x0a, 0x0e, 0x01, 0x0c,
      0x00, 0x23, 0x00, 0x41, 0x0a, 0x6c, 0x41, 0x00, 0x40, 0x00, 0x6a, 0x0b,
      0x00, 0x1b, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05,
      0x70, 0x72, 0x6f, 0x62, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74, 0x07,
      0x04, 0x01, 0x00, 0x01, 0x67};
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (import "callee" "probe" (func $probe (;0;) (type $t)))
  //   (table (;0;) 1 funcref)
  //   (memory (;0;) 1)
  //   (global $g (;0;) (mut i32) i32.const 170)
  //   (export "run" (func $run))
  //   (elem (;0;) (i32.const 0) func $probe)
  //   (func $run (;1;) (type $t) (result i32)
  //     i32.const 0
  //     return_call_indirect (type $t)
  //   )
  // )
  std::array<WasmEdge::Byte, 120> CallerWasm{
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

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

// The callee's instructions must charge the same gas budget as the caller's, so
// a limit one below the measured total must fail.
TEST(LLVMRegressionTest, CrossModuleGasMeteringSpansCall) {
  Configure Conf;
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

// Two instances of one compiled module, called through call_ref, must each
// read their own global.
TEST(LLVMRegressionTest,
     CrossModuleSameCompiledModuleTwoInstancesKeepOwnState) {
  // (module
  //   (type (;0;) (func (result i32)))
  //   (global $g (;0;) (mut i32) i32.const 0)
  //   (export "g" (global $g))
  //   (export "read" (func $read))
  //   (func $read (;0;) (type 0) (result i32)
  //     global.get $g
  //   )
  // )
  std::array<WasmEdge::Byte, 71> SharedWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x06, 0x06, 0x01, 0x7f, 0x01,
      0x41, 0x00, 0x0b, 0x07, 0x0c, 0x02, 0x01, 0x67, 0x03, 0x00, 0x04, 0x72,
      0x65, 0x61, 0x64, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x23, 0x00,
      0x0b, 0x00, 0x14, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07, 0x01, 0x00,
      0x04, 0x72, 0x65, 0x61, 0x64, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (import "inst_a" "read" (func $ra (;0;) (type $t)))
  //   (import "inst_b" "read" (func $rb (;1;) (type $t)))
  //   (export "run" (func $run))
  //   (elem (;0;) declare func $ra $rb)
  //   (func $run (;2;) (type $t) (result i32)
  //     ref.func $ra
  //     call_ref $t
  //     i32.const 1000
  //     i32.mul
  //     ref.func $rb
  //     call_ref $t
  //     i32.add
  //   )
  // )
  std::array<WasmEdge::Byte, 115> CallerWasm{
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

  Configure Conf;

  auto SharedMod = compileToJIT(Conf, SharedWasm);
  ASSERT_NE(SharedMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

// Host function returning the first i32 of the calling frame's memory 0.
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

// A host function called by a compiled cross-module callee must see the
// callee's memory in its CallingFrame.
TEST(LLVMRegressionTest, CrossModuleHostFunctionSeesCalleeMemory) {
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (import "host" "read0" (func $read0 (;0;) (type $t)))
  //   (memory (;0;) 1)
  //   (export "probe" (func $probe))
  //   (func $probe (;1;) (type $t) (result i32)
  //     call $read0
  //   )
  //   (data (;0;) (i32.const 0) "\22\22\22\00")
  // )
  std::array<WasmEdge::Byte, 101> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x02, 0x0e, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x05,
      0x72, 0x65, 0x61, 0x64, 0x30, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05,
      0x03, 0x01, 0x00, 0x01, 0x07, 0x09, 0x01, 0x05, 0x70, 0x72, 0x6f, 0x62,
      0x65, 0x00, 0x01, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x0b,
      0x0a, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x04, 0x22, 0x22, 0x22, 0x00, 0x00,
      0x1c, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0f, 0x02, 0x00, 0x05, 0x72,
      0x65, 0x61, 0x64, 0x30, 0x01, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x04,
      0x04, 0x01, 0x00, 0x01, 0x74};
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (import "callee" "probe" (func $probe (;0;) (type $t)))
  //   (memory (;0;) 1)
  //   (export "run" (func $run))
  //   (elem (;0;) declare func $probe)
  //   (func $run (;1;) (type $t) (result i32)
  //     ref.func $probe
  //     call_ref $t
  //   )
  //   (data (;0;) (i32.const 0) "\11\11\11\00")
  // )
  std::array<WasmEdge::Byte, 108> CallerWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
      0x65, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x03, 0x02, 0x01,
      0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75,
      0x6e, 0x00, 0x01, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x08,
      0x01, 0x06, 0x00, 0xd2, 0x00, 0x14, 0x00, 0x0b, 0x0b, 0x0a, 0x01, 0x00,
      0x41, 0x00, 0x0b, 0x04, 0x11, 0x11, 0x11, 0x00, 0x00, 0x1a, 0x04, 0x6e,
      0x61, 0x6d, 0x65, 0x01, 0x0d, 0x02, 0x00, 0x05, 0x70, 0x72, 0x6f, 0x62,
      0x65, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74};

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

// A cross-module struct.new_default must use the callee's type section, where
// type 1 differs from the caller's, and be owned by the callee.
TEST(LLVMRegressionTest, CrossModuleStructNewOwnedByCallee) {
  // (module
  //   (type $T (;0;) (struct (field i32)))
  //   (type $S (;1;)
  //     (struct (field $tag (mut i32)) (field $next (ref null $T))))
  //   (type $MkTy (;2;) (func (result (ref $S))))
  //   (export "make" (func $make))
  //   (func $make (;0;) (type $MkTy) (result (ref $S))
  //     struct.new_default $S
  //   )
  // )
  std::array<WasmEdge::Byte, 97> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x11, 0x03,
      0x5f, 0x01, 0x7f, 0x00, 0x5f, 0x02, 0x7f, 0x01, 0x63, 0x00, 0x00,
      0x60, 0x00, 0x01, 0x64, 0x01, 0x03, 0x02, 0x01, 0x02, 0x07, 0x08,
      0x01, 0x04, 0x6d, 0x61, 0x6b, 0x65, 0x00, 0x00, 0x0a, 0x07, 0x01,
      0x05, 0x00, 0xfb, 0x01, 0x01, 0x0b, 0x00, 0x2d, 0x04, 0x6e, 0x61,
      0x6d, 0x65, 0x01, 0x07, 0x01, 0x00, 0x04, 0x6d, 0x61, 0x6b, 0x65,
      0x04, 0x0d, 0x03, 0x00, 0x01, 0x54, 0x01, 0x01, 0x53, 0x02, 0x04,
      0x4d, 0x6b, 0x54, 0x79, 0x0a, 0x0e, 0x01, 0x01, 0x02, 0x00, 0x03,
      0x74, 0x61, 0x67, 0x01, 0x04, 0x6e, 0x65, 0x78, 0x74};
  // (module
  //   (type $AFiller (;0;) (func))
  //   (type $Collide (;1;) (struct (field (mut i32)) (field (mut i32))))
  //   (type $T_caller (;2;) (struct (field i32)))
  //   (type $S_caller (;3;)
  //     (struct (field (mut i32)) (field (ref null $T_caller))))
  //   (type $MkTy (;4;) (func (result (ref $S_caller))))
  //   (import "callee" "make" (func $make (;0;) (type $MkTy)))
  //   (table (;0;) 1 funcref)
  //   (export "run" (func $run))
  //   (elem (;0;) (i32.const 0) func $make)
  //   (func $run (;1;) (type $MkTy) (result (ref $S_caller))
  //     i32.const 0
  //     call_indirect (type $MkTy)
  //   )
  // )
  std::array<WasmEdge::Byte, 160> CallerWasm{
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

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

  const auto *Inst = (*R)[0]
                         .first.get<RefVariant>()
                         .getPtr<Runtime::Instance::StructInstance>();
  ASSERT_NE(Inst, nullptr);
  EXPECT_EQ(Inst->getModule(), CalleeInst.get())
      << "the struct created by the callee's struct.new_default is not "
         "owned by the callee module";
  EXPECT_NE(Inst->getModule(), CallerInst.get())
      << "the struct is owned by the caller instead of the callee";
  EXPECT_EQ(Inst->getField(0).get<uint32_t>(), 0u);
  EXPECT_TRUE(Inst->getField(1).get<RefVariant>().isNull())
      << "the default-initialized concrete reference field did not resolve "
         "to a null bottom type through the callee's own type section";
}

// A compiled throw must resolve its tag in the throwing module: tag 1 is $b1 in
// the callee but the caller's own $a1.
TEST(LLVMRegressionTest, CrossModuleCompiledThrowUsesCalleeTagSpace) {
  // (module
  //   (type $v (;0;) (func))
  //   (tag $b0 (;0;) (type $v))
  //   (tag $b1 (;1;) (type $v))
  //   (export "throw1" (func $throw1))
  //   (export "b1" (tag 1))
  //   (func $throw1 (;0;) (type $v)
  //     throw $b1
  //   )
  // )
  std::array<WasmEdge::Byte, 85> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
      0x60, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x0d, 0x05, 0x02, 0x00,
      0x00, 0x00, 0x00, 0x07, 0x0f, 0x02, 0x06, 0x74, 0x68, 0x72, 0x6f,
      0x77, 0x31, 0x00, 0x00, 0x02, 0x62, 0x31, 0x04, 0x01, 0x0a, 0x06,
      0x01, 0x04, 0x00, 0x08, 0x01, 0x0b, 0x00, 0x21, 0x04, 0x6e, 0x61,
      0x6d, 0x65, 0x01, 0x09, 0x01, 0x00, 0x06, 0x74, 0x68, 0x72, 0x6f,
      0x77, 0x31, 0x04, 0x04, 0x01, 0x00, 0x01, 0x76, 0x0b, 0x09, 0x02,
      0x00, 0x02, 0x62, 0x30, 0x01, 0x02, 0x62, 0x31};
  // (module
  //   (type $v (;0;) (func))
  //   (type (;1;) (func (result i32)))
  //   (import "callee" "b1" (tag $ib1 (;0;) (type $v)))
  //   (import "callee" "throw1" (func $throw1 (;0;) (type $v)))
  //   (tag $a1 (;1;) (type $v))
  //   (export "run" (func 1))
  //   (elem (;0;) declare func $throw1)
  //   (func (;1;) (type 1) (result i32)
  //     block $on_b1
  //       block $on_any
  //         try_table (catch $ib1 $on_b1) (catch_all $on_any) ;; label = @3
  //           ref.func $throw1
  //           call_ref $v
  //         end
  //         i32.const 0
  //         return
  //       end
  //       i32.const 2
  //       return
  //     end
  //     i32.const 1
  //   )
  // )
  std::array<WasmEdge::Byte, 164> CallerWasm{
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

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

// A cross-module interpreter stack trace must attribute each frame to its own
// module, not to the top (callee) module.
TEST(LLVMRegressionTest, CrossModuleInterpreterTrapAttributesCallerModule) {
  // (module
  //   (type (;0;) (func (result i32)))
  //   (export "f" (func $f))
  //   (func $g (;0;) (type 0) (result i32)
  //     unreachable
  //   )
  //   (func $f (;1;) (type 0) (result i32)
  //     call $g
  //   )
  // )
  std::array<WasmEdge::Byte, 55> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
      0x60, 0x00, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00, 0x00, 0x07, 0x05,
      0x01, 0x01, 0x66, 0x00, 0x01, 0x0a, 0x0a, 0x02, 0x03, 0x00, 0x00,
      0x0b, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x0e, 0x04, 0x6e, 0x61,
      0x6d, 0x65, 0x01, 0x07, 0x02, 0x00, 0x01, 0x67, 0x01, 0x01, 0x66};
  // (module
  //   (type (;0;) (func (result i32)))
  //   (import "trapper2" "f" (func $f (;0;) (type 0)))
  //   (export "run" (func 1))
  //   (func (;1;) (type 0) (result i32)
  //     call $f
  //   )
  // )
  std::array<WasmEdge::Byte, 65> CallerWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
      0x60, 0x00, 0x01, 0x7f, 0x02, 0x0e, 0x01, 0x08, 0x74, 0x72, 0x61,
      0x70, 0x70, 0x65, 0x72, 0x32, 0x01, 0x66, 0x00, 0x00, 0x03, 0x02,
      0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01,
      0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x0b, 0x04,
      0x6e, 0x61, 0x6d, 0x65, 0x01, 0x04, 0x01, 0x00, 0x01, 0x66};

  Configure Conf;

  auto CalleeMod = loadModule(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = loadModule(Conf, CallerWasm);
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

// A trap in a frameless cross-module compiled callee must be attributed to the
// callee's module.
TEST(LLVMRegressionTest, CrossModuleCompiledFramelessTrapAttributedToCallee) {
  // (module
  //   (type (;0;) (func (result i32)))
  //   (export "boom" (func 0))
  //   (func (;0;) (type 0) (result i32)
  //     unreachable
  //   )
  // )
  std::array<WasmEdge::Byte, 36> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x08, 0x01, 0x04, 0x62,
      0x6f, 0x6f, 0x6d, 0x00, 0x00, 0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b};
  // (module
  //   (type $t (;0;) (func (result i32)))
  //   (import "trapper" "boom" (func $boom (;0;) (type $t)))
  //   (table (;0;) 1 funcref)
  //   (export "run" (func 1))
  //   (elem (;0;) (i32.const 0) func $boom)
  //   (func (;1;) (type $t) (result i32)
  //     i32.const 0
  //     call_indirect (type $t)
  //   )
  // )
  std::array<WasmEdge::Byte, 94> CallerWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x07, 0x74, 0x72, 0x61, 0x70, 0x70,
      0x65, 0x72, 0x04, 0x62, 0x6f, 0x6f, 0x6d, 0x00, 0x00, 0x03, 0x02, 0x01,
      0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72,
      0x75, 0x6e, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01,
      0x00, 0x0a, 0x09, 0x01, 0x07, 0x00, 0x41, 0x00, 0x11, 0x00, 0x00, 0x0b,
      0x00, 0x14, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x07, 0x01, 0x00, 0x04,
      0x62, 0x6f, 0x6f, 0x6d, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74};

  Configure Conf;

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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
    // code registers only on some platforms.
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

// Cross-module recursion shares one stack budget: 300 round trips exceed
// 256 KiB though each hop fits.
TEST(LLVMRegressionTest, CrossModuleRecursionSharesStackLimit) {
  // (module
  //   (type $t (;0;) (func (param i64) (result i64)))
  //   (table (;0;) 1 funcref)
  //   (export "tab" (table 0))
  //   (export "g" (func 0))
  //   (func (;0;) (type $t) (param i64) (result i64)
  //     local.get 0
  //     i32.const 0
  //     call_indirect (type $t)
  //   )
  // )
  std::array<WasmEdge::Byte, 65> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01,
      0x60, 0x01, 0x7e, 0x01, 0x7e, 0x03, 0x02, 0x01, 0x00, 0x04, 0x04,
      0x01, 0x70, 0x00, 0x01, 0x07, 0x0b, 0x02, 0x03, 0x74, 0x61, 0x62,
      0x01, 0x00, 0x01, 0x67, 0x00, 0x00, 0x0a, 0x0b, 0x01, 0x09, 0x00,
      0x20, 0x00, 0x41, 0x00, 0x11, 0x00, 0x00, 0x0b, 0x00, 0x0b, 0x04,
      0x6e, 0x61, 0x6d, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x74};
  // (module
  //   (type $t (;0;) (func (param i64) (result i64)))
  //   (import "callee" "g" (func $g (;0;) (type $t)))
  //   (import "callee" "tab" (table (;0;) 1 funcref))
  //   (export "f" (func $f))
  //   (elem (;0;) (i32.const 0) func $f)
  //   (func $f (;1;) (type $t) (param i64) (result i64)
  //     local.get 0
  //     i64.eqz
  //     if (result i64) ;; label = @1
  //       i64.const 0
  //     else
  //       local.get 0
  //       i64.const 1
  //       i64.sub
  //       call $g
  //       i64.const 1
  //       i64.add
  //     end
  //   )
  // )
  std::array<WasmEdge::Byte, 112> CallerWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
      0x01, 0x7e, 0x01, 0x7e, 0x02, 0x1b, 0x02, 0x06, 0x63, 0x61, 0x6c, 0x6c,
      0x65, 0x65, 0x01, 0x67, 0x00, 0x00, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65,
      0x65, 0x03, 0x74, 0x61, 0x62, 0x01, 0x70, 0x00, 0x01, 0x03, 0x02, 0x01,
      0x00, 0x07, 0x05, 0x01, 0x01, 0x66, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00,
      0x41, 0x00, 0x0b, 0x01, 0x01, 0x0a, 0x17, 0x01, 0x15, 0x00, 0x20, 0x00,
      0x50, 0x04, 0x7e, 0x42, 0x00, 0x05, 0x20, 0x00, 0x42, 0x01, 0x7d, 0x10,
      0x00, 0x42, 0x01, 0x7c, 0x0b, 0x0b, 0x00, 0x14, 0x04, 0x6e, 0x61, 0x6d,
      0x65, 0x01, 0x07, 0x02, 0x00, 0x01, 0x67, 0x01, 0x01, 0x66, 0x04, 0x04,
      0x01, 0x00, 0x01, 0x74};

  Configure Conf;
  Conf.getRuntimeConfigure().setMaxStackSize(UINT64_C(256) << 10);

  auto CalleeMod = compileToJIT(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
  ASSERT_NE(CallerMod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto CalleeInstOrErr = ExecEngine.registerModule(Store, *CalleeMod, "callee");
  ASSERT_TRUE(CalleeInstOrErr);
  auto CalleeInst = std::move(*CalleeInstOrErr);

  auto CallerInstOrErr = ExecEngine.instantiateModule(Store, *CallerMod);
  ASSERT_TRUE(CallerInstOrErr);
  auto CallerInst = std::move(*CallerInstOrErr);

  const auto *F = CallerInst->findFuncExports("f");
  ASSERT_NE(F, nullptr);
  ASSERT_TRUE(F->isCompiledFunction());

  const std::array<ValType, 1> ArgTypes{ValType(TypeCode::I64)};
  const std::array<ValVariant, 1> ShallowArgs{ValVariant(UINT64_C(5))};
  auto Shallow = ExecEngine.invoke(F, ShallowArgs, ArgTypes);
  ASSERT_TRUE(Shallow);
  EXPECT_EQ((*Shallow)[0].first.get<uint64_t>(), UINT64_C(5));

  const std::array<ValVariant, 1> DeepArgs{ValVariant(UINT64_C(300))};
  auto Deep = ExecEngine.invoke(F, DeepArgs, ArgTypes);
  ASSERT_FALSE(Deep);
  EXPECT_EQ(Deep.error(), ErrCode::Value::CallStackExhausted);
}

TEST(LLVMRegressionTest, CheckConfigure) {
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

// f32x4.max must return the RHS NaN when both operands are NaN (issue #4257).
TEST(LLVMRegressionTest, SIMDF32x4MaxNaNHandling) {
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
  std::array<WasmEdge::Byte, 88> Wasm{
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
              u8path("SIMDNaNTest" WASMEDGE_LIB_EXTENSION);

  auto Module = *Loader.parseModule(Wasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Path));

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

TEST(LLVMRegressionTest, Memory64BoundsCheck) {
  // A far out-of-bounds memory64 index escapes the guard page and must trap.
  // (module
  //   (memory $m1 (export "mem1") i64 1)
  //   (func (export "peek") (param i64) (result i64)
  //     (i64.load $m1 (local.get 0)))
  //   (func (export "poke") (param i64 i64)
  //     (i64.store $m1 (local.get 0) (local.get 1))))
  std::array<WasmEdge::Byte, 90> Wasm{
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
              u8path("AOTMemory64Test" WASMEDGE_LIB_EXTENSION);

  auto Module = *Loader.parseModule(Wasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Path));

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
      : Path(std::filesystem::temp_directory_path() /
             u8path(std::string(Stem) + "-" +
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

TEST(LLVMRegressionTest, NativeLoadSharedLibraryRequiresAOTMode) {
  // (module
  //   (type (;0;) (func (result i32)))
  //   (export "f" (func 0))
  //   (func (;0;) (type 0) (result i32)
  //     i32.const 42
  //   )
  // )
  std::array<WasmEdge::Byte, 34> Wasm{
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

    auto Module = Loader.parseModule(Wasm);
    ASSERT_TRUE(Module);
    ASSERT_TRUE(ValidatorEngine.validate(**Module));
    auto Data = Compiler.compile(**Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Path));
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

TEST(LLVMRegressionTest, NullLocalAbstractRefTypeIsBottomTyped) {
  // Compiled null locals of the abstract ref types must be bottom-typed, as in
  // the interpreter, or ref.test/ref.cast fail on them (issue #5343).
  // (module
  //   (type $t (struct (field (mut structref))))
  //   (type $a (array i32))
  //   (type $f (func))
  //   (type $r (func (result i32)))
  //   (func (export "test_struct") (type $r) (local structref)
  //     local.get 0 ref.test (ref null $t))
  //   (func (export "test_struct_nonnull") (type $r) (local structref)
  //     local.get 0 ref.test (ref $t))
  //   (func (export "cast_struct") (type $r) (local structref)
  //     local.get 0 ref.cast (ref null $t) drop i32.const 7)
  //   (func (export "test_any") (type $r) (local anyref)
  //     local.get 0 ref.test (ref null $t))
  //   (func (export "test_i31") (type $r) (local i31ref)
  //     local.get 0 ref.test (ref null $t))
  //   (func (export "test_array") (type $r) (local arrayref)
  //     local.get 0 ref.test (ref null $a))
  //   (func (export "test_func") (type $r) (local funcref)
  //     local.get 0 ref.test (ref null $f))
  //   (func (export "test_func_nonnull") (type $r) (local funcref)
  //     local.get 0 ref.test (ref $f))
  //   (func (export "cast_func") (type $r) (local funcref)
  //     local.get 0 ref.cast (ref null $f) drop i32.const 7)
  //   (func (export "test_extern") (type $r) (local externref)
  //     local.get 0 ref.test (ref null noextern)))
  std::array<WasmEdge::Byte, 294> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x04, 0x5f,
      0x01, 0x6b, 0x01, 0x5e, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01,
      0x7f, 0x03, 0x0b, 0x0a, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
      0x03, 0x03, 0x07, 0x90, 0x01, 0x0a, 0x0b, 0x74, 0x65, 0x73, 0x74, 0x5f,
      0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x00, 0x13, 0x74, 0x65, 0x73,
      0x74, 0x5f, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x5f, 0x6e, 0x6f, 0x6e,
      0x6e, 0x75, 0x6c, 0x6c, 0x00, 0x01, 0x0b, 0x63, 0x61, 0x73, 0x74, 0x5f,
      0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x02, 0x08, 0x74, 0x65, 0x73,
      0x74, 0x5f, 0x61, 0x6e, 0x79, 0x00, 0x03, 0x08, 0x74, 0x65, 0x73, 0x74,
      0x5f, 0x69, 0x33, 0x31, 0x00, 0x04, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f,
      0x61, 0x72, 0x72, 0x61, 0x79, 0x00, 0x05, 0x09, 0x74, 0x65, 0x73, 0x74,
      0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x06, 0x11, 0x74, 0x65, 0x73, 0x74,
      0x5f, 0x66, 0x75, 0x6e, 0x63, 0x5f, 0x6e, 0x6f, 0x6e, 0x6e, 0x75, 0x6c,
      0x6c, 0x00, 0x07, 0x09, 0x63, 0x61, 0x73, 0x74, 0x5f, 0x66, 0x75, 0x6e,
      0x63, 0x00, 0x08, 0x0b, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x65, 0x78, 0x74,
      0x65, 0x72, 0x6e, 0x00, 0x09, 0x0a, 0x6b, 0x0a, 0x09, 0x01, 0x01, 0x6b,
      0x20, 0x00, 0xfb, 0x15, 0x00, 0x0b, 0x09, 0x01, 0x01, 0x6b, 0x20, 0x00,
      0xfb, 0x14, 0x00, 0x0b, 0x0c, 0x01, 0x01, 0x6b, 0x20, 0x00, 0xfb, 0x17,
      0x00, 0x1a, 0x41, 0x07, 0x0b, 0x09, 0x01, 0x01, 0x6e, 0x20, 0x00, 0xfb,
      0x15, 0x00, 0x0b, 0x09, 0x01, 0x01, 0x6c, 0x20, 0x00, 0xfb, 0x15, 0x00,
      0x0b, 0x09, 0x01, 0x01, 0x6a, 0x20, 0x00, 0xfb, 0x15, 0x01, 0x0b, 0x09,
      0x01, 0x01, 0x70, 0x20, 0x00, 0xfb, 0x15, 0x02, 0x0b, 0x09, 0x01, 0x01,
      0x70, 0x20, 0x00, 0xfb, 0x14, 0x02, 0x0b, 0x0c, 0x01, 0x01, 0x70, 0x20,
      0x00, 0xfb, 0x17, 0x02, 0x1a, 0x41, 0x07, 0x0b, 0x09, 0x01, 0x01, 0x6f,
      0x20, 0x00, 0xfb, 0x15, 0x72, 0x0b};

  Configure Conf;
  auto Mod = compileToJIT(Conf, Wasm);
  ASSERT_NE(Mod, nullptr);

  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;
  auto InstOrErr = ExecEngine.instantiateModule(Store, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);

  // A null structref local bottoms out at none: it matches (ref null $t) but
  // not (ref $t), and ref.cast to (ref null $t) must not trap.
  const auto *TestStruct = Inst->findFuncExports("test_struct");
  ASSERT_NE(TestStruct, nullptr);
  ASSERT_TRUE(TestStruct->isCompiledFunction());
  auto RTestStruct = ExecEngine.invoke(TestStruct, {}, {});
  ASSERT_TRUE(RTestStruct);
  ASSERT_EQ(RTestStruct->size(), 1u);
  EXPECT_EQ((*RTestStruct)[0].first.get<uint32_t>(), 1u);

  const auto *TestStructNonNull = Inst->findFuncExports("test_struct_nonnull");
  ASSERT_NE(TestStructNonNull, nullptr);
  ASSERT_TRUE(TestStructNonNull->isCompiledFunction());
  auto RTestStructNonNull = ExecEngine.invoke(TestStructNonNull, {}, {});
  ASSERT_TRUE(RTestStructNonNull);
  ASSERT_EQ(RTestStructNonNull->size(), 1u);
  EXPECT_EQ((*RTestStructNonNull)[0].first.get<uint32_t>(), 0u);

  const auto *CastStruct = Inst->findFuncExports("cast_struct");
  ASSERT_NE(CastStruct, nullptr);
  ASSERT_TRUE(CastStruct->isCompiledFunction());
  auto RCastStruct = ExecEngine.invoke(CastStruct, {}, {});
  ASSERT_TRUE(RCastStruct) << "ref.cast trapped on a null structref local";
  ASSERT_EQ(RCastStruct->size(), 1u);
  EXPECT_EQ((*RCastStruct)[0].first.get<uint32_t>(), 7u);

  // The anyref, i31ref, and arrayref null locals bottom out at none as well.
  const auto *TestAny = Inst->findFuncExports("test_any");
  ASSERT_NE(TestAny, nullptr);
  ASSERT_TRUE(TestAny->isCompiledFunction());
  auto RTestAny = ExecEngine.invoke(TestAny, {}, {});
  ASSERT_TRUE(RTestAny);
  ASSERT_EQ(RTestAny->size(), 1u);
  EXPECT_EQ((*RTestAny)[0].first.get<uint32_t>(), 1u);

  const auto *TestI31 = Inst->findFuncExports("test_i31");
  ASSERT_NE(TestI31, nullptr);
  ASSERT_TRUE(TestI31->isCompiledFunction());
  auto RTestI31 = ExecEngine.invoke(TestI31, {}, {});
  ASSERT_TRUE(RTestI31);
  ASSERT_EQ(RTestI31->size(), 1u);
  EXPECT_EQ((*RTestI31)[0].first.get<uint32_t>(), 1u);

  const auto *TestArray = Inst->findFuncExports("test_array");
  ASSERT_NE(TestArray, nullptr);
  ASSERT_TRUE(TestArray->isCompiledFunction());
  auto RTestArray = ExecEngine.invoke(TestArray, {}, {});
  ASSERT_TRUE(RTestArray);
  ASSERT_EQ(RTestArray->size(), 1u);
  EXPECT_EQ((*RTestArray)[0].first.get<uint32_t>(), 1u);

  // A null funcref local bottoms out at nofunc: it matches (ref null $f) but
  // not (ref $f), and ref.cast to (ref null $f) must not trap.
  const auto *TestFunc = Inst->findFuncExports("test_func");
  ASSERT_NE(TestFunc, nullptr);
  ASSERT_TRUE(TestFunc->isCompiledFunction());
  auto RTestFunc = ExecEngine.invoke(TestFunc, {}, {});
  ASSERT_TRUE(RTestFunc);
  ASSERT_EQ(RTestFunc->size(), 1u);
  EXPECT_EQ((*RTestFunc)[0].first.get<uint32_t>(), 1u);

  const auto *TestFuncNonNull = Inst->findFuncExports("test_func_nonnull");
  ASSERT_NE(TestFuncNonNull, nullptr);
  ASSERT_TRUE(TestFuncNonNull->isCompiledFunction());
  auto RTestFuncNonNull = ExecEngine.invoke(TestFuncNonNull, {}, {});
  ASSERT_TRUE(RTestFuncNonNull);
  ASSERT_EQ(RTestFuncNonNull->size(), 1u);
  EXPECT_EQ((*RTestFuncNonNull)[0].first.get<uint32_t>(), 0u);

  const auto *CastFunc = Inst->findFuncExports("cast_func");
  ASSERT_NE(CastFunc, nullptr);
  ASSERT_TRUE(CastFunc->isCompiledFunction());
  auto RCastFunc = ExecEngine.invoke(CastFunc, {}, {});
  ASSERT_TRUE(RCastFunc) << "ref.cast trapped on a null funcref local";
  ASSERT_EQ(RCastFunc->size(), 1u);
  EXPECT_EQ((*RCastFunc)[0].first.get<uint32_t>(), 7u);

  // A null externref local bottoms out at noextern.
  const auto *TestExtern = Inst->findFuncExports("test_extern");
  ASSERT_NE(TestExtern, nullptr);
  ASSERT_TRUE(TestExtern->isCompiledFunction());
  auto RTestExtern = ExecEngine.invoke(TestExtern, {}, {});
  ASSERT_TRUE(RTestExtern);
  ASSERT_EQ(RTestExtern->size(), 1u);
  EXPECT_EQ((*RTestExtern)[0].first.get<uint32_t>(), 1u);
}

TEST(LLVMRegressionTest, TmpValuesLoopKeepsNativeStackFlat) {
  // Temporaries of runtime calls in a loop must not grow the native stack per
  // iteration (issue #4636); calls to the interpreted callee take the fallback.
  // (module
  //   (func (export "one") (result i32) (i32.const 1)))
  std::array<WasmEdge::Byte, 36> CalleeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x6f,
      0x6e, 0x65, 0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x01, 0x0b};

  // (module
  //   (type $s (struct (field (mut i32)) (field (mut i64))))
  //   (type $a (array (mut i32)))
  //   (type $v (func (result i32)))
  //   (import "callee" "one" (func $one (type $v)))
  //   (table 1 funcref) (elem (i32.const 0) func $one) (elem declare func $one)
  //   (tag $e (param i32))
  //   (func (export "run") (param $n i32) (result i32)
  //     (local $i i32) (local $sum i32)
  //     (local $o (ref null $s)) (local $arr (ref null $a))
  //     (loop $l
  //       (local.set $o (struct.new $s (i32.const 0) (i64.const 0)))
  //       (struct.set $s 0 (local.get $o) (i32.const 1))
  //       (local.set $sum (i32.add (local.get $sum)
  //         (struct.get $s 0 (local.get $o))))
  //       (local.set $arr (array.new_fixed $a 2 (i32.const 0) (i32.const 0)))
  //       (array.set $a (local.get $arr) (i32.const 0) (i32.const 1))
  //       (array.fill $a (local.get $arr) (i32.const 1) (i32.const 1)
  //         (i32.const 1))
  //       (local.set $sum (i32.add (local.get $sum)
  //         (array.get $a (local.get $arr) (i32.const 1))))
  //       (drop (array.new $a (i32.const 0) (i32.const 1)))
  //       (local.set $sum (i32.add (local.get $sum)
  //         (block $h (result i32)
  //           (try_table (catch $e $h) (throw $e (i32.const 1)))
  //           (i32.const 0))))
  //       (local.set $sum (i32.add (local.get $sum)
  //         (call_indirect (type $v) (i32.const 0))))
  //       (local.set $sum (i32.add (local.get $sum)
  //         (call_ref $v (ref.func $one))))
  //       (br_if $l (i32.lt_u
  //         (local.tee $i (i32.add (local.get $i) (i32.const 1)))
  //         (local.get $n))))
  //     (local.get $sum)))
  std::array<WasmEdge::Byte, 237> CallerWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x17, 0x05, 0x5f,
      0x02, 0x7f, 0x01, 0x7e, 0x01, 0x5e, 0x7f, 0x01, 0x60, 0x00, 0x01, 0x7f,
      0x60, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x02, 0x0e, 0x01,
      0x06, 0x63, 0x61, 0x6c, 0x6c, 0x65, 0x65, 0x03, 0x6f, 0x6e, 0x65, 0x00,
      0x02, 0x03, 0x02, 0x01, 0x04, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01, 0x0d,
      0x03, 0x01, 0x00, 0x03, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00,
      0x01, 0x09, 0x0b, 0x02, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x00, 0x03, 0x00,
      0x01, 0x00, 0x0a, 0x94, 0x01, 0x01, 0x91, 0x01, 0x03, 0x02, 0x7f, 0x01,
      0x63, 0x00, 0x01, 0x63, 0x01, 0x03, 0x40, 0x41, 0x00, 0x42, 0x00, 0xfb,
      0x00, 0x00, 0x21, 0x03, 0x20, 0x03, 0x41, 0x01, 0xfb, 0x05, 0x00, 0x00,
      0x20, 0x02, 0x20, 0x03, 0xfb, 0x02, 0x00, 0x00, 0x6a, 0x21, 0x02, 0x41,
      0x00, 0x41, 0x00, 0xfb, 0x08, 0x01, 0x02, 0x21, 0x04, 0x20, 0x04, 0x41,
      0x00, 0x41, 0x01, 0xfb, 0x0e, 0x01, 0x20, 0x04, 0x41, 0x01, 0x41, 0x01,
      0x41, 0x01, 0xfb, 0x10, 0x01, 0x20, 0x02, 0x20, 0x04, 0x41, 0x01, 0xfb,
      0x0b, 0x01, 0x6a, 0x21, 0x02, 0x41, 0x00, 0x41, 0x01, 0xfb, 0x06, 0x01,
      0x1a, 0x20, 0x02, 0x02, 0x7f, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x41,
      0x01, 0x08, 0x00, 0x0b, 0x41, 0x00, 0x0b, 0x6a, 0x21, 0x02, 0x20, 0x02,
      0x41, 0x00, 0x11, 0x02, 0x00, 0x6a, 0x21, 0x02, 0x20, 0x02, 0xd2, 0x00,
      0x14, 0x02, 0x6a, 0x21, 0x02, 0x20, 0x01, 0x41, 0x01, 0x6a, 0x22, 0x01,
      0x20, 0x00, 0x49, 0x0d, 0x00, 0x0b, 0x20, 0x02, 0x0b};

  Configure Conf;
  auto CalleeMod = loadModule(Conf, CalleeWasm);
  ASSERT_NE(CalleeMod, nullptr);
  auto CallerMod = compileToJIT(Conf, CallerWasm);
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

  // A leak of 240 bytes per iteration would need 24 MB of stack.
  const std::array<ValVariant, 1> Args{ValVariant(uint32_t(100000U))};
  const std::array<ValType, 1> ArgTypes{ValType(TypeCode::I32)};
  auto R = ExecEngine.invoke(Run, Args, ArgTypes);
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), 500000u);
}

} // namespace
