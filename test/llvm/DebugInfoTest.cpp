// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"

#include "common/configure.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include <gtest/gtest.h>
#include <llvm-c/Core.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
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

} // namespace
