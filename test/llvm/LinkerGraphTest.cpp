// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include <llvm/Support/JSON.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "LinkerTestHelpers.h"
#include "LinkerTestUtils.h"
#include "linker/compact_unwind.h"
#include "linker/layout.h"
#include "linker/link_graph.h"
#include "linker/object_reader.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"
#include "linker/thunks.h"

#include <gtest/gtest.h>

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/MachO.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <llvm/Object/ELFObjectFile.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using namespace WasmEdge::LLVM::Linker;
using namespace WasmEdge::LLVM::Test;

TEST(LinkGraphTest, OwnsContentAndUsesStableIds) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  std::vector<uint8_t> Content{0x48, 0x89, 0xE5};
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 16, 3, 0, 0, Content});
  ASSERT_TRUE(Text);
  EXPECT_EQ(*Text, 0U);

  Content[0] = 0;
  ASSERT_EQ(Graph.sections().size(), 1U);
  EXPECT_EQ(Graph.sections()[*Text].Content[0], 0x48U);

  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 4, 0, 0, {1, 2, 3, 4}});
  ASSERT_TRUE(Data);
  EXPECT_EQ(*Data, 1U);
  EXPECT_EQ(*Text, 0U);

  auto Entry = Graph.addSymbol(Symbol{"entry", *Text, 0, 3, true});
  ASSERT_TRUE(Entry);
  EXPECT_EQ(*Entry, 0U);
  EXPECT_EQ(Graph.target(), Target::X86_64);
  EXPECT_EQ(Graph.endianness(), Endianness::Little);
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsSecondInputObject) {
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("first.o"));
  auto Result = Graph.beginInput("second.o");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "link graph accepts exactly one input object");
}

TEST(LinkGraphTest, RejectsInvalidSectionAlignment) {
  LinkGraph Graph(Target::AArch64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  auto Zero = Graph.addSection(Section{"zero", SectionKind::Data, 0, 0});
  ASSERT_FALSE(Zero);
  EXPECT_EQ(Zero.error().SectionName, "zero");

  auto NonPowerOfTwo =
      Graph.addSection(Section{"three", SectionKind::Data, 3, 0});
  ASSERT_FALSE(NonPowerOfTwo);
  EXPECT_EQ(NonPowerOfTwo.error().Message,
            "section alignment must be a non-zero power of two");
}

TEST(LinkGraphTest, RejectsContentBeyondSectionVirtualSize) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  auto Result =
      Graph.addSection(Section{".data", SectionKind::Data, 1, 1, 0, 0, {1, 2}});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().SectionName, ".data");
  EXPECT_EQ(Result.error().Message,
            "section content exceeds section virtual size");
}

TEST(LinkGraphTest, RejectsDuplicateSymbolDefinitions) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"same", *Text, 0, 1, false}));

  auto Duplicate = Graph.addSymbol(Symbol{"same", *Text, 1, 1, false});
  ASSERT_FALSE(Duplicate);
  EXPECT_EQ(Duplicate.error().SymbolName, "same");
  EXPECT_EQ(Duplicate.error().Message, "duplicate symbol definition");
}

TEST(LinkGraphTest, RejectsDuplicateSymbolsAcrossGrowthCopiesAndMoves) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Bss = Graph.addSection(Section{".bss", SectionKind::BSS, 1, 1});
  ASSERT_TRUE(Bss);

  constexpr uint32_t SymbolCount = 20000;
  for (uint32_t I = 0; I < SymbolCount; ++I) {
    auto Added = Graph.addSymbol(
        Symbol{"symbol" + std::to_string(I), *Bss, 0, 0, false});
    ASSERT_TRUE(Added);
    ASSERT_EQ(*Added, I);
  }
  EXPECT_FALSE(Graph.addSymbol(Symbol{"symbol0", *Bss, 0, 0, false}));
  EXPECT_FALSE(Graph.addSymbol(Symbol{"symbol19999", *Bss, 0, 0, false}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"missing", *Bss, 0, 0, false}));

  LinkGraph Copied = Graph;
  EXPECT_FALSE(Copied.addSymbol(Symbol{"symbol12345", *Bss, 0, 0, false}));

  LinkGraph Moved = std::move(Graph);
  EXPECT_FALSE(Moved.addSymbol(Symbol{"symbol5432", *Bss, 0, 0, false}));
}

TEST(LinkGraphTest, ResetsMovedFromGraphForReuse) {
  const auto Populate = [](LinkGraph &Graph, std::string_view Input) {
    EXPECT_TRUE(Graph.beginInput(Input));
    auto Text = Graph.addSection(Section{
        ".text", SectionKind::Text, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
    EXPECT_TRUE(Text);
    if (!Text)
      return;
    auto Target = Graph.addSymbol(Symbol{"target", *Text, 0, 4, false});
    EXPECT_TRUE(Target);
    if (!Target)
      return;
    EXPECT_TRUE(Graph.addRelocation(
        Relocation{*Text, 0, 2, *Target, 0, false, ObjectFormat::ELF, 4}));
    EXPECT_TRUE(Graph.addRebase(Rebase{*Text, 4, 0, 0, 4}));
    EXPECT_TRUE(Graph.validate());
  };

  LinkGraph Source(Target::X86_64, Endianness::Little);
  Populate(Source, "source.o");
  LinkGraph Moved(std::move(Source));
  EXPECT_TRUE(Moved.validate());
  EXPECT_FALSE(Source.validate());
  EXPECT_TRUE(Source.sections().empty());
  EXPECT_TRUE(Source.symbols().empty());
  EXPECT_TRUE(Source.relocations().empty());
  EXPECT_TRUE(Source.rebases().empty());
  Populate(Source, "reused-constructor-source.o");

  LinkGraph Assigned(Target::X86_64, Endianness::Little);
  Populate(Assigned, "discarded.o");
  Assigned = std::move(Source);
  EXPECT_TRUE(Assigned.validate());
  EXPECT_FALSE(Source.validate());
  EXPECT_TRUE(Source.sections().empty());
  EXPECT_TRUE(Source.symbols().empty());
  EXPECT_TRUE(Source.relocations().empty());
  EXPECT_TRUE(Source.rebases().empty());
  Populate(Source, "reused-assignment-source.o");
}

TEST(LinkGraphTest, CopyAndSelfAssignmentPreserveCoherentGraph) {
  LinkGraph Source(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Source.beginInput("source.o"));
  auto Text = Source.addSection(Section{
      ".text", SectionKind::Text, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Source.addSymbol(Symbol{"target", *Text, 0, 4, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Source.addRelocation(
      Relocation{*Text, 0, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Source.addRebase(Rebase{*Text, 4, 0, 0, 4}));

  LinkGraph Destination(Target::ARM, Endianness::Big, ObjectFormat::COFF);
  ASSERT_TRUE(Destination.beginInput("destination.o"));
  ASSERT_TRUE(Destination.addSection(
      Section{"old", SectionKind::Data, 1, 1, 0, 0, {1}}));
  Destination = Source;
  EXPECT_TRUE(Destination.validate());
  EXPECT_EQ(Destination.target(), Source.target());
  EXPECT_EQ(Destination.endianness(), Source.endianness());
  EXPECT_EQ(Destination.format(), Source.format());
  EXPECT_EQ(Destination.sections().size(), Source.sections().size());
  EXPECT_EQ(Destination.symbols().size(), Source.symbols().size());
  EXPECT_EQ(Destination.relocations().size(), Source.relocations().size());
  EXPECT_EQ(Destination.rebases().size(), Source.rebases().size());
  EXPECT_FALSE(Destination.addSymbol(Symbol{"target", *Text, 0, 4, false}));

  auto &Alias = Source;
  Source = Alias;
  EXPECT_TRUE(Source.validate());
}

TEST(LinkGraphTest, StoresMachOBuildVersionsInInsertionOrder) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.addMachOBuildVersion(
      MachOBuildVersion{1, 0x000D0000, 0x000E0000, {{3, 0x004A0000}}}));
  ASSERT_TRUE(Graph.addMachOBuildVersion(MachOBuildVersion{
      2, 0x000F0100, 0x00100000, {{1, 0x000F0000}, {4, 0x00100000}}}));

  ASSERT_EQ(Graph.machOBuildVersions().size(), 2U);
  const auto &First = Graph.machOBuildVersions()[0];
  EXPECT_EQ(First.Platform, 1U);
  EXPECT_EQ(First.MinimumOS, 0x000D0000U);
  EXPECT_EQ(First.SDK, 0x000E0000U);
  ASSERT_EQ(First.Tools.size(), 1U);
  EXPECT_EQ(First.Tools[0].Tool, 3U);
  EXPECT_EQ(First.Tools[0].Version, 0x004A0000U);
  const auto &Second = Graph.machOBuildVersions()[1];
  EXPECT_EQ(Second.Platform, 2U);
  ASSERT_EQ(Second.Tools.size(), 2U);
  EXPECT_EQ(Second.Tools[0].Tool, 1U);
  EXPECT_EQ(Second.Tools[1].Tool, 4U);

  LinkGraph ELFGraph(Target::X86_64, Endianness::Little, ObjectFormat::ELF);
  auto Rejected = ELFGraph.addMachOBuildVersion(MachOBuildVersion{});
  ASSERT_FALSE(Rejected);
  EXPECT_EQ(Rejected.error().Message,
            "Mach-O build version requires a Mach-O link graph");
  EXPECT_TRUE(ELFGraph.machOBuildVersions().empty());
}

TEST(LinkGraphTest, RejectsMachOBuildVersionAfterRelocations) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(applyRelocations(Graph));

  auto Result = Graph.addMachOBuildVersion(
      MachOBuildVersion{1, 0x000D0000, 0x000E0000, {}});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "link graph relocations already applied");
  EXPECT_TRUE(Graph.machOBuildVersions().empty());
}

TEST(LinkGraphTest, PreservesMachOBuildVersionWhenPruningEHFrame) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{"__text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  auto Function =
      Graph.addSymbol(Symbol{"_f", *Text, 0, 1, false, std::nullopt, true});
  ASSERT_TRUE(Function);
  ASSERT_TRUE(Graph.addSection(Section{"__eh_frame",
                                       SectionKind::Unwind,
                                       1,
                                       1,
                                       0,
                                       0,
                                       {0},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{
      *Function, 1, 0, std::nullopt, std::nullopt, std::nullopt}));
  ASSERT_TRUE(Graph.addMachOBuildVersion(
      MachOBuildVersion{1, 0x000D0000, 0x000E0000, {{3, 0x004A0000}}}));

  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());
  ASSERT_EQ(Graph.sections().size(), 1U);
  ASSERT_EQ(Graph.machOBuildVersions().size(), 1U);
  EXPECT_EQ(Graph.machOBuildVersions()[0].Platform, 1U);
  ASSERT_EQ(Graph.machOBuildVersions()[0].Tools.size(), 1U);
  EXPECT_EQ(Graph.machOBuildVersions()[0].Tools[0].Version, 0x004A0000U);
}

TEST(LinkGraphTest, CopiesMovesAndSwapsMachOBuildVersions) {
  const MachOBuildVersion Version{
      1, 0x000D0000, 0x000E0000, {{3, 0x004A0000}, {4, 0x000E0100}}};
  LinkGraph Source(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Source.addMachOBuildVersion(Version));

  LinkGraph Copied(Source);
  ASSERT_EQ(Copied.machOBuildVersions().size(), 1U);
  EXPECT_EQ(Copied.machOBuildVersions()[0].Tools[1].Version, 0x000E0100U);
  LinkGraph CopyAssigned(Target::X86_64, Endianness::Little,
                         ObjectFormat::MachO);
  CopyAssigned = Source;
  ASSERT_EQ(CopyAssigned.machOBuildVersions().size(), 1U);
  EXPECT_EQ(CopyAssigned.machOBuildVersions()[0].MinimumOS, 0x000D0000U);

  LinkGraph Moved(std::move(Source));
  ASSERT_EQ(Moved.machOBuildVersions().size(), 1U);
  EXPECT_TRUE(Source.machOBuildVersions().empty());
  EXPECT_TRUE(Source.addMachOBuildVersion(
      MachOBuildVersion{2, 0x000F0000, 0x00100000, {}}));

  LinkGraph MoveAssigned(Target::X86_64, Endianness::Little,
                         ObjectFormat::MachO);
  MoveAssigned = std::move(Source);
  ASSERT_EQ(MoveAssigned.machOBuildVersions().size(), 1U);
  EXPECT_EQ(MoveAssigned.machOBuildVersions()[0].Platform, 2U);
  EXPECT_TRUE(Source.machOBuildVersions().empty());
  EXPECT_TRUE(Source.addMachOBuildVersion(Version));

  Moved.swap(MoveAssigned);
  EXPECT_EQ(Moved.machOBuildVersions()[0].Platform, 2U);
  EXPECT_EQ(MoveAssigned.machOBuildVersions()[0].Platform, 1U);
}

TEST(LinkGraphTest, RejectsUndefinedSymbols) {
  LinkGraph Graph(Target::S390X, Endianness::Big);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Result =
      Graph.addSymbol(Symbol{"missing", InvalidSectionId, 0, 0, false});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().SymbolName, "missing");
  EXPECT_EQ(Result.error().Message, "undefined symbol");
  EXPECT_TRUE(Graph.symbols().empty());
}

TEST(LinkGraphTest, RejectsZeroInputObjects) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "link graph requires one input object");
}

