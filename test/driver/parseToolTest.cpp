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

// Runs WasmEdge_Driver_UniTool in a forked child process and returns its exit
// code. Forking is necessary because UniTool with ToolType::All loads plugins
// into global state, which corrupts on repeated in-process calls.
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

struct ToolResult {
  int ExitCode;
  std::string Stdout;
};

// Same as callUniToolInProcess but also captures the child's stdout via a pipe.
// Used for output-verification tests that check section headers, names, etc.
ToolResult callUniToolCaptureStdout(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  int Pipe[2];
  if (pipe(Pipe) != 0) {
    return {-1, {}};
  }
  pid_t Pid = fork();
  if (Pid == 0) {
    close(Pipe[0]);
    dup2(Pipe[1], STDOUT_FILENO);
    close(Pipe[1]);
    int Ret =
        WasmEdge_Driver_UniTool(static_cast<int>(Argv.size()), Argv.data());
    fflush(stdout);
    _exit(Ret);
  }
  close(Pipe[1]);
  std::string Output;
  char Buf[4096];
  ssize_t N;
  while ((N = read(Pipe[0], Buf, sizeof(Buf))) > 0) {
    Output.append(Buf, static_cast<size_t>(N));
  }
  close(Pipe[0]);
  int Status = 0;
  waitpid(Pid, &Status, 0);
  int Code = WIFEXITED(Status) ? WEXITSTATUS(Status) : -1;
  return {Code, std::move(Output)};
}

// Checks that Output contains every string in Needles.
// On failure, reports which substring was missing and prints the full output.
::testing::AssertionResult
containsAll(const std::string &Output,
            std::initializer_list<const char *> Needles) {
  for (const char *Needle : Needles) {
    if (Output.find(Needle) == std::string::npos) {
      return ::testing::AssertionFailure()
             << "output missing substring: \"" << Needle << "\"\n"
             << "full output:\n"
             << Output;
    }
  }
  return ::testing::AssertionSuccess();
}

std::string writeWasmToFile(const uint8_t *Data, size_t Size,
                            const std::string &FileName) {
  std::string Path = TestDataPath + "/" + FileName;
  std::ofstream Ofs(Path, std::ios::binary);
  Ofs.write(reinterpret_cast<const char *>(Data), Size);
  Ofs.close();
  return Path;
}

