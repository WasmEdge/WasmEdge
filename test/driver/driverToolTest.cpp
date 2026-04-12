// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"

#include <cstdlib>
#include <gtest/gtest.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

std::string TestDataPath;

int callUniToolInProcess(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  pid_t Pid = fork();
  if (Pid == 0) {
    int Ret =
        WasmEdge_Driver_UniTool(static_cast<int>(Argv.size()), Argv.data());
    _exit(Ret);
  }
  int Status = 0;
  waitpid(Pid, &Status, 0);
  if (WIFEXITED(Status)) {
    return WEXITSTATUS(Status);
  }
  return -1;
}

std::string simplePath() { return TestDataPath + "/simple.wasm"; }
std::string invalidPath() { return TestDataPath + "/invalid.wasm"; }
std::string nonExistPath() { return TestDataPath + "/nonexist.wasm"; }

// === Parse subcommand tests ===

TEST(DriverParse, ValidModule) {
  EXPECT_EQ(callUniToolInProcess({"parse", simplePath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, InvalidFile) {
  EXPECT_NE(callUniToolInProcess({"parse", nonExistPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, WithDisableSIMD) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-simd", simplePath().c_str()}),
      EXIT_SUCCESS);
}

// === Validate subcommand tests ===

TEST(DriverValidate, ValidModule) {
  EXPECT_EQ(callUniToolInProcess({"validate", simplePath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverValidate, InvalidModule) {
  EXPECT_EQ(callUniToolInProcess({"validate", invalidPath().c_str()}),
            EXIT_FAILURE);
}

TEST(DriverValidate, NonExistentFile) {
  EXPECT_NE(callUniToolInProcess({"validate", nonExistPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverValidate, WithWasm2) {
  EXPECT_EQ(
      callUniToolInProcess({"validate", "--wasm-2", simplePath().c_str()}),
      EXIT_SUCCESS);
}

// === Instantiate subcommand tests ===

TEST(DriverInstantiate, ValidModule) {
  EXPECT_EQ(callUniToolInProcess({"instantiate", simplePath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverInstantiate, InvalidModule) {
  EXPECT_EQ(callUniToolInProcess({"instantiate", invalidPath().c_str()}),
            EXIT_FAILURE);
}

TEST(DriverInstantiate, NonExistentFile) {
  EXPECT_NE(callUniToolInProcess({"instantiate", nonExistPath().c_str()}),
            EXIT_SUCCESS);
}

// === Compile subcommand tests ===

TEST(DriverCompile, ValidModule) {
  std::string Output = TestDataPath + "/simple_aot.wasm";
  EXPECT_EQ(
      callUniToolInProcess({"compile", simplePath().c_str(), Output.c_str()}),
      EXIT_SUCCESS);
  std::remove(Output.c_str());
}

TEST(DriverCompile, NonExistentFile) {
  std::string Output = TestDataPath + "/out.wasm";
  EXPECT_NE(
      callUniToolInProcess({"compile", nonExistPath().c_str(), Output.c_str()}),
      EXIT_SUCCESS);
}

// === Run subcommand tests ===

TEST(DriverRun, ReactorAdd) {
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", simplePath().c_str(), "add", "3", "5"}),
            EXIT_SUCCESS);
}

TEST(DriverRun, ReactorSub) {
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", simplePath().c_str(), "sub", "10", "4"}),
            EXIT_SUCCESS);
}

TEST(DriverRun, NonExistentFile) {
  EXPECT_NE(callUniToolInProcess({"run", nonExistPath().c_str()}),
            EXIT_SUCCESS);
}

} // namespace

GTEST_API_ int main(int Argc, char *Argv[]) {
  testing::InitGoogleTest(&Argc, Argv);
  TestDataPath = "driverTestData";
  if (Argc > 1) {
    TestDataPath = Argv[1];
  }
  return RUN_ALL_TESTS();
}
