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

struct ToolResult {
  int ExitCode;
  std::string Stdout;
};

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

static const std::array<uint8_t, 25> MemNoMaxWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x02,
    0x0f, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x06, 0x6d, 0x65,
    0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x01};

static const std::array<uint8_t, 12> InvalidWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

static const std::array<uint8_t, 8> NotWasmBytes{0x7f, 0x45, 0x4c, 0x46,
                                                 0x00, 0x00, 0x00, 0x00};

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

TEST(DriverParse, MissingFileArgument) {
  EXPECT_NE(callUniToolInProcess({"parse"}), EXIT_SUCCESS);
}

TEST(DriverParse, EmptyPathArgument) {
  EXPECT_NE(callUniToolInProcess({"parse", ""}), EXIT_SUCCESS);
}

TEST(DriverParse, NonExistentFile) {
  std::string Path = TestDataPath + "/nonexist.wasm";
  EXPECT_NE(callUniToolInProcess({"parse", Path.c_str()}), EXIT_SUCCESS);
}

TEST(DriverParse, DirectoryAsInput) {
  EXPECT_NE(callUniToolInProcess({"parse", TestDataPath.c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, InvalidWasmContent) {
  std::string Path =
      writeWasmToFile(InvalidWasm.data(), InvalidWasm.size(), "invalid_p.wasm");
  EXPECT_NE(callUniToolInProcess({"parse", Path.c_str()}), EXIT_SUCCESS);
  std::remove(Path.c_str());
}

TEST(DriverParse, NotWasmAtAll) {
  std::string Path =
      writeWasmToFile(NotWasmBytes.data(), NotWasmBytes.size(), "elf_p.wasm");
  EXPECT_NE(callUniToolInProcess({"parse", Path.c_str()}), EXIT_SUCCESS);
  std::remove(Path.c_str());
}

TEST(DriverParse, FullFeaturedModule) {
  EXPECT_EQ(callUniToolInProcess({"parse", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, MinimalModule) {
  std::string Path =
      writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(), "minimal_p.wasm");
  EXPECT_EQ(callUniToolInProcess({"parse", Path.c_str()}), EXIT_SUCCESS);
  std::remove(Path.c_str());
}

TEST(DriverParse, DisableSIMD) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-simd", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableBulkMemory) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-bulk-memory", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableReferenceTypes) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-reference-types",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableMultiValue) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-multi-value", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableMutableGlobals) {
  std::string Path = writeWasmToFile(MinimalWasm.data(), MinimalWasm.size(),
                                     "minimal_mutglob_p.wasm");
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-import-export-mut-globals", Path.c_str()}),
            EXIT_SUCCESS);
  std::remove(Path.c_str());
}

TEST(DriverParse, DisableMutableGlobalsRejectsMutImport) {
  EXPECT_NE(
      callUniToolInProcess({"parse", "--disable-import-export-mut-globals",
                            parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, DisableNonTrapF2I) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-non-trap-float-to-int",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableSignExtension) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-sign-extension-operators",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableTailCall) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-tail-call", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableExtendedConst) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-extended-const", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableMultiMemory) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-multi-memory", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableRelaxedSIMD) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-relaxed-simd", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableMemory64) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--disable-memory64", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableGC) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-gc", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, DisableFunctionReference) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-function-reference",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, DisableExceptionHandling) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-exception-handling",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, Wasm1Standard) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--wasm-1", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, Wasm2Standard) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--wasm-2", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, Wasm3Standard) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--wasm-3", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, EnableAll) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--enable-all", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, EnableThreads) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--enable-threads", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableComponent) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--enable-component", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableTailCallDeprecated) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--enable-tail-call", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableExtendedConstDeprecated) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--enable-extended-const", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableFunctionReferenceDeprecated) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-function-reference",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableGCDeprecated) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--enable-gc", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, EnableMultiMemoryDeprecated) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--enable-multi-memory", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableRelaxedSIMDDeprecated) {
  EXPECT_EQ(callUniToolInProcess(
                {"parse", "--enable-relaxed-simd", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, EnableExceptionHandlingDeprecated) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--enable-exception-handling",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, ForbiddenPlugin) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--forbidden-plugin", "someplugin",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, ForbiddenPluginMultiple) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--forbidden-plugin", "plugA",
                                  "--forbidden-plugin", "plugB",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, UnknownFlag) {
  EXPECT_NE(callUniToolInProcess(
                {"parse", "--no-such-flag", parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, ExtraPositionalArgument) {
  EXPECT_NE(
      callUniToolInProcess({"parse", parseTestPath().c_str(), "extra-arg"}),
      EXIT_SUCCESS);
}

TEST(DriverParse, MultipleDisableFlags) {
  EXPECT_EQ(
      callUniToolInProcess({"parse", "--disable-simd", "--disable-bulk-memory",
                            "--disable-multi-value", parseTestPath().c_str()}),
      EXIT_SUCCESS);
}

TEST(DriverParse, DisableAndEnableMixed) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--disable-simd", "--enable-threads",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, StandardPlusDisable) {
  EXPECT_EQ(callUniToolInProcess({"parse", "--wasm-2", "--disable-simd",
                                  parseTestPath().c_str()}),
            EXIT_SUCCESS);
}

TEST(DriverParse, ImportMemoryNoMax) {
  std::string Path = writeWasmToFile(MemNoMaxWasm.data(), MemNoMaxWasm.size(),
                                     "memnomax_p.wasm");
  EXPECT_EQ(callUniToolInProcess({"parse", Path.c_str()}), EXIT_SUCCESS);
  std::remove(Path.c_str());
}

TEST(DriverParse, OutputSectionHeaders) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout,
      {"file format wasm 0x1", "Section Details:", "Type[4]:", "Import[5]:",
       "Function[4]:", "Global[6]:", "Export[12]:", "Code[4]:", "Custom:"}));
}

TEST(DriverParse, OutputNameAnnotations) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout, {"<imported_func>", "<nop_func>", "<add_func>", "<swap_func>",
                 "<call_import>", "<imported_g_mut>", "<imported_g_const>",
                 "<g_i32>", "<g_f64>", "<g_mut_local>", "- name: \"name\""}));
}

TEST(DriverParse, OutputImportsAndExports) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(
      R.Stdout, {"<- env.log", "<- env.memory", "<- env.table", "<- env.g_mut",
                 "<- env.g_const", "-> \"nop_func\"", "-> \"add_func\"",
                 "-> \"swap_func\"", "-> \"call_import\"", "-> \"memory\"",
                 "-> \"table\"", "-> \"g_i32\""}));
}

TEST(DriverParse, OutputGlobalInitExpressions) {
  auto R = callUniToolCaptureStdout({"parse", parseTestPath().c_str()});
  ASSERT_EQ(R.ExitCode, EXIT_SUCCESS);
  EXPECT_TRUE(containsAll(R.Stdout, {"init i32=42", "init i64=100", "init f32=",
                                     "init f64=", "init global.get 1"}));
}

TEST(DriverParse, MinimalModuleShowsEmptyCounts) {
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
