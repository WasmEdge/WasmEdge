// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "DebugInfoTestUtils.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::DebugInfoTest;
using namespace std::literals;

/// The directory of the outputs of this program. It goes away with all of
/// its files when the program ends, so a test that stops at a failed
/// assertion leaves no library or object file.
const std::filesystem::path &outputDirectory() {
  struct Directory {
    std::filesystem::path Path =
        std::filesystem::temp_directory_path() /
        fmt::format("wasmedge-dwarf-{}", std::random_device{}());
    Directory() { std::filesystem::create_directories(Path); }
    ~Directory() {
      std::error_code Error;
      std::filesystem::remove_all(Path, Error);
    }
  };
  static const Directory Output;
  return Output.Path;
}

std::filesystem::path tempOutput(std::string_view Stem) {
  return outputDirectory() / fmt::format("{}" WASMEDGE_LIB_EXTENSION, Stem);
}

std::filesystem::path
compileModuleToSO(Configure Conf, const AST::Module &Mod, bool Debug,
                  CompilerConfigure::OptimizationLevel Level,
                  std::filesystem::path Out, std::vector<Byte> Bytes = {}) {
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getCompilerConfigure().setOptimizationLevel(Level);
  Conf.getCompilerConfigure().setDebugInfo(Debug);
  LLVM::Compiler Compiler(Conf);
  auto Data = Compiler.compile(Mod);
  if (!Data) {
    return {};
  }
  LLVM::CodeGen CG(Conf);
  if (!CG.codegen(Bytes, std::move(*Data), Out)) {
    return {};
  }
  return Out;
}

std::filesystem::path compileToSO(Configure Conf, std::string_view Name,
                                  bool Debug,
                                  CompilerConfigure::OptimizationLevel Level,
                                  std::filesystem::path Out = {}) {
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  auto Mod = loadFixture(Conf, Name);
  if (!Mod) {
    return {};
  }
  if (Out.empty()) {
    Out = tempOutput(
        fmt::format("{}-{}-{}", Name, Debug, static_cast<int>(Level)));
  }
  return compileModuleToSO(Conf, *Mod, Debug, Level, Out);
}

std::filesystem::path debugFileOf(std::filesystem::path SO) {
#if WASMEDGE_OS_MACOS
  auto O = llvm::object::ObjectFile::createObjectFile(SO.string());
  if (!O) {
    llvm::consumeError(O.takeError());
    return SO;
  }
  const auto *MachO =
      llvm::dyn_cast<llvm::object::MachOObjectFile>(O->getBinary());
  if (!MachO) {
    return SO;
  }
  for (const auto &Symbol : MachO->symbols()) {
    const auto Ref = Symbol.getRawDataRefImpl();
    if (MachO->getSymbol64TableEntry(Ref).n_type != llvm::MachO::N_OSO) {
      continue;
    }
    auto Name = Symbol.getName();
    if (!Name) {
      llvm::consumeError(Name.takeError());
      return SO;
    }
    return Name->str();
  }
#endif
  return SO;
}

void removeOutput(const std::filesystem::path &SO) {
  const auto Debug = debugFileOf(SO);
  std::error_code Error;
  std::filesystem::remove(SO, Error);
  std::filesystem::remove(Debug, Error);
}

std::vector<std::pair<std::string, std::string>>
sectionContents(const std::filesystem::path &P) {
  std::vector<std::pair<std::string, std::string>> Result;
  auto O = llvm::object::ObjectFile::createObjectFile(P.string());
  if (!O) {
    llvm::consumeError(O.takeError());
    return Result;
  }
  for (const auto &Sec : O->getBinary()->sections()) {
#if LLVM_VERSION_MAJOR < 10
    llvm::StringRef Name;
    if (Sec.getName(Name)) {
      continue;
    }
    std::string SecName = Name.str();
#else
    auto Name = Sec.getName();
    if (!Name) {
      llvm::consumeError(Name.takeError());
      continue;
    }
    std::string SecName = Name->str();
#endif
    auto Data = Sec.getContents();
    if (!Data) {
      llvm::consumeError(Data.takeError());
      continue;
    }
    Result.emplace_back(std::move(SecName), Data->str());
  }
  return Result;
}

