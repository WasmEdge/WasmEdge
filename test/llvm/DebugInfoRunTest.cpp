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
    for (auto Mode : {RunMode::JIT, RunMode::LazyJIT}) {
      for (auto Level :
           {OL::O0, OL::O1, OL::O2, OL::O3, OL::Os, OL::Oz, OL::Og}) {
        Configure Conf;
        Conf.getRuntimeConfigure().setRunMode(Mode);
        Conf.getCompilerConfigure().setOptimizationLevel(Level);
        Conf.getCompilerConfigure().setDebugInfo(true);
        VM::VM Machine(Conf);
        ASSERT_TRUE(Machine.loadWasm(readHexFixture(Name)));
        ASSERT_TRUE(Machine.validate());
        ASSERT_TRUE(Machine.instantiate());
        auto R = Machine.execute(Fn);
        ASSERT_TRUE(R) << Name << " mode " << static_cast<int>(Mode)
                       << " level " << static_cast<int>(Level);
        EXPECT_EQ((*R)[0].first.get<uint32_t>(), Want);
      }
    }
  }
}

std::optional<uint64_t> debugModuleId(const llvm::Module &M) {
  for (const auto *CU : M.debug_compile_units()) {
    for (const auto *GVE : CU->getGlobalVariables()) {
      const auto *E = GVE->getExpression();
      if (GVE->getVariable()->getName() == "__wasmedge_debug_module" &&
          E->getNumElements() == 3) {
        return E->getElement(1);
      }
    }
  }
  return std::nullopt;
}

TEST(DebugInfoRun, LazyJITModulesCarryDebugInfo) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
  Conf.getCompilerConfigure().setDebugInfo(true);
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  LLVM::Compiler Compiler(Conf);
  auto Infra = Compiler.compileInfrastructure(*Mod);
  ASSERT_TRUE(Infra);
  auto *InfraM = llvm::unwrap(Infra->extract().LLModule.unwrap());
  const bool LinesOnly = nativeGetsOnlyLines();
  auto *MB = InfraM->getGlobalVariable("__wasmedge_debug_membase");
  std::optional<uint64_t> InfraId;
  if (LinesOnly) {
    EXPECT_EQ(MB, nullptr);
  } else {
    ASSERT_NE(MB, nullptr);
    EXPECT_FALSE(MB->isDeclaration());
    llvm::SmallVector<llvm::DIGlobalVariableExpression *, 4> GVEs;
    MB->getDebugInfo(GVEs);
    EXPECT_FALSE(GVEs.empty());
    InfraId = debugModuleId(*InfraM);
    ASSERT_TRUE(InfraId);
  }
  const uint32_t Count =
      static_cast<uint32_t>(Mod->getCodeSection().getContent().size());
  std::vector<uint32_t> All(Count);
  std::iota(All.begin(), All.end(), 0U);
  auto Batch = Compiler.compileFunctions(std::move(*Infra), *Mod, All);
  ASSERT_TRUE(Batch);
  auto *BatchM = llvm::unwrap(Batch->extract().LLModule.unwrap());
  uint32_t WithSP = 0;
  for (auto &F : *BatchM) {
    WithSP += F.getSubprogram() != nullptr;
  }
  EXPECT_GE(WithSP, 3U);
  EXPECT_EQ(debugModuleId(*BatchM), InfraId);
  EXPECT_FALSE(llvm::verifyModule(*BatchM, &llvm::errs()));
  if (LinesOnly) {
    return;
  }

  auto NoGlobals = loadFixture(Conf, "dwarf_branch_O2.wasm"sv);
  ASSERT_NE(NoGlobals, nullptr);
  auto NoGlobalsInfra = Compiler.compileInfrastructure(*NoGlobals);
  ASSERT_TRUE(NoGlobalsInfra);
  EXPECT_EQ(llvm::unwrap(NoGlobalsInfra->extract().LLModule.unwrap())
                ->getGlobalVariable("__wasmedge_debug_membase"),
            nullptr);
  const auto NoGlobalsId =
      debugModuleId(*llvm::unwrap(NoGlobalsInfra->extract().LLModule.unwrap()));
  ASSERT_TRUE(NoGlobalsId);
  const std::vector<uint32_t> First{0U};
  auto NoGlobalsBatch =
      Compiler.compileFunctions(std::move(*NoGlobalsInfra), *NoGlobals, First);
  ASSERT_TRUE(NoGlobalsBatch);
  EXPECT_EQ(
      debugModuleId(*llvm::unwrap(NoGlobalsBatch->extract().LLModule.unwrap())),
      NoGlobalsId);

  auto Other = Compiler.compileInfrastructure(*Mod);
  ASSERT_TRUE(Other);
  const auto OtherId =
      debugModuleId(*llvm::unwrap(Other->extract().LLModule.unwrap()));
  ASSERT_TRUE(OtherId);
  EXPECT_NE(*OtherId, *InfraId);
}

} // namespace
