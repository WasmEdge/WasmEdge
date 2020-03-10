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

#include <limits>
#include <cmath>

namespace {

SSVM::FileMgrFStream Mgr;

TEST(FileManagerTest, SetPath) {
  /// 1. Test opening data file.
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readU64Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readS64Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readF32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readF64Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgrTestData/readNameTest.bin"));
}

TEST(FileManagerTest, ReadByte) {
  /// 2. Test unsigned char reading.
  SSVM::Expect<uint8_t> ReadByte;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
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
}

TEST(FileManagerTest, ReadBytes) {
  /// 3. Test unsigned char list reading.
  SSVM::Expect<std::vector<uint8_t>> ReadBytes;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readByteTest.bin"));
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
}

TEST(FileManagerTest, ReadUnsigned32) {
  /// 4. Test unsigned 32bit integer decoding.
  SSVM::Expect<uint32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU32Test.bin"));
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ(INT32_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU32());
  EXPECT_EQ((unsigned int)INT32_MAX + 1, ReadNum.value());
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
}

TEST(FileManagerTest, ReadUnsigned64) {
  /// 5. Test unsigned 64bit integer decoding.
  SSVM::Expect<uint64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readU64Test.bin"));
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(0, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ(INT64_MAX, ReadNum.value());
  ASSERT_TRUE(ReadNum = Mgr.readU64());
  EXPECT_EQ((uint64_t)INT64_MAX + 1, ReadNum.value());
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
}

TEST(FileManagerTest, ReadSigned32) {
  /// 6. Test signed 32bit integer decoding.
  SSVM::Expect<int32_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS32Test.bin"));
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
}

TEST(FileManagerTest, ReadSigned64) {
  /// 7. Test signed 64bit integer decoding.
  SSVM::Expect<int64_t> ReadNum;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readS64Test.bin"));
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
}

TEST(FileManagerTest, ReadFloat32) {
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
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readF32Test.bin"));
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
}

TEST(FileManagerTest, ReadFloat64) {
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
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readF64Test.bin"));
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
}

TEST(FileManagerTest, ReadName) {
  /// 10. Test utf-8 string reading.
  SSVM::Expect<std::string> ReadStr;
  ASSERT_TRUE(Mgr.setPath("filemgrTestData/readNameTest.bin"));
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("test", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ(" ", ReadStr.value());
  ASSERT_TRUE(ReadStr = Mgr.readName());
  EXPECT_EQ("Loader", ReadStr.value());
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