struct NativeDwarf {
  llvm::object::OwningBinary<llvm::object::ObjectFile> Obj;
  std::unique_ptr<llvm::DWARFContext> Ctx;
  bool open(const std::filesystem::path &P) {
    auto O =
        llvm::object::ObjectFile::createObjectFile(debugFileOf(P).string());
    if (!O) {
      llvm::consumeError(O.takeError());
      return false;
    }
    Obj = std::move(*O);
    Ctx = llvm::DWARFContext::create(*Obj.getBinary());
    return Ctx->getNumCompileUnits() > 0;
  }
  bool hasSubprogram(std::string_view Name) {
    for (const auto &CU : Ctx->compile_units()) {
      for (const auto &Entry : CU->dies()) {
        llvm::DWARFDie D(CU.get(), &Entry);
        if (D.getTag() == llvm::dwarf::DW_TAG_subprogram &&
            D.getName(llvm::DINameKind::ShortName) &&
            Name == D.getName(llvm::DINameKind::ShortName)) {
          return true;
        }
      }
    }
    return false;
  }
  bool hasLine(std::string_view File, uint32_t Line) {
    for (const auto &CU : Ctx->compile_units()) {
      auto *LT = Ctx->getLineTableForUnit(CU.get());
      if (!LT) {
        continue;
      }
      for (const auto &Row : LT->Rows) {
        std::string Path;
        LT->getFileNameByIndex(
            Row.File, CU->getCompilationDir(),
            llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath,
            Path);
        const std::string_view View(Path);
        if (Row.Line == Line && View.size() >= File.size() &&
            View.substr(View.size() - File.size()) == File) {
          return true;
        }
      }
    }
    return false;
  }
};

uint32_t markerLine(std::string_view Marker,
                    std::string_view Source = "dwarf_basic.c"sv) {
  std::ifstream In(fixture(Source));
  std::string Text;
  for (uint32_t N = 1; std::getline(In, Text); ++N) {
    if (Text.find(Marker) != std::string::npos) {
      return N;
    }
  }
  return 0;
}

TEST(DebugInfoAOT, LinesAndSubprograms) {
  for (auto Level : {CompilerConfigure::OptimizationLevel::O0,
                     CompilerConfigure::OptimizationLevel::O2}) {
    auto SO = compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level);
    ASSERT_FALSE(SO.empty());
    NativeDwarf D;
    ASSERT_TRUE(D.open(SO));
    EXPECT_TRUE(D.hasSubprogram("sumList"sv));
    EXPECT_TRUE(D.hasSubprogram("fact"sv));
    EXPECT_TRUE(D.hasSubprogram("run"sv));
    EXPECT_TRUE(D.hasLine("dwarf_basic.c"sv, markerLine("BREAK_SUMLIST"sv)));
    EXPECT_TRUE(D.hasLine("dwarf_basic.c"sv, markerLine("BREAK_RUN"sv)));
    if (Level == CompilerConfigure::OptimizationLevel::O0) {
      EXPECT_TRUE(D.hasLine("dwarf_basic.c"sv, markerLine("BREAK_FACT"sv)));
    }
    removeOutput(SO);
  }
}

TEST(DebugInfoAOT, LineRowsKeepTheirDiscriminators) {
  const auto Line = markerLine("LOOP_LINE"sv, "dwarf_discriminator.c"sv);
  ASSERT_NE(Line, 0U);
  auto Discriminators = [Line](llvm::DWARFContext &Ctx) {
    std::set<uint32_t> Result;
    for (const auto &CU : Ctx.compile_units()) {
      if (const auto *LT = Ctx.getLineTableForUnit(CU.get())) {
        for (const auto &Row : LT->Rows) {
          if (Row.Line == Line && Row.Discriminator != 0) {
            Result.insert(Row.Discriminator);
          }
        }
      }
    }
    return Result;
  };
  std::set<uint32_t> Wasm;
  {
    Configure Conf;
    auto Mod = loadFixture(Conf, "dwarf_discriminator_O2.wasm"sv);
    ASSERT_NE(Mod, nullptr);
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    Wasm = Discriminators(R->getContext());
  }
  ASSERT_FALSE(Wasm.empty());
  auto SO = compileToSO(Configure{}, "dwarf_discriminator_O2.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  EXPECT_EQ(Discriminators(*D.Ctx), Wasm);
  removeOutput(SO);
}

TEST(DebugInfoAOT, InlinedTwiceAppearsAsInlinedSubroutine) {
  auto SO = compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  bool Found = false;
  for (const auto &CU : D.Ctx->compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() == llvm::dwarf::DW_TAG_inlined_subroutine) {
        auto Origin = Die.getAttributeValueAsReferencedDie(
            llvm::dwarf::DW_AT_abstract_origin);
        Found |= Origin && Origin.getName(llvm::DINameKind::ShortName) &&
                 "twice"sv == Origin.getName(llvm::DINameKind::ShortName);
      }
    }
  }
  EXPECT_TRUE(Found);
  removeOutput(SO);
}

