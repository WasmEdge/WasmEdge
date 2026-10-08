// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"

#include "common/configure.h"
#include "common/defines.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <llvm-c/Core.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace WasmEdge;
using namespace std::literals;

std::filesystem::path fixture(std::string_view Name) {
  return std::filesystem::u8path("debuginfo"sv) / std::filesystem::u8path(Name);
}

std::vector<uint8_t> readHexFixture(std::string_view Name) {
  std::ifstream In(fixture(std::string(Name) + ".hex"), std::ios::binary);
  EXPECT_TRUE(In.is_open()) << Name;
  std::vector<uint8_t> Bytes;
  int Pending = -1;
  char C;
  while (In.get(C)) {
    if (C == ' ' || C == '\t' || C == '\r' || C == '\n') {
      continue;
    }
    int V = 0;
    if (C >= '0' && C <= '9') {
      V = C - '0';
    } else if (C >= 'a' && C <= 'f') {
      V = C - 'a' + 10;
    } else if (C >= 'A' && C <= 'F') {
      V = C - 'A' + 10;
    } else {
      ADD_FAILURE() << Name << ": bad hex character " << static_cast<int>(C);
      return {};
    }
    if (Pending < 0) {
      Pending = V;
    } else {
      Bytes.push_back(static_cast<uint8_t>(Pending * 16 + V));
      Pending = -1;
    }
  }
  if (Pending >= 0) {
    ADD_FAILURE() << Name << ": odd number of hex digits";
    return {};
  }
  return Bytes;
}

std::unique_ptr<AST::Module> loadFixture(const Configure &Conf,
                                         std::string_view Name) {
  auto Bytes = readHexFixture(Name);
  if (Bytes.empty()) {
    return nullptr;
  }
  Loader::Loader Ldr(Conf, &Executor::Executor::Intrinsics);
  auto Mod = Ldr.parseModule(Bytes);
  if (!Mod) {
    return nullptr;
  }
  Validator::Validator Valid(Conf);
  if (!Valid.validate(**Mod)) {
    return nullptr;
  }
  return std::move(*Mod);
}

uint32_t lineOf(const llvm::DILineInfo &Info) { return Info.Line; }

template <typename T> uint32_t lineOf(const std::optional<T> &Info) {
  return Info ? lineOf(*Info) : 0U;
}

TEST(DwarfReader, NoDebugSectionsGivesNull) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_nodebug.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  EXPECT_EQ(LLVM::DebugInfo::DwarfReader::create(*Mod), nullptr);
}

TEST(DwarfReader, MatchesEveryNamedFunction) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  EXPECT_EQ(R->getErrorCount(), 0U);
  std::vector<std::string> Names;
  for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
    if (auto Die = R->getSubprogram(I)) {
      Names.emplace_back(Die.getName(llvm::DINameKind::ShortName));
    }
  }
  EXPECT_NE(std::find(Names.begin(), Names.end(), "sumList"), Names.end());
  EXPECT_NE(std::find(Names.begin(), Names.end(), "fact"), Names.end());
  EXPECT_NE(std::find(Names.begin(), Names.end(), "run"), Names.end());
}

TEST(DwarfReader, LineForFirstInstructionOfSumList) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  const auto Segs = Mod->getCodeSection().getContent();
  for (uint32_t I = 0; I < Segs.size(); ++I) {
    auto Die = R->getSubprogram(I);
    if (!Die || Die.getName(llvm::DINameKind::ShortName) != "sumList"sv) {
      continue;
    }
    const auto Addr =
        R->toAddress(Segs[I].getExpr().getInstrs().front().getOffset());
    auto Info = R->getContext().getLineInfoForAddress(
        {Addr, llvm::object::SectionedAddress::UndefSection});
    EXPECT_EQ(lineOf(Info), Die.getDeclLine());
    return;
  }
  FAIL() << "sumList not found";
}

TEST(DwarfReader, CorruptDebugInfoIsCounted) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  for (auto &Custom : Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      auto &Bytes = Custom.getContent();
      Bytes.resize(Bytes.size() / 2);
    }
  }
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
    (void)R->getSubprogram(I);
  }
  EXPECT_GT(R->getErrorCount(), 0U);
}

TEST(DwarfReader, FallbackFunctionName) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  EXPECT_EQ(R->getFunctionName(100000U), "func[100000]"s);
}

llvm::DWARFDie findType(llvm::DWARFContext &Ctx, llvm::dwarf::Tag Tag,
                        std::string_view Name) {
  for (const auto &CU : Ctx.compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() == Tag && Die.getName(llvm::DINameKind::ShortName) &&
          Name == Die.getName(llvm::DINameKind::ShortName)) {
        return Die;
      }
    }
  }
  return {};
}

struct TypeFixture {
  Configure Conf;
  std::unique_ptr<AST::Module> Mod;
  std::unique_ptr<LLVM::DebugInfo::DwarfReader> Reader;
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  std::unique_ptr<LLVM::DebugInfo::ModuleDebugInfo> DI;
  bool load(std::string_view Name) {
    Mod = loadFixture(Conf, Name);
    if (!Mod) {
      return false;
    }
    Reader = LLVM::DebugInfo::DwarfReader::create(*Mod);
    M = std::make_unique<llvm::Module>("t", Ctx);
    DI = LLVM::DebugInfo::ModuleDebugInfo::create(
        *Mod, llvm::wrap(M.get()), LLVM::DebugInfo::ModuleDebugInfo::Part::All);
    return Reader && DI;
  }
};

