// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Component validator unit tests
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of validating file as AST component nodes.
///
//===----------------------------------------------------------------------===//

#include "loader/loader.h"
#include "validator/validator.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge::AST;

WasmEdge::Configure Conf;
WasmEdge::Loader::Loader Ldr{Conf};
WasmEdge::Validator::Validator Vdtr{Conf};

TEST(ComponentTest, NestedCoreModuleTypes) {
  Conf.addProposal(WasmEdge::Proposal::ComponentModel);

  std::filesystem::path path{"testData/nested-core-module-types.wasm"};
  auto Comp = Ldr.parseComponent(path);
  EXPECT_TRUE(Comp);

  auto Res = Vdtr.validate(*(*Comp).get());
  EXPECT_FALSE(Res);
  EXPECT_EQ(WasmEdge::ErrCode::Value::NestedCoreModuleTypes, Res.error());
}

} // namespace
