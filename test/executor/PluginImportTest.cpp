// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/executor/PluginImportTest.cpp - Plugin imports ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of the imports of uninstalled official plugins.
///
//===----------------------------------------------------------------------===//

#include "executor/executor.h"
#include "loader/loader.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"

#include <gtest/gtest.h>

#include <array>

namespace {

using namespace WasmEdge;

// The functions imported from an uninstalled official plugin are stood in for
// at instantiation, and trap when called.
TEST(PluginImportTest, UninstalledPluginFunctionsTrap) {
  // (module
  //   (import "wasi_ephemeral_nn" "load"
  //     (func $load (param i32 i32 i32 i32 i32) (result i32)))
  //   (import "wasi_ephemeral_nn" "compute" (func $compute (param i32) (result
  //   i32))) (func (export "add") (param i32 i32) (result i32)
  //     (i32.add (local.get 0) (local.get 1)))
  //   (func (export "run_load") (result i32)
  //     (call $load (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 0)
  //       (i32.const 0)))
  //   (func (export "run_compute") (result i32)
  //     (call $compute (i32.const 0))))
  std::array<WasmEdge::Byte, 164> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x19, 0x04, 0x60,
      0x05, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x01,
      0x7f, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7f, 0x02,
      0x36, 0x02, 0x11, 0x77, 0x61, 0x73, 0x69, 0x5f, 0x65, 0x70, 0x68, 0x65,
      0x6d, 0x65, 0x72, 0x61, 0x6c, 0x5f, 0x6e, 0x6e, 0x04, 0x6c, 0x6f, 0x61,
      0x64, 0x00, 0x00, 0x11, 0x77, 0x61, 0x73, 0x69, 0x5f, 0x65, 0x70, 0x68,
      0x65, 0x6d, 0x65, 0x72, 0x61, 0x6c, 0x5f, 0x6e, 0x6e, 0x07, 0x63, 0x6f,
      0x6d, 0x70, 0x75, 0x74, 0x65, 0x00, 0x01, 0x03, 0x04, 0x03, 0x02, 0x03,
      0x03, 0x07, 0x20, 0x03, 0x03, 0x61, 0x64, 0x64, 0x00, 0x02, 0x08, 0x72,
      0x75, 0x6e, 0x5f, 0x6c, 0x6f, 0x61, 0x64, 0x00, 0x03, 0x0b, 0x72, 0x75,
      0x6e, 0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x00, 0x04, 0x0a,
      0x1f, 0x03, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b, 0x0e, 0x00,
      0x41, 0x00, 0x41, 0x00, 0x41, 0x00, 0x41, 0x00, 0x41, 0x00, 0x10, 0x00,
      0x0b, 0x06, 0x00, 0x41, 0x00, 0x10, 0x01, 0x0b};

  Configure Conf;
  Loader::Loader LoadEngine(Conf);
  Validator::Validator ValidEngine(Conf);
  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;
  auto Mod = LoadEngine.parseModule(Wasm);
  ASSERT_TRUE(Mod);
  ASSERT_TRUE(ValidEngine.validate(**Mod));
  auto ModInst = ExecEngine.instantiateModule(Store, **Mod);
  ASSERT_TRUE(ModInst);
  EXPECT_EQ(Store.findModule("wasi_ephemeral_nn"), nullptr);

  // The functions defined by the module still run.
  auto *Add = (*ModInst)->findFuncExports("add");
  ASSERT_NE(Add, nullptr);
  const std::array<ValVariant, 2> Args{ValVariant(uint32_t(1)),
                                       ValVariant(uint32_t(2))};
  const std::array<ValType, 2> ArgTypes{ValType(TypeCode::I32),
                                        ValType(TypeCode::I32)};
  auto SumRes = ExecEngine.invoke(Add, Args, ArgTypes);
  ASSERT_TRUE(SumRes);
  EXPECT_EQ((*SumRes)[0].first.get<uint32_t>(), 3U);

  auto *RunLoad = (*ModInst)->findFuncExports("run_load");
  ASSERT_NE(RunLoad, nullptr);
  auto LoadRes = ExecEngine.invoke(RunLoad, {}, {});
  ASSERT_FALSE(LoadRes);
  EXPECT_EQ(LoadRes.error(), ErrCode::Value::HostFuncError);

  auto *RunCompute = (*ModInst)->findFuncExports("run_compute");
  ASSERT_NE(RunCompute, nullptr);
  auto ComputeRes = ExecEngine.invoke(RunCompute, {}, {});
  ASSERT_FALSE(ComputeRes);
  EXPECT_EQ(ComputeRes.error(), ErrCode::Value::HostFuncError);
}

// Only the function imports of official plugin modules are stood in for.
TEST(PluginImportTest, OtherImportsStayUnknown) {
  // (module
  //   (import "not_a_plugin" "f" (func)))
  std::array<WasmEdge::Byte, 34> UnknownWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x12, 0x01, 0x0c, 0x6e, 0x6f, 0x74, 0x5f, 0x61, 0x5f,
      0x70, 0x6c, 0x75, 0x67, 0x69, 0x6e, 0x01, 0x66, 0x00, 0x00};

  // (module
  //   (import "wasi_ephemeral_nn" "memory" (memory 1)))
  std::array<WasmEdge::Byte, 39> MemoryWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x02, 0x1d,
      0x01, 0x11, 0x77, 0x61, 0x73, 0x69, 0x5f, 0x65, 0x70, 0x68,
      0x65, 0x6d, 0x65, 0x72, 0x61, 0x6c, 0x5f, 0x6e, 0x6e, 0x06,
      0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x01};

  Configure Conf;
  Loader::Loader LoadEngine(Conf);
  Validator::Validator ValidEngine(Conf);
  Executor::Executor ExecEngine(Conf);
  Runtime::StoreManager Store;

  auto Unknown = LoadEngine.parseModule(UnknownWasm);
  ASSERT_TRUE(Unknown);
  ASSERT_TRUE(ValidEngine.validate(**Unknown));
  auto UnknownRes = ExecEngine.instantiateModule(Store, **Unknown);
  ASSERT_FALSE(UnknownRes);
  EXPECT_EQ(UnknownRes.error(), ErrCode::Value::UnknownImport);

  auto Memory = LoadEngine.parseModule(MemoryWasm);
  ASSERT_TRUE(Memory);
  ASSERT_TRUE(ValidEngine.validate(**Memory));
  auto MemoryRes = ExecEngine.instantiateModule(Store, **Memory);
  ASSERT_FALSE(MemoryRes);
  EXPECT_EQ(MemoryRes.error(), ErrCode::Value::UnknownImport);
}

} // namespace
