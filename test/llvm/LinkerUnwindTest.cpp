// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "LinkerTestHelpers.h"
#include "LinkerTestUtils.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/layout.h"
#include "linker/link_graph.h"
#include "linker/object_reader.h"
#include "linker/relocation.h"

#include <gtest/gtest.h>

#include <llvm/BinaryFormat/MachO.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/LEB128.h>
#include <llvm/Support/raw_ostream.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using namespace WasmEdge::LLVM::Linker;
using namespace WasmEdge::LLVM::Test;

class GeneratedMachOEHFrameTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

TEST_P(GeneratedMachOEHFrameTest, NormalizesGeneratedMachOFrames) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Triple = Test.Triple;
  const auto Architecture = Test.Architecture;
  const auto Object = makeObject(llvm::Triple(Triple), false, false, "f0", {},
                                 false, false, "generic", {}, true, false,
                                 false, false, false, false, {}, false, true);
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph) << Triple;
  ASSERT_TRUE(compactUnwindToEHFrame(*Graph)) << Triple;
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph)) << Triple;
  constexpr uint64_t FirstBase = UINT64_C(0x100000000);
  constexpr uint64_t SecondBase = UINT64_C(0x700000000);
  const auto First = machOEHFrameStarts(*Graph, FirstBase);
  const auto Second = machOEHFrameStarts(*Graph, SecondBase);
  ASSERT_TRUE(First) << Triple;
  ASSERT_TRUE(Second) << Triple;
  ASSERT_EQ(First->size(), Second->size());
  EXPECT_EQ(First->size(), 2U) << Triple;
  for (size_t I = 0; I < First->size(); ++I)
    EXPECT_EQ((*First)[I] - FirstBase, (*Second)[I] - SecondBase) << Triple;
  for (const std::string_view Name : {"_f0", "_t0"}) {
    const auto Symbol =
        std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                     [&](const auto &Value) { return Value.Name == Name; });
    ASSERT_NE(Symbol, Graph->symbols().end()) << Triple << " " << Name;
    const uint64_t Address =
        FirstBase + Graph->sections()[Symbol->Section].Address + Symbol->Offset;
    EXPECT_NE(std::find(First->begin(), First->end(), Address), First->end())
        << Triple << " " << Name;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedMachOEHFrameTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"arm64-apple-macosx", Target::AArch64}),
    objectArchitectureCaseName);

TEST(MachOEHFrameTest, RelocationOwnsFDEAddressAndAppliesAddend) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Fixture = makeX86_64MachOEHFrameFixture(false);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  auto &RelocationValue =
      const_cast<Relocation &>(Graph->relocations().front());
  ASSERT_EQ(RelocationValue.Offset, Fixture.Field);
  RelocationValue.Addend = 1;
  RelocationValue.AddendIsImplicit = false;
  const auto ReferencedSymbol = RelocationValue.Symbol;
  auto &EH = const_cast<Section &>(Graph->sections()[RelocationValue.Section]);
  ASSERT_TRUE(Internal::writeUnsigned(EH.Content, Fixture.Field, 8,
                                      Endianness::Little,
                                      UINT64_C(0x7FFFFFFFFFFFFFFF)));

  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 1U);
  const auto &Symbol = Graph->symbols()[ReferencedSymbol];
  EXPECT_EQ(Starts->front(),
            Graph->sections()[Symbol.Section].Address + Symbol.Offset + 1);
}

TEST(MachOEHFrameTest, RejectsNonPointerWidthFDERelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Fixture = makeX86_64MachOEHFrameFixture(false);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  const_cast<Relocation &>(Graph->relocations().front()).PatchSize = 4;
  EXPECT_FALSE(normalizeMachOEHFrame(*Graph));
}

class GeneratedMachOCompactUnwindTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

