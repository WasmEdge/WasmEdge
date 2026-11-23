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

#if LLVM_VERSION_MAJOR < 12
#else
/// The native addresses of the rows for Line, in address order.
std::vector<uint64_t> lineAddresses(NativeDwarf &D, uint32_t Line) {
  std::vector<uint64_t> Result;
  for (const auto &CU : D.Ctx->compile_units()) {
    if (auto *LT = D.Ctx->getLineTableForUnit(CU.get())) {
      for (const auto &Row : LT->Rows) {
        if (Row.Line == Line && !Row.EndSequence) {
          Result.push_back(Row.Address.Address);
        }
      }
    }
  }
  std::sort(Result.begin(), Result.end());
  return Result;
}

/// True when a bounded entry with a location expression covers Address.
bool coversAddress(const llvm::DWARFLocationExpressionsVector &Locations,
                   uint64_t Address) {
  return std::any_of(Locations.begin(), Locations.end(),
                     [Address](const llvm::DWARFLocationExpression &L) {
                       return L.Range && L.Range->LowPC <= Address &&
                              Address < L.Range->HighPC && !L.Expr.empty();
                     });
}

/// The value of a constant location expression: DW_OP_lit, DW_OP_constu, or
/// DW_OP_consts, then DW_OP_stack_value.
std::optional<uint64_t> constantOf(llvm::ArrayRef<uint8_t> Expr) {
  namespace dwarf = llvm::dwarf;
  if (Expr.size() < 2 || Expr.back() != dwarf::DW_OP_stack_value) {
    return std::nullopt;
  }
  const auto Op = Expr.front();
  if (Op >= dwarf::DW_OP_lit0 && Op <= dwarf::DW_OP_lit31) {
    return Expr.size() == 2 ? std::optional<uint64_t>(Op - dwarf::DW_OP_lit0)
                            : std::nullopt;
  }
  if (Op != dwarf::DW_OP_constu && Op != dwarf::DW_OP_consts) {
    return std::nullopt;
  }
  unsigned Size = 0;
  const char *Error = nullptr;
  const auto *Begin = Expr.data() + 1;
  const auto *End = Expr.data() + Expr.size() - 1;
  const uint64_t Value = Op == dwarf::DW_OP_constu
                             ? llvm::decodeULEB128(Begin, &Size, End, &Error)
                             : static_cast<uint64_t>(llvm::decodeSLEB128(
                                   Begin, &Size, End, &Error));
  if (Error || Begin + Size != End) {
    return std::nullopt;
  }
  return Value;
}
#endif

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

TEST(DebugInfoAOT, FunctionWithoutDwarfGetsArtificialSubprogram) {
  for (auto Level : {CompilerConfigure::OptimizationLevel::O0,
                     CompilerConfigure::OptimizationLevel::O2}) {
    for (auto Name : {"dwarf_basic_O0.wasm"sv, "dwarf_basic_O2.wasm"sv}) {
      auto SO = compileToSO(Configure{}, Name, true, Level);
      ASSERT_FALSE(SO.empty());
      NativeDwarf D;
      ASSERT_TRUE(D.open(SO));
      EXPECT_TRUE(D.hasSubprogram("run"sv)) << Name;
      bool Artificial = false;
      for (const auto &CU : D.Ctx->compile_units()) {
        for (const auto &Entry : CU->dies()) {
          llvm::DWARFDie Die(CU.get(), &Entry);
          if (Die.getTag() == llvm::dwarf::DW_TAG_subprogram &&
              Die.getName(llvm::DINameKind::ShortName) &&
              "plain"sv == Die.getName(llvm::DINameKind::ShortName)) {
            Artificial =
                static_cast<bool>(Die.find(llvm::dwarf::DW_AT_artificial));
          }
        }
      }
      EXPECT_TRUE(Artificial) << Name;
      removeOutput(SO);
    }
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

TEST(DebugInfoAOT, CxxFunctionsKeepTheirScopes) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  namespace dwarf = llvm::dwarf;
  auto SO = compileToSO(Configure{}, "dwarf_scopes_O0.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  auto NameOf = [](llvm::DWARFDie Die) {
    const char *N = Die ? Die.getName(llvm::DINameKind::ShortName) : nullptr;
    return std::string(N ? N : "");
  };
  std::set<std::string> RunScopes;
  bool MethodInClass = false;
  bool ScopedEnum = false;
  bool ThisIsArtificial = false;
  for (const auto &CU : D.Ctx->compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      const auto Name = NameOf(Die);
      const auto Parent = Die.getParent();
      if (Die.getTag() == dwarf::DW_TAG_subprogram && Name == "run" &&
          Parent.getTag() == dwarf::DW_TAG_namespace) {
        RunScopes.insert(NameOf(Parent));
      }
      if (Die.getTag() == dwarf::DW_TAG_subprogram && Name == "add" &&
          Parent.getTag() == dwarf::DW_TAG_structure_type &&
          NameOf(Parent) == "Counter") {
        MethodInClass = true;
      }
      if (Die.getTag() == dwarf::DW_TAG_enumeration_type && Name == "Color") {
        ScopedEnum = dwarf::toUnsigned(Die.find(dwarf::DW_AT_enum_class), 0);
      }
      if (Die.getTag() == dwarf::DW_TAG_subprogram && Name == "add" &&
          Die.find(dwarf::DW_AT_low_pc)) {
        const auto This =
            Die.getAttributeValueAsReferencedDie(dwarf::DW_AT_object_pointer);
        ThisIsArtificial =
            This && NameOf(This) == "this" &&
            dwarf::toUnsigned(This.find(dwarf::DW_AT_artificial), 0);
      }
    }
  }
  EXPECT_EQ(RunScopes, (std::set<std::string>{"a", "b"}));
  EXPECT_TRUE(MethodInClass);
  EXPECT_TRUE(ScopedEnum);
  EXPECT_TRUE(ThisIsArtificial);
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

TEST(DebugInfoAOT, OutOfLineCopyOfInlinedFunctionKeepsTypes) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  auto SO = compileToSO(Configure{}, "dwarf_inline_O2.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  auto TypeName = [](llvm::DWARFDie Die) -> std::string_view {
    while (Die) {
      if (auto T =
              Die.getAttributeValueAsReferencedDie(llvm::dwarf::DW_AT_type)) {
        const char *Name = T.getName(llvm::DINameKind::ShortName);
        return Name ? Name : ""sv;
      }
      Die = Die.getAttributeValueAsReferencedDie(
          llvm::dwarf::DW_AT_abstract_origin);
    }
    return {};
  };
  uint32_t Checked = 0;
  for (const auto &CU : D.Ctx->compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() != llvm::dwarf::DW_TAG_subprogram ||
          !Die.find(llvm::dwarf::DW_AT_low_pc) ||
          !Die.getName(llvm::DINameKind::ShortName) ||
          "half"sv != Die.getName(llvm::DINameKind::ShortName)) {
        continue;
      }
      EXPECT_EQ(TypeName(Die), "int"sv);
      for (auto C : Die.children()) {
        if (C.getTag() == llvm::dwarf::DW_TAG_formal_parameter) {
          EXPECT_EQ(TypeName(C), "int"sv);
          ++Checked;
        }
      }
    }
  }
  EXPECT_EQ(Checked, 1U);
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

