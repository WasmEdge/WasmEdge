// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "zlibfunc.h"
#include "zlibmodule.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
WasmEdge::Runtime::CallingFrame DummyCallFrame(nullptr, nullptr);

WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_zlib/"
      "libwasmedgePluginWasmEdgeZlib" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_zlib"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_zlib"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

} // namespace

TEST(WasmEdgeZlibTest, Module) {
  // Create the wasmedge_process module instance.
  auto *ZlibMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeZlibModule *>(createModule());
  EXPECT_FALSE(ZlibMod == nullptr);
  EXPECT_TRUE(ZlibMod->getEnv().ZStreamMap.empty());
  EXPECT_EQ(ZlibMod->getFuncExportNum(), 6U);
  EXPECT_NE(ZlibMod->findFuncExports("wasmedge_zlib_deflateInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("wasmedge_zlib_inflateInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("wasmedge_zlib_deflate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("wasmedge_zlib_inflate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("wasmedge_zlib_deflateEnd"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("wasmedge_zlib_inflateEnd"), nullptr);

  delete ZlibMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