TEST_P(GeneratedMachOCompactUnwindTest,
       InventoriesObservedLLVMGeneratedMachORecords) {
  struct Fixture {
    const char *Name;
    bool UnwindTable;
    bool Optimize;
    bool Representative;
    bool Exceptions;
    bool TypeWrapper;
  };
  constexpr std::array<Fixture, 6> Fixtures{{
      {"leaf", false, false, false, false, false},
      {"default-unwind", true, false, false, false, false},
      {"optimized-unwind", true, true, false, false, false},
      {"representative-mixed", true, true, true, false, false},
      {"type-wrapper-compact-only", true, false, false, false, true},
      {"exceptions-mixed-zero-compact-personality-lsda", true, false, false,
       true, false},
  }};
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  REQUIRE_LLVM_TARGET(Triple);
  for (const auto &Fixture : Fixtures) {
    const auto Inventory = collectCompactUnwindInventory(
        makeObject(llvm::Triple(Triple), false, false, "f0", {}, false, false,
                   "generic", {}, Fixture.UnwindTable, Fixture.Optimize, false,
                   false, Fixture.Representative, Fixture.Exceptions, {}, false,
                   Fixture.TypeWrapper));
    if (std::string_view(Triple) == "x86_64-apple-macosx") {
      EXPECT_FALSE(Inventory.HasCompactUnwind) << Fixture.Name;
      EXPECT_TRUE(Inventory.Records.empty()) << Fixture.Name;
      EXPECT_TRUE(Inventory.Relocations.empty()) << Fixture.Name;
      EXPECT_EQ(Inventory.HasEHFrame, Fixture.UnwindTable) << Fixture.Name;
      continue;
    }
    if (!Fixture.UnwindTable) {
      EXPECT_FALSE(Inventory.HasCompactUnwind) << Fixture.Name;
      EXPECT_TRUE(Inventory.Records.empty()) << Fixture.Name;
      EXPECT_TRUE(Inventory.Relocations.empty()) << Fixture.Name;
      EXPECT_FALSE(Inventory.HasEHFrame) << Fixture.Name;
      continue;
    }
    const size_t ExpectedCount = Fixture.TypeWrapper  ? 2U
                                 : Fixture.Exceptions ? 3U
                                                      : 1U;
    ASSERT_TRUE(Inventory.HasCompactUnwind) << Fixture.Name;
    ASSERT_EQ(Inventory.Records.size(), ExpectedCount) << Fixture.Name;
    ASSERT_EQ(Inventory.Relocations.size(), ExpectedCount) << Fixture.Name;
    const bool MixedCompactAndDwarf =
        Fixture.Representative || Fixture.Exceptions;
    EXPECT_EQ(Inventory.HasEHFrame, MixedCompactAndDwarf) << Fixture.Name;
    uint64_t Function = 0;
    std::set<uint64_t> RelocationOffsets;
    std::set<std::string> RelocationSymbols;
    for (size_t I = 0; I < Inventory.Records.size(); ++I) {
      const auto &Record = Inventory.Records[I];
      EXPECT_EQ(Record.Function, Function) << Fixture.Name << " " << I;
      EXPECT_GT(Record.Length, 0U) << Fixture.Name << " " << I;
      EXPECT_EQ(Record.Encoding,
                I == 0 && (Fixture.Representative || Fixture.Exceptions)
                    ? UINT32_C(0x03000000)
                    : UINT32_C(0x02000000))
          << Fixture.Name << " " << I;
      EXPECT_EQ(Record.Personality, 0U) << Fixture.Name << " " << I;
      EXPECT_EQ(Record.LSDA, 0U) << Fixture.Name << " " << I;
      Function += Record.Length;
    }
    for (const auto &Relocation : Inventory.Relocations) {
      EXPECT_EQ(Relocation.Type,
                static_cast<uint64_t>(llvm::MachO::ARM64_RELOC_UNSIGNED))
          << Fixture.Name;
      EXPECT_EQ(Relocation.Offset % sizeof(RawCompactUnwindRecord), 0U)
          << Fixture.Name;
      RelocationOffsets.emplace(Relocation.Offset);
      RelocationSymbols.emplace(Relocation.Symbol);
    }
    std::set<uint64_t> ExpectedOffsets;
    for (size_t I = 0; I < ExpectedCount; ++I)
      ExpectedOffsets.emplace(I * sizeof(RawCompactUnwindRecord));
    EXPECT_EQ(RelocationOffsets, ExpectedOffsets) << Fixture.Name;
    EXPECT_EQ(RelocationSymbols, (std::set<std::string>{""})) << Fixture.Name;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedMachOCompactUnwindTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"arm64-apple-macosx", Target::AArch64}),
    objectArchitectureCaseName);

TEST(CompactUnwindTest, ConvertsAArch64FramelessAndSemanticCoverage) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02002000);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 1U);
  EXPECT_EQ(Starts->front(), Graph.sections().front().Address);
  const auto EH =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph.sections().end());
  const std::vector<WasmEdge::Byte> CFI{0x0C, 31, 32, 0x08, 30};
  EXPECT_NE(std::search(EH->Content.begin(), EH->Content.end(), CFI.begin(),
                        CFI.end()),
            EH->Content.end());
  ASSERT_GE(EH->Content.size(), 4U);
  EXPECT_EQ(
      std::vector<WasmEdge::Byte>(EH->Content.end() - 4, EH->Content.end()),
      std::vector<WasmEdge::Byte>(4));
}

TEST(CompactUnwindTest, ConvertsX86RBPFrame) {
  auto Graph = makeCompactUnwindGraph(Target::X86_64, 0x01010001);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  EXPECT_EQ(Starts->size(), 1U);
  const auto EH =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph.sections().end());
  const std::vector<WasmEdge::Byte> CFI{0x0C, 6, 16, 0x90, 1, 0x86, 2, 0x83, 3};
  EXPECT_NE(std::search(EH->Content.begin(), EH->Content.end(), CFI.begin(),
                        CFI.end()),
            EH->Content.end());
}

TEST(CompactUnwindTest, ConvertsMultipleOrderedRecords) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02000000, "_t0");
  auto Second = Graph.addSymbol(Symbol{"_f0", 0, 16, 16, true, {}, true});
  ASSERT_TRUE(Second);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Second, 16, 0x02001000, {}, {}, {}}));
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  EXPECT_EQ(Starts->size(), 2U);
  EXPECT_TRUE(std::is_sorted(Starts->begin(), Starts->end()));
}

void expectUnsupportedCompactUnwindRejectedAtomically(Target Architecture,
                                                      uint32_t Encoding) {
  auto Graph = makeCompactUnwindGraph(Architecture, Encoding);
  const auto Sections = Graph.sections();
  const auto References = Graph.ehFrameReferences();
  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(Graph.sections().size(), Sections.size());
  EXPECT_EQ(Graph.ehFrameReferences().size(), References.size());
  EXPECT_EQ(Graph.sections().front().Content, Sections.front().Content);
}

TEST(CompactUnwindTest, RejectsUnsupportedX86_64RecordAtomically) {
  expectUnsupportedCompactUnwindRejectedAtomically(Target::X86_64, 0x01010009);
}

TEST(CompactUnwindTest, RejectsUnsupportedAArch64RecordsAtomically) {
  expectUnsupportedCompactUnwindRejectedAtomically(Target::AArch64, 0x02000020);
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x42000000);
  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(Graph.sections().size(), 1U);

  auto MissingFDE = makeCompactUnwindGraph(Target::AArch64, 0x03000000);
  EXPECT_FALSE(compactUnwindToEHFrame(MissingFDE));
  EXPECT_EQ(MissingFDE.sections().size(), 1U);

  for (const bool HasLSDA : {false, true}) {
    LinkGraph Optional(Target::AArch64, Endianness::Little,
                       ObjectFormat::MachO);
    ASSERT_TRUE(Optional.beginInput("optional.o"));
    auto Text = Optional.addSection(
        Section{"__text", SectionKind::Text, 4, 16, 0, 0,
                std::vector<WasmEdge::Byte>(16), SectionPurpose::Default});
    auto Data = Optional.addSection(
        Section{"__data", SectionKind::Data, 8, 8, 0, 0,
                std::vector<WasmEdge::Byte>(8), SectionPurpose::Default});
    ASSERT_TRUE(Text && Data);
    auto Function =
        Optional.addSymbol(Symbol{"_f0", *Text, 0, 16, true, {}, true});
    auto Value = Optional.addSymbol(Symbol{"optional", *Data, 0, 8, false});
    ASSERT_TRUE(Function && Value);
    ASSERT_TRUE(Optional.addCompactUnwind(CompactUnwindRecord{
        *Function,
        16,
        0x02000000,
        HasLSDA ? std::optional<SymbolId>{} : std::optional<SymbolId>{*Value},
        HasLSDA ? std::optional<SymbolId>{*Value} : std::optional<SymbolId>{},
        {}}));
    EXPECT_FALSE(compactUnwindToEHFrame(Optional));
    EXPECT_EQ(Optional.sections().size(), 2U);
  }
}

