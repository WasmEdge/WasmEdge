// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/llvm/LLVMInterruptTest.cpp - Interrupt --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of interrupting the asynchronous AOT executions.
///
//===----------------------------------------------------------------------===//

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// (module
//   (type (;0;) (func))
//   (memory (;0;) 1)
//   (export "_start" (func 0))
//   (func (;0;) (type 0)
//     loop ;; label = @1
//       br 0 (;@1;)
//     end
//   )
// )
std::array<WasmEdge::Byte, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

// Cancelling an asynchronous AOT run must interrupt it, for a native library
// and for a universal wasm artifact.
TEST(LLVMInterruptTest, AsyncRunWasmFile) {
  for (const auto Format : {CompilerConfigure::OutputFormat::Native,
                            CompilerConfigure::OutputFormat::Wasm}) {
    SCOPED_TRACE(static_cast<unsigned>(Format));
    Configure Conf;
    Conf.getCompilerConfigure().setInterruptible(true);
    Conf.getCompilerConfigure().setOutputFormat(Format);
    Conf.getRuntimeConfigure().setRunMode(RunMode::AOT);

    VM::VM VM(Conf);
    Loader::Loader Loader(Conf);
    Validator::Validator ValidatorEngine(Conf);
    LLVM::Compiler Compiler(Conf);
    LLVM::CodeGen CodeGen(Conf);
    const auto Path = std::filesystem::temp_directory_path() /
                      u8path(Format == CompilerConfigure::OutputFormat::Native
                                 ? "AOTcoreTest" WASMEDGE_LIB_EXTENSION
                                 : "AOTcoreTest.aot.wasm");
    auto Module = *Loader.parseModule(AsyncWasm);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
    {
      auto Timeout =
          std::chrono::system_clock::now() + std::chrono::milliseconds(1);
      auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
      EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
      AsyncResult.cancel();
      auto Result = AsyncResult.get();
      EXPECT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::Interrupted);
    }
    {
      auto Timeout = std::chrono::milliseconds(1);
      auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
      EXPECT_FALSE(AsyncResult.waitFor(Timeout));
      AsyncResult.cancel();
      auto Result = AsyncResult.get();
      EXPECT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::Interrupted);
    }
    VM.cleanup();
    EXPECT_NO_THROW(std::filesystem::remove(Path));
  }
}

TEST(LLVMInterruptTest, AsyncExecute) {
  for (const auto Format : {CompilerConfigure::OutputFormat::Native,
                            CompilerConfigure::OutputFormat::Wasm}) {
    SCOPED_TRACE(static_cast<unsigned>(Format));
    Configure Conf;
    Conf.getCompilerConfigure().setInterruptible(true);
    Conf.getCompilerConfigure().setOutputFormat(Format);
    Conf.getRuntimeConfigure().setRunMode(RunMode::AOT);

    VM::VM VM(Conf);
    Loader::Loader Loader(Conf);
    Validator::Validator ValidatorEngine(Conf);
    LLVM::Compiler Compiler(Conf);
    LLVM::CodeGen CodeGen(Conf);
    const auto Path = std::filesystem::temp_directory_path() /
                      u8path(Format == CompilerConfigure::OutputFormat::Native
                                 ? "AOTcoreTest" WASMEDGE_LIB_EXTENSION
                                 : "AOTcoreTest.aot.wasm");
    auto Module = *Loader.parseModule(AsyncWasm);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
    ASSERT_TRUE(VM.loadWasm(Path));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    {
      auto Timeout =
          std::chrono::system_clock::now() + std::chrono::milliseconds(1);
      auto AsyncResult = VM.asyncExecute("_start");
      EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
      AsyncResult.cancel();
      auto Result = AsyncResult.get();
      EXPECT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::Interrupted);
    }
    {
      auto Timeout = std::chrono::milliseconds(1);
      auto AsyncResult = VM.asyncExecute("_start");
      EXPECT_FALSE(AsyncResult.waitFor(Timeout));
      AsyncResult.cancel();
      auto Result = AsyncResult.get();
      EXPECT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::Interrupted);
    }
    VM.cleanup();
    EXPECT_NO_THROW(std::filesystem::remove(Path));
  }
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
