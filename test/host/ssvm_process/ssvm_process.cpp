// SPDX-License-Identifier: Apache-2.0
#include "host/ssvm_process/processfunc.h"
#include "host/ssvm_process/processmodule.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
using namespace std::literals;

namespace {
void fillMemContent(SSVM::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}
} // namespace

TEST(SSVMProcessTest, SetProgName) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));
  SSVM::Host::SSVMProcessSetProgName SSVMProcessSetProgName(Env);
  fillMemContent(MemInst, 0, 64);
  char *Buf = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("echo").c_str(), 4, Buf);

  EXPECT_TRUE(SSVMProcessSetProgName.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)}, {}));
  EXPECT_EQ(Env.Name, "echo");
  EXPECT_FALSE(SSVMProcessSetProgName.run(
      nullptr, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)}, {}));
}

TEST(SSVMProcessTest, AddArg) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));
  SSVM::Host::SSVMProcessAddArg SSVMProcessAddArg(Env);
  fillMemContent(MemInst, 0, 64);
  char *Arg1 = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("arg1").c_str(), 4, Arg1);
  char *Arg2 = MemInst.getPointer<char *>(4);
  std::copy_n(std::string("arg2").c_str(), 4, Arg2);
  char *Arg3 = MemInst.getPointer<char *>(30);
  std::copy_n(std::string("--final-arg").c_str(), 11, Arg3);

  EXPECT_TRUE(SSVMProcessAddArg.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)}, {}));
  EXPECT_EQ(Env.Args.size(), 1U);
  EXPECT_EQ(Env.Args[0], "arg1");
  EXPECT_TRUE(SSVMProcessAddArg.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(4), UINT32_C(4)}, {}));
  EXPECT_EQ(Env.Args.size(), 2U);
  EXPECT_EQ(Env.Args[1], "arg2");
  EXPECT_TRUE(SSVMProcessAddArg.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(Env.Args.size(), 3U);
  EXPECT_EQ(Env.Args[2], "--final-arg");
  EXPECT_FALSE(SSVMProcessAddArg.run(
      nullptr, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)}, {}));
}

TEST(SSVMProcessTest, AddEnv) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));
  SSVM::Host::SSVMProcessAddEnv SSVMProcessAddEnv(Env);
  fillMemContent(MemInst, 0, 256);
  char *Env1 = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("ENV1").c_str(), 4, Env1);
  char *Val1 = MemInst.getPointer<char *>(4);
  std::copy_n(std::string("VALUE1").c_str(), 6, Val1);
  char *Env2 = MemInst.getPointer<char *>(30);
  std::copy_n(std::string("LD_LIBRARY_PATH").c_str(), 15, Env2);
  char *Val2 = MemInst.getPointer<char *>(50);
  std::copy_n(std::string("/usr/local/lib").c_str(), 14, Val2);

  EXPECT_TRUE(SSVMProcessAddEnv.run(
      &MemInst,
      std::array<SSVM::ValVariant, 4>{UINT32_C(0), UINT32_C(4), UINT32_C(4),
                                      UINT32_C(6)},
      {}));
  EXPECT_EQ(Env.Envs.size(), 1U);
  EXPECT_EQ(Env.Envs["ENV1"], "VALUE1");
  EXPECT_TRUE(SSVMProcessAddEnv.run(
      &MemInst,
      std::array<SSVM::ValVariant, 4>{UINT32_C(30), UINT32_C(15), UINT32_C(50),
                                      UINT32_C(14)},
      {}));
  EXPECT_EQ(Env.Envs.size(), 2U);
  EXPECT_EQ(Env.Envs["LD_LIBRARY_PATH"], "/usr/local/lib");
  EXPECT_FALSE(SSVMProcessAddEnv.run(
      nullptr,
      std::array<SSVM::ValVariant, 4>{UINT32_C(0), UINT32_C(4), UINT32_C(4),
                                      UINT32_C(6)},
      {}));
}

TEST(SSVMProcessTest, AddStdIn) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));
  SSVM::Host::SSVMProcessAddStdIn SSVMProcessAddStdIn(Env);
  fillMemContent(MemInst, 0, 64);
  uint8_t *Buf1 = MemInst.getPointer<uint8_t *>(0);
  std::copy_n(std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}).begin(), 4, Buf1);
  uint8_t *Buf2 = MemInst.getPointer<uint8_t *>(30);
  std::copy_n(std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o', ' ', 's', 's', 'v',
                                   'm', '\n'}
                  .begin(),
              11, Buf2);

  EXPECT_TRUE(SSVMProcessAddStdIn.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)}, {}));
  EXPECT_EQ(Env.StdIn.size(), 4U);
  EXPECT_EQ(Env.StdIn, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}));
  EXPECT_TRUE(SSVMProcessAddStdIn.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(Env.StdIn.size(), 15U);
  EXPECT_EQ(Env.StdIn,
            std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 'h', 'e', 'l', 'l',
                                  'o', ' ', 's', 's', 'v', 'm', '\n'}));
  EXPECT_FALSE(SSVMProcessAddStdIn.run(
      nullptr, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)}, {}));
}

