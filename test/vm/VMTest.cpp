// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/vm/VMTest.cpp - VM characterization tests -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests of the VM stage machine, module registration, plugin mock fallback,
/// and reuse after cleanup.
///
//===----------------------------------------------------------------------===//

#include "vm/vm.h"

#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "host/filesystem.h"
#include "host/wasi/wasimodule.h"
#include "runtime/callingframe.h"
#ifdef WASMEDGE_USE_LLVM
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#endif

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>
#include <vector>

namespace {

using namespace WasmEdge;

// (module
//   (func (export "f") (result i32) i32.const 7))
const std::array<WasmEdge::Byte, 34> ConstFuncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
    0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0b};

// (component)
const std::array<WasmEdge::Byte, 8> EmptyComponentWasm{0x00, 0x61, 0x73, 0x6d,
                                                       0x0d, 0x00, 0x01, 0x00};

// Load, validate, and instantiate the constant function module.
void instantiateFixture(VM::VM &TestVM) {
  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
}

// Execute "f" and expect the constant result.
void expectConstResult(VM::VM &TestVM, uint32_t Expected) {
  auto Res = TestVM.execute("f");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), Expected);
}

TEST(VMTest, SnapshotRoundTrip) {
  // (module
  //   (type (;0;) (func (result i32)))
  //   (type (;1;) (func))
  //   (table (;0;) 1 4 funcref)
  //   (memory (;0;) 1 3)
  //   (global (;0;) (mut i32) i32.const 7)
  //   (export "memory" (memory 0))
  //   (export "table" (table 0))
  //   (export "global" (global 0))
  //   (export "f" (func 2))
  //   (export "g" (func 3))
  //   (elem (;0;) (i32.const 0) func 0)
  //   (elem (;1;) declare func 1)
  //   (func (;0;) (type 0) (result i32)
  //     global.get 0
  //     i32.const 0
  //     i32.load
  //     i32.add
  //   )
  //   (func (;1;) (type 0) (result i32)
  //     i32.const 99
  //   )
  //   (func (;2;) (type 0) (result i32)
  //     i32.const 0
  //     call_indirect (type 0)
  //   )
  //   (func (;3;) (type 1)
  //     i32.const 11
  //     global.set 0
  //     i32.const 0
  //     i32.const 23
  //     i32.store
  //     i32.const 2
  //     memory.grow
  //     drop
  //     i32.const 131072
  //     i32.const 42
  //     i32.store
  //     ref.null func
  //     i32.const 2
  //     table.grow 0
  //     drop
  //     i32.const 0
  //     ref.func 1
  //     table.set 0
  //   )
  // )
  std::array<WasmEdge::Byte, 166> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
      0x00, 0x01, 0x7f, 0x60, 0x00, 0x00, 0x03, 0x05, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x04, 0x05, 0x01, 0x70, 0x01, 0x01, 0x04, 0x05, 0x04, 0x01, 0x01,
      0x01, 0x03, 0x06, 0x06, 0x01, 0x7f, 0x01, 0x41, 0x07, 0x0b, 0x07, 0x23,
      0x05, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x05, 0x74,
      0x61, 0x62, 0x6c, 0x65, 0x01, 0x00, 0x06, 0x67, 0x6c, 0x6f, 0x62, 0x61,
      0x6c, 0x03, 0x00, 0x01, 0x66, 0x00, 0x02, 0x01, 0x67, 0x00, 0x03, 0x09,
      0x0b, 0x02, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x00, 0x03, 0x00, 0x01, 0x01,
      0x0a, 0x44, 0x04, 0x0a, 0x00, 0x23, 0x00, 0x41, 0x00, 0x28, 0x02, 0x00,
      0x6a, 0x0b, 0x05, 0x00, 0x41, 0xe3, 0x00, 0x0b, 0x07, 0x00, 0x41, 0x00,
      0x11, 0x00, 0x00, 0x0b, 0x29, 0x00, 0x41, 0x0b, 0x24, 0x00, 0x41, 0x00,
      0x41, 0x17, 0x36, 0x02, 0x00, 0x41, 0x02, 0x40, 0x00, 0x1a, 0x41, 0x80,
      0x80, 0x08, 0x41, 0x2a, 0x36, 0x02, 0x00, 0xd0, 0x70, 0x41, 0x02, 0xfc,
      0x0f, 0x00, 0x1a, 0x41, 0x00, 0xd2, 0x01, 0x26, 0x00, 0x0b};
  const auto Dir =
      std::filesystem::temp_directory_path() / "wasmedge-vm-restore";
  std::filesystem::create_directories(Dir);
  struct Cleanup {
    std::filesystem::path Path;
    ~Cleanup() {
      std::error_code EC;
      std::filesystem::remove_all(Path, EC);
    }
  } Guard{Dir};
  for (unsigned Mode = 0; Mode <
#ifdef WASMEDGE_USE_LLVM
                          4
#else
                          1
#endif
       ;
       ++Mode) {
    SCOPED_TRACE(Mode);
    Configure Conf;
    Conf.addHostRegistration(HostRegistration::Wasi);
    if (Mode == 1) {
      Conf.getRuntimeConfigure().setRunMode(RunMode::AOT);
    }
    if (Mode == 2) {
      Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
    }
    if (Mode == 3) {
      Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
    }
    VM::VM TestVM(Conf);
    std::vector<Byte> Code(Wasm.begin(), Wasm.end());
#ifdef WASMEDGE_USE_LLVM
    if (Mode == 1) {
      auto Module = TestVM.getLoader().parseModule(Code);
      ASSERT_TRUE(Module);
      ASSERT_TRUE(TestVM.getValidator().validate(**Module));
      LLVM::Compiler Compiler(Conf);
      auto CompiledData = Compiler.compile(**Module);
      ASSERT_TRUE(CompiledData);
      LLVM::CodeGen CodeGen(Conf);
      ASSERT_TRUE(
          CodeGen.codegen(Code, std::move(*CompiledData), Dir / "state.wasm"));
      const auto Compiled = Loader::Loader::loadFile(Dir / "state.wasm");
      ASSERT_TRUE(Compiled);
      Code = *Compiled;
    }
#endif
    ASSERT_TRUE(TestVM.registerModule("registered", Code));
    const auto NoActive = Dir / "no-active.core";
    ASSERT_TRUE(TestVM.snapshot(NoActive));
    ASSERT_TRUE(TestVM.restore(NoActive));
    ASSERT_TRUE(TestVM.loadWasm(Code));
    ASSERT_TRUE(TestVM.validate());
    ASSERT_TRUE(TestVM.instantiate());
    if (Mode == 1) {
      ASSERT_TRUE(
          TestVM.getActiveModule()->findFuncExports("f")->isCompiledFunction());
    }
    auto Result = TestVM.execute("f");
    ASSERT_TRUE(Result);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 7U);
    const auto Path = Dir / "state.core";
    ASSERT_TRUE(TestVM.snapshot(Path));
    const auto Saved = Loader::Loader::loadFile(Path);
    ASSERT_TRUE(Saved);
    for (unsigned Iteration = 0; Iteration < 2; ++Iteration) {
      ASSERT_TRUE(TestVM.execute("g"));
      ASSERT_TRUE(TestVM.execute("registered", "g"));
      ASSERT_TRUE(TestVM.restore(Path));
      for (const bool Registered : {false, true}) {
        Result = Registered ? TestVM.execute("registered", "f")
                            : TestVM.execute("f");
        ASSERT_TRUE(Result);
        EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 7U);
      }
      const auto Again = Dir / "again.core";
      ASSERT_TRUE(TestVM.snapshot(Again));
      const auto Restored = Loader::Loader::loadFile(Again);
      ASSERT_TRUE(Restored);
      EXPECT_EQ(*Saved, *Restored);
    }
  }
}

