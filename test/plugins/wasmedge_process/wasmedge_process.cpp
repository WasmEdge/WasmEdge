// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "processfunc.h"
#include "processmodule.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

WasmEdge::Runtime::CallingFrame DummyCallFrame(nullptr, nullptr);

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

std::unique_ptr<WasmEdge::Host::WasmEdgeProcessModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_process/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeProcess" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_process"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_process"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeProcessModule>(
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

using namespace std::literals::string_view_literals;

TEST(WasmEdgeProcessTest, SetProgName) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 64].
  fillMemContent(MemInst, 0, 64);
  // Set the memory[0, 4] as string "echo".
  fillMemContent(MemInst, 0, "echo"sv);

  // Get the function "wasmedge_process_set_prog_name".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_set_prog_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessSetProgName &>(
          FuncInst->getHostFunc());

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Name, "echo");

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncInst.run(
      DummyCallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, AddArg) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 64].
  fillMemContent(MemInst, 0, 64);
  // Set the memory[0, 4] as string "echo".
  fillMemContent(MemInst, 0, "arg1"sv);
  // Set the memory[4, 8] as string "arg2".
  fillMemContent(MemInst, 4, "arg2"sv);
  // Set the memory[30, 41] as string "--final-arg".
  fillMemContent(MemInst, 30, "--final-arg"sv);

  // Get the function "wasmedge_process_add_arg".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_add_arg");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeProcessAddArg &>(
      FuncInst->getHostFunc());

  // Test: Run function successfully to add "arg1".
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Args.size(), 1U);
  EXPECT_EQ(ProcMod->getEnv().Args[0], "arg1");

  // Test: Run function successfully to add "arg2".
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(4), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Args.size(), 2U);
  EXPECT_EQ(ProcMod->getEnv().Args[1], "arg2");

  // Test: Run function successfully to add "--final-arg".
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Args.size(), 3U);
  EXPECT_EQ(ProcMod->getEnv().Args[2], "--final-arg");

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncInst.run(
      DummyCallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, AddEnv) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 256].
  fillMemContent(MemInst, 0, 256);
  // Set the memory[0, 4] as string "ENV1".
  fillMemContent(MemInst, 0, "ENV1"sv);
  // Set the memory[4, 10] as string "VALUE1".
  fillMemContent(MemInst, 4, "VALUE1"sv);
  // Set the memory[30, 45] as string "LD_LIBRARY_PATH".
  fillMemContent(MemInst, 30, "LD_LIBRARY_PATH"sv);
  // Set the memory[50, 64] as string "/usr/local/lib".
  fillMemContent(MemInst, 50, "/usr/local/lib"sv);

  // Get the function "wasmedge_process_add_env".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_add_env");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeProcessAddEnv &>(
      FuncInst->getHostFunc());

  // Test: Run function successfully to add "ENV1", "VALUE1".
  EXPECT_TRUE(
      HostFuncInst.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           UINT32_C(0), UINT32_C(4), UINT32_C(4), UINT32_C(6)},
                       {}));
  EXPECT_EQ(ProcMod->getEnv().Envs.size(), 1U);
  EXPECT_EQ(ProcMod->getEnv().Envs["ENV1"], "VALUE1");

  // Test: Run function successfully to add "LD_LIBRARY_PATH", "/usr/local/lib".
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(30), UINT32_C(15),
                                                  UINT32_C(50), UINT32_C(14)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Envs.size(), 2U);
  EXPECT_EQ(ProcMod->getEnv().Envs["LD_LIBRARY_PATH"], "/usr/local/lib");

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(
      HostFuncInst.run(DummyCallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           UINT32_C(0), UINT32_C(4), UINT32_C(4), UINT32_C(6)},
                       {}));
}

