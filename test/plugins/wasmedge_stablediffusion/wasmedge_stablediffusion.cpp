#include "common/defines.h"
#include "runtime/instance/module.h"
#include "sd_func.h"
#include "sd_module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  bool LoadState = WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_stablediffusion/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginStableDiffusion" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_stablediffusion"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_stablediffusion"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

// TODO: unit tests for every functions.

TEST(WasmEdgeImageTest, Module) {
  // Create the wasmedge_image module instance.
  auto *SBMod = dynamic_cast<WasmEdge::Host::SDModule *>(createModule());
  EXPECT_FALSE(SBMod == nullptr);
  EXPECT_EQ(SBMod->getFuncExportNum(), 2U);
  EXPECT_NE(SBMod->findFuncExports("create_context"), nullptr);
  EXPECT_NE(SBMod->findFuncExports("text_to_image"), nullptr);
  delete SBMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
