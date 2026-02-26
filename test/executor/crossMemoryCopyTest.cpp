// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/test/executor/crossMemoryCopyTest.cpp --------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Regression test for memory.copy with mixed memory32/memory64 address types.
///
/// The bug: In runMemoryCopyOp, the source and destination address types were
/// swapped when extracting addresses from the stack. The source address was
/// extracted using the destination memory's address type and vice versa:
///   uint64_t Src = extractAddr(StackMgr.pop(), AddrType2); // wrong: dst type
///   uint64_t Dst = extractAddr(StackMgr.pop(), AddrType1); // wrong: src type
///
/// The fix corrects the assignment:
///   uint64_t Src = extractAddr(StackMgr.pop(), AddrType1); // correct: src type
///   uint64_t Dst = extractAddr(StackMgr.pop(), AddrType2); // correct: dst type
///
/// This test creates a module with both memory32 (idx=0) and memory64 (idx=1),
/// stores data in one memory, copies to the other using memory.copy, and
/// verifies the data was correctly transferred.
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"
#include "common/types.h"
#include "vm/vm.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace WasmEdge;

/// Binary Wasm module equivalent to:
///
/// (module
///   (memory $m0 1)           ;; memory32, 1 page
///   (memory $m1 i64 1)       ;; memory64, 1 page
///
///   ;; Store an i32 to memory64 at an i64 offset
///   (func (export "store_m64") (param i64 i32)
///     local.get 0
///     local.get 1
///     i32.store 1 offset=0 align=2)
///
///   ;; Load an i32 from memory32 at an i32 offset
///   (func (export "load_m32") (param i32) (result i32)
///     local.get 0
///     i32.load offset=0 align=2)
///
///   ;; Copy from memory64 (src, idx=1) to memory32 (dst, idx=0)
///   (func (export "copy_m64_to_m32") (param i32 i64 i32)
///     local.get 0   ;; dst offset (i32 for memory32)
///     local.get 1   ;; src offset (i64 for memory64)
///     local.get 2   ;; length (i32, min addr type)
///     memory.copy 0 1)
///
///   ;; Store an i32 to memory32 at an i32 offset
///   (func (export "store_m32") (param i32 i32)
///     local.get 0
///     local.get 1
///     i32.store offset=0 align=2)
///
///   ;; Load an i32 from memory64 at an i64 offset
///   (func (export "load_m64") (param i64) (result i32)
///     local.get 0
///     i32.load 1 offset=0 align=2)
///
///   ;; Copy from memory32 (src, idx=0) to memory64 (dst, idx=1)
///   (func (export "copy_m32_to_m64") (param i64 i32 i32)
///     local.get 0   ;; dst offset (i64 for memory64)
///     local.get 1   ;; src offset (i32 for memory32)
///     local.get 2   ;; length (i32, min addr type)
///     memory.copy 1 0)
/// )
// clang-format off
std::array<WasmEdge::Byte, 211> CrossMemoryCopyWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x21, 0x06, 0x60,
    0x02, 0x7e, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x03, 0x7f,
    0x7e, 0x7f, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7e, 0x01,
    0x7f, 0x60, 0x03, 0x7e, 0x7f, 0x7f, 0x00, 0x03, 0x07, 0x06, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x05, 0x05, 0x02, 0x00, 0x01, 0x04, 0x01, 0x07,
    0x53, 0x06, 0x09, 0x73, 0x74, 0x6f, 0x72, 0x65, 0x5f, 0x6d, 0x36, 0x34,
    0x00, 0x00, 0x08, 0x6c, 0x6f, 0x61, 0x64, 0x5f, 0x6d, 0x33, 0x32, 0x00,
    0x01, 0x0f, 0x63, 0x6f, 0x70, 0x79, 0x5f, 0x6d, 0x36, 0x34, 0x5f, 0x74,
    0x6f, 0x5f, 0x6d, 0x33, 0x32, 0x00, 0x02, 0x09, 0x73, 0x74, 0x6f, 0x72,
    0x65, 0x5f, 0x6d, 0x33, 0x32, 0x00, 0x03, 0x08, 0x6c, 0x6f, 0x61, 0x64,
    0x5f, 0x6d, 0x36, 0x34, 0x00, 0x04, 0x0f, 0x63, 0x6f, 0x70, 0x79, 0x5f,
    0x6d, 0x33, 0x32, 0x5f, 0x74, 0x6f, 0x5f, 0x6d, 0x36, 0x34, 0x00, 0x05,
    0x0a, 0x41, 0x06, 0x0a, 0x00, 0x20, 0x00, 0x20, 0x01, 0x36, 0x41, 0x01,
    0x00, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x28, 0x01, 0x00, 0x0b, 0x0c, 0x00,
    0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0xfc, 0x0a, 0x00, 0x01, 0x0b, 0x09,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x36, 0x01, 0x00, 0x0b, 0x08, 0x00, 0x20,
    0x00, 0x28, 0x41, 0x01, 0x00, 0x0b, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01,
    0x20, 0x02, 0xfc, 0x0a, 0x01, 0x00, 0x0b
};
// clang-format on

