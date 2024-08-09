#include "common/defines.h"
#include "plugin/plugin.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "types.h"
#include "wasillmmodule.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using WasmEdge::Host::WASILLM::ErrNo;

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(
      std::filesystem::u8path("../../../plugins/wasi_llm/" WASMEDGE_LIB_PREFIX
                              "wasmedgePluginWasiLLM" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_llm"sv)) {
    if (const auto *Module = Plugin->findModule("wasi_llm"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

TEST(WasiLLMTest, TrainGPT2) {
  // Create wasi_llm module instance.
  auto *LLMMod = dynamic_cast<WasmEdge::Host::WasiLLMModule *>(createModule());
  EXPECT_NE(LLMMod, nullptr);
  EXPECT_EQ(LLMMod->getFuncExportNum(), 4U);

  // Create the calling frame with memory instance.
  // WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  // Mod.addHostMemory(
  //     "memory",
  //     std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
  //                   WasmEdge::AST::MemoryType(60000)));
  // auto *MemInstPtr = Mod.findMemoryExports("memory");
  // EXPECT_NE(MemInstPtr, nullptr);
  // auto &MemInst = *MemInstPtr;
  // WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // auto *ModelCreate = LLMMod->findFuncExports("");
  // EXPECT_NE(ModelCreate, nullptr);
  // EXPECT_TRUE(ModelCreate->isHostFunction());
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
