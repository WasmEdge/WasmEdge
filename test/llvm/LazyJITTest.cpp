// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/llvm/LazyJITTest.cpp - Lazy JIT compilation tests ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for the lazy JIT (per-function) compilation
/// feature. It verifies that:
/// 1. Lazy JIT mode can be enabled via configuration
/// 2. Functions are compiled on-demand rather than upfront
/// 3. The behavior is correct compared to eager compilation
///
//===----------------------------------------------------------------------===//

#include "common/configure.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "vm/vm.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"
#include <gtest/gtest.h>
#include <thread>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Module with 6 functions: add, mul, sub, const42, unused1, unused2
std::vector<uint8_t> SimpleWasm = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x17, 0x04, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x01, 0x7f,
    0x01, 0x7f, 0x60, 0x03, 0x7f, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x07, 0x06,
    0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x07, 0x31, 0x06, 0x03, 0x61, 0x64,
    0x64, 0x00, 0x00, 0x03, 0x6d, 0x75, 0x6c, 0x00, 0x01, 0x03, 0x73, 0x75,
    0x62, 0x00, 0x02, 0x07, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x34, 0x32, 0x00,
    0x03, 0x07, 0x75, 0x6e, 0x75, 0x73, 0x65, 0x64, 0x31, 0x00, 0x04, 0x07,
    0x75, 0x6e, 0x75, 0x73, 0x65, 0x64, 0x32, 0x00, 0x05, 0x0a, 0x32, 0x06,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b, 0x07, 0x00, 0x20, 0x00,
    0x20, 0x01, 0x6c, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6b, 0x0b,
    0x04, 0x00, 0x41, 0x2a, 0x0b, 0x08, 0x00, 0x20, 0x00, 0x41, 0xe4, 0x00,
    0x6a, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x20, 0x02, 0x6c,
    0x0b};

std::vector<uint8_t> FibonacciWasm = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01,
    0x60, 0x01, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07,
    0x01, 0x03, 0x66, 0x69, 0x62, 0x00, 0x00, 0x0a, 0x1e, 0x01, 0x1c,
    0x00, 0x20, 0x00, 0x41, 0x02, 0x48, 0x04, 0x7f, 0x41, 0x01, 0x05,
    0x20, 0x00, 0x41, 0x02, 0x6b, 0x10, 0x00, 0x20, 0x00, 0x41, 0x01,
    0x6b, 0x10, 0x00, 0x6a, 0x0b, 0x0b};

// Module with 1 imported function and 3 local functions:
//   import "env" "host_add" (func (param i32 i32) (result i32))
//   export "call_host"  -> calls host_add
//   export "double_add" -> calls host_add twice, adds results
//   export "local_mul"  -> pure local i32.mul
std::vector<uint8_t> ImportWasm = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x03, 0x65, 0x6e, 0x76,
    0x08, 0x68, 0x6f, 0x73, 0x74, 0x5f, 0x61, 0x64, 0x64, 0x00, 0x00, 0x03,
    0x04, 0x03, 0x00, 0x00, 0x00, 0x07, 0x26, 0x03, 0x09, 0x63, 0x61, 0x6c,
    0x6c, 0x5f, 0x68, 0x6f, 0x73, 0x74, 0x00, 0x01, 0x0a, 0x64, 0x6f, 0x75,
    0x62, 0x6c, 0x65, 0x5f, 0x61, 0x64, 0x64, 0x00, 0x02, 0x09, 0x6c, 0x6f,
    0x63, 0x61, 0x6c, 0x5f, 0x6d, 0x75, 0x6c, 0x00, 0x03, 0x0a, 0x22, 0x03,
    0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0b, 0x0f, 0x00, 0x20,
    0x00, 0x20, 0x01, 0x10, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x6a,
    0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6c, 0x0b};

// Library module exporting "add" and "mul" (i32,i32)->i32
std::vector<uint8_t> MathLibWasm = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00, 0x00, 0x07, 0x0d,
    0x02, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x03, 0x6d, 0x75, 0x6c, 0x00,
    0x01, 0x0a, 0x11, 0x02, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6c, 0x0b};