TEST(DebugInfoAOT, SelfReferentialTypeStillCompiles) {
  Configure Conf;
  Conf.getCompilerConfigure().setDebugInfo(true);
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  uint64_t ArrayOffset = 0;
  uint64_t AttrOffset = 0;
  uint32_t SelfRef = 0;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    for (const auto &CU : R->getContext().compile_units()) {
      for (const auto &Entry : CU->dies()) {
        llvm::DWARFDie D(CU.get(), &Entry);
        if (AttrOffset != 0 || D.getTag() != llvm::dwarf::DW_TAG_array_type) {
          continue;
        }
        for (const auto &A : D.attributes()) {
          if (A.Attr == llvm::dwarf::DW_AT_type &&
              A.Value.getForm() == llvm::dwarf::DW_FORM_ref4) {
            ArrayOffset = D.getOffset();
            AttrOffset = A.Offset;
            SelfRef = static_cast<uint32_t>(D.getOffset() - CU->getOffset());
          }
        }
      }
    }
  }
  ASSERT_NE(AttrOffset, 0U);
  for (auto &Custom : Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      auto &Bytes = Custom.getContent();
      ASSERT_LE(AttrOffset + 4, Bytes.size());
      for (uint32_t I = 0; I < 4; ++I) {
        Bytes[AttrOffset + I] = static_cast<Byte>(SelfRef >> (I * 8));
      }
    }
  }
  llvm::LLVMContext Ctx;
  llvm::Module M("t", Ctx);
  auto DI = LLVM::DebugInfo::ModuleDebugInfo::create(
      *Mod, llvm::wrap(&M), LLVM::DebugInfo::ModuleDebugInfo::Part::All);
  ASSERT_NE(DI, nullptr);
  EXPECT_NE(DI->translateType(ArrayOffset), nullptr);
  DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(M, &llvm::errs()));
  const auto SO = compileModuleToSO(Conf, *Mod, true,
                                    CompilerConfigure::OptimizationLevel::O0,
                                    tempOutput("self-referential"sv));
  EXPECT_FALSE(SO.empty());
  removeOutput(SO);
}