TEST(DebugInfoAOT, InlinedCallKeepsCallFile) {
  auto SO = compileToSO(Configure{}, "dwarf_inline_O2.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  std::vector<std::pair<uint64_t, std::string>> Calls;
  for (const auto &CU : D.Ctx->compile_units()) {
    auto *LT = D.Ctx->getLineTableForUnit(CU.get());
    if (!LT) {
      continue;
    }
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() != llvm::dwarf::DW_TAG_inlined_subroutine) {
        continue;
      }
      auto File =
          llvm::dwarf::toUnsigned(Die.find(llvm::dwarf::DW_AT_call_file));
      std::string Path;
      if (File &&
          LT->getFileNameByIndex(
              *File, CU->getCompilationDir(),
              llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath,
              Path)) {
        const auto Slash = Path.find_last_of("/\\");
        Calls.emplace_back(
            llvm::dwarf::toUnsigned(Die.find(llvm::dwarf::DW_AT_call_line), 0),
            Slash == std::string::npos ? Path : Path.substr(Slash + 1));
      }
    }
  }
  auto Has = [&Calls](uint64_t Line, std::string_view File) {
    return std::find(Calls.begin(), Calls.end(),
                     std::make_pair(Line, std::string(File))) != Calls.end();
  };
  EXPECT_TRUE(Has(7, "dwarf_inline.c"sv));
  EXPECT_TRUE(Has(4, "dwarf_inline_body.inc"sv));
  EXPECT_FALSE(Has(4, "dwarf_inline.c"sv));
  removeOutput(SO);
}

TEST(DebugInfoAOT, InlinedFunctionStaysInItsUnit) {
  auto SO = compileToSO(Configure{}, "dwarf_units_O2.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  uint32_t Found = 0;
  for (const auto &CU : D.Ctx->compile_units()) {
    const char *UnitName =
        CU->getUnitDIE().getName(llvm::DINameKind::ShortName);
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      const char *Name = Die.getName(llvm::DINameKind::ShortName);
      if (Die.getTag() != llvm::dwarf::DW_TAG_subprogram || !Name ||
          "helper"sv != Name) {
        continue;
      }
      ++Found;
      ASSERT_NE(UnitName, nullptr);
      const std::string_view Unit(UnitName);
      EXPECT_TRUE(Unit.size() >= "dwarf_units_helper.c"sv.size() &&
                  Unit.substr(Unit.size() - "dwarf_units_helper.c"sv.size()) ==
                      "dwarf_units_helper.c"sv)
          << Unit;
    }
  }
  EXPECT_GT(Found, 0U);
  removeOutput(SO);
}