TEST(CompactUnwindTest, PreservesExistingDwarfFallbackAndSynthesizesMissing) {
  auto Existing = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(compactUnwindToEHFrame(Existing));
  const auto ExistingEH =
      std::find_if(Existing.sections().begin(), Existing.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(ExistingEH, Existing.sections().end());
  ASSERT_EQ(Existing.ehFrameReferences().size(), 1U);

  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("mixed.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 4, 64, 0, 0,
                                       std::vector<WasmEdge::Byte>(64),
                                       SectionPurpose::Default, 0x1000});
  auto EH = Graph.addSection(*ExistingEH);
  ASSERT_TRUE(Text && EH);
  auto First = Graph.addSymbol(Symbol{"_t0", *Text, 0, 16, true, {}, true});
  auto Second = Graph.addSymbol(Symbol{"_f0", *Text, 16, 16, true, {}, true});
  ASSERT_TRUE(First && Second);
  ASSERT_TRUE(Graph.addEHFrameReference(EHFrameReference{
      *EH, Existing.ehFrameReferences().front().Offset, *First}));
  auto FDE = Graph.addSymbol(
      Symbol{"fallback_fde", *EH,
             Existing.ehFrameReferences().front().Offset - 8, 0, false});
  ASSERT_TRUE(FDE);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*First, 16, 0x03000000, {}, {}, *FDE}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Second, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(std::count_if(Graph.sections().begin(), Graph.sections().end(),
                          [](const auto &Section) {
                            return Section.Purpose == SectionPurpose::EHFrame;
                          }),
            1);
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  EXPECT_EQ(Starts->size(), 2U);
}

TEST(EHFrameTest, AcceptsUnwindlessGraphWithoutSemanticFunctions) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("unwindless.o"));
  ASSERT_TRUE(Graph.addSection(Section{
      "__text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}}));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));

  EXPECT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
}

TEST(EHFrameTest, RejectsUnwindlessSemanticFunction) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("unwindless.o"));
  ASSERT_TRUE(Graph.addSection(Section{
      "__text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"_f0", 0, 0, 4, true, {}, true}));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));

  EXPECT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_FALSE(validateMachOEHFrameCoverage(Graph));
}

TEST(EHFrameTest, RequiresTypeWrapperCoverage) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Object = makeObject(llvm::Triple("arm64-apple-macosx"), false, false,
                           "f0", {}, false, false, "generic", {}, true, false,
                           false, false, false, false, {}, false, true);
  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(compactUnwindToEHFrame(*Graph));
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph));

  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  const auto T0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_t0"; });
  const auto F0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_f0"; });
  ASSERT_NE(T0, Graph->symbols().end());
  ASSERT_NE(F0, Graph->symbols().end());
  const uint64_t T0Address =
      Graph->sections()[T0->Section].Address + T0->Offset;
  const uint64_t F0Address =
      Graph->sections()[F0->Section].Address + F0->Offset;
  ASSERT_NE(std::find(Starts->begin(), Starts->end(), T0Address),
            Starts->end());
  ASSERT_NE(std::find(Starts->begin(), Starts->end(), F0Address),
            Starts->end());

  const auto EH =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph->sections().end());
  const SectionId EHId = static_cast<SectionId>(EH - Graph->sections().begin());
  auto Content = Graph->sections()[EHId].Content;
  bool Mutated = false;
  for (size_t Offset = 0; Offset + 16 <= Content.size();) {
    const uint32_t Length = read32le(Content, Offset);
    if (Length == 0)
      break;
    const uint32_t CIE = read32le(Content, Offset + 4);
    if (CIE != 0) {
      const size_t Field = Offset + 8;
      int64_t Delta = static_cast<int64_t>(read64le(Content, Field));
      if (static_cast<int64_t>(Graph->sections()[EHId].Address + Field) +
              Delta ==
          static_cast<int64_t>(T0Address)) {
        Delta = static_cast<int64_t>(F0Address -
                                     (Graph->sections()[EHId].Address + Field));
        write64le(Content, Field, static_cast<uint64_t>(Delta));
        Mutated = true;
        break;
      }
    }
    Offset += Length + 4;
  }
  ASSERT_TRUE(Mutated);
  LinkGraph Corrupted(Graph->target(), Graph->endianness(), Graph->format());
  ASSERT_TRUE(Corrupted.beginInput("corrupted-eh-frame"));
  for (SectionId I = 0; I < Graph->sections().size(); ++I) {
    auto SectionValue = Graph->sections()[I];
    if (I == EHId)
      SectionValue.Content = std::move(Content);
    ASSERT_TRUE(Corrupted.addSection(std::move(SectionValue)));
  }
  for (const auto &Symbol : Graph->symbols())
    ASSERT_TRUE(Corrupted.addSymbol(Symbol));
  for (const auto &Relocation : Graph->relocations())
    ASSERT_TRUE(Corrupted.addRelocation(Relocation));
  for (const auto &Rebase : Graph->rebases())
    ASSERT_TRUE(Corrupted.addRebase(Rebase));
  for (const auto &Reference : Graph->ehFrameReferences())
    ASSERT_TRUE(Corrupted.addEHFrameReference(Reference));
  EXPECT_FALSE(validateMachOEHFrameCoverage(Corrupted));
}

