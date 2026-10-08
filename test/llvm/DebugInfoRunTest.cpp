// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "DebugInfoTestUtils.h"

#include <llvm/Support/TargetSelect.h>

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

TEST(DebugInfoRun, JITTeardownUnregistersDebugObjects) {
#if WASMEDGE_OS_WINDOWS || (WASMEDGE_OS_MACOS && LLVM_VERSION_MAJOR < 15)
  GTEST_SKIP() << "the JIT gives the debugger no debug info here";
#else
  const auto Count = LLVM::countJITDebugObjects;
  const auto Before = Count();
  for (uint32_t I = 0; I < 2; ++I) {
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
    Conf.getCompilerConfigure().setDebugInfo(true);
    VM::VM Machine(Conf);
    ASSERT_TRUE(Machine.loadWasm(readHexFixture("dwarf_basic_O0.wasm"sv)));
    ASSERT_TRUE(Machine.validate());
    ASSERT_TRUE(Machine.instantiate());
    ASSERT_TRUE(Machine.execute("run"sv));
    EXPECT_GT(Count(), Before);
  }
  EXPECT_EQ(Count(), Before);
#endif
}

#if LLVM_VERSION_MAJOR < 21 || WASMEDGE_OS_WINDOWS || WASMEDGE_OS_MACOS
#else
/// Fails every link after its fixups, after the debug object plugins ran
/// their own fixup passes.
class FailingLinkPlugin final : public llvm::orc::ObjectLinkingLayer::Plugin {
public:
  void modifyPassConfig(llvm::orc::MaterializationResponsibility &,
                        llvm::jitlink::LinkGraph &,
                        llvm::jitlink::PassConfiguration &Config) override {
    Config.PostFixupPasses.push_back([](llvm::jitlink::LinkGraph &) {
      return llvm::make_error<llvm::StringError>(
          "injected link failure", llvm::inconvertibleErrorCode());
    });
  }
  llvm::Error
  notifyFailed(llvm::orc::MaterializationResponsibility &) override {
    return llvm::Error::success();
  }
  llvm::Error notifyRemovingResources(llvm::orc::JITDylib &,
                                      llvm::orc::ResourceKey) override {
    return llvm::Error::success();
  }
  void notifyTransferringResources(llvm::orc::JITDylib &,
                                   llvm::orc::ResourceKey,
                                   llvm::orc::ResourceKey) override {}
};

/// A module with debug info whose function Name returns 7.
llvm::orc::ThreadSafeModule debugModule(const llvm::orc::LLJIT &J,
                                        std::string_view Name) {
  auto Ctx = std::make_unique<llvm::LLVMContext>();
  auto M = std::make_unique<llvm::Module>(Name, *Ctx);
  M->setDataLayout(J.getDataLayout());
  M->setTargetTriple(J.getTargetTriple());
  M->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                   llvm::DEBUG_METADATA_VERSION);
  M->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
  llvm::DIBuilder DB(*M);
  auto *File = DB.createFile("seven.c", "/wasmedge");
  DB.createCompileUnit(
#if LLVM_VERSION_MAJOR < 22
      llvm::dwarf::DW_LANG_C11,
#else
      llvm::DISourceLanguageName(llvm::dwarf::DW_LANG_C11),
#endif
      File, "test", false, "", 0);
  const llvm::StringRef FnName(Name.data(), Name.size());
  auto *Fn = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt32Ty(*Ctx), false),
      llvm::Function::ExternalLinkage, FnName, *M);
  auto *SP = DB.createFunction(
      File, FnName, "", File, 1,
      DB.createSubroutineType(DB.getOrCreateTypeArray({})), 1,
      llvm::DINode::FlagZero, llvm::DISubprogram::SPFlagDefinition);
  Fn->setSubprogram(SP);
  llvm::IRBuilder<> B(llvm::BasicBlock::Create(*Ctx, "entry", Fn));
  B.SetCurrentDebugLocation(llvm::DILocation::get(*Ctx, 1, 1, SP));
  B.CreateRet(B.getInt32(7));
  DB.finalize();
  return llvm::orc::ThreadSafeModule(std::move(M), std::move(Ctx));
}