// parse_test.wasm: compiled from parse_test.wat with --debug-names.
// Contains: 4 types, 5 imports (func, memory, table, 2 globals),
// 4 functions, 6 globals (various types + init exprs), 12 exports,
// name section with function and global names.
static const std::array<uint8_t, 502> ParseTestWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x15, 0x04, 0x60,
    0x02, 0x7f, 0x7f, 0x02, 0x7f, 0x7f, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00,
    0x00, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02, 0x44, 0x05, 0x03, 0x65,
    0x6e, 0x76, 0x03, 0x6c, 0x6f, 0x67, 0x00, 0x01, 0x03, 0x65, 0x6e, 0x76,
    0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x01, 0x01, 0x0a, 0x03,
    0x65, 0x6e, 0x76, 0x05, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x01, 0x70, 0x00,
    0x01, 0x03, 0x65, 0x6e, 0x76, 0x05, 0x67, 0x5f, 0x6d, 0x75, 0x74, 0x03,
    0x7f, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x07, 0x67, 0x5f, 0x63, 0x6f, 0x6e,
    0x73, 0x74, 0x03, 0x7f, 0x00, 0x03, 0x05, 0x04, 0x02, 0x03, 0x00, 0x01,
    0x06, 0x2a, 0x06, 0x7f, 0x00, 0x41, 0x2a, 0x0b, 0x7e, 0x00, 0x42, 0xe4,
    0x00, 0x0b, 0x7d, 0x00, 0x43, 0xc3, 0xf5, 0x48, 0x40, 0x0b, 0x7c, 0x00,
    0x44, 0x58, 0x39, 0xb4, 0xc8, 0x76, 0xbe, 0x05, 0x40, 0x0b, 0x7f, 0x01,
    0x41, 0x00, 0x0b, 0x7f, 0x00, 0x23, 0x01, 0x0b, 0x07, 0x80, 0x01, 0x0c,
    0x05, 0x67, 0x5f, 0x69, 0x33, 0x32, 0x03, 0x02, 0x05, 0x67, 0x5f, 0x69,
    0x36, 0x34, 0x03, 0x03, 0x05, 0x67, 0x5f, 0x66, 0x33, 0x32, 0x03, 0x04,
    0x05, 0x67, 0x5f, 0x66, 0x36, 0x34, 0x03, 0x05, 0x0b, 0x67, 0x5f, 0x6d,
    0x75, 0x74, 0x5f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x03, 0x06, 0x08, 0x6e,
    0x6f, 0x70, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x01, 0x08, 0x61, 0x64,
    0x64, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x02, 0x09, 0x73, 0x77, 0x61,
    0x70, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x03, 0x0b, 0x63, 0x61, 0x6c,
    0x6c, 0x5f, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x00, 0x04, 0x06, 0x6d,
    0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x05, 0x74, 0x61, 0x62, 0x6c,
    0x65, 0x01, 0x00, 0x0d, 0x67, 0x5f, 0x66, 0x72, 0x6f, 0x6d, 0x5f, 0x69,
    0x6d, 0x70, 0x6f, 0x72, 0x74, 0x03, 0x07, 0x0a, 0x1b, 0x04, 0x03, 0x00,
    0x01, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b, 0x06, 0x00,
    0x20, 0x01, 0x20, 0x00, 0x0b, 0x06, 0x00, 0x20, 0x00, 0x10, 0x00, 0x0b,
    0x00, 0xbb, 0x01, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x3c, 0x05, 0x00,
    0x0d, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x5f, 0x66, 0x75,
    0x6e, 0x63, 0x01, 0x08, 0x6e, 0x6f, 0x70, 0x5f, 0x66, 0x75, 0x6e, 0x63,
    0x02, 0x08, 0x61, 0x64, 0x64, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x03, 0x09,
    0x73, 0x77, 0x61, 0x70, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x04, 0x0b, 0x63,
    0x61, 0x6c, 0x6c, 0x5f, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x02, 0x0b,
    0x05, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04,
    0x0c, 0x01, 0x00, 0x09, 0x73, 0x77, 0x61, 0x70, 0x5f, 0x74, 0x79, 0x70,
    0x65, 0x07, 0x5b, 0x08, 0x00, 0x0e, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74,
    0x65, 0x64, 0x5f, 0x67, 0x5f, 0x6d, 0x75, 0x74, 0x01, 0x10, 0x69, 0x6d,
    0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x5f, 0x67, 0x5f, 0x63, 0x6f, 0x6e,
    0x73, 0x74, 0x02, 0x05, 0x67, 0x5f, 0x69, 0x33, 0x32, 0x03, 0x05, 0x67,
    0x5f, 0x69, 0x36, 0x34, 0x04, 0x05, 0x67, 0x5f, 0x66, 0x33, 0x32, 0x05,
    0x05, 0x67, 0x5f, 0x66, 0x36, 0x34, 0x06, 0x0b, 0x67, 0x5f, 0x6d, 0x75,
    0x74, 0x5f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x07, 0x0d, 0x67, 0x5f, 0x66,
    0x72, 0x6f, 0x6d, 0x5f, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74};

// mem_no_max.wasm: imports env.memory with min=1, no max limit.
// Tests that parse handles memory without a max page count.
static const std::array<uint8_t, 25> MemNoMaxWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x02,
    0x0f, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x06, 0x6d, 0x65,
    0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x01};

// Truncated wasm: valid magic + version, then garbage bytes.
static const std::array<uint8_t, 12> InvalidWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

// ELF header bytes, not wasm at all.
static const std::array<uint8_t, 8> NotWasmBytes{0x7f, 0x45, 0x4c, 0x46,
                                                 0x00, 0x00, 0x00, 0x00};