TEST(LinkGraphTest, RejectsInvalidSectionIds) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  auto Result = Graph.addSymbol(Symbol{"bad", 7, 0, 0, false});
  ASSERT_FALSE(Result);
  ASSERT_TRUE(Result.error().Section);
  EXPECT_EQ(*Result.error().Section, 7U);
  EXPECT_EQ(Result.error().Message, "invalid section ID");
}

TEST(LinkGraphTest, RejectsSymbolsBeyondSectionVirtualSize) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Bss = Graph.addSection(Section{".bss", SectionKind::BSS, 8, 8});
  ASSERT_TRUE(Bss);

  auto Result = Graph.addSymbol(Symbol{"too_large", *Bss, 7, 2, false});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().SymbolName, "too_large");
  EXPECT_EQ(Result.error().Offset, 7U);
  EXPECT_EQ(Result.error().Message,
            "symbol extends beyond section virtual size");
}

TEST(LinkGraphTest, StoresRelocationsAndRebases) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 4, false});
  ASSERT_TRUE(TargetSymbol);

  Relocation Stored{*Text, 0, 42, *TargetSymbol, -4};
  Stored.PatchSize = 4;
  ASSERT_TRUE(Graph.addRelocation(Stored));
  ASSERT_TRUE(Graph.addRebase(Rebase{*Text, 2, 7, 8}));
  ASSERT_EQ(Graph.relocations().size(), 1U);
  EXPECT_EQ(Graph.relocations()[0].Symbol, *TargetSymbol);
  ASSERT_EQ(Graph.rebases().size(), 1U);
  EXPECT_EQ(Graph.rebases()[0].Addend, 8);
}

TEST(LinkGraphTest, RejectsOverlappingRelocationsRegardlessOfOrder) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("input.o"));
    auto Text = Graph.addSection(Section{
        ".text", SectionKind::Text, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
    ASSERT_TRUE(Text);
    auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
    ASSERT_TRUE(TargetSymbol);
    Relocation First{*Text, 1, 2, *TargetSymbol, 0};
    First.PatchSize = 4;
    Relocation Second{*Text, 3, 2, *TargetSymbol, 0};
    Second.PatchSize = 4;
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Second : First));
    ASSERT_TRUE(Graph.addRelocation(Reverse ? First : Second));
    auto Result = Graph.validate();
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
  }
}

TEST(LinkGraphTest, AllowsUnsortedAdjacentRelocations) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{".text",
                                       SectionKind::Text,
                                       1,
                                       12,
                                       0,
                                       0,
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  for (const uint64_t Offset : {UINT64_C(8), UINT64_C(0), UINT64_C(4)}) {
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Text, Offset, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  }
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsRelocationOverlappingLaterSuccessor) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text",
              SectionKind::Text,
              1,
              16,
              0,
              0,
              {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 12, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));

  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, 1, *TargetSymbol, 0, false, ObjectFormat::ELF, 8}));
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
}

TEST(LinkGraphTest, AllowsZeroWidthMetadataOverlapInEitherOrder) {
  for (const bool MetadataFirst : {false, true}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("input.o"));
    auto Text = Graph.addSection(
        Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Text);
    auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    Relocation Patch{*Text,
                     0,
                     llvm::ELF::R_RISCV_PCREL_HI20,
                     *TargetSymbol,
                     0,
                     false,
                     ObjectFormat::ELF,
                     4};
    Relocation Metadata{*Text, 0,     llvm::ELF::R_RISCV_RELAX, *TargetSymbol,
                        0,     false, ObjectFormat::ELF,        NoPatch};
    ASSERT_TRUE(Graph.addRelocation(MetadataFirst ? Metadata : Patch));
    ASSERT_TRUE(Graph.addRelocation(MetadataFirst ? Patch : Metadata));
    EXPECT_TRUE(Graph.validate());
  }
}

