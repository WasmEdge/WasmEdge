// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "opencvmini_func.h"
#include "opencvmini_module.h"
#include "runtime/instance/module.h"

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
      "../../../plugins/wasmedge_opencvmini/"
      "libwasmedgePluginWasmEdgeOpenCVMini" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_opencvmini"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_opencvmini"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeOpecvminiTest, Module) {
  // Create the wasmedge_opencvmini module instance.
  auto *ImgMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeOpenCVMiniModule *>(createModule());
  EXPECT_FALSE(ImgMod == nullptr);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 19U);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_imdecode"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_imencode"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_rectangle"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_cvt_color"), nullptr);
  delete ImgMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