TEST(EHFrameTest, CollapsesAliasedSemanticFunctionAddresses) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Object = makeObject(llvm::Triple("arm64-apple-macosx"), false, false,
                           "f0", {}, false, false, "generic", {}, true, false,
                           false, false, false, false, {}, false, true);
  auto Covered = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Covered);
  ASSERT_TRUE(compactUnwindToEHFrame(*Covered));
  ASSERT_TRUE(layout(*Covered, 0, 0x4000));
  auto &Symbols = const_cast<std::vector<Symbol> &>(Covered->symbols());
  const auto T0 =
      std::find_if(Symbols.begin(), Symbols.end(),
                   [](const auto &Symbol) { return Symbol.Name == "_t0"; });
  ASSERT_NE(T0, Symbols.end());
  const SectionId T0Section = T0->Section;
  const uint64_t T0Offset = T0->Offset;
  const uint64_t T0Size = T0->Size;
  Symbols.push_back(
      Symbol{"_t1", T0Section, T0Offset, T0Size, true, std::nullopt, true});
  ASSERT_TRUE(normalizeMachOEHFrame(*Covered));
  EXPECT_TRUE(validateMachOEHFrameCoverage(*Covered));
  Symbols.back().Offset = T0Offset + 1;
  EXPECT_FALSE(validateMachOEHFrameCoverage(*Covered));
}

TEST(EHFrameTest, DecodesInt64SLEB128) {
  const std::array<WasmEdge::Byte, 10> Max{0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                           0xFF, 0xFF, 0xFF, 0xFF, 0x00};
  const std::array<WasmEdge::Byte, 10> Min{0x80, 0x80, 0x80, 0x80, 0x80,
                                           0x80, 0x80, 0x80, 0x80, 0x7F};
  auto MaxValue = Internal::decodeSLEB128(Max);
  auto MinValue = Internal::decodeSLEB128(Min);
  ASSERT_TRUE(MaxValue);
  ASSERT_TRUE(MinValue);
  EXPECT_EQ(*MaxValue, INT64_MAX);
  EXPECT_EQ(*MinValue, INT64_MIN);
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}));
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7E}));
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 11>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  auto PaddedZero = Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00});
  auto PaddedMinusOne = Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F});
  ASSERT_TRUE(PaddedZero);
  ASSERT_TRUE(PaddedMinusOne);
  EXPECT_EQ(*PaddedZero, 0);
  EXPECT_EQ(*PaddedMinusOne, -1);
  for (const int64_t Value :
       {INT64_C(0), INT64_C(1), INT64_C(-1), INT64_C(63), INT64_C(-64),
        INT64_C(1) << 40, -(INT64_C(1) << 40), INT64_MAX, INT64_MIN}) {
    for (const unsigned PadTo : {0U, 5U, 10U}) {
      std::string Encoded;
      llvm::raw_string_ostream Stream(Encoded);
      llvm::encodeSLEB128(Value, Stream, PadTo);
      Stream.flush();
      const auto *Data =
          reinterpret_cast<const WasmEdge::Byte *>(Encoded.data());
      auto Decoded = Internal::decodeSLEB128(
          WasmEdge::Span<const WasmEdge::Byte>(Data, Encoded.size()));
      ASSERT_TRUE(Decoded) << Value << " padded to " << PadTo;
      EXPECT_EQ(*Decoded, Value);
    }
  }
}

TEST(EHFrameTest, ChecksResolvedAddressArithmetic) {
  EXPECT_EQ(Internal::resolveMachOFDEAddress(0, 0, 0, INT64_MAX),
            static_cast<uint64_t>(INT64_MAX));
  EXPECT_EQ(Internal::resolveMachOFDEAddress(UINT64_MAX, 0, 0, INT64_MIN),
            static_cast<uint64_t>(INT64_MAX));
  EXPECT_EQ(Internal::resolveMachOFDEAddress(UINT64_MAX - 10, 4, 3, 2),
            UINT64_MAX - 1);
  EXPECT_FALSE(Internal::resolveMachOFDEAddress(UINT64_MAX - 10, 8, 3, 1));
  EXPECT_EQ(Internal::resolveMachOFDEAddress(5, 4, 3, -2), 10U);
  EXPECT_FALSE(Internal::resolveMachOFDEAddress(0, 0, 1, -2));
  EXPECT_FALSE(Internal::resolveMachOFDEAddress(0, 0, 0, INT64_MIN));
}

TEST(EHFrameTest, RejectsMalformedRecordsAtomically) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("malformed.o"));
  ASSERT_TRUE(Graph.addSection(Section{"__eh_frame",
                                       SectionKind::Unwind,
                                       8,
                                       8,
                                       0,
                                       0,
                                       {4, 0, 0, 0, 0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  const auto Before = Graph.sections()[0].Content;
  EXPECT_FALSE(normalizeMachOEHFrame(Graph));
  EXPECT_EQ(Graph.sections()[0].Content, Before);
}

TEST(EHFrameTest, ReportsFDEInitialLocationFields) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeAArch64MachOEHFrameObject();
  const auto Object = machOEHFrameObject(Bytes);
  ASSERT_NE(Object.Content, 0U);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "eh-frame.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__eh_frame")
      continue;
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    const std::vector<WasmEdge::Byte> Frame(Content->bytes_begin(),
                                            Content->bytes_end());
    auto Fields = machOEHFrameFields(Frame, Target::AArch64);
    ASSERT_TRUE(Fields);
    ASSERT_EQ(Fields->size(), 1U);
    return;
  }
  FAIL() << "generated object has no EH frame section";
}

TEST(EHFrameTest, RejectsReferenceOutsideFDEInitialLocationField) {
  auto Source = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(compactUnwindToEHFrame(Source));
  ASSERT_EQ(Source.ehFrameReferences().size(), 1U);
  auto &References =
      const_cast<std::vector<EHFrameReference> &>(Source.ehFrameReferences());
  References.front().Offset = 8;
  const auto Before = Source.sections().back().Content;
  EXPECT_FALSE(normalizeMachOEHFrame(Source));
  EXPECT_EQ(Source.sections().back().Content, Before);

  auto WrongSection = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(compactUnwindToEHFrame(WrongSection));
  auto &WrongReferences = const_cast<std::vector<EHFrameReference> &>(
      WrongSection.ehFrameReferences());
  WrongReferences.front().Section = 0;
  EXPECT_FALSE(normalizeMachOEHFrame(WrongSection));
}

