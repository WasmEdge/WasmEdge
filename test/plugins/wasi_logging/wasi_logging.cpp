// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "plugin/wasi_logging/func.h"
#include "plugin/wasi_logging/module.h"

#include "common/defines.h"
#include "common/filesystem.h"
#include "host/wasi/wasimodule.h"
#include "runtime/instance/module.h"

#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>
#include <string>
#include <vector>

namespace {

class WasiWiredModule : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  using ModuleInstance::ModuleInstance;
  void wireWASIModule(const ModuleInstance *M) noexcept { setWASIModule(M); }
};

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
  // Create two wasi-logging modules for testing multiple modules.
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
  fillMemContent(MemInst, 160, "Message 1 labeled"sv);
  fillMemContent(MemInst, 192, "Message 2 labeled"sv);
  fillMemContent(MemInst, 224, "Message 3 labeled"sv);

  // Get the function "log".
  auto *FuncInst1 = WasiLoggingMod1->findFuncExports("log");
  auto *FuncInst2 = WasiLoggingMod2->findFuncExports("log");
  ASSERT_NE(FuncInst1, nullptr);
  ASSERT_NE(FuncInst2, nullptr);
  ASSERT_TRUE(FuncInst1->isHostFunction());
  ASSERT_TRUE(FuncInst2->isHostFunction());
  auto &HostFuncInst1 = FuncInst1->getHostFunc();
  auto &HostFuncInst2 = FuncInst2->getHostFunc();

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

  // Without a filesystem in the caller, the other contexts only label the
  // messages.
  std::filesystem::remove("out.log");
  std::filesystem::remove("out2.log");
  std::ostringstream Labeled;
  auto StdoutLogger = WasiLoggingMod1->getEnv().StdoutLogger;
  ASSERT_TRUE(StdoutLogger);
  StdoutLogger->sinks().push_back(
      std::make_shared<spdlog::sinks::ostream_sink_mt>(Labeled));
  // Context out.log: message 1
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
  // Context out2.log: message 2
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
  // Context out.log: message 3
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
  StdoutLogger->sinks().pop_back();
  EXPECT_NE(Labeled.str().find("[out.log] Message 1 labeled"sv),
            std::string::npos);
  EXPECT_NE(Labeled.str().find("[out2.log] Message 2 labeled"sv),
            std::string::npos);
  EXPECT_NE(Labeled.str().find("[out.log] Message 3 labeled"sv),
            std::string::npos);
  EXPECT_FALSE(std::filesystem::exists("out.log"));
  EXPECT_FALSE(std::filesystem::exists("out2.log"));

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

