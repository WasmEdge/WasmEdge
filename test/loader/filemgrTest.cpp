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

namespace {

AST::FileMgrFStream Mgr;
TEST(FileManagerTest, SetPath) {
  EXPECT_TRUE(Mgr.setPath("filemgr/readU32Test.bin"));
  EXPECT_TRUE(Mgr.setPath("filemgr/readS32Test.bin"));
}

TEST(FileManagerTest, ReadUnsigned32) {
  unsigned int ReadNum = 0;
  EXPECT_TRUE(Mgr.setPath("filemgr/readU32Test.bin"));
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(0, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(INT32_MAX, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ((unsigned int)INT32_MAX + 1, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(UINT32_MAX, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(165484164U, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(134U, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(3484157468U, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(13018U, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(98765432U, ReadNum);
  EXPECT_TRUE(Mgr.readU32(ReadNum));
  EXPECT_EQ(891055U, ReadNum);
}

TEST(FileManagerTest, ReadSigned32) {
  int ReadNum = 0;
  EXPECT_TRUE(Mgr.setPath("filemgr/readS32Test.bin"));
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(0, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(INT32_MAX, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(INT32_MIN, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(-1, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(1, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(134, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(-348415746, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(13018, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(-98765432, ReadNum);
  EXPECT_TRUE(Mgr.readS32(ReadNum));
  EXPECT_EQ(891055, ReadNum);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}