/// Rewrites the low_pc and the high_pc size of named subprograms.
bool setSubprogramRanges(
    AST::Module &Mod,
    const std::map<std::string, std::pair<uint32_t, uint32_t>> &Ranges) {
  std::vector<std::pair<uint64_t, uint32_t>> Patches;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(Mod);
    if (!R) {
      return false;
    }
    for (const auto &CU : R->getContext().compile_units()) {
      for (const auto &Entry : CU->dies()) {
        llvm::DWARFDie Die(CU.get(), &Entry);
        const char *Name = Die.getName(llvm::DINameKind::ShortName);
        if (Die.getTag() != llvm::dwarf::DW_TAG_subprogram || !Name) {
          continue;
        }
        auto It = Ranges.find(Name);
        if (It == Ranges.end()) {
          continue;
        }
        const auto [Low, High] = It->second;
        for (const auto &A : Die.attributes()) {
          if (A.Attr == llvm::dwarf::DW_AT_low_pc && A.ByteSize == 4) {
            Patches.emplace_back(A.Offset, Low);
          } else if (A.Attr == llvm::dwarf::DW_AT_high_pc && A.ByteSize == 4 &&
                     A.Value.getForm() == llvm::dwarf::DW_FORM_data4) {
            Patches.emplace_back(A.Offset, High - Low);
          }
        }
      }
    }
  }
  for (auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != ".debug_info"sv) {
      continue;
    }
    auto &Bytes = Custom.getContent();
    for (const auto &[Offset, Value] : Patches) {
      for (uint32_t I = 0; I < 4; ++I) {
        Bytes[Offset + I] = static_cast<uint8_t>(Value >> (I * 8));
      }
    }
  }
  return Patches.size() == Ranges.size() * 2;
}

TEST(DebugInfoAOT, SubprogramOverTwoBodiesKeepsOtherFunctions) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  std::map<std::string, std::pair<uint32_t, uint32_t>> Old;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
      auto Die = R->getSubprogram(I);
      if (!Die) {
        continue;
      }
      auto Ranges = Die.getAddressRanges();
      ASSERT_TRUE(static_cast<bool>(Ranges));
      ASSERT_EQ(Ranges->size(), 1U);
      Old[Die.getName(llvm::DINameKind::ShortName)] = {
          static_cast<uint32_t>(Ranges->front().LowPC),
          static_cast<uint32_t>(Ranges->front().HighPC)};
    }
  }
  ASSERT_EQ(Old.count("sumList"), 1U);
  ASSERT_EQ(Old.count("fact"), 1U);
  ASSERT_LT(Old["sumList"].second, Old["fact"].first);
  ASSERT_TRUE(setSubprogramRanges(
      *Mod, {{"sumList", {Old["sumList"].first, Old["fact"].second}},
             {"fact", {0, Old["fact"].second}}}));
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    uint32_t Bodies = 0;
    for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
      if (auto Die = R->getSubprogram(I);
          Die && Die.getName(llvm::DINameKind::ShortName) == "sumList"sv) {
        ++Bodies;
      }
    }
    ASSERT_EQ(Bodies, 2U);
  }
  const auto SO = compileModuleToSO(Conf, *Mod, true,
                                    CompilerConfigure::OptimizationLevel::O0,
                                    tempOutput("shared-subprogram"sv));
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  EXPECT_TRUE(D.hasSubprogram("run"sv));
  EXPECT_TRUE(D.hasLine("dwarf_basic.c"sv, markerLine("BREAK_RUN"sv)));
  EXPECT_TRUE(D.hasLine("dwarf_basic.c"sv, markerLine("BREAK_SUMLIST"sv)));
  removeOutput(SO);
}

TEST(DebugInfoAOT, RecompileRemovesOnlyTheReplacedObject) {
  const auto SO = tempOutput("recompile"sv);
  std::vector<std::filesystem::path> Neighbors;
  for (auto Suffix : {".0123456789.o"sv, ".o"sv}) {
    auto P = SO;
    P.replace_extension();
    P += Suffix;
    std::ofstream(P) << "user";
    Neighbors.push_back(std::move(P));
  }
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  const auto First = debugFileOf(SO);
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  const auto Second = debugFileOf(SO);
  if (First != SO) {
    EXPECT_NE(First, Second);
    EXPECT_FALSE(std::filesystem::exists(First));
    EXPECT_TRUE(std::filesystem::exists(Second));
  }
  std::error_code Error;
  for (const auto &P : Neighbors) {
    EXPECT_TRUE(std::filesystem::exists(P)) << P;
    std::filesystem::remove(P, Error);
  }
  removeOutput(SO);
}

