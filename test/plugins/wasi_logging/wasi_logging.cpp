#include "common/defines.h"
#include "runtime/instance/module.h"
#include "wasi_logging/func.h"
#include "wasi_logging/module.h"

#include <gtest/gtest.h>
#include <iostream>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace {

WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasi_logging/"
      "libwasmedgePluginWasiLogging" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_logging"sv)) {
    if (const auto *Module = Plugin->findModule("wasi:logging/logging"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, const std::string &Str) noexcept {
  char *Buf = MemInst.getPointer<char *>(Offset);
  std::copy_n(Str.c_str(), Str.length(), Buf);
}

} // namespace

TEST(WasiLoggingTests, func_log) {
  // Create the wasi-logging module instance.
  auto WasiLoggingMod =
      dynamic_cast<WasmEdge::Host::WasiLoggingModule *>(createModule());
  EXPECT_NE(WasiLoggingMod, nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  EXPECT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 32].
  fillMemContent(MemInst, 0, 32);
  // Set strings in memory
  fillMemContent(MemInst, 0, std::string("CxtStr"));
  fillMemContent(MemInst, 8, std::string("stderr"));
  fillMemContent(MemInst, 16, std::string("MsgStr"));

  // Get the function "log"
  auto *FuncInst = WasiLoggingMod->findFuncExports("log");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst =
      dynamic_cast<WasmEdge::Host::WasiLoggingLog &>(FuncInst->getHostFunc());

  // Show All Level
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(1), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(2), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));

  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(3), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(4), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(5), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_FALSE(WasiLoggingMod->getEnv().isCxtStrStderr);

  // Stderr Context
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(8), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_TRUE(WasiLoggingMod->getEnv().isCxtStrStderr);

  // UnKnown Level
  EXPECT_FALSE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(6), UINT32_C(0), UINT32_C(6), UINT32_C(16), UINT32_C(6)},
      {}));
  EXPECT_FALSE(WasiLoggingMod->getEnv().isCxtStrStderr);

  delete WasiLoggingMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