TEST(CompactUnwindTest, ConvertsSparseX86RBPRegisterSlots) {
  auto Graph = makeCompactUnwindGraph(Target::X86_64, 0x01040081);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  const auto EH =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph.sections().end());
  const std::vector<WasmEdge::Byte> CFI{0x83, 6, 0x8C, 4};
  EXPECT_NE(std::search(EH->Content.begin(), EH->Content.end(), CFI.begin(),
                        CFI.end()),
            EH->Content.end());
}

TEST(CompactUnwindTest, ConvertsAArch64FramePairs) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x04000303);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  const auto &Bytes = Graph.sections().back().Content;
  const std::vector<WasmEdge::Byte> CFI{
      0x0C, 29,   16, 0x9E, 1,    0x9D, 2, 0x93, 3,  0x94, 4,    0x95, 5, 0x96,
      6,    0x05, 72, 7,    0x05, 73,   8, 0x05, 74, 9,    0x05, 75,   10};
  EXPECT_NE(std::search(Bytes.begin(), Bytes.end(), CFI.begin(), CFI.end()),
            Bytes.end());
}

TEST(CompactUnwindTest, ConvertsAArch64FramelessPairs) {
  const auto DecodeRules = [](WasmEdge::Span<const WasmEdge::Byte> Bytes,
                              uint64_t StackSize) {
    std::map<uint32_t, int64_t> Rules;
    std::vector<WasmEdge::Byte> Prefix{0x0C, 31};
    uint64_t Value = StackSize;
    do {
      WasmEdge::Byte Part = static_cast<WasmEdge::Byte>(Value & 0x7F);
      Value >>= 7;
      Prefix.push_back(Part | (Value == 0 ? 0 : 0x80));
    } while (Value != 0);
    Prefix.insert(Prefix.end(), {0x08, 30});
    auto Cursor =
        std::search(Bytes.begin(), Bytes.end(), Prefix.begin(), Prefix.end());
    EXPECT_NE(Cursor, Bytes.end());
    if (Cursor == Bytes.end())
      return Rules;
    Cursor += static_cast<ptrdiff_t>(Prefix.size());
    while (Cursor != Bytes.end()) {
      uint32_t Register = 0;
      if ((*Cursor & 0xC0) == 0x80) {
        Register = *Cursor++ & 0x3F;
      } else if (*Cursor++ == 0x05 && Cursor != Bytes.end()) {
        Register = *Cursor++;
      } else {
        break;
      }
      if (Cursor == Bytes.end())
        break;
      const uint64_t Factor = *Cursor++;
      Rules.emplace(Register, -8 * static_cast<int64_t>(Factor));
    }
    return Rules;
  };
  for (const auto &[Encoding, Expected] :
       std::array<std::pair<uint32_t, std::map<uint32_t, int64_t>>, 2>{
           {{{0x0200400F},
             {{19, -8},
              {20, -16},
              {21, -24},
              {22, -32},
              {23, -40},
              {24, -48},
              {25, -56},
              {26, -64}}},
            {{0x02021010}, {{27, -8}, {28, -16}}}}}) {
    auto Graph = makeCompactUnwindGraph(Target::AArch64, Encoding);
    ASSERT_TRUE(compactUnwindToEHFrame(Graph));
    const auto &Bytes = Graph.sections().back().Content;
    EXPECT_EQ(DecodeRules(Bytes, ((Encoding >> 12) & 0xFFF) * 16), Expected);
  }
  auto Reserved = makeCompactUnwindGraph(Target::AArch64, 0x02001020);
  EXPECT_FALSE(validateCompactUnwind(Reserved));
}

void expectNotFunctionStartFlagPreserved(Target Architecture,
                                         uint32_t Encoding) {
  auto Graph = makeCompactUnwindGraph(Architecture, Encoding);
  ASSERT_TRUE(validateCompactUnwind(Graph));
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  const auto Unwind = std::find_if(
      Graph.sections().begin(), Graph.sections().end(),
      [](const auto &S) { return S.Purpose == SectionPurpose::UnwindInfo; });
  ASSERT_NE(Unwind, Graph.sections().end());
  const std::array<WasmEdge::Byte, 4> Encoded{
      0, 0,
      Architecture == Target::X86_64 ? WasmEdge::Byte{2} : WasmEdge::Byte{0},
      0x82};
  EXPECT_NE(std::search(Unwind->Content.begin(), Unwind->Content.end(),
                        Encoded.begin(), Encoded.end()),
            Unwind->Content.end());
}

TEST(CompactUnwindTest, PreservesAArch64NotFunctionStartFlag) {
  expectNotFunctionStartFlagPreserved(Target::AArch64, 0x82000000);
}

TEST(CompactUnwindTest, PreservesX86_64NotFunctionStartFlag) {
  expectNotFunctionStartFlagPreserved(Target::X86_64, 0x82020000);
}

TEST(CompactUnwindTest, ConvertsX86FramelessModes) {
  auto Immediate = makeCompactUnwindGraph(Target::X86_64, 0x02360804);
  ASSERT_TRUE(compactUnwindToEHFrame(Immediate));
  const std::vector<WasmEdge::Byte> ImmediateCFI{0x0C, 7,    0xB0, 3,    0x90,
                                                 1,    0x86, 2,    0x83, 3};
  EXPECT_NE(std::search(Immediate.sections().back().Content.begin(),
                        Immediate.sections().back().Content.end(),
                        ImmediateCFI.begin(), ImmediateCFI.end()),
            Immediate.sections().back().Content.end());

  std::vector<WasmEdge::Byte> Text(64);
  const std::array<WasmEdge::Byte, 5> Instruction{0x48, 0x81, 0xEC, 0x48, 0x38};
  std::copy(Instruction.begin(), Instruction.end(), Text.begin() + 2);
  auto Indirect = makeCompactUnwindGraph(Target::X86_64, 0x03056804, "_f0",
                                         std::move(Text));
  ASSERT_TRUE(compactUnwindToEHFrame(Indirect));
}

