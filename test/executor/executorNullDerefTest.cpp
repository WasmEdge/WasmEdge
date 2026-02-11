// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/executor/executorNullDerefTest.cpp -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests for unchecked Expect<> dereference in toBottomType.
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
//   (type $s (struct (field i32)))
//   (func (export "test") (result i32)
//     i32.const 7  struct.new $s  struct.get $s 0))
std::array<Byte, 48> StructNewWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02,
    0x5f, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01,
    0x01, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x00,
    0x0a, 0x0d, 0x01, 0x0b, 0x00, 0x41, 0x07, 0xfb, 0x00, 0x00, 0xfb,
    0x02, 0x00, 0x00, 0x0b,
};

// Module with struct having ref field pointing to non-existent type 99.
// (module
//   (type $s (struct (field (ref null 99))))
//   (type $fn (func (result (ref null struct))))
//   (func (export "test") (result (ref null struct))
//     struct.new_default $s))
std::array<Byte, 48> StructBadRefTypeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x02,
    0x5f, 0x01, 0x63, 0x63, 0x00, 0x60, 0x00, 0x01, 0x63, 0x6b, 0x03,
    0x02, 0x01, 0x01, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74,
    0x00, 0x00, 0x0a, 0x07, 0x01, 0x05, 0x00, 0xfb, 0x01, 0x00, 0x0b,
};

TEST(ExecutorToBottomType, ValidStructNew) {
  Configure Conf;
  Loader::Loader Ldr(Conf);
  Validator::Validator Valid(Conf);
  Executor::Executor Exec(Conf);
  Runtime::StoreManager Store;

  auto ASTMod = Ldr.parseModule(StructNewWasm);
  ASSERT_TRUE(ASTMod) << ASTMod.error();
  ASSERT_TRUE(Valid.validate(**ASTMod));
  auto ModInst = Exec.instantiateModule(Store, **ASTMod);
  ASSERT_TRUE(ModInst) << ModInst.error();
  auto *FuncInst = (*ModInst)->findFuncExports("test");
  ASSERT_NE(FuncInst, nullptr);
  auto Res = Exec.invoke(FuncInst, {}, {});
  ASSERT_TRUE(Res) << Res.error();
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ((*Res)[0].first.get<uint32_t>(), 7U);
}

TEST(ExecutorToBottomType, LoaderRejectsBadRefType) {
  // The loader rejects struct fields referencing non-existent types,
  // providing defense-in-depth for the toBottomType code path.
  Configure Conf;
  Loader::Loader Ldr(Conf);

  auto ASTMod = Ldr.parseModule(StructBadRefTypeWasm);
  EXPECT_FALSE(ASTMod);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