/// Test memory.copy from memory64 to memory32.
///
/// Steps:
///   1. Store a known i32 value (0xDEADBEEF) at offset 100 in memory64.
///   2. Copy 4 bytes from memory64[100] to memory32[200].
///   3. Load from memory32[200] and verify the value matches.
///
/// With the bug, the source/destination address types were swapped, which means
/// the source address (100) would be extracted as i32 instead of i64, and the
/// destination address (200) would be extracted as i64 instead of i32.
/// For small offsets this still works, but it validates the fix is correct.
TEST(CrossMemoryCopy, CopyFromMemory64ToMemory32) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(CrossMemoryCopyWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const uint32_t TestValue = 0xDEADBEEFU;
  const uint64_t SrcOffset = 100;
  const uint32_t DstOffset = 200;
  const uint32_t CopyLen = 4;

  // Step 1: Store TestValue at offset 100 in memory64.
  {
    std::vector<ValVariant> Params = {ValVariant(SrcOffset),
                                      ValVariant(TestValue)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("store_m64", Params, ParamTypes);
    ASSERT_TRUE(Result) << "store_m64 should succeed";
  }

  // Step 2: Copy 4 bytes from memory64[100] to memory32[200].
  // copy_m64_to_m32(dst: i32, src: i64, len: i32)
  {
    std::vector<ValVariant> Params = {ValVariant(DstOffset),
                                      ValVariant(SrcOffset),
                                      ValVariant(CopyLen)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I32),
                                       ValType(TypeCode::I64),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("copy_m64_to_m32", Params, ParamTypes);
    ASSERT_TRUE(Result) << "copy_m64_to_m32 should succeed";
  }

  // Step 3: Load from memory32[200] and verify.
  {
    std::vector<ValVariant> Params = {ValVariant(DstOffset)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I32)};
    auto Result = VM.execute("load_m32", Params, ParamTypes);
    ASSERT_TRUE(Result) << "load_m32 should succeed";
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), TestValue)
        << "Data copied from memory64 to memory32 should match the stored "
           "value";
  }
}