TEST(LinkGraphTest, AllowsTwoZeroWidthMetadataRecordsAtOneSite) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Metadata{*Text, 0,     llvm::ELF::R_RISCV_RELAX, *TargetSymbol,
                      0,     false, ObjectFormat::ELF,        NoPatch};
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, AllowsNoPatchRecordsAmongUnsortedRelocations) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  const Relocation Metadata{
      *Text, 2,     llvm::ELF::R_RISCV_RELAX, *TargetSymbol,
      0,     false, ObjectFormat::ELF,        NoPatch};
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, AllowsOnlyCompleteRISCVSymbolDifferencePairs) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("difference.o"));
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    const Relocation Add{*Data, 0,     llvm::ELF::R_RISCV_ADD32, *TargetSymbol,
                         0,     false, ObjectFormat::ELF,        4};
    const Relocation Sub{*Data, 0,     llvm::ELF::R_RISCV_SUB32, *TargetSymbol,
                         0,     false, ObjectFormat::ELF,        4};
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Sub : Add));
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Add : Sub));
    EXPECT_TRUE(Graph.validate());
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Sub : Add));
    auto Duplicate = Graph.validate();
    ASSERT_FALSE(Duplicate);
    EXPECT_EQ(Duplicate.error().Message, "overlapping relocation patches");
  }

  LinkGraph Unmatched(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Unmatched.beginInput("unmatched.o"));
  auto Data = Unmatched.addSection(
      Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  auto TargetSymbol = Unmatched.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Unmatched.addRelocation(
      Relocation{*Data, 0, llvm::ELF::R_RISCV_ADD32, *TargetSymbol, 0, false,
                 ObjectFormat::ELF, 4}));
  auto Valid = Unmatched.validate();
  ASSERT_FALSE(Valid);
  EXPECT_EQ(Valid.error().Message, "unpaired RISC-V symbol difference");

  LinkGraph WrongWidth(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(WrongWidth.beginInput("width.o"));
  Data = WrongWidth.addSection(Section{
      ".data", SectionKind::Data, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  TargetSymbol = WrongWidth.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  auto Added = WrongWidth.addRelocation(
      Relocation{*Data, 0, llvm::ELF::R_RISCV_ADD32, *TargetSymbol, 0, false,
                 ObjectFormat::ELF, 8});
  ASSERT_FALSE(Added);
  EXPECT_EQ(Added.error().Message, "invalid relocation patch size");
  EXPECT_TRUE(WrongWidth.relocations().empty());
}

TEST(LinkGraphTest, RejectsEveryRISCVSymbolDifferenceTriplePermutation) {
  const std::array<std::array<uint32_t, 3>, 6> Cases{{
      {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_ADD32,
       llvm::ELF::R_RISCV_SUB32},
      {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_SUB32,
       llvm::ELF::R_RISCV_ADD32},
      {llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_ADD32,
       llvm::ELF::R_RISCV_ADD32},
      {llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_SUB32,
       llvm::ELF::R_RISCV_ADD32},
      {llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_ADD32,
       llvm::ELF::R_RISCV_SUB32},
      {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_SUB32,
       llvm::ELF::R_RISCV_SUB32},
  }};
  for (const auto &Types : Cases) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("difference.o"));
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    for (const uint32_t Type : Types) {
      ASSERT_TRUE(Graph.addRelocation(Relocation{
          *Data, 0, Type, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
    }
    auto Result = Graph.validate();
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
  }
}

TEST(LinkGraphTest, EnforcesCanonicalX86RelocationPatchSizes) {
  struct Case {
    ObjectFormat Format;
    uint32_t Type;
    uint8_t Width;
  };
  const std::array<Case, 7> Cases{{
      {ObjectFormat::ELF, 1, 8},
      {ObjectFormat::ELF, 2, 4},
      {ObjectFormat::ELF, 4, 4},
      {ObjectFormat::ELF, 41, 4},
      {ObjectFormat::ELF, 42, 4},
      {ObjectFormat::MachO, 1, 4},
      {ObjectFormat::COFF, 4, 4},
  }};
  for (const auto &Test : Cases) {
    for (const uint8_t Width :
         {uint8_t{0}, uint8_t{1}, uint8_t{4}, uint8_t{8}}) {
      LinkGraph Graph(Target::X86_64, Endianness::Little);
      ASSERT_TRUE(Graph.beginInput("input.o"));
      auto Text = Graph.addSection(
          Section{".text",
                  SectionKind::Text,
                  1,
                  16,
                  0,
                  0,
                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
      ASSERT_TRUE(Text);
      auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
      ASSERT_TRUE(TargetSymbol);
      Relocation Value{*Text, 0, Test.Type, *TargetSymbol, 0};
      Value.Format = Test.Format;
      Value.PatchSize = Width;
      auto Result = Graph.addRelocation(Value);
      EXPECT_EQ(static_cast<bool>(Result), Width == Test.Width);
      if (!Result) {
        EXPECT_EQ(Result.error().Message, "invalid relocation patch size");
      }
    }
  }
}

TEST(LinkGraphTest, RejectsUnsupportedX86RelocationBeforeRangeValidation) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Unknown{*Text, 0, 0xFFFF, *TargetSymbol, 0};
  Unknown.PatchSize = 1;
  auto Result = Graph.addRelocation(Unknown);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "unsupported relocation patch size");
}

TEST(LinkGraphTest, WrongPatchSizeCannotBypassOverlapDetection) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text",
              SectionKind::Text,
              1,
              16,
              0,
              0,
              {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation First{*Text, 0, 1, *TargetSymbol, 0};
  First.PatchSize = 8;
  ASSERT_TRUE(Graph.addRelocation(First));
  Relocation Bypass{*Text, 7, 2, *TargetSymbol, 0};
  Bypass.PatchSize = 1;
  auto Result = Graph.addRelocation(Bypass);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "invalid relocation patch size");
}

TEST(LinkGraphTest, ValidationRejectsMutatedRelocationPatchSize) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Value{*Text, 0, 1, *TargetSymbol, 0};
  Value.PatchSize = 8;
  ASSERT_TRUE(Graph.addRelocation(Value));
  const_cast<std::vector<Relocation> &>(Graph.relocations())[0].PatchSize = 1;
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "invalid relocation patch size");
}

TEST(LinkGraphTest, ValidationRejectsMutatedNonAdjacentRelocationOverlap) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{".text",
                                       SectionKind::Text,
                                       1,
                                       12,
                                       0,
                                       0,
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  for (const uint64_t Offset : {UINT64_C(0), UINT64_C(4), UINT64_C(8)}) {
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Text, Offset, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  }

  const_cast<std::vector<Relocation> &>(Graph.relocations())[2].Offset = 2;
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
}

TEST(LinkGraphTest, ValidationRejectsMutatedRISCVSymbolDifferenceTriple) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  const Relocation Add{*Data, 0,     llvm::ELF::R_RISCV_ADD32, *TargetSymbol,
                       0,     false, ObjectFormat::ELF,        4};
  const Relocation Sub{*Data, 0,     llvm::ELF::R_RISCV_SUB32, *TargetSymbol,
                       0,     false, ObjectFormat::ELF,        4};
  ASSERT_TRUE(Graph.addRelocation(Add));
  ASSERT_TRUE(Graph.addRelocation(Sub));
  const_cast<std::vector<Relocation> &>(Graph.relocations()).push_back(Add);

  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
}

TEST(LinkGraphTest, RejectsOverlappingRebasesRegardlessOfOrder) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("input.o"));
    auto Data = Graph.addSection(Section{".data",
                                         SectionKind::Data,
                                         1,
                                         12,
                                         0,
                                         0,
                                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
    ASSERT_TRUE(Data);
    const Rebase First{*Data, 1, 1, 0, 8};
    const Rebase Second{*Data, 7, 1, 0, 4};
    ASSERT_TRUE(Graph.addRebase(Reverse ? Second : First));
    ASSERT_TRUE(Graph.addRebase(Reverse ? First : Second));
    auto Result = Graph.validate();
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "overlapping rebase patches");
  }
}

TEST(LinkGraphTest, AllowsUnsortedAdjacentRebases) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(Section{".data",
                                       SectionKind::Data,
                                       1,
                                       12,
                                       0,
                                       0,
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  for (const uint64_t Offset : {UINT64_C(8), UINT64_C(0), UINT64_C(4)})
    ASSERT_TRUE(Graph.addRebase(Rebase{*Data, Offset, 1, 0, 4}));
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, ValidationRejectsMutatedNonAdjacentRebaseOverlap) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(Section{".data",
                                       SectionKind::Data,
                                       1,
                                       12,
                                       0,
                                       0,
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  for (const uint64_t Offset : {UINT64_C(0), UINT64_C(4), UINT64_C(8)})
    ASSERT_TRUE(Graph.addRebase(Rebase{*Data, Offset, 1, 0, 4}));

  const_cast<std::vector<Rebase> &>(Graph.rebases())[2].Offset = 2;
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping rebase patches");
}

TEST(LinkGraphTest, ValidatesLargeSymbolAndRelocationGraph) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("stress.o"));
  constexpr uint32_t RecordCount = 20000;
  std::vector<WasmEdge::Byte> Content(RecordCount * 4);
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 4, Content.size(), 0, 0, std::move(Content)});
  ASSERT_TRUE(Data);
  for (uint32_t I = 0; I < RecordCount; ++I) {
    auto SymbolValue = Graph.addSymbol(
        Symbol{"stress" + std::to_string(I), *Data, I * 4, 0, false});
    ASSERT_TRUE(SymbolValue);
    ASSERT_TRUE(Graph.addRelocation(Relocation{*Data, I * 4, 2, *SymbolValue, 0,
                                               false, ObjectFormat::ELF, 4}));
  }

  EXPECT_TRUE(Graph.validate());
  EXPECT_FALSE(Graph.addSymbol(Symbol{"stress19999", *Data, 0, 0, false}));
}

TEST(LinkGraphTest, RejectsRebaseOverlappingLaterSuccessor) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 24, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  ASSERT_TRUE(Graph.addRebase(Rebase{*Data, 8, 1, 0, 4}));
  ASSERT_TRUE(Graph.addRebase(Rebase{*Data, 20, 1, 0, 4}));

  ASSERT_TRUE(Graph.addRebase(Rebase{*Data, 0, 1, 0, 12}));
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping rebase patches");
}

TEST(LinkGraphTest, RejectsInvalidPatchSections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);

  auto RelocationResult = Graph.addRelocation(
      Relocation{InvalidSectionId, 0, 42, *TargetSymbol, 0});
  ASSERT_FALSE(RelocationResult);
  EXPECT_EQ(RelocationResult.error().Message, "invalid section ID");
  auto RebaseResult = Graph.addRebase(Rebase{InvalidSectionId, 0, 7, 0});
  ASSERT_FALSE(RebaseResult);
  EXPECT_EQ(RebaseResult.error().Message, "invalid section ID");
}

TEST(LinkGraphTest, RejectsPatchOffsetsOutsideSectionContent) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 4, 0, 0, {0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);

  Relocation Outside{*Text, 1, 42, *TargetSymbol, 0};
  Outside.PatchSize = 4;
  auto RelocationResult = Graph.addRelocation(Outside);
  ASSERT_FALSE(RelocationResult);
  EXPECT_EQ(RelocationResult.error().Offset, 1U);
  EXPECT_EQ(RelocationResult.error().Message,
            "relocation offset is outside section content");
  auto RebaseResult = Graph.addRebase(Rebase{*Text, 1, 7, 0});
  ASSERT_FALSE(RebaseResult);
  EXPECT_EQ(RebaseResult.error().Offset, 1U);
  EXPECT_EQ(RebaseResult.error().Message,
            "rebase offset is outside section content");
}

