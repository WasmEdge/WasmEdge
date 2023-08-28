// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "plugin/plugin.h"
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
  WasmEdge::Plugin::Plugin::load(
      std::filesystem::u8path("../../../plugins/wasmedge_rustls/"
                              "libwasmedge_rustls" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("rustls"sv)) {
    if (const auto *Module = Plugin->findModule("rustls_client"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeRUSTLSTest, Module) {
  // Create the wasmedge_rustls module instance.
  auto *TLSMod = createModule();
  EXPECT_FALSE(TLSMod == nullptr);
  EXPECT_EQ(TLSMod->getFuncExportNum(), 11U);
  EXPECT_NE(TLSMod->findFuncExports("default_config"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("new_codec"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("codec_is_handshaking"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("codec_wants"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("delete_codec"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("send_close_notify"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("process_new_packets"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("write_raw"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("write_tls"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("read_raw"), nullptr);
  EXPECT_NE(TLSMod->findFuncExports("read_tls"), nullptr);
  delete TLSMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
