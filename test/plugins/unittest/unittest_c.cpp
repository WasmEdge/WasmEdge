// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "wasmedge/wasmedge.h"

#include <gtest/gtest.h>

#include <cstring>

namespace {
WasmEdge_ModuleInstanceContext *createModuleC() {
  WasmEdge_PluginLoadFromPath(
      "./" WASMEDGE_LIB_PREFIX
      "wasmedgePluginTestModuleC" WASMEDGE_LIB_EXTENSION);
  WasmEdge_String Str = WasmEdge_StringCreateByCString("wasmedge_plugintest_c");
  const WasmEdge_PluginContext *PluginCxt = WasmEdge_PluginFind(Str);
  WasmEdge_StringDelete(Str);
  if (!PluginCxt) {
    return nullptr;
  }

  Str = WasmEdge_StringCreateByCString("wasmedge_plugintest_c_module");
  WasmEdge_ModuleInstanceContext *ModCxt =
      WasmEdge_PluginCreateModule(PluginCxt, Str);
  WasmEdge_StringDelete(Str);
  return ModCxt;
}

WasmEdge_ModuleInstanceContext *createModuleCPP() {
  WasmEdge_PluginLoadFromPath(
      "./" WASMEDGE_LIB_PREFIX
      "wasmedgePluginTestModuleCPP" WASMEDGE_LIB_EXTENSION);
  WasmEdge_String Str =
      WasmEdge_StringCreateByCString("wasmedge_plugintest_cpp");
  const WasmEdge_PluginContext *PluginCxt = WasmEdge_PluginFind(Str);
  WasmEdge_StringDelete(Str);
  if (!PluginCxt) {
    return nullptr;
  }

  Str = WasmEdge_StringCreateByCString("wasmedge_plugintest_cpp_module");
  WasmEdge_ModuleInstanceContext *ModCxt =
      WasmEdge_PluginCreateModule(PluginCxt, Str);
  WasmEdge_StringDelete(Str);
  return ModCxt;
}
} // namespace

TEST(wasmedgePluginTests, C_Run) {
  auto *ExecCxt = WasmEdge_ExecutorCreate(nullptr, nullptr);
  auto *StoreCxt = WasmEdge_StoreCreate();
  WasmEdge_Result Res;
  WasmEdge_FunctionInstanceContext *FuncCxt;
  WasmEdge_String FuncName;
  WasmEdge_Value Params[2], Returns[1];

  // Create the wasmedge_plugintest_c_module module instance.
  auto *ModInstC = createModuleC();
  EXPECT_FALSE(ModInstC == nullptr);
  int32_t *HostData =
      static_cast<int32_t *>(WasmEdge_ModuleInstanceGetHostData(ModInstC));
  EXPECT_FALSE(HostData == nullptr);

  // Create the wasmedge_plugintest_cpp_module module instance.
  auto *ModInstCPP = createModuleCPP();
  EXPECT_FALSE(ModInstCPP == nullptr);

  // Test: Run the function "wasmedge_plugintest_c.add".
  FuncName = WasmEdge_StringCreateByCString("add");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModInstC, FuncName);
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  Params[0] = WasmEdge_ValueGenI32(111);
  Params[1] = WasmEdge_ValueGenI32(333);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 444);
  EXPECT_EQ(*HostData, 444);

  // Test: Run the function "wasmedge_plugintest_c.sub".
  FuncName = WasmEdge_StringCreateByCString("sub");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModInstC, FuncName);
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  Params[0] = WasmEdge_ValueGenI32(666);
  Params[1] = WasmEdge_ValueGenI32(555);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 111);
  EXPECT_EQ(*HostData, 555);

  // Test: Run the function "wasmedge_plugintest_cpp.add".
  FuncName = WasmEdge_StringCreateByCString("add");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModInstCPP, FuncName);
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  Params[0] = WasmEdge_ValueGenI32(111);
  Params[1] = WasmEdge_ValueGenI32(333);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 444);

  // Test: Run the function "wasmedge_plugintest_cpp.sub".
  FuncName = WasmEdge_StringCreateByCString("sub");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModInstCPP, FuncName);
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  Params[0] = WasmEdge_ValueGenI32(666);
  Params[1] = WasmEdge_ValueGenI32(555);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 111);

  // Test: Run the function "wasmedge_plugintest_cpp.arg_len".
  FuncName = WasmEdge_StringCreateByCString("arg_len");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModInstCPP, FuncName);
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 0);

  // Test: Run the function "wasmedge_plugintest_cpp.name_size".
  FuncName = WasmEdge_StringCreateByCString("name_size");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModInstCPP, FuncName);
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(FuncCxt, nullptr);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, nullptr, 0, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 0);

  WasmEdge_ExecutorDelete(ExecCxt);
  WasmEdge_StoreDelete(StoreCxt);
  WasmEdge_ModuleInstanceDelete(ModInstC);
  WasmEdge_ModuleInstanceDelete(ModInstCPP);
}

TEST(wasmedgePluginTests, C_Module) {
  WasmEdge_String NameBuf[16];

  // Create the wasmedge_plugintest_c_module module instance.
  auto *ModInstC = createModuleC();
  ASSERT_FALSE(ModInstC == nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunctionLength(ModInstC), 2U);
  std::memset(NameBuf, 0, sizeof(WasmEdge_String) * 16);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(ModInstC, NameBuf, 16), 2U);
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[0], WasmEdge_StringWrap("add", 3U)));
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[1], WasmEdge_StringWrap("sub", 3U)));
  WasmEdge_ModuleInstanceDelete(ModInstC);

  // Create the wasmedge_plugintest_cpp_module module instance.
  auto *ModInstCPP = createModuleCPP();
  ASSERT_FALSE(ModInstCPP == nullptr);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunctionLength(ModInstCPP), 5U);
  std::memset(NameBuf, 0, sizeof(WasmEdge_String) * 16);
  EXPECT_EQ(WasmEdge_ModuleInstanceListFunction(ModInstCPP, NameBuf, 16), 5U);
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[0], WasmEdge_StringWrap("add", 3U)));
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[1], WasmEdge_StringWrap("arg_len", 7U)));
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[2], WasmEdge_StringWrap("name_size", 9U)));
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[3], WasmEdge_StringWrap("opt", 3U)));
  EXPECT_TRUE(
      WasmEdge_StringIsEqual(NameBuf[4], WasmEdge_StringWrap("sub", 3U)));
  WasmEdge_ModuleInstanceDelete(ModInstCPP);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
