// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "func.h"
#include "module.h"
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
      "../../../plugins/wasi_clocks/"
      "libwasmedgePluginWasiClocks" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_wasi_clocks"sv)) {
    if (const auto *Module = Plugin->findModule("wasi_clocks"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

TEST(WasmedgeWasiClocksTests, Module) {
  // Create the wasmedge_wasi_clocks module instance.
  auto *ImgMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeWasiClocksModule *>(createModule());
  EXPECT_FALSE(ImgMod == nullptr);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 7U);
  EXPECT_NE(ImgMod->findFuncExports("wasi_clocks_monotonic_clock_now"),
            nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasi_clocks_monotonic_clock_resolution"),
            nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasi_clocks_wall_clock_now"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasi_clocks_wall_clock_resolution"),
            nullptr);
  delete ImgMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