TEST(CompactUnwindTest, ValidatesAllDocumentedAArch64ModesEarly) {
  for (const uint32_t Encoding : {UINT32_C(0x04000F1F), UINT32_C(0x02001001)}) {
    auto Graph = makeCompactUnwindGraph(Target::AArch64, Encoding);
    EXPECT_TRUE(validateCompactUnwind(Graph));
  }
}

TEST(CompactUnwindTest, RejectsMalformedX86_64FramelessEncodings) {
  for (const uint32_t Encoding :
       {UINT32_C(0x02001C00), UINT32_C(0x02000406), UINT32_C(0x03056804)}) {
    auto Graph = makeCompactUnwindGraph(Target::X86_64, Encoding);
    EXPECT_FALSE(validateCompactUnwind(Graph));
    EXPECT_EQ(Graph.sections().size(), 1U);
  }
  auto Truncated = makeCompactUnwindGraph(Target::X86_64, 0x030E6804);
  EXPECT_FALSE(validateCompactUnwind(Truncated));
}

TEST(CompactUnwindTest, RejectsMalformedAArch64FramelessEncoding) {
  auto Reserved = makeCompactUnwindGraph(Target::AArch64, 0x02001020);
  EXPECT_FALSE(validateCompactUnwind(Reserved));
}

TEST(CompactUnwindTest, AcceptsX86DwarfFallbackRelocationAssociation) {
  auto Existing = makeCompactUnwindGraph(Target::X86_64, 0x01000000);
  ASSERT_TRUE(compactUnwindToEHFrame(Existing));
  const auto EH =
      std::find_if(Existing.sections().begin(), Existing.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Existing.sections().end());

  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("x86-dwarf.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 1, 16, 0, 0,
                                       std::vector<WasmEdge::Byte>(16)});
  auto Frame = Graph.addSection(*EH);
  ASSERT_TRUE(Text && Frame);
  auto Function = Graph.addSymbol(Symbol{"_f0", *Text, 0, 16, true, {}, true});
  auto FDE = Graph.addSymbol(Symbol{"fde", *Frame, 16, 0, false});
  ASSERT_TRUE(Function && FDE);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Frame, 24, llvm::MachO::X86_64_RELOC_UNSIGNED, *Function, 0,
                 false, ObjectFormat::MachO, 8}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Function, 16, 0x04000000, {}, {}, *FDE}));
  EXPECT_TRUE(compactUnwindToEHFrame(Graph));
}

TEST(CompactUnwindTest, RejectsX86DwarfFallbackWithoutExactAssociation) {
  auto Graph = makeCompactUnwindGraph(Target::X86_64, 0x04000000);
  const auto Sections = Graph.sections();
  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(Graph.sections().size(), Sections.size());
  EXPECT_EQ(Graph.sections().front().Content, Sections.front().Content);
}

std::vector<WasmEdge::Byte> makeEHFrameBytes() {
  return {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 'z',  'R',
          0x00, 0x01, 0x78, 0x1E, 0x01, 0x10, 0x00, 0x00, 0x00, 0x18, 0x00,
          0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
}

TEST(EHFrameTest, ParsesMinimalFrameAndRejectsMalformedRecords) {
  const auto Valid = makeEHFrameBytes();
  auto Fields = machOEHFrameFields(Valid, Target::X86_64);
  ASSERT_TRUE(Fields);
  EXPECT_EQ(*Fields, (std::set<size_t>{28}));
  EXPECT_EQ(Internal::ehFrameTerminatorOffset(Valid), 48U);
  auto AArch64Alignment = Valid;
  AArch64Alignment[12] = 4;
  EXPECT_TRUE(machOEHFrameFields(AArch64Alignment, Target::AArch64));

  struct Case {
    const char *Name;
    std::function<void(std::vector<WasmEdge::Byte> &)> Mutate;
  };
  const auto Set = [](size_t Offset, WasmEdge::Byte Value) {
    return [Offset, Value](std::vector<WasmEdge::Byte> &Bytes) {
      Bytes[Offset] = Value;
    };
  };
  const auto Resize = [](size_t Size) {
    return [Size](std::vector<WasmEdge::Byte> &Bytes) { Bytes.resize(Size); };
  };
  const std::vector<Case> Cases{
      {"truncated length", Resize(2)},
      {"terminator padding", [](auto &Bytes) { Bytes.push_back(1); }},
      {"DWARF64 length",
       [](auto &Bytes) {
         std::fill(Bytes.begin(), Bytes.begin() + 4, WasmEdge::Byte{0xFF});
       }},
      {"oversized length", Set(1, 1)},
      {"truncated identifier",
       [](auto &Bytes) { Bytes = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00}; }},
      {"version", Set(8, 2)},
      {"augmentation", Set(10, 'X')},
      {"augmentation terminator", Set(11, 1)},
      {"truncated data alignment", Set(0, 9)},
      {"code alignment", Set(12, 4)},
      {"positive data alignment", Set(13, 0x08)},
      {"augmentation length", Set(15, 2)},
      {"pointer encoding", Set(16, 0)},
      {"truncated FDE range", Set(20, 12)},
      {"FDE augmentation", Set(44, 1)},
      {"FDE references itself", Set(24, 4)},
      {"FDE references beyond start", Set(24, 100)},
      {"missing terminator", Resize(48)},
      {"terminator only",
       [](auto &Bytes) { Bytes = {0x00, 0x00, 0x00, 0x00}; }},
      {"CIE only",
       [](auto &Bytes) {
         Bytes.erase(Bytes.begin() + 20, Bytes.begin() + 48);
       }},
  };
  for (const auto &Test : Cases) {
    SCOPED_TRACE(Test.Name);
    auto Bytes = Valid;
    Test.Mutate(Bytes);
    EXPECT_FALSE(machOEHFrameFields(Bytes, Target::X86_64));
  }

  auto Truncated = Valid;
  Truncated.resize(2);
  EXPECT_FALSE(Internal::ehFrameTerminatorOffset(Truncated));
  Truncated = Valid;
  Truncated.resize(48);
  EXPECT_FALSE(Internal::ehFrameTerminatorOffset(Truncated));
}