TEST(DebugInfoAOT, UnrepresentableVariantsAreOmitted) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  namespace dwarf = llvm::dwarf;
  for (const bool DiscrList : {false, true}) {
    Configure Conf;
    auto Mod = loadFixture(Conf, "dwarf_enum_O0.wasm"sv);
    ASSERT_NE(Mod, nullptr);
    uint64_t Code = 0;
    {
      auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
      ASSERT_NE(R, nullptr);
      auto Tagged =
          findType(R->getContext(), dwarf::DW_TAG_structure_type, "Tagged"sv);
      ASSERT_TRUE(Tagged);
      for (auto Part : Tagged.children()) {
        if (Part.getTag() != dwarf::DW_TAG_variant_part) {
          continue;
        }
        auto Target =
            DiscrList
                ? Part.getFirstChild().getSibling()
                : Part.getAttributeValueAsReferencedDie(dwarf::DW_AT_discr);
        ASSERT_TRUE(Target);
        Code = Target.getAbbreviationDeclarationPtr()->getCode();
      }
    }
    ASSERT_NE(Code, 0U);
    ASSERT_GT(
        renameAbbrevAttribute(
            *Mod, [Code](uint64_t C, uint64_t) { return C == Code; },
            DiscrList ? dwarf::DW_AT_discr_value : dwarf::DW_AT_type,
            DiscrList ? dwarf::DW_AT_discr_list : dwarf::DW_AT_description),
        0U);
    const auto SO = compileModuleToSO(
        Conf, *Mod, true, CompilerConfigure::OptimizationLevel::O0,
        tempOutput(DiscrList ? "discr-list"sv : "no-discr"sv));
    ASSERT_FALSE(SO.empty()) << DiscrList;
    NativeDwarf D;
    ASSERT_TRUE(D.open(SO));
    auto Native = findType(*D.Ctx, dwarf::DW_TAG_structure_type, "Tagged"sv);
    ASSERT_TRUE(Native) << DiscrList;
    for (auto Part : Native.children()) {
      EXPECT_NE(Part.getTag(), dwarf::DW_TAG_variant_part) << DiscrList;
    }
    removeOutput(SO);
  }
}

llvm::DWARFDie findVariable(llvm::DWARFContext &Ctx, std::string_view Fn,
                            std::string_view Var) {
  for (const auto &CU : Ctx.compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie D(CU.get(), &Entry);
      const auto Tag = D.getTag();
      if ((Tag != llvm::dwarf::DW_TAG_variable &&
           Tag != llvm::dwarf::DW_TAG_formal_parameter) ||
          !D.getName(llvm::DINameKind::ShortName) ||
          Var != D.getName(llvm::DINameKind::ShortName)) {
        continue;
      }
      auto P = D.getParent();
      while (P && P.getTag() == llvm::dwarf::DW_TAG_lexical_block) {
        P = P.getParent();
      }
      const bool InFn = P && P.getName(llvm::DINameKind::ShortName) &&
                        Fn == P.getName(llvm::DINameKind::ShortName);
      const bool Global =
          Fn.empty() && P && P.getTag() == llvm::dwarf::DW_TAG_compile_unit;
      if (InFn || Global) {
        return D;
      }
    }
  }
  return {};
}

const llvm::Value *fakeUseOperand(const llvm::Instruction &I) {
  const auto *Call = llvm::dyn_cast<llvm::CallInst>(&I);
  if (!Call || Call->arg_size() == 0) {
    return nullptr;
  }
#if LLVM_VERSION_MAJOR < 20
#else
  if (Call->getIntrinsicID() == llvm::Intrinsic::fake_use) {
    return Call->getArgOperand(0);
  }
#endif
  if (const auto *IA =
          llvm::dyn_cast<llvm::InlineAsm>(Call->getCalledOperand());
      IA && IA->getAsmString().empty()) {
    return Call->getArgOperand(0);
  }
  return nullptr;
}

TEST(DebugInfoAOT, OgKeepsVariableLocations) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  auto SO = compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::Og);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  for (auto [Fn, Var] :
       {std::pair{"sumList"sv, "Head"sv}, std::pair{"sumList"sv, "Total"sv},
        std::pair{"sumList"sv, "N"sv},
        std::pair{"sumList"sv, "__wasm_memory"sv},
        std::pair{""sv, "Counter"sv}}) {
    auto V = findVariable(*D.Ctx, Fn, Var);
    ASSERT_TRUE(V) << Fn << "::" << Var;
    EXPECT_TRUE(V.find(llvm::dwarf::DW_AT_location)) << Fn << "::" << Var;
  }

  const auto Line = markerLine("BREAK_SUMLIST"sv);
  std::vector<uint64_t> Addresses;
  for (const auto &CU : D.Ctx->compile_units()) {
    if (auto *LT = D.Ctx->getLineTableForUnit(CU.get())) {
      for (const auto &Row : LT->Rows) {
        if (Row.Line == Line && !Row.EndSequence) {
          Addresses.push_back(Row.Address.Address);
        }
      }
    }
  }
  ASSERT_FALSE(Addresses.empty());
  auto Total = findVariable(*D.Ctx, "sumList"sv, "Total"sv);
  ASSERT_TRUE(Total);
  bool Covered = false;
