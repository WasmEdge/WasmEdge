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
#include "common/spdlog.h"
#include "common/types.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace {

using namespace WasmEdge;

// (module
//   (func (export "f") (result i32) i32.const 7))
const std::array<WasmEdge::Byte, 34> ConstFuncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
    0x00, 0x00, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0b};

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
  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  EXPECT_TRUE(TestVM.holdsModule());
  auto Res = TestVM.execute("f");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 7U);

  auto FnList = TestVM.getFunctionList();
  ASSERT_EQ(FnList.size(), 1U);
  EXPECT_EQ(FnList[0].first, "f");
}

TEST(VMTest, StageRegisterWhileInstantiatedAllowsReinstantiate) {
  Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  ASSERT_TRUE(TestVM.registerModule("ext", ConstFuncWasm));
  ASSERT_TRUE(TestVM.instantiate());
  auto Res = TestVM.execute("f");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 7U);
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

TEST(VMTest, PluginWasiImportModulePresence) {
  Configure ConfWithWasi;
  ConfWithWasi.addHostRegistration(HostRegistration::Wasi);
  VM::VM WasiVM(ConfWithWasi);
  EXPECT_NE(WasiVM.getImportModule(HostRegistration::Wasi), nullptr);

  Configure ConfPlain;
  VM::VM PlainVM(ConfPlain);
  EXPECT_EQ(PlainVM.getImportModule(HostRegistration::Wasi), nullptr);
}

TEST(VMTest, CleanupThenReuse) {
  Configure Conf;
  VM::VM TestVM(Conf);
  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  auto Res = TestVM.execute("f");
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 7U);

  TestVM.cleanup();
  auto AfterCleanup = TestVM.execute("f");
  ASSERT_FALSE(AfterCleanup);
  EXPECT_EQ(AfterCleanup.error(), ErrCode::Value::WrongInstanceAddress);

  ASSERT_TRUE(TestVM.loadWasm(ConstFuncWasm));
  ASSERT_TRUE(TestVM.validate());
  ASSERT_TRUE(TestVM.instantiate());
  auto AfterReuse = TestVM.execute("f");
  ASSERT_TRUE(AfterReuse);
  ASSERT_EQ(AfterReuse->size(), 1U);
  EXPECT_EQ((*AfterReuse)[0].first.get<uint32_t>(), 7U);
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