TEST(EHFrameTest, DecodesStrictULEB128AndRejectsTrailingSLEB128Bytes) {
  std::array<WasmEdge::Byte, 10> Bytes{0x80, 0x80, 0x80, 0x80, 0x80,
                                       0x80, 0x80, 0x80, 0x80, 0x01};
  size_t Offset = 0;
  uint64_t Value = 0;
  ASSERT_TRUE(Internal::readULEB128(Bytes, Offset, Value));
  EXPECT_EQ(Value, UINT64_C(1) << 63);
  EXPECT_EQ(Offset, Bytes.size());
  Bytes.back() = 0x02;
  Offset = 0;
  EXPECT_FALSE(Internal::readULEB128(Bytes, Offset, Value));
  EXPECT_FALSE(
      Internal::decodeSLEB128(std::array<WasmEdge::Byte, 2>{0x00, 0x00}));
}

LinkGraph makeRawEHFrameGraph(uint64_t InputAddress, int64_t Raw, size_t &Field,
                              SectionId &RawSection) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02002000);
  EXPECT_TRUE(compactUnwindToEHFrame(Graph));
  const auto EH = std::find_if(
      Graph.sections().begin(), Graph.sections().end(), [](const auto &Value) {
        return Value.Purpose == SectionPurpose::EHFrame;
      });
  EXPECT_NE(EH, Graph.sections().end());
  if (EH == Graph.sections().end())
    return Graph;
  Section Copy = *EH;
  Copy.Name = "__eh_frame_raw";
  Copy.InputAddress = InputAddress;
  const auto Fields = machOEHFrameFields(Copy.Content, Target::AArch64);
  EXPECT_TRUE(Fields && Fields->size() == 1U);
  if (!Fields || Fields->empty())
    return Graph;
  Field = *Fields->begin();
  EXPECT_TRUE(
      Internal::writeSigned(Copy.Content, Field, 8, Endianness::Little, Raw));
  auto Id = Graph.addSection(std::move(Copy));
  EXPECT_TRUE(Id);
  RawSection = Id ? *Id : InvalidSectionId;
  return Graph;
}

TEST(EHFrameTest, NormalizesUnreferencedRawFDEInitialLocation) {
  constexpr uint64_t InputAddress = 0x8000;
  size_t Field = 0;
  SectionId RawSection = InvalidSectionId;
  int64_t Raw = 0x1000 - static_cast<int64_t>(InputAddress);
  auto Graph = makeRawEHFrameGraph(InputAddress, 0, Field, RawSection);
  ASSERT_NE(RawSection, InvalidSectionId);
  Raw -= static_cast<int64_t>(Field);
  Graph = makeRawEHFrameGraph(InputAddress, Raw, Field, RawSection);
  ASSERT_NE(RawSection, InvalidSectionId);
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  const auto &Text = Graph.sections()[0];
  const auto &Normalized = Graph.sections()[RawSection];
  EXPECT_EQ(
      Internal::readSigned(Normalized.Content, Field, 8, Endianness::Little),
      static_cast<int64_t>(Text.Address) -
          static_cast<int64_t>(Normalized.Address + Field));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));

  for (const auto &[Address, Delta] :
       {std::pair<uint64_t, int64_t>{InputAddress, 0x7777},
        std::pair<uint64_t, int64_t>{0, -static_cast<int64_t>(Field) - 1}}) {
    auto Unresolved = makeRawEHFrameGraph(Address, Delta, Field, RawSection);
    ASSERT_NE(RawSection, InvalidSectionId);
    const auto Before = Unresolved.sections()[RawSection].Content;
    EXPECT_FALSE(normalizeMachOEHFrame(Unresolved));
    EXPECT_EQ(Unresolved.sections()[RawSection].Content, Before);
  }
}

TEST(EHFrameTest, NormalizesImplicitFDERelocationAndRejectsConflicts) {
  size_t Field = 0;
  SectionId RawSection = InvalidSectionId;
  auto Graph = makeRawEHFrameGraph(0x8000, 4, Field, RawSection);
  ASSERT_NE(RawSection, InvalidSectionId);
  Relocation Implicit{
      RawSection,          Field, llvm::MachO::ARM64_RELOC_UNSIGNED, 0, 0, true,
      ObjectFormat::MachO, 8};
  ASSERT_TRUE(Graph.addRelocation(Implicit));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(Graph.relocations().empty());
  const auto &Normalized = Graph.sections()[RawSection];
  EXPECT_EQ(
      Internal::readSigned(Normalized.Content, Field, 8, Endianness::Little),
      static_cast<int64_t>(Graph.sections()[0].Address + 4) -
          static_cast<int64_t>(Normalized.Address + Field));

  auto Duplicate = makeRawEHFrameGraph(0x8000, 0, Field, RawSection);
  ASSERT_NE(RawSection, InvalidSectionId);
  const auto Referenced = Duplicate.ehFrameReferences().front();
  ASSERT_TRUE(Duplicate.addRelocation(Relocation{
      Referenced.Section, Referenced.Offset, llvm::MachO::ARM64_RELOC_UNSIGNED,
      0, 0, false, ObjectFormat::MachO, 8}));
  EXPECT_FALSE(normalizeMachOEHFrame(Duplicate));
  EXPECT_EQ(Duplicate.relocations().size(), 1U);

  auto Moved = makeRawEHFrameGraph(0x8000, 0, Field, RawSection);
  ASSERT_NE(RawSection, InvalidSectionId);
  auto Other = Moved.addSymbol(Symbol{"_other", 0, 4, 0, false});
  ASSERT_TRUE(Other);
  Implicit.Section = RawSection;
  Implicit.Offset = Field;
  Implicit.Symbol = *Other;
  ASSERT_TRUE(Moved.addRelocation(Implicit));
  const_cast<std::vector<Symbol> &>(Moved.symbols())[*Other].Section =
      InvalidSectionId;
  EXPECT_FALSE(normalizeMachOEHFrame(Moved));
  EXPECT_EQ(Moved.relocations().size(), 1U);
}