TEST(LinkGraphTest, ProvidesSectionPlacementMutators) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 1, 5, 0, 0, {0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Mutable{*Text, 0, 42, *TargetSymbol, 0};
  Mutable.PatchSize = 4;
  ASSERT_TRUE(Graph.addRelocation(Mutable));
  ASSERT_TRUE(Graph.addRebase(Rebase{*Text, 4, 7, 0}));

  ASSERT_TRUE(Graph.setSectionAddress(*Text, 64));
  ASSERT_TRUE(Graph.setSectionFileOffset(*Text, 32));
  const_cast<std::vector<Relocation> &>(Graph.relocations())[0].Addend = 8;
  const_cast<std::vector<Rebase> &>(Graph.rebases())[0].Addend = 16;
  EXPECT_EQ(Graph.sections()[*Text].Address, 64U);
  EXPECT_EQ(Graph.sections()[*Text].FileOffset, 32U);
  EXPECT_EQ(Graph.sections()[*Text].Content[0], 0U);
  EXPECT_EQ(Graph.relocations()[0].Addend, 8);
  EXPECT_EQ(Graph.rebases()[0].Addend, 16);

  EXPECT_FALSE(Graph.setSectionAddress(InvalidSectionId, 0));
  EXPECT_FALSE(Graph.setSectionFileOffset(InvalidSectionId, 0));
}

TEST(LinkGraphTest, ValidatesMutatedSectionInvariants) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 2, 0, 0, {0}});
  ASSERT_TRUE(Text);

  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  Sections[*Text].Alignment = 0;
  auto AlignmentResult = Graph.validate();
  ASSERT_FALSE(AlignmentResult);
  EXPECT_EQ(AlignmentResult.error().Message,
            "section alignment must be a non-zero power of two");

  Sections[*Text].Alignment = 1;
  Sections[*Text].VirtualSize = 0;
  auto SizeResult = Graph.validate();
  ASSERT_FALSE(SizeResult);
  EXPECT_EQ(SizeResult.error().Message,
            "section content exceeds section virtual size");
}

TEST(LinkGraphTest, ValidatesMutatedPatchOffsets) {
  LinkGraph RelocationGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(RelocationGraph.beginInput("input.o"));
  auto Text = RelocationGraph.addSection(
      Section{".text", SectionKind::Text, 1, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol =
      RelocationGraph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Mutable{*Text, 0, 42, *TargetSymbol, 0};
  Mutable.PatchSize = 4;
  ASSERT_TRUE(RelocationGraph.addRelocation(Mutable));
  const_cast<std::vector<Relocation> &>(RelocationGraph.relocations())[0]
      .Offset = 1;
  auto RelocationResult = RelocationGraph.validate();
  ASSERT_FALSE(RelocationResult);
  EXPECT_EQ(RelocationResult.error().Message,
            "relocation offset is outside section content");

  LinkGraph RebaseGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(RebaseGraph.beginInput("input.o"));
  Text = RebaseGraph.addSection(
      Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(RebaseGraph.addRebase(Rebase{*Text, 0, 7, 0}));
  const_cast<std::vector<Rebase> &>(RebaseGraph.rebases())[0].Offset = 1;
  auto RebaseResult = RebaseGraph.validate();
  ASSERT_FALSE(RebaseResult);
  EXPECT_EQ(RebaseResult.error().Message,
            "rebase offset is outside section content");
}

TEST(LinkGraphTest, SymbolIdsRemainStableAcrossVectorGrowth) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Bss = Graph.addSection(Section{".bss", SectionKind::BSS, 1, 64});
  ASSERT_TRUE(Bss);

  auto First = Graph.addSymbol(Symbol{"first", *Bss, 0, 1, false});
  ASSERT_TRUE(First);
  for (uint32_t I = 1; I < 64; ++I) {
    ASSERT_TRUE(Graph.addSymbol(
        Symbol{"symbol" + std::to_string(I), *Bss, I, 1, false}));
  }

  EXPECT_EQ(*First, 0U);
  EXPECT_EQ(Graph.symbols()[*First].Name, "first");
}

TEST(LinkGraphTest, SectionOffsetsDefaultToZero) {
  Section Value{".data", SectionKind::Data, 4};
  EXPECT_EQ(Value.Address, 0U);
  EXPECT_EQ(Value.FileOffset, 0U);
  EXPECT_EQ(Value.VirtualSize, 0U);
}

TEST(LinkGraphTest, StoresValidOrderedCompactUnwindRecords) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       32,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       0x1000});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 8, 8, 0, 0, {}});
  auto ReadOnly = Graph.addSection(
      Section{"__const", SectionKind::ReadOnly, 8, 8, 0, 0, {}});
  auto EHFrame = Graph.addSection(Section{"__eh_frame",
                                          SectionKind::Unwind,
                                          8,
                                          8,
                                          0,
                                          0,
                                          {},
                                          SectionPurpose::EHFrame});
  ASSERT_TRUE(Text && Data && ReadOnly && EHFrame);
  auto First = Graph.addSymbol(Symbol{"first", *Text, 0, 0, false});
  auto Second = Graph.addSymbol(Symbol{"second", *Text, 16, 0, false});
  auto Personality = Graph.addSymbol(Symbol{"personality", *Data, 0, 0, false});
  auto LSDA = Graph.addSymbol(Symbol{"lsda", *ReadOnly, 0, 0, false});
  auto FDE = Graph.addSymbol(Symbol{"fde", *EHFrame, 0, 0, false});
  ASSERT_TRUE(First && Second && Personality && LSDA && FDE);

  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*First, 16, 0x02000000, *Personality, *LSDA, *FDE}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Second, 8, 0x03000000, {}, {}, {}}));
  ASSERT_EQ(Graph.compactUnwind().size(), 2U);
  EXPECT_EQ(Graph.compactUnwind()[0].Function, *First);
  EXPECT_EQ(Graph.compactUnwind()[1].Encoding, 0x03000000U);
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsInvalidCompactUnwindFunctionsAndRanges) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       16,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       UINT64_MAX - 7});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 4, 16, 0, 0, {}});
  ASSERT_TRUE(Text && Data);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 8, 0, false});
  auto NonText = Graph.addSymbol(Symbol{"data", *Data, 0, 0, false});
  ASSERT_TRUE(Function && NonText);
  const auto Reject = [&](CompactUnwindRecord Record,
                          std::string_view Message) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  Reject({InvalidSymbolId, 1, 0, {}, {}, {}},
         "invalid compact unwind function symbol ID");
  Reject({*NonText, 1, 0, {}, {}, {}},
         "compact unwind function must reference text section");
  Reject({*Function, 0, 0, {}, {}, {}},
         "compact unwind function length must be non-zero");
  Reject({*Function, 9, 0, {}, {}, {}},
         "compact unwind function range exceeds text section");
  Reject({*Function, 8, 0, {}, {}, {}},
         "compact unwind function address overflows");
  EXPECT_TRUE(Graph.compactUnwind().empty());
}

TEST(LinkGraphTest, RejectsInvalidCompactUnwindOptionalSymbols) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{"__text", SectionKind::Text, 4, 8, 0, 0, {}});
  auto BSS =
      Graph.addSection(Section{"__bss", SectionKind::BSS, 4, 8, 0, 0, {}});
  auto Unwind = Graph.addSection(
      Section{"__unwind", SectionKind::Unwind, 4, 8, 0, 0, {}});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 4, 8, 0, 0, {}});
  auto ReadOnly = Graph.addSection(
      Section{"__const", SectionKind::ReadOnly, 4, 8, 0, 0, {}});
  auto EHFrame = Graph.addSection(Section{"__eh_frame",
                                          SectionKind::Unwind,
                                          4,
                                          8,
                                          0,
                                          0,
                                          {},
                                          SectionPurpose::EHFrame});
  auto Empty =
      Graph.addSection(Section{"__empty", SectionKind::Data, 4, 0, 0, 0, {}});
  ASSERT_TRUE(Text && BSS && Unwind && Data && ReadOnly && EHFrame && Empty);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 0, 0, false});
  auto BSSSymbol = Graph.addSymbol(Symbol{"bss", *BSS, 0, 0, false});
  auto WrongFDE = Graph.addSymbol(Symbol{"fde", *Unwind, 0, 0, false});
  auto PersonalityEnd =
      Graph.addSymbol(Symbol{"personality_end", *Data, 8, 0, false});
  auto LSDAEnd = Graph.addSymbol(Symbol{"lsda_end", *ReadOnly, 8, 0, false});
  auto FDEEnd = Graph.addSymbol(Symbol{"fde_end", *EHFrame, 8, 0, false});
  auto EmptySymbol = Graph.addSymbol(Symbol{"empty", *Empty, 0, 0, false});
  ASSERT_TRUE(Function && BSSSymbol && WrongFDE && PersonalityEnd && LSDAEnd &&
              FDEEnd && EmptySymbol);
  const auto Reject = [&](CompactUnwindRecord Record,
                          std::string_view Message) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  Reject({*Function, 1, 0, InvalidSymbolId, {}, {}},
         "invalid compact unwind personality symbol ID");
  Reject({*Function, 1, 0, {}, InvalidSymbolId, {}},
         "invalid compact unwind LSDA symbol ID");
  Reject({*Function, 1, 0, {}, {}, InvalidSymbolId},
         "invalid compact unwind FDE symbol ID");
  Reject({*Function, 1, 0, *BSSSymbol, {}, {}},
         "compact unwind personality must reference a non-BSS section");
  Reject({*Function, 1, 0, {}, *BSSSymbol, {}},
         "compact unwind LSDA must reference a non-BSS section");
  Reject({*Function, 1, 0, {}, {}, *WrongFDE},
         "compact unwind FDE must reference an EH frame section");

  const auto RejectOutside = [&](CompactUnwindRecord Record, SymbolId Symbol,
                                 SectionId Section, std::string_view SymbolName,
                                 std::string_view SectionName) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message,
              "compact unwind symbol is outside section storage");
    EXPECT_EQ(Result.error().Symbol, Symbol);
    EXPECT_EQ(Result.error().Section, Section);
    EXPECT_EQ(Result.error().Offset, Graph.symbols()[Symbol].Offset);
    EXPECT_EQ(Result.error().SymbolName, SymbolName);
    EXPECT_EQ(Result.error().SectionName, SectionName);
  };
  RejectOutside({*Function, 1, 0, *PersonalityEnd, {}, {}}, *PersonalityEnd,
                *Data, "personality_end", "__data");
  RejectOutside({*Function, 1, 0, {}, *LSDAEnd, {}}, *LSDAEnd, *ReadOnly,
                "lsda_end", "__const");
  RejectOutside({*Function, 1, 0, {}, {}, *FDEEnd}, *FDEEnd, *EHFrame,
                "fde_end", "__eh_frame");
  RejectOutside({*Function, 1, 0, *EmptySymbol, {}, {}}, *EmptySymbol, *Empty,
                "empty", "__empty");
}

