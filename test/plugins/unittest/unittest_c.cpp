// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "wasmedge/wasmedge.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

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

TEST(wasmedgePluginTests, C_DefaultPaths) {
  // Without WASMEDGE_PLUGIN_PATH, the plug-in installed next to the WasmEdge
  // library is loaded. Only the C plug-in is installed there.
  {
    ASSERT_EQ(
        WasmEdge_PluginFind(WasmEdge_StringWrap("wasmedge_plugintest_c", 21U)),
        nullptr);
#if WASMEDGE_OS_WINDOWS
    ::_putenv_s("WASMEDGE_PLUGIN_PATH", "");
    const auto Dir = std::filesystem::path(WASMEDGE_LIB_DIR "/../lib/wasmedge")
                         .lexically_normal();
#else
    ::unsetenv("WASMEDGE_PLUGIN_PATH");
    const auto Dir = std::filesystem::path(WASMEDGE_LIB_DIR "/wasmedge");
#endif
    const std::filesystem::path File =
        WASMEDGE_LIB_PREFIX "wasmedgePluginTestModuleC" WASMEDGE_LIB_EXTENSION;
    // The first directory created for the test, removed at the end.
    auto Top = Dir;
    while (!std::filesystem::exists(Top.parent_path())) {
      Top = Top.parent_path();
    }
    struct DirCleanup {
      std::filesystem::path Path;
      std::filesystem::path Library;
      ~DirCleanup() {
        // A loaded library cannot be removed on Windows, so move it out first.
        std::error_code Error;
        const auto Moved = Library.filename().string() + ".loaded";
        std::filesystem::rename(Library, Moved, Error);
        std::filesystem::remove(Moved, Error);
        std::filesystem::remove_all(Path, Error);
      }
    } Cleanup{Top, Dir / File};
    std::error_code Error;
    std::filesystem::create_directories(Dir, Error);
    ASSERT_FALSE(Error);
    std::filesystem::copy_file(
        File, Dir / File, std::filesystem::copy_options::overwrite_existing,
        Error);
    ASSERT_FALSE(Error);
    WasmEdge_PluginLoadWithDefaultPaths();
    EXPECT_NE(
        WasmEdge_PluginFind(WasmEdge_StringWrap("wasmedge_plugintest_c", 21U)),
        nullptr);
    EXPECT_EQ(WasmEdge_PluginFind(
                  WasmEdge_StringWrap("wasmedge_plugintest_cpp", 23U)),
              nullptr);
  }

#if WASMEDGE_OS_WINDOWS
  const std::string Separator = ";";
#else
  const std::string Separator = ":";
#endif
  // The list starts and ends with a separator, and the entry is an absolute
  // path, which contains the colon of the drive on Windows.
  const std::string Paths =
      Separator + std::filesystem::current_path().string() + Separator;
#if WASMEDGE_OS_WINDOWS
  ASSERT_EQ(::_putenv_s("WASMEDGE_PLUGIN_PATH", Paths.c_str()), 0);
#else
  ASSERT_EQ(::setenv("WASMEDGE_PLUGIN_PATH", Paths.c_str(), 1), 0);
#endif
  WasmEdge_PluginLoadWithDefaultPaths();
#if WASMEDGE_OS_WINDOWS
  ::_putenv_s("WASMEDGE_PLUGIN_PATH", "");
#else
  ::unsetenv("WASMEDGE_PLUGIN_PATH");
#endif
  EXPECT_NE(
      WasmEdge_PluginFind(WasmEdge_StringWrap("wasmedge_plugintest_c", 21U)),
      nullptr);
  EXPECT_NE(
      WasmEdge_PluginFind(WasmEdge_StringWrap("wasmedge_plugintest_cpp", 23U)),
      nullptr);
}

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
