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

#include "common/ast/module.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

SSVM::FileMgrFStream Mgr;

TEST(WagonTest, Load__add_ex_main) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/add-ex-main.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__add_ex) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/add-ex.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__address) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/address.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__basic) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/basic.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__binary) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/binary.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__block) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/block.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__br_if) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/br_if.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__br_table) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/br_table.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__br) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/br.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__break_drop) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/break-drop.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__brif_loop) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/brif-loop.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__brif) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/brif.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__brtable) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/brtable.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__bug_49) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/bug-49.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__call_indirect) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/call_indirect.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__call_zero_args) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/call-zero-args.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__call) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/call.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__callindirect) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/callindirect.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__cast) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/cast.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__compare) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/compare.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__convert) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/convert.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__custom_section) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/custom_section.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__empty) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/empty.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__endianness) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/endianness.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__expr_block) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/expr-block.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__expr_br) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/expr-br.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__expr_brif) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/expr-brif.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__expr_if) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/expr-if.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__f64) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/f64.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__fac) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/fac.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__forward) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/forward.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__get_local) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/get_local.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__globals) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/globals.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__hello_world_tinygo) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/hello-world-tinygo.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__i32) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/i32.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__i64) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/i64.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__if) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/if.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__ifelse_stack_bug) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/ifelse-stack-bug.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__int_exprs) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/int_exprs.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__load) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/load.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__loop) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/loop.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__memory_redundancy) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/memory_redundancy.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__names) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/names.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__nested_if) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/nested-if.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__nofuncs) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/nofuncs.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__nop) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/nop.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__resizing) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/resizing.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__return_void) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/return-void.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__return) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/return.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__rust_basic) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/rust-basic.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__select) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/select.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__start) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/start.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__store) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/store.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__switch) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/switch.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__tee_local) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/tee_local.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__traps_int_div) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/traps_int_div.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__traps_int_rem) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/traps_int_rem.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__traps_mem) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/traps_mem.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__unary) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/unary.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__unreachable) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/unreachable.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__unwind) {
  ASSERT_TRUE(Mgr.setPath("wagonTestData/unwind.wasm"));
  SSVM::AST::Module Mod;
  ASSERT_TRUE(Mod.loadBinary(Mgr));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