TEST(DebugInfoAOT, RecompileWithoutDebugInfoRemovesTheObject) {
  const auto SO = tempOutput("recompile-nodebug"sv);
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  const auto First = debugFileOf(SO);
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, false, Level, SO),
            SO);
  EXPECT_EQ(debugFileOf(SO), SO);
  if (First != SO) {
    EXPECT_FALSE(std::filesystem::exists(First));
  }
  removeOutput(SO);
}

TEST(DebugInfoAOT, FailedRebuildWithoutDebugInfoKeepsTheEarlierBuild) {
#if WASMEDGE_OS_WINDOWS
  GTEST_SKIP() << "directory permissions do not stop the writes here";
#else
  auto Dir = tempOutput("readonly"sv);
  Dir += ".d";
  std::filesystem::create_directories(Dir);
  const auto SO = Dir / ("library" WASMEDGE_LIB_EXTENSION);
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  auto readAll = [](const std::filesystem::path &P) {
    std::ifstream In(P, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(In), {});
  };
  auto listDir = [&Dir]() {
    std::vector<std::filesystem::path> Names;
    for (const auto &E : std::filesystem::directory_iterator(Dir)) {
      Names.push_back(E.path().filename());
    }
    std::sort(Names.begin(), Names.end());
    return Names;
  };
  const auto Library = readAll(SO);
  const auto Before = listDir();
  std::filesystem::permissions(Dir,
                               std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_exec,
                               std::filesystem::perm_options::replace);
  const bool Writable = static_cast<bool>(std::ofstream(Dir / "probe"));
  if (!Writable) {
    EXPECT_TRUE(
        compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, false, Level, SO)
            .empty());
  }
  std::filesystem::permissions(Dir, std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace);
  std::error_code Error;
  if (Writable) {
    std::filesystem::remove_all(Dir, Error);
    GTEST_SKIP() << "the user can write to a read-only directory";
  }
  EXPECT_EQ(readAll(SO), Library);
  EXPECT_EQ(listDir(), Before);
  std::filesystem::remove_all(Dir, Error);
#endif
}

TEST(DebugInfoAOT, RecompileKeepsAnObjectOfAnotherTool) {
  const auto SO = tempOutput("recompile-foreign"sv);
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  const auto First = debugFileOf(SO);
  if (First == SO) {
    removeOutput(SO);
    GTEST_SKIP() << "the library keeps its own debug info";
  }
  auto Foreign = SO;
  Foreign += ".userobject.o";
  std::string Bytes;
  {
    std::ifstream In(SO, std::ios::binary);
    Bytes.assign(std::istreambuf_iterator<char>(In), {});
  }
  const auto At = Bytes.find(First.string());
  ASSERT_NE(At, std::string::npos);
  Bytes.replace(At, First.string().size(), Foreign.string());
  std::ofstream(SO, std::ios::binary | std::ios::trunc) << Bytes;
  std::filesystem::rename(First, Foreign);
  ASSERT_EQ(debugFileOf(SO), Foreign);
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  EXPECT_TRUE(std::filesystem::exists(Foreign));
  std::error_code Error;
  std::filesystem::remove(Foreign, Error);
  removeOutput(SO);
}

TEST(DebugInfoAOT, RecompileKeepsAnObjectChangedAfterTheLink) {
  const auto SO = tempOutput("recompile-changed"sv);
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  const auto First = debugFileOf(SO);
  if (First == SO) {
    removeOutput(SO);
    GTEST_SKIP() << "the library keeps its own debug info";
  }
  std::filesystem::last_write_time(
      First, std::filesystem::last_write_time(First) - std::chrono::hours(1));
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  EXPECT_NE(debugFileOf(SO), First);
  EXPECT_TRUE(std::filesystem::exists(First));
  std::error_code Error;
  std::filesystem::remove(First, Error);
  removeOutput(SO);
}

TEST(DebugInfoAOT, PercentInTheLibraryPathStays) {
  const auto SO = tempOutput("percent%name"sv);
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  const auto First = debugFileOf(SO);
  if (First == SO) {
    removeOutput(SO);
    GTEST_SKIP() << "the library keeps its own debug info";
  }
  const auto Prefix = SO.stem().u8string() + ".";
  EXPECT_EQ(First.parent_path(), SO.parent_path());
  EXPECT_EQ(First.filename().u8string().compare(0, Prefix.size(), Prefix), 0)
      << First;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  EXPECT_FALSE(std::filesystem::exists(First));
  removeOutput(SO);
}

