// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/runtime/MemInstanceTest.cpp -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests for MemoryInstance: page limits and memory.copy overlap handling.
///
//===----------------------------------------------------------------------===//

#include "common/configure.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "runtime/instance/memory.h"
#include "vm/vm.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace WasmEdge;

TEST(MemInstanceTest, LimitPages) {
  using MemInst = WasmEdge::Runtime::Instance::MemoryInstance;
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setMaxMemoryPage(256);

  MemInst Inst1(WasmEdge::AST::MemoryType(257),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst1.getDataPtr() == nullptr);
  EXPECT_EQ(Inst1.getPageSize(), 256U);

  MemInst Inst2(WasmEdge::AST::MemoryType(257));
  ASSERT_FALSE(Inst2.getDataPtr() == nullptr);

  MemInst Inst3(WasmEdge::AST::MemoryType(1),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst3.getDataPtr() == nullptr);
  ASSERT_FALSE(Inst3.growPage(256));
  ASSERT_TRUE(Inst3.growPage(255));

  MemInst Inst4(WasmEdge::AST::MemoryType(1));
  ASSERT_FALSE(Inst4.getDataPtr() == nullptr);
  ASSERT_TRUE(Inst4.growPage(256));

  MemInst Inst5(WasmEdge::AST::MemoryType(1, 128),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst5.getDataPtr() == nullptr);
  ASSERT_FALSE(Inst5.growPage(128));
  ASSERT_TRUE(Inst5.growPage(127));

  MemInst Inst6(WasmEdge::AST::MemoryType(1),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst6.growPage(0xFFFFFFFF));
}

// (module
//   (memory (export "mem") 1)
//   (func (export "copy") (param $dst i32) (param $src i32) (param $len i32)
//     local.get $dst
//     local.get $src
//     local.get $len
//     memory.copy)
//   (func (export "load8") (param $addr i32) (result i32)
//     local.get $addr
//     i32.load8_u)
//   (func (export "store8") (param $addr i32) (param $val i32)
//     local.get $addr
//     local.get $val
//     i32.store8))
const std::array<WasmEdge::Byte, 105> OverlapTestWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x11, 0x03, 0x60,
    0x03, 0x7f, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x02,
    0x7f, 0x7f, 0x00, 0x03, 0x04, 0x03, 0x00, 0x01, 0x02, 0x05, 0x03, 0x01,
    0x00, 0x01, 0x07, 0x1f, 0x04, 0x04, 0x63, 0x6f, 0x70, 0x79, 0x00, 0x00,
    0x05, 0x6c, 0x6f, 0x61, 0x64, 0x38, 0x00, 0x01, 0x06, 0x73, 0x74, 0x6f,
    0x72, 0x65, 0x38, 0x00, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x0a,
    0x20, 0x03, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfc, 0x0a,
    0x00, 0x00, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x2d, 0x00, 0x00, 0x0b, 0x09,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x3a, 0x00, 0x00, 0x0b};

const ValType I32Type = ValType(TypeCode::I32);

std::vector<uint8_t> readMemory(VM::VM &VM, uint32_t Start, uint32_t Len) {
  std::vector<uint8_t> Result;
  Result.reserve(Len);
  for (uint32_t I = 0; I < Len; ++I) {
    std::vector<ValVariant> Params = {Start + I};
    std::vector<ValType> ParamTypes = {I32Type};
    auto Res = VM.execute("load8", Params, ParamTypes);
    EXPECT_TRUE(Res) << "load8 failed at addr=" << Start + I;
    Result.push_back(static_cast<uint8_t>((*Res)[0].first.get<uint32_t>()));
  }
  return Result;
}

// dst > src with overlap: a forward-only copy would clobber unread source
// bytes.
TEST(MemInstanceTest, CopyForwardOverlap) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 8; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{3}, uint32_t{0}, uint32_t{5}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=3 src=0 len=5";

  std::vector<uint8_t> Expected = {0x41, 0x42, 0x43, 0x44, 0x45};
  auto Actual = readMemory(TestVM, 3, 5);
  EXPECT_EQ(Actual, Expected)
      << "Forward overlapping memory.copy produced incorrect results. "
         "This indicates that std::copy was used instead of std::memmove.";

  std::vector<uint8_t> Prefix = {0x41, 0x42, 0x43};
  auto ActualPrefix = readMemory(TestVM, 0, 3);
  EXPECT_EQ(ActualPrefix, Prefix) << "Source prefix should be unchanged.";
}