#if LLVM_VERSION_MAJOR < 10
  auto Attr = Total.find(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(Attr);
  if (Attr->getAsBlock()) {
    Covered = true;
  } else if (auto Offset = Attr->getAsSectionOffset()) {
    const auto *List = D.Ctx->getDebugLoc()->getLocationListAtOffset(*Offset);
    ASSERT_NE(List, nullptr);
    const auto Base = Total.getDwarfUnit()->getBaseAddress();
    const uint64_t BaseAddress = Base ? Base->Address : 0;
    for (const auto &E : List->Entries) {
      for (const auto Address : Addresses) {
        Covered |=
            BaseAddress + E.Begin <= Address && Address < BaseAddress + E.End;
      }
    }
  }
#else
  auto Locations = Total.getLocations(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(static_cast<bool>(Locations));
  for (const auto &Loc : *Locations) {
    for (const auto Address : Addresses) {
      Covered |= !Loc.Range ||
                 (Loc.Range->LowPC <= Address && Address < Loc.Range->HighPC);
    }
  }
#endif
  EXPECT_TRUE(Covered);
  removeOutput(SO);
}

TEST(DebugInfoAOT, OgKeepsBlockVariablesUntilScopeEnd) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
#if LLVM_VERSION_MAJOR < 19
  GTEST_SKIP() << "debug records need LLVM 19 or newer";
#else
  Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getCompilerConfigure().setOptimizationLevel(
      CompilerConfigure::OptimizationLevel::Og);
  Conf.getCompilerConfigure().setDebugInfo(true);
  auto Mod = loadFixture(Conf, "dwarf_scope_Og.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  LLVM::Compiler Compiler(Conf);
  auto Data = Compiler.compile(*Mod);
  ASSERT_TRUE(Data);
  auto &M = *llvm::unwrap(Data->extract().LLModule.unwrap());
  EXPECT_FALSE(llvm::verifyModule(M, &llvm::errs()));
  auto It = std::find_if(M.begin(), M.end(), [](const llvm::Function &F) {
    return F.getSubprogram() && F.getSubprogram()->getName() == "scoped";
  });
  ASSERT_NE(It, M.end());
  std::vector<const llvm::Value *> Steps;
  for (const auto &I : llvm::instructions(*It)) {
    for (const llvm::DbgVariableRecord &DVR :
         llvm::filterDbgVars(I.getDbgRecordRange())) {
      if (DVR.getVariable()->getName() == "Step") {
        for (const auto *V : DVR.location_ops()) {
          Steps.push_back(V);
        }
      }
    }
  }
  auto Returns = [](const llvm::BasicBlock &BB) {
    const auto *Term = BB.getTerminator();
    if (llvm::isa<llvm::ReturnInst>(Term)) {
      return true;
    }
#if LLVM_VERSION_MAJOR < 23
    const auto *Br = llvm::dyn_cast<llvm::BranchInst>(Term);
    return Br && Br->isUnconditional() &&
           llvm::isa<llvm::ReturnInst>(Br->getSuccessor(0)->getTerminator());
#else
    const auto *Br = llvm::dyn_cast<llvm::UncondBrInst>(Term);
    return Br &&
           llvm::isa<llvm::ReturnInst>(Br->getSuccessor()->getTerminator());
#endif
  };
  bool StepInScope = false;
  bool ArgAtRet = false;
  uint32_t MaxUsesAtRet = 0;
  for (const auto &BB : *It) {
    const bool AtRet = Returns(BB);
    uint32_t UsesAtRet = 0;
    for (const auto &I : BB) {
      const auto *V = fakeUseOperand(I);
      if (!V) {
        continue;
      }
      const bool IsStep =
          std::find(Steps.begin(), Steps.end(), V) != Steps.end();
      StepInScope |= IsStep && !AtRet;
      if (AtRet) {
        ++UsesAtRet;
        ArgAtRet |= llvm::isa<llvm::Argument>(V);
      }
    }
    MaxUsesAtRet = std::max(MaxUsesAtRet, UsesAtRet);
  }
  EXPECT_TRUE(StepInScope);
  EXPECT_TRUE(ArgAtRet);
  EXPECT_EQ(MaxUsesAtRet, 1U);
#endif
}

TEST(DebugInfoAOT, OgPlacesFakeUsesAtScopeExits) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_scope_Og.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  auto Step = findVariable(R->getContext(), "scoped"sv, "Step"sv);
  ASSERT_TRUE(Step);
  auto Ranges = Step.getParent().getAddressRanges();
  ASSERT_TRUE(static_cast<bool>(Ranges));
  ASSERT_EQ(Ranges->size(), 1U);
  const auto Low = Ranges->front().LowPC;
  const auto High = Ranges->front().HighPC;
  const auto &Seg = Mod->getCodeSection().getContent()[1];

  llvm::LLVMContext Ctx;
  llvm::Module M("t", Ctx);
  auto DI = LLVM::DebugInfo::ModuleDebugInfo::create(
      *Mod, llvm::wrap(&M), LLVM::DebugInfo::ModuleDebugInfo::Part::All);
  ASSERT_NE(DI, nullptr);
  auto *I32 = llvm::Type::getInt32Ty(Ctx);
  auto *Fn = llvm::Function::Create(llvm::FunctionType::get(I32, {}, false),
                                    llvm::Function::ExternalLinkage, "f", M);
  auto FI = DI->beginFunction(1, 1, llvm::wrap(Fn), true);
  ASSERT_NE(FI, nullptr);
  auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Fn);
  llvm::IRBuilder<> B(Entry);
  std::vector<LLVMValueRef> Locals;
  for (uint32_t I = 0; I < 5; ++I) {
    Locals.push_back(llvm::wrap(B.CreateAlloca(I32)));
  }
  auto *Mark = B.CreateAlloca(I32);
  FI->bindFrame(Locals, nullptr, {}, false);
  auto *Body = llvm::BasicBlock::Create(Ctx, "body", Fn);
  B.CreateBr(Body);
  B.SetInsertPoint(Body);
  llvm::Instruction *EarlyRet = nullptr;
  llvm::Instruction *EndMark = nullptr;
  for (const auto &Instr : Seg.getExpr().getInstrs()) {
    FI->setLocation(llvm::wrap(&B), Instr.getOffset());
    const auto Addr = R->toAddress(Instr.getOffset());
    auto *Store = B.CreateStore(B.getInt32(Instr.getOffset()), Mark, true);
    if (!EndMark && Addr >= High) {
      EndMark = Store;
    }
    if (!EarlyRet && Low <= Addr && Addr < High) {
      auto *Early = llvm::BasicBlock::Create(Ctx, "early", Fn);
      auto *Next = llvm::BasicBlock::Create(Ctx, "next", Fn);
      B.CreateCondBr(B.CreateLoad(B.getInt1Ty(), Mark), Early, Next);
      B.SetInsertPoint(Early);
      EarlyRet = B.CreateRet(B.getInt32(0));
      B.SetInsertPoint(Next);
    }
  }
  auto *FinalRet = B.CreateRet(B.getInt32(0));
  ASSERT_NE(EarlyRet, nullptr);
  ASSERT_NE(EndMark, nullptr);
  FI->finish(llvm::wrap(Entry));
  DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(M, &llvm::errs()));

  std::vector<std::pair<const llvm::Instruction *, LLVMValueRef>> Uses;
  for (const auto &I : llvm::instructions(*Fn)) {
    const auto *Op = fakeUseOperand(I);
    if (!Op) {
      continue;
    }
    EXPECT_FALSE(I.getDebugLoc());
    const auto *Next = I.getNextNode();
    while (llvm::isa<llvm::LoadInst>(Next) ||
           llvm::isa<llvm::IntrinsicInst>(Next) || fakeUseOperand(*Next)) {
      Next = Next->getNextNode();
    }
    const auto *Load = llvm::cast<llvm::LoadInst>(Op);
    Uses.emplace_back(Next, llvm::wrap(Load->getPointerOperand()));
  }
  auto Has = [&](const llvm::Instruction *At, uint32_t Local) {
    return std::count(Uses.begin(), Uses.end(), std::pair{At, Locals[Local]});
  };
  EXPECT_EQ(Has(EndMark, 2), 1);
  EXPECT_EQ(Has(EndMark, 0), 0);
  EXPECT_EQ(Has(EarlyRet, 2), 1);
  EXPECT_EQ(Has(EarlyRet, 0), 1);
  EXPECT_EQ(Has(FinalRet, 0), 1);
  EXPECT_EQ(Has(FinalRet, 2), 0);
  EXPECT_EQ(Uses.size(), 4U);
}

