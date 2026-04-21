// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"

#include <array>
#include <cstdlib>
#include <fstream>
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

std::string writeWasmToFile(const uint8_t *Data, size_t Size,
                            const std::string &FileName) {
  std::string Path = TestDataPath + "/" + FileName;
  std::ofstream Ofs(Path, std::ios::binary);
  Ofs.write(reinterpret_cast<const char *>(Data), Size);
  Ofs.close();
  return Path;
}

static const std::array<uint8_t, 127> SimpleWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00, 0x00, 0x05, 0x03,
    0x01, 0x00, 0x01, 0x06, 0x06, 0x01, 0x7f, 0x01, 0x41, 0x00, 0x0b, 0x07,
    0x20, 0x04, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x03, 0x73, 0x75, 0x62,
    0x00, 0x01, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x07,
    0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x03, 0x00, 0x0a, 0x11, 0x02,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b, 0x07, 0x00, 0x20, 0x00,
    0x20, 0x01, 0x6b, 0x0b, 0x00, 0x25, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01,
    0x0b, 0x02, 0x00, 0x03, 0x61, 0x64, 0x64, 0x01, 0x03, 0x73, 0x75, 0x62,
    0x02, 0x05, 0x02, 0x00, 0x00, 0x01, 0x00, 0x07, 0x0a, 0x01, 0x00, 0x07,
    0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72};

static const std::array<uint8_t, 29> InvalidWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05,
    0x01, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x0a,
    0x08, 0x01, 0x06, 0x00, 0x41, 0x01, 0x41, 0x02, 0x0b};

std::string simplePath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(SimpleWasm.data(), SimpleWasm.size(), "simple.wasm");
  }
  return Path;
}

std::string invalidPath() {
  static std::string Path;
  if (Path.empty()) {
    Path =
        writeWasmToFile(InvalidWasm.data(), InvalidWasm.size(), "invalid.wasm");
  }
  return Path;
}

std::string nonExistPath() { return TestDataPath + "/nonexist.wasm"; }

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