TEST(LinkGraphTest, RejectsUnorderedAndOverlappingCompactUnwindRecords) {
  const auto MakeGraph = [] {
    LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
    EXPECT_TRUE(Graph.beginInput("input.o"));
    EXPECT_TRUE(Graph.addSection(Section{"__text",
                                         SectionKind::Text,
                                         4,
                                         32,
                                         0,
                                         0,
                                         {},
                                         SectionPurpose::Default,
                                         0x1000}));
    EXPECT_TRUE(Graph.addSymbol(Symbol{"first", 0, 0, 0, false}));
    EXPECT_TRUE(Graph.addSymbol(Symbol{"second", 0, 8, 0, false}));
    return Graph;
  };
  for (const auto &Second : {CompactUnwindRecord{0, 1, 0, {}, {}, {}},
                             CompactUnwindRecord{1, 1, 0, {}, {}, {}},
                             CompactUnwindRecord{1, 8, 0, {}, {}, {}}}) {
    auto Graph = MakeGraph();
    const CompactUnwindRecord First =
        Second.Function == 1 && Second.Length == 8
            ? CompactUnwindRecord{0, 16, 0, {}, {}, {}}
            : CompactUnwindRecord{1, 1, 0, {}, {}, {}};
    ASSERT_TRUE(Graph.addCompactUnwind(First));
    auto Result = Graph.addCompactUnwind(Second);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message,
              Second.Function == 0 ? "compact unwind records are unordered"
              : Second.Length == 1
                  ? "duplicate compact unwind function address"
                  : "overlapping compact unwind function ranges");
  }
}

TEST(LinkGraphTest, RejectsCompactUnwindMutationAfterRelocation) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       16,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       0x1000}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"first", 0, 0, 0, false}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"second", 0, 8, 0, false}));
  ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{0, 8, 0, {}, {}, {}}));
  ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{1, 8, 0, {}, {}, {}}));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Late = Graph.addCompactUnwind(CompactUnwindRecord{1, 8, 0, {}, {}, {}});
  ASSERT_FALSE(Late);
  EXPECT_EQ(Late.error().Message, "link graph relocations already applied");
}

TEST(LinkGraphTest, RejectsCompactUnwindForNonMachOGraphs) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::ELF);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"function", 0, 0, 0, false}));
  auto Result =
      Graph.addCompactUnwind(CompactUnwindRecord{0, 1, 0, {}, {}, {}});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "compact unwind requires a Mach-O link graph");
}

TEST(LayoutTest, GroupsAndAlignsSections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 4, 0, 0, {1, 2, 3, 4}});
  auto Unwind = Graph.addSection(
      Section{".eh_frame", SectionKind::Unwind, 4, 3, 0, 0, {1, 2, 3}});
  auto ReadOnly = Graph.addSection(
      Section{".rodata", SectionKind::ReadOnly, 2, 2, 0, 0, {1, 2}});
  auto TextZ = Graph.addSection(
      Section{".text.z", SectionKind::Text, 4, 3, 0, 0, {1, 2, 3}});
  auto TextA =
      Graph.addSection(Section{".text.a", SectionKind::Text, 8, 1, 0, 0, {1}});
  ASSERT_TRUE(Data && Unwind && ReadOnly && TextZ && TextA);

  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*TextA].Address, 0U);
  EXPECT_EQ(Graph.sections()[*TextA].FileOffset, 0U);
  EXPECT_EQ(Graph.sections()[*TextZ].Address, 4U);
  EXPECT_EQ(Graph.sections()[*TextZ].FileOffset, 4U);
  EXPECT_EQ(Graph.sections()[*ReadOnly].Address, 8U);
  EXPECT_EQ(Graph.sections()[*ReadOnly].FileOffset, 8U);
  EXPECT_EQ(Graph.sections()[*Unwind].Address, 12U);
  EXPECT_EQ(Graph.sections()[*Unwind].FileOffset, 12U);
  EXPECT_EQ(Graph.sections()[*Data].Address, 16U);
  EXPECT_EQ(Graph.sections()[*Data].FileOffset, 16U);
}

TEST(LayoutTest, BssConsumesVirtualButNotFileSpace) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto First =
      Graph.addSection(Section{"a", SectionKind::Data, 1, 3, 0, 0, {1, 2, 3}});
  auto Second =
      Graph.addSection(Section{"z", SectionKind::Data, 4, 1, 0, 0, {1}});
  auto Bss = Graph.addSection(Section{"b", SectionKind::BSS, 8, 9});
  ASSERT_TRUE(First && Second && Bss);

  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*First].Address, 0U);
  EXPECT_EQ(Graph.sections()[*Second].Address, 4U);
  EXPECT_EQ(Graph.sections()[*Second].FileOffset, 4U);
  EXPECT_EQ(Graph.sections()[*Bss].Address, 8U);
  EXPECT_EQ(Graph.sections()[*Bss].FileOffset, 0U);
  EXPECT_EQ(Graph.sections()[*Bss].Address + Graph.sections()[*Bss].VirtualSize,
            17U);

  ASSERT_TRUE(Graph.setSectionFileOffset(*Bss, 123));
  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*Second].FileOffset, 4U);
  EXPECT_EQ(Graph.sections()[*Bss].FileOffset, 0U);
}

TEST(LayoutTest, IsIndependentOfInsertionOrderForUniqueNames) {
  auto MakeGraph = [](bool Reverse) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    EXPECT_TRUE(Graph.beginInput("input.o"));
    Section A{"a", SectionKind::ReadOnly, 4, 2, 0, 0, {1, 2}};
    Section Z{"z", SectionKind::ReadOnly, 8, 3, 0, 0, {1, 2, 3}};
    EXPECT_TRUE(Graph.addSection(Reverse ? Z : A));
    EXPECT_TRUE(Graph.addSection(Reverse ? A : Z));
    return Graph;
  };
  auto First = MakeGraph(false);
  auto Second = MakeGraph(true);

  ASSERT_TRUE(layout(First));
  ASSERT_TRUE(layout(Second));
  for (const auto &Section : First.sections()) {
    const auto Other = std::find_if(
        Second.sections().begin(), Second.sections().end(),
        [&](const auto &Value) { return Value.Name == Section.Name; });
    ASSERT_NE(Other, Second.sections().end());
    EXPECT_EQ(Other->Address, Section.Address);
    EXPECT_EQ(Other->FileOffset, Section.FileOffset);
  }
}

TEST(LayoutTest, OrdersDuplicateNamesByOriginalOrdinal) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto First =
      Graph.addSection(Section{"same", SectionKind::Text, 1, 2, 0, 0, {1, 2}});
  auto Second =
      Graph.addSection(Section{"same", SectionKind::Text, 4, 1, 0, 0, {1}});
  ASSERT_TRUE(First && Second);

  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*First].Address, 0U);
  EXPECT_EQ(Graph.sections()[*Second].Address, 4U);
}

TEST(LayoutTest, AppliesNonzeroImageBase) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{"text", SectionKind::Text, 16, 2, 0, 0, {1, 2}});
  ASSERT_TRUE(Text);

  ASSERT_TRUE(layout(Graph, 0x1003));
  EXPECT_EQ(Graph.sections()[*Text].Address, 0x1010U);
  EXPECT_EQ(Graph.sections()[*Text].FileOffset, 0U);
}

TEST(LayoutTest, AlignsUniversalPermissionGroupsToPages) {
  constexpr uint64_t PageSize = 4096;
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{".text", SectionKind::Text, 16, 32, 0, 0,
                                       std::vector<WasmEdge::Byte>(32)});
  auto Data = Graph.addSection(Section{".data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)});
  ASSERT_TRUE(Text && Data);

  ASSERT_TRUE(layout(Graph, 0, PageSize));
  EXPECT_EQ(Graph.sections()[*Text].Address, 0U);
  EXPECT_EQ(Graph.sections()[*Data].Address, PageSize);
}

TEST(LayoutTest, IsIdempotent) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{"text", SectionKind::Text, 8, 3, 0, 0, {1, 2, 3}}));
  ASSERT_TRUE(Graph.addSection(Section{"bss", SectionKind::BSS, 16, 7}));
  ASSERT_TRUE(layout(Graph, 0x1000));
  const auto Sections = Graph.sections();

  ASSERT_TRUE(layout(Graph, 0x1000));
  EXPECT_EQ(Graph.sections()[0].Address, Sections[0].Address);
  EXPECT_EQ(Graph.sections()[0].FileOffset, Sections[0].FileOffset);
  EXPECT_EQ(Graph.sections()[1].Address, Sections[1].Address);
  EXPECT_EQ(Graph.sections()[1].FileOffset, Sections[1].FileOffset);
}

TEST(LayoutTest, RejectsAlignmentOverflow) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"text", SectionKind::Text, 2, 0}));

  auto Result = layout(Graph, std::numeric_limits<uint64_t>::max());
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "section address alignment overflows");
  EXPECT_EQ(Result.error().SectionName, "text");
}

