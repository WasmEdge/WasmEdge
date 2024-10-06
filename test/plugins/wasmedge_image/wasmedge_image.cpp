// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "image_func.h"
#include "image_module.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
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

std::unique_ptr<WasmEdge::Host::WasmEdgeImageModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_image/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeImage" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_image"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_image"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeImageModule>(
          Module->create());
    }
  }
  return {};
}

} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeImageTest, Module) {
  // Create the wasmedge_image module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 2U);
  EXPECT_NE(ImgMod->findFuncExports("load_jpg"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("load_png"), nullptr);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
