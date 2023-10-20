// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "runtime/instance/module.h"
#include "tensorflow_func.h"
#include "tensorflow_module.h"

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
      "../../../plugins/wasmedge_tensorflow/"
      "libwasmedgePluginWasmEdgeTensorflow" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_tensorflow"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_tensorflow"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeTensorflowTest, Module) {
  // Create the wasmedge_tensorflow module instance.
  auto *TFMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeTensorflowModule *>(createModule());
  EXPECT_FALSE(TFMod == nullptr);
  EXPECT_EQ(TFMod->getFuncExportNum(), 11U);
  EXPECT_NE(TFMod->findFuncExports("create_session"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("create_session_saved_model"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("delete_session"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("run_session"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("get_output_tensor"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("get_tensor_len"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("get_tensor_data"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("append_input"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("append_output"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("clear_input"), nullptr);
  EXPECT_NE(TFMod->findFuncExports("clear_output"), nullptr);
  delete TFMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
