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

// See parseToolTest.cpp for fork-isolation rationale.
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

// See parseToolTest.cpp for module descriptions.
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

static const std::array<uint8_t, 12> TruncatedWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

static const std::array<uint8_t, 8> NotWasmBytes{0x7f, 0x45, 0x4c, 0x46,
                                                 0x00, 0x00, 0x00, 0x00};

// provider.wasm: exports function "add" (i32, i32) -> i32.
static const std::array<uint8_t, 41> ProviderWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
    0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b};

// consumer.wasm: imports "provider"."add" and re-exports it.
static const std::array<uint8_t, 44> ConsumerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x08, 0x70,
    0x72, 0x6f, 0x76, 0x69, 0x64, 0x65, 0x72, 0x03, 0x61, 0x64, 0x64,
    0x00, 0x00, 0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00};

std::string simplePath() {
  static std::string Path;
  if (Path.empty()) {
    Path =
        writeWasmToFile(SimpleWasm.data(), SimpleWasm.size(), "simple_r.wasm");
  }
  return Path;
}

std::string providerPath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(ProviderWasm.data(), ProviderWasm.size(),
                           "provider_r.wasm");
  }
  return Path;
}

std::string consumerPath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(ConsumerWasm.data(), ConsumerWasm.size(),
                           "consumer_r.wasm");
  }
  return Path;
}

std::string nonExistPath() { return TestDataPath + "/nonexist.wasm"; }

// Test error handling for invalid inputs to the run subcommand.
// Covers: missing file, non-existent file, corrupted wasm, non-wasm binary,
// unknown flag.
// Commands tested:
//   wasmedge run
//   wasmedge run nonexist.wasm
//   wasmedge run <truncated-wasm>
//   wasmedge run <elf-binary>
//   wasmedge run --no-such-flag simple_r.wasm
TEST(RunSubcommand, ErrorHandling) {
  // No file argument provided.
  EXPECT_NE(callUniToolInProcess({"run"}), EXIT_SUCCESS);

  // Non-existent file should fail to load.
  EXPECT_NE(callUniToolInProcess({"run", nonExistPath().c_str()}),
            EXIT_SUCCESS);

  // Valid wasm magic but truncated/garbage section data.
  std::string TruncPath = writeWasmToFile(TruncatedWasm.data(),
                                          TruncatedWasm.size(), "trunc_r.wasm");
  EXPECT_NE(callUniToolInProcess({"run", TruncPath.c_str()}), EXIT_SUCCESS);
  std::remove(TruncPath.c_str());

  // Not wasm at all (ELF magic bytes).
  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf_r.wasm");
  EXPECT_NE(callUniToolInProcess({"run", ElfPath.c_str()}), EXIT_SUCCESS);
  std::remove(ElfPath.c_str());

  // Unknown CLI flag should be rejected.
  // Command: wasmedge run --no-such-flag simple_r.wasm
  EXPECT_NE(
      callUniToolInProcess({"run", "--no-such-flag", simplePath().c_str()}),
      EXIT_SUCCESS);
}

// Test the run subcommand in reactor mode, calling exported functions.
// Commands tested:
//   wasmedge run --reactor simple_r.wasm add 3 5
//   wasmedge run --reactor simple_r.wasm sub 10 4
//   wasmedge run --reactor simple_r.wasm no_such_func
//   wasmedge run --reactor simple_r.wasm
TEST(RunSubcommand, ReactorMode) {
  // Call exported "add" function: add(3, 5) should succeed and return 8.
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", simplePath().c_str(), "add", "3", "5"}),
            EXIT_SUCCESS);

  // Call exported "sub" function: sub(10, 4) should succeed and return 6.
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", simplePath().c_str(), "sub", "10", "4"}),
            EXIT_SUCCESS);

  // Calling a function name that doesn't exist in the export list should fail.
  EXPECT_NE(callUniToolInProcess(
                {"run", "--reactor", simplePath().c_str(), "no_such_func"}),
            EXIT_SUCCESS);

  // Reactor mode without a function name argument should fail.
  EXPECT_NE(callUniToolInProcess({"run", "--reactor", simplePath().c_str()}),
            EXIT_SUCCESS);
}

// Test proposal flags with the run subcommand.
// The run subcommand uses addOptions which includes all proposal flags via
// addLinkerOptions -> addParserOptions -> addProposalOptions.
// Commands tested:
//   wasmedge run --reactor --disable-simd simple_r.wasm add 1 2
//   wasmedge run --reactor --enable-all simple_r.wasm add 1 2
//   wasmedge run --reactor --wasm-2 simple_r.wasm add 1 2
TEST(RunSubcommand, ProposalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", "--disable-simd", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", "--enable-all", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", "--wasm-2", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
}