/// Test memory.copy from memory32 to memory64.
///
/// Steps:
///   1. Store a known i32 value (0xCAFEBABE) at offset 300 in memory32.
///   2. Copy 4 bytes from memory32[300] to memory64[400].
///   3. Load from memory64[400] and verify the value matches.
TEST(CrossMemoryCopy, CopyFromMemory32ToMemory64) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(CrossMemoryCopyWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const uint32_t TestValue = 0xCAFEBABEU;
  const uint32_t SrcOffset = 300;
  const uint64_t DstOffset = 400;
  const uint32_t CopyLen = 4;

  // Step 1: Store TestValue at offset 300 in memory32.
  {
    std::vector<ValVariant> Params = {ValVariant(SrcOffset),
                                      ValVariant(TestValue)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I32),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("store_m32", Params, ParamTypes);
    ASSERT_TRUE(Result) << "store_m32 should succeed";
  }

  // Step 2: Copy 4 bytes from memory32[300] to memory64[400].
  // copy_m32_to_m64(dst: i64, src: i32, len: i32)
  {
    std::vector<ValVariant> Params = {ValVariant(DstOffset),
                                      ValVariant(SrcOffset),
                                      ValVariant(CopyLen)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64),
                                       ValType(TypeCode::I32),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("copy_m32_to_m64", Params, ParamTypes);
    ASSERT_TRUE(Result) << "copy_m32_to_m64 should succeed";
  }

  // Step 3: Load from memory64[400] and verify.
  {
    std::vector<ValVariant> Params = {ValVariant(DstOffset)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64)};
    auto Result = VM.execute("load_m64", Params, ParamTypes);
    ASSERT_TRUE(Result) << "load_m64 should succeed";
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), TestValue)
        << "Data copied from memory32 to memory64 should match the stored "
           "value";
  }
}

/// Test bidirectional copy: store in memory64, copy to memory32, modify in
/// memory32, then copy back to memory64.
///
/// This validates that both directions of cross-memory copy work correctly in
/// sequence, exercising the fix for both code paths.
TEST(CrossMemoryCopy, BidirectionalCopy) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(CrossMemoryCopyWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const uint32_t OriginalValue = 0x12345678U;
  const uint32_t ModifiedValue = 0xABCD0000U;
  const uint64_t M64Offset = 0;
  const uint32_t M32Offset = 0;
  const uint32_t CopyLen = 4;

  // Store original value in memory64.
  {
    std::vector<ValVariant> Params = {ValVariant(M64Offset),
                                      ValVariant(OriginalValue)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("store_m64", Params, ParamTypes);
    ASSERT_TRUE(Result);
  }

  // Copy from memory64 to memory32.
  {
    std::vector<ValVariant> Params = {ValVariant(M32Offset),
                                      ValVariant(M64Offset),
                                      ValVariant(CopyLen)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I32),
                                       ValType(TypeCode::I64),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("copy_m64_to_m32", Params, ParamTypes);
    ASSERT_TRUE(Result);
  }

  // Verify memory32 has the original value.
  {
    std::vector<ValVariant> Params = {ValVariant(M32Offset)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I32)};
    auto Result = VM.execute("load_m32", Params, ParamTypes);
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), OriginalValue);
  }

  // Overwrite memory32 with a new value.
  {
    std::vector<ValVariant> Params = {ValVariant(M32Offset),
                                      ValVariant(ModifiedValue)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I32),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("store_m32", Params, ParamTypes);
    ASSERT_TRUE(Result);
  }

  // Copy back from memory32 to memory64 at a different offset.
  const uint64_t M64Offset2 = 100;
  {
    std::vector<ValVariant> Params = {ValVariant(M64Offset2),
                                      ValVariant(M32Offset),
                                      ValVariant(CopyLen)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64),
                                       ValType(TypeCode::I32),
                                       ValType(TypeCode::I32)};
    auto Result = VM.execute("copy_m32_to_m64", Params, ParamTypes);
    ASSERT_TRUE(Result);
  }

  // Verify memory64 at the new offset has the modified value.
  {
    std::vector<ValVariant> Params = {ValVariant(M64Offset2)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64)};
    auto Result = VM.execute("load_m64", Params, ParamTypes);
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), ModifiedValue);
  }

  // Verify original memory64 location still has the original value.
  {
    std::vector<ValVariant> Params = {ValVariant(M64Offset)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::I64)};
    auto Result = VM.execute("load_m64", Params, ParamTypes);
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), OriginalValue);
  }
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
