// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "testplugin.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "./libwasmedgePluginTestModule" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_plugintest"sv)) {
    WasmEdge::PO::ArgumentParser Parser;
    Plugin->registerOptions(Parser);
    Parser.set_raw_value<std::string>("name"sv, std::string("test_name"));
    Parser.set_raw_value<std::vector<std::string>>(
        "arg"sv, std::vector<std::string>({"arg0", "arg1", "arg2", "arg3"}));
    if (const auto *Module = Plugin->findModule("wasmedge_plugintest"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

TEST(wasmedgePluginTests, CPP_Run) {
  // Create the wasmedge_plugintest module instance.
  auto *TestMod =
      dynamic_cast<WasmEdge::Host::WasmEdgePluginTestModule *>(createModule());
  EXPECT_FALSE(TestMod == nullptr);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  // Get the function "arg_len".
  auto *FuncInst1 = TestMod->findFuncExports("arg_len");
  EXPECT_NE(FuncInst1, nullptr);
  EXPECT_TRUE(FuncInst1->isHostFunction());
  auto &HostFuncInst1 =
      dynamic_cast<WasmEdge::Host::WasmEdgePluginTestFuncArgLen &>(
          FuncInst1->getHostFunc());

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst1.run(CallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 4);

  // Get the function "name_size".
  auto *FuncInst2 = TestMod->findFuncExports("name_size");
  EXPECT_NE(FuncInst2, nullptr);
  EXPECT_TRUE(FuncInst2->isHostFunction());
  auto &HostFuncInst2 =
      dynamic_cast<WasmEdge::Host::WasmEdgePluginTestFuncNameSize &>(
          FuncInst2->getHostFunc());

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst2.run(CallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 9);

  delete TestMod;
}

TEST(wasmedgePluginTests, CPP_Module) {
  // Create the wasmedge_plugintest module instance.
  auto *TestMod =
      dynamic_cast<WasmEdge::Host::WasmEdgePluginTestModule *>(createModule());
  EXPECT_FALSE(TestMod == nullptr);
  EXPECT_EQ(TestMod->getFuncExportNum(), 4U);
  EXPECT_NE(TestMod->findFuncExports("add"), nullptr);
  EXPECT_NE(TestMod->findFuncExports("sub"), nullptr);
  EXPECT_NE(TestMod->findFuncExports("arg_len"), nullptr);
  EXPECT_NE(TestMod->findFuncExports("name_size"), nullptr);
  delete TestMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
