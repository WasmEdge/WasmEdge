// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/aot/AOTcoreTest.cpp - Wasm test suites --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "common/defines.h"
#include "common/spdlog.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

// Parameterized testing class.
class NativeCoreTest : public testing::TestWithParam<std::string> {};
class CustomWasmCoreTest : public testing::TestWithParam<std::string> {};
class JITCoreTest : public testing::TestWithParam<std::string> {};

TEST_P(NativeCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  auto Compile = [&, Conf = std::cref(Conf)](
                     const std::string &Filename) -> Expect<std::string> {
    WasmEdge::Configure CopyConf = Conf.get();
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    CopyConf.getCompilerConfigure().setOutputFormat(
        CompilerConfigure::OutputFormat::Native);
    CopyConf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O0);
    CopyConf.getCompilerConfigure().setDumpIR(true);
    WasmEdge::LLVM::Compiler Compiler(CopyConf);
    WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
    auto Path = std::filesystem::u8path(Filename);
    Path.replace_extension(std::filesystem::u8path(WASMEDGE_LIB_EXTENSION));
    const auto SOPath = Path.u8string();
    std::vector<WasmEdge::Byte> Data;
    std::unique_ptr<WasmEdge::AST::Module> Module;
    return Loader.loadFile(Filename)
        .and_then([&](auto Result) noexcept {
          Data = std::move(Result);
          return Loader.parseModule(Data);
        })
        .and_then([&](auto Result) noexcept {
          Module = std::move(Result);
          return ValidatorEngine.validate(*Module);
        })
        .and_then([&]() noexcept { return Compiler.compile(*Module); })
        .and_then([&](auto Result) noexcept {
          return CodeGen.codegen(Data, std::move(Result), SOPath);
        })
        .and_then([&]() noexcept { return Expect<std::string>{SOPath}; });
  };
  T.onModule = [&VM, &Compile](const std::string &ModName,
                               const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&VM, &ModName](const std::string &SOFilename) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFilename);
          } else {
            return VM.loadWasm(SOFilename)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename);
  };
  T.onValidate = [&VM, &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename)
        .and_then([&](const std::string &SOFilename) -> Expect<void> {
          return VM.loadWasm(SOFilename);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onInstantiate = [&VM,
                     &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename)
        .and_then([&](const std::string &SOFilename) -> Expect<void> {
          return VM.loadWasm(SOFilename);
        })
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet = [&VM](const std::string &ModName, const std::string &Field)
      -> Expect<std::pair<ValVariant, ValType>> {
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

TEST_P(CustomWasmCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  auto Compile = [&, Conf = std::cref(Conf)](
                     const std::string &Filename) -> Expect<std::string> {
    WasmEdge::Configure CopyConf = Conf.get();
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    CopyConf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O0);
    CopyConf.getCompilerConfigure().setDumpIR(true);
    WasmEdge::LLVM::Compiler Compiler(CopyConf);
    WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
    auto Path = std::filesystem::u8path(Filename);
    Path.replace_extension(std::filesystem::u8path(".aot.wasm"));
    const auto SOPath = Path.u8string();
    std::vector<WasmEdge::Byte> Data;
    std::unique_ptr<WasmEdge::AST::Module> Module;
    return Loader.loadFile(Filename)
        .and_then([&](auto Result) noexcept {
          Data = std::move(Result);
          return Loader.parseModule(Data);
        })
        .and_then([&](auto Result) noexcept {
          Module = std::move(Result);
          return ValidatorEngine.validate(*Module);
        })
        .and_then([&]() noexcept { return Compiler.compile(*Module); })
        .and_then([&](auto Result) noexcept {
          return CodeGen.codegen(Data, std::move(Result), SOPath);
        })
        .and_then([&]() noexcept { return Expect<std::string>{SOPath}; });
  };
  T.onModule = [&VM, &Compile](const std::string &ModName,
                               const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&VM, &ModName](const std::string &SOFilename) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFilename);
          } else {
            return VM.loadWasm(SOFilename)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename);
  };
  T.onValidate = [&VM, &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename)
        .and_then([&](const std::string &SOFilename) -> Expect<void> {
          return VM.loadWasm(SOFilename);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onInstantiate = [&VM,
                     &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename)
        .and_then([&](const std::string &SOFilename) -> Expect<void> {
          return VM.loadWasm(SOFilename);
        })
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet = [&VM](const std::string &ModName, const std::string &Field)
      -> Expect<std::pair<ValVariant, ValType>> {
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

TEST_P(JITCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge::Configure CopyConf = Conf;
  CopyConf.getRuntimeConfigure().setEnableJIT(true);
  CopyConf.getCompilerConfigure().setOptimizationLevel(
      WasmEdge::CompilerConfigure::OptimizationLevel::O0);
  CopyConf.getCompilerConfigure().setDumpIR(true);
  WasmEdge::VM::VM VM(CopyConf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  T.onModule = [&VM](const std::string &ModName,
                     const std::string &Filename) -> Expect<void> {
    if (!ModName.empty()) {
      return VM.registerModule(ModName, Filename);
    } else {
      return VM.loadWasm(Filename)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename);
  };
  T.onValidate = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename).and_then([&VM]() { return VM.validate(); });
  };
  T.onInstantiate = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet = [&VM](const std::string &ModName, const std::string &Field)
      -> Expect<std::pair<ValVariant, ValType>> {
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(
    TestUnit, NativeCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CustomWasmCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, JITCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::JIT)));

std::array<WasmEdge::Byte, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

TEST(AsyncRunWsmFile, NativeInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
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
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncExecute, NativeInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
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
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncRunWsmFile, CustomWasmInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Wasm);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest.aot.wasm");
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
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncExecute, CustomWasmInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Wasm);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest.aot.wasm");
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
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(Configure, ConfigureTest) {
  {
    WasmEdge::Configure Conf;
    WasmEdge::LLVM::Compiler Compiler(Conf);
    auto Result = Compiler.checkConfigure();
    EXPECT_TRUE(Result);
  }
  {
    WasmEdge::Configure Conf;
    Conf.addProposal(Proposal::ExceptionHandling);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    auto Result = Compiler.checkConfigure();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::InvalidConfigure);
  }
}