// Minimal valid wasm: just the magic number and version, no sections.
static const std::array<uint8_t, 8> MinimalWasm{0x00, 0x61, 0x73, 0x6d,
                                                0x01, 0x00, 0x00, 0x00};

std::string parseTestPath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(ParseTestWasm.data(), ParseTestWasm.size(),
                           "parse_test.wasm");
  }
  return Path;
}

// Test error handling for invalid inputs to the parse subcommand.
// Covers: missing argument, empty path, non-existent file, directory as input,
// corrupted wasm content, and non-wasm binary input.
// Commands tested:
//   wasmedge parse
//   wasmedge parse ""
//   wasmedge parse driverTestData/nonexist.wasm
//   wasmedge parse driverTestData/
//   wasmedge parse <truncated-wasm>
//   wasmedge parse <elf-binary>
TEST(ParseSubcommand, ErrorHandling) {
  // No file argument provided.
  EXPECT_NE(callUniToolInProcess({"parse"}), EXIT_SUCCESS);

  // Empty string as file path.
  EXPECT_NE(callUniToolInProcess({"parse", ""}), EXIT_SUCCESS);

  // File does not exist on disk.
  std::string NonExist = TestDataPath + "/nonexist.wasm";
  EXPECT_NE(callUniToolInProcess({"parse", NonExist.c_str()}), EXIT_SUCCESS);

  // Directory path instead of a file.
  EXPECT_NE(callUniToolInProcess({"parse", TestDataPath.c_str()}),
            EXIT_SUCCESS);

  // Valid wasm magic but truncated/garbage section data.
  std::string InvalidPath =
      writeWasmToFile(InvalidWasm.data(), InvalidWasm.size(), "invalid_p.wasm");
  EXPECT_NE(callUniToolInProcess({"parse", InvalidPath.c_str()}), EXIT_SUCCESS);
  std::remove(InvalidPath.c_str());

  // Not wasm at all (ELF magic bytes).
  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf_p.wasm");
  EXPECT_NE(callUniToolInProcess({"parse", ElfPath.c_str()}), EXIT_SUCCESS);
  std::remove(ElfPath.c_str());

  // Unknown CLI flag should be rejected.
  // Command: wasmedge parse --no-such-flag parse_test.wasm
  EXPECT_NE(callUniToolInProcess(
                {"parse", "--no-such-flag", parseTestPath().c_str()}),
            EXIT_SUCCESS);

  // Extra positional argument after the wasm file is not allowed.
  // Command: wasmedge parse parse_test.wasm extra-arg
  EXPECT_NE(
      callUniToolInProcess({"parse", parseTestPath().c_str(), "extra-arg"}),
      EXIT_SUCCESS);
}

// Test that parse succeeds on valid modules of different sizes.
// Commands tested:
//   wasmedge parse parse_test.wasm       (full-featured module)
//   wasmedge parse minimal_p.wasm        (header-only, no sections)
//   wasmedge parse memnomax_p.wasm       (imported memory without max limit)
TEST(ParseSubcommand, ValidModules) {
  // Full-featured module with types, imports, globals, name section.
  EXPECT_EQ(callUniToolInProcess({"parse", parseTestPath().c_str()}),
            EXIT_SUCCESS);

  // Minimal module: only the 8-byte wasm header, all section counts are 0.
  std::string MinPath =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "minimal_p.wasm");
  EXPECT_EQ(callUniToolInProcess({"parse", MinPath.c_str()}), EXIT_SUCCESS);
  std::remove(MinPath.c_str());

  // Module that imports memory with no max page limit.
  // Verifies parse handles the "pages: initial=N" format (no max field).
  std::string MemPath = writeWasmToFile(MemNoMaxWasm.data(),
                                        MemNoMaxWasm.size(), "memnomax_p.wasm");
  EXPECT_EQ(callUniToolInProcess({"parse", MemPath.c_str()}), EXIT_SUCCESS);
  std::remove(MemPath.c_str());
}

