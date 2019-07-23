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
#include "limits"
#include "gtest/gtest.h"
#include <cmath>

namespace {

FileMgrFStream Mgr;
Loader::ErrCode SuccessCode = Loader::ErrCode::Success;

TEST(FileManagerTest, SetPath) {
  /// 1. Test opening data file.
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readByteTest.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readU32Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readU64Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readS32Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readS64Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readF32Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readF64Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readNameTest.bin"), SuccessCode);
}

TEST(FileManagerTest, ReadByte) {
  /// 2. Test unsigned char reading.
  unsigned char ReadByte = 0x00;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readByteTest.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x00, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0xFF, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x1F, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x2E, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x3D, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x4C, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x5B, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x6A, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x79, ReadByte);
  EXPECT_EQ(Mgr.readByte(ReadByte), SuccessCode);
  EXPECT_EQ(0x88, ReadByte);
}

TEST(FileManagerTest, ReadBytes) {
  /// 3. Test unsigned char list reading.
  std::vector<unsigned char> ReadBytes;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readByteTest.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readBytes(ReadBytes, 1), SuccessCode);
  EXPECT_EQ(0x00, ReadBytes[0]);
  EXPECT_EQ(Mgr.readBytes(ReadBytes, 2), SuccessCode);
  EXPECT_EQ(0xFF, ReadBytes[1]);
  EXPECT_EQ(0x1F, ReadBytes[2]);
  EXPECT_EQ(Mgr.readBytes(ReadBytes, 3), SuccessCode);
  EXPECT_EQ(0x2E, ReadBytes[3]);
  EXPECT_EQ(0x3D, ReadBytes[4]);
  EXPECT_EQ(0x4C, ReadBytes[5]);
  EXPECT_EQ(Mgr.readBytes(ReadBytes, 4), SuccessCode);
  EXPECT_EQ(0x5B, ReadBytes[6]);
  EXPECT_EQ(0x6A, ReadBytes[7]);
  EXPECT_EQ(0x79, ReadBytes[8]);
  EXPECT_EQ(0x88, ReadBytes[9]);
}

TEST(FileManagerTest, ReadUnsigned32) {
  /// 4. Test unsigned 32bit integer decoding.
  unsigned int ReadNum = 0;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readU32Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(0, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(INT32_MAX, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ((unsigned int)INT32_MAX + 1, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(UINT32_MAX, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(165484164U, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(134U, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(3484157468U, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(13018U, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(98765432U, ReadNum);
  EXPECT_EQ(Mgr.readU32(ReadNum), SuccessCode);
  EXPECT_EQ(891055U, ReadNum);
}

TEST(FileManagerTest, ReadUnsigned64) {
  /// 5. Test unsigned 64bit integer decoding.
  uint64_t ReadNum = 0;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readU64Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(0, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(INT64_MAX, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ((uint64_t)INT64_MAX + 1, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(UINT64_MAX, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(8234131023748ULL, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(13139587396049293857ULL, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(34841574681334ULL, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(13018U, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(17234298579837453943ULL, ReadNum);
  EXPECT_EQ(Mgr.readU64(ReadNum), SuccessCode);
  EXPECT_EQ(891055U, ReadNum);
}

TEST(FileManagerTest, ReadSigned32) {
  /// 6. Test signed 32bit integer decoding.
  int ReadNum = 0;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readS32Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(0, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(INT32_MAX, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(INT32_MIN, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(-1, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(1, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(134, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(-348415746, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(13018, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(-98765432, ReadNum);
  EXPECT_EQ(Mgr.readS32(ReadNum), SuccessCode);
  EXPECT_EQ(891055, ReadNum);
}

TEST(FileManagerTest, ReadSigned64) {
  /// 7. Test signed 64bit integer decoding.
  int64_t ReadNum = 0;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readS64Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(0, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(INT64_MAX, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(INT64_MIN, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(-1, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(1, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(134, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(-3484157981297146LL, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(8124182798172984173LL, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(-9198734298341434797LL, ReadNum);
  EXPECT_EQ(Mgr.readS64(ReadNum), SuccessCode);
  EXPECT_EQ(7124932496753367824LL, ReadNum);
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
  float ReadNum = 0;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readF32Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_EQ(+0.0f, ReadNum);
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_EQ(-0.0f, ReadNum);
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isinf(ReadNum));
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isinf(ReadNum));
  EXPECT_EQ(Mgr.readF32(ReadNum), SuccessCode);
  EXPECT_TRUE(isinf(ReadNum));
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
  double ReadNum = 0;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readF64Test.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_EQ(+0.0f, ReadNum);
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_EQ(-0.0f, ReadNum);
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isnan(ReadNum));
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isinf(ReadNum));
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isinf(ReadNum));
  EXPECT_EQ(Mgr.readF64(ReadNum), SuccessCode);
  EXPECT_TRUE(isinf(ReadNum));
}

TEST(FileManagerTest, ReadName) {
  /// 10. Test utf-8 string reading.
  std::string ReadStr;
  EXPECT_EQ(Mgr.setPath("filemgrTestData/readNameTest.bin"), SuccessCode);
  EXPECT_EQ(Mgr.readName(ReadStr), SuccessCode);
  EXPECT_EQ("", ReadStr);
  EXPECT_EQ(Mgr.readName(ReadStr), SuccessCode);
  EXPECT_EQ("test", ReadStr);
  EXPECT_EQ(Mgr.readName(ReadStr), SuccessCode);
  EXPECT_EQ("test ", ReadStr);
  EXPECT_EQ(Mgr.readName(ReadStr), SuccessCode);
  EXPECT_EQ("test Loader", ReadStr);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}