/// Adds the module of debugModule and looks up its function, so that the
/// JIT links it.
void addAndLink(llvm::orc::LLJIT &J, llvm::orc::ResourceTrackerSP RT,
                std::string_view Name) {
  if (auto Err = J.addIRModule(RT, debugModule(J, Name))) {
    ADD_FAILURE() << llvm::toString(std::move(Err));
    return;
  }
  auto Sym = J.lookup(llvm::StringRef(Name.data(), Name.size()));
  if (!Sym) {
    ADD_FAILURE() << llvm::toString(Sym.takeError());
  }
}
#endif

TEST(DebugInfoRun, FailedJITLinkFailsTheLookup) {
#if LLVM_VERSION_MAJOR < 21 || WASMEDGE_OS_WINDOWS || WASMEDGE_OS_MACOS
  GTEST_SKIP() << "the test builds a JITLink layer with the LLVM 21 API";
#else
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  const auto Before = LLVM::countJITDebugObjects();
  const auto PendingBefore = LLVM::pendingJITDebugEntries();
  {
    auto J =
        llvm::orc::LLJITBuilder()
            .setObjectLinkingLayerCreator(
                [](llvm::orc::ExecutionSession &ES)
                    -> llvm::Expected<std::unique_ptr<llvm::orc::ObjectLayer>> {
                  return std::make_unique<llvm::orc::ObjectLinkingLayer>(ES);
                })
            .create();
    ASSERT_TRUE(static_cast<bool>(J)) << llvm::toString(J.takeError());
    if (auto Err = LLVM::enableJITDebugger(**J)) {
      FAIL() << llvm::toString(std::move(Err));
    }
    llvm::cast<llvm::orc::ObjectLinkingLayer>((*J)->getObjLinkingLayer())
        .addPlugin(std::make_unique<FailingLinkPlugin>());
    if (auto Err = (*J)->addIRModule(debugModule(**J, "seven"))) {
      FAIL() << llvm::toString(std::move(Err));
    }
    auto Sym = (*J)->lookup("seven");
    ASSERT_FALSE(static_cast<bool>(Sym));
    llvm::consumeError(Sym.takeError());
    EXPECT_EQ(LLVM::countJITDebugObjects(), Before);
    EXPECT_EQ(LLVM::pendingJITDebugEntries(), PendingBefore);
  }
  EXPECT_EQ(LLVM::countJITDebugObjects(), Before);
#endif
}

#if LLVM_VERSION_MAJOR < 21 || WASMEDGE_OS_WINDOWS || WASMEDGE_OS_MACOS
#else
/// An LLJIT with the debugger support of WasmEdge.
std::unique_ptr<llvm::orc::LLJIT> debuggerJIT() {
  auto J = llvm::orc::LLJITBuilder().create();
  if (!J) {
    ADD_FAILURE() << llvm::toString(J.takeError());
    return nullptr;
  }
  if (auto Err = LLVM::enableJITDebugger(**J)) {
    ADD_FAILURE() << llvm::toString(std::move(Err));
    return nullptr;
  }
  return std::move(*J);
}

#endif