TEST(VMTest, SnapshotRejectsComponent) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.forceValidateForComponent());
  ASSERT_TRUE(TestVM.instantiate());
  const auto Snapshot = TestVM.snapshot();
  ASSERT_FALSE(Snapshot);
  EXPECT_EQ(Snapshot.error(), ErrCode::Value::CoredumpUnsupported);
  const auto Restore = TestVM.restore("unused.core");
  ASSERT_FALSE(Restore);
  EXPECT_EQ(Restore.error(), ErrCode::Value::CoredumpUnsupported);
}

TEST(VMTest, StageValidateBeforeLoadFails) {
  Configure Conf;
  VM::VM TestVM(Conf);
  auto Res = TestVM.validate();
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongVMWorkflow);
}

TEST(VMTest, StageInstantiateBeforeValidateFails) {
  Configure Conf;
  VM::VM FreshVM(Conf);
  auto FreshRes = FreshVM.instantiate();
  ASSERT_FALSE(FreshRes);
  EXPECT_EQ(FreshRes.error(), ErrCode::Value::WrongVMWorkflow);

  VM::VM LoadedVM(Conf);
  ASSERT_TRUE(LoadedVM.loadWasm(ConstFuncWasm));
  auto LoadedRes = LoadedVM.instantiate();
  ASSERT_FALSE(LoadedRes);
  EXPECT_EQ(LoadedRes.error(), ErrCode::Value::WrongVMWorkflow);
}