TEST(DebugInfoAOT, LongLibraryNameStillLinks) {
#if WASMEDGE_OS_MACOS
  const auto SO =
      outputDirectory() / (std::string(239, 'n') + WASMEDGE_LIB_EXTENSION);
  for (const bool Debug : {false, true}) {
    EXPECT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, Debug,
                          CompilerConfigure::OptimizationLevel::O0, SO),
              SO)
        << Debug;
  }
  removeOutput(SO);
#else
  GTEST_SKIP() << "only macOS links at a temporary path";
#endif
}

TEST(DebugInfoAOT, ConcurrentRecompilesKeepOnlyTheLastObject) {
  const auto SO = tempOutput("recompile-concurrent"sv);
  const auto Level = CompilerConfigure::OptimizationLevel::O0;
  ASSERT_EQ(compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO),
            SO);
  std::atomic<uint32_t> Built = 0;
  std::vector<std::thread> Threads;
  for (uint32_t I = 0; I < 4; ++I) {
    Threads.emplace_back([&] {
      if (compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true, Level, SO) ==
          SO) {
        ++Built;
      }
    });
  }
  for (auto &T : Threads) {
    T.join();
  }
  EXPECT_EQ(Built.load(), 4U);
  const auto Prefix = SO.stem().u8string() + ".";
  std::vector<std::string> Objects;
  for (const auto &E : std::filesystem::directory_iterator(SO.parent_path())) {
    const auto Name = E.path().filename().u8string();
    if (Name.size() == Prefix.size() + "0123456789.o"sv.size() &&
        Name.compare(0, Prefix.size(), Prefix) == 0 &&
        Name.compare(Name.size() - 2, 2, ".o") == 0) {
      Objects.push_back(Name);
    }
  }
  const auto Kept = debugFileOf(SO);
  if (Kept == SO) {
    EXPECT_TRUE(Objects.empty());
  } else {
    EXPECT_EQ(Objects, std::vector<std::string>{Kept.filename().u8string()});
  }
  removeOutput(SO);
}

TEST(DebugInfoAOT, OffMeansNoDebugSections) {
  auto SO = compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, false,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  EXPECT_FALSE(D.open(SO));
  removeOutput(SO);
}

TEST(DebugInfoAOT, NoDwarfModuleHasIdenticalSections) {
  const auto Out = tempOutput("nodebug"sv);
  auto Sections = [&Out](bool Debug) {
    std::vector<std::pair<std::string, std::string>> Result;
    if (!compileToSO(Configure{}, "dwarf_basic_nodebug.wasm"sv, Debug,
                     CompilerConfigure::OptimizationLevel::O2, Out)
             .empty()) {
      Result = sectionContents(Out);
    }
    removeOutput(Out);
    return Result;
  };
  const auto Off = Sections(false);
  const auto On = Sections(true);
  ASSERT_FALSE(Off.empty());
  EXPECT_EQ(Off, On);
}

TEST(DebugInfoAOT, CorruptDwarfStillCompilesAndRuns) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  for (auto &Custom : Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      auto &Bytes = Custom.getContent();
      Bytes.resize(Bytes.size() / 2);
    }
  }
  const auto SO = compileModuleToSO(
      Conf, *Mod, true, CompilerConfigure::OptimizationLevel::O0,
      tempOutput("corrupt"sv), readHexFixture("dwarf_basic_O0.wasm"sv));
  ASSERT_FALSE(SO.empty());
  {
    Configure RunConf;
    RunConf.getRuntimeConfigure().setRunMode(RunMode::AOT);
    VM::VM Machine(RunConf);
    ASSERT_TRUE(Machine.loadWasm(SO));
    ASSERT_TRUE(Machine.validate());
    ASSERT_TRUE(Machine.instantiate());
    auto R = Machine.execute("run"sv);
    ASSERT_TRUE(R);
    EXPECT_EQ((*R)[0].first.get<uint32_t>(), 43U);
  }
  removeOutput(SO);
}

} // namespace