TEST(WasmEdgeProcessTest, AddStdIn) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 64].
  fillMemContent(MemInst, 0, 64);
  // Set the memory[0, 4] as string "\01\02\03\04".
  fillMemContent(MemInst, 0, "\01\02\03\04"sv);
  // Set the memory[30, 46] as string "hello, wasmedge\n".
  fillMemContent(MemInst, 30, "hello, wasmedge\n"sv);

  // Get the function "wasmedge_process_add_stdin".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_add_stdin");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeProcessAddStdIn &>(
      FuncInst->getHostFunc());

  // Test: Run function successfully to add "\01\02\03\04".
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().StdIn.size(), 4U);
  EXPECT_EQ(ProcMod->getEnv().StdIn,
            std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}));

  // Test: Run function successfully to add "hello, wasmedge\n".
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(30), UINT32_C(16)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().StdIn.size(), 20U);
  EXPECT_EQ(ProcMod->getEnv().StdIn,
            std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 'h', 'e', 'l',
                                  'l',  'o',  ',',  ' ',  'w', 'a', 's',
                                  'm',  'e',  'd',  'g',  'e', '\n'}));

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncInst.run(
      DummyCallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, SetTimeOut) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Get the function "wasmedge_process_set_timeout".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_set_timeout");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessSetTimeOut &>(
          FuncInst->getHostFunc());

  // Test: Run function successfully to set timeout 100.
  EXPECT_TRUE(HostFuncInst.run(
      DummyCallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(100)}, {}));
  EXPECT_EQ(ProcMod->getEnv().TimeOut, 100U);
}

