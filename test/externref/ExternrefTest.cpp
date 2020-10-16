// SPDX-License-Identifier: Apache-2.0
#include "ExternrefTest.h"
#include "common/value.h"
#include "vm/configure.h"
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
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::ExternMod ExtMod;
  std::vector<SSVM::ValVariant> FuncArgs;
  VM.registerModule(ExtMod);
  ASSERT_TRUE(VM.loadWasm("externrefTestData/funcs.wasm"));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  /// Functor instance
  SquareStruct SS;
  /// Class instance
  AddClass AC;

  /// Test 1: call add -- 1234 + 5678
  FuncArgs = {SSVM::genExternRef(&AC), 1234U, 5678U};
  auto Res1 = VM.execute("call_add", FuncArgs);
  ASSERT_TRUE(Res1);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ(std::get<uint32_t>((*Res1)[0]), 6912U);

  /// Test 2: call mul -- 789 * 4321
  FuncArgs = {SSVM::genExternRef(MulFunc), 789U, 4321U};
  auto Res2 = VM.execute("call_mul", FuncArgs);
  ASSERT_TRUE(Res2);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ(std::get<uint32_t>((*Res2)[0]), 3409269U);

  /// Test 3: call square -- 8256^2
  FuncArgs = {SSVM::genExternRef(&SS), 8256U};
  auto Res3 = VM.execute("call_square", FuncArgs);
  ASSERT_TRUE(Res3);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ(std::get<uint32_t>((*Res3)[0]), 68161536U);

  /// Test 4: call sum and square -- (210 + 654)^2
  FuncArgs = {SSVM::genExternRef(&AC), SSVM::genExternRef(&SS), 210U, 654U};
  auto Res4 = VM.execute("call_add_square", FuncArgs);
  ASSERT_TRUE(Res4);
  EXPECT_EQ((*Res1).size(), 1U);
  EXPECT_EQ(std::get<uint32_t>((*Res4)[0]), 746496U);
}

TEST(ExternRefTest, Ref__STL) {
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::ExternMod ExtMod;
  std::vector<SSVM::ValVariant> FuncArgs;
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
  FuncArgs = {SSVM::genExternRef(&STLSS), SSVM::genExternRef(&STLStr)};
  auto Res1 = VM.execute("call_ostream_str", FuncArgs);
  ASSERT_TRUE(Res1);
  EXPECT_EQ((*Res1).size(), 0U);
  EXPECT_EQ(STLSS.str(), "hello world!");

  /// Test 2: call ostream << uint32_t
  FuncArgs = {SSVM::genExternRef(&STLSS), 123456U};
  auto Res2 = VM.execute("call_ostream_u32", FuncArgs);
  ASSERT_TRUE(Res2);
  EXPECT_EQ((*Res2).size(), 0U);
  EXPECT_EQ(STLSS.str(), "hello world!123456");

  /// Test 3: call map insert {key, val}
  STLStrKey = "one";
  STLStrVal = "1";
  FuncArgs = {SSVM::genExternRef(&STLMap), SSVM::genExternRef(&STLStrKey),
              SSVM::genExternRef(&STLStrVal)};
  auto Res3 = VM.execute("call_map_insert", FuncArgs);
  ASSERT_TRUE(Res3);
  EXPECT_EQ((*Res3).size(), 0U);
  EXPECT_NE(STLMap.find(STLStrKey), STLMap.end());
  EXPECT_EQ(STLMap.find(STLStrKey)->second, STLStrVal);

  /// Test 4: call map erase {key}
  STLStrKey = "one";
  FuncArgs = {SSVM::genExternRef(&STLMap), SSVM::genExternRef(&STLStrKey)};
  auto Res4 = VM.execute("call_map_erase", FuncArgs);
  ASSERT_TRUE(Res4);
  EXPECT_EQ((*Res4).size(), 0U);
  EXPECT_EQ(STLMap.find(STLStrKey), STLMap.end());

  /// Test 5: call set insert {key}
  FuncArgs = {SSVM::genExternRef(&STLSet), 3456U};
  auto Res5 = VM.execute("call_set_insert", FuncArgs);
  ASSERT_TRUE(Res5);
  EXPECT_EQ((*Res5).size(), 0U);
  EXPECT_NE(STLSet.find(3456U), STLSet.end());

  /// Test 6: call set erase {key}
  FuncArgs = {SSVM::genExternRef(&STLSet), 3456U};
  auto Res6 = VM.execute("call_set_erase", FuncArgs);
  ASSERT_TRUE(Res6);
  EXPECT_EQ((*Res6).size(), 0U);
  EXPECT_EQ(STLSet.find(3456U), STLSet.end());

  /// Test 7: call vector push {val}
  STLVec = {10, 20, 30, 40, 50, 60, 70, 80, 90};
  FuncArgs = {SSVM::genExternRef(&STLVec), 100U};
  auto Res7 = VM.execute("call_vector_push", FuncArgs);
  ASSERT_TRUE(Res7);
  EXPECT_EQ((*Res7).size(), 0U);
  EXPECT_EQ(STLVec.size(), 10U);
  EXPECT_EQ(STLVec[9], 100U);

  /// Test 8: call vector[3:8) sum
  auto ItBegin = STLVec.begin() + 3;
  auto ItEnd = STLVec.end() - 2;
  FuncArgs = {SSVM::genExternRef(&ItBegin), SSVM::genExternRef(&ItEnd)};
  auto Res8 = VM.execute("call_vector_sum", FuncArgs);
  ASSERT_TRUE(Res8);
  EXPECT_EQ((*Res8).size(), 1U);
  EXPECT_EQ(std::get<uint32_t>((*Res8)[0]), 40U + 50U + 60U + 70U + 80U);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  SSVM::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
