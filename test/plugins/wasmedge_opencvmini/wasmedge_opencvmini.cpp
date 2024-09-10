// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

std::unique_ptr<WasmEdge::Host::WasmEdgeOpenCVMiniModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_opencvmini/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeOpenCVMini" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_opencvmini"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_opencvmini"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeOpenCVMiniModule>(
          Module->create());
    }
  }
  return {};
}

} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeOpecvminiTest, Module) {
  // Create the wasmedge_opencvmini module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 19U);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_imdecode"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_imencode"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_rectangle"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_cvt_color"), nullptr);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
