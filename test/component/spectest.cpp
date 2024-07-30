// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include <arm_neon.h>
#include <gtest/gtest.h>

#include "common/types.h"
#include "vm/vm.h"

namespace {

using namespace WasmEdge;

template <typename T> void assertOk(Expect<T> Res, const char *Message) {
  if (!Res) {
    EXPECT_TRUE(false) << Message;
  }
}

TEST(Component, LoadAndRun_SimpleBinary) {
  Configure Conf{};
  Conf.addProposal(Proposal::Component);
  VM::VM VM{Conf};

  assertOk(VM.loadWasm("data/core.wasm"), "failed to load component binary");
  assertOk(VM.validate(), "failed to validate");
  assertOk(VM.instantiate(), "failed to instantiate");

  uint64_t V = 100;
  auto Res = VM.execute("mdup", {ValInterface(ValVariant(V))},
                        {ValType(TypeCode::I64)});
  assertOk(Res, "failed to execute");
  std::vector<std::pair<ValInterface, ValType>> Result = *Res;
  auto Ret = std::get<ValVariant>(Result[0].first).get<uint64_t>();
  EXPECT_EQ(Ret, 200);
}

TEST(Component, Load_HttpBinary) {
  Configure Conf{};
  Conf.addProposal(Proposal::Component);
  VM::VM VM{Conf};

  assertOk(VM.loadWasm("data/http.wasm"), "failed to load component binary");
  assertOk(VM.validate(), "failed to validate");
}

} // namespace
