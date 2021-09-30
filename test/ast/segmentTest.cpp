// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/ast/segmentTest.cpp - AST segment unit tests --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST segment nodes, which are element
/// segment, code segment, data segment, and global segment.
///
//===----------------------------------------------------------------------===//

#include "ast/segment.h"

#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgr Mgr;
WasmEdge::Configure Conf;

TEST(SegmentTest, LoadGlobalSegment) {
  /// 1. Test load global segment.
  ///
  ///   1.  Load invalid empty global segment.
  ///   2.  Load global segment with expression of only End operation.
  ///   3.  Load global segment with non-empty expression.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::GlobalSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x7FU, 0x00, /// Global type.
      0x0BU        /// Expression.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::GlobalSegment Seg2;
  EXPECT_TRUE(Seg2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x7FU, 0x00U,              /// Table index
      0x45U, 0x46U, 0x47U, 0x0BU /// Expression
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::GlobalSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

TEST(SegmentTest, LoadElementSegment) {
  /// 2. Test load element segment.
  ///
  ///   1.  Load invalid empty element segment.
  ///   2.  Load element segment with expression of only End operation and empty
  ///       function indices list.
  ///   3.  Load element segment with expression and function indices list.
  ///   4.  Load element segment with invalid checking byte.
  ///   5.  Load element segment with invalid checking byte without Ref-Types
  ///       proposal.
  ///   6.  Load element segment with unexpected end of table index.
  ///   7.  Load element segment with unexpected end of offset expression.
  ///   8.  Load element segment with invalid element kind.
  ///   9.  Load element segment with unexpected end of element kind.
  ///   10. Load element segment with unexpected end of initialization vector
  ///       count.
  ///   11. Load element segment with unexpected end of initialization function
  ///       index.
  ///   12. Load element segment with unexpected end of reference type.
  ///   13. Load element segment with unexpected end of initialization
  ///       expression vector count.
  ///   14. Load element segment with invalid instructions in initialization
  ///       expressions.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::ElementSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x00U, /// Prefix checking byte
      0x0BU, /// Offset expression
      0x00U  /// Function indices list
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::ElementSegment Seg2;
  EXPECT_TRUE(Seg2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x00U,                             /// Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Offset expression
      0x03U,                             /// Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0x00U,                             /// vec[1]
      0xB9U, 0x60U                       /// vec[2]
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::ElementSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x09U,                             /// Prefix invalid checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Offset expression
      0x03U,                             /// Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0x00U,                             /// vec[1]
      0xB9U, 0x60U                       /// vec[2]
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::ElementSegment Seg4;
  EXPECT_FALSE(Seg4.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec5 = {
      0x01U,       /// Prefix invalid checking byte without Ref-Types proposal
      0x00U,       /// Element kind
      0x03U,       /// Vector length = 3
      0x0AU,       /// vec[0]
      0x00U,       /// vec[1]
      0xB9U, 0x60U /// vec[2]
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::ElementSegment Seg5;
  EXPECT_FALSE(Seg5.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec6 = {
      0x02U /// Prefix checking byte
            /// Missed table index
            /// Missed offset expression
            /// Missed element kind
            /// Missed initialization vector
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::ElementSegment Seg6;
  EXPECT_FALSE(Seg6.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec7 = {
      0x00U,              /// Prefix checking byte
      0x45U, 0x46U, 0x47U /// Offset expression
                          /// 0x0BU     Missed end of offset expression
                          /// Missed initialization vector
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::ElementSegment Seg7;
  EXPECT_FALSE(Seg7.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec8 = {
      0x01U,       /// Prefix checking byte
      0x08U,       /// Invalid element kind
      0x03U,       /// Vector length = 3
      0x0AU,       /// vec[0]
      0x00U,       /// vec[1]
      0xB9U, 0x60U /// vec[2]
  };
  Mgr.setCode(Vec8);
  WasmEdge::AST::ElementSegment Seg8;
  EXPECT_FALSE(Seg8.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec9 = {
      0x01U /// Prefix checking byte
            /// Missed element kind
            /// Missed initialization vector
  };
  Mgr.setCode(Vec9);
  WasmEdge::AST::ElementSegment Seg9;
  EXPECT_FALSE(Seg9.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec10 = {
      0x01U, /// Prefix checking byte
      0x00U  /// Element kind
             /// Missed initialization vector
  };
  Mgr.setCode(Vec10);
  WasmEdge::AST::ElementSegment Seg10;
  EXPECT_FALSE(Seg10.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec11 = {
      0x01U, /// Prefix checking byte
      0x00U, /// Element kind
      0x03U, /// Vector length = 3
      0x0AU  /// vec[0]
             /// Missed vec[1] and vec[2]
  };
  Mgr.setCode(Vec11);
  WasmEdge::AST::ElementSegment Seg11;
  EXPECT_FALSE(Seg11.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec12 = {
      0x05U /// Prefix checking byte
            /// Missed reference type
            /// Missed initialization expressions
  };
  Mgr.setCode(Vec12);
  WasmEdge::AST::ElementSegment Seg12;
  EXPECT_FALSE(Seg12.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec13 = {
      0x04U,                     /// Prefix checking byte of 0x04
      0x45U, 0x46U, 0x47U, 0x0BU /// Offset expression
                                 /// Missed initialization expressions
  };
  Mgr.setCode(Vec13);
  WasmEdge::AST::ElementSegment Seg13;
  EXPECT_FALSE(Seg13.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec14 = {
      0x04U,                      /// Prefix checking byte of 0x04
      0x45U, 0x46U, 0x47U, 0x0BU, /// Offset expression
      0x03U,                      /// Vector length = 3
      0xD0U, 0x6FU, 0x0BU,        /// vec[0]: expression 0
      0xD2U, 0x05U, 0x0BU,        /// vec[1]: expression 1
      0x20U, 0x05U, 0x0BU /// vec[2]: expression 2 with invalid instruction
  };
  Mgr.setCode(Vec14);
  WasmEdge::AST::ElementSegment Seg14;
  EXPECT_FALSE(Seg14.loadBinary(Mgr, Conf));
}

TEST(SegmentTest, LoadCodeSegment) {
  /// 3. Test load code segment.
  ///
  ///   1.  Load invalid empty code segment.
  ///   2.  Load invalid code segment of zero content size.
  ///   3.  Load code segment of empty locals and expression with only End
  ///       operation.
  ///   4.  Load code segment with expression and local lists.
  ///   5.  Load code segment with unexpected end of local number type.
  ///   6.  Load code segment with invalid local number type without Ref-Types
  ///       proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::CodeSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Code segment size
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::CodeSegment Seg2;
  EXPECT_FALSE(Seg2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x82U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size
      0x00U,                             /// Vector length = 0
      0x0BU                              /// Expression
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::CodeSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x93U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size
      0x04U,                             /// Vector length = 4
      0x01U, 0x7CU,                      /// vec[0]
      0x03U, 0x7DU,                      /// vec[1]
      0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7EU, /// vec[2]
      0xF3U, 0xFFU, 0xFFU, 0x0FU, 0x7FU, /// vec[3]
      0x45U, 0x46U, 0x47U, 0x0BU         /// Expression
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::CodeSegment Seg4;
  EXPECT_TRUE(Seg4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec5 = {
      0x88U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size
      0x04U,                             /// Vector length = 2
      0x01U, 0x7CU,                      /// vec[0]
      0x03U /// 0x7DU                    /// vec[1], missed value type
            /// 0x45U, 0x46U, 0x0BU      /// Missed Expression
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::CodeSegment Seg5;
  EXPECT_FALSE(Seg5.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec6 = {
      0x93U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size
      0x04U,                             /// Vector length = 4
      0x01U, 0x7CU,                      /// vec[0]
      0x03U, 0x6FU,                      /// vec[1], ExternRef
      0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7EU, /// vec[2]
      0xF3U, 0xFFU, 0xFFU, 0x0FU, 0x7FU, /// vec[3]
      0x45U, 0x46U, 0x47U, 0x0BU         /// Expression
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::CodeSegment Seg6;
  EXPECT_FALSE(Seg6.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
}

TEST(SegmentTest, LoadDataSegment) {
  /// 4. Test load data segment.
  ///
  ///   1.  Load invalid empty data segment.
  ///   2.  Load data segment of expression with only End operation and empty
  ///       initialization data.
  ///   3.  Load data segment with expression and initialization data.
  ///   4.  Load data segment with invalid checking byte.
  ///   5.  Load data segment with invalid checking byte without Bulk-Mem
  ///       proposal.
  ///   6.  Load data segment with unexpected end of memory index.
  ///   7.  Load data segment with unexpected end of expression.
  ///   8.  Load data segment with unexpected end of initialization data vector.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::DataSegment Seg1;
  EXPECT_FALSE(Seg1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x00U, /// Prefix checking byte
      0x0BU, /// Expression
      0x00U  /// Vector length = 0
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::DataSegment Seg2;
  EXPECT_TRUE(Seg2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x00U,                            /// Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       /// Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U /// Vector length = 4, "test"
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::DataSegment Seg3;
  EXPECT_TRUE(Seg3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x06U,                            /// Prefix invalid checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       /// Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U /// Vector length = 4, "test"
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::DataSegment Seg4;
  EXPECT_FALSE(Seg4.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec5 = {
      0x01U, /// Prefix invalid checking byte without Bulk-Mem proposal
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U /// Vector length = 4, "test"
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::DataSegment Seg5;
  EXPECT_FALSE(Seg5.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec6 = {
      0x02U /// Prefix checking byte
            /// Missed memory index, offset expression, and initialization data.
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::DataSegment Seg6;
  EXPECT_FALSE(Seg6.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec7 = {
      0x02U,       /// Prefix checking byte
      0x45U, 0x46U /// Missed end of expression
                   /// Missed initialization data
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::DataSegment Seg7;
  EXPECT_FALSE(Seg7.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec8 = {
      0x02U,              /// Prefix checking byte
      0x45U, 0x46U, 0x0BU /// Expression
                          /// Missed initialization data
  };
  Mgr.setCode(Vec8);
  WasmEdge::AST::DataSegment Seg8;
  EXPECT_FALSE(Seg8.loadBinary(Mgr, Conf));
}

} // namespace