/// Sets the DW_OP_addr operand of a global variable.
bool setGlobalAddress(AST::Module &Mod, std::string_view Var,
                      uint32_t Address) {
  uint64_t At = 0;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(Mod);
    if (!R) {
      return false;
    }
    auto Die = findVariable(R->getContext(), ""sv, Var);
    if (!Die) {
      return false;
    }
    for (const auto &A : Die.attributes()) {
      if (A.Attr == llvm::dwarf::DW_AT_location &&
          A.Value.getForm() == llvm::dwarf::DW_FORM_exprloc &&
          A.ByteSize == 6) {
        At = A.Offset + 2;
      }
    }
  }
  for (auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != ".debug_info"sv || At == 0) {
      continue;
    }
    auto &Bytes = Custom.getContent();
    if (Bytes[At - 1] != llvm::dwarf::DW_OP_addr) {
      return false;
    }
    for (uint32_t I = 0; I < 4; ++I) {
      Bytes[At + I] = static_cast<uint8_t>(Address >> (I * 8));
    }
    return true;
  }
  return false;
}

TEST(DebugInfoAOT, GlobalAtAddressZeroKeepsItsLocation) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  for (const auto [Address, Live] :
       {std::pair{UINT32_C(0), true}, std::pair{UINT32_C(0xFFFFFFFF), false}}) {
    Configure Conf;
    auto Mod = loadFixture(Conf, "dwarf_data_O0.wasm"sv);
    ASSERT_NE(Mod, nullptr);
    ASSERT_TRUE(setGlobalAddress(*Mod, "Only"sv, Address));
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    EXPECT_EQ(R->hasGlobalVariables(), Live) << Address;
    const auto SO = compileModuleToSO(
        Conf, *Mod, true, CompilerConfigure::OptimizationLevel::O0,
        tempOutput(fmt::format("global-{}", Address)));
    ASSERT_FALSE(SO.empty());
    NativeDwarf D;
    ASSERT_TRUE(D.open(SO));
    auto Only = findVariable(*D.Ctx, ""sv, "Only"sv);
    EXPECT_EQ(Only && Only.find(llvm::dwarf::DW_AT_location), Live) << Address;
    removeOutput(SO);
  }
}