TEST(WasmEdgeProcessTest, Run) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 64].
  fillMemContent(MemInst, 0, 64);
  // Set the memory[0, 4] as string "\01\02\03\04".
  fillMemContent(MemInst, 0, "\01\02\03\04"sv);
  // Set the memory[30, 46] as string "hello, wasmedge\n".
  fillMemContent(MemInst, 30, "hello, wasmedge\n"sv);

  // Get the function "wasmedge_process_run".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_run");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeProcessRun &>(
      FuncInst->getHostFunc());

  // Return value.
  std::array<WasmEdge::ValVariant, 1> RetVal;

  // Test: Run function failed to run "c++" without allowing all commands.
  ProcMod->getEnv().AllowedAll = false;
  ProcMod->getEnv().Name = "c++";
  EXPECT_TRUE(HostFuncInst.run(DummyCallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);
  EXPECT_TRUE(ProcMod->getEnv().StdOut.size() == 0);
  EXPECT_TRUE(ProcMod->getEnv().StdErr.size() > 0);
  std::string ErrStr =
      "Permission denied: Command \"c++\" is not in the white list. Please use "
      "--allow-command=c++ or --allow-command-all to add \"c++\" command into "
      "the white list.\n";
  EXPECT_TRUE(std::equal(ProcMod->getEnv().StdErr.begin(),
                         ProcMod->getEnv().StdErr.end(), ErrStr.begin()));

  // Test: Run function successfully to run "c++" with allowing all commands.
  ProcMod->getEnv().AllowedAll = true;
  ProcMod->getEnv().Name = "c++";
  EXPECT_TRUE(HostFuncInst.run(DummyCallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);
  EXPECT_TRUE(ProcMod->getEnv().StdOut.size() == 0);
  EXPECT_TRUE(ProcMod->getEnv().StdErr.size() > 0);

  // Test: Run function successfully to run "c++" with allowing this command.
  ProcMod->getEnv().AllowedAll = false;
  ProcMod->getEnv().AllowedCmd.insert("c++");
  ProcMod->getEnv().Name = "c++";
  EXPECT_TRUE(HostFuncInst.run(DummyCallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);
  EXPECT_TRUE(ProcMod->getEnv().StdOut.size() == 0);
  EXPECT_TRUE(ProcMod->getEnv().StdErr.size() > 0);

  // Test: Run function successfully to run "/bin/echo" with allowing this
  // command.
  ProcMod->getEnv().AllowedAll = false;
  ProcMod->getEnv().AllowedCmd.clear();
  ProcMod->getEnv().AllowedCmd.insert("/bin/echo");
  ProcMod->getEnv().Name = "/bin/echo";
  ProcMod->getEnv().Args.push_back("123456 test");
  EXPECT_TRUE(HostFuncInst.run(DummyCallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
  EXPECT_TRUE(ProcMod->getEnv().StdOut.size() == 12);
  EXPECT_TRUE(ProcMod->getEnv().StdErr.size() == 0);
  std::string OutStr = "123456 test\n";
  EXPECT_TRUE(std::equal(ProcMod->getEnv().StdOut.begin(),
                         ProcMod->getEnv().StdOut.end(), OutStr.begin()));
}

TEST(WasmEdgeProcessTest, GetExitCode) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Get the function "wasmedge_process_get_exit_code".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_get_exit_code");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessGetExitCode &>(
          FuncInst->getHostFunc());

  // Test: Run function successfully to get exit code.
  std::array<WasmEdge::ValVariant, 1> RetVal;
  EXPECT_TRUE(HostFuncInst.run(DummyCallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
}

TEST(WasmEdgeProcessTest, GetStdOut) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 256].
  fillMemContent(MemInst, 0, 256);

  // Get the function "wasmedge_process_run".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_run");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncRun = dynamic_cast<WasmEdge::Host::WasmEdgeProcessRun &>(
      FuncInst->getHostFunc());
  // Get the function "wasmedge_process_run".
  FuncInst = ProcMod->findFuncExports("wasmedge_process_get_stdout_len");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetStdOutLen =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessGetStdOutLen &>(
          FuncInst->getHostFunc());
  // Get the function "wasmedge_process_run".
  FuncInst = ProcMod->findFuncExports("wasmedge_process_get_stdout");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetStdOut =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessGetStdOut &>(
          FuncInst->getHostFunc());

  // Return value.
  std::array<WasmEdge::ValVariant, 1> RetVal;

  // Run the command "echo $(pwd)".
  ProcMod->getEnv().Name = "echo";
  ProcMod->getEnv().AllowedCmd.insert("echo");
  ProcMod->getEnv().Args.push_back("$(pwd)");
  EXPECT_TRUE(HostFuncRun.run(DummyCallFrame, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  // Test: Run wasmedge_process_get_stdout_len successfully.
  EXPECT_TRUE(HostFuncGetStdOutLen.run(DummyCallFrame, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0U);

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncGetStdOut.run(
      DummyCallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
      {}));

  // Test: Run wasmedge_process_get_stdout successfully.
  EXPECT_TRUE(HostFuncGetStdOut.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  EXPECT_TRUE(std::equal(ProcMod->getEnv().StdOut.begin(),
                         ProcMod->getEnv().StdOut.end(),
                         MemInst.getPointer<uint8_t *>(0)));
}

TEST(WasmEdgeProcessTest, GetStdErr) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 256].
  fillMemContent(MemInst, 0, 256);

  // Get the function "wasmedge_process_run".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_run");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncRun = dynamic_cast<WasmEdge::Host::WasmEdgeProcessRun &>(
      FuncInst->getHostFunc());
  // Get the function "wasmedge_process_run".
  FuncInst = ProcMod->findFuncExports("wasmedge_process_get_stderr_len");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetStdErrLen =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessGetStdErrLen &>(
          FuncInst->getHostFunc());
  // Get the function "wasmedge_process_run".
  FuncInst = ProcMod->findFuncExports("wasmedge_process_get_stderr");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetStdErr =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessGetStdErr &>(
          FuncInst->getHostFunc());

  // Return value.
  std::array<WasmEdge::ValVariant, 1> RetVal;

  // Run the command "c++".
  ProcMod->getEnv().Name = "c++";
  ProcMod->getEnv().AllowedCmd.insert("c++");
  EXPECT_TRUE(HostFuncRun.run(DummyCallFrame, {}, RetVal));
  EXPECT_NE(RetVal[0].get<uint32_t>(), 0U);

  // Test: Run wasmedge_process_get_stdout_len successfully.
  EXPECT_TRUE(HostFuncGetStdErrLen.run(DummyCallFrame, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0U);

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncGetStdErr.run(
      DummyCallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
      {}));

  // Test: Run wasmedge_process_get_stdout successfully.
  EXPECT_TRUE(HostFuncGetStdErr.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  EXPECT_TRUE(std::equal(ProcMod->getEnv().StdOut.begin(),
                         ProcMod->getEnv().StdOut.end(),
                         MemInst.getPointer<uint8_t *>(0)));
}

TEST(WasmEdgeProcessTest, Module) {
  // Create the wasmedge_process module instance.
  auto ProcMod = createModule();
  ASSERT_TRUE(ProcMod);

  EXPECT_EQ(ProcMod->getEnv().ExitCode, 0U);
  EXPECT_EQ(ProcMod->getFuncExportNum(), 11U);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_set_prog_name"),
            nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_add_arg"), nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_add_env"), nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_add_stdin"), nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_set_timeout"), nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_run"), nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_get_exit_code"),
            nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_get_stdout_len"),
            nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_get_stdout"), nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_get_stderr_len"),
            nullptr);
  EXPECT_NE(ProcMod->findFuncExports("wasmedge_process_get_stderr"), nullptr);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