// Test run-specific flags that are not available to other subcommands.
// addOptions adds: --enable-instruction-count, --enable-gas-measuring,
// --enable-time-measuring, --enable-all-statistics, --force-interpreter,
// --enable-jit, --enable-coredump, --coredump-for-wasmgdb, --allow-af-unix,
// --time-limit, --gas-limit, --dir, --env, --memory-page-limit.
// Commands tested:
//   wasmedge run --reactor --enable-instruction-count simple_r.wasm add 1 2
//   wasmedge run --reactor --enable-gas-measuring simple_r.wasm add 1 2
//   wasmedge run --reactor --enable-time-measuring simple_r.wasm add 1 2
//   wasmedge run --reactor --enable-all-statistics simple_r.wasm add 1 2
//   wasmedge run --reactor --force-interpreter simple_r.wasm add 1 2
//   wasmedge run --reactor --enable-jit simple_r.wasm add 1 2
//   wasmedge run --reactor --enable-coredump simple_r.wasm add 1 2
//   wasmedge run --reactor --coredump-for-wasmgdb simple_r.wasm add 1 2
//   wasmedge run --reactor --allow-af-unix simple_r.wasm add 1 2
//   wasmedge run --reactor --time-limit 5000 simple_r.wasm add 1 2
//   wasmedge run --reactor --gas-limit 100000 simple_r.wasm add 1 2
//   wasmedge run --reactor --dir .:. simple_r.wasm add 1 2
//   wasmedge run --reactor --env HOME=/tmp simple_r.wasm add 1 2
//   wasmedge run --reactor --memory-page-limit 256 simple_r.wasm add 1 2
TEST(RunSubcommand, RunSpecificFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(
      callUniToolInProcess({"run", "--reactor", "--enable-instruction-count",
                            Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--enable-gas-measuring",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--enable-time-measuring",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--enable-all-statistics",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--force-interpreter",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", "--enable-jit", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--enable-coredump", Path,
                                  "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--coredump-for-wasmgdb",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", "--allow-af-unix", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--time-limit", "5000",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--gas-limit", "100000",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess(
                {"run", "--reactor", "--dir", ".:.", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--env", "HOME=/tmp",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--memory-page-limit",
                                  "256", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
}

// Test --module flag for linking external modules with the run subcommand.
// Same semantics as instantiate --module (name:path format).
// Commands tested:
//   wasmedge run --reactor --module provider:<provider.wasm> consumer_r.wasm
//               add 3 5
//   wasmedge run --reactor --module badformat consumer_r.wasm add 1 2
//   wasmedge run --reactor --module provider:nonexist.wasm consumer_r.wasm
//               add 1 2
TEST(RunSubcommand, LinkedModules) {
  std::string ProvPath = providerPath();
  std::string ConsPath = consumerPath();
  std::string ModArg = "provider:" + ProvPath;

  EXPECT_EQ(
      callUniToolInProcess({"run", "--reactor", "--module", ModArg.c_str(),
                            ConsPath.c_str(), "add", "3", "5"}),
      EXIT_SUCCESS);

  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--module", "badformat",
                                  ConsPath.c_str(), "add", "1", "2"}),
            EXIT_FAILURE);

  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--module",
                                  "provider:nonexist.wasm", ConsPath.c_str(),
                                  "add", "1", "2"}),
            EXIT_FAILURE);
}

// Test global flags (available to all subcommands via addGlobalOptions).
// Commands tested:
//   wasmedge run --reactor --log-level error simple_r.wasm add 1 2
//   wasmedge run --reactor --forbidden-plugin someplugin simple_r.wasm add 1 2
//   wasmedge run --reactor --forbidden-plugin pA --forbidden-plugin pB
//               simple_r.wasm add 1 2
TEST(RunSubcommand, GlobalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--log-level", "error",
                                  Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"run", "--reactor", "--forbidden-plugin",
                                  "someplugin", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callUniToolInProcess({"run", "--reactor", "--forbidden-plugin", "pA",
                            "--forbidden-plugin", "pB", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
}

// Test that invoking WasmEdge_Driver_UniTool without a subcommand falls back
// to the "run" behavior. When no subcommand is given, the first positional
// argument is treated as the wasm file to run.
// Commands tested:
//   wasmedge --reactor simple_r.wasm add 3 5       (fallback to run)
//   wasmedge nonexist.wasm                          (fallback to run, file
//                                                    missing)
//   wasmedge --reactor --force-interpreter simple_r.wasm add 1 2
TEST(NoSubcommand, FallbackToRun) {
  // Without a subcommand, --reactor <file> <func> <args> should work as "run".
  EXPECT_EQ(callUniToolInProcess(
                {"--reactor", simplePath().c_str(), "add", "3", "5"}),
            EXIT_SUCCESS);

  // Without a subcommand, a non-existent file should still fail like "run".
  EXPECT_NE(callUniToolInProcess({nonExistPath().c_str()}), EXIT_SUCCESS);

  // Run-specific flags should also work without the "run" subcommand.
  EXPECT_EQ(callUniToolInProcess({"--reactor", "--force-interpreter",
                                  simplePath().c_str(), "add", "1", "2"}),
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