TEST(SSVMProcessTest, SetTimeOut) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Host::SSVMProcessSetTimeOut SSVMProcessSetTimeOut(Env);

  EXPECT_TRUE(SSVMProcessSetTimeOut.run(
      nullptr, std::array<SSVM::ValVariant, 1>{UINT32_C(100)}, {}));
  EXPECT_EQ(Env.TimeOut, 100U);
}

TEST(SSVMProcessTest, Run) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Host::SSVMProcessRun SSVMProcessRun(Env);
  std::array<SSVM::ValVariant, 1> RetVal;

  Env.Name = "c++";
  EXPECT_TRUE(SSVMProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(RetVal[0]), -1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);
  std::string ErrStr =
      "Permission denied: Command \"c++\" is not in the white list. Please use "
      "--allow-command=c++ or --allow-command-all to add \"c++\" command into "
      "the white list.\n";
  EXPECT_TRUE(std::equal(Env.StdErr.begin(), Env.StdErr.end(), ErrStr.begin()));

  Env.AllowedAll = true;
  Env.Name = "c++";
  EXPECT_TRUE(SSVMProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(RetVal[0]), 1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);

  Env.AllowedAll = false;
  Env.AllowedCmd.insert("c++");
  Env.Name = "c++";
  EXPECT_TRUE(SSVMProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(RetVal[0]), 1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);
}

TEST(SSVMProcessTest, GetExitCode) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Host::SSVMProcessGetExitCode SSVMProcessGetExitCode(Env);
  std::array<SSVM::ValVariant, 1> RetVal;

  EXPECT_TRUE(SSVMProcessGetExitCode.run(nullptr, {}, RetVal));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(RetVal[0]), 0);
}

TEST(SSVMProcessTest, GetStdOut) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));
  SSVM::Host::SSVMProcessRun SSVMProcessRun(Env);
  SSVM::Host::SSVMProcessGetStdOutLen SSVMProcessGetStdOutLen(Env);
  SSVM::Host::SSVMProcessGetStdOut SSVMProcessGetStdOut(Env);
  fillMemContent(MemInst, 0, 256);
  std::array<SSVM::ValVariant, 1> RetVal;

  Env.Name = "echo";
  Env.AllowedCmd.insert("echo");
  Env.Args.push_back("$(pwd)");
  EXPECT_TRUE(SSVMProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(SSVM::retrieveValue<uint32_t>(RetVal[0]), 0U);
  EXPECT_TRUE(SSVMProcessGetStdOutLen.run(nullptr, {}, RetVal));
  uint32_t Len = SSVM::retrieveValue<uint32_t>(RetVal[0]);
  EXPECT_TRUE(Len > 0U);
  EXPECT_FALSE(SSVMProcessGetStdOut.run(
      nullptr, std::array<SSVM::ValVariant, 1>{UINT32_C(0)}, {}));
  EXPECT_TRUE(SSVMProcessGetStdOut.run(
      &MemInst, std::array<SSVM::ValVariant, 1>{UINT32_C(0)}, {}));
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(0);
  EXPECT_TRUE(std::equal(Env.StdOut.begin(), Env.StdOut.end(), Buf));
}

TEST(SSVMProcessTest, GetStdErr) {
  SSVM::Host::SSVMProcessEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));
  SSVM::Host::SSVMProcessRun SSVMProcessRun(Env);
  SSVM::Host::SSVMProcessGetStdErrLen SSVMProcessGetStdErrLen(Env);
  SSVM::Host::SSVMProcessGetStdErr SSVMProcessGetStdErr(Env);
  fillMemContent(MemInst, 0, 256);
  std::array<SSVM::ValVariant, 1> RetVal;

  Env.Name = "c++";
  Env.AllowedCmd.insert("c++");
  EXPECT_TRUE(SSVMProcessRun.run(nullptr, {}, RetVal));
  EXPECT_NE(SSVM::retrieveValue<uint32_t>(RetVal[0]), 0U);
  EXPECT_TRUE(SSVMProcessGetStdErrLen.run(nullptr, {}, RetVal));
  uint32_t Len = SSVM::retrieveValue<uint32_t>(RetVal[0]);
  EXPECT_TRUE(Len > 0);
  EXPECT_FALSE(SSVMProcessGetStdErr.run(
      nullptr, std::array<SSVM::ValVariant, 1>{UINT32_C(0)}, {}));
  EXPECT_TRUE(SSVMProcessGetStdErr.run(
      &MemInst, std::array<SSVM::ValVariant, 1>{UINT32_C(0)}, {}));
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(0);
  EXPECT_TRUE(std::equal(Env.StdErr.begin(), Env.StdErr.end(), Buf));
}

TEST(SSVMProcessTest, Module) {
  SSVM::Host::SSVMProcessModule Mod = SSVM::Host::SSVMProcessModule();
  const auto &FuncMap = Mod.getFuncs();
  EXPECT_EQ(Mod.getEnv().ExitCode, 0U);
  EXPECT_EQ(FuncMap.size(), 11U);
  EXPECT_NE(FuncMap.find("ssvm_process_set_prog_name"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_add_arg"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_add_env"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_add_stdin"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_set_timeout"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_run"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_get_exit_code"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_get_stdout_len"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_get_stdout"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_get_stderr_len"), FuncMap.end());
  EXPECT_NE(FuncMap.find("ssvm_process_get_stderr"), FuncMap.end());
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
