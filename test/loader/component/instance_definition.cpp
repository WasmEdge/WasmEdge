// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Load component model: Type
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading file as AST component nodes.
///
//===----------------------------------------------------------------------===//

#include "loader/loader.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace WasmEdge::AST;

WasmEdge::Configure Conf;
WasmEdge::Loader::Loader Ldr{Conf};

TEST(ComponentTest, InstanceDefinition) {
  Conf.addProposal(WasmEdge::Proposal::ComponentModel);

  std::filesystem::path path{"testData/instance-definition.wasm"};
  auto Comp = Ldr.parseComponent(path);
  EXPECT_TRUE(Comp);
}

} // namespace