TEST(DebugInfoRun, RemovingAResourceRemovesOnlyItsDebugObjects) {
#if LLVM_VERSION_MAJOR < 21 || WASMEDGE_OS_WINDOWS || WASMEDGE_OS_MACOS
  GTEST_SKIP() << "the test builds a JITLink layer with the LLVM 21 API";
#else
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  const auto Count = LLVM::countJITDebugObjects;
  const auto Before = Count();
  auto First = debuggerJIT();
  auto Second = debuggerJIT();
  ASSERT_TRUE(First && Second);
  auto Tracker = First->getMainJITDylib().createResourceTracker();
  addAndLink(*First, Tracker, "first");
  EXPECT_EQ(Count(), Before + 1);
  if (auto Err = Tracker->remove()) {
    FAIL() << llvm::toString(std::move(Err));
  }
  Tracker.reset();
  EXPECT_EQ(Count(), Before);
  addAndLink(*Second, Second->getMainJITDylib().getDefaultResourceTracker(),
             "second");
  EXPECT_EQ(Count(), Before + 1);
  addAndLink(*First, First->getMainJITDylib().createResourceTracker(), "again");
  EXPECT_EQ(Count(), Before + 2);
  First.reset();
  EXPECT_EQ(Count(), Before + 1);
  Second.reset();
  EXPECT_EQ(Count(), Before);
#endif
}

TEST(DebugInfoRun, RemovedPendingEntryIsNeverAdded) {
#if LLVM_VERSION_MAJOR < 15 || WASMEDGE_OS_WINDOWS
  GTEST_SKIP() << "the JIT has no entry registry here";
#else
  static const char Object[16] = {};
  const auto Start = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Object));
  const auto Before = LLVM::countJITDebugObjects();
  const auto Cancelled = LLVM::reserveJITDebugEntry();
  LLVM::removeJITDebugEntries({Cancelled});
  LLVM::addJITDebugEntry(Cancelled, Start, sizeof(Object));
  EXPECT_EQ(LLVM::countJITDebugObjects(), Before);
  const auto Kept = LLVM::reserveJITDebugEntry();
  LLVM::addJITDebugEntry(Kept, Start, sizeof(Object));
  EXPECT_EQ(LLVM::countJITDebugObjects(), Before + 1);
  LLVM::removeJITDebugEntries({Kept});
  EXPECT_EQ(LLVM::countJITDebugObjects(), Before);
#endif
}

TEST(DebugInfoRun, MovedJITHandleIsDisposedOnce) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  {
    LLVM::OrcLLJIT Empty;
  }
  auto J = LLVM::OrcLLJIT::create();
  ASSERT_TRUE(J);
  LLVM::OrcLLJIT Moved(std::move(*J));
  EXPECT_TRUE(Moved);
  EXPECT_FALSE(*J);
}

TEST(DebugInfoRun, DebugObjectOwnersKeepEntriesWhileTheMapGrows) {
  for (uintptr_t Keys = 1; Keys <= 64; ++Keys) {
    LLVM::DebugObjectOwners Owners;
    for (uintptr_t Key = 0; Key < Keys; ++Key) {
      Owners.add(Key, {Key * 2, Key * 2 + 1});
    }
    Owners.transfer(Keys + 100, 0);
    Owners.transfer(1, Keys + 100);
    EXPECT_TRUE(Owners.take(0).empty()) << Keys;
    EXPECT_TRUE(Owners.take(Keys + 100).empty()) << Keys;
    auto Moved = Owners.take(1);
    std::sort(Moved.begin(), Moved.end());
    auto Want = Keys > 1 ? std::vector<uint64_t>{0, 1, 2, 3}
                         : std::vector<uint64_t>{0, 1};
    EXPECT_EQ(Moved, Want) << Keys;
    EXPECT_EQ(Owners.takeAll().size(), Keys > 2 ? (Keys - 2) * 2 : 0) << Keys;
  }
}