TEST(VMTest, StageExecuteWithoutInstanceFails) {
  Configure Conf;
  VM::VM TestVM(Conf);
  auto Res = TestVM.execute("f");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongInstanceAddress);
}

TEST(VMTest, StageLoadValidateInstantiateExecute) {
  Configure Conf;
  VM::VM TestVM(Conf);
  EXPECT_FALSE(TestVM.holdsModule());
  instantiateFixture(TestVM);
  EXPECT_TRUE(TestVM.holdsModule());
  expectConstResult(TestVM, 7U);

  auto FnList = TestVM.getFunctionList();
  ASSERT_EQ(FnList.size(), 1U);
  EXPECT_EQ(FnList[0].first, "f");
}

TEST(VMTest, StageRegisterWhileInstantiatedAllowsReinstantiate) {
  Configure Conf;
  VM::VM TestVM(Conf);
  instantiateFixture(TestVM);
  ASSERT_TRUE(TestVM.registerModule("ext", ConstFuncWasm));
  ASSERT_TRUE(TestVM.instantiate());
  expectConstResult(TestVM, 7U);
}

TEST(VMTest, StageLoadComponentAfterModuleReplacesModule) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());
  EXPECT_FALSE(TestVM.holdsModule());
}

TEST(VMTest, StageLoadModuleAfterComponentReplacesComponent) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());

  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  EXPECT_FALSE(TestVM.holdsComponent());
  auto Res = TestVM.forceValidateForComponent();
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongVMWorkflow);
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  expectConstResult(TestVM, 7U);
}

TEST(VMTest, StageLoadASTModuleAfterComponentReplacesComponent) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  auto Module = TestVM.getLoader().parseModule(ConstFuncWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.loadWasm(**Module));
  auto Res = TestVM.forceValidateForComponent();
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongVMWorkflow);
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  expectConstResult(TestVM, 7U);
}

TEST(VMTest, StageRunWasmFileKeepsNewInstanceAfterCallFails) {
  // Same as ConstFuncWasm, but the function returns 9.
  auto OtherConstFuncWasm = ConstFuncWasm;
  OtherConstFuncWasm[32] = 0x09;
  Configure Conf;
  VM::VM FreshVM(Conf);
  ASSERT_TRUE(FreshVM.runWasmFile(OtherConstFuncWasm, "f"));
  EXPECT_TRUE(FreshVM.instantiate());
  expectConstResult(FreshVM, 9U);

  VM::VM TestVM(Conf);
  instantiateFixture(TestVM);
  auto Res = TestVM.runWasmFile(OtherConstFuncWasm, "f");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 9U);
  expectConstResult(TestVM, 9U);
  EXPECT_TRUE(TestVM.validate());
  EXPECT_TRUE(TestVM.instantiate());
  expectConstResult(TestVM, 9U);

  VM::VM FailedVM(Conf);
  instantiateFixture(FailedVM);
  auto FailedRes = FailedVM.runWasmFile(OtherConstFuncWasm, "no-such-func");
  ASSERT_FALSE(FailedRes);
  EXPECT_EQ(FailedRes.error(), ErrCode::Value::FuncNotFound);
  expectConstResult(FailedVM, 9U);
  EXPECT_TRUE(FailedVM.instantiate());
  expectConstResult(FailedVM, 9U);
}

TEST(VMTest, StageRunWasmFileFromASTModuleKeepsStage) {
  Configure Conf;
  VM::VM TestVM(Conf);
  auto Module = TestVM.getLoader().parseModule(ConstFuncWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(TestVM.runWasmFile(**Module, "f"));
  expectConstResult(TestVM, 7U);
  auto Res = TestVM.validate();
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongVMWorkflow);

  instantiateFixture(TestVM);
  ASSERT_TRUE(TestVM.runWasmFile(**Module, "f"));
  EXPECT_TRUE(TestVM.instantiate());
  expectConstResult(TestVM, 7U);
}

TEST(VMTest, StageReloadComponentReleasesComponentInstance) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());

  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  EXPECT_FALSE(TestVM.holdsComponent());
  auto Res = TestVM.executeComponent("f");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongInstanceAddress);
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());
}

