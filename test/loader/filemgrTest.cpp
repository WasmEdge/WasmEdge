// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/loader/filemgrTest.cpp - file manager unit tests --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of FileMgr interface.
///
//===----------------------------------------------------------------------===//

#include "loader/filemgr.h"
#include "common/errcode.h"
#include "gtest/gtest.h"

#include <cmath>
#include <limits>

namespace {

SSVM::FileMgrFStream FMgr;
SSVM::FileMgrVector VMgr;

TEST(FileManagerTest, File__SetPath) {
  /// 1. Test opening data file.
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readU32Test.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readU32TestTooLong.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readU32TestTooLarge.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readU64Test.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readU64TestTooLong.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readU64TestTooLarge.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readS32Test.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readS32TestTooLong.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readS32TestTooLarge.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readS64Test.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readS64TestTooLong.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readS64TestTooLarge.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readF32Test.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readF64Test.bin"));
  EXPECT_TRUE(FMgr.setPath("filemgrTestData/readNameTest.bin"));
  EXPECT_FALSE(FMgr.setPath("filemgrTestData/NO_THIS_FILE.bin"));
  EXPECT_FALSE(FMgr.setCode(std::array<uint8_t, 2>{0x00, 0xFF}));
  EXPECT_FALSE(VMgr.setPath("filemgrTestData/readByteTest.bin"));
}

TEST(FileManagerTest, File__ReadByte) {
  /// 2. Test unsigned char reading.
  SSVM::Expect<uint8_t> ReadByte;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x00, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0xFF, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x1F, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x2E, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x3D, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x4C, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x5B, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x6A, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x79, ReadByte.value());
  ASSERT_TRUE(ReadByte = FMgr.readByte());
  EXPECT_EQ(0x88, ReadByte.value());
  ASSERT_FALSE(ReadByte = FMgr.readByte());
  EXPECT_EQ(10U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadBytes) {
  /// 3. Test unsigned char list reading.
  SSVM::Expect<std::vector<uint8_t>> ReadBytes;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadBytes = FMgr.readBytes(1));
  EXPECT_EQ(0x00, ReadBytes.value()[0]);
  ASSERT_TRUE(ReadBytes = FMgr.readBytes(2));
  EXPECT_EQ(0xFF, ReadBytes.value()[0]);
  EXPECT_EQ(0x1F, ReadBytes.value()[1]);
  ASSERT_TRUE(ReadBytes = FMgr.readBytes(3));
  EXPECT_EQ(0x2E, ReadBytes.value()[0]);
  EXPECT_EQ(0x3D, ReadBytes.value()[1]);
  EXPECT_EQ(0x4C, ReadBytes.value()[2]);
  ASSERT_TRUE(ReadBytes = FMgr.readBytes(4));
  EXPECT_EQ(0x5B, ReadBytes.value()[0]);
  EXPECT_EQ(0x6A, ReadBytes.value()[1]);
  EXPECT_EQ(0x79, ReadBytes.value()[2]);
  EXPECT_EQ(0x88, ReadBytes.value()[3]);
  ASSERT_FALSE(ReadBytes = FMgr.readBytes(1));
  EXPECT_EQ(10U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadUnsigned32) {
  /// 4. Test unsigned 32bit integer decoding.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readU32Test.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(UINT32_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX) + UINT32_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(UINT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(165484164U, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(134U, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(3484157468U, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(98765432U, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU32());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = FMgr.readU32());
  EXPECT_EQ(36U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadUnsigned64) {
  /// 5. Test unsigned 64bit integer decoding.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readU64Test.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(UINT64_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX) + UINT64_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(UINT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(8234131023748ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(13139587396049293857ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(34841574681334ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(17234298579837453943ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readU64());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = FMgr.readU64());
  EXPECT_EQ(69U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadSigned32) {
  /// 6. Test signed 32bit integer decoding.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readS32Test.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(INT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(INT32_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(-348415746, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(13018, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(-98765432, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS32());
  EXPECT_EQ(891055, ReadNum.value());
  ASSERT_FALSE(ReadNum = FMgr.readS32());
  EXPECT_EQ(30U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadSigned64) {
  /// 7. Test signed 64bit integer decoding.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readS64Test.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(INT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(INT64_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(-3484157981297146LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(8124182798172984173LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(-9198734298341434797LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readS64());
  EXPECT_EQ(7124932496753367824LL, ReadNum.value());
  ASSERT_FALSE(ReadNum = FMgr.readS64());
  EXPECT_EQ(63U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadFloat32) {
  /// 8. Test Special Cases float.
  ///
  ///   1.  +0.0
  ///   2.  -0.0
  ///   3.  sqrt(-1) : NaN
  ///   4.  log(-1) : NaN
  ///   5.  0.0 / 0.0 : NaN
  ///   6.  -0.0 / 0.0 : NaN
  ///   7.  log(0) : +inf
  ///   8.  1.0 / 0.0 : +inf
  ///   9.  -1.0 / 0.0 : -inf
  SSVM::Expect<float> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readF32Test.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = FMgr.readF32());
  EXPECT_EQ(36U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadFloat64) {
  /// 9. Test Special Cases double.
  ///
  ///   1.  +0.0
  ///   2.  -0.0
  ///   3.  sqrt(-1) : NaN
  ///   4.  log(-1) : NaN
  ///   5.  0.0 / 0.0 : NaN
  ///   6.  -0.0 / 0.0 : NaN
  ///   7.  log(0) : +inf
  ///   8.  1.0 / 0.0 : +inf
  ///   9.  -1.0 / 0.0 : -inf
  SSVM::Expect<double> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readF64Test.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = FMgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = FMgr.readF64());
  EXPECT_EQ(72U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadName) {
  /// 10. Test utf-8 string reading.
  SSVM::Expect<std::string> ReadStr;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readNameTest.bin"));
  EXPECT_EQ(0U, FMgr.getOffset());
  ASSERT_TRUE(ReadStr = FMgr.readName());
  EXPECT_EQ("", ReadStr.value());
  ASSERT_TRUE(ReadStr = FMgr.readName());
  EXPECT_EQ("test", ReadStr.value());
  ASSERT_TRUE(ReadStr = FMgr.readName());
  EXPECT_EQ(" ", ReadStr.value());
  ASSERT_TRUE(ReadStr = FMgr.readName());
  EXPECT_EQ("Loader", ReadStr.value());
  ASSERT_FALSE(ReadStr = FMgr.readName());
  EXPECT_EQ(15U, FMgr.getOffset());
}

TEST(FileManagerTest, File__ReadUnsigned32TooLong) {
  /// 11. Test unsigned 32bit integer decoding in too long case.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readU32TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readU32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadUnsigned32TooLarge) {
  /// 12. Test unsigned 32bit integer decoding in too large case.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readU32TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readU32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned32TooLong) {
  /// 13. Test signed 32bit integer decoding in too long case.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readS32TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readS32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned32TooLarge) {
  /// 14. Test signed 32bit integer decoding in too large case.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readS32TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readS32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__ReadUnsigned64TooLong) {
  /// 15. Test unsigned 64bit integer decoding in too long case.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readU64TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readU64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadUnsigned64TooLarge) {
  /// 16. Test unsigned 64bit integer decoding in too large case.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readU64TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readU64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned64TooLong) {
  /// 17. Test signed 64bit integer decoding in too long case.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readS64TestTooLong.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readS64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, File__ReadSigned64TooLarge) {
  /// 18. Test signed 64bit integer decoding in too large case.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(FMgr.setPath("filemgrTestData/readS64TestTooLarge.bin"));
  ASSERT_FALSE(ReadNum = FMgr.readS64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadByte) {
  /// 19. Test unsigned char reading.
  SSVM::Expect<uint8_t> ReadByte;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 10>{
      0x00, 0xFF, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79, 0x88}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x00, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0xFF, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x1F, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x2E, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x3D, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x4C, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x5B, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x6A, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x79, ReadByte.value());
  ASSERT_TRUE(ReadByte = VMgr.readByte());
  EXPECT_EQ(0x88, ReadByte.value());
  ASSERT_FALSE(ReadByte = VMgr.readByte());
  EXPECT_EQ(10U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadBytes) {
  /// 20. Test unsigned char list reading.
  SSVM::Expect<std::vector<uint8_t>> ReadBytes;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 10>{
      0x00, 0xFF, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79, 0x88}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadBytes = VMgr.readBytes(1));
  EXPECT_EQ(0x00, ReadBytes.value()[0]);
  ASSERT_TRUE(ReadBytes = VMgr.readBytes(2));
  EXPECT_EQ(0xFF, ReadBytes.value()[0]);
  EXPECT_EQ(0x1F, ReadBytes.value()[1]);
  ASSERT_TRUE(ReadBytes = VMgr.readBytes(3));
  EXPECT_EQ(0x2E, ReadBytes.value()[0]);
  EXPECT_EQ(0x3D, ReadBytes.value()[1]);
  EXPECT_EQ(0x4C, ReadBytes.value()[2]);
  ASSERT_TRUE(ReadBytes = VMgr.readBytes(4));
  EXPECT_EQ(0x5B, ReadBytes.value()[0]);
  EXPECT_EQ(0x6A, ReadBytes.value()[1]);
  EXPECT_EQ(0x79, ReadBytes.value()[2]);
  EXPECT_EQ(0x88, ReadBytes.value()[3]);
  ASSERT_FALSE(ReadBytes = VMgr.readBytes(1));
  EXPECT_EQ(10U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadUnsigned32) {
  /// 21. Test unsigned 32bit integer decoding.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 36>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x80, 0x80, 0x80, 0x08, 0xFF,
      0xFF, 0xFF, 0xFF, 0x0F, 0x84, 0xAD, 0xF4, 0x4E, 0x86, 0x01, 0x9C, 0x8C,
      0xB0, 0xFD, 0x0C, 0xDA, 0x65, 0xF8, 0x94, 0x8C, 0x2F, 0xAF, 0xB1, 0x36}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(UINT32_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(uint32_t(INT32_MAX) + UINT32_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(UINT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(165484164U, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(134U, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(3484157468U, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(98765432U, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU32());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = VMgr.readU32());
  EXPECT_EQ(36U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadUnsigned64) {
  /// 22. Test unsigned 64bit integer decoding.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 69>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x80, 0x80,
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x84, 0xCF, 0xD1, 0xC3, 0xD2, 0xEF,
      0x01, 0xA1, 0xA4, 0xDF, 0xA5, 0xEC, 0xA3, 0xCC, 0xAC, 0xB6, 0x01, 0xF6,
      0xD5, 0xBA, 0xFD, 0x82, 0xF6, 0x07, 0xDA, 0x65, 0xF7, 0x8C, 0xEC, 0xA1,
      0xF4, 0xE7, 0xA1, 0x96, 0xEF, 0x01, 0xAF, 0xB1, 0x36}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(UINT64_C(0), ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX), ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(uint64_t(INT64_MAX) + UINT64_C(1), ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(UINT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(8234131023748ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(13139587396049293857ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(34841574681334ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(13018U, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(17234298579837453943ULL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readU64());
  EXPECT_EQ(891055U, ReadNum.value());
  ASSERT_FALSE(ReadNum = VMgr.readU64());
  EXPECT_EQ(69U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadSigned32) {
  /// 23. Test signed 32bit integer decoding.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 30>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x80, 0x80, 0x80,
      0x78, 0x7F, 0x01, 0x86, 0x01, 0xFE, 0xB1, 0xEE, 0xD9, 0x7E,
      0xDA, 0xE5, 0x00, 0x88, 0xEB, 0xF3, 0x50, 0xAF, 0xB1, 0x36}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(INT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(INT32_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(-348415746, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(13018, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(-98765432, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS32());
  EXPECT_EQ(891055, ReadNum.value());
  ASSERT_FALSE(ReadNum = VMgr.readS32());
  EXPECT_EQ(30U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadSigned64) {
  /// 24. Test signed 64bit integer decoding.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 63>{
      0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F, 0x7F,
      0x01, 0x86, 0x01, 0x86, 0xEC, 0xBB, 0x89, 0xD4, 0xE5, 0xE7, 0x79,
      0xED, 0xA6, 0xC2, 0xFB, 0xE0, 0xA6, 0xB9, 0xDF, 0xF0, 0x00, 0xD3,
      0xB4, 0xA0, 0xA1, 0xC8, 0xFC, 0xE1, 0xAB, 0x80, 0x7F, 0x90, 0xB6,
      0xFC, 0xAC, 0xB3, 0x8B, 0xB6, 0xF0, 0xE2, 0x00}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(INT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(INT64_MIN, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(-1, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(1, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(134, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(-3484157981297146LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(8124182798172984173LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(-9198734298341434797LL, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readS64());
  EXPECT_EQ(7124932496753367824LL, ReadNum.value());
  ASSERT_FALSE(ReadNum = VMgr.readS64());
  EXPECT_EQ(63U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadFloat32) {
  /// 25. Test Special Cases float.
  ///
  ///   1.  +0.0
  ///   2.  -0.0
  ///   3.  sqrt(-1) : NaN
  ///   4.  log(-1) : NaN
  ///   5.  0.0 / 0.0 : NaN
  ///   6.  -0.0 / 0.0 : NaN
  ///   7.  log(0) : +inf
  ///   8.  1.0 / 0.0 : +inf
  ///   9.  -1.0 / 0.0 : -inf
  SSVM::Expect<float> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 36>{
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0xC0, 0xFF,
      0x00, 0x00, 0xC0, 0xFF, 0x00, 0x00, 0xC0, 0x7F, 0x00, 0x00, 0xC0, 0x7F,
      0x00, 0x00, 0x80, 0xFF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF32());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = VMgr.readF32());
  EXPECT_EQ(36U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadFloat64) {
  /// 26. Test Special Cases double.
  ///
  ///   1.  +0.0
  ///   2.  -0.0
  ///   3.  sqrt(-1) : NaN
  ///   4.  log(-1) : NaN
  ///   5.  0.0 / 0.0 : NaN
  ///   6.  -0.0 / 0.0 : NaN
  ///   7.  log(0) : +inf
  ///   8.  1.0 / 0.0 : +inf
  ///   9.  -1.0 / 0.0 : -inf
  SSVM::Expect<double> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 72>{
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xF8, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xF0, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_EQ(+0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_EQ(-0.0f, ReadNum.value());
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isnan(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_TRUE(ReadNum = VMgr.readF64());
  EXPECT_TRUE(std::isinf(ReadNum.value()));
  ASSERT_FALSE(ReadNum = VMgr.readF64());
  EXPECT_EQ(72U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadName) {
  /// 27. Test utf-8 string reading.
  SSVM::Expect<std::string> ReadStr;
  ASSERT_TRUE(VMgr.setCode(
      std::array<uint8_t, 15>{0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x01, 0x20,
                              0x06, 0x4C, 0x6F, 0x61, 0x64, 0x65, 0x72}));
  EXPECT_EQ(0U, VMgr.getOffset());
  ASSERT_TRUE(ReadStr = VMgr.readName());
  EXPECT_EQ("", ReadStr.value());
  ASSERT_TRUE(ReadStr = VMgr.readName());
  EXPECT_EQ("test", ReadStr.value());
  ASSERT_TRUE(ReadStr = VMgr.readName());
  EXPECT_EQ(" ", ReadStr.value());
  ASSERT_TRUE(ReadStr = VMgr.readName());
  EXPECT_EQ("Loader", ReadStr.value());
  ASSERT_FALSE(ReadStr = VMgr.readName());
  EXPECT_EQ(15U, VMgr.getOffset());
}

TEST(FileManagerTest, Vector__ReadUnsigned32TooLong) {
  /// 28. Test unsigned 32bit integer decoding in too long case.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(
      VMgr.setCode(std::array<uint8_t, 6>{0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  ASSERT_FALSE(ReadNum = VMgr.readU32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadUnsigned32TooLarge) {
  /// 29. Test unsigned 32bit integer decoding in too large case.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(
      VMgr.setCode(std::array<uint8_t, 5>{0x80, 0x80, 0x80, 0x80, 0x1F}));
  ASSERT_FALSE(ReadNum = VMgr.readU32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned32TooLong) {
  /// 30. Test signed 32bit integer decoding in too long case.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(
      VMgr.setCode(std::array<uint8_t, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  ASSERT_FALSE(ReadNum = VMgr.readS32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned32TooLarge) {
  /// 31. Test signed 32bit integer decoding in too large case.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(
      VMgr.setCode(std::array<uint8_t, 5>{0xFF, 0xFF, 0xFF, 0xFF, 0x4F}));
  ASSERT_FALSE(ReadNum = VMgr.readS32());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadUnsigned64TooLong) {
  /// 32. Test unsigned 64bit integer decoding in too long case.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 11>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  ASSERT_FALSE(ReadNum = VMgr.readU64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadUnsigned64TooLarge) {
  /// 33. Test unsigned 64bit integer decoding in too large case.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 10>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7E}));
  ASSERT_FALSE(ReadNum = VMgr.readU64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned64TooLong) {
  /// 34. Test signed 64bit integer decoding in too long case.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 11>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  ASSERT_FALSE(ReadNum = VMgr.readS64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLong, ReadNum.error());
}

TEST(FileManagerTest, Vector__ReadSigned64TooLarge) {
  /// 35. Test signed 64bit integer decoding in too large case.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(VMgr.setCode(std::array<uint8_t, 10>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41}));
  ASSERT_FALSE(ReadNum = VMgr.readS64());
  EXPECT_EQ(SSVM::ErrCode::IntegerTooLarge, ReadNum.error());
}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