TEST(DebugInfoTypes, RecursiveStructUsesWasmPointer) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_basic_O0.wasm"sv));
  auto NodeDie = findType(F.Reader->getContext(),
                          llvm::dwarf::DW_TAG_structure_type, "Node"sv);
  ASSERT_TRUE(NodeDie);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(NodeDie.getOffset())));
  ASSERT_EQ(T->getElements().size(), 2U);
  auto *Next = llvm::cast<llvm::DIDerivedType>(T->getElements()[1]);
  EXPECT_EQ(Next->getName(), "Next");
  EXPECT_EQ(Next->getOffsetInBits(), 32U);
  auto *Ptr = llvm::cast<llvm::DICompositeType>(Next->getBaseType());
  EXPECT_EQ(Ptr->getName(), "__wasm_ptr");
  EXPECT_EQ(Ptr->getSizeInBits(), 32U);
  auto *Pointee = llvm::cast<llvm::DIDerivedType>(Ptr->getElements()[1]);
  auto *Arr = llvm::cast<llvm::DICompositeType>(Pointee->getBaseType());
  EXPECT_EQ(Arr->getBaseType(), T);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, RustEnumHasVariantPart) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_enum_O0.wasm"sv));
  auto ShapeDie = findType(F.Reader->getContext(),
                           llvm::dwarf::DW_TAG_structure_type, "Shape"sv);
  ASSERT_TRUE(ShapeDie);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(ShapeDie.getOffset())));
  bool HasVariantPart = false;
  for (auto *E : T->getElements()) {
    if (auto *C = llvm::dyn_cast<llvm::DICompositeType>(E)) {
      HasVariantPart |= C->getTag() == llvm::dwarf::DW_TAG_variant_part;
    }
  }
  EXPECT_TRUE(HasVariantPart);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, UnknownTagBecomesOpaqueBytes) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_basic_O0.wasm"sv));
  auto CUDie =
      F.Reader->getContext().compile_units().begin()->get()->getUnitDIE();
  auto *T = llvm::unwrap(F.DI->translateType(CUDie.getOffset()));
  auto *Arr = llvm::dyn_cast_or_null<llvm::DICompositeType>(T);
  ASSERT_NE(Arr, nullptr);
  EXPECT_EQ(Arr->getTag(), llvm::dwarf::DW_TAG_array_type);
}

std::filesystem::path compileToSO(Configure Conf, std::string_view Name,
                                  bool Debug,
                                  CompilerConfigure::OptimizationLevel Level,
                                  std::filesystem::path Out = {}) {
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);
  Conf.getCompilerConfigure().setOptimizationLevel(Level);
  Conf.getCompilerConfigure().setDebugInfo(Debug);
  auto Mod = loadFixture(Conf, Name);
  if (!Mod) {
    return {};
  }
  LLVM::Compiler Compiler(Conf);
  auto Data = Compiler.compile(*Mod);
  if (!Data) {
    return {};
  }
  if (Out.empty()) {
    Out = std::filesystem::temp_directory_path() /
          fmt::format("wasmedge-dwarf-{}-{}-{}" WASMEDGE_LIB_EXTENSION, Name,
                      Debug, static_cast<int>(Level));
  }
  LLVM::CodeGen CG(Conf);
  std::vector<Byte> Bytes;
  if (!CG.codegen(Bytes, std::move(*Data), Out)) {
    return {};
  }
  return Out;
}

std::filesystem::path debugFileOf(std::filesystem::path SO) {
#if WASMEDGE_OS_MACOS
  SO += ".o";
#endif
  return SO;
}

void removeOutput(const std::filesystem::path &SO) {
  std::error_code Error;
  std::filesystem::remove(SO, Error);
  std::filesystem::remove(debugFileOf(SO), Error);
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

uint32_t markerLine(std::string_view Marker) {
  std::ifstream In(fixture("dwarf_basic.c"sv));
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

TEST(DebugInfoAOT, OffMeansNoDebugSections) {
  auto SO = compileToSO(Configure{}, "dwarf_basic_O0.wasm"sv, false,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  EXPECT_FALSE(D.open(SO));
  removeOutput(SO);
}

TEST(DebugInfoAOT, NoDwarfModuleHasIdenticalSections) {
  const auto Out = std::filesystem::temp_directory_path() /
                   "wasmedge-dwarf-nodebug" WASMEDGE_LIB_EXTENSION;
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
  Conf.getCompilerConfigure().setDebugInfo(true);
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  for (auto &Custom : Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      auto &Bytes = Custom.getContent();
      Bytes.resize(Bytes.size() / 2);
    }
  }
  LLVM::Compiler Compiler(Conf);
  EXPECT_TRUE(Compiler.compile(*Mod));
}

} // namespace