TEST(EHFrameTest, RejectsNormalizationInvalidatingOtherPatches) {
  struct Case {
    const char *Name;
    std::function<void(LinkGraph &)> Mutate;
  };
  const std::array<Case, 4> Cases{{
      {"invalid relocation symbol",
       [](LinkGraph &Graph) {
         const_cast<std::vector<Relocation> &>(Graph.relocations())[0].Symbol =
             InvalidSymbolId;
       }},
      {"duplicate relocation offset",
       [](LinkGraph &Graph) {
         const_cast<std::vector<Relocation> &>(Graph.relocations())[1].Offset =
             0;
       }},
      {"overlapping relocation",
       [](LinkGraph &Graph) {
         const_cast<std::vector<Relocation> &>(Graph.relocations())[1].Offset =
             4;
       }},
      {"rebase outside content",
       [](LinkGraph &Graph) {
         const_cast<std::vector<Rebase> &>(Graph.rebases())[0].Offset = 16;
       }},
  }};
  for (const auto &Test : Cases) {
    SCOPED_TRACE(Test.Name);
    auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02002000);
    ASSERT_TRUE(compactUnwindToEHFrame(Graph));
    auto Data = Graph.addSection(Section{"__data", SectionKind::Data, 8, 16, 0,
                                         0, std::vector<WasmEdge::Byte>(16)});
    ASSERT_TRUE(Data);
    for (const uint64_t Offset : {UINT64_C(0), UINT64_C(8)}) {
      ASSERT_TRUE(Graph.addRelocation(
          Relocation{*Data, Offset, llvm::MachO::ARM64_RELOC_UNSIGNED, 0, 0,
                     false, ObjectFormat::MachO, 8}));
    }
    ASSERT_TRUE(
        Graph.addRebase(Rebase{*Data, 0, 0, 0, 8, ObjectFormat::MachO}));
    Test.Mutate(Graph);
    const auto EH =
        std::find_if(Graph.sections().begin(), Graph.sections().end(),
                     [](const auto &Value) {
                       return Value.Purpose == SectionPurpose::EHFrame;
                     });
    ASSERT_NE(EH, Graph.sections().end());
    const auto Before = EH->Content;
    EXPECT_FALSE(normalizeMachOEHFrame(Graph));
    EXPECT_EQ(EH->Content, Before);
    EXPECT_EQ(Graph.relocations().size(), 2U);
  }
}

TEST(EHFrameTest, RejectsInvalidCoverageSymbolsAndMissingFrames) {
  const auto Make = [] {
    LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
    EXPECT_TRUE(Graph.beginInput("coverage.o"));
    auto Text =
        Graph.addSection(Section{"__text", SectionKind::Text, 4, 16, 0, 0, {}});
    auto UnwindInfo = Graph.addSection(Section{"__unwind_info",
                                               SectionKind::Unwind,
                                               4,
                                               4,
                                               0,
                                               0,
                                               {},
                                               SectionPurpose::UnwindInfo});
    EXPECT_TRUE(Text && UnwindInfo);
    auto Function =
        Graph.addSymbol(Symbol{"_f0", *Text, 0, 4, true, std::nullopt, true});
    EXPECT_TRUE(Function);
    EXPECT_TRUE(Graph.addCompactUnwind(
        CompactUnwindRecord{*Function, 4, 0x02000000, {}, {}, {}}));
    return Graph;
  };
  auto Covered = Make();
  EXPECT_TRUE(validateMachOEHFrameCoverage(Covered));
  EXPECT_FALSE(machOEHFrameStarts(Covered, 0));

  auto InvalidFunction = Make();
  const_cast<std::vector<CompactUnwindRecord> &>(
      InvalidFunction.compactUnwind())[0]
      .Function = InvalidSymbolId;
  EXPECT_FALSE(validateMachOEHFrameCoverage(InvalidFunction));

  auto InvalidSection = Make();
  const_cast<std::vector<Symbol> &>(InvalidSection.symbols())[0].Section =
      InvalidSectionId;
  EXPECT_FALSE(validateMachOEHFrameCoverage(InvalidSection));

  auto InvalidSemantic = Make();
  ASSERT_TRUE(InvalidSemantic.addSymbol(
      Symbol{"_f1", 0, 8, 0, true, std::nullopt, true}));
  const_cast<std::vector<Symbol> &>(InvalidSemantic.symbols())[1].Section =
      InvalidSectionId;
  EXPECT_FALSE(validateMachOEHFrameCoverage(InvalidSemantic));

  auto Overflow = Make();
  ASSERT_TRUE(
      Overflow.addSymbol(Symbol{"_f1", 0, 8, 0, true, std::nullopt, true}));
  ASSERT_TRUE(Overflow.setSectionAddress(0, UINT64_MAX - 3));
  EXPECT_FALSE(validateMachOEHFrameCoverage(Overflow));

  LinkGraph ELF(Target::AArch64, Endianness::Little);
  ASSERT_TRUE(ELF.beginInput("elf.o"));
  ASSERT_TRUE(ELF.addSection(Section{".eh_frame",
                                     SectionKind::Unwind,
                                     8,
                                     4,
                                     0,
                                     0,
                                     {1, 2, 3, 4},
                                     SectionPurpose::EHFrame}));
  EXPECT_TRUE(normalizeMachOEHFrame(ELF));
  EXPECT_TRUE(validateMachOEHFrameCoverage(ELF));
  EXPECT_EQ(ELF.sections()[0].Content,
            (std::vector<WasmEdge::Byte>{1, 2, 3, 4}));
}

} // namespace