TEST(VMTest, StageFailedRunWasmFileFallsBackToStoredUnitStage) {
  // (module
  //   (func (export "f") (result i32)))
  std::array<WasmEdge::Byte, 32> InvalidWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
      0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01,
      0x01, 0x66, 0x00, 0x00, 0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b};
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  instantiateFixture(TestVM);
  ASSERT_FALSE(TestVM.runWasmFile(InvalidWasm, "f"));
  expectConstResult(TestVM, 7U);
  auto Res = TestVM.instantiate();
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongVMWorkflow);
  EXPECT_FALSE(TestVM.validate());

  VM::VM CompVM(Conf);
  auto RunRes = CompVM.runWasmFile(EmptyComponentWasm, "f");
  ASSERT_FALSE(RunRes);
  EXPECT_EQ(RunRes.error(), ErrCode::Value::RuntimeError);
  EXPECT_TRUE(CompVM.instantiate());
  EXPECT_TRUE(CompVM.holdsComponent());
}

TEST(VMTest, StageRunWasmFileFromPathStoresComponent) {
  std::filesystem::path Dir;
  {
    std::random_device Device;
    for (int I = 0; I < 100 && Dir.empty(); ++I) {
      auto Candidate = std::filesystem::temp_directory_path() /
                       ("wasmedge-vm-stage-path-" + std::to_string(Device()));
      std::error_code EC;
      if (std::filesystem::create_directory(Candidate, EC)) {
        Dir = std::move(Candidate);
      } else {
        ASSERT_FALSE(EC) << EC.message();
      }
    }
  }
  ASSERT_FALSE(Dir.empty());
  struct Cleanup {
    std::filesystem::path Path;
    ~Cleanup() {
      std::error_code EC;
      std::filesystem::remove_all(Path, EC);
    }
  } Guard{Dir};
  const auto Path = Dir / "component.wasm";
  {
    std::ofstream File(Path, std::ios::binary);
    File.write(reinterpret_cast<const char *>(EmptyComponentWasm.data()),
               static_cast<std::streamsize>(EmptyComponentWasm.size()));
    File.close();
    ASSERT_TRUE(File);
  }
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  instantiateFixture(TestVM);
  auto Res = TestVM.runWasmFile(Path, "f");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::RuntimeError);
  expectConstResult(TestVM, 7U);
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());
  EXPECT_FALSE(TestVM.holdsModule());
}

TEST(VMTest, StageInstantiateComponentReleasesModuleInstance) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  instantiateFixture(TestVM);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.validate());
  expectConstResult(TestVM, 7U);
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());
  EXPECT_FALSE(TestVM.holdsModule());
  auto Res = TestVM.execute("f");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongInstanceAddress);
}

TEST(VMTest, StageRunWasmFileFromASTModuleReleasesComponentInstance) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  auto Module = TestVM.getLoader().parseModule(ConstFuncWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  ASSERT_TRUE(TestVM.runWasmFile(**Module, "f"));
  EXPECT_TRUE(TestVM.holdsModule());
  EXPECT_FALSE(TestVM.holdsComponent());

  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());
  EXPECT_FALSE(TestVM.holdsModule());
}

TEST(VMTest, RegisterAndExecuteByName) {
  Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.registerModule("ext", ConstFuncWasm));
  auto Res = TestVM.execute("ext", "f");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 7U);
}

TEST(VMTest, RegisterDuplicateNameFails) {
  Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.registerModule("dup", ConstFuncWasm));
  auto Res = TestVM.registerModule("dup", ConstFuncWasm);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::ModuleNameConflict);
}

TEST(VMTest, UnregisterRemovesStoreLookup) {
  Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.registerModule("ext", ConstFuncWasm));
  {
    auto Res = TestVM.execute("ext", "f");
    ASSERT_TRUE(Res);
  }
  ASSERT_TRUE(TestVM.unregisterModule("ext"));
  auto Res = TestVM.execute("ext", "f");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongInstanceAddress);
}

TEST(VMTest, UnregisterUnknownNameSucceeds) {
  Configure Conf;
  VM::VM TestVM(Conf);
  EXPECT_TRUE(TestVM.unregisterModule("no-such-module"));
}

TEST(VMTest, PluginMockModulesRegistered) {
  Configure Conf;
  VM::VM TestVM(Conf);
  EXPECT_NE(TestVM.getStoreManager().findModule("wasi_ephemeral_nn"), nullptr);
}