// Consumer module importing "math"."add" and "math"."mul",
// exporting "add_and_square" = (a+b)*(a+b) and "sum_of_squares" = a*a+b*b
std::vector<uint8_t> MathConsumerWasm = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02, 0x17, 0x02, 0x04, 0x6d,
    0x61, 0x74, 0x68, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x04, 0x6d,
    0x61, 0x74, 0x68, 0x03, 0x6d, 0x75, 0x6c, 0x00, 0x00, 0x03, 0x03,
    0x02, 0x00, 0x00, 0x07, 0x23, 0x02, 0x0e, 0x61, 0x64, 0x64, 0x5f,
    0x61, 0x6e, 0x64, 0x5f, 0x73, 0x71, 0x75, 0x61, 0x72, 0x65, 0x00,
    0x02, 0x0e, 0x73, 0x75, 0x6d, 0x5f, 0x6f, 0x66, 0x5f, 0x73, 0x71,
    0x75, 0x61, 0x72, 0x65, 0x73, 0x00, 0x03, 0x0a, 0x23, 0x02, 0x10,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x20, 0x00, 0x20, 0x01,
    0x10, 0x00, 0x10, 0x01, 0x0b, 0x10, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x10, 0x01, 0x20, 0x01, 0x20, 0x01, 0x10, 0x01, 0x10, 0x00, 0x0b};

// Module exercising the runtime intrinsics table from lazily compiled code:
//   (memory 1)
//   export "trap" -> unreachable   (reaches the trap intrinsic)
//   export "grow" -> memory.grow   (reaches the memory-grow intrinsic)
std::vector<uint8_t> IntrinsicsWasm = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02,
    0x60, 0x00, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x03, 0x03,
    0x02, 0x00, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x0f, 0x02,
    0x04, 0x74, 0x72, 0x61, 0x70, 0x00, 0x00, 0x04, 0x67, 0x72, 0x6f,
    0x77, 0x00, 0x01, 0x0a, 0x0c, 0x02, 0x03, 0x00, 0x00, 0x0b, 0x06,
    0x00, 0x20, 0x00, 0x40, 0x00, 0x0b};

class HostAdd : public Runtime::HostFunction<HostAdd> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t A, uint32_t B) {
    return A + B;
  }
};

class TestEnvModule : public Runtime::Instance::ModuleInstance {
public:
  TestEnvModule() : ModuleInstance("env") {
    addHostFunc("host_add", std::make_unique<HostAdd>());
  }
};

class LazyJITTest : public ::testing::Test {
protected:
  // Helper to create a VM with eager JIT
  std::unique_ptr<VM::VM> createEagerJITVM() {
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
    Conf.getCompilerConfigure().setOptimizationLevel(
        CompilerConfigure::OptimizationLevel::O1);
    return std::make_unique<VM::VM>(Conf);
  }

  // Helper to create a VM with lazy JIT
  std::unique_ptr<VM::VM> createLazyJITVM() {
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
    Conf.getCompilerConfigure().setOptimizationLevel(
        CompilerConfigure::OptimizationLevel::O1);
    return std::make_unique<VM::VM>(Conf);
  }
};

TEST_F(LazyJITTest, ConfigurationDefaultDisabled) {
  Configure Conf;
  EXPECT_EQ(Conf.getRuntimeConfigure().getRunMode(), RunMode::Interpreter);
}

TEST_F(LazyJITTest, ConfigurationEnableDisable) {
  Configure Conf;

  // Enable lazy JIT
  Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
  EXPECT_EQ(Conf.getRuntimeConfigure().getRunMode(), RunMode::LazyJIT);

  // Disable lazy JIT (back to interpreter)
  Conf.getRuntimeConfigure().setRunMode(RunMode::Interpreter);
  EXPECT_NE(Conf.getRuntimeConfigure().getRunMode(), RunMode::LazyJIT);
}

TEST_F(LazyJITTest, RuntimeConfigureIndependent) {
  RuntimeConfigure RConf1;
  RuntimeConfigure RConf2;

  RConf1.setRunMode(RunMode::LazyJIT);
  EXPECT_EQ(RConf1.getRunMode(), RunMode::LazyJIT);
  EXPECT_NE(RConf2.getRunMode(), RunMode::LazyJIT);
}