TEST(DebugInfoAOT, VariableWithoutALocationKeepsItsName) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  namespace dwarf = llvm::dwarf;
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  uint64_t Code = 0;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    for (const auto &CU : R->getContext().compile_units()) {
      for (const auto &Entry : CU->dies()) {
        llvm::DWARFDie Die(CU.get(), &Entry);
        const char *Name = Die.getName(llvm::DINameKind::ShortName);
        if (Die.getTag() == dwarf::DW_TAG_variable && Name &&
            "Total"sv == Name) {
          Code = Die.getAbbreviationDeclarationPtr()->getCode();
        }
      }
    }
  }
  ASSERT_NE(Code, 0U);
  ASSERT_GT(renameAbbrevAttribute(
                *Mod, [Code](uint64_t C, uint64_t) { return C == Code; },
                dwarf::DW_AT_location, dwarf::DW_AT_description),
            0U);
  const auto SO = compileModuleToSO(Conf, *Mod, true,
                                    CompilerConfigure::OptimizationLevel::O0,
                                    tempOutput("no-location"sv));
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  auto Total = findVariable(*D.Ctx, "sumList"sv, "Total"sv);
  ASSERT_TRUE(Total);
  EXPECT_FALSE(Total.find(dwarf::DW_AT_location));
  EXPECT_TRUE(Total.find(dwarf::DW_AT_type));
  removeOutput(SO);
}

TEST(DebugInfoAOT, VariablesHaveLocations) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  auto SO = compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  for (auto [Fn, Var] :
       {std::pair{"sumList"sv, "Head"sv}, std::pair{"sumList"sv, "Total"sv},
        std::pair{"sumList"sv, "N"sv},
        std::pair{"sumList"sv, "__wasm_memory"sv}, std::pair{"fact"sv, "N"sv},
        std::pair{""sv, "Counter"sv}, std::pair{""sv, "Nodes"sv}}) {
    auto V = findVariable(*D.Ctx, Fn, Var);
    ASSERT_TRUE(V) << Fn << "::" << Var;
    EXPECT_TRUE(V.find(llvm::dwarf::DW_AT_location)) << Fn << "::" << Var;
  }
  removeOutput(SO);
}