// Test all individual --disable-* proposal flags with the parse subcommand.
// Each flag disables a specific wasm proposal during loading.
// Most flags still allow parse_test.wasm to load successfully because the
// module doesn't use the disabled feature. The exception is
// --disable-import-export-mut-globals which rejects mutable global imports.
// Commands tested:
//   wasmedge parse --disable-simd parse_test.wasm
//   wasmedge parse --disable-bulk-memory parse_test.wasm
//   wasmedge parse --disable-reference-types parse_test.wasm
//   wasmedge parse --disable-multi-value parse_test.wasm
//   wasmedge parse --disable-non-trap-float-to-int parse_test.wasm
//   wasmedge parse --disable-sign-extension-operators parse_test.wasm
//   wasmedge parse --disable-tail-call parse_test.wasm
//   wasmedge parse --disable-extended-const parse_test.wasm
//   wasmedge parse --disable-multi-memory parse_test.wasm
//   wasmedge parse --disable-relaxed-simd parse_test.wasm
//   wasmedge parse --disable-memory64 parse_test.wasm
//   wasmedge parse --disable-gc parse_test.wasm
//   wasmedge parse --disable-function-reference parse_test.wasm
//   wasmedge parse --disable-exception-handling parse_test.wasm
//   wasmedge parse --disable-import-export-mut-globals minimal_p.wasm
//   wasmedge parse --disable-import-export-mut-globals parse_test.wasm
TEST(ParseSubcommand, DisableProposalFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-simd", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-bulk-memory", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-reference-types", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-multi-value", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-non-trap-float-to-int", Path}),
      EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-sign-extension-operators", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-tail-call", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-extended-const", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-multi-memory", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-relaxed-simd", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-memory64", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-gc", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-function-reference", Path}),
      EXIT_SUCCESS);
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-exception-handling", Path}),
      EXIT_SUCCESS);

  // --disable-import-export-mut-globals on a module without mutable global
  // imports should succeed.
  std::string MinPath =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "min_mg_p.wasm");
  EXPECT_EQ(
      callUniToolInProcess(
          {"parse", "--disable-import-export-mut-globals", MinPath.c_str()}),
      EXIT_SUCCESS);
  std::remove(MinPath.c_str());

  // parse_test.wasm imports a mutable global (env.g_mut), so disabling
  // mutable globals should cause a load failure.
  EXPECT_NE(callUniToolInProcess(
                {"parse", "--disable-import-export-mut-globals", Path}),
            EXIT_SUCCESS);
}

// Test --enable-* flags and deprecated enable aliases.
// These are accepted by the parser and should not cause failures.
// Commands tested:
//   wasmedge parse --enable-all parse_test.wasm
//   wasmedge parse --enable-threads parse_test.wasm
//   wasmedge parse --enable-component parse_test.wasm
//   wasmedge parse --enable-tail-call parse_test.wasm
//   wasmedge parse --enable-extended-const parse_test.wasm
//   wasmedge parse --enable-function-reference parse_test.wasm
//   wasmedge parse --enable-gc parse_test.wasm
//   wasmedge parse --enable-multi-memory parse_test.wasm
//   wasmedge parse --enable-relaxed-simd parse_test.wasm
//   wasmedge parse --enable-exception-handling parse_test.wasm
TEST(ParseSubcommand, EnableProposalFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-all", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-threads", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-component", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-tail-call", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-extended-const", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--enable-function-reference", Path}),
      EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-gc", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-multi-memory", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-relaxed-simd", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--enable-exception-handling", Path}),
      EXIT_SUCCESS);
}