TEST(DebugInfoRun, NoDwarfModuleRecordsNoDebugInfo) {
  const bool Supported = LLVM::jitDebuggerSupported();
  for (auto [Name, Want] : {std::pair{"dwarf_basic_nodebug.wasm"sv, false},
                            std::pair{"dwarf_basic_O0.wasm"sv, true}}) {
    const bool Emitted = Want && Supported;
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
    Conf.getCompilerConfigure().setDebugInfo(true);
    auto Mod = loadFixture(Conf, Name);
    ASSERT_NE(Mod, nullptr);
    LLVM::Compiler Compiler(Conf);
    auto Eager = Compiler.compile(*Mod);
    ASSERT_TRUE(Eager);
    EXPECT_EQ(Eager->extract().HasDebugInfo, Emitted) << Name;

    Conf.getRuntimeConfigure().setRunMode(RunMode::LazyJIT);
    LLVM::Compiler LazyCompiler(Conf);
    auto Infra = LazyCompiler.compileInfrastructure(*Mod);
    ASSERT_TRUE(Infra);
    EXPECT_EQ(Infra->extract().HasDebugInfo, Emitted) << Name;
    ASSERT_EQ(Infra->extract().DebugReader.has_value(), Supported) << Name;
    if (Supported) {
      EXPECT_EQ(static_cast<bool>(*Infra->extract().DebugReader), Want) << Name;
    }
    const std::vector<uint32_t> First{0U};
    auto Batch = LazyCompiler.compileFunctions(std::move(*Infra), *Mod, First);
    ASSERT_TRUE(Batch);
    EXPECT_EQ(Batch->extract().DebugReader.has_value(), Supported) << Name;
  }
  for (auto Mode : {RunMode::JIT, RunMode::LazyJIT}) {
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(Mode);
    Conf.getCompilerConfigure().setDebugInfo(true);
    VM::VM Machine(Conf);
    ASSERT_TRUE(Machine.loadWasm(readHexFixture("dwarf_basic_nodebug.wasm"sv)));
    ASSERT_TRUE(Machine.validate());
    ASSERT_TRUE(Machine.instantiate());
    auto R = Machine.execute("run"sv);
    ASSERT_TRUE(R) << static_cast<int>(Mode);
    EXPECT_EQ((*R)[0].first.get<uint32_t>(), 43U);
  }
}

TEST(DebugInfoRun, JITUsesO0OnlyForAModuleWithDwarf) {
  using OL = CompilerConfigure::OptimizationLevel;
  auto LevelFor = [](const Configure &Conf, const AST::Module &Mod) {
    return LLVM::jitOptimizationLevel(Conf, Mod);
  };
  auto Drop = [](AST::Module &Mod, std::string_view Name) {
    auto &Customs = Mod.getCustomSections();
    Customs.erase(std::remove_if(Customs.begin(), Customs.end(),
                                 [Name](const AST::CustomSection &C) {
                                   return C.getName() == Name;
                                 }),
                  Customs.end());
  };
  for (auto Mode : {RunMode::JIT, RunMode::LazyJIT}) {
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(Mode);
    Conf.getCompilerConfigure().setOptimizationLevel(OL::O1);
    Conf.getCompilerConfigure().setDebugInfo(true);
    auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
    ASSERT_NE(Mod, nullptr);
    EXPECT_EQ(LevelFor(Conf, *Mod), OL::O1);
    Conf.getCompilerConfigure().setJITDebugO0(true);
    EXPECT_EQ(LevelFor(Conf, *Mod),
              LLVM::jitDebuggerSupported() ? OL::O0 : OL::O1);
    Drop(*Mod, ".debug_line"sv);
    EXPECT_EQ(LevelFor(Conf, *Mod),
              LLVM::jitDebuggerSupported() ? OL::O0 : OL::O1);
    Drop(*Mod, ".debug_info"sv);
    EXPECT_EQ(LevelFor(Conf, *Mod), OL::O1);

    VM::VM Machine(Conf);
    ASSERT_TRUE(Machine.loadWasm(*Mod));
    ASSERT_TRUE(Machine.validate());
    ASSERT_TRUE(Machine.instantiate());
    auto R = Machine.execute("run"sv);
    ASSERT_TRUE(R) << static_cast<int>(Mode);
    EXPECT_EQ((*R)[0].first.get<uint32_t>(), 43U);
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
  if (!LLVM::jitDebuggerSupported()) {
    GTEST_SKIP() << "the JIT gives the debugger no debug info here";
  }
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