TEST(DebugInfoAOT, LocationListKeepsRanges) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  auto SO = compileToSO(Configure{}, "dwarf_basic_O2.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  auto Head = findVariable(*D.Ctx, "sumList"sv, "Head"sv);
  ASSERT_TRUE(Head);
  auto Attr = Head.find(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(Attr);
  EXPECT_FALSE(Attr->getAsBlock());
#if LLVM_VERSION_MAJOR < 12
#else
  auto Locations = Head.getLocations(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(static_cast<bool>(Locations));
  const auto Live = lineAddresses(D, markerLine("struct Node *N = Head"sv));
  const auto Dead = lineAddresses(D, markerLine("return Total;"sv));
  ASSERT_FALSE(Live.empty());
  ASSERT_FALSE(Dead.empty());
  EXPECT_TRUE(coversAddress(*Locations, Live.front()))
      << std::hex << Live.front();
  for (const auto Address : Dead) {
    EXPECT_FALSE(coversAddress(*Locations, Address)) << std::hex << Address;
  }
  auto Total = findVariable(*D.Ctx, "sumList"sv, "Total"sv);
  ASSERT_TRUE(Total);
  auto TotalLocations = Total.getLocations(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(static_cast<bool>(TotalLocations));
  bool TotalIsZero = false;
  bool TotalInLocal = false;
  for (const auto &L : *TotalLocations) {
    if (const auto Value = constantOf(L.Expr)) {
      TotalIsZero |= *Value == 0;
    } else if (!L.Expr.empty()) {
      TotalInLocal = true;
    }
  }
  EXPECT_TRUE(TotalIsZero);
  EXPECT_TRUE(TotalInLocal);
  auto I = findVariable(*D.Ctx, "run"sv, "I"sv);
  ASSERT_TRUE(I);
  auto ILocations = I.getLocations(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(static_cast<bool>(ILocations));
  std::set<uint64_t> IValues;
  for (const auto &L : *ILocations) {
    if (const auto Value = constantOf(L.Expr)) {
      IValues.insert(*Value);
    }
  }
  EXPECT_EQ(IValues, (std::set<uint64_t>{1, 2, 3}));
#endif
  removeOutput(SO);
}

TEST(DebugInfoAOT, DebugSlotsKeepTheStackCheckOfASmallLeaf) {
  auto HasStackCheck = [](bool Debug) -> std::optional<bool> {
    Configure Conf;
    Conf.getCompilerConfigure().setOptimizationLevel(
        CompilerConfigure::OptimizationLevel::O0);
    Conf.getCompilerConfigure().setDebugInfo(Debug);
    Conf.getCompilerConfigure().setOutputFormat(
        CompilerConfigure::OutputFormat::Native);
    auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
    if (!Mod) {
      return std::nullopt;
    }
    LLVM::Compiler Compiler(Conf);
    auto Data = Compiler.compile(*Mod);
    if (!Data) {
      return std::nullopt;
    }
    auto *M = llvm::unwrap(Data->extract().LLModule.unwrap());
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    if (!R) {
      return std::nullopt;
    }
    const auto &Imports = Mod->getImportSection().getContent();
    const auto Imported = static_cast<uint32_t>(std::count_if(
        Imports.begin(), Imports.end(), [](const AST::ImportDesc &D) {
          return D.getExternalType() == ExternalType::Function;
        }));
    for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
      if (R->getFunctionName(I + Imported) != "sumList") {
        continue;
      }
      const auto *Fn = M->getFunction("f" + std::to_string(I + Imported));
      if (!Fn) {
        return std::nullopt;
      }
      for (const auto &B : *Fn) {
        for (const auto &Instr : B) {
          const auto *Call = llvm::dyn_cast<llvm::CallInst>(&Instr);
          const auto *Callee = Call ? Call->getCalledFunction() : nullptr;
          if (Callee && Callee->getName().substr(0, 14) == "llvm.stacksave") {
            return true;
          }
        }
      }
      return false;
    }
    return std::nullopt;
  };
  const auto Plain = HasStackCheck(false);
  const auto Debug = HasStackCheck(true);
  ASSERT_TRUE(Plain);
  ASSERT_TRUE(Debug);
  EXPECT_FALSE(*Plain);
  EXPECT_TRUE(*Debug);
}

TEST(DebugInfoAOT, UnitWithoutLineTableKeepsNamesAndTypes) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto &Customs = Mod->getCustomSections();
  Customs.erase(std::remove_if(Customs.begin(), Customs.end(),
                               [](const AST::CustomSection &C) {
                                 return C.getName() == ".debug_line"sv;
                               }),
                Customs.end());
  const auto SO = compileModuleToSO(
      Conf, *Mod, true, CompilerConfigure::OptimizationLevel::O0,
      tempOutput("nolines"sv), readHexFixture("dwarf_basic_O0.wasm"sv));
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  auto Head = findVariable(*D.Ctx, "sumList"sv, "Head"sv);
  ASSERT_TRUE(Head);
  auto Type = Head.getAttributeValueAsReferencedDie(llvm::dwarf::DW_AT_type);
  ASSERT_TRUE(Type);
  const char *TypeName = Type.getName(llvm::DINameKind::ShortName);
  EXPECT_STREQ(TypeName ? TypeName : "", "__wasm_ptr<Node>");
  removeOutput(SO);
}

/// Joins the first wasm local range of the variable with the entry after it,
/// so that the range also covers the start of the next path.
bool widenFirstLocalRange(AST::Module &Mod, std::string_view Fn,
                          std::string_view Var) {
  uint64_t ListOffset = 0;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(Mod);
    if (!R) {
      return false;
    }
    auto Die = findVariable(R->getContext(), Fn, Var);
    if (!Die) {
      return false;
    }
    auto Offset =
        llvm::dwarf::toSectionOffset(Die.find(llvm::dwarf::DW_AT_location));
    if (!Offset) {
      return false;
    }
    ListOffset = *Offset;
  }
  for (auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != ".debug_loc"sv) {
      continue;
    }
    auto &Bytes = Custom.getContent();
    auto read = [&](size_t At, size_t Size) {
      uint64_t Value = 0;
      for (size_t I = 0; I < Size; ++I) {
        Value |= static_cast<uint64_t>(Bytes[At + I]) << (I * 8);
      }
      return Value;
    };
    for (size_t At = ListOffset; At + 10 <= Bytes.size();) {
      const auto Begin = read(At, 4);
      const auto End = read(At + 4, 4);
      if (Begin == 0 && End == 0) {
        return false;
      }
      if (Begin == UINT32_MAX) {
        At += 8;
        continue;
      }
      const auto Size = read(At + 8, 2);
      const size_t Next = At + 10 + Size;
      const bool Local =
          Size >= 2 && Bytes[At + 10] == 0xed && Bytes[At + 11] == 0x00;
      if (Local && Next + 8 <= Bytes.size() && read(Next, 4) > End) {
        for (size_t I = 0; I < 4; ++I) {
          Bytes[At + 4 + I] = Bytes[Next + I];
        }
        return true;
      }
      At = Next;
    }
  }
  return false;
}

TEST(DebugInfoAOT, RangeReachesEveryPathThatEntersIt) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
#if LLVM_VERSION_MAJOR < 12
  GTEST_SKIP() << "LLVM 11 and older cannot list native locations";
#else
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_branch_O2.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  ASSERT_TRUE(widenFirstLocalRange(*Mod, "choose"sv, "R"sv));
  const auto SO = compileModuleToSO(Conf, *Mod, true,
                                    CompilerConfigure::OptimizationLevel::O0,
                                    tempOutput("branch"sv));
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  auto R = findVariable(*D.Ctx, "choose"sv, "R"sv);
  ASSERT_TRUE(R);
  auto Locations = R.getLocations(llvm::dwarf::DW_AT_location);
  ASSERT_TRUE(static_cast<bool>(Locations));
  const auto Then =
      lineAddresses(D, markerLine("THEN_PATH"sv, "dwarf_branch.c"sv));
  const auto Else =
      lineAddresses(D, markerLine("ELSE_PATH"sv, "dwarf_branch.c"sv));
  ASSERT_FALSE(Then.empty());
  ASSERT_FALSE(Else.empty());
  for (const auto Address : Then) {
    EXPECT_FALSE(coversAddress(*Locations, Address)) << std::hex << Address;
  }
  for (const auto Address : Else) {
    EXPECT_TRUE(coversAddress(*Locations, Address)) << std::hex << Address;
  }
  removeOutput(SO);
