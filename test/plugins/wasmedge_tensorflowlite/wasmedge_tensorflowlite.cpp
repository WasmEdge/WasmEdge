// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "runtime/instance/module.h"
#include "tensorflowlite_func.h"
#include "tensorflowlite_module.h"

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
      "../../../plugins/wasmedge_tensorflowlite/"
      "libwasmedgePluginWasmEdgeTensorflowLite" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_tensorflowlite"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_tensorflowlite"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeTensorflowLiteTest, Module) {
  // Create the wasmedge_tensorflowlite module instance.
  auto *TFLiteMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeTensorflowLiteModule *>(
          createModule());
  EXPECT_FALSE(TFLiteMod == nullptr);
  EXPECT_EQ(TFLiteMod->getFuncExportNum(), 7U);
  EXPECT_NE(TFLiteMod->findFuncExports("create_session"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("delete_session"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("run_session"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("get_output_tensor"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("get_tensor_len"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("get_tensor_data"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("append_input"), nullptr);
  delete TFLiteMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
