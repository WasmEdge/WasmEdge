// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/loader/wagonTest.cpp - wagon wasm loading unit tests ----===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading WASM in Wagon project.
///
//===----------------------------------------------------------------------===//

#include "ast/module.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

SSVM::FileMgrFStream Mgr;
SSVM::Loader::ErrCode SuccessCode = SSVM::Loader::ErrCode::Success;

TEST(WagonTest, Load__add_ex_main) {
  Mgr.setPath("wagonTestData/add-ex-main.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__add_ex) {
  Mgr.setPath("wagonTestData/add-ex.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__address) {
  Mgr.setPath("wagonTestData/address.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__basic) {
  Mgr.setPath("wagonTestData/basic.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__binary) {
  Mgr.setPath("wagonTestData/binary.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__block) {
  Mgr.setPath("wagonTestData/block.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__br_if) {
  Mgr.setPath("wagonTestData/br_if.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__br_table) {
  Mgr.setPath("wagonTestData/br_table.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__br) {
  Mgr.setPath("wagonTestData/br.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__break_drop) {
  Mgr.setPath("wagonTestData/break-drop.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__brif_loop) {
  Mgr.setPath("wagonTestData/brif-loop.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__brif) {
  Mgr.setPath("wagonTestData/brif.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__brtable) {
  Mgr.setPath("wagonTestData/brtable.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__bug_49) {
  Mgr.setPath("wagonTestData/bug-49.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__call_indirect) {
  Mgr.setPath("wagonTestData/call_indirect.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__call_zero_args) {
  Mgr.setPath("wagonTestData/call-zero-args.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__call) {
  Mgr.setPath("wagonTestData/call.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__callindirect) {
  Mgr.setPath("wagonTestData/callindirect.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__cast) {
  Mgr.setPath("wagonTestData/cast.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__compare) {
  Mgr.setPath("wagonTestData/compare.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__convert) {
  Mgr.setPath("wagonTestData/convert.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__custom_section) {
  Mgr.setPath("wagonTestData/custom_section.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__empty) {
  Mgr.setPath("wagonTestData/empty.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__endianness) {
  Mgr.setPath("wagonTestData/endianness.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__expr_block) {
  Mgr.setPath("wagonTestData/expr-block.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__expr_br) {
  Mgr.setPath("wagonTestData/expr-br.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__expr_brif) {
  Mgr.setPath("wagonTestData/expr-brif.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__expr_if) {
  Mgr.setPath("wagonTestData/expr-if.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__f64) {
  Mgr.setPath("wagonTestData/f64.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__fac) {
  Mgr.setPath("wagonTestData/fac.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__forward) {
  Mgr.setPath("wagonTestData/forward.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__get_local) {
  Mgr.setPath("wagonTestData/get_local.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__globals) {
  Mgr.setPath("wagonTestData/globals.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__hello_world_tinygo) {
  Mgr.setPath("wagonTestData/hello-world-tinygo.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__i32) {
  Mgr.setPath("wagonTestData/i32.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__i64) {
  Mgr.setPath("wagonTestData/i64.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__if) {
  Mgr.setPath("wagonTestData/if.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__ifelse_stack_bug) {
  Mgr.setPath("wagonTestData/ifelse-stack-bug.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__int_exprs) {
  Mgr.setPath("wagonTestData/int_exprs.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__load) {
  Mgr.setPath("wagonTestData/load.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__loop) {
  Mgr.setPath("wagonTestData/loop.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__memory_redundancy) {
  Mgr.setPath("wagonTestData/memory_redundancy.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__names) {
  Mgr.setPath("wagonTestData/names.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__nested_if) {
  Mgr.setPath("wagonTestData/nested-if.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__nofuncs) {
  Mgr.setPath("wagonTestData/nofuncs.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__nop) {
  Mgr.setPath("wagonTestData/nop.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__resizing) {
  Mgr.setPath("wagonTestData/resizing.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__return_void) {
  Mgr.setPath("wagonTestData/return-void.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__return) {
  Mgr.setPath("wagonTestData/return.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__rust_basic) {
  Mgr.setPath("wagonTestData/rust-basic.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__select) {
  Mgr.setPath("wagonTestData/select.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__start) {
  Mgr.setPath("wagonTestData/start.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__store) {
  Mgr.setPath("wagonTestData/store.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__switch) {
  Mgr.setPath("wagonTestData/switch.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__tee_local) {
  Mgr.setPath("wagonTestData/tee_local.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__traps_int_div) {
  Mgr.setPath("wagonTestData/traps_int_div.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__traps_int_rem) {
  Mgr.setPath("wagonTestData/traps_int_rem.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__traps_mem) {
  Mgr.setPath("wagonTestData/traps_mem.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__unary) {
  Mgr.setPath("wagonTestData/unary.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__unreachable) {
  Mgr.setPath("wagonTestData/unreachable.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__unwind) {
  Mgr.setPath("wagonTestData/unwind.wasm");
  SSVM::AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