#endif
}

TEST(DebugInfoAOT, ConstantLocalsKeepTheirValue) {
  SKIP_IF_NATIVE_GETS_ONLY_LINES();
  auto SO = compileToSO(Configure{}, "dwarf_const_O2.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  for (const auto &[Name, Value] : {std::pair{"Fixed"sv, INT64_C(42)},
                                    std::pair{"Negative"sv, INT64_C(-2)}}) {
    auto V = findVariable(*D.Ctx, "scale"sv, Name);
    ASSERT_TRUE(V) << Name;
    auto Constant = V.find(llvm::dwarf::DW_AT_const_value);
    ASSERT_TRUE(Constant) << Name;
    auto Signed = Constant->getAsSignedConstant();
    ASSERT_TRUE(Signed) << Name;
    EXPECT_EQ(*Signed, Value) << Name;
  }
  removeOutput(SO);

  Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getCompilerConfigure().setOptimizationLevel(
      CompilerConfigure::OptimizationLevel::O0);
  Conf.getCompilerConfigure().setDebugInfo(true);
  auto Mod = loadFixture(Conf, "dwarf_const_O2.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  LLVM::Compiler Compiler(Conf);
  auto Data = Compiler.compile(*Mod);
  ASSERT_TRUE(Data);
  auto &M = *llvm::unwrap(Data->extract().LLModule.unwrap());
  uint32_t Guarded = 0;
  for (const auto &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    const auto &Entry = F.getEntryBlock();
    const bool HasGuard =
        std::any_of(Entry.begin(), Entry.end(), [](const llvm::Instruction &I) {
          const auto *Call = llvm::dyn_cast<llvm::CallInst>(&I);
          const auto *Callee = Call ? Call->getCalledFunction() : nullptr;
          return Callee &&
                 Callee->getIntrinsicID() == llvm::Intrinsic::stacksave;
        });
    if (!HasGuard) {
      continue;
    }
    ++Guarded;
    for (const auto &I : Entry) {
      EXPECT_FALSE(llvm::isa<llvm::StoreInst>(I)) << F.getName().str();
    }
  }
  EXPECT_GT(Guarded, 0U);
#if LLVM_VERSION_MAJOR < 19
#else
  std::map<std::string, int64_t> Values;
  for (const auto &F : M) {
    for (const auto &I : llvm::instructions(F)) {
      for (const llvm::DbgVariableRecord &DVR :
           llvm::filterDbgVars(I.getDbgRecordRange())) {
        const auto Name = DVR.getVariable()->getName().str();
        if (Name != "Fixed" && Name != "Negative") {
          continue;
        }
        EXPECT_FALSE(DVR.isDbgDeclare()) << Name;
        EXPECT_EQ(&I, &F.getEntryBlock().front()) << Name;
        if (const auto *C =
                llvm::dyn_cast<llvm::ConstantInt>(DVR.getValue(0))) {
          Values[Name] = C->getSExtValue();
        }
      }
    }
  }
  EXPECT_EQ(Values["Fixed"], 42);
  EXPECT_EQ(Values["Negative"], -2);
#endif
}

TEST(DebugInfoAOT, Memory64GetsOnlyLines) {
  for (auto Name :
       {"dwarf_basic_mem64_O0.wasm"sv, "dwarf_basic_mem64_import_O0.wasm"sv}) {
    Configure Conf;
    Conf.addProposal(Proposal::Memory64);
    auto SO =
        compileToSO(Conf, Name, true, CompilerConfigure::OptimizationLevel::O0);
    ASSERT_FALSE(SO.empty()) << Name;
    NativeDwarf D;
    ASSERT_TRUE(D.open(SO)) << Name;
    EXPECT_TRUE(D.hasSubprogram("sumList"sv)) << Name;
    EXPECT_TRUE(D.hasLine("dwarf_basic.c"sv, markerLine("BREAK_SUMLIST"sv)))
        << Name;
    EXPECT_FALSE(findVariable(*D.Ctx, "sumList"sv, "Head"sv)) << Name;
    EXPECT_FALSE(findVariable(*D.Ctx, ""sv, "Counter"sv)) << Name;
    EXPECT_FALSE(findVariable(*D.Ctx, "sumList"sv, "__wasm_memory"sv)) << Name;
    bool HasTypes = false;
    for (const auto &CU : D.Ctx->compile_units()) {
      for (const auto &Entry : CU->dies()) {
        llvm::DWARFDie Die(CU.get(), &Entry);
        HasTypes |= Die.getTag() == llvm::dwarf::DW_TAG_structure_type ||
                    Die.getTag() == llvm::dwarf::DW_TAG_base_type;
      }
    }
    EXPECT_FALSE(HasTypes) << Name;
    removeOutput(SO);
  }
}

TEST(DebugInfoAOT, NoMemoryModuleCompiles) {
  auto SO = compileToSO(Configure{}, "dwarf_nomem_O0.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  EXPECT_TRUE(D.hasSubprogram("add3"sv));
  removeOutput(SO);
}

} // namespace
