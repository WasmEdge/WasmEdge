// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include <arm_neon.h>
#include <gtest/gtest.h>

#include "common/types.h"
#include "vm/vm.h"

namespace {

using namespace WasmEdge;

TEST(Component, LoadAndRun_SimpleBinary) {
  Configure Conf{};
  Conf.addProposal(Proposal::Component);
  VM::VM VM{Conf};

  if (auto Res = VM.loadWasm("data/core.wasm"); !Res) {
    EXPECT_TRUE(false) << "failed to load component binary";
  }

  VM.validate();
  VM.instantiate();
  auto Res = VM.execute("mdup", {ValInterface(ValVariant(100))},
                        {ValType(TypeCode::I64)});
  if (!Res) {
    EXPECT_TRUE(false) << "failed to execute";
  }
  std::vector<std::pair<ValInterface, ValType>> Result = *Res;
  auto Ret = std::get<ValVariant>(Result[0].first).get<int64_t>();
  EXPECT_EQ(Ret, 200);
}

} // namespace