TEST(WasiLoggingTests, func_log_file) {
  using namespace std::literals::string_view_literals;
  auto WasiLoggingMod = createModule();
  ASSERT_TRUE(WasiLoggingMod);
  auto *FuncInst = WasiLoggingMod->findFuncExports("log");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = FuncInst->getHostFunc();

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  fillMemContent(MemInst, 128, "Message to file"sv);

  // Log the message at the info level with the context.
  auto Log = [&](std::string_view Context) {
    fillMemContent(MemInst, 0, 128);
    fillMemContent(MemInst, 0, Context);
    return HostFuncInst
        .run(CallFrame,
             std::initializer_list<WasmEdge::ValVariant>{
                 UINT32_C(2), UINT32_C(0),
                 static_cast<uint32_t>(Context.size()), UINT32_C(128),
                 UINT32_C(15)},
             {})
        .has_value();
  };
  auto CountLines = [](const std::filesystem::path &Path) {
    std::ifstream File(Path);
    const std::string Content((std::istreambuf_iterator<char>(File)),
                              std::istreambuf_iterator<char>());
    size_t Count = 0;
    for (auto Pos = Content.find("[info] Message to file\n"sv);
         Pos != std::string::npos;
         Pos = Content.find("[info] Message to file\n"sv, Pos + 1)) {
      ++Count;
    }
    return Count;
  };

  // The caller has the guest directories "." and "/logs" preopened.
  const std::filesystem::path Root("wasi_logging_root");
  const std::filesystem::path Logs("wasi_logging_logs");
  std::filesystem::remove_all(Root);
  std::filesystem::remove_all(Logs);
  std::filesystem::remove("wasi_logging_escape.log");
  std::filesystem::create_directory(Root);
  std::filesystem::create_directory(Logs);
  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{".:wasi_logging_root",
                                        "/logs:wasi_logging_logs"},
               "test", {}, {});
  Mod.wireWASIModule(&WasiMod);

  // The file is appended under the preopened directory of the context, with
  // the directories of the context created.
  EXPECT_TRUE(Log("/dir/out.log"sv));
  EXPECT_TRUE(Log("dir/out.log"sv));
  EXPECT_EQ(CountLines(Root / "dir" / "out.log"), 2U);
  EXPECT_TRUE(Log("/logs/out.log"sv));
  EXPECT_EQ(CountLines(Logs / "out.log"), 1U);
  EXPECT_FALSE(std::filesystem::exists(Root / "logs"));

  // A context leaving the preopened directory only labels the message.
  std::ostringstream Labeled;
  auto StdoutLogger = WasiLoggingMod->getEnv().StdoutLogger;
  ASSERT_TRUE(StdoutLogger);
  StdoutLogger->sinks().push_back(
      std::make_shared<spdlog::sinks::ostream_sink_mt>(Labeled));
  EXPECT_TRUE(Log("../wasi_logging_escape.log"sv));
  StdoutLogger->sinks().pop_back();
  EXPECT_NE(
      Labeled.str().find("[../wasi_logging_escape.log] Message to file"sv),
      std::string::npos);
  EXPECT_FALSE(std::filesystem::exists("wasi_logging_escape.log"));

  // A read-only preopened directory gets no file.
  WasmEdge::Host::WasiModule ReadOnlyWasiMod;
  ReadOnlyWasiMod.init(std::vector<std::string>{".:wasi_logging_root:readonly"},
                       "test", {}, {});
  Mod.wireWASIModule(&ReadOnlyWasiMod);
  EXPECT_TRUE(Log("readonly.log"sv));
  EXPECT_FALSE(std::filesystem::exists(Root / "readonly.log"));

  // The directories are created under the preopened directory of the context
  // only, whatever the directories above it are.
  WasmEdge::Host::WasiModule NestedWasiMod;
  NestedWasiMod.init(std::vector<std::string>{".:wasi_logging_root:readonly",
                                              "/var/log:wasi_logging_logs"},
                     "test", {}, {});
  Mod.wireWASIModule(&NestedWasiMod);
  EXPECT_TRUE(Log("/var/log/app/deep/out.log"sv));
  EXPECT_EQ(CountLines(Logs / "app" / "deep" / "out.log"), 1U);
  EXPECT_FALSE(std::filesystem::exists(Root / "var"));
  WasmEdge::Host::WasiModule LogsWasiMod;
  LogsWasiMod.init(std::vector<std::string>{"/logs:wasi_logging_logs"}, "test",
                   {}, {});
  Mod.wireWASIModule(&LogsWasiMod);
  EXPECT_TRUE(Log("/logs/only/out.log"sv));
  EXPECT_EQ(CountLines(Logs / "only" / "out.log"), 1U);

  std::filesystem::remove_all(Root);
  std::filesystem::remove_all(Logs);
}

TEST(WasiLoggingTests, func_log_oob) {
  using namespace std::literals::string_view_literals;
  auto WasiLoggingMod = createModule();
  ASSERT_TRUE(WasiLoggingMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Place a valid "stdout" context at offset 0.
  fillMemContent(MemInst, 0, "stdout"sv);
  // Place one byte of message data at the last byte of the page.
  fillMemContent(MemInst, 65535, 1, 'x');

  auto *FuncInst = WasiLoggingMod->findFuncExports("log");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = FuncInst->getHostFunc();

  // OOB context: CxtPtr=65535, CxtLen=100 extends 99 bytes past the page end.
  // Expected: HostFuncError (not a crash).
  EXPECT_FALSE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(65535), UINT32_C(100), UINT32_C(0),
          UINT32_C(6)},
      {}));

  // OOB message: valid context ("stdout"), MsgPtr=65535, MsgLen=100.
  // Expected: HostFuncError (not a crash).
  EXPECT_FALSE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(0), UINT32_C(6), UINT32_C(65535),
          UINT32_C(100)},
      {}));
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
