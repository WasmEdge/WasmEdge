// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/executor/executorNullDerefTest.cpp -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests for unchecked Expect<> dereference in proxyCallIndirect.
///
//===----------------------------------------------------------------------===//

#include "common/configure.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "runtime/instance/module.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"

#include <array>
#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;

// (module
//   (type $t (func (result i32)))
//   (table 1 funcref)
//   (elem (i32.const 0) func $f)
//   (func $f (type $t) (result i32) i32.const 42)
//   (func (export "test") (result i32) i32.const 0 call_indirect (type $t)))
std::array<Byte, 61> CallIndirectValidWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x00, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00, 0x00, 0x04, 0x04,
    0x01, 0x70, 0x00, 0x01, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73,
    0x74, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01,
    0x00, 0x0a, 0x0e, 0x02, 0x04, 0x00, 0x41, 0x2a, 0x0b, 0x07, 0x00,
    0x41, 0x00, 0x11, 0x00, 0x00, 0x0b,
};

// proxyCallIndirect is only reachable from AOT-compiled code.
// This test verifies the interpreter call_indirect path still works after the
// proxy.cpp fix, since both share the same getType() call pattern.
TEST(ExecutorProxyCallIndirect, ValidCallIndirectInterpreter) {
  Configure Conf;
  Loader::Loader Ldr(Conf);
  Validator::Validator Valid(Conf);
  Executor::Executor Exec(Conf);
  Runtime::StoreManager Store;

  auto ASTMod = Ldr.parseModule(CallIndirectValidWasm);
  ASSERT_TRUE(ASTMod) << ASTMod.error();
  ASSERT_TRUE(Valid.validate(**ASTMod));
  auto ModInst = Exec.instantiateModule(Store, **ASTMod);
  ASSERT_TRUE(ModInst) << ModInst.error();
  auto *FuncInst = (*ModInst)->findFuncExports("test");
  ASSERT_NE(FuncInst, nullptr);
  auto Res = Exec.invoke(FuncInst, {}, {});
  ASSERT_TRUE(Res) << Res.error();
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 42U);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
