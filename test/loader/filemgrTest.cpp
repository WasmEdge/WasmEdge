// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/filemgrTest.cpp - file manager unit tests ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of FileMgr interface.
///
//===----------------------------------------------------------------------===//

#include "loader/filemgr.h"

#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

WasmEdge::FileMgr Mgr;

TEST(FileManagerTest, File__SetPath) {
  // 1. Test opening data file.
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU32TestTooLong.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU32TestTooLarge.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU64Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU64TestTooLong.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU64TestTooLarge.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS32TestTooLong.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS32TestTooLarge.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS64Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS64TestTooLong.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS64TestTooLarge.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readF32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readF64Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readNameTest.bin"));
  EXPECT_FALSE(Mgr.setPath("filemgrTestData/NO_THIS_FILE.bin"));
  EXPECT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x00, 0xFF}));
}

TEST(FileManagerTest, File__ReadByte) {
  // 2. Test unsigned char reading.
  WasmEdge::Expect<uint8_t> ReadByte;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x00, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0xFF, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x1F, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x2E, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x3D, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x4C, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x5B, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x6A, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x79, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x88, ReadByte.value());
  ASSERT_FALSE(ReadByte = Mgr.readByte());
  EXPECT_EQ(10U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadBytes) {
  // 3. Test unsigned char list reading.
  WasmEdge::Expect<std::vector<uint8_t>> ReadBytes;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(1));
  EXPECT_EQ(0x00, ReadBytes.value()[0]);
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(2));
  EXPECT_EQ(0xFF, ReadBytes.value()[0]);
  EXPECT_EQ(0x1F, ReadBytes.value()[1]);
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(3));
  EXPECT_EQ(0x2E, ReadBytes.value()[0]);
  EXPECT_EQ(0x3D, ReadBytes.value()[1]);
  EXPECT_EQ(0x4C, ReadBytes.value()[2]);
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(4));
  EXPECT_EQ(0x5B, ReadBytes.value()[0]);
  EXPECT_EQ(0x6A, ReadBytes.value()[1]);
  EXPECT_EQ(0x79, ReadBytes.value()[2]);
  EXPECT_EQ(0x88, ReadBytes.value()[3]);
  ASSERT_FALSE(ReadBytes = Mgr.readBytes(1));
  EXPECT_EQ(10U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadUnsigned32) {
  // 4. Test unsigned 32bit integer decoding.
  WasmEdge::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU32Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(UINT32_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX) + UINT32_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(UINT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(165484164U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(134U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(3484157468U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(98765432U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(36U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadUnsigned64) {
  // 5. Test unsigned 64bit integer decoding.
  WasmEdge::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU64Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(UINT64_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX) + UINT64_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(UINT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(8234131023748ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(13139587396049293857ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(34841574681334ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(17234298579837453943ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(69U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadSigned32) {
  // 6. Test signed 32bit integer decoding.
  WasmEdge::Expect<int32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS32Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(INT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(INT32_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(-348415746, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(13018, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(-98765432, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(891055, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(30U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadSigned64) {
  // 7. Test signed 64bit integer decoding.
  WasmEdge::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS64Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(INT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(INT64_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(-3484157981297146LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(8124182798172984173LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(-9198734298341434797LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(7124932496753367824LL, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(63U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadFloat32) {
  // 8. Test Special Cases float.
  //
  //   1.  +0.0
  //   2.  -0.0
  //   3.  sqrt(-1) : NaN
  //   4.  log(-1) : NaN
  //   5.  0.0 / 0.0 : NaN
  //   6.  -0.0 / 0.0 : NaN
  //   7.  log(0) : +inf
  //   8.  1.0 / 0.0 : +inf
  //   9.  -1.0 / 0.0 : -inf
  WasmEdge::Expect<float> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readF32Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = Mgr.readF32());
  EXPECT_EQ(36U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadFloat64) {
  // 9. Test Special Cases double.
  //
  //   1.  +0.0
  //   2.  -0.0
  //   3.  sqrt(-1) : NaN
  //   4.  log(-1) : NaN
  //   5.  0.0 / 0.0 : NaN
  //   6.  -0.0 / 0.0 : NaN
  //   7.  log(0) : +inf
  //   8.  1.0 / 0.0 : +inf
  //   9.  -1.0 / 0.0 : -inf
  WasmEdge::Expect<double> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readF64Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = Mgr.readF64());
  EXPECT_EQ(72U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadName) {
  // 10. Test utf-8 string reading.
  WasmEdge::Expect<std::string> ReadStr;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readNameTest.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("test", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ(" ", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("Loader", ReadStr.value());
  ASSERT_FALSE(ReadStr = Mgr.readName());
  EXPECT_EQ(15U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadUnsigned32TooLong) {
  // 11. Test unsigned 32bit integer decoding in too long case.
  WasmEdge::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU32TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadUnsigned32TooLarge) {
  // 12. Test unsigned 32bit integer decoding in too large case.
  WasmEdge::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU32TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned32TooLong) {
  // 13. Test signed 32bit integer decoding in too long case.
  WasmEdge::Expect<int32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS32TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned32TooLarge) {
  // 14. Test signed 32bit integer decoding in too large case.
  WasmEdge::Expect<int32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS32TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__ReadUnsigned64TooLong) {
  // 15. Test unsigned 64bit integer decoding in too long case.
  WasmEdge::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU64TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadUnsigned64TooLarge) {
  // 16. Test unsigned 64bit integer decoding in too large case.
  WasmEdge::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU64TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned64TooLong) {
  // 17. Test signed 64bit integer decoding in too long case.
  WasmEdge::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS64TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned64TooLarge) {
  // 18. Test signed 64bit integer decoding in too large case.
  WasmEdge::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS64TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__PeekByte) {
  // 19. Test unsigned char peeking.
  WasmEdge::Expect<uint8_t> PeekByte;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x00, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0xFF, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x1F, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x2E, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x3D, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x4C, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x5B, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x6A, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x79, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x88, PeekByte.value());
  Mgr.readByte();
  ASSERT_FALSE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(10U, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadSigned33) {
  // 20. Test signed 33bit integer decoding.
  WasmEdge::Expect<int64_t> ReadNum;
  // Reuse the test data of reading S32
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS32Test.bin"));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(INT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(INT32_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(-348415746, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(13018, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(-98765432, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS33());
  EXPECT_EQ(891055, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readS33());
  EXPECT_EQ(30U, Mgr.getOffset());

  std::vector<uint8_t> TestData = {
      // First number.
      // The first 4 bytes are 0b11111111, which indicates 4*7=28 lowest bits
      // be 1.
      // The last byte is 0b00001111. The highest bit is 0, indicating that this
      // is the last byte. The fifth lowest bit is 0, indicating this number is
      // a positive number. Therefore, the sixth and seventh bit must also be 0.
      // The lowest 4 bits are all 1.
      // In total, the represented number is 2^32 - 1.
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0x0F,
      // Second number.
      // The first 4 bytes are 0b10000000, which indicates 4*7=28 lowest bits
      // be 0.
      // The last byte is 0b01110000. The highest bit is 0, indicating that this
      // is the last byte. The fifth lowest bit is 1, indicating this number is
      // a negative number. Therefore, the sixth and seventh bit must also be 1.
      // The lowest 4 bits are all 0.
      // In total, the represented number is 0b1 with 32 tailing zeros, which is
      // -2^32.
      0x80,
      0x80,
      0x80,
      0x80,
      0x70,
  };

  ASSERT_TRUE(Mgr.setCode(std::move(TestData)));
  ASSERT_EQ((1LL << 32) - 1, Mgr.readS33().value());
  ASSERT_EQ(5, Mgr.getOffset());
  ASSERT_EQ(-(1LL << 32), Mgr.readS33().value());
  ASSERT_EQ(10, Mgr.getOffset());
}

TEST(FileManagerTest, File__ReadSigned33TooLong) {
  // 21. Test signed 33bit integer decoding in too long case.
  WasmEdge::Expect<int64_t> ReadNum;
  // Reuse the test data of reading S32. Loading too long for S32 is the same as
  // S33, since both of them occupy at most 5 bytes.
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS32TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = Mgr.readS33());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned33TooLarge) {
  // 22. Test signed 33bit integer decoding in too large case.
  WasmEdge::Expect<int64_t> ReadNum;
  // The first 4 bytes starts with bit 1, which indicates there is a coming
  // fifth byte. The last byte is 0b00101111. The highest bit is 0, indicating
  // that this is the last byte. The fifth lowest bit is 0, indicating this
  // number is a positive number. Therefore, the sixth and seventh bit must also
  // be 0. However, the sixth lowest bit is 1, which will cause loading a too
  // large positive number.
  ASSERT_TRUE(
      Mgr.setCode(std::vector<uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0x1F})));
  ASSERT_FALSE(ReadNum = Mgr.readS33());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
  // The first 4 bytes starts with bit 1, which indicates there is a coming
  // fifth byte. The last byte is 0b01011111. The highest bit is 0, indicating
  // that this is the last byte. The fifth lowest bit is 1, indicating this
  // number is a negative number. Therefore, the sixth and seventh bit must also
  // be 1. However, the sixth lowest bit is 0, which will cause loading a too
  // large negative number.
  ASSERT_TRUE(
      Mgr.setCode(std::vector<uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0x5F})));
  ASSERT_FALSE(ReadNum = Mgr.readS33());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadByte) {
  // 1. Test unsigned char reading.
  WasmEdge::Expect<uint8_t> ReadByte;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x00, 0xFF, 0x1F, 0x2E, 0x3D,
                                               0x4C, 0x5B, 0x6A, 0x79, 0x88}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x00, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0xFF, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x1F, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x2E, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x3D, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x4C, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x5B, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x6A, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x79, ReadByte.value());
  ASSERT_TRUE(ReadByte = Mgr.readByte());
  EXPECT_EQ(0x88, ReadByte.value());
  ASSERT_FALSE(ReadByte = Mgr.readByte());
  EXPECT_EQ(10U, Mgr.getOffset());
  ASSERT_FALSE(ReadByte = Mgr.readByte());
  EXPECT_EQ(10U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadBytes) {
  // 2. Test unsigned char list reading.
  WasmEdge::Expect<std::vector<uint8_t>> ReadBytes;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x00, 0xFF, 0x1F, 0x2E, 0x3D,
                                               0x4C, 0x5B, 0x6A, 0x79, 0x88}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(1));
  EXPECT_EQ(0x00, ReadBytes.value()[0]);
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(2));
  EXPECT_EQ(0xFF, ReadBytes.value()[0]);
  EXPECT_EQ(0x1F, ReadBytes.value()[1]);
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(3));
  EXPECT_EQ(0x2E, ReadBytes.value()[0]);
  EXPECT_EQ(0x3D, ReadBytes.value()[1]);
  EXPECT_EQ(0x4C, ReadBytes.value()[2]);
  ASSERT_TRUE(ReadBytes = Mgr.readBytes(4));
  EXPECT_EQ(0x5B, ReadBytes.value()[0]);
  EXPECT_EQ(0x6A, ReadBytes.value()[1]);
  EXPECT_EQ(0x79, ReadBytes.value()[2]);
  EXPECT_EQ(0x88, ReadBytes.value()[3]);
  ASSERT_FALSE(ReadBytes = Mgr.readBytes(1));
  EXPECT_EQ(10U, Mgr.getOffset());
  ASSERT_FALSE(ReadBytes = Mgr.readBytes(1));
  EXPECT_EQ(10U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadUnsigned32) {
  // 3. Test unsigned 32bit integer decoding.
  WasmEdge::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x80, 0x80, 0x80, 0x08, 0xFF,
      0xFF, 0xFF, 0xFF, 0x0F, 0x84, 0xAD, 0xF4, 0x4E, 0x86, 0x01, 0x9C, 0x8C,
      0xB0, 0xFD, 0x0C, 0xDA, 0x65, 0xF8, 0x94, 0x8C, 0x2F, 0xAF, 0xB1, 0x36}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(UINT32_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX) + UINT32_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(UINT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(165484164U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(134U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(3484157468U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(98765432U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(36U, Mgr.getOffset());
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(36U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadUnsigned64) {
  // 4. Test unsigned 64bit integer decoding.
  WasmEdge::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x80, 0x80,
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x84, 0xCF, 0xD1, 0xC3, 0xD2, 0xEF,
      0x01, 0xA1, 0xA4, 0xDF, 0xA5, 0xEC, 0xA3, 0xCC, 0xAC, 0xB6, 0x01, 0xF6,
      0xD5, 0xBA, 0xFD, 0x82, 0xF6, 0x07, 0xDA, 0x65, 0xF7, 0x8C, 0xEC, 0xA1,
      0xF4, 0xE7, 0xA1, 0x96, 0xEF, 0x01, 0xAF, 0xB1, 0x36}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(UINT64_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX) + UINT64_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(UINT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(8234131023748ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(13139587396049293857ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(34841574681334ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(17234298579837453943ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(69U, Mgr.getOffset());
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(69U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadSigned32) {
  // 5. Test signed 32bit integer decoding.
  WasmEdge::Expect<int32_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x80, 0x80, 0x80,
      0x78, 0x7F, 0x01, 0x86, 0x01, 0xFE, 0xB1, 0xEE, 0xD9, 0x7E,
      0xDA, 0xE5, 0x00, 0x88, 0xEB, 0xF3, 0x50, 0xAF, 0xB1, 0x36}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(INT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(INT32_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(-348415746, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(13018, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(-98765432, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS32());
  EXPECT_EQ(891055, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(30U, Mgr.getOffset());
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(30U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadSigned64) {
  // 6. Test signed 64bit integer decoding.
  WasmEdge::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F, 0x7F,
      0x01, 0x86, 0x01, 0x86, 0xEC, 0xBB, 0x89, 0xD4, 0xE5, 0xE7, 0x79,
      0xED, 0xA6, 0xC2, 0xFB, 0xE0, 0xA6, 0xB9, 0xDF, 0xF0, 0x00, 0xD3,
      0xB4, 0xA0, 0xA1, 0xC8, 0xFC, 0xE1, 0xAB, 0x80, 0x7F, 0x90, 0xB6,
      0xFC, 0xAC, 0xB3, 0x8B, 0xB6, 0xF0, 0xE2, 0x00}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(INT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(INT64_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(-3484157981297146LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(8124182798172984173LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(-9198734298341434797LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readS64());
  EXPECT_EQ(7124932496753367824LL, ReadNum.value());
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(63U, Mgr.getOffset());
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(63U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadFloat32) {
  // 7. Test Special Cases float.
  //
  //   1.  +0.0
  //   2.  -0.0
  //   3.  sqrt(-1) : NaN
  //   4.  log(-1) : NaN
  //   5.  0.0 / 0.0 : NaN
  //   6.  -0.0 / 0.0 : NaN
  //   7.  log(0) : +inf
  //   8.  1.0 / 0.0 : +inf
  //   9.  -1.0 / 0.0 : -inf
  WasmEdge::Expect<float> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0xC0, 0xFF,
      0x00, 0x00, 0xC0, 0xFF, 0x00, 0x00, 0xC0, 0x7F, 0x00, 0x00, 0xC0, 0x7F,
      0x00, 0x00, 0x80, 0xFF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = Mgr.readF32());
  EXPECT_EQ(36U, Mgr.getOffset());
  ASSERT_FALSE(ReadNum = Mgr.readF32());
  EXPECT_EQ(36U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadFloat64) {
  // 8. Test Special Cases double.
  //
  //   1.  +0.0
  //   2.  -0.0
  //   3.  sqrt(-1) : NaN
  //   4.  log(-1) : NaN
  //   5.  0.0 / 0.0 : NaN
  //   6.  -0.0 / 0.0 : NaN
  //   7.  log(0) : +inf
  //   8.  1.0 / 0.0 : +inf
  //   9.  -1.0 / 0.0 : -inf
  WasmEdge::Expect<double> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xF8, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xF0, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = Mgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = Mgr.readF64());
  EXPECT_EQ(72U, Mgr.getOffset());
  ASSERT_FALSE(ReadNum = Mgr.readF64());
  EXPECT_EQ(72U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadName) {
  // 9. Test utf-8 string reading.
  WasmEdge::Expect<std::string> ReadStr;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x00, 0x04, 0x74, 0x65, 0x73,
                                               0x74, 0x01, 0x20, 0x06, 0x4C,
                                               0x6F, 0x61, 0x64, 0x65, 0x72}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("test", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ(" ", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("Loader", ReadStr.value());
  ASSERT_FALSE(ReadStr = Mgr.readName());
  EXPECT_EQ(15U, Mgr.getOffset());
  ASSERT_FALSE(ReadStr = Mgr.readName());
  EXPECT_EQ(15U, Mgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadUnsigned32TooLong) {
  // 10. Test unsigned 32bit integer decoding in too long case.
  WasmEdge::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(
      Mgr.setCode(std::vector<uint8_t>{0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadUnsigned32TooLarge) {
  // 11. Test unsigned 32bit integer decoding in too large case.
  WasmEdge::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x80, 0x80, 0x80, 0x80, 0x1F}));
  ASSERT_FALSE(ReadNum = Mgr.readU32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned32TooLong) {
  // 12. Test signed 32bit integer decoding in too long case.
  WasmEdge::Expect<int32_t> ReadNum;
  ASSERT_TRUE(
      Mgr.setCode(std::vector<uint8_t>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned32TooLarge) {
  // 13. Test signed 32bit integer decoding in too large case.
  WasmEdge::Expect<int32_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0xFF, 0xFF, 0xFF, 0xFF, 0x4F}));
  ASSERT_FALSE(ReadNum = Mgr.readS32());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadUnsigned64TooLong) {
  // 14. Test unsigned 64bit integer decoding in too long case.
  WasmEdge::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadUnsigned64TooLarge) {
  // 15. Test unsigned 64bit integer decoding in too large case.
  WasmEdge::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x80, 0x80, 0x80, 0x80, 0x80,
                                               0x80, 0x80, 0x80, 0x80, 0x7E}));
  ASSERT_FALSE(ReadNum = Mgr.readU64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned64TooLong) {
  // 16. Test signed 64bit integer decoding in too long case.
  WasmEdge::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned64TooLarge) {
  // 17. Test signed 64bit integer decoding in too large case.
  WasmEdge::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                               0xFF, 0xFF, 0xFF, 0xFF, 0x41}));
  ASSERT_FALSE(ReadNum = Mgr.readS64());
  EXPECT_EQ(WasmEdge::ErrCode::Value::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__PeekByte) {
  // 18. Test unsigned char peeking.
  WasmEdge::Expect<uint8_t> PeekByte;
  ASSERT_TRUE(Mgr.setCode(std::vector<uint8_t>{0x00, 0xFF, 0x1F, 0x2E, 0x3D,
                                               0x4C, 0x5B, 0x6A, 0x79, 0x88}));
  EXPECT_EQ(0U, Mgr.getOffset());
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x00, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0xFF, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x1F, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x2E, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x3D, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x4C, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x5B, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x6A, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x79, PeekByte.value());
  Mgr.readByte();
  ASSERT_TRUE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(0x88, PeekByte.value());
  Mgr.readByte();
  ASSERT_FALSE(PeekByte = Mgr.peekByte());
  EXPECT_EQ(10U, Mgr.getOffset());
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
