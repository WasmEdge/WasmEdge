// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "vm/vm.h"
#include "llvm/jit.h"
#include <gtest/gtest.h>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Simple module with 6 functions: add, mul, sub, const42, unused1, unused2
// Compiled from:
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

class LazyJITTest : public ::testing::Test {
protected:
  // Helper to create a VM with eager JIT
  std::unique_ptr<VM::VM> createEagerJITVM() {
    Configure Conf;
    Conf.getRuntimeConfigure().setEnableJIT(true);
    Conf.getRuntimeConfigure().setEnableLazyJIT(false);
    Conf.getCompilerConfigure().setOptimizationLevel(
        CompilerConfigure::OptimizationLevel::O0);
    return std::make_unique<VM::VM>(Conf);
  }

  // Helper to create a VM with lazy JIT
  std::unique_ptr<VM::VM> createLazyJITVM() {
    Configure Conf;
    Conf.getRuntimeConfigure().setEnableJIT(true);
    Conf.getRuntimeConfigure().setEnableLazyJIT(true);
    Conf.getCompilerConfigure().setOptimizationLevel(
        CompilerConfigure::OptimizationLevel::O0);
    return std::make_unique<VM::VM>(Conf);
  }

  // Helper to create a VM with interpreter mode
  std::unique_ptr<VM::VM> createInterpreterVM() {
    Configure Conf;
    Conf.getRuntimeConfigure().setEnableJIT(false);
    Conf.getRuntimeConfigure().setEnableLazyJIT(false);
    return std::make_unique<VM::VM>(Conf);
  }
};

TEST_F(LazyJITTest, ConfigurationDefaultDisabled) {
  Configure Conf;
  EXPECT_FALSE(Conf.getRuntimeConfigure().isEnableLazyJIT());
  EXPECT_FALSE(Conf.getRuntimeConfigure().isEnableJIT());
}

TEST_F(LazyJITTest, ConfigurationEnableDisable) {
  Configure Conf;

  // Enable lazy JIT
  Conf.getRuntimeConfigure().setEnableLazyJIT(true);
  EXPECT_TRUE(Conf.getRuntimeConfigure().isEnableLazyJIT());

  // Disable lazy JIT
  Conf.getRuntimeConfigure().setEnableLazyJIT(false);
  EXPECT_FALSE(Conf.getRuntimeConfigure().isEnableLazyJIT());
}

TEST_F(LazyJITTest, ConfigurationCopy) {
  Configure Conf1;
  Conf1.getRuntimeConfigure().setEnableLazyJIT(true);
  Conf1.getRuntimeConfigure().setEnableJIT(true);

  // Copy constructor
  Configure Conf2(Conf1);
  EXPECT_TRUE(Conf2.getRuntimeConfigure().isEnableLazyJIT());
  EXPECT_TRUE(Conf2.getRuntimeConfigure().isEnableJIT());

  // Modify original should not affect copy
  Conf1.getRuntimeConfigure().setEnableLazyJIT(false);
  EXPECT_TRUE(Conf2.getRuntimeConfigure().isEnableLazyJIT());
}

TEST_F(LazyJITTest, RuntimeConfigureIndependent) {
  RuntimeConfigure RConf1;
  RuntimeConfigure RConf2;

  RConf1.setEnableLazyJIT(true);
  EXPECT_TRUE(RConf1.isEnableLazyJIT());
  EXPECT_FALSE(RConf2.isEnableLazyJIT());
}

