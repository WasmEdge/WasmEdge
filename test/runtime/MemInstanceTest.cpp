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
#include "system/allocator.h"
#include "vm/vm.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

TEST(MemInstanceTest, ShrinkAndRegrow) {
  // Test shrinking and growing a memory again.
  //
  //   1.  Through the allocator.
  //   2.  Through the memory instance.

  // Through the allocator.
  {
    constexpr uint64_t PageSize = Runtime::Instance::MemoryInstance::kPageSize;
    auto *Pointer = Allocator::allocate(3);
    ASSERT_NE(Pointer, nullptr);
    std::fill_n(Pointer, 3 * PageSize, 0xA5);
    auto *Smaller = Allocator::shrink(Pointer, 3, 1);
    ASSERT_NE(Smaller, nullptr);
    EXPECT_TRUE(std::all_of(Smaller, Smaller + PageSize,
                            [](Byte B) { return B == 0xA5; }));
    Pointer = Allocator::resize(Smaller, 1, 3);
    ASSERT_NE(Pointer, nullptr);
    EXPECT_TRUE(std::all_of(Pointer + PageSize, Pointer + 3 * PageSize,
                            [](Byte B) { return B == 0; }));
    Smaller = Allocator::shrink(Pointer, 3, 0);
    ASSERT_NE(Smaller, nullptr);
    Pointer = Allocator::resize(Smaller, 0, 1);
    ASSERT_NE(Pointer, nullptr);
    EXPECT_TRUE(std::all_of(Pointer, Pointer + PageSize,
                            [](Byte B) { return B == 0; }));
    Allocator::release(Pointer, 1);
  }

  // Through the memory instance.
  {
    Runtime::Instance::MemoryInstance Mem(AST::MemoryType(1, 3));
    const auto *SizePtr = Mem.getPageSizePtr();
    auto **DataPtr = &Mem.getDataPtr();
    ASSERT_TRUE(Mem.growPage(2));
    ASSERT_TRUE(Mem.fillBytes(0xA5, 0, Mem.getSize()));
    EXPECT_FALSE(Mem.shrinkPage(4));
    ASSERT_TRUE(Mem.shrinkPage(2));
    EXPECT_EQ(Mem.getPageSize(), 1U);
    EXPECT_EQ(*SizePtr, 1U);
    EXPECT_EQ(DataPtr, &Mem.getDataPtr());
    ASSERT_TRUE(Mem.growPage(2));
    const auto Bytes = Mem.getBytes(Mem.kPageSize, 2 * Mem.kPageSize);
    ASSERT_TRUE(Bytes);
    EXPECT_TRUE(std::all_of(Bytes->begin(), Bytes->end(),
                            [](Byte B) { return B == 0; }));
    ASSERT_TRUE(Mem.shrinkPage(3));
    EXPECT_EQ(*SizePtr, 0U);
    ASSERT_TRUE(Mem.growPage(1));
    EXPECT_EQ(*SizePtr, 1U);
  }
}

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

// memory.copy must behave as memmove for overlapping, adjacent, disjoint, and
// empty ranges; a forward-only copy would clobber unread source bytes.
TEST(MemInstanceTest, CopyOverlap) {
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
  std::array<WasmEdge::Byte, 105> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x11, 0x03, 0x60,
      0x03, 0x7f, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x02,
      0x7f, 0x7f, 0x00, 0x03, 0x04, 0x03, 0x00, 0x01, 0x02, 0x05, 0x03, 0x01,
      0x00, 0x01, 0x07, 0x1f, 0x04, 0x04, 0x63, 0x6f, 0x70, 0x79, 0x00, 0x00,
      0x05, 0x6c, 0x6f, 0x61, 0x64, 0x38, 0x00, 0x01, 0x06, 0x73, 0x74, 0x6f,
      0x72, 0x65, 0x38, 0x00, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x0a,
      0x20, 0x03, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfc, 0x0a,
      0x00, 0x00, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x2d, 0x00, 0x00, 0x0b, 0x09,
      0x00, 0x20, 0x00, 0x20, 0x01, 0x3a, 0x00, 0x00, 0x0b};

  // Each case fills [0, Fill) with 'A' + address, copies Len bytes from Src to
  // Dst, and expects Expected in the memory from address 0.
  struct Case {
    std::string_view Name;
    uint32_t Fill;
    uint32_t Dst;
    uint32_t Src;
    uint32_t Len;
    std::string Expected;
  };
  std::vector<Case> Cases{
      {"forward overlap"sv, 8, 3, 0, 5, "ABCABCDE"s},
      {"backward overlap"sv, 8, 0, 3, 5, "DEFGHFGH"s},
      {"exact overlap"sv, 8, 2, 2, 4, "ABCDEFGH"s},
      {"non-overlapping"sv, 5, 10, 0, 5, "ABCDE\0\0\0\0\0ABCDE"s},
      {"zero length"sv, 5, 0, 2, 0, "ABCDE"s},
      {"adjacent regions"sv, 5, 5, 0, 5, "ABCDEABCDE"s},
      {"single byte overlap"sv, 5, 4, 0, 5, "ABCDABCDE"s},
  };
  // The large case moves 200 of 256 filled bytes forward by 10.
  Case Large{"large forward overlap"sv, 256, 10, 0, 200, {}};
  for (uint32_t I = 0; I < 256; ++I) {
    Large.Expected.push_back(
        static_cast<char>('A' + (I >= 10 && I < 210 ? I - 10 : I)));
  }
  Cases.push_back(std::move(Large));

  const ValType I32(TypeCode::I32);
  for (const auto &C : Cases) {
    SCOPED_TRACE(C.Name);
    Configure Conf;
    VM::VM TestVM(Conf);
    ASSERT_TRUE(TestVM.loadWasm(Wasm));
    ASSERT_TRUE(TestVM.validate());
    ASSERT_TRUE(TestVM.instantiate());
    for (uint32_t I = 0; I < C.Fill; ++I) {
      ASSERT_TRUE(TestVM.execute("store8",
                                 {ValVariant(I), ValVariant(uint32_t('A') + I)},
                                 {I32, I32}));
    }
    ASSERT_TRUE(TestVM.execute(
        "copy", {ValVariant(C.Dst), ValVariant(C.Src), ValVariant(C.Len)},
        {I32, I32, I32}));
    std::string Actual;
    for (uint32_t I = 0; I < C.Expected.size(); ++I) {
      auto Res = TestVM.execute("load8", {ValVariant(I)}, {I32});
      ASSERT_TRUE(Res);
      Actual.push_back(static_cast<char>((*Res)[0].first.get<uint32_t>()));
    }
    EXPECT_EQ(Actual, C.Expected);
  }
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
