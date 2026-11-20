// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "common/filesystem.h"
#include "runtime/instance/module.h"
#include "tensorflowlite_func.h"
#include "tensorflowlite_module.h"

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

std::unique_ptr<WasmEdge::Host::WasmEdgeTensorflowLiteModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(WasmEdge::u8path(
      "../../../plugins/wasmedge_tensorflowlite/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeTensorflowLite" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_tensorflowlite"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_tensorflowlite"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeTensorflowLiteModule>(
          Module->create());
    }
  }
  return {};
}

} // namespace

// TODO: add unit tests for every function.

TEST(WasmEdgeTensorflowLiteTest, Module) {
  // Create the wasmedge_tensorflowlite module instance.
  auto TFLiteMod = createModule();
  ASSERT_TRUE(TFLiteMod);
  EXPECT_EQ(TFLiteMod->getFuncExportNum(), 7U);
  EXPECT_NE(TFLiteMod->findFuncExports("create_session"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("delete_session"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("run_session"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("get_output_tensor"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("get_tensor_len"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("get_tensor_data"), nullptr);
  EXPECT_NE(TFLiteMod->findFuncExports("append_input"), nullptr);
}

TEST(WasmEdgeTensorflowLiteTest, SessionTableGrowth) {
  // Create the wasmedge_tensorflowlite module instance.
  auto TFLiteMod = createModule();
  ASSERT_TRUE(TFLiteMod);
  auto &Env = TFLiteMod->getEnv();

  // Existing sessions keep their address when the session table grows.
  constexpr uint32_t SessionCount = 40U;
  std::array<decltype(Env.getContext(0U)), SessionCount> Sessions;
  for (uint32_t I = 0; I < SessionCount; ++I) {
    EXPECT_EQ(Env.newContext(), I);
    Sessions[I] = Env.getContext(I);
    ASSERT_NE(Sessions[I], nullptr);
  }
  for (uint32_t I = 0; I < SessionCount; ++I) {
    EXPECT_EQ(Env.getContext(I), Sessions[I]);
  }
  EXPECT_EQ(Env.getContext(SessionCount), nullptr);

  // The deleted session ID is recycled.
  Env.deleteContext(3U);
  EXPECT_EQ(Env.getContext(3U), nullptr);
  EXPECT_EQ(Env.newContext(), 3U);
  EXPECT_NE(Env.getContext(3U), nullptr);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
