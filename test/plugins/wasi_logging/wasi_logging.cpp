// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "plugin/wasi_logging/func.h"
#include "plugin/wasi_logging/module.h"

#include "common/defines.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>
#include <memory>
#include <sstream>

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

std::unique_ptr<WasmEdge::Host::WasiLoggingModule> createModule() {
  using namespace std::literals::string_view_literals;
  // The built-in plugins are loaded when loading from default paths.
  WasmEdge::Plugin::Plugin::loadFromDefaultPaths();
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_logging"sv)) {
    if (const auto *Module = Plugin->findModule("wasi:logging/logging"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasiLoggingModule>(
          Module->create());
    }
  }
  return {};
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, std::string_view Str) noexcept {
  char *Buf = MemInst.getPointer<char *>(Offset);
  std::copy_n(Str.data(), Str.length(), Buf);
}

} // namespace

TEST(WasiLoggingTests, func_log) {
  using namespace std::literals::string_view_literals;
  // Create the wasi-logging module instance.
  // Here create 2 wasi-logging modules for testing in multiple modules.
  auto WasiLoggingMod1 = createModule();
  ASSERT_TRUE(WasiLoggingMod1);
  auto WasiLoggingMod2 = createModule();
  ASSERT_TRUE(WasiLoggingMod2);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  EXPECT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 256].
  fillMemContent(MemInst, 0, 256);
  // Set strings in memory.
  fillMemContent(MemInst, 0, "stdout"sv);
  fillMemContent(MemInst, 8, "stderr"sv);
  fillMemContent(MemInst, 16, "out.log"sv);
  fillMemContent(MemInst, 24, "out2.log"sv);
  fillMemContent(MemInst, 128, "This is log message"sv);
  fillMemContent(MemInst, 160, "Message 1 to file"sv);
  fillMemContent(MemInst, 192, "Message 2 to file"sv);
  fillMemContent(MemInst, 224, "Message 3 to file"sv);

  // Get the function "log".
  auto *FuncInst1 = WasiLoggingMod1->findFuncExports("log");
  auto *FuncInst2 = WasiLoggingMod2->findFuncExports("log");
  EXPECT_NE(FuncInst1, nullptr);
  EXPECT_NE(FuncInst2, nullptr);
  EXPECT_TRUE(FuncInst1->isHostFunction());
  EXPECT_TRUE(FuncInst2->isHostFunction());
  auto &HostFuncInst1 = dynamic_cast<WasmEdge::Host::WASILogging::Log &>(
      FuncInst1->getHostFunc());
  auto &HostFuncInst2 = dynamic_cast<WasmEdge::Host::WASILogging::Log &>(
      FuncInst2->getHostFunc());

  // Show All Level
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(1), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(2), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(3), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(4), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(5), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));

  // Stderr Context
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(8), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_TRUE(HostFuncInst2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(8), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));

  // Log to out.txt: message 1
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(16), UINT32_C(7), UINT32_C(160), UINT32_C(17)},
      {}));
  EXPECT_TRUE(HostFuncInst2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(16), UINT32_C(7), UINT32_C(160), UINT32_C(17)},
      {}));
  // Log to out2.txt: message 2
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(24), UINT32_C(8), UINT32_C(192), UINT32_C(17)},
      {}));
  EXPECT_TRUE(HostFuncInst2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(24), UINT32_C(8), UINT32_C(192), UINT32_C(17)},
      {}));
  // Log to out.txt: message 3
  EXPECT_TRUE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(16), UINT32_C(7), UINT32_C(224), UINT32_C(17)},
      {}));
  EXPECT_TRUE(HostFuncInst2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(16), UINT32_C(7), UINT32_C(224), UINT32_C(17)},
      {}));

  // UnKnown Level
  EXPECT_FALSE(HostFuncInst1.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(6), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
  EXPECT_FALSE(HostFuncInst2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(6), UINT32_C(0), UINT32_C(6), UINT32_C(128), UINT32_C(19)},
      {}));
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
