//===-- ssvm/test/loader/sectionTest.cpp - AST section unit tests ---------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST section nodes.
///
//===----------------------------------------------------------------------===//

#include "loader/section.h"
#include "filemgrTest.h"
#include "gtest/gtest.h"

namespace {

FileMgrTest Mgr;

TEST(SectionTest, LoadCustomSection) {
  /// 1. Test load custom section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load custom section without contents.
  ///   3.  Load custom section with contents.
  Mgr.clearBuffer();
  AST::CustomSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setVector(Vec2);
  AST::CustomSection Sec2;
  EXPECT_TRUE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x87U, 0x80U, 0x80U, 0x80U, 0x00U,              /// Content size = 7
      0x00U, 0xFFU, 0xEEU, 0xDDU, 0xCCU, 0xBBU, 0xAAU /// Content
  };
  Mgr.setVector(Vec3);
  AST::CustomSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr));
}

TEST(SectionTest, LoadFunctionSection) {
  /// 4. Test load function section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load function section without contents.
  ///   3.  Load function section with content of zero vector.
  ///   4.  Load function section with contents.
  Mgr.clearBuffer();
  AST::FunctionSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setVector(Vec2);
  AST::FunctionSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 9
      0x00U,                             /// Vector length = 0
  };
  Mgr.setVector(Vec3);
  AST::FunctionSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x89U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 9
      0x03U,                             /// Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0x00U,                             /// vec[1]
      0xB9U, 0x60U                       /// vec[2]
  };
  Mgr.setVector(Vec4);
  AST::FunctionSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr));
}

TEST(SectionTest, LoadStartSection) {
  /// 9. Test load start section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load start section without contents.
  ///   3.  Load start section with contents.
  Mgr.clearBuffer();
  AST::StartSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setVector(Vec2);
  AST::StartSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x85U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 5
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Content
  };
  Mgr.setVector(Vec3);
  AST::StartSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr));
}

} // namespace