TEST_F(LazyJITTest, LazyJITCorrectness) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // Test add function - should trigger lazy compilation
  std::vector<ValVariant> AddP = {10U, 5U};
  auto AddResult = VM->execute("add", AddP, Types);
  ASSERT_TRUE(AddResult);
  EXPECT_EQ((*AddResult)[0].first.get<uint32_t>(), 15U);

  // Test mul function - should trigger lazy compilation
  std::vector<ValVariant> MulP = {7U, 6U};
  auto MulResult = VM->execute("mul", MulP, Types);
  ASSERT_TRUE(MulResult);
  EXPECT_EQ((*MulResult)[0].first.get<uint32_t>(), 42U);

  // Test sub function - should trigger lazy compilation
  std::vector<ValVariant> SubP = {20U, 8U};
  auto SubResult = VM->execute("sub", SubP, Types);
  ASSERT_TRUE(SubResult);
  EXPECT_EQ((*SubResult)[0].first.get<uint32_t>(), 12U);

  // Test const42 function
  std::vector<ValVariant> EmptyP = {};
  std::vector<ValType> EmptyT = {};
  auto ConstResult = VM->execute("const42", EmptyP, EmptyT);
  ASSERT_TRUE(ConstResult);
  EXPECT_EQ((*ConstResult)[0].first.get<uint32_t>(), 42U);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITRepeatCalls) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 0U);

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // First call - triggers lazy compilation
  std::vector<ValVariant> P1 = {10U, 5U};
  auto Result1 = VM->execute("add", P1, Types);
  ASSERT_TRUE(Result1);
  EXPECT_EQ((*Result1)[0].first.get<uint32_t>(), 15U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  // Second call - uses cached compiled code
  std::vector<ValVariant> P2 = {100U, 200U};
  auto Result2 = VM->execute("add", P2, Types);
  ASSERT_TRUE(Result2);
  EXPECT_EQ((*Result2)[0].first.get<uint32_t>(), 300U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  // Third call - different values
  std::vector<ValVariant> P3 = {1U, 1U};
  auto Result3 = VM->execute("add", P3, Types);
  ASSERT_TRUE(Result3);
  EXPECT_EQ((*Result3)[0].first.get<uint32_t>(), 2U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITFibonacci) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(FibonacciWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32)};

  // Test fibonacci function with various inputs
  // fib(0) = 1, fib(1) = 1, fib(2) = 2, fib(3) = 3, fib(4) = 5, fib(5) =8
  std::vector<std::pair<uint32_t, uint32_t>> TestCases = {
      {0, 1}, {1, 1}, {2, 2}, {3, 3}, {4, 5}, {5, 8}, {10, 89}};

  for (const auto &[Input, Expected] : TestCases) {
    std::vector<ValVariant> Params = {Input};
    auto Result = VM->execute("fib", Params, Types);
    ASSERT_TRUE(Result) << "fib(" << Input << ") failed";
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), Expected)
        << "fib(" << Input << ") = " << Expected;
  }

  VM->cleanup();
}