// Test --wasm-N standard selection flags.
// Each selects a different base proposal set; all should parse successfully.
// Commands tested:
//   wasmedge parse --wasm-1 parse_test.wasm
//   wasmedge parse --wasm-2 parse_test.wasm
//   wasmedge parse --wasm-3 parse_test.wasm
TEST(ParseSubcommand, WasmStandardFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callUniToolInProcess({"parse", "--wasm-1", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--wasm-2", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callUniToolInProcess({"parse", "--wasm-3", Path}), EXIT_SUCCESS);
}

// Test combinations of multiple flags in a single invocation.
// Ensures the argument parser handles multiple flags correctly.
// Commands tested:
//   wasmedge parse --disable-simd --disable-bulk-memory --disable-multi-value
//                  parse_test.wasm
//   wasmedge parse --disable-simd --enable-threads parse_test.wasm
//   wasmedge parse --wasm-2 --disable-simd parse_test.wasm
TEST(ParseSubcommand, CombinedFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  // Multiple disable flags together.
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-simd", "--disable-bulk-memory",
                            "--disable-multi-value", Path}),
      EXIT_SUCCESS);

  // Mixed disable + enable flags.
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-simd", "--enable-threads", Path}),
            EXIT_SUCCESS);

  // Standard selection combined with a disable flag.
  EXPECT_EQ(callUniToolInProcess({"parse", "--wasm-2", "--disable-simd", Path}),
            EXIT_SUCCESS);
}

// Test --forbidden-plugin flag with parse subcommand.
// This flag is accepted at the parser level and should not affect loading.
// Commands tested:
//   wasmedge parse --forbidden-plugin someplugin parse_test.wasm
//   wasmedge parse --forbidden-plugin plugA --forbidden-plugin plugB
//                  parse_test.wasm
TEST(ParseSubcommand, ForbiddenPluginFlag) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(
      callUniToolInProcess({"parse", "--forbidden-plugin", "someplugin", Path}),
      EXIT_SUCCESS);

  EXPECT_EQ(callUniToolInProcess({"parse", "--forbidden-plugin", "plugA",
                                  "--forbidden-plugin", "plugB", Path}),
            EXIT_SUCCESS);
}

// Verify that the parse output contains the expected section headers and counts
// matching the wasm-objdump -x format.
// Command: wasmedge parse parse_test.wasm
TEST(ParseSubcommand, OutputSectionHeaders) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout,
      {"file format wasm 0x1", "Section Details:", "Type[4]:", "Import[5]:",
       "Function[4]:", "Global[6]:", "Export[12]:", "Code[4]:", "Custom:"}));
}

// Verify that function and global names from the "name" custom section appear
// in the output, annotated with angle brackets like wasm-objdump.
// Command: wasmedge parse parse_test.wasm
TEST(ParseSubcommand, OutputNameAnnotations) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout, {"<imported_func>", "<nop_func>", "<add_func>", "<swap_func>",
                 "<call_import>", "<imported_g_mut>", "<imported_g_const>",
                 "<g_i32>", "<g_f64>", "<g_mut_local>", "- name: \"name\""}));
}

// Verify import and export formatting in the parse output.
// Imports should show "<- module.name", exports should show '-> "name"'.
// Command: wasmedge parse parse_test.wasm
TEST(ParseSubcommand, OutputImportsAndExports) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout, {"<- env.log", "<- env.memory", "<- env.table", "<- env.g_mut",
                 "<- env.g_const", "-> \"nop_func\"", "-> \"add_func\"",
                 "-> \"swap_func\"", "-> \"call_import\"", "-> \"memory\"",
                 "-> \"table\"", "-> \"g_i32\""}));
}

// Verify global initialization expressions appear in the output.
// The parse_test.wasm has globals initialized with i32.const, i64.const,
// f32.const, f64.const, and global.get instructions.
// Command: wasmedge parse parse_test.wasm
TEST(ParseSubcommand, OutputGlobalInitExpressions) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(R.Stdout, {"init i32=42", "init i64=100", "init f32=",
                                     "init f64=", "init global.get 1"}));
}

// Verify that a minimal module (header only, no sections) shows all zero counts
// in the section summary.
// Command: wasmedge parse minimal_out_p.wasm
TEST(ParseSubcommand, MinimalModuleEmptyCounts) {
  std::string Path = writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(),
                                     "minimal_out_p.wasm");
  auto R = callUniToolCaptureStdout({"parse", Path.c_str()});
  std::remove(Path.c_str());
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(R.Stdout, {"Type[0]:", "Import[0]:", "Function[0]:",
                                     "Global[0]:", "Export[0]:", "Code[0]:"}));
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
