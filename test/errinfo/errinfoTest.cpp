// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/errinfo.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

namespace {

TEST(ErrInfoTest, Info__File) {
  WasmEdge::ErrInfo::InfoFile Info1("file.txt");
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Loading) {
  WasmEdge::ErrInfo::InfoLoading Info1(30);
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__AST) {
  WasmEdge::ErrInfo::InfoAST Info1(WasmEdge::ASTNodeAttr::Module);
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__InstanceBound) {
  WasmEdge::ErrInfo::InfoInstanceBound Info1(WasmEdge::ExternalType::Memory, 2,
                                             1);
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__ForbidIndex) {
  WasmEdge::ErrInfo::InfoForbidIndex Info1(
      WasmEdge::ErrInfo::IndexCategory::FunctionType, 2, 1);
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoForbidIndex Info2(
      WasmEdge::ErrInfo::IndexCategory::Memory, 2, 0);
  std::cout << Info2 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Exporting) {
  WasmEdge::ErrInfo::InfoExporting Info1("export");
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Limit) {
  WasmEdge::ErrInfo::InfoLimit Info1(true, 10, 20);
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoLimit Info2(false, 30);
  std::cout << Info2 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Registering) {
  WasmEdge::ErrInfo::InfoRegistering Info1("host_func");
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Linking) {
  WasmEdge::ErrInfo::InfoLinking Info1("module", "func");
  std::cout << Info1 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Executing) {
  WasmEdge::ErrInfo::InfoExecuting Info1("", "func");
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoExecuting Info2("module", "func");
  std::cout << Info2 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Mismatch) {
  WasmEdge::ErrInfo::InfoMismatch Info1(static_cast<uint8_t>(16), 8888);
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info2(WasmEdge::ValType::ExternRef,
                                        WasmEdge::ValType::FuncRef);
  std::cout << Info2 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info3(
      {WasmEdge::ValType::I32, WasmEdge::ValType::FuncRef},
      {WasmEdge::ValType::F64, WasmEdge::ValType::ExternRef,
       WasmEdge::ValType::V128});
  std::cout << Info3 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info4(WasmEdge::ValMut::Const,
                                        WasmEdge::ValMut::Var);
  std::cout << Info4 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info5(WasmEdge::ExternalType::Function,
                                        WasmEdge::ExternalType::Global);
  std::cout << Info5 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info6(
      {WasmEdge::ValType::I32, WasmEdge::ValType::FuncRef},
      {WasmEdge::ValType::I64, WasmEdge::ValType::F64},
      {WasmEdge::ValType::F64, WasmEdge::ValType::ExternRef,
       WasmEdge::ValType::V128},
      {WasmEdge::ValType::V128});
  std::cout << Info6 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info7(WasmEdge::RefType::ExternRef, true, 10,
                                        20, WasmEdge::RefType::FuncRef, true,
                                        20, 50);
  std::cout << Info7 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info8(WasmEdge::RefType::ExternRef, false, 10,
                                        10, WasmEdge::RefType::FuncRef, false,
                                        20, 20);
  std::cout << Info8 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info9(true, 10, 20, true, 20, 50);
  std::cout << Info9 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info10(false, 10, 10, false, 20, 20);
  std::cout << Info10 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info11(
      WasmEdge::ValType::I32, WasmEdge::ValMut::Var, WasmEdge::ValType::I64,
      WasmEdge::ValMut::Const);
  std::cout << Info11 << std::endl;
  WasmEdge::ErrInfo::InfoMismatch Info12(12345678U, 98765432U);
  std::cout << Info12 << std::endl;

  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Instruction) {
  std::vector<WasmEdge::ValVariant> Args = {
      0, 1000,
      WasmEdge::FuncRef(
          reinterpret_cast<WasmEdge::Runtime::Instance::FunctionInstance *>(
              100))};
  WasmEdge::ErrInfo::InfoInstruction Info1(
      WasmEdge::OpCode::Block, 255, Args,
      {WasmEdge::ValType::I32, WasmEdge::ValType::I32, WasmEdge::ValType::I32});
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info2(
      WasmEdge::OpCode::Block, 255, Args,
      {WasmEdge::ValType::I32, WasmEdge::ValType::I32, WasmEdge::ValType::I32});
  std::cout << Info2 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info3(
      WasmEdge::OpCode::Block, 255, Args,
      {WasmEdge::ValType::I64, WasmEdge::ValType::I64, WasmEdge::ValType::I64});
  std::cout << Info3 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info4(
      WasmEdge::OpCode::Block, 255, Args,
      {WasmEdge::ValType::F32, WasmEdge::ValType::F32, WasmEdge::ValType::F32});
  std::cout << Info4 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info5(
      WasmEdge::OpCode::Block, 255, Args,
      {WasmEdge::ValType::F64, WasmEdge::ValType::F64, WasmEdge::ValType::F64});
  std::cout << Info5 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info6(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::ValType::V128,
                                            WasmEdge::ValType::V128,
                                            WasmEdge::ValType::V128});
  std::cout << Info6 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info7(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::ValType::FuncRef,
                                            WasmEdge::ValType::FuncRef,
                                            WasmEdge::ValType::FuncRef});
  std::cout << Info7 << std::endl;
  WasmEdge::ErrInfo::InfoInstruction Info8(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::ValType::ExternRef,
                                            WasmEdge::ValType::ExternRef,
                                            WasmEdge::ValType::ExternRef});
  std::cout << Info8 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Boundary) {
  WasmEdge::ErrInfo::InfoBoundary Info1(3, 5, 2);
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoBoundary Info2(3, 0, 2);
  std::cout << Info2 << std::endl;
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Proposal) {
  WasmEdge::ErrInfo::InfoProposal Info1(WasmEdge::Proposal::SIMD);
  std::cout << Info1 << std::endl;
  WasmEdge::ErrInfo::InfoProposal Info2(static_cast<WasmEdge::Proposal>(250U));
  std::cout << Info2 << std::endl;
  EXPECT_TRUE(true);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