TEST(MemInstanceTest, CopyBackwardOverlap) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 8; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{0}, uint32_t{3}, uint32_t{5}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=0 src=3 len=5";

  std::vector<uint8_t> Expected = {0x44, 0x45, 0x46, 0x47, 0x48};
  auto Actual = readMemory(TestVM, 0, 5);
  EXPECT_EQ(Actual, Expected)
      << "Backward overlapping memory.copy produced incorrect results.";

  std::vector<uint8_t> Tail = {0x46, 0x47, 0x48};
  auto ActualTail = readMemory(TestVM, 5, 3);
  EXPECT_EQ(ActualTail, Tail) << "Trailing bytes should be unchanged.";
}

TEST(MemInstanceTest, CopyExactOverlap) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 8; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{2}, uint32_t{2}, uint32_t{4}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=2 src=2 len=4";

  std::vector<uint8_t> Expected = {0x41, 0x42, 0x43, 0x44,
                                   0x45, 0x46, 0x47, 0x48};
  auto Actual = readMemory(TestVM, 0, 8);
  EXPECT_EQ(Actual, Expected)
      << "Self-copy (dst == src) should leave memory unchanged.";
}

TEST(MemInstanceTest, CopyNonOverlapping) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 5; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }
  for (uint32_t I = 10; I < 15; ++I) {
    std::vector<ValVariant> Params = {I, uint32_t{0}};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{10}, uint32_t{0}, uint32_t{5}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=10 src=0 len=5";

  std::vector<uint8_t> Expected = {0x41, 0x42, 0x43, 0x44, 0x45};
  auto Actual = readMemory(TestVM, 10, 5);
  EXPECT_EQ(Actual, Expected) << "Non-overlapping copy should work correctly.";

  auto ActualSrc = readMemory(TestVM, 0, 5);
  EXPECT_EQ(ActualSrc, Expected) << "Source region should be unchanged.";
}

TEST(MemInstanceTest, CopyZeroLength) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 5; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{0}, uint32_t{2}, uint32_t{0}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=0 src=2 len=0";

  std::vector<uint8_t> Expected = {0x41, 0x42, 0x43, 0x44, 0x45};
  auto Actual = readMemory(TestVM, 0, 5);
  EXPECT_EQ(Actual, Expected) << "Zero-length copy should be a no-op.";
}

TEST(MemInstanceTest, CopyLargeForwardOverlap) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  const uint32_t Size = 256;
  for (uint32_t I = 0; I < Size; ++I) {
    std::vector<ValVariant> Params = {I, I & 0xFF};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{10}, uint32_t{0},
                                        uint32_t{200}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=10 src=0 len=200";

  for (uint32_t I = 0; I < 200; ++I) {
    uint8_t Expected = I & 0xFF;
    std::vector<ValVariant> LoadParams = {10 + I};
    std::vector<ValType> LoadParamTypes = {I32Type};
    auto Res = TestVM.execute("load8", LoadParams, LoadParamTypes);
    ASSERT_TRUE(Res) << "load8 failed at addr=" << 10 + I;
    uint8_t Actual = static_cast<uint8_t>((*Res)[0].first.get<uint32_t>());
    EXPECT_EQ(Actual, Expected)
        << "Large forward overlap mismatch at offset " << I;
  }
}

TEST(MemInstanceTest, CopyAdjacentRegions) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 5; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }
  for (uint32_t I = 5; I < 10; ++I) {
    std::vector<ValVariant> Params = {I, uint32_t{0}};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  std::vector<ValVariant> CopyParams = {uint32_t{5}, uint32_t{0}, uint32_t{5}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=5 src=0 len=5";

  std::vector<uint8_t> Expected = {0x41, 0x42, 0x43, 0x44, 0x45,
                                   0x41, 0x42, 0x43, 0x44, 0x45};
  auto Actual = readMemory(TestVM, 0, 10);
  EXPECT_EQ(Actual, Expected) << "Adjacent copy should duplicate the region.";
}

TEST(MemInstanceTest, CopySingleByteOverlap) {
  WasmEdge::Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(OverlapTestWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  for (uint32_t I = 0; I < 5; ++I) {
    std::vector<ValVariant> Params = {I, 0x41 + I};
    std::vector<ValType> ParamTypes = {I32Type, I32Type};
    ASSERT_TRUE(TestVM.execute("store8", Params, ParamTypes))
        << "store8 failed at addr=" << I;
  }

  // Source and destination share exactly one byte (address 4).
  std::vector<ValVariant> CopyParams = {uint32_t{4}, uint32_t{0}, uint32_t{5}};
  std::vector<ValType> CopyParamTypes = {I32Type, I32Type, I32Type};
  ASSERT_TRUE(TestVM.execute("copy", CopyParams, CopyParamTypes))
      << "memory.copy failed: dst=4 src=0 len=5";

  std::vector<uint8_t> Expected = {0x41, 0x42, 0x43, 0x44, 0x45};
  auto Actual = readMemory(TestVM, 4, 5);
  EXPECT_EQ(Actual, Expected)
      << "Single-byte overlap copy should produce correct results.";
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