TEST_F(LazyJITTest, EagerVsLazyResultsMatch) {
  auto EagerVM = createEagerJITVM();
  auto LazyVM = createLazyJITVM();

  ASSERT_TRUE(EagerVM->loadWasm(SimpleWasm));
  ASSERT_TRUE(EagerVM->validate());
  ASSERT_TRUE(EagerVM->instantiate());

  ASSERT_TRUE(LazyVM->loadWasm(SimpleWasm));
  ASSERT_TRUE(LazyVM->validate());
  ASSERT_TRUE(LazyVM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // Test with various inputs
  std::vector<std::pair<uint32_t, uint32_t>> TestInputs = {
      {10, 5}, {0, 0}, {100, 200}, {7, 6}, {0, 100}};

  for (const auto &[A, B] : TestInputs) {
    std::vector<ValVariant> Params = {A, B};

    auto EagerAdd = EagerVM->execute("add", Params, Types);
    auto LazyAdd = LazyVM->execute("add", Params, Types);

    ASSERT_TRUE(EagerAdd);
    ASSERT_TRUE(LazyAdd);
    EXPECT_EQ((*EagerAdd)[0].first.get<uint32_t>(),
              (*LazyAdd)[0].first.get<uint32_t>())
        << "add(" << A << ", " << B << ") mismatch";

    auto EagerMul = EagerVM->execute("mul", Params, Types);
    auto LazyMul = LazyVM->execute("mul", Params, Types);

    ASSERT_TRUE(EagerMul);
    ASSERT_TRUE(LazyMul);
    EXPECT_EQ((*EagerMul)[0].first.get<uint32_t>(),
              (*LazyMul)[0].first.get<uint32_t>())
        << "mul(" << A << ", " << B << ") mismatch";
  }

  EagerVM->cleanup();
  LazyVM->cleanup();
}

TEST_F(LazyJITTest, EagerVsLazyFibonacciMatch) {
  auto EagerVM = createEagerJITVM();
  auto LazyVM = createLazyJITVM();

  ASSERT_TRUE(EagerVM->loadWasm(FibonacciWasm));
  ASSERT_TRUE(EagerVM->validate());
  ASSERT_TRUE(EagerVM->instantiate());

  ASSERT_TRUE(LazyVM->loadWasm(FibonacciWasm));
  ASSERT_TRUE(LazyVM->validate());
  ASSERT_TRUE(LazyVM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32)};

  for (uint32_t N = 0; N <= 15; ++N) {
    std::vector<ValVariant> Params = {N};
    auto EagerResult = EagerVM->execute("fib", Params, Types);
    auto LazyResult = LazyVM->execute("fib", Params, Types);

    ASSERT_TRUE(EagerResult) << "Eager fib(" << N << ") failed";
    ASSERT_TRUE(LazyResult) << "Lazy fib(" << N << ") failed";
    EXPECT_EQ((*EagerResult)[0].first.get<uint32_t>(),
              (*LazyResult)[0].first.get<uint32_t>())
        << "fib(" << N << ") mismatch between eager and lazy JIT";
  }

  EagerVM->cleanup();
  LazyVM->cleanup();
}

TEST_F(LazyJITTest, LazyJITCallNonExistent) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  // Try to call a non-existent function
  std::vector<ValVariant> EmptyP = {};
  std::vector<ValType> EmptyT = {};
  auto Result = VM->execute("nonexistent", EmptyP, EmptyT);
  EXPECT_FALSE(Result);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITMultipleInstantiations) {
  for (int Idx = 0; Idx < 3; ++Idx) {
    auto VM = createLazyJITVM();

    ASSERT_TRUE(VM->loadWasm(SimpleWasm));
    ASSERT_TRUE(VM->validate());
    ASSERT_TRUE(VM->instantiate());

    std::vector<ValVariant> Params = {static_cast<uint32_t>(Idx),
                                      static_cast<uint32_t>(Idx * 10)};
    std::vector<ValType> Types = {ValType(TypeCode::I32),
                                  ValType(TypeCode::I32)};
    auto Result = VM->execute("add", Params, Types);

    ASSERT_TRUE(Result);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(),
              static_cast<uint32_t>(Idx + Idx * 10));

    VM->cleanup();
  }
}

TEST_F(LazyJITTest, LazyJITReinstantiateSameVM) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValVariant> Params = {static_cast<uint32_t>(2),
                                    static_cast<uint32_t>(3)};
  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // Compile "add" lazily on the first instance.
  auto Result = VM->execute("add", Params, Types);
  ASSERT_TRUE(Result);
  EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 5U);
  const auto CompiledBefore = VM->getLazyCompiledFuncCount();
  EXPECT_GT(CompiledBefore, 0U);

  // Re-instantiate the same loaded module on the same VM. The engine must
  // rebind the persisted JIT state to the new module instance.
  ASSERT_TRUE(VM->instantiate());

  // The previously compiled function is restored to compiled mode...
  auto *AddFunc = VM->getActiveModule()->findFuncExports("add");
  ASSERT_NE(AddFunc, nullptr);
  EXPECT_TRUE(AddFunc->isCompiledFunction());

  // ...still executes correctly...
  Result = VM->execute("add", Params, Types);
  ASSERT_TRUE(Result);
  EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 5U);

  // ...and lazy compilation keeps working for functions not yet compiled.
  Result = VM->execute("mul", Params, Types);
  ASSERT_TRUE(Result);
  EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 6U);
  EXPECT_GT(VM->getLazyCompiledFuncCount(), CompiledBefore);
}

