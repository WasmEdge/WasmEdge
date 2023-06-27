// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "image_func.h"
#include "image_module.h"
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
      "../../../plugins/wasmedge_image/"
      "libwasmedgePluginWasmEdgeImage" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_image"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_image"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeImageTest, Module) {
  // Create the wasmedge_image module instance.
  auto *ImgMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeImageModule *>(createModule());
  EXPECT_FALSE(ImgMod == nullptr);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 2U);
  EXPECT_NE(ImgMod->findFuncExports("load_jpg"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("load_png"), nullptr);
  delete ImgMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
