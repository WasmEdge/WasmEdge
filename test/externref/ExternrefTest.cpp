// SPDX-License-Identifier: Apache-2.0

#include "ExternrefTest.h"

#include "common/configure.h"
#include "common/types.h"
#include "vm/vm.h"
#include "gtest/gtest.h"

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

TEST(ExternRefTest, Ref__Functions) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::ExternMod ExtMod;
  std::vector<WasmEdge::ValVariant> FuncArgs;
  std::vector<WasmEdge::ValType> FuncArgTypes;
  VM.registerModule(ExtMod);
  ASSERT_TRUE(VM.loadWasm("externrefTestData/funcs.wasm"));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  /// Functor instance
  SquareStruct SS;
  /// Class instance
  AddClass AC;

  /// Test 1: call add -- 1234 + 5678
  FuncArgs = {WasmEdge::ExternRef(&AC), 1234U, 5678U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32,
                  WasmEdge::ValType::I32};
  auto Res1 = VM.execute("call_add", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res1);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ((*Res1)[0].get<uint32_t>(), 6912U);

  /// Test 2: call mul -- 789 * 4321
  FuncArgs = {WasmEdge::ExternRef(MulFunc), 789U, 4321U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32,
                  WasmEdge::ValType::I32};
  auto Res2 = VM.execute("call_mul", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res2);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ((*Res2)[0].get<uint32_t>(), 3409269U);

  /// Test 3: call square -- 8256^2
  FuncArgs = {WasmEdge::ExternRef(&SS), 8256U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32};
  auto Res3 = VM.execute("call_square", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res3);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ((*Res3)[0].get<uint32_t>(), 68161536U);

  /// Test 4: call sum and square -- (210 + 654)^2
  FuncArgs = {WasmEdge::ExternRef(&AC), WasmEdge::ExternRef(&SS), 210U, 654U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::ExternRef,
                  WasmEdge::ValType::I32, WasmEdge::ValType::I32};
  auto Res4 = VM.execute("call_add_square", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res4);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ((*Res4)[0].get<uint32_t>(), 746496U);
}

TEST(ExternRefTest, Ref__STL) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::ExternMod ExtMod;
  std::vector<WasmEdge::ValVariant> FuncArgs;
  std::vector<WasmEdge::ValType> FuncArgTypes;
  VM.registerModule(ExtMod);
  ASSERT_TRUE(VM.loadWasm("externrefTestData/stl.wasm"));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  /// STL Instances
  std::stringstream STLSS;
  std::string STLStr, STLStrKey, STLStrVal;
  std::vector<uint32_t> STLVec;
  std::map<std::string, std::string> STLMap;
  std::set<uint32_t> STLSet;

  /// Test 1: call ostream << std::string
  STLStr = "hello world!";
  FuncArgs = {WasmEdge::ExternRef(&STLSS), WasmEdge::ExternRef(&STLStr)};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::ExternRef};
  auto Res1 = VM.execute("call_ostream_str", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res1);
  EXPECT_EQ((*Res1).size(), 0U);
  EXPECT_EQ(STLSS.str(), "hello world!");

  /// Test 2: call ostream << uint32_t
  FuncArgs = {WasmEdge::ExternRef(&STLSS), 123456U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32};
  auto Res2 = VM.execute("call_ostream_u32", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res2);
  EXPECT_EQ((*Res2).size(), 0U);
  EXPECT_EQ(STLSS.str(), "hello world!123456");

  /// Test 3: call map insert {key, val}
  STLStrKey = "one";
  STLStrVal = "1";
  FuncArgs = {WasmEdge::ExternRef(&STLMap), WasmEdge::ExternRef(&STLStrKey),
              WasmEdge::ExternRef(&STLStrVal)};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::ExternRef,
                  WasmEdge::ValType::ExternRef};
  auto Res3 = VM.execute("call_map_insert", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res3);
  EXPECT_EQ((*Res3).size(), 0U);
  EXPECT_NE(STLMap.find(STLStrKey), STLMap.end());
  EXPECT_EQ(STLMap.find(STLStrKey)->second, STLStrVal);

  /// Test 4: call map erase {key}
  STLStrKey = "one";
  FuncArgs = {WasmEdge::ExternRef(&STLMap), WasmEdge::ExternRef(&STLStrKey)};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::ExternRef};
  auto Res4 = VM.execute("call_map_erase", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res4);
  EXPECT_EQ((*Res4).size(), 0U);
  EXPECT_EQ(STLMap.find(STLStrKey), STLMap.end());

  /// Test 5: call set insert {key}
  FuncArgs = {WasmEdge::ExternRef(&STLSet), 3456U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32};
  auto Res5 = VM.execute("call_set_insert", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res5);
  EXPECT_EQ((*Res5).size(), 0U);
  EXPECT_NE(STLSet.find(3456U), STLSet.end());

  /// Test 6: call set erase {key}
  FuncArgs = {WasmEdge::ExternRef(&STLSet), 3456U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32};
  auto Res6 = VM.execute("call_set_erase", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res6);
  EXPECT_EQ((*Res6).size(), 0U);
  EXPECT_EQ(STLSet.find(3456U), STLSet.end());

  /// Test 7: call vector push {val}
  STLVec = {10, 20, 30, 40, 50, 60, 70, 80, 90};
  FuncArgs = {WasmEdge::ExternRef(&STLVec), 100U};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::I32};
  auto Res7 = VM.execute("call_vector_push", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res7);
  EXPECT_EQ((*Res7).size(), 0U);
  EXPECT_EQ(STLVec.size(), 10U);
  EXPECT_EQ(STLVec[9], 100U);

  /// Test 8: call vector[3:8) sum
  auto ItBegin = STLVec.begin() + 3;
  auto ItEnd = STLVec.end() - 2;
  FuncArgs = {WasmEdge::ExternRef(&ItBegin), WasmEdge::ExternRef(&ItEnd)};
  FuncArgTypes = {WasmEdge::ValType::ExternRef, WasmEdge::ValType::ExternRef};
  auto Res8 = VM.execute("call_vector_sum", FuncArgs, FuncArgTypes);
  ASSERT_TRUE(Res8);
  EXPECT_EQ((*Res8).size(), 1U);
  EXPECT_EQ((*Res8)[0].get<uint32_t>(), 40U + 50U + 60U + 70U + 80U);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
