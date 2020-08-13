// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/ast/segmentTest.cpp - AST segment unit tests ------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST segment nodes, which are element
/// segment, code segment, data segment, and global segment.
///
//===----------------------------------------------------------------------===//

#include "common/ast/segment.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

SSVM::FileMgrVector Mgr;

TEST(SegmentTest, LoadGlobalSegment) {
  /// 1. Test load global segment.
  ///
  ///   1.  Load invalid empty global segment.
  ///   2.  Load global segment with expression of only End operation.
  ///   3.  Load global segment with non-empty expression.
  Mgr.clearBuffer();
  SSVM::AST::GlobalSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x7FU, 0x00, /// Global type.
      0x0BU        /// Expression.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::GlobalSegment Seg2;
  EXPECT_TRUE(Seg2.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x7FU, 0x00U,              /// Table index
      0x45U, 0x46U, 0x47U, 0x0BU /// Expression
  };
  Mgr.setCode(Vec3);
  SSVM::AST::GlobalSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SegmentTest, LoadElementSegment) {
  /// 2. Test load element segment.
  ///
  ///   1.  Load invalid empty element segment.
  ///   2.  Load element segment with expression of only End operation and empty
  ///       function indices list.
  ///   3.  Load element segment with expression and function indices list.
  /// TODO: Add prefix 0x01 ~ 0x07 tests.
  Mgr.clearBuffer();
  SSVM::AST::ElementSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x00U, /// Prefix 0x00
      0x0BU, /// Expression
      0x00U  /// Function indices list
  };
  Mgr.setCode(Vec2);
  SSVM::AST::ElementSegment Seg2;
  EXPECT_TRUE(Seg2.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x00U,                             /// Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Expression
      0x03U,                             /// Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0x00U,                             /// vec[1]
      0xB9U, 0x60U                       /// vec[2]
  };
  Mgr.setCode(Vec3);
  SSVM::AST::ElementSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SegmentTest, LoadCodeSegment) {
  /// 3. Test load code segment.
  ///
  ///   1.  Load invalid empty code segment.
  ///   2.  Load invalid code segment of zero content size.
  ///   3.  Load code segment of empty locals and expression with only End
  ///       operation.
  ///   4.  Load code segment with expression and local lists.
  Mgr.clearBuffer();
  SSVM::AST::CodeSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Code segment size
  };
  Mgr.setCode(Vec2);
  SSVM::AST::CodeSegment Seg2;
  EXPECT_FALSE(Seg2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x82U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size
      0x00U,                             /// Vector length = 0
      0x0BU                              /// Expression
  };
  Mgr.setCode(Vec3);
  SSVM::AST::CodeSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x95U, 0x80U, 0x80U, 0x80U, 0x00U,        /// Code segment size
      0x04U,                                    /// Vector length = 4
      0x01U, 0x7CU,                             /// vec[0]
      0x03U, 0x7DU,                             /// vec[1]
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7EU, /// vec[2]
      0xF3U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7FU, /// vec[3]
      0x45U, 0x46U, 0x47U, 0x0BU                /// Expression
  };
  Mgr.setCode(Vec4);
  SSVM::AST::CodeSegment Seg4;
  EXPECT_TRUE(Seg4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SegmentTest, LoadDataSegment) {
  /// 4. Test load data segment.
  ///
  ///   1.  Load invalid empty data segment.
  ///   2.  Load data segment of expression with only End operation and empty
  ///       initialization data.
  ///   3.  Load data segment with expression and initialization data.
  /// TODO: Add prefix 0x01 and 0x02 tests.
  Mgr.clearBuffer();
  SSVM::AST::DataSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x00U, /// Prefix 0x00
      0x0BU, /// Expression
      0x00U  /// Vector length = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::DataSegment Seg2;
  EXPECT_TRUE(Seg2.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x00U,                            /// Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU,       /// Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U /// Vector length = 4, "test"
  };
  Mgr.setCode(Vec3);
  SSVM::AST::DataSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

} // namespace