class Collect : public WasmEdge::Runtime::HostFunction<Collect> {
public:
  Expect<void> body(const WasmEdge::Runtime::CallingFrame &CF) {
    auto &Allocator = CF.getExecutor()->getAllocator();
    Allocator.manualCollect(Allocator.getStack());
    return {};
  }
};

class Record : public WasmEdge::Runtime::HostFunction<Record> {
public:
  Expect<void> body(const WasmEdge::Runtime::CallingFrame &CF) {
    MemoryUsageLog.push_back(CF.getExecutor()->getAllocator().getMemoryUsage());
    return {};
  }
  Span<const uint64_t> getLog() const noexcept { return MemoryUsageLog; }

private:
  std::vector<uint64_t> MemoryUsageLog;
};

class GCModule : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  GCModule() : ModuleInstance("gc") {
    addHostFunc("coll", std::make_unique<Collect>());
    auto RP = std::make_unique<Record>();
    R = RP.get();
    addHostFunc("rec", std::move(RP));
  }
  Span<const uint64_t> getLog() const noexcept { return R->getLog(); }

private:
  Record *R = nullptr;
};

TEST(GC, gc) {
  std::array<WasmEdge::Byte, 117> Wasm{
      0x00, 0x61, 0x73, 0x6d,       // wasm magic
      0x01, 0x00, 0x00, 0x00,       // module version
      0x01,                         // type section
      0x07,                         // section size
      0x02,                         // type count
      0x5e,                         // array type
      0x7f, 0x01,                   // i32 mutable
      0x60,                         // function type
      0x00, 0x00,                   // 0 arguments 0 results
      0x02,                         // import section
      0x14,                         // section size
      0x02,                         // import count
      0x02, 0x67, 0x63,             // "gc"
      0x04, 0x63, 0x6f, 0x6c, 0x6c, // "coll"
      0x00,                         // import kind: function
      0x01,                         // import type index: 1
      0x02, 0x67, 0x63,             // "gc"
      0x03, 0x72, 0x65, 0x63,       // "rec"
      0x00,                         // import kind: function
      0x01,                         // import type index: 0
      0x03,                         // function section
      0x02,                         // section size
      0x01,                         // function count
      0x01,                         // function type index: 1
      0x07,                         // export section
      0x06,                         // section size
      0x01,                         // export count
      0x02, 0x67, 0x63,             // export name "gc"
      0x00,                         // export kind: function
      0x02,                         // export index
      0x0a,                         // code section
      0x40,                         // section size
      0x01,                         // function count
      0x3e,                         // function size
      0x00,                         // local size
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x41, 0x04,                   // i32.const 4
      0xfb, 0x07, 0x00,             // array.new_default 0
      0x10, 0x01,                   // call 1 (gc.rec) 96
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 96
      0x1a,                         // drop
      0x10, 0x01,                   // call 1 (gc.rec) 96
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x41, 0x08,                   // i32.const 8
      0xfb, 0x07, 0x00,             // array.new_default 0
      0x1a,                         // drop
      0x10, 0x01,                   // call 1 (gc.rec) 160
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 160
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x41, 0x0c,                   // i32.const 12
      0xfb, 0x07, 0x00,             // array.new_default 0
      0x10, 0x01,                   // call 1 (gc.rec) 224
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 224
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 224
      0x1a,                         // drop
      0x10, 0x00,                   // call 0 (gc.coll)
      0x10, 0x01,                   // call 1 (gc.rec) 0
      0x0b,                         // end
  };

  const auto Path =
      std::filesystem::temp_directory_path() /
      std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::GC);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  {
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    auto Module = *Loader.parseModule(Wasm);
    ASSERT_TRUE(ValidatorEngine.validate(*Module));
    auto Data = Compiler.compile(*Module);
    ASSERT_TRUE(Data);
    ASSERT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Path));
  }

  WasmEdge::VM::VM VM(Conf);
  GCModule GCMod;
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.execute("gc");
  auto Result = GCMod.getLog();

  EXPECT_EQ(Result.size(), 13);
  EXPECT_EQ(Result[0], 0);
  EXPECT_EQ(Result[1], 0);
  EXPECT_EQ(Result[2], 96);
  EXPECT_EQ(Result[3], 96);
  EXPECT_EQ(Result[4], 96);
  EXPECT_GE(Result[5], 0);
  EXPECT_GE(Result[6], 160);
  EXPECT_GE(Result[7], 160);
  EXPECT_GE(Result[8], 0);
  EXPECT_GE(Result[9], 224);
  EXPECT_GE(Result[10], 224);
  EXPECT_GE(Result[11], 0); // 0~224
  EXPECT_GE(Result[12], 0);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
