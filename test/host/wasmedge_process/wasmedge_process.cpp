// SPDX-License-Identifier: Apache-2.0

#include "host/wasmedge_process/processfunc.h"
#include "host/wasmedge_process/processmodule.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

using namespace std::literals;

namespace {
void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}
} // namespace

TEST(WasmEdgeProcessTest, SetProgName) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(WasmEdge::AST::Limit(1));
  WasmEdge::Host::WasmEdgeProcessSetProgName WasmEdgeProcessSetProgName(Env);
  fillMemContent(MemInst, 0, 64);
  char *Buf = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("echo").c_str(), 4, Buf);

  EXPECT_TRUE(WasmEdgeProcessSetProgName.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(Env.Name, "echo");
  EXPECT_FALSE(WasmEdgeProcessSetProgName.run(
      nullptr, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, AddArg) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(WasmEdge::AST::Limit(1));
  WasmEdge::Host::WasmEdgeProcessAddArg WasmEdgeProcessAddArg(Env);
  fillMemContent(MemInst, 0, 64);
  char *Arg1 = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("arg1").c_str(), 4, Arg1);
  char *Arg2 = MemInst.getPointer<char *>(4);
  std::copy_n(std::string("arg2").c_str(), 4, Arg2);
  char *Arg3 = MemInst.getPointer<char *>(30);
  std::copy_n(std::string("--final-arg").c_str(), 11, Arg3);

  EXPECT_TRUE(WasmEdgeProcessAddArg.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(Env.Args.size(), 1U);
  EXPECT_EQ(Env.Args[0], "arg1");
  EXPECT_TRUE(WasmEdgeProcessAddArg.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(4), UINT32_C(4)},
      {}));
  EXPECT_EQ(Env.Args.size(), 2U);
  EXPECT_EQ(Env.Args[1], "arg2");
  EXPECT_TRUE(WasmEdgeProcessAddArg.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(Env.Args.size(), 3U);
  EXPECT_EQ(Env.Args[2], "--final-arg");
  EXPECT_FALSE(WasmEdgeProcessAddArg.run(
      nullptr, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, AddEnv) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(WasmEdge::AST::Limit(1));
  WasmEdge::Host::WasmEdgeProcessAddEnv WasmEdgeProcessAddEnv(Env);
  fillMemContent(MemInst, 0, 256);
  char *Env1 = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("ENV1").c_str(), 4, Env1);
  char *Val1 = MemInst.getPointer<char *>(4);
  std::copy_n(std::string("VALUE1").c_str(), 6, Val1);
  char *Env2 = MemInst.getPointer<char *>(30);
  std::copy_n(std::string("LD_LIBRARY_PATH").c_str(), 15, Env2);
  char *Val2 = MemInst.getPointer<char *>(50);
  std::copy_n(std::string("/usr/local/lib").c_str(), 14, Val2);

  EXPECT_TRUE(WasmEdgeProcessAddEnv.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 4>{UINT32_C(0), UINT32_C(4), UINT32_C(4),
                                          UINT32_C(6)},
      {}));
  EXPECT_EQ(Env.Envs.size(), 1U);
  EXPECT_EQ(Env.Envs["ENV1"], "VALUE1");
  EXPECT_TRUE(WasmEdgeProcessAddEnv.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 4>{UINT32_C(30), UINT32_C(15),
                                          UINT32_C(50), UINT32_C(14)},
      {}));
  EXPECT_EQ(Env.Envs.size(), 2U);
  EXPECT_EQ(Env.Envs["LD_LIBRARY_PATH"], "/usr/local/lib");
  EXPECT_FALSE(WasmEdgeProcessAddEnv.run(
      nullptr,
      std::array<WasmEdge::ValVariant, 4>{UINT32_C(0), UINT32_C(4), UINT32_C(4),
                                          UINT32_C(6)},
      {}));
}

TEST(WasmEdgeProcessTest, AddStdIn) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(WasmEdge::AST::Limit(1));
  WasmEdge::Host::WasmEdgeProcessAddStdIn WasmEdgeProcessAddStdIn(Env);
  fillMemContent(MemInst, 0, 64);
  uint8_t *Buf1 = MemInst.getPointer<uint8_t *>(0);
  std::copy_n(std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}).begin(), 4, Buf1);
  uint8_t *Buf2 = MemInst.getPointer<uint8_t *>(30);
  std::copy_n(std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o', ' ', 's', 's', 'v',
                                   'm', '\n'}
                  .begin(),
              11, Buf2);

  EXPECT_TRUE(WasmEdgeProcessAddStdIn.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(Env.StdIn.size(), 4U);
  EXPECT_EQ(Env.StdIn, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}));
  EXPECT_TRUE(WasmEdgeProcessAddStdIn.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(Env.StdIn.size(), 15U);
  EXPECT_EQ(Env.StdIn,
            std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 'h', 'e', 'l', 'l',
                                  'o', ' ', 's', 's', 'v', 'm', '\n'}));
  EXPECT_FALSE(WasmEdgeProcessAddStdIn.run(
      nullptr, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, SetTimeOut) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Host::WasmEdgeProcessSetTimeOut WasmEdgeProcessSetTimeOut(Env);

  EXPECT_TRUE(WasmEdgeProcessSetTimeOut.run(
      nullptr, std::array<WasmEdge::ValVariant, 1>{UINT32_C(100)}, {}));
  EXPECT_EQ(Env.TimeOut, 100U);
}

