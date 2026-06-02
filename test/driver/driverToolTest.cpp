// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "driver/tool.h"
#include "driver/unitool.h"
#include "po/argument_parser.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#if !WASMEDGE_OS_WINDOWS
#include <unistd.h>
#endif
#include <vector>

namespace {

std::string TestDataPath;

std::string writeWasmToFile(const uint8_t *Data, size_t Size,
                            const std::string &FileName) {
  std::string Path = TestDataPath + "/" + FileName;
  std::filesystem::create_directories(TestDataPath);
  std::ofstream Ofs(Path, std::ios::binary);
  Ofs.write(reinterpret_cast<const char *>(Data),
            static_cast<std::streamsize>(Size));
  Ofs.close();
  return Path;
}

int callParse(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  return WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                   WasmEdge::Driver::ToolType::Parse);
}

int callValidate(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  return WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                   WasmEdge::Driver::ToolType::Validate);
}

int callCompile(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  return WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                   WasmEdge::Driver::ToolType::Compiler);
}

/* TODO: Uncomment when plugin re-entrancy issue is fixed.
int callInstantiate(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  return WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                   WasmEdge::Driver::ToolType::Instantiate);
}

int callRun(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  return WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                   WasmEdge::Driver::ToolType::Tool);
}

int callUniToolAll(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  return WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                   WasmEdge::Driver::ToolType::All);
}
*/

#if !WASMEDGE_OS_WINDOWS
struct ToolResult {
  int ExitCode;
  std::string Stdout;
};

ToolResult callParseCaptureStdout(std::initializer_list<const char *> Args) {
  std::vector<const char *> Argv = {"wasmedge"};
  Argv.insert(Argv.end(), Args.begin(), Args.end());
  int Pipe[2];
  if (pipe(Pipe) != 0) {
    return {-1, {}};
  }
  int SavedStdout = dup(STDOUT_FILENO);
  dup2(Pipe[1], STDOUT_FILENO);
  close(Pipe[1]);
  int Ret =
      WasmEdge::Driver::UniTool(static_cast<int>(Argv.size()), Argv.data(),
                                WasmEdge::Driver::ToolType::Parse);
  fflush(stdout);
  dup2(SavedStdout, STDOUT_FILENO);
  close(SavedStdout);
  std::string Output;
  char Buf[4096];
  ssize_t N;
  while ((N = read(Pipe[0], Buf, sizeof(Buf))) > 0) {
    Output.append(Buf, static_cast<size_t>(N));
  }
  close(Pipe[0]);
  return {Ret, std::move(Output)};
}

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
#endif

// simple.wasm: self-contained module exporting add, sub, memory, counter.
// Has types, functions, memory, mutable global, name section.
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

// invalid.wasm: declares (func (result i32)) but body pushes two i32 values
// without consuming the extra one, causing a type mismatch during validation.
static const std::array<uint8_t, 29> InvalidWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05,
    0x01, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x0a,
    0x08, 0x01, 0x06, 0x00, 0x41, 0x01, 0x41, 0x02, 0x0b};

// Truncated wasm: valid magic + version, then garbage bytes.
static const std::array<uint8_t, 12> TruncatedWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

// ELF header bytes, not wasm at all.
static const std::array<uint8_t, 8> NotWasmBytes{0x7f, 0x45, 0x4c, 0x46,
                                                 0x00, 0x00, 0x00, 0x00};

// Minimal valid wasm: just the magic number and version, no sections.
static const std::array<uint8_t, 8> MinimalWasm{0x00, 0x61, 0x73, 0x6d,
                                                0x01, 0x00, 0x00, 0x00};

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
static const std::array<uint8_t, 25> MemNoMaxWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x02,
    0x0f, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x06, 0x6d, 0x65,
    0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x01};

/* TODO: Uncomment when plugin re-entrancy issue is fixed.
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
*/

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

std::string parseTestPath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(ParseTestWasm.data(), ParseTestWasm.size(),
                           "parse_test.wasm");
  }
  return Path;
}

