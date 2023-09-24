#include "common/defines.h"
#include "runtime/instance/module.h"
#include "wasi_random/func.h"
#include "wasi_random/module.h"

#include <gtest/gtest.h>

namespace {

#ifndef WASMEDGE_LIB_PREFIX
#if WASMEDGE_OS_WINDOWS
#define WASMEDGE_LIB_PREFIX ""
#else
#define WASMEDGE_LIB_PREFIX "lib"
#endif
#endif

WasmEdge::Runtime::Instance::ModuleInstance *createRandomModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasi_random/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasiRandom" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_random"sv)) {
    if (const auto *Module = Plugin->findModule("wasi:random/random"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

WasmEdge::Runtime::Instance::ModuleInstance *createInsecureModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasi_random/"
      "libwasmedgePluginWasiRandom" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_random"sv)) {
    if (const auto *Module = Plugin->findModule("wasi:random/insecure"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

WasmEdge::Runtime::Instance::ModuleInstance *createInsecureSeedModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasi_random/"
      "libwasmedgePluginWasiRandom" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_random"sv)) {
    if (const auto *Module =
            Plugin->findModule("wasi:random/insecure-seed"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}
} // namespace

TEST(WasiRandomModTests, Module) {
  // Create the wasi-random module instance.
  auto WasiRandomMod =
      dynamic_cast<WasmEdge::Host::WasiRandomModule *>(createRandomModule());
  EXPECT_NE(WasiRandomMod, nullptr);
  EXPECT_NE(WasiRandomMod->findFuncExports("get_random_bytes"), nullptr);
  EXPECT_NE(WasiRandomMod->findFuncExports("get_random_u64"), nullptr);

  auto WasiInseureRandomMod =
      dynamic_cast<WasmEdge::Host::WasiRandomInsecureModule *>(
          createInsecureModule());
  EXPECT_NE(WasiInseureRandomMod, nullptr);
  EXPECT_NE(WasiInseureRandomMod->findFuncExports("get_insecure_random_bytes"),
            nullptr);
  EXPECT_NE(WasiInseureRandomMod->findFuncExports("get_insecure_random_u64"),
            nullptr);

  auto WasiInseureSeedMod =
      dynamic_cast<WasmEdge::Host::WasiRandomInsecureSeedModule *>(
          createInsecureSeedModule());
  EXPECT_NE(WasiInseureSeedMod, nullptr);
  EXPECT_NE(WasiInseureSeedMod->findFuncExports("insecure_seed"), nullptr);

  delete WasiRandomMod;
  delete WasiInseureRandomMod;
  delete WasiInseureSeedMod;
}

TEST(WasiRandomModTests, Random) {
  // Create the wasi-random module instance.
  auto WasiRandomMod =
      dynamic_cast<WasmEdge::Host::WasiRandomModule *>(createRandomModule());
  EXPECT_NE(WasiRandomMod, nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  EXPECT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 128).
  fillMemContent(MemInst, 0, 128);

  // Get the function "get_random_bytes"
  auto *FuncInst = WasiRandomMod->findFuncExports("get_random_bytes");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &WasiGetRandomBytes = dynamic_cast<WasmEdge::Host::WasiGetRandomBytes &>(
      FuncInst->getHostFunc());

  EXPECT_TRUE(
      WasiGetRandomBytes.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(128), UINT32_C(0), UINT32_C(128)},
                             {}));

  // No enough space
  EXPECT_FALSE(
      WasiGetRandomBytes.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(128), UINT32_C(0), UINT32_C(10)},
                             {}));

  delete WasiRandomMod;
}

TEST(WasiRandomModTests, Insecure) {
  // Create the wasi-random insecure module instance.
  auto WasiRandomInsecureMod =
      dynamic_cast<WasmEdge::Host::WasiRandomInsecureModule *>(
          createInsecureModule());
  EXPECT_NE(WasiRandomInsecureMod, nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  EXPECT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 128).
  fillMemContent(MemInst, 0, 128);

  // Get the function "get_insecure_random_bytes"
  auto *FuncInst =
      WasiRandomInsecureMod->findFuncExports("get_insecure_random_bytes");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &WasiGetInsecureRandomBytes =
      dynamic_cast<WasmEdge::Host::WasiGetInsecureRandomBytes &>(
          FuncInst->getHostFunc());

  EXPECT_TRUE(WasiGetInsecureRandomBytes.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(128), UINT32_C(0),
                                                  UINT32_C(128)},
      {}));

  // No enough space
  EXPECT_FALSE(WasiGetInsecureRandomBytes.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(128), UINT32_C(0),
                                                  UINT32_C(10)},
      {}));

  delete WasiRandomInsecureMod;
}

TEST(WasiRandomModTests, InsecureSeed) {
  // Create the wasi-random insecure-seed module instance.
  auto WasiInsecureSeedMod =
      dynamic_cast<WasmEdge::Host::WasiRandomInsecureSeedModule *>(
          createInsecureSeedModule());
  EXPECT_NE(WasiInsecureSeedMod, nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  EXPECT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 16).
  fillMemContent(MemInst, 0, 16);

  // Get the function "get_insecure_random_bytes"
  auto *FuncInst = WasiInsecureSeedMod->findFuncExports("insecure_seed");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &WasiInsecureSeed =
      dynamic_cast<WasmEdge::Host::WasiInsecureSeed &>(FuncInst->getHostFunc());

  EXPECT_TRUE(WasiInsecureSeed.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(8)},
      {}));

  delete WasiInsecureSeedMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
