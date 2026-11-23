// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "DebugInfoTestUtils.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::DebugInfoTest;
using namespace std::literals;

TEST(DebugInfoRun, FixtureMatrix) {
  using OL = CompilerConfigure::OptimizationLevel;
  for (auto [Name, Fn, Want] :
       {std::tuple{"dwarf_basic_O0.wasm"sv, "run"sv, 43U},
        std::tuple{"dwarf_basic_O2.wasm"sv, "run"sv, 43U},
        std::tuple{"dwarf_basic_O0.wasm"sv, "plain"sv, 5U},
        std::tuple{"dwarf_basic_O2.wasm"sv, "plain"sv, 5U},
        std::tuple{"dwarf_enum_O0.wasm"sv, "pick"sv, 7U}}) {
    for (auto Level :
         {OL::O0, OL::O1, OL::O2, OL::O3, OL::Os, OL::Oz, OL::Og}) {
      Configure Conf;
      Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
      Conf.getCompilerConfigure().setOptimizationLevel(Level);
      Conf.getCompilerConfigure().setDebugInfo(true);
      VM::VM Machine(Conf);
      ASSERT_TRUE(Machine.loadWasm(readHexFixture(Name)));
      ASSERT_TRUE(Machine.validate());
      ASSERT_TRUE(Machine.instantiate());
      auto R = Machine.execute(Fn);
      ASSERT_TRUE(R) << Name << " level " << static_cast<int>(Level);
      EXPECT_EQ((*R)[0].first.get<uint32_t>(), Want);
    }
  }
}

} // namespace
