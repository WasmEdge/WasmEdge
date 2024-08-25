// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace {

TEST(ErrInfoTest, Info__File) {
  WasmEdge::ErrInfo::InfoFile Info1("file.txt");
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Loading) {
  WasmEdge::ErrInfo::InfoLoading Info1(30);
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__AST) {
  WasmEdge::ErrInfo::InfoAST Info1(WasmEdge::ASTNodeAttr::Module);
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__InstanceBound) {
  WasmEdge::ErrInfo::InfoInstanceBound Info1(WasmEdge::ExternalType::Memory, 2,
                                             1);
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__ForbidIndex) {
  WasmEdge::ErrInfo::InfoForbidIndex Info1(
      WasmEdge::ErrInfo::IndexCategory::FunctionType, 2, 1);
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoForbidIndex Info2(
      WasmEdge::ErrInfo::IndexCategory::Memory, 2, 0);
  fmt::print("{}\n"sv, Info2);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Exporting) {
  WasmEdge::ErrInfo::InfoExporting Info1("export");
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Limit) {
  WasmEdge::ErrInfo::InfoLimit Info1(true, 10, 20);
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoLimit Info2(false, 30);
  fmt::print("{}\n"sv, Info2);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Registering) {
  WasmEdge::ErrInfo::InfoRegistering Info1("host_func");
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Linking) {
  WasmEdge::ErrInfo::InfoLinking Info1("module", "func");
  fmt::print("{}\n"sv, Info1);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Executing) {
  WasmEdge::ErrInfo::InfoExecuting Info1("", "func");
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoExecuting Info2("module", "func");
  fmt::print("{}\n"sv, Info2);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Mismatch) {
  WasmEdge::ErrInfo::InfoMismatch Info1(static_cast<uint8_t>(16), 8888);
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoMismatch Info2(WasmEdge::TypeCode::ExternRef,
                                        WasmEdge::TypeCode::FuncRef);
  fmt::print("{}\n"sv, Info2);
  WasmEdge::ErrInfo::InfoMismatch Info3(
      {WasmEdge::TypeCode::I32, WasmEdge::TypeCode::FuncRef},
      {WasmEdge::TypeCode::F64, WasmEdge::TypeCode::ExternRef,
       WasmEdge::TypeCode::V128});
  fmt::print("{}\n"sv, Info3);
  WasmEdge::ErrInfo::InfoMismatch Info4(WasmEdge::ValMut::Const,
                                        WasmEdge::ValMut::Var);
  fmt::print("{}\n"sv, Info4);
  WasmEdge::ErrInfo::InfoMismatch Info5(WasmEdge::ExternalType::Function,
                                        WasmEdge::ExternalType::Global);
  fmt::print("{}\n"sv, Info5);
  WasmEdge::ErrInfo::InfoMismatch Info6(
      {WasmEdge::TypeCode::I32, WasmEdge::TypeCode::FuncRef},
      {WasmEdge::TypeCode::I64, WasmEdge::TypeCode::F64},
      {WasmEdge::TypeCode::F64, WasmEdge::TypeCode::ExternRef,
       WasmEdge::TypeCode::V128},
      {WasmEdge::TypeCode::V128});
  fmt::print("{}\n"sv, Info6);
  WasmEdge::ErrInfo::InfoMismatch Info7(WasmEdge::TypeCode::ExternRef, true, 10,
                                        20, WasmEdge::TypeCode::FuncRef, true,
                                        20, 50);
  fmt::print("{}\n"sv, Info7);
  WasmEdge::ErrInfo::InfoMismatch Info8(WasmEdge::TypeCode::ExternRef, false,
                                        10, 10, WasmEdge::TypeCode::FuncRef,
                                        false, 20, 20);
  fmt::print("{}\n"sv, Info8);
  WasmEdge::ErrInfo::InfoMismatch Info9(true, 10, 20, true, 20, 50);
  fmt::print("{}\n"sv, Info9);
  WasmEdge::ErrInfo::InfoMismatch Info10(false, 10, 10, false, 20, 20);
  fmt::print("{}\n"sv, Info10);
  WasmEdge::ErrInfo::InfoMismatch Info11(
      WasmEdge::TypeCode::I32, WasmEdge::ValMut::Var, WasmEdge::TypeCode::I64,
      WasmEdge::ValMut::Const);
  fmt::print("{}\n"sv, Info11);
  WasmEdge::ErrInfo::InfoMismatch Info12(12345678U, 98765432U);
  fmt::print("{}\n"sv, Info12);

  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Instruction) {
  std::vector<WasmEdge::ValVariant> Args = {
      0, 1000,
      WasmEdge::RefVariant(
          reinterpret_cast<WasmEdge::Runtime::Instance::FunctionInstance *>(
              100))};
  WasmEdge::ErrInfo::InfoInstruction Info1(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::I32,
                                            WasmEdge::TypeCode::I32,
                                            WasmEdge::TypeCode::I32});
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoInstruction Info2(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::I32,
                                            WasmEdge::TypeCode::I32,
                                            WasmEdge::TypeCode::I32});
  fmt::print("{}\n"sv, Info2);
  WasmEdge::ErrInfo::InfoInstruction Info3(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::I64,
                                            WasmEdge::TypeCode::I64,
                                            WasmEdge::TypeCode::I64});
  fmt::print("{}\n"sv, Info3);
  WasmEdge::ErrInfo::InfoInstruction Info4(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::F32,
                                            WasmEdge::TypeCode::F32,
                                            WasmEdge::TypeCode::F32});
  fmt::print("{}\n"sv, Info4);
  WasmEdge::ErrInfo::InfoInstruction Info5(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::F64,
                                            WasmEdge::TypeCode::F64,
                                            WasmEdge::TypeCode::F64});
  fmt::print("{}\n"sv, Info5);
  WasmEdge::ErrInfo::InfoInstruction Info6(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::V128,
                                            WasmEdge::TypeCode::V128,
                                            WasmEdge::TypeCode::V128});
  fmt::print("{}\n"sv, Info6);
  WasmEdge::ErrInfo::InfoInstruction Info7(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::FuncRef,
                                            WasmEdge::TypeCode::FuncRef,
                                            WasmEdge::TypeCode::FuncRef});
  fmt::print("{}\n"sv, Info7);
  WasmEdge::ErrInfo::InfoInstruction Info8(WasmEdge::OpCode::Block, 255, Args,
                                           {WasmEdge::TypeCode::ExternRef,
                                            WasmEdge::TypeCode::ExternRef,
                                            WasmEdge::TypeCode::ExternRef});
  fmt::print("{}\n"sv, Info8);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Boundary) {
  WasmEdge::ErrInfo::InfoBoundary Info1(3, 5, 2);
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoBoundary Info2(3, 0, 2);
  fmt::print("{}\n"sv, Info2);
  EXPECT_TRUE(true);
}

TEST(ErrInfoTest, Info__Proposal) {
  WasmEdge::ErrInfo::InfoProposal Info1(WasmEdge::Proposal::SIMD);
  fmt::print("{}\n"sv, Info1);
  WasmEdge::ErrInfo::InfoProposal Info2(static_cast<WasmEdge::Proposal>(250U));
  fmt::print("{}\n"sv, Info2);
  EXPECT_TRUE(true);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
