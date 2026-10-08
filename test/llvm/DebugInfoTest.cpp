// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "data.h"
#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"

#include "common/configure.h"
#include "common/defines.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <llvm-c/Core.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/IR/DebugInfoMetadata.h>
#if LLVM_VERSION_MAJOR < 19
#else
#include <llvm/IR/DebugProgramInstruction.h>
#endif
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
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
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
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

TEST(DwarfReader, StackValueDropsMemoryLocations) {
  using LLVM::DebugInfo::decodeLocation;
  using Kind = LLVM::DebugInfo::WasmLocation::Kind;
  const uint8_t FrameBase[] = {0x91, 0x0c};
  const uint8_t FrameBaseValue[] = {0x91, 0x0c, 0x9f};
  const uint8_t Addr[] = {0x03, 0x10, 0x00, 0x00, 0x00};
  const uint8_t AddrValue[] = {0x03, 0x10, 0x00, 0x00, 0x00, 0x9f};
  const uint8_t LocalValue[] = {0xed, 0x00, 0x02, 0x9f};
  const auto FB = decodeLocation(FrameBase, nullptr);
  EXPECT_EQ(FB.K, Kind::FrameBaseOffset);
  EXPECT_EQ(FB.Offset, 12);
  EXPECT_EQ(decodeLocation(FrameBaseValue, nullptr).K, Kind::None);
  const auto A = decodeLocation(Addr, nullptr);
  EXPECT_EQ(A.K, Kind::Address);
  EXPECT_EQ(A.Index, 16U);
  EXPECT_EQ(decodeLocation(AddrValue, nullptr).K, Kind::None);
  const auto L = decodeLocation(LocalValue, nullptr);
  EXPECT_EQ(L.K, Kind::Local);
  EXPECT_EQ(L.Index, 2U);
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
  auto *Elem = llvm::cast<llvm::DIDerivedType>(Arr->getBaseType());
  EXPECT_EQ(Elem->getTag(), llvm::dwarf::DW_TAG_pointer_type);
  EXPECT_EQ(Elem->getBaseType(), T);
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

TEST(DebugInfoTypes, NamespacedStaticGetsGlobalVariable) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_enum_O0.wasm"sv));
  auto *MB = F.M->getGlobalVariable("__wasmedge_debug_membase");
  ASSERT_NE(MB, nullptr);
  llvm::SmallVector<llvm::DIGlobalVariableExpression *, 4> GVEs;
  MB->getDebugInfo(GVEs);
  const llvm::DIGlobalVariable *Calls = nullptr;
  for (auto *GVE : GVEs) {
    if (GVE->getVariable()->getName() == "CALLS") {
      Calls = GVE->getVariable();
    }
  }
  ASSERT_NE(Calls, nullptr);
  auto *NS = llvm::dyn_cast_or_null<llvm::DINamespace>(Calls->getScope());
  ASSERT_NE(NS, nullptr);
  EXPECT_EQ(NS->getName(), "dwarf_enum");
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
  LLVM::Compiler Compiler(Conf);
  EXPECT_TRUE(Compiler.compile(*Mod));
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
  auto FI = DI->beginFunction(1, 1, llvm::wrap(Fn));
  ASSERT_NE(FI, nullptr);
  auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Fn);
  llvm::IRBuilder<> B(Entry);
  std::vector<LLVMValueRef> Locals;
  for (uint32_t I = 0; I < 5; ++I) {
    Locals.push_back(llvm::wrap(B.CreateAlloca(I32)));
  }
  auto *Mark = B.CreateAlloca(I32);
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
  FI->finish(llvm::wrap(Entry), Locals, nullptr, nullptr, nullptr, true);
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

TEST(DebugInfoAOT, VariablesHaveLocations) {
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

TEST(DebugInfoAOT, NoMemoryModuleCompiles) {
  auto SO = compileToSO(Configure{}, "dwarf_nomem_O0.wasm"sv, true,
                        CompilerConfigure::OptimizationLevel::O0);
  ASSERT_FALSE(SO.empty());
  NativeDwarf D;
  ASSERT_TRUE(D.open(SO));
  EXPECT_TRUE(D.hasSubprogram("add3"sv));
  removeOutput(SO);
}

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
  auto *MB = InfraM->getGlobalVariable("__wasmedge_debug_membase");
  ASSERT_NE(MB, nullptr);
  EXPECT_FALSE(MB->isDeclaration());
  llvm::SmallVector<llvm::DIGlobalVariableExpression *, 4> GVEs;
  MB->getDebugInfo(GVEs);
  EXPECT_FALSE(GVEs.empty());
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
  EXPECT_FALSE(llvm::verifyModule(*BatchM, &llvm::errs()));
}

} // namespace