TEST_F(LazyJITTest, LazyJITOnlySomeFunctionsCalled) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  auto *AddFunc = VM->getActiveModule()->findFuncExports("add");
  auto *MulFunc = VM->getActiveModule()->findFuncExports("mul");
  auto *SubFunc = VM->getActiveModule()->findFuncExports("sub");

  ASSERT_NE(AddFunc, nullptr);
  ASSERT_NE(MulFunc, nullptr);
  ASSERT_NE(SubFunc, nullptr);

  // Initially none should be compiled
  EXPECT_FALSE(AddFunc->isCompiledFunction());
  EXPECT_FALSE(MulFunc->isCompiledFunction());
  EXPECT_FALSE(SubFunc->isCompiledFunction());
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 0U);

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // Call 'add' should trigger compilation of function 0
  std::vector<ValVariant> AddP = {1U, 2U};
  auto AddResult = VM->execute("add", AddP, Types);
  ASSERT_TRUE(AddResult);
  EXPECT_EQ((*AddResult)[0].first.get<uint32_t>(), 3U);

  // Check state: add compiled, others not
  EXPECT_TRUE(AddFunc->isCompiledFunction());
  EXPECT_FALSE(MulFunc->isCompiledFunction());
  EXPECT_FALSE(SubFunc->isCompiledFunction());
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  // Call 'mul' should trigger compilation of function 1
  std::vector<ValVariant> MulP = {3U, 4U};
  auto MulResult = VM->execute("mul", MulP, Types);
  ASSERT_TRUE(MulResult);
  EXPECT_EQ((*MulResult)[0].first.get<uint32_t>(), 12U);

  // Check state: add and mul compiled, sub not
  EXPECT_TRUE(AddFunc->isCompiledFunction());
  EXPECT_TRUE(MulFunc->isCompiledFunction());
  EXPECT_FALSE(SubFunc->isCompiledFunction());
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 2U);

  // Call 'add' again should NOT increase compiled count (already compiled)
  std::vector<ValVariant> AddP2 = {5U, 6U};
  auto AddResult2 = VM->execute("add", AddP2, Types);
  ASSERT_TRUE(AddResult2);
  EXPECT_EQ((*AddResult2)[0].first.get<uint32_t>(), 11U);

  // Check state: add and mul compiled, sub not
  EXPECT_TRUE(AddFunc->isCompiledFunction());
  EXPECT_TRUE(MulFunc->isCompiledFunction());
  EXPECT_FALSE(SubFunc->isCompiledFunction());
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 2U);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITWithImports) {
  TestEnvModule HostMod;

  auto VM = createLazyJITVM();
  ASSERT_TRUE(VM->registerModule(HostMod));

  ASSERT_TRUE(VM->loadWasm(ImportWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 0U);

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // call_host delegates to the imported host_add; only the local wrapper
  // should be lazy-compiled, not the import.
  std::vector<ValVariant> P1 = {3U, 7U};
  auto R1 = VM->execute("call_host", P1, Types);
  ASSERT_TRUE(R1);
  EXPECT_EQ((*R1)[0].first.get<uint32_t>(), 10U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  // local_mul is a pure local function with no import usage.
  std::vector<ValVariant> P2 = {4U, 5U};
  auto R2 = VM->execute("local_mul", P2, Types);
  ASSERT_TRUE(R2);
  EXPECT_EQ((*R2)[0].first.get<uint32_t>(), 20U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 2U);

  // double_add calls host_add twice and adds the results.
  std::vector<ValVariant> P3 = {6U, 8U};
  auto R3 = VM->execute("double_add", P3, Types);
  ASSERT_TRUE(R3);
  EXPECT_EQ((*R3)[0].first.get<uint32_t>(), 28U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 3U);

  // Repeat call_host – count must not change (cached).
  std::vector<ValVariant> P4 = {100U, 200U};
  auto R4 = VM->execute("call_host", P4, Types);
  ASSERT_TRUE(R4);
  EXPECT_EQ((*R4)[0].first.get<uint32_t>(), 300U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 3U);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITWasmImportsWasm) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->registerModule("math", MathLibWasm));

  ASSERT_TRUE(VM->loadWasm(MathConsumerWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 0U);

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  std::vector<ValVariant> P1 = {3U, 4U};
  auto R1 = VM->execute("add_and_square", P1, Types);
  ASSERT_TRUE(R1);
  EXPECT_EQ((*R1)[0].first.get<uint32_t>(), 49U);
  uint32_t CountAfterFirst = VM->getLazyCompiledFuncCount();
  EXPECT_EQ(CountAfterFirst, 3U);

  std::vector<ValVariant> P2 = {3U, 4U};
  auto R2 = VM->execute("sum_of_squares", P2, Types);
  ASSERT_TRUE(R2);
  EXPECT_EQ((*R2)[0].first.get<uint32_t>(), 25U);
  uint32_t CountAfterSecond = VM->getLazyCompiledFuncCount();
  EXPECT_EQ(CountAfterSecond, 4U);

  std::vector<ValVariant> P3 = {10U, 20U};
  auto R3 = VM->execute("add_and_square", P3, Types);
  ASSERT_TRUE(R3);
  EXPECT_EQ((*R3)[0].first.get<uint32_t>(), 900U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), CountAfterSecond);

  std::vector<ValVariant> P4 = {5U, 12U};
  auto R4 = VM->execute("sum_of_squares", P4, Types);
  ASSERT_TRUE(R4);
  EXPECT_EQ((*R4)[0].first.get<uint32_t>(), 169U);
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), CountAfterSecond);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITReferenceModuleNotReleasedOnCleanup) {
  bool Destroyed = false;
  auto Deleter = [&Destroyed](AST::Module *M) {
    Destroyed = true;
    delete M;
  };

  Configure Conf;
  Loader::Loader LoaderEngine(Conf);
  auto ModOrErr = LoaderEngine.parseWasmUnit(SimpleWasm);
  ASSERT_TRUE(ModOrErr);
  auto &RawMod = std::get<std::unique_ptr<AST::Module>>(*ModOrErr);
  auto Module = std::shared_ptr<AST::Module>(RawMod.release(), Deleter);

  auto VM = createLazyJITVM();
  ASSERT_TRUE(VM->registerModule("test", *Module));

  VM->cleanup();
  EXPECT_FALSE(Destroyed);

  Module.reset();
  EXPECT_TRUE(Destroyed);
}

