// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/aot/AOTwagonTest.cpp - wagon wasm compiling unit tests --===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading WASM in Wagon project.
///
//===----------------------------------------------------------------------===//

#include "aot/compiler.h"
#include "ast/module.h"
#include "loader/loader.h"
#include "gtest/gtest.h"
#include <vector>

#define BODY(NAME)                                                             \
  do {                                                                         \
    auto Data = *Loader.loadFile("../loader/wagonTestData/" NAME ".wasm"sv);   \
    auto Module = *Loader.parseModule(Data);                                   \
    SSVM::AOT::Compiler Compiler;                                              \
    Compiler.setDumpIR(true);                                                  \
    auto Status = Compiler.compile(Data, *Module,                              \
                                   "../loader/wagonTestData/" NAME ".so"sv);   \
    if (Status) {                                                              \
      ASSERT_TRUE(Status);                                                     \
    } else {                                                                   \
      ASSERT_EQ(Status.error(), SSVM::ErrCode::Success);                       \
    }                                                                          \
  } while (false)

namespace {

using namespace std::literals::string_view_literals;

SSVM::ProposalConfigure ProposalConf;
SSVM::Loader::Loader Loader(ProposalConf);

TEST(WagonTest, Load__add_ex_main) { BODY("add-ex-main"); }
TEST(WagonTest, Load__add_ex) { BODY("add-ex"); }
TEST(WagonTest, Load__address) { BODY("address"); }
TEST(WagonTest, Load__basic) { BODY("basic"); }
TEST(WagonTest, Load__binary) { BODY("binary"); }
TEST(WagonTest, Load__block) { BODY("block"); }
TEST(WagonTest, Load__br_if) { BODY("br_if"); }
TEST(WagonTest, Load__br_table) { BODY("br_table"); }
TEST(WagonTest, Load__br) { BODY("br"); }
TEST(WagonTest, Load__break_drop) { BODY("break-drop"); }
TEST(WagonTest, Load__brif_loop) { BODY("brif-loop"); }
TEST(WagonTest, Load__brif) { BODY("brif"); }
TEST(WagonTest, Load__brtable) { BODY("brtable"); }
TEST(WagonTest, Load__bug_49) { BODY("bug-49"); }
TEST(WagonTest, Load__call_indirect) { BODY("call_indirect"); }
TEST(WagonTest, Load__call_zero_args) { BODY("call-zero-args"); }
TEST(WagonTest, Load__call) { BODY("call"); }
TEST(WagonTest, Load__callindirect) { BODY("callindirect"); }
TEST(WagonTest, Load__cast) { BODY("cast"); }
TEST(WagonTest, Load__compare) { BODY("compare"); }
TEST(WagonTest, Load__convert) { BODY("convert"); }
TEST(WagonTest, Load__custom_section) { BODY("custom_section"); }
TEST(WagonTest, Load__empty) { BODY("empty"); }
TEST(WagonTest, Load__endianness) { BODY("endianness"); }
TEST(WagonTest, Load__expr_block) { BODY("expr-block"); }
TEST(WagonTest, Load__expr_br) { BODY("expr-br"); }
TEST(WagonTest, Load__expr_brif) { BODY("expr-brif"); }
TEST(WagonTest, Load__expr_if) { BODY("expr-if"); }
TEST(WagonTest, Load__f64) { BODY("f64"); }
TEST(WagonTest, Load__fac) { BODY("fac"); }
TEST(WagonTest, Load__forward) { BODY("forward"); }
TEST(WagonTest, Load__get_local) { BODY("get_local"); }
TEST(WagonTest, Load__globals) { BODY("globals"); }
TEST(WagonTest, Load__hello_world_tinygo) { BODY("hello-world-tinygo"); }
TEST(WagonTest, Load__i32) { BODY("i32"); }
TEST(WagonTest, Load__i64) { BODY("i64"); }
TEST(WagonTest, Load__if) { BODY("if"); }
TEST(WagonTest, Load__ifelse_stack_bug) { BODY("ifelse-stack-bug"); }
TEST(WagonTest, Load__int_exprs) { BODY("int_exprs"); }
TEST(WagonTest, Load__load) { BODY("load"); }
TEST(WagonTest, Load__loop) { BODY("loop"); }
TEST(WagonTest, Load__memory_redundancy) { BODY("memory_redundancy"); }
TEST(WagonTest, Load__names) { BODY("names"); }
TEST(WagonTest, Load__nested_if) { BODY("nested-if"); }
TEST(WagonTest, Load__nofuncs) { BODY("nofuncs"); }
TEST(WagonTest, Load__nop) { BODY("nop"); }
TEST(WagonTest, Load__resizing) { BODY("resizing"); }
TEST(WagonTest, Load__return_void) { BODY("return-void"); }
TEST(WagonTest, Load__return) { BODY("return"); }
TEST(WagonTest, Load__rust_basic) { BODY("rust-basic"); }
TEST(WagonTest, Load__select) { BODY("select"); }
TEST(WagonTest, Load__start) { BODY("start"); }
TEST(WagonTest, Load__store) { BODY("store"); }
TEST(WagonTest, Load__switch) { BODY("switch"); }
TEST(WagonTest, Load__tee_local) { BODY("tee_local"); }
TEST(WagonTest, Load__traps_int_div) { BODY("traps_int_div"); }
TEST(WagonTest, Load__traps_int_rem) { BODY("traps_int_rem"); }
TEST(WagonTest, Load__traps_mem) { BODY("traps_mem"); }
TEST(WagonTest, Load__unary) { BODY("unary"); }
TEST(WagonTest, Load__unreachable) { BODY("unreachable"); }
TEST(WagonTest, Load__unwind) { BODY("unwind"); }

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