TEST(VMTest, WasiImportModulePresence) {
  Configure ConfWithWasi;
  ConfWithWasi.addHostRegistration(HostRegistration::Wasi);
  VM::VM WasiVM(ConfWithWasi);
  EXPECT_NE(WasiVM.getImportModule(HostRegistration::Wasi), nullptr);

  Configure ConfPlain;
  VM::VM PlainVM(ConfPlain);
  EXPECT_EQ(PlainVM.getImportModule(HostRegistration::Wasi), nullptr);
}

TEST(VMTest, WasiImportGrantsFileSystem) {
  // (module
  //   (import "wasi_snapshot_preview1" "proc_exit" (func (param i32)))
  //   (func (export "f") (result i32) i32.const 7))
  std::array<WasmEdge::Byte, 76> WasiImportWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02,
      0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x02, 0x24, 0x01,
      0x16, 0x77, 0x61, 0x73, 0x69, 0x5f, 0x73, 0x6e, 0x61, 0x70, 0x73,
      0x68, 0x6f, 0x74, 0x5f, 0x70, 0x72, 0x65, 0x76, 0x69, 0x65, 0x77,
      0x31, 0x09, 0x70, 0x72, 0x6f, 0x63, 0x5f, 0x65, 0x78, 0x69, 0x74,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x01, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x01, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0b};

  // The host functions called by a module importing WASI open its files with
  // the filesystem of the WASI module; a module without WASI has none.
  Configure Conf;
  Conf.addHostRegistration(HostRegistration::Wasi);
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(WasiImportWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  const auto *WasiMod = dynamic_cast<const Host::WasiModule *>(
      TestVM.getImportModule(HostRegistration::Wasi));
  ASSERT_NE(WasiMod, nullptr);
  ASSERT_NE(TestVM.getActiveModule(), nullptr);
  const Host::FileSystem *WasiFS = WasiMod->getEnv();
  EXPECT_EQ(
      Runtime::CallingFrame(nullptr, TestVM.getActiveModule()).getFileSystem(),
      WasiFS);

  instantiateFixture(TestVM);
  ASSERT_NE(TestVM.getActiveModule(), nullptr);
  EXPECT_EQ(
      Runtime::CallingFrame(nullptr, TestVM.getActiveModule()).getFileSystem(),
      nullptr);
}

TEST(VMTest, CleanupThenReuse) {
  Configure Conf;
  VM::VM TestVM(Conf);
  instantiateFixture(TestVM);
  expectConstResult(TestVM, 7U);

  TestVM.cleanup();
  auto Res = TestVM.execute("f");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongInstanceAddress);

  instantiateFixture(TestVM);
  expectConstResult(TestVM, 7U);
}

TEST(VMTest, CleanupReleasesComponentInstance) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(EmptyComponentWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsComponent());

  TestVM.cleanup();
  EXPECT_FALSE(TestVM.holdsComponent());
  auto Res = TestVM.validate();
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::WrongVMWorkflow);

  instantiateFixture(TestVM);
  expectConstResult(TestVM, 7U);
}

TEST(VMTest, CleanupRestoresBuiltinHosts) {
  Configure Conf;
  Conf.addHostRegistration(HostRegistration::Wasi);
  VM::VM TestVM(Conf);
  ASSERT_NE(TestVM.getImportModule(HostRegistration::Wasi), nullptr);
  TestVM.cleanup();
  EXPECT_NE(TestVM.getImportModule(HostRegistration::Wasi), nullptr);
  EXPECT_NE(TestVM.getStoreManager().findModule("wasi_ephemeral_nn"), nullptr);
}

TEST(VMTest, MultipleVM) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM1(Conf);
  WasmEdge::VM::VM VM2(Conf);
  // (module
  //   (func (export "_start")))
  std::array<WasmEdge::Byte, 36> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0a, 0x01, 0x06, 0x5f, 0x73,
      0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b};
  ASSERT_TRUE(VM1.loadWasm(Wasm));
  ASSERT_TRUE(VM1.validate());
  ASSERT_TRUE(VM1.instantiate());
  ASSERT_TRUE(VM2.loadWasm(Wasm));
  ASSERT_TRUE(VM2.validate());
  ASSERT_TRUE(VM2.instantiate());
  auto Result1 = VM1.execute("_start");
  auto Result2 = VM2.execute("_start");
  EXPECT_TRUE(Result1);
  EXPECT_TRUE(Result2);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