TEST_F(LazyJITTest, LazyJITReferenceModuleNotReleasedOnVMDestruction) {
  bool Destroyed = false;
  auto Deleter = [&Destroyed](AST::Module *M) {
    Destroyed = true;
    delete M;
  };

  Configure Conf;
  Loader::Loader LoaderEngine(Conf);
  auto ModOrErr = LoaderEngine.parseWasmUnit(SimpleWasm);
  ASSERT_TRUE(ModOrErr);
  auto &RawMod = std::get<std::unique_ptr<AST::Module>>(*ModOrErr);
  auto Module = std::shared_ptr<AST::Module>(RawMod.release(), Deleter);

  {
    auto VM = createLazyJITVM();
    ASSERT_TRUE(VM->registerModule("test", *Module));
  }
  EXPECT_FALSE(Destroyed);

  Module.reset();
  EXPECT_TRUE(Destroyed);
}

TEST_F(LazyJITTest, JITAddLookupFailure) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
  Conf.getCompilerConfigure().setOptimizationLevel(
      CompilerConfigure::OptimizationLevel::O1);

  Loader::Loader LoaderEngine(Conf);
  auto ModOrErr = LoaderEngine.parseWasmUnit(SimpleWasm);
  ASSERT_TRUE(ModOrErr);
  auto &Module = std::get<std::unique_ptr<AST::Module>>(*ModOrErr);

  Validator::Validator ValidatorEngine(Conf);
  auto ValRes = ValidatorEngine.validate(*Module);
  ASSERT_TRUE(ValRes);

  LLVM::Compiler Compiler(Conf);
  ASSERT_TRUE(Compiler.checkConfigure());

  auto CompileRes = Compiler.compileInfrastructure(*Module);
  ASSERT_TRUE(CompileRes);

  auto &LLData = *CompileRes;

  LLVM::JIT JIT(Conf);
  auto ExecRes = JIT.loadLazy(LLData);
  ASSERT_TRUE(ExecRes);

  auto JITLib = std::static_pointer_cast<LLVM::JITLibrary>(*ExecRes);

  std::vector<uint32_t> CompileLocals = {0};
  auto FuncCompileRes =
      Compiler.compileFunctions(std::move(LLData), *Module, CompileLocals);
  ASSERT_TRUE(FuncCompileRes);

  std::vector<uint32_t> InvalidIndices = {999};

  auto AddResult = JIT.add(*JITLib, *FuncCompileRes, InvalidIndices);
  EXPECT_FALSE(AddResult);
  EXPECT_EQ(AddResult.error(), ErrCode::Value::LazyCompilationError);
}