/* TODO: Uncomment when plugin re-entrancy issue is fixed.
std::string providerPath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(ProviderWasm.data(), ProviderWasm.size(),
                           "provider.wasm");
  }
  return Path;
}

std::string consumerPath() {
  static std::string Path;
  if (Path.empty()) {
    Path = writeWasmToFile(ConsumerWasm.data(), ConsumerWasm.size(),
                           "consumer.wasm");
  }
  return Path;
}
*/

std::string nonExistPath() { return TestDataPath + "/nonexist.wasm"; }

// ---------------------------------------------------------------------------
// ParseSubcommand tests
// ---------------------------------------------------------------------------

TEST(ParseSubcommand, ErrorHandling) {
  EXPECT_NE(callParse({}), EXIT_SUCCESS);

  std::string NonExist = TestDataPath + "/nonexist.wasm";
  EXPECT_NE(callParse({NonExist.c_str()}), EXIT_SUCCESS);
  EXPECT_NE(callParse({TestDataPath.c_str()}), EXIT_SUCCESS);

  std::string InvalidPath =
      writeWasmToFile(TruncatedWasm.data(), TruncatedWasm.size(), "trunc.wasm");
  EXPECT_NE(callParse({InvalidPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(InvalidPath.c_str());

  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf.wasm");
  EXPECT_NE(callParse({ElfPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(ElfPath.c_str());

  EXPECT_NE(callParse({"--no-such-flag", parseTestPath().c_str()}),
            EXIT_SUCCESS);
  EXPECT_NE(callParse({parseTestPath().c_str(), "extra-arg"}), EXIT_SUCCESS);
}

TEST(ParseSubcommand, ValidModules) {
  EXPECT_EQ(callParse({parseTestPath().c_str()}), EXIT_SUCCESS);

  std::string MinPath =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "minimal.wasm");
  EXPECT_EQ(callParse({MinPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(MinPath.c_str());

  std::string MemPath = writeWasmToFile(MemNoMaxWasm.data(),
                                        MemNoMaxWasm.size(), "memnomax.wasm");
  EXPECT_EQ(callParse({MemPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(MemPath.c_str());
}

TEST(ParseSubcommand, DisableProposalFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callParse({"--disable-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-bulk-memory", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-reference-types", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-multi-value", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-non-trap-float-to-int", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-sign-extension-operators", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-tail-call", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-extended-const", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-multi-memory", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-relaxed-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-memory64", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-gc", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-function-reference", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-exception-handling", Path}), EXIT_SUCCESS);

  std::string MinPath =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "min_mg.wasm");
  EXPECT_EQ(callParse({"--disable-import-export-mut-globals", MinPath.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(MinPath.c_str());

  EXPECT_NE(callParse({"--disable-import-export-mut-globals", Path}),
            EXIT_SUCCESS);
}

TEST(ParseSubcommand, EnableProposalFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callParse({"--enable-all", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-threads", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-component", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-tail-call", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-extended-const", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-function-reference", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-gc", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-multi-memory", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-relaxed-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--enable-exception-handling", Path}), EXIT_SUCCESS);
}

TEST(ParseSubcommand, WasmStandardFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callParse({"--wasm-1", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--wasm-2", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--wasm-3", Path}), EXIT_SUCCESS);
}

TEST(ParseSubcommand, CombinedFlags) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callParse({"--disable-simd", "--disable-bulk-memory",
                       "--disable-multi-value", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--disable-simd", "--enable-threads", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--wasm-2", "--disable-simd", Path}), EXIT_SUCCESS);
}

TEST(ParseSubcommand, ForbiddenPluginFlag) {
  std::string PathStr = parseTestPath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callParse({"--forbidden-plugin", "someplugin", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callParse({"--forbidden-plugin", "plugA", "--forbidden-plugin",
                       "plugB", Path}),
            EXIT_SUCCESS);
}

#if !WASMEDGE_OS_WINDOWS
TEST(ParseSubcommand, OutputSectionHeaders) {
  auto R = callParseCaptureStdout({parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout,
      {"file format wasm 0x1", "Section Details:", "Type[4]:", "Import[5]:",
       "Function[4]:", "Global[6]:", "Export[12]:", "Code[4]:", "Custom:"}));
}

TEST(ParseSubcommand, OutputNameAnnotations) {
  auto R = callParseCaptureStdout({parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout, {"<imported_func>", "<nop_func>", "<add_func>", "<swap_func>",
                 "<call_import>", "<imported_g_mut>", "<imported_g_const>",
                 "<g_i32>", "<g_f64>", "<g_mut_local>", "- name: \"name\""}));
}

TEST(ParseSubcommand, OutputImportsAndExports) {
  auto R = callParseCaptureStdout({parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout, {"<- env.log", "<- env.memory", "<- env.table", "<- env.g_mut",
                 "<- env.g_const", "-> \"nop_func\"", "-> \"add_func\"",
                 "-> \"swap_func\"", "-> \"call_import\"", "-> \"memory\"",
                 "-> \"table\"", "-> \"g_i32\""}));
}

TEST(ParseSubcommand, OutputGlobalInitExpressions) {
  auto R = callParseCaptureStdout({parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(R.Stdout, {"init i32=42", "init i64=100", "init f32=",
                                     "init f64=", "init global.get 1"}));
}

TEST(ParseSubcommand, MinimalModuleEmptyCounts) {
  std::string Path = writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(),
                                     "minimal_out.wasm");
  auto R = callParseCaptureStdout({Path.c_str()});
  std::filesystem::remove(Path.c_str());
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(R.Stdout, {"Type[0]:", "Import[0]:", "Function[0]:",
                                     "Global[0]:", "Export[0]:", "Code[0]:"}));
}
#endif

// ---------------------------------------------------------------------------
// ValidateSubcommand tests
// ---------------------------------------------------------------------------

TEST(ValidateSubcommand, ErrorHandling) {
  EXPECT_NE(callValidate({}), EXIT_SUCCESS);
  EXPECT_NE(callValidate({nonExistPath().c_str()}), EXIT_SUCCESS);
  EXPECT_NE(callValidate({TestDataPath.c_str()}), EXIT_SUCCESS);

  std::string TruncPath =
      writeWasmToFile(TruncatedWasm.data(), TruncatedWasm.size(), "trunc.wasm");
  EXPECT_NE(callValidate({TruncPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(TruncPath.c_str());

  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf.wasm");
  EXPECT_NE(callValidate({ElfPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(ElfPath.c_str());

  EXPECT_NE(callValidate({"--no-such-flag", simplePath().c_str()}),
            EXIT_SUCCESS);
  EXPECT_NE(callValidate({simplePath().c_str(), "extra-arg"}), EXIT_SUCCESS);
}

TEST(ValidateSubcommand, ValidAndInvalidModules) {
  EXPECT_EQ(callValidate({simplePath().c_str()}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({invalidPath().c_str()}), EXIT_FAILURE);

  std::string MinPath =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "minimal.wasm");
  EXPECT_EQ(callValidate({MinPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(MinPath.c_str());

  EXPECT_EQ(callValidate({parseTestPath().c_str()}), EXIT_SUCCESS);
}

TEST(ValidateSubcommand, DisableProposalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callValidate({"--disable-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-bulk-memory", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-reference-types", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-multi-value", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-non-trap-float-to-int", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-sign-extension-operators", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-tail-call", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-extended-const", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-multi-memory", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-relaxed-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-memory64", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-gc", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-function-reference", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-exception-handling", Path}), EXIT_SUCCESS);

  EXPECT_EQ(callValidate({"--disable-import-export-mut-globals", Path}),
            EXIT_SUCCESS);
  EXPECT_NE(callValidate({"--disable-import-export-mut-globals",
                          parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(ValidateSubcommand, EnableProposalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callValidate({"--enable-all", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-threads", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-component", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-tail-call", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-extended-const", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-function-reference", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-gc", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-multi-memory", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-relaxed-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--enable-exception-handling", Path}), EXIT_SUCCESS);
}

TEST(ValidateSubcommand, WasmStandardFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callValidate({"--wasm-1", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--wasm-2", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--wasm-3", Path}), EXIT_SUCCESS);
}

TEST(ValidateSubcommand, CombinedFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callValidate({"--disable-simd", "--disable-bulk-memory", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--disable-simd", "--enable-threads", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callValidate({"--wasm-2", "--disable-simd", Path}), EXIT_SUCCESS);
}

TEST(ValidateSubcommand, ForbiddenPluginFlag) {
  EXPECT_EQ(
      callValidate({"--forbidden-plugin", "someplugin", simplePath().c_str()}),
      EXIT_SUCCESS);
}

// ---------------------------------------------------------------------------
// CompileSubcommand tests
// ---------------------------------------------------------------------------

TEST(CompileSubcommand, ErrorHandling) {
  std::string Output = TestDataPath + "/err_out.wasm";

  EXPECT_NE(callCompile({}), EXIT_SUCCESS);
  EXPECT_NE(callCompile({simplePath().c_str()}), EXIT_SUCCESS);
  EXPECT_NE(callCompile({nonExistPath().c_str(), Output.c_str()}),
            EXIT_SUCCESS);

  std::string TruncPath =
      writeWasmToFile(TruncatedWasm.data(), TruncatedWasm.size(), "trunc.wasm");
  EXPECT_NE(callCompile({TruncPath.c_str(), Output.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(TruncPath.c_str());

  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf.wasm");
  EXPECT_NE(callCompile({ElfPath.c_str(), Output.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(ElfPath.c_str());

  EXPECT_NE(
      callCompile({"--no-such-flag", simplePath().c_str(), Output.c_str()}),
      EXIT_SUCCESS);

  std::filesystem::remove(Output.c_str());
}

TEST(CompileSubcommand, ValidAndInvalidModules) {
  std::string Output = TestDataPath + "/simple_aot.wasm";
  EXPECT_EQ(callCompile({simplePath().c_str(), Output.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  std::string BadOutput = TestDataPath + "/invalid_aot.wasm";
  EXPECT_NE(callCompile({invalidPath().c_str(), BadOutput.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(BadOutput.c_str());
}

TEST(CompileSubcommand, ProposalFlags) {
  std::string Output = TestDataPath + "/proposal_out.wasm";
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callCompile({"--disable-simd", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--enable-all", Path, Output.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--wasm-2", Path, Output.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(
      callCompile({"--disable-simd", "--enable-threads", Path, Output.c_str()}),
      EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());
}

TEST(CompileSubcommand, CompilerSpecificFlags) {
  std::string Output = TestDataPath + "/compiler_out.wasm";
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callCompile({"--generic-binary", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--interruptible", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--dump", Path, Output.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--enable-instruction-count", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--enable-gas-measuring", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--enable-time-measuring", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--enable-all-statistics", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "0", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "1", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "2", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "3", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "s", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "z", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());

  EXPECT_EQ(callCompile({"--optimize", "invalid", Path, Output.c_str()}),
            EXIT_SUCCESS);
  std::filesystem::remove(Output.c_str());
}

TEST(CompileSubcommand, OutputFormat) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  std::string WasmOutput = TestDataPath + "/fmt_out.wasm";
  EXPECT_EQ(callCompile({Path, WasmOutput.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(WasmOutput.c_str());

  std::string NativeOutput = TestDataPath + "/fmt_out" WASMEDGE_LIB_EXTENSION;
  EXPECT_EQ(callCompile({Path, NativeOutput.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(NativeOutput.c_str());
}

TEST(RunSubcommand, RunModeFlagParses) {
  WasmEdge::Driver::DriverToolOptions Opt;
  WasmEdge::PO::ArgumentParser Parser;
  Opt.addOptions(Parser);

  const char *Argv[] = {"wasmedge", "--enable-jit", "--run-mode=jit",
                        "dummy.wasm"};
  EXPECT_TRUE(Parser.parse(stdout, 4, Argv));
  EXPECT_TRUE(Opt.ConfEnableJIT.value());
  EXPECT_EQ(Opt.ConfRunMode.value(), "jit");
}

/* ---------------------------------------------------------------------------
 * InstantiateSubcommand tests
 * TODO: Commented out due to plugin re-entrancy issue.
 * UniTool with ToolType::Instantiate calls addLinkerOptions ->
 * loadFromDefaultPaths -> addPluginOptions, which leaves dangling pointers
 * on repeated calls. Uncomment when the plugin system is made re-entrant.
 * ---------------------------------------------------------------------------

TEST(InstantiateSubcommand, ErrorHandling) {
  EXPECT_NE(callInstantiate({}), EXIT_SUCCESS);
  EXPECT_NE(callInstantiate({""}), EXIT_SUCCESS);
  EXPECT_NE(callInstantiate({nonExistPath().c_str()}), EXIT_SUCCESS);
  EXPECT_NE(callInstantiate({TestDataPath.c_str()}), EXIT_SUCCESS);

  std::string TruncPath =
      writeWasmToFile(TruncatedWasm.data(), TruncatedWasm.size(), "trunc.wasm");
  EXPECT_NE(callInstantiate({TruncPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(TruncPath.c_str());

  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf.wasm");
  EXPECT_NE(callInstantiate({ElfPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(ElfPath.c_str());

  EXPECT_NE(callInstantiate({"--no-such-flag", simplePath().c_str()}),
            EXIT_SUCCESS);
  EXPECT_NE(callInstantiate({simplePath().c_str(), "extra-arg"}), EXIT_SUCCESS);
}

TEST(InstantiateSubcommand, ValidAndInvalidModules) {
  EXPECT_EQ(callInstantiate({simplePath().c_str()}), EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({invalidPath().c_str()}), EXIT_FAILURE);

  std::string MinPath =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "minimal.wasm");
  EXPECT_EQ(callInstantiate({MinPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(MinPath.c_str());

  EXPECT_NE(callInstantiate({parseTestPath().c_str()}), EXIT_SUCCESS);
}

TEST(InstantiateSubcommand, ProposalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callInstantiate({"--disable-simd", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--enable-all", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--wasm-2", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--disable-simd", "--enable-threads", Path}),
            EXIT_SUCCESS);
}

TEST(InstantiateSubcommand, LinkerFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callInstantiate({"--dir", ".:.", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--env", "HOME=/tmp", Path}), EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--memory-page-limit", "256", Path}),
            EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--dir", ".:.", "--env", "HOME=/tmp",
                             "--memory-page-limit", "256", Path}),
            EXIT_SUCCESS);
}

TEST(InstantiateSubcommand, LinkedModules) {
  std::string ProvPath = providerPath();
  std::string ConsPath = consumerPath();
  std::string ModArg = "provider:" + ProvPath;

  EXPECT_EQ(callInstantiate({"--module", ModArg.c_str(), ConsPath.c_str()}),
            EXIT_SUCCESS);
  EXPECT_EQ(callInstantiate({"--module", "badformat", ConsPath.c_str()}),
            EXIT_FAILURE);
  EXPECT_EQ(
      callInstantiate({"--module", "provider:nonexist.wasm", ConsPath.c_str()}),
      EXIT_FAILURE);
  EXPECT_NE(callInstantiate({ConsPath.c_str()}), EXIT_SUCCESS);
}

TEST(InstantiateSubcommand, ForbiddenPluginFlag) {
  EXPECT_EQ(callInstantiate(
                {"--forbidden-plugin", "someplugin", simplePath().c_str()}),
            EXIT_SUCCESS);
}

 * ---------------------------------------------------------------------------
 * RunSubcommand tests
 * TODO: Commented out due to plugin re-entrancy issue.
 * UniTool with ToolType::Tool calls addLinkerOptions -> loadFromDefaultPaths
 * -> addPluginOptions, which leaves dangling pointers on repeated calls.
 * Uncomment when the plugin system is made re-entrant.
 * ---------------------------------------------------------------------------

TEST(RunSubcommand, ErrorHandling) {
  EXPECT_NE(callRun({}), EXIT_SUCCESS);
  EXPECT_NE(callRun({nonExistPath().c_str()}), EXIT_SUCCESS);

  std::string TruncPath =
      writeWasmToFile(TruncatedWasm.data(), TruncatedWasm.size(), "trunc.wasm");
  EXPECT_NE(callRun({TruncPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(TruncPath.c_str());

  std::string ElfPath =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf.wasm");
  EXPECT_NE(callRun({ElfPath.c_str()}), EXIT_SUCCESS);
  std::filesystem::remove(ElfPath.c_str());

  EXPECT_NE(callRun({"--no-such-flag", simplePath().c_str()}), EXIT_SUCCESS);
}

TEST(RunSubcommand, ReactorMode) {
  EXPECT_EQ(callRun({"--reactor", simplePath().c_str(), "add", "3", "5"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", simplePath().c_str(), "sub", "10", "4"}),
            EXIT_SUCCESS);
  EXPECT_NE(callRun({"--reactor", simplePath().c_str(), "no_such_func"}),
            EXIT_SUCCESS);
  EXPECT_NE(callRun({"--reactor", simplePath().c_str()}), EXIT_SUCCESS);
}

TEST(RunSubcommand, ProposalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callRun({"--reactor", "--disable-simd", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--enable-all", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--wasm-2", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
}

TEST(RunSubcommand, RunSpecificFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(callRun({"--reactor", "--enable-instruction-count", Path, "add",
                     "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--enable-gas-measuring", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--enable-time-measuring", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--enable-all-statistics", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--force-interpreter", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--enable-jit", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--enable-coredump", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--coredump-for-wasmgdb", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--allow-af-unix", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--time-limit", "5000", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(
      callRun({"--reactor", "--gas-limit", "100000", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--dir", ".:.", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--env", "HOME=/tmp", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--memory-page-limit", "256", Path, "add",
                     "1", "2"}),
            EXIT_SUCCESS);
}

TEST(RunSubcommand, LinkedModules) {
  std::string ProvPath = providerPath();
  std::string ConsPath = consumerPath();
  std::string ModArg = "provider:" + ProvPath;

  EXPECT_EQ(callRun({"--reactor", "--module", ModArg.c_str(), ConsPath.c_str(),
                     "add", "3", "5"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--module", "badformat", ConsPath.c_str(),
                     "add", "1", "2"}),
            EXIT_FAILURE);
  EXPECT_EQ(callRun({"--reactor", "--module", "provider:nonexist.wasm",
                     ConsPath.c_str(), "add", "1", "2"}),
            EXIT_FAILURE);
}

TEST(RunSubcommand, GlobalFlags) {
  std::string PathStr = simplePath();
  const char *Path = PathStr.c_str();

  EXPECT_EQ(
      callRun({"--reactor", "--log-level", "error", Path, "add", "1", "2"}),
      EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--forbidden-plugin", "someplugin", Path,
                     "add", "1", "2"}),
            EXIT_SUCCESS);
  EXPECT_EQ(callRun({"--reactor", "--forbidden-plugin", "pA",
                     "--forbidden-plugin", "pB", Path, "add", "1", "2"}),
            EXIT_SUCCESS);
}

TEST(NoSubcommand, FallbackToRun) {
  EXPECT_EQ(
      callUniToolAll({"--reactor", simplePath().c_str(), "add", "3", "5"}),
      EXIT_SUCCESS);
  EXPECT_NE(callUniToolAll({nonExistPath().c_str()}), EXIT_SUCCESS);
  EXPECT_EQ(callUniToolAll({"--reactor", "--force-interpreter",
                            simplePath().c_str(), "add", "1", "2"}),
            EXIT_SUCCESS);
}
*/

} // namespace

GTEST_API_ int main(int Argc, char *Argv[]) {
  testing::InitGoogleTest(&Argc, Argv);
  TestDataPath = "driverTestData";
  if (Argc > 1) {
    TestDataPath = Argv[1];
  }
  return RUN_ALL_TESTS();
}
