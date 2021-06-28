// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/ast/typeTest.cpp - AST type unit tests --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST type nodes.
///
//===----------------------------------------------------------------------===//

#include "ast/type.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgr Mgr;
WasmEdge::Configure Conf;

TEST(TypeTest, LoadLimit) {
  /// 1. Test load limit.
  ///
  ///   1.  Load invalid empty limit.
  ///   2.  Load invalid types of limit.
  ///   3.  Load limit with only min.
  ///   4.  Load invalid limit with fail of loading max.
  ///   5.  Load limit with min and max.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Limit Lim1;
  EXPECT_FALSE(Lim1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x02U, /// Unknown limit type
      0x00U  /// Min = 0
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Limit Lim2;
  EXPECT_FALSE(Lim2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x00U,                            /// Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Min = 4294967295
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Limit Lim3;
  EXPECT_TRUE(Lim3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x01U,                            /// Has min and max
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Min = 4294967295
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Limit Lim4;
  EXPECT_FALSE(Lim4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0x01U,                             /// Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Max = 4294967295
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Limit Lim5;
  EXPECT_TRUE(Lim5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

TEST(TypeTest, LoadFunctionType) {
  /// 2. Test load function type.
  ///
  ///   1.  Load invalid empty function type.
  ///   2.  Load invalid types of function type.
  ///   3.  Load void parameter and result function type.
  ///   4.  Load non-void parameter function type.
  ///   5.  Load non-void result function type.
  ///   6.  Load function type with parameters and result.
  ///   7.  Load invalid parameters with unexpected end.
  ///   8.  Load invalid results with unexpected end.
  ///   9.  Load invalid parameters with invalid value types.
  ///   10. Load invalid results with invalid value types.
  ///   11. Load invalid parameters with ExternRef without Ref-Types proposal.
  ///   12. Load invalid results with ExternRef without Ref-Types proposal.
  ///   13. Load invalid parameters with invalid value types without Ref-Types
  ///       proposal.
  ///   14. Load invalid results with invalid value types without Ref-Types
  ///       proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::FunctionType Fun1;
  EXPECT_FALSE(Fun1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {0xFFU, /// Invalid function type header
                                     0x00U, 0x00U};
  Mgr.setCode(Vec2);
  WasmEdge::AST::FunctionType Fun2;
  EXPECT_FALSE(Fun2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x60U, /// Function type header
      0x00U, /// Parameter length = 0
      0x00U  /// Result length = 0
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::FunctionType Fun3;
  EXPECT_TRUE(Fun3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x60U,                      /// Function type header
      0x04U,                      /// Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, /// Parameter list
      0x00U                       /// Result length = 0
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::FunctionType Fun4;
  EXPECT_TRUE(Fun4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec5 = {
      0x60U, /// Function type header
      0x00U, /// Parameter length = 0
      0x01U, /// Result length = 1 (must be <= 1 now)
      0x7CU  /// Result list
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::FunctionType Fun5;
  EXPECT_TRUE(Fun5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec6 = {
      0x60U,                      /// Function type header
      0x04U,                      /// Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, /// Parameter list
      0x01U,                      /// Result length = 1
      0x7CU                       /// Result list
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::FunctionType Fun6;
  EXPECT_TRUE(Fun6.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec7 = {
      0x60U,       /// Function type header
      0x04U,       /// Parameter length = 4
      0x7CU, 0x7DU /// Parameter list only in 2
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::FunctionType Fun7;
  EXPECT_FALSE(Fun7.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec8 = {
      0x60U,                      /// Function type header
      0x04U,                      /// Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, /// Parameter list
      0x02U,                      /// Result length = 2
      0x7CU                       /// Result list only in 1
  };
  Mgr.setCode(Vec8);
  WasmEdge::AST::FunctionType Fun8;
  EXPECT_FALSE(Fun8.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec9 = {
      0x60U,        /// Function type header
      0x02U,        /// Parameter length = 2
      0x6DU, 0x6DU, /// Parameter list with invalid value types
      0x01U,        /// Result length = 2
      0x7CU, 0x7FU  /// Result list
  };
  Mgr.setCode(Vec9);
  WasmEdge::AST::FunctionType Fun9;
  EXPECT_FALSE(Fun9.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec10 = {
      0x60U,        /// Function type header
      0x02U,        /// Parameter length = 2
      0x7CU, 0x7FU, /// Parameter list
      0x01U,        /// Result length = 2
      0x6DU, 0x6DU  /// Result list with invalid value types
  };
  Mgr.setCode(Vec10);
  WasmEdge::AST::FunctionType Fun10;
  EXPECT_FALSE(Fun10.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec11 = {
      0x60U,        /// Function type header
      0x02U,        /// Parameter length = 2
      0x6FU, 0x6FU, /// Parameter list with ExternRef
      0x01U,        /// Result length = 2
      0x7CU, 0x7FU  /// Result list
  };
  Mgr.setCode(Vec11);
  WasmEdge::AST::FunctionType Fun11;
  EXPECT_FALSE(Fun11.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec12 = {
      0x60U,        /// Function type header
      0x02U,        /// Parameter length = 2
      0x7CU, 0x7FU, /// Parameter list
      0x01U,        /// Result length = 2
      0x6FU, 0x6FU  /// Result list with ExternRef
  };
  Mgr.setCode(Vec12);
  WasmEdge::AST::FunctionType Fun12;
  EXPECT_FALSE(Fun12.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec13 = {
      0x60U,        /// Function type header
      0x02U,        /// Parameter length = 2
      0x6DU, 0x6DU, /// Parameter list with invalid value types
      0x01U,        /// Result length = 2
      0x7CU, 0x7FU  /// Result list
  };
  Mgr.setCode(Vec13);
  WasmEdge::AST::FunctionType Fun13;
  EXPECT_FALSE(Fun13.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec14 = {
      0x60U,        /// Function type header
      0x02U,        /// Parameter length = 2
      0x7CU, 0x7FU, /// Parameter list
      0x01U,        /// Result length = 2
      0x6DU, 0x6DU  /// Result list with invalid value types
  };
  Mgr.setCode(Vec14);
  WasmEdge::AST::FunctionType Fun14;
  EXPECT_FALSE(Fun14.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
}

TEST(TypeTest, LoadMemoryType) {
  /// 1. Test load memory type, which is limit.
  ///
  ///   1.  Load invalid empty limit.
  ///   2.  Load invalid types of limit.
  ///   3.  Load limit with only min.
  ///   4.  Load invalid limit with fail of loading max.
  ///   5.  Load limit with min and max.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::MemoryType Mem1;
  EXPECT_FALSE(Mem1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x02U, /// Unknown limit type
      0x00U  /// Min = 0
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::MemoryType Mem2;
  EXPECT_FALSE(Mem2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x00U,                            /// Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Min = 4294967295
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::MemoryType Mem3;
  EXPECT_TRUE(Mem3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x01U,                            /// Has min and max
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Min = 4294967295
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::MemoryType Mem4;
  EXPECT_FALSE(Mem4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0x01U,                             /// Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Max = 4294967295
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::MemoryType Mem5;
  EXPECT_TRUE(Mem5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

TEST(TypeTest, LoadTableType) {
  /// 1. Test load table type, which is reference type and limit.
  ///
  ///   1.  Load invalid empty table type.
  ///   2.  Load invalid reference type.
  ///   3.  Load invalid types of limit in table type.
  ///   4.  Load limit with only min.
  ///   5.  Load invalid limit with fail of loading max.
  ///   6.  Load limit with min and max.
  ///   7.  Load invalid ExternRef without Ref-Types proposal.
  ///   8.  Load invalid reference type without Ref-Types proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::TableType Tab1;
  EXPECT_FALSE(Tab1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0xFFU, /// Unknown reference type
      0x00U, /// Limit with only has min
      0x00U  /// Min = 0
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::TableType Tab2;
  EXPECT_FALSE(Tab2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x70U, /// Reference type
      0x02U, /// Unknown limit type
      0x00U  /// Min = 0
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::TableType Tab3;
  EXPECT_FALSE(Tab3.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec4 = {
      0x70U,                            /// Reference type
      0x00U,                            /// Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Min = 4294967295
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::TableType Tab4;
  EXPECT_TRUE(Tab4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec5 = {
      0x70U,                            /// Reference type
      0x01U,                            /// Has min and max
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Min = 4294967295
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::TableType Tab5;
  EXPECT_FALSE(Tab5.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec6 = {
      0x70U,                             /// Reference type
      0x01U,                             /// Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Max = 4294967295
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::TableType Tab6;
  EXPECT_TRUE(Tab6.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec7 = {
      0xFFU, /// Unknown reference type
      0x00U, /// Limit with only has min
      0x00U  /// Min = 0
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::TableType Tab7;
  EXPECT_FALSE(Tab7.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec8 = {
      0x6FU, /// Unknown reference type
      0x00U, /// Limit with only has min
      0x00U  /// Min = 0
  };
  Mgr.setCode(Vec8);
  WasmEdge::AST::TableType Tab8;
  EXPECT_FALSE(Tab8.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
}

TEST(TypeTest, LoadGlobalType) {
  /// 1. Test load global type.
  ///
  ///   1.  Load invalid empty global type.
  ///   2.  Load invalid global type without mutation.
  ///   3.  Load invalid mutation of global type.
  ///   4.  Load valid global type.
  ///   5.  Load invalid global type with invalid value types.
  ///   6.  Load invalid global type with ExternRef without Ref-Types proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::GlobalType Glb1;
  EXPECT_FALSE(Glb1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x7CU /// F64 number type
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::GlobalType Glb2;
  EXPECT_FALSE(Glb2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x7CU, /// F64 number type
      0xFFU  /// Invalid mutation type
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::GlobalType Glb3;
  EXPECT_FALSE(Glb3.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec4 = {
      0x7CU, /// F64 number type
      0x00U  /// Const mutation
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::GlobalType Glb4;
  EXPECT_TRUE(Glb4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec5 = {
      0x6DU, /// Unknown value type
      0x00U  /// Const mutation
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::GlobalType Glb5;
  EXPECT_FALSE(Glb5.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec6 = {
      0x6FU, /// ExternRef
      0x00U  /// Const mutation
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::GlobalType Glb6;
  EXPECT_FALSE(Glb6.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
}

} // namespace