TEST(WasmEdgeProcessTest, Run) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Host::WasmEdgeProcessRun WasmEdgeProcessRun(Env);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  Env.Name = "c++";
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);
  std::string ErrStr =
      "Permission denied: Command \"c++\" is not in the white list. Please use "
      "--allow-command=c++ or --allow-command-all to add \"c++\" command into "
      "the white list.\n";
  EXPECT_TRUE(std::equal(Env.StdErr.begin(), Env.StdErr.end(), ErrStr.begin()));

  Env.AllowedAll = true;
  Env.Name = "c++";
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);

  Env.AllowedAll = false;
  Env.AllowedCmd.insert("c++");
  Env.Name = "c++";
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);

  Env.AllowedAll = false;
  Env.AllowedCmd.clear();
  Env.AllowedCmd.insert("/bin/echo");
  Env.Name = "/bin/echo";
  Env.Args.push_back("123456 test");
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
  EXPECT_TRUE(Env.StdOut.size() == 12);
  EXPECT_TRUE(Env.StdErr.size() == 0);
  std::string OutStr = "123456 test\n";
  EXPECT_TRUE(std::equal(Env.StdOut.begin(), Env.StdOut.end(), OutStr.begin()));
}

TEST(WasmEdgeProcessTest, GetExitCode) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Host::WasmEdgeProcessGetExitCode WasmEdgeProcessGetExitCode(Env);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  EXPECT_TRUE(WasmEdgeProcessGetExitCode.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
}

TEST(WasmEdgeProcessTest, GetStdOut) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(WasmEdge::AST::Limit(1));
  WasmEdge::Host::WasmEdgeProcessRun WasmEdgeProcessRun(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdOutLen WasmEdgeProcessGetStdOutLen(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdOut WasmEdgeProcessGetStdOut(Env);
  fillMemContent(MemInst, 0, 256);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  Env.Name = "echo";
  Env.AllowedCmd.insert("echo");
  Env.Args.push_back("$(pwd)");
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_TRUE(WasmEdgeProcessGetStdOutLen.run(nullptr, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0U);
  EXPECT_FALSE(WasmEdgeProcessGetStdOut.run(
      nullptr, std::array<WasmEdge::ValVariant, 1>{UINT32_C(0)}, {}));
  EXPECT_TRUE(WasmEdgeProcessGetStdOut.run(
      &MemInst, std::array<WasmEdge::ValVariant, 1>{UINT32_C(0)}, {}));
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(0);
  EXPECT_TRUE(std::equal(Env.StdOut.begin(), Env.StdOut.end(), Buf));
}

TEST(WasmEdgeProcessTest, GetStdErr) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(WasmEdge::AST::Limit(1));
  WasmEdge::Host::WasmEdgeProcessRun WasmEdgeProcessRun(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdErrLen WasmEdgeProcessGetStdErrLen(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdErr WasmEdgeProcessGetStdErr(Env);
  fillMemContent(MemInst, 0, 256);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  Env.Name = "c++";
  Env.AllowedCmd.insert("c++");
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_NE(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_TRUE(WasmEdgeProcessGetStdErrLen.run(nullptr, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0);
  EXPECT_FALSE(WasmEdgeProcessGetStdErr.run(
      nullptr, std::array<WasmEdge::ValVariant, 1>{UINT32_C(0)}, {}));
  EXPECT_TRUE(WasmEdgeProcessGetStdErr.run(
      &MemInst, std::array<WasmEdge::ValVariant, 1>{UINT32_C(0)}, {}));
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(0);
  EXPECT_TRUE(std::equal(Env.StdErr.begin(), Env.StdErr.end(), Buf));
}

TEST(WasmEdgeProcessTest, Module) {
  WasmEdge::Host::WasmEdgeProcessModule Mod =
      WasmEdge::Host::WasmEdgeProcessModule();
  const auto &FuncMap = Mod.getFuncs();
  EXPECT_EQ(Mod.getEnv().ExitCode, 0U);
  EXPECT_EQ(FuncMap.size(), 11U);
  EXPECT_NE(FuncMap.find("wasmedge_process_set_prog_name"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_add_arg"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_add_env"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_add_stdin"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_set_timeout"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_run"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_get_exit_code"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_get_stdout_len"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_get_stdout"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_get_stderr_len"), FuncMap.end());
  EXPECT_NE(FuncMap.find("wasmedge_process_get_stderr"), FuncMap.end());
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