TEST_F(LazyJITTest, InterpreterModeBaseline) {
  auto VM = createInterpreterVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  // Test add function
  std::vector<ValVariant> AddParams = {10U, 5U};
  std::vector<ValType> AddTypes = {ValType(TypeCode::I32),
                                   ValType(TypeCode::I32)};
  auto AddResult = VM->execute("add", AddParams, AddTypes);
  ASSERT_TRUE(AddResult);
  ASSERT_EQ((*AddResult).size(), 1U);
  EXPECT_EQ((*AddResult)[0].first.get<uint32_t>(), 15U);

  // Test mul function
  std::vector<ValVariant> MulParams = {7U, 6U};
  auto MulResult = VM->execute("mul", MulParams, AddTypes);
  ASSERT_TRUE(MulResult);
  ASSERT_EQ((*MulResult).size(), 1U);
  EXPECT_EQ((*MulResult)[0].first.get<uint32_t>(), 42U);

  // Test const42 function
  std::vector<ValVariant> ConstParams = {};
  std::vector<ValType> ConstTypes = {};
  auto ConstResult = VM->execute("const42", ConstParams, ConstTypes);
  ASSERT_TRUE(ConstResult);
  ASSERT_EQ((*ConstResult).size(), 1U);
  EXPECT_EQ((*ConstResult)[0].first.get<uint32_t>(), 42U);

  VM->cleanup();
}

TEST_F(LazyJITTest, EagerJITBaseline) {
  auto VM = createEagerJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // Test add function
  std::vector<ValVariant> AddP = {10U, 5U};
  auto AddResult = VM->execute("add", AddP, Types);
  ASSERT_TRUE(AddResult);
  EXPECT_EQ((*AddResult)[0].first.get<uint32_t>(), 15U);

  // Test mul function
  std::vector<ValVariant> MulP = {7U, 6U};
  auto MulResult = VM->execute("mul", MulP, Types);
  ASSERT_TRUE(MulResult);
  EXPECT_EQ((*MulResult)[0].first.get<uint32_t>(), 42U);

  // Test sub function
  std::vector<ValVariant> SubP = {20U, 8U};
  auto SubResult = VM->execute("sub", SubP, Types);
  ASSERT_TRUE(SubResult);
  EXPECT_EQ((*SubResult)[0].first.get<uint32_t>(), 12U);

  VM->cleanup();
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

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // First call - triggers lazy compilation
  std::vector<ValVariant> P1 = {10U, 5U};
  auto Result1 = VM->execute("add", P1, Types);
  ASSERT_TRUE(Result1);
  EXPECT_EQ((*Result1)[0].first.get<uint32_t>(), 15U);

  // Second call - uses cached compiled code
  std::vector<ValVariant> P2 = {100U, 200U};
  auto Result2 = VM->execute("add", P2, Types);
  ASSERT_TRUE(Result2);
  EXPECT_EQ((*Result2)[0].first.get<uint32_t>(), 300U);

  // Third call - different values
  std::vector<ValVariant> P3 = {1U, 1U};
  auto Result3 = VM->execute("add", P3, Types);
  ASSERT_TRUE(Result3);
  EXPECT_EQ((*Result3)[0].first.get<uint32_t>(), 2U);

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

TEST_F(LazyJITTest, LazyJITOnlySomeFunctionsCalled) {
  // This test verifies that in lazy JIT mode, only called functions are
  // compiled
  auto VM = createLazyJITVM();

  ASSERT_TRUE(VM->loadWasm(SimpleWasm));
  ASSERT_TRUE(VM->validate());
  ASSERT_TRUE(VM->instantiate());

  std::vector<ValType> Types = {ValType(TypeCode::I32), ValType(TypeCode::I32)};

  // Call 'add' - should trigger compilation of function 0
  std::vector<ValVariant> AddP = {1U, 2U};
  auto AddResult = VM->execute("add", AddP, Types);
  ASSERT_TRUE(AddResult);
  EXPECT_EQ((*AddResult)[0].first.get<uint32_t>(), 3U);

  // Call 'mul' - should trigger compilation of function 1
  std::vector<ValVariant> MulP = {3U, 4U};
  auto MulResult = VM->execute("mul", MulP, Types);
  ASSERT_TRUE(MulResult);
  EXPECT_EQ((*MulResult)[0].first.get<uint32_t>(), 12U);

  // Call 'add' again - should NOT increase compiled count (already compiled)
  std::vector<ValVariant> AddP2 = {5U, 6U};
  auto AddResult2 = VM->execute("add", AddP2, Types);
  ASSERT_TRUE(AddResult2);
  EXPECT_EQ((*AddResult2)[0].first.get<uint32_t>(), 11U);

  VM->cleanup();
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