TEST(LayoutTest, RejectsSectionSizeAndImageBaseOverflow) {
  LinkGraph SizeGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(SizeGraph.beginInput("input.o"));
  ASSERT_TRUE(SizeGraph.addSection(Section{
      "first", SectionKind::Text, 1, std::numeric_limits<uint64_t>::max()}));
  ASSERT_TRUE(SizeGraph.addSection(Section{"second", SectionKind::Text, 1, 1}));
  auto SizeResult = layout(SizeGraph);
  ASSERT_FALSE(SizeResult);
  EXPECT_EQ(SizeResult.error().Message, "section virtual size overflows");
  EXPECT_EQ(SizeResult.error().SectionName, "second");

  LinkGraph BaseGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(BaseGraph.beginInput("input.o"));
  ASSERT_TRUE(BaseGraph.addSection(Section{"text", SectionKind::Text, 1, 3}));
  auto BaseResult = layout(BaseGraph, std::numeric_limits<uint64_t>::max() - 1);
  ASSERT_FALSE(BaseResult);
  EXPECT_EQ(BaseResult.error().Message, "section virtual size overflows");
  EXPECT_EQ(BaseResult.error().SectionName, "text");
}

TEST(LayoutTest, RejectsInvalidMutatedGraphBeforeLayout) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"text", SectionKind::Text, 1, 0}));
  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  Sections[0].Alignment = 0;

  auto Result = layout(Graph);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "section alignment must be a non-zero power of two");
}

TEST(LayoutTest, RetainsAllSectionsIncludingEmptySections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"empty", SectionKind::Text, 8, 0}));
  ASSERT_TRUE(Graph.addSection(
      Section{"unused", SectionKind::ReadOnly, 4, 1, 0, 0, {1}}));
  ASSERT_TRUE(Graph.addSection(Section{"zero", SectionKind::BSS, 16, 0}));

  ASSERT_TRUE(layout(Graph));
  ASSERT_EQ(Graph.sections().size(), 3U);
  EXPECT_EQ(Graph.sections()[0].Address, 0U);
  EXPECT_EQ(Graph.sections()[1].Address, 0U);
  EXPECT_EQ(Graph.sections()[2].Address, 16U);
}

TEST(LinkGraphTest, ClassifiesRelocationPatchSizesForEveryFormat) {
  struct Case {
    ObjectFormat Format;
    Target Architecture;
    uint32_t Type;
    uint8_t MetadataSize;
    std::optional<uint8_t> Expected;
  };
  const Target Invalid = static_cast<Target>(UINT8_MAX);
  const std::vector<Case> Cases{
      {ObjectFormat::ELF, Target::X86_64, llvm::ELF::R_X86_64_REX_GOTPCRELX, 0,
       4},
      {ObjectFormat::ELF, Target::X86_64, llvm::ELF::R_X86_64_GOTPCREL, 0,
       std::nullopt},
      {ObjectFormat::MachO, Target::X86_64, llvm::MachO::X86_64_RELOC_UNSIGNED,
       4, std::nullopt},
      {ObjectFormat::MachO, Target::X86_64, llvm::MachO::X86_64_RELOC_GOT, 8,
       std::nullopt},
      {ObjectFormat::COFF, Target::X86_64, llvm::COFF::IMAGE_REL_AMD64_ADDR64,
       0, 8},
      {ObjectFormat::COFF, Target::X86_64, llvm::COFF::IMAGE_REL_AMD64_REL32, 0,
       4},
      {ObjectFormat::COFF, Target::X86_64, llvm::COFF::IMAGE_REL_AMD64_REL32_5,
       0, 4},
      {ObjectFormat::COFF, Target::X86_64, llvm::COFF::IMAGE_REL_AMD64_ADDR32NB,
       0, 4},
      {ObjectFormat::COFF, Target::X86_64, llvm::COFF::IMAGE_REL_AMD64_ABSOLUTE,
       0, std::nullopt},
      {ObjectFormat::COFF, Target::X86_64, llvm::COFF::IMAGE_REL_AMD64_SECTION,
       0, std::nullopt},
      {ObjectFormat::MachO, Target::AArch64, llvm::MachO::ARM64_RELOC_UNSIGNED,
       4, std::nullopt},
      {ObjectFormat::MachO, Target::AArch64, llvm::MachO::ARM64_RELOC_UNSIGNED,
       8, 8},
      {ObjectFormat::MachO, Target::AArch64,
       llvm::MachO::ARM64_RELOC_GOT_LOAD_PAGE21, 4, std::nullopt},
      {ObjectFormat::COFF, Target::AArch64, llvm::COFF::IMAGE_REL_ARM64_ADDR64,
       0, 8},
      {ObjectFormat::COFF, Target::AArch64,
       llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, 0, 4},
      {ObjectFormat::COFF, Target::AArch64,
       llvm::COFF::IMAGE_REL_ARM64_ABSOLUTE, 0, std::nullopt},
      {ObjectFormat::MachO, Target::ARM, llvm::ELF::R_ARM_ABS32, 4,
       std::nullopt},
      {ObjectFormat::COFF, Target::RISCV64, llvm::ELF::R_RISCV_64, 8,
       std::nullopt},
      {ObjectFormat::ELF, Target::ARM, llvm::ELF::R_ARM_MOVW_ABS_NC, 4,
       std::nullopt},
      {ObjectFormat::ELF, Target::AArch64,
       llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC, 0, 4},
      {ObjectFormat::ELF, Target::AArch64, llvm::ELF::R_AARCH64_ABS32, 4,
       std::nullopt},
      {ObjectFormat::ELF, Target::RISCV64, llvm::ELF::R_RISCV_SET8, 0, 1},
      {ObjectFormat::ELF, Target::RISCV64, llvm::ELF::R_RISCV_SET6, 0, 1},
      {ObjectFormat::ELF, Target::RISCV64, llvm::ELF::R_RISCV_SUB6, 0, 1},
      {ObjectFormat::ELF, Target::RISCV64, llvm::ELF::R_RISCV_HI20, 4,
       std::nullopt},
      {ObjectFormat::ELF, Target::S390X, llvm::ELF::R_390_PLT32DBL, 0, 4},
      {ObjectFormat::ELF, Target::S390X, llvm::ELF::R_390_GOT32, 4,
       std::nullopt},
      {ObjectFormat::ELF, Invalid, 0, BytePatch, BytePatch},
      {ObjectFormat::ELF, Invalid, 0, HalfPatch, HalfPatch},
      {ObjectFormat::ELF, Invalid, 0, WordPatch, WordPatch},
      {ObjectFormat::ELF, Invalid, 0, DoubleWordPatch, DoubleWordPatch},
      {ObjectFormat::ELF, Invalid, 0, 3, std::nullopt},
  };
  for (const auto &Test : Cases) {
    SCOPED_TRACE(Test.Type);
    EXPECT_EQ(relocationPatchSize(Test.Format, Test.Architecture, Test.Type,
                                  Test.MetadataSize),
              Test.Expected);
  }
}

TEST(LinkGraphTest, RejectsInvalidLinksAndFormatSpecificMetadata) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{"__text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  Section Linked{"__data", SectionKind::Data, 1, 1, 0, 0, {0}};
  Linked.LinkedSection = 5;
  auto LinkedResult = Graph.addSection(Linked);
  ASSERT_FALSE(LinkedResult);
  EXPECT_EQ(LinkedResult.error().Message, "invalid linked section ID");
  EXPECT_EQ(LinkedResult.error().SectionName, "__data");
  auto SetLinkedResult = Graph.setLinkedSection(*Text, 7);
  ASSERT_FALSE(SetLinkedResult);
  EXPECT_EQ(SetLinkedResult.error().Message, "invalid linked section ID");
  EXPECT_FALSE(Graph.sections()[*Text].LinkedSection);
  auto FlagsResult = Graph.setELFFlags(1);
  ASSERT_FALSE(FlagsResult);
  EXPECT_EQ(FlagsResult.error().Message, "ELF flags require an ELF link graph");
  EXPECT_EQ(Graph.elfFlags(), 0U);
  auto Function = Graph.addSymbol(Symbol{"_f", *Text, 0, 1, false});
  ASSERT_TRUE(Function);
  auto ReferenceResult =
      Graph.addEHFrameReference(EHFrameReference{*Text, 0, *Function});
  ASSERT_FALSE(ReferenceResult);
  EXPECT_EQ(ReferenceResult.error().Message, "invalid EH frame reference");
  EXPECT_TRUE(Graph.ehFrameReferences().empty());

  LinkGraph ELF(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(ELF.beginInput("input.o"));
  Text = ELF.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  Function = ELF.addSymbol(Symbol{"f", *Text, 0, 1, false});
  ASSERT_TRUE(Function);
  const_cast<std::vector<CompactUnwindRecord> &>(ELF.compactUnwind())
      .push_back(CompactUnwindRecord{*Function, 1, 0, {}, {}, {}});
  auto ValidateResult = ELF.validate();
  ASSERT_FALSE(ValidateResult);
  EXPECT_EQ(ValidateResult.error().Message,
            "compact unwind requires a Mach-O link graph");
}

TEST(LinkGraphTest, RejectsCompactUnwindSymbolsWithInvalidPlacement) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       16,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       UINT64_MAX - 4});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 4, 8, 0, 0, {}});
  ASSERT_TRUE(Text && Data);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 0, 0, false});
  auto Moved = Graph.addSymbol(Symbol{"moved", *Text, 4, 0, false});
  auto Personality = Graph.addSymbol(Symbol{"personality", *Data, 0, 0, false});
  ASSERT_TRUE(Function && Moved && Personality);
  const auto Reject = [&](CompactUnwindRecord Record,
                          std::string_view Message) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  Reject({*Function, 8, 0, {}, {}, {}},
         "compact unwind function address overflows");
  auto &Symbols = const_cast<std::vector<Symbol> &>(Graph.symbols());
  Symbols[*Moved].Section = InvalidSectionId;
  Reject({*Moved, 1, 0, {}, {}, {}},
         "invalid compact unwind function section ID");
  Symbols[*Personality].Section = InvalidSectionId;
  Reject({*Function, 1, 0, *Personality, {}, {}},
         "invalid compact unwind personality section ID");
  EXPECT_TRUE(Graph.compactUnwind().empty());
}