TEST_F(LazyJITTest, LazyJITIntrinsicsReachableFromCompiledCode) {
  // Batch modules only declare the "intrinsics" global; it resolves against
  // the infrastructure module's definition patched at prepare time. Run two
  // operations that call back into the runtime through that table — an
  // unresolved or null table pointer would crash instead.
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(IntrinsicsWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValVariant> GrowParams = {1U};
  std::vector<ValType> GrowTypes = {ValType(TypeCode::I32)};
  auto GrowRes = VM->execute("grow", GrowParams, GrowTypes);
  ASSERT_TRUE(GrowRes);
  // memory.grow returns the previous size in pages.
  EXPECT_EQ((*GrowRes)[0].first.get<uint32_t>(), 1U);

  auto TrapRes = VM->execute("trap");
  ASSERT_FALSE(TrapRes);
  EXPECT_EQ(TrapRes.error(), ErrCode::Value::Unreachable);

  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 2U);
}

TEST_F(LazyJITTest, LazyJITConcurrentSameFunction) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};
  std::vector<std::thread> Threads;
  const size_t NumThreads = 8;
  Threads.reserve(NumThreads);

  for (size_t Idx = 0; Idx < NumThreads; ++Idx) {
    Threads.emplace_back([&VM, &Types, Idx]() {
      std::vector<ValVariant> Params = {static_cast<uint32_t>(Idx), 10U};
      auto Result = VM->execute("add", Params, Types);
      EXPECT_TRUE(Result);
      if (Result) {
        EXPECT_EQ((*Result)[0].first.get<uint32_t>(),
                  static_cast<uint32_t>(Idx + 10));
      }
    });
  }

  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITConcurrentDifferentFunctions) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types2 = {ValType(TypeCode::I32),
                                 ValType(TypeCode::I32)};
  std::vector<ValType> Types0 = {};

  std::vector<std::thread> Threads;
  Threads.emplace_back([&VM, &Types2]() {
    std::vector<ValVariant> Params = {10U, 20U};
    auto Result = VM->execute("add", Params, Types2);
    EXPECT_TRUE(Result);
    if (Result) {
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 30U);
    }
  });
  Threads.emplace_back([&VM, &Types2]() {
    std::vector<ValVariant> Params = {5U, 6U};
    auto Result = VM->execute("mul", Params, Types2);
    EXPECT_TRUE(Result);
    if (Result) {
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 30U);
    }
  });
  Threads.emplace_back([&VM, &Types2]() {
    std::vector<ValVariant> Params = {50U, 20U};
    auto Result = VM->execute("sub", Params, Types2);
    EXPECT_TRUE(Result);
    if (Result) {
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 30U);
    }
  });
  Threads.emplace_back([&VM, &Types0]() {
    std::vector<ValVariant> Params = {};
    auto Result = VM->execute("const42", Params, Types0);
    EXPECT_TRUE(Result);
    if (Result) {
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 42U);
    }
  });

  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 4U);

  VM->cleanup();
}

TEST_F(LazyJITTest, LazyJITConcurrentFibonacci) {
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(FibonacciWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32)};
  std::vector<std::thread> Threads;
  const size_t NumThreads = 8;
  Threads.reserve(NumThreads);

  for (size_t Idx = 0; Idx < NumThreads; ++Idx) {
    Threads.emplace_back([&VM, &Types, Idx]() {
      std::vector<ValVariant> Params = {static_cast<uint32_t>(Idx)};
      auto Result = VM->execute("fib", Params, Types);
      EXPECT_TRUE(Result);
      if (Result) {
        uint32_t Expected = 0;
        if (Idx == 0 || Idx == 1) {
          Expected = 1;
        } else {
          uint32_t Prev2 = 1;
          uint32_t Prev1 = 1;
          for (size_t Jdx = 2; Jdx <= Idx; ++Jdx) {
            Expected = Prev2 + Prev1;
            Prev2 = Prev1;
            Prev1 = Expected;
          }
        }
        EXPECT_EQ((*Result)[0].first.get<uint32_t>(), Expected);
      }
    });
  }

  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_EQ(VM->getLazyCompiledFuncCount(), 1U);

  VM->cleanup();
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
