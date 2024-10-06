// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "plugin/plugin.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {

std::unique_ptr<WasmEdge::Runtime::Instance::ModuleInstance> createModuleC() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "./" WASMEDGE_LIB_PREFIX
      "wasmedgePluginTestModuleC" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_plugintest_c"sv)) {
    if (const auto *Module =
            Plugin->findModule("wasmedge_plugintest_c_module"sv)) {
      return Module->create();
    }
  }
  return {};
}

std::unique_ptr<WasmEdge::Runtime::Instance::ModuleInstance> createModuleCPP() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "./" WASMEDGE_LIB_PREFIX
      "wasmedgePluginTestModuleCPP" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_plugintest_cpp"sv)) {
    WasmEdge::PO::ArgumentParser Parser;
    Plugin->registerOptions(Parser);
    Parser.set_raw_value<std::string>("name"sv, std::string("test_name"));
    Parser.set_raw_value<std::vector<std::string>>(
        "arg"sv, std::vector<std::string>({"arg0", "arg1", "arg2", "arg3"}));
    Parser.set_raw_value("opt"sv);
    if (const auto *Module =
            Plugin->findModule("wasmedge_plugintest_cpp_module"sv)) {
      return Module->create();
    }
  }
  return {};
}
} // namespace

TEST(wasmedgePluginTests, CPP_Run) {
  // Create the wasmedge_plugintest_cpp_module module instance.
  auto TestModCPP = createModuleCPP();
  ASSERT_TRUE(TestModCPP);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  // Get the function "arg_len".
  auto *FuncInst1 = TestModCPP->findFuncExports("arg_len");
  EXPECT_NE(FuncInst1, nullptr);
  EXPECT_TRUE(FuncInst1->isHostFunction());
  auto &HostFuncInst1 = FuncInst1->getHostFunc();

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst1.run(CallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 4);

  // Get the function "name_size".
  auto *FuncInst2 = TestModCPP->findFuncExports("name_size");
  EXPECT_NE(FuncInst2, nullptr);
  EXPECT_TRUE(FuncInst2->isHostFunction());
  auto &HostFuncInst2 = FuncInst2->getHostFunc();

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst2.run(CallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 9);

  // Get the function "opt".
  auto *FuncInst3 = TestModCPP->findFuncExports("opt");
  EXPECT_NE(FuncInst3, nullptr);
  EXPECT_TRUE(FuncInst3->isHostFunction());
  auto &HostFuncInst3 = FuncInst3->getHostFunc();

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst3.run(CallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);

  // Create the wasmedge_plugintest_c_module module instance.
  auto TestModC = createModuleC();
  ASSERT_TRUE(TestModC);
  // The host functions are implemented in the C API.
  // Therefore not test to invoke them here.
}

TEST(wasmedgePluginTests, CPP_Module) {
  // Create the wasmedge_plugintest_cpp_module module instance.
  auto TestModCPP = createModuleCPP();
  ASSERT_TRUE(TestModCPP);
  EXPECT_EQ(TestModCPP->getFuncExportNum(), 5U);
  EXPECT_NE(TestModCPP->findFuncExports("add"), nullptr);
  EXPECT_NE(TestModCPP->findFuncExports("sub"), nullptr);
  EXPECT_NE(TestModCPP->findFuncExports("arg_len"), nullptr);
  EXPECT_NE(TestModCPP->findFuncExports("opt"), nullptr);
  EXPECT_NE(TestModCPP->findFuncExports("name_size"), nullptr);

  // Create the wasmedge_plugintest_c_module module instance.
  auto TestModC = createModuleC();
  ASSERT_TRUE(TestModC);
  EXPECT_EQ(TestModC->getFuncExportNum(), 2U);
  EXPECT_NE(TestModC->findFuncExports("add"), nullptr);
  EXPECT_NE(TestModC->findFuncExports("sub"), nullptr);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