LinkGraph makePrunableMachOGraph() {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  EXPECT_TRUE(Graph.beginInput("prune.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 1, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)});
  auto Data = Graph.addSection(Section{"__data", SectionKind::Data, 8, 16, 0, 0,
                                       std::vector<WasmEdge::Byte>(16)});
  auto EHFrame = Graph.addSection(
      Section{"__eh_frame", SectionKind::Unwind, 8, 8, 0, 0,
              std::vector<WasmEdge::Byte>(8), SectionPurpose::EHFrame});
  EXPECT_TRUE(Text && Data && EHFrame);
  auto Function =
      Graph.addSymbol(Symbol{"_f", *Text, 0, 1, false, std::nullopt, true});
  auto Frame = Graph.addSymbol(Symbol{"_frame", *EHFrame, 0, 1, false});
  auto Personality = Graph.addSymbol(Symbol{"_p", *Data, 0, 8, false});
  EXPECT_TRUE(Function && Frame && Personality);
  EXPECT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{
      *Function, 1, 0, *Personality, std::nullopt, std::nullopt}));
  EXPECT_TRUE(Graph.addRelocation(
      Relocation{*Data, 0, llvm::MachO::X86_64_RELOC_UNSIGNED, *Function, 0,
                 false, ObjectFormat::MachO, 8}));
  EXPECT_TRUE(Graph.addRebase(Rebase{*Data, 8, 0, 0, 8, ObjectFormat::MachO}));
  EXPECT_TRUE(
      Graph.addRebase(Rebase{*EHFrame, 0, 0, 0, 8, ObjectFormat::MachO}));
  EXPECT_TRUE(Graph.setLinkedSection(*Data, *Text));
  return Graph;
}

TEST(LinkGraphTest, PrunesUnreferencedMachOEHFrameAndRemapsIds) {
  auto Graph = makePrunableMachOGraph();
  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());
  ASSERT_EQ(Graph.sections().size(), 2U);
  EXPECT_EQ(Graph.sections()[0].Name, "__text");
  EXPECT_EQ(Graph.sections()[1].Name, "__data");
  EXPECT_EQ(Graph.sections()[1].LinkedSection, std::optional<SectionId>{0});
  ASSERT_EQ(Graph.symbols().size(), 2U);
  EXPECT_EQ(Graph.symbols()[1].Name, "_p");
  EXPECT_EQ(Graph.symbols()[1].Section, 1U);
  ASSERT_EQ(Graph.relocations().size(), 1U);
  EXPECT_EQ(Graph.relocations()[0].Section, 1U);
  EXPECT_EQ(Graph.relocations()[0].Symbol, 0U);
  ASSERT_EQ(Graph.rebases().size(), 1U);
  EXPECT_EQ(Graph.rebases()[0].Section, 1U);
  EXPECT_EQ(Graph.rebases()[0].Offset, 8U);
  ASSERT_EQ(Graph.compactUnwind().size(), 1U);
  EXPECT_EQ(Graph.compactUnwind()[0].Personality, std::optional<SymbolId>{1});
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsPruningMachOEHFrameStillInUse) {
  static constexpr SectionId Text = 0;
  static constexpr SectionId Data = 1;
  static constexpr SectionId EHFrame = 2;
  static constexpr SymbolId Function = 0;
  static constexpr SymbolId Frame = 1;
  struct Case {
    const char *Message;
    std::function<void(LinkGraph &)> Mutate;
  };
  const std::array<Case, 5> Cases{{
      {"section links to pruned EH frame",
       [](LinkGraph &Graph) {
         ASSERT_TRUE(Graph.setLinkedSection(Data, EHFrame));
       }},
      {"relocation targets pruned EH frame",
       [](LinkGraph &Graph) {
         ASSERT_TRUE(Graph.addRelocation(
             Relocation{Text, 0, llvm::MachO::X86_64_RELOC_UNSIGNED, Frame, 0,
                        false, ObjectFormat::MachO, 8}));
       }},
      {"compact unwind targets pruned EH frame",
       [](LinkGraph &Graph) {
         const_cast<std::vector<CompactUnwindRecord> &>(Graph.compactUnwind())
             .front()
             .LSDA = Frame;
       }},
      {"overlapping rebase patches",
       [](LinkGraph &Graph) {
         auto &Rebases = const_cast<std::vector<Rebase> &>(Graph.rebases());
         Rebases.push_back(Rebases.front());
       }},
      {"symbol extends beyond section virtual size",
       [](LinkGraph &Graph) {
         const_cast<std::vector<Symbol> &>(Graph.symbols())[Function].Offset =
             100;
       }},
  }};
  for (const auto &Test : Cases) {
    SCOPED_TRACE(Test.Message);
    auto Graph = makePrunableMachOGraph();
    Test.Mutate(Graph);
    auto Result = Graph.pruneUnreferencedMachOEHFrame();
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Test.Message);
    ASSERT_EQ(Graph.sections().size(), 3U);
    EXPECT_EQ(Graph.sections()[EHFrame].Purpose, SectionPurpose::EHFrame);
    EXPECT_EQ(Graph.symbols().size(), 3U);
  }
}

TEST(LinkGraphTest, ValidatesMutatedMachOUnwindInfoSection) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(Graph.validate());
  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  ASSERT_EQ(Sections.back().Purpose, SectionPurpose::UnwindInfo);
  ASSERT_FALSE(Sections.back().Content.empty());
  Sections.back().Content.pop_back();
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "incoherent Mach-O unwind info section");
}

TEST(LayoutTest, RejectsInvalidSegmentAlignmentAndExtraPlacements) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  auto Alignment = layout(Graph, 0, 3);
  ASSERT_FALSE(Alignment);
  EXPECT_EQ(Alignment.error().Message, "invalid segment alignment");
  EXPECT_EQ(Graph.sections()[0].Address, 0U);
  const std::array<Internal::Placement, 2> Placements{
      {{0x1000, 0x10}, {0x2000, 0x20}}};
  auto Placed = Internal::applyPlacements(Graph, Placements, 0);
  ASSERT_FALSE(Placed);
  EXPECT_EQ(Placed.error().Message, "invalid section ID");
}

struct BranchThunkCase {
  ObjectFormat Format;
  uint32_t Branch;
  bool ImplicitAddend;
  std::array<const char *, 3> Names;
};

uint64_t aarch64PageAddress(uint64_t Place, uint32_t Instruction) {
  const uint64_t Immediate =
      ((Instruction >> 29) & 0x3U) | (((Instruction >> 5) & 0x7FFFFU) << 2);
  return (Place & ~UINT64_C(0xFFF)) +
         static_cast<uint64_t>(llvm::SignExtend64<21>(Immediate) << 12);
}

TEST(BranchThunkTest, RedirectsOutOfRangeAArch64BranchesThroughThunks) {
  constexpr uint64_t Gap = UINT64_C(160) << 20;
  for (const auto &Test : {
           BranchThunkCase{ObjectFormat::ELF,
                           llvm::ELF::R_AARCH64_CALL26,
                           false,
                           {".text.a", ".text.b", ".text.c"}},
           BranchThunkCase{ObjectFormat::MachO,
                           llvm::MachO::ARM64_RELOC_BRANCH26,
                           false,
                           {"__a", "__b", "__c"}},
           BranchThunkCase{ObjectFormat::COFF,
                           llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                           true,
                           {".text$a", ".text$b", ".text$c"}},
       }) {
    LinkGraph Graph(Target::AArch64, Endianness::Little, Test.Format);
    ASSERT_TRUE(Graph.beginInput("thunk.o"));
    std::vector<WasmEdge::Byte> CallerCode(12);
    write32le(CallerCode, 0, 0x94000000);
    write32le(CallerCode, 4, Test.ImplicitAddend ? 0x94000001 : 0x14000000);
    write32le(CallerCode, 8, 0xD65F03C0);
    auto Caller = Graph.addSection(
        Section{Test.Names[0], SectionKind::Text, 4, 12, 0, 0, CallerCode});
    auto Filler = Graph.addSection(
        Section{Test.Names[1], SectionKind::Text, 4, Gap, 0, 0, {}});
    auto Callee = Graph.addSection(
        Section{Test.Names[2], SectionKind::Text, 4, 8, 0, 0,
                std::vector<WasmEdge::Byte>{0x1F, 0x20, 0x03, 0xD5, 0xC0, 0x03,
                                            0x5F, 0xD6}});
    ASSERT_TRUE(Caller && Filler && Callee);
    auto Function = Graph.addSymbol(Symbol{"callee", *Callee, 0, 8, true});
    ASSERT_TRUE(Function);
    const int64_t SecondAddend = Test.ImplicitAddend ? 0 : 4;
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Caller, 0, Test.Branch, *Function, 0, Test.ImplicitAddend,
                   Test.Format, WordPatch}));
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Caller, 4, Test.Branch, *Function, SecondAddend,
                   Test.ImplicitAddend, Test.Format, WordPatch}));
    const uint64_t ImageBase =
        Test.Format == ObjectFormat::COFF ? PEImageBase : 0;
    ASSERT_TRUE(layout(Graph, ImageBase));

    auto Inserted = insertBranchThunks(Graph);
    ASSERT_TRUE(Inserted);
    EXPECT_TRUE(*Inserted);
    ASSERT_TRUE(layout(Graph, ImageBase));
    auto Again = insertBranchThunks(Graph);
    ASSERT_TRUE(Again);
    EXPECT_FALSE(*Again);
    ASSERT_EQ(Graph.sections().size(), 4U);
    const auto &Island = Graph.sections()[3];
    ASSERT_TRUE(Island.Anchor);
    EXPECT_EQ(Island.Anchor->Section, *Caller);
    EXPECT_FALSE(Island.Anchor->After);
    EXPECT_EQ(Island.Content.size(), 24U);
    EXPECT_EQ(Island.Address + Island.VirtualSize,
              Graph.sections()[*Caller].Address);
    ASSERT_TRUE(applyRelocations(Graph));

    const auto &Code = Graph.sections()[*Caller];
    const auto &Thunks = Graph.sections()[3];
    const uint64_t Target = Graph.sections()[*Callee].Address;
    for (const uint64_t Offset : {UINT64_C(0), UINT64_C(4)}) {
      const uint32_t Branch = read32le(Code.Content, Offset);
      const uint64_t Place = Code.Address + Offset;
      const uint64_t Thunk =
          Place + static_cast<uint64_t>(
                      llvm::SignExtend64<26>(Branch & 0x03FFFFFFU) << 2);
      ASSERT_GE(Thunk, Thunks.Address);
      ASSERT_LT(Thunk, Thunks.Address + Thunks.VirtualSize);
      const uint64_t ThunkOffset = Thunk - Thunks.Address;
      const uint32_t Adrp = read32le(Thunks.Content, ThunkOffset);
      const uint32_t Add = read32le(Thunks.Content, ThunkOffset + 4);
      const uint64_t Expected = Target + Offset;
      EXPECT_EQ(aarch64PageAddress(Thunk, Adrp), Expected & ~UINT64_C(0xFFF));
      EXPECT_EQ((Add >> 10) & 0xFFFU, Expected & 0xFFFU);
      EXPECT_EQ(read32le(Thunks.Content, ThunkOffset + 8), 0xD61F0200U);
    }
  }
}

