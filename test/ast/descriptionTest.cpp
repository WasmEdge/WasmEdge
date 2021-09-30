// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/ast/descriptionTest.cpp - AST description unit tests ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST description nodes, which are ImportDesc
/// and ExportDesc.
///
//===----------------------------------------------------------------------===//

#include "ast/description.h"

#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgr Mgr;
WasmEdge::Configure Conf;

TEST(DescriptionTest, LoadImportDesc) {
  /// 1. Test load import description.
  ///
  ///   1.  Load invalid empty import description.
  ///   2.  Load import description with empty module and external name.
  ///   3.  Load import description with module and external names.
  ///   4.  Load import description with invalid external type.
  ///   5.  Load import description of table type.
  ///   6.  Load import description of memory type.
  ///   7.  Load import description of global type.
  ///   8.  Load invalid import description of global type without Mut-Globals
  ///       proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::ImportDesc Imp1;
  EXPECT_FALSE(Imp1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x00U,       /// Empty module name
      0x00U,       /// Empty external name
      0x00U, 0x00U /// function type and index
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::ImportDesc Imp2;
  EXPECT_TRUE(Imp2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x00U, 0x00U /// function type and index
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::ImportDesc Imp3;
  EXPECT_TRUE(Imp3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x04U, 0x00U                                     /// Invalid external type
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::ImportDesc Imp4;
  EXPECT_FALSE(Imp4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x01U,                                           /// Table type
      0x70U,                                           /// Reference type
      0x01U,                                           /// Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU,               /// Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU                /// Max = 4294967295
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::ImportDesc Imp5;
  EXPECT_TRUE(Imp5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec6 = {
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x02U,                                           /// Memory type
      0x01U,                                           /// Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU,               /// Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU                /// Max = 4294967295
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::ImportDesc Imp6;
  EXPECT_TRUE(Imp6.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec7 = {
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x03U,                                           /// Global type
      0x7CU, 0x00U                                     /// Const F64 number type
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::ImportDesc Imp7;
  EXPECT_TRUE(Imp7.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  Conf.removeProposal(WasmEdge::Proposal::ImportExportMutGlobals);

  std::vector<unsigned char> Vec8 = {
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x03U,                                           /// Global type
      0x7CU, 0x01U                                     /// Mut F64 number type
  };
  Mgr.setCode(Vec8);
  WasmEdge::AST::ImportDesc Imp8;
  EXPECT_FALSE(Imp8.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::ImportExportMutGlobals);
}

TEST(DescriptionTest, LoadExportDesc) {
  /// 2. Test load export description.
  ///
  ///   1.  Load invalid empty export description.
  ///   2.  Load export description with empty module name.
  ///   3.  Load export description with non-empty module name.
  ///   4.  Load export description with invalid external type.
  ///   5.  Load export description of table type.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::ExportDesc Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x00U,       /// Empty external name
      0x00U, 0x00U /// function type and index
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::ExportDesc Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x00U, 0x00U /// function type and index
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::ExportDesc Exp3;
  EXPECT_TRUE(Exp3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x04U, 0x00U                                     /// Invalid external type
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::ExportDesc Exp4;
  EXPECT_FALSE(Exp4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// External name: Loader
      0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Table type and table index
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::ExportDesc Exp5;
  EXPECT_TRUE(Exp5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

} // namespace
