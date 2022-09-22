// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===----------------------------------------------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// This file contains tests of Wasm component-model
///
//===----------------------------------------------------------------------===//

#include <gtest/gtest.h>

#include "executor/executor.h"
#include "loader/loader.h"
#include "vm/vm.h"

namespace {

TEST(ComponentExecution, Init) {
  WasmEdge::Configure Conf{};
  Conf.addProposal(WasmEdge::Proposal::ComponentModel);
  WasmEdge::VM::VM VM{Conf};
  VM.registerComponent("INIT", "testData/init.wasm");
  FAIL();
}

} // namespace