TEST(BranchThunkTest, LeavesReachableBranchesAlone) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::ELF);
  ASSERT_TRUE(Graph.beginInput("near.o"));
  std::vector<WasmEdge::Byte> Code(8);
  write32le(Code, 0, 0x94000000);
  write32le(Code, 4, 0xD65F03C0);
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 4, 8, 0, 0, Code});
  ASSERT_TRUE(Text);
  auto Function = Graph.addSymbol(Symbol{"f", *Text, 4, 4, true});
  ASSERT_TRUE(Function);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_AARCH64_CALL26, *Function, 0, false,
                 ObjectFormat::ELF, WordPatch}));
  ASSERT_TRUE(layout(Graph));
  auto Inserted = insertBranchThunks(Graph);
  ASSERT_TRUE(Inserted);
  EXPECT_FALSE(*Inserted);
  EXPECT_EQ(Graph.sections().size(), 1U);
}

struct TextSplitCase {
  const char *Triple;
  const char *TextName;
  const char *Assembly;
  size_t Pieces;
};

std::vector<SectionId> textPieces(const LinkGraph &Graph,
                                  std::string_view Name) {
  std::vector<SectionId> Pieces;
  for (SectionId Id = 0; Id < Graph.sections().size(); ++Id)
    if (Graph.sections()[Id].Name == Name && !Graph.sections()[Id].Anchor)
      Pieces.push_back(Id);
  std::sort(Pieces.begin(), Pieces.end(), [&](SectionId L, SectionId R) {
    return Graph.sections()[L].InputAddress < Graph.sections()[R].InputAddress;
  });
  return Pieces;
}

TEST(BranchThunkTest, SplitsLargeTextAtFunctionStarts) {
  constexpr std::array<std::pair<uint64_t, uint64_t>, 5> Branches{{
      {0, 24},
      {4, 16},
      {8, 16},
      {16, 0},
      {24, 16},
  }};
  for (const auto &Test : {
           TextSplitCase{"aarch64-unknown-linux-gnu", ".text", R"(.text
.p2align 2
.globl f0
.type f0,%function
f0:
  .cfi_startproc
  bl f2
  bl .Lhidden
  b .Lhidden
  ret
  .cfi_endproc
.size f0, .-f0
.p2align 2
.Lhidden:
  .cfi_startproc
  bl f0
  ret
  .cfi_endproc
.p2align 2
.globl f2
.type f2,%function
f2:
  .cfi_startproc
  bl .Lhidden
  ret
  .cfi_endproc
.size f2, .-f2
)",
                         3},
           TextSplitCase{"arm64-apple-macosx", "__text", R"(.text
.p2align 2
.globl _f0
_f0:
  .cfi_startproc
  bl _f2
  bl Lhidden
  b Lhidden
  ret
  .cfi_endproc
.p2align 2
Lhidden:
  .cfi_startproc
  bl _f0
  ret
  .cfi_endproc
.p2align 2
.globl _f2
_f2:
  .cfi_startproc
  bl Lhidden
  ret
  .cfi_endproc
.subsections_via_symbols
)",
                         2},
       }) {
    SCOPED_TRACE(Test.Triple);
    REQUIRE_LLVM_TARGET(Test.Triple);
    auto Graph = ObjectReader::read(
        makeAssemblyObject(llvm::Triple(Test.Triple), Test.Assembly),
        Target::AArch64);
    ASSERT_TRUE(Graph);
    const uint64_t Base =
        Graph->sections()[textPieces(*Graph, Test.TextName).front()]
            .InputAddress;
    auto Split = splitTextSections(*Graph, 8);
    ASSERT_TRUE(Split);
    EXPECT_TRUE(*Split);
    const auto Pieces = textPieces(*Graph, Test.TextName);
    ASSERT_EQ(Pieces.size(), Test.Pieces);
    Section Padding{"__padding",
                    SectionKind::Text,
                    4,
                    256,
                    0,
                    0,
                    std::vector<WasmEdge::Byte>(256)};
    Padding.Anchor = SectionAnchor{Pieces.front(), true};
    ASSERT_TRUE(Graph->addSection(std::move(Padding)));
    ASSERT_TRUE(layout(*Graph));

    auto PieceOf = [&](uint64_t Original) {
      const Section *Result = &Graph->sections()[Pieces.front()];
      for (const SectionId Piece : Pieces)
        if (Graph->sections()[Piece].InputAddress - Base <= Original)
          Result = &Graph->sections()[Piece];
      return Result;
    };
    auto AddressOf = [&](uint64_t Original) {
      const auto *Piece = PieceOf(Original);
      return Piece->Address + Original - (Piece->InputAddress - Base);
    };
    if (Graph->format() == ObjectFormat::ELF) {
      std::vector<uint64_t> Frames;
      for (const auto &Rel : Graph->relocations()) {
        if (Graph->sections()[Rel.Section].Purpose != SectionPurpose::EHFrame)
          continue;
        const auto &Target = Graph->symbols()[Rel.Symbol];
        Frames.push_back(Graph->sections()[Target.Section].Address +
                         Target.Offset + static_cast<uint64_t>(Rel.Addend));
      }
      std::sort(Frames.begin(), Frames.end());
      EXPECT_EQ(Frames, (std::vector<uint64_t>{AddressOf(0), AddressOf(16),
                                               AddressOf(24)}));
    }
    ASSERT_TRUE(applyRelocations(*Graph));
    for (const auto &[From, To] : Branches) {
      SCOPED_TRACE(From);
      const auto *Piece = PieceOf(From);
      const uint32_t Instruction =
          read32le(Piece->Content, From - (Piece->InputAddress - Base));
      const uint64_t Destination =
          AddressOf(From) +
          static_cast<uint64_t>(
              llvm::SignExtend64<26>(Instruction & 0x03FFFFFFU) * 4);
      EXPECT_EQ(Destination, AddressOf(To));
    }
  }
}

TEST(BranchThunkTest, KeepsTextWholeForDataReferencesInsideFunctions) {
  REQUIRE_LLVM_TARGET("aarch64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeAssemblyObject(llvm::Triple("aarch64-unknown-linux-gnu"), R"(.text
.globl f0
.type f0,%function
f0:
  ret
.size f0, .-f0
.globl f1
.type f1,%function
f1:
  nop
.Linside:
  ret
.size f1, .-f1
.section .rodata
.word .Linside - .
)"),
      Target::AArch64);
  ASSERT_TRUE(Graph);
  auto Split = splitTextSections(*Graph, 4);
  ASSERT_TRUE(Split);
  EXPECT_FALSE(*Split);
  EXPECT_EQ(textPieces(*Graph, ".text").size(), 1U);
}

TEST(BranchThunkTest, KeepsResolvedConditionalBranchesInOnePiece) {
  REQUIRE_LLVM_TARGET("aarch64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeAssemblyObject(llvm::Triple("aarch64-unknown-linux-gnu"), R"(.text
.globl f0
.type f0,%function
f0:
  cbz x0, .Linside
  ret
.size f0, .-f0
.globl f1
.type f1,%function
f1:
  nop
.Linside:
  ret
.size f1, .-f1
.globl f2
.type f2,%function
f2:
  ret
.size f2, .-f2
)"),
      Target::AArch64);
  ASSERT_TRUE(Graph);
  const auto Pieces = textPieces(*Graph, ".text");
  ASSERT_EQ(Pieces.size(), 1U);
  const SectionId Original = Pieces.front();
  auto Split = splitTextSections(*Graph, 4);
  ASSERT_TRUE(Split);
  EXPECT_TRUE(*Split);
  EXPECT_EQ(textPieces(*Graph, ".text").size(), 2U);
  for (const auto &Value : Graph->symbols()) {
    if (Value.Name == "f1") {
      EXPECT_EQ(Value.Section, Original);
    }
    if (Value.Name == "f2") {
      EXPECT_NE(Value.Section, Original);
      EXPECT_EQ(Value.Offset, 0U);
    }
  }
}

} // namespace
