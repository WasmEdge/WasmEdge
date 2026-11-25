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
#include "linker/architecture.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/layout.h"
#include "linker/link_graph.h"
#include "linker/macho_writer.h"
#include "linker/object_reader.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"
#include "linker/universal_wasm_writer.h"
#include "linker/writer.h"

#include "aot/version.h"
#include "common/spdlog.h"
#include "loader/loader.h"
#include "loader/shared_library.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <gtest/gtest.h>

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Config/llvm-config.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <llvm/Object/COFF.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#if LLVM_VERSION_MAJOR < 16
#include <llvm/Support/Host.h>
#else
#include <llvm/TargetParser/Host.h>
#endif
#include <llvm/Target/TargetOptions.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#if !WASMEDGE_OS_WINDOWS
#include <unistd.h>
#endif

namespace {

using namespace WasmEdge::LLVM::Linker;
using namespace WasmEdge::LLVM::Test;

TEST(LinkerHostTest, UsesAOTArchitectureContract) {
#if defined(__x86_64__) || defined(_M_X64)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::X86_64);
#elif defined(__aarch64__) || defined(_M_ARM64)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::AArch64);
#elif defined(__riscv) && __riscv_xlen == 64
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::RISCV64);
#elif defined(__arm__) && __ARM_ARCH == 7 && defined(__ARM_PCS_VFP)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::ARMv7HardFloat);
#elif defined(__arm__) && __ARM_ARCH == 7
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::ARMv7);
#elif defined(_M_ARM) && _M_ARM == 7
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::ARMv7);
#elif defined(__s390x__)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::S390X);
#else
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::Unsupported);
#endif
}

void expectMachOBuildVersionsEqual(
    const std::vector<MachOBuildVersion> &Actual,
    const std::vector<MachOBuildVersion> &Expected) {
  ASSERT_EQ(Actual.size(), Expected.size());
  for (size_t I = 0; I < Expected.size(); ++I) {
    EXPECT_EQ(Actual[I].Platform, Expected[I].Platform);
    EXPECT_EQ(Actual[I].MinimumOS, Expected[I].MinimumOS);
    EXPECT_EQ(Actual[I].SDK, Expected[I].SDK);
    ASSERT_EQ(Actual[I].Tools.size(), Expected[I].Tools.size());
    for (size_t J = 0; J < Expected[I].Tools.size(); ++J) {
      EXPECT_EQ(Actual[I].Tools[J].Tool, Expected[I].Tools[J].Tool);
      EXPECT_EQ(Actual[I].Tools[J].Version, Expected[I].Tools[J].Version);
    }
  }
}

std::optional<std::vector<uint32_t>>
machOUnwindFunctionStarts(const std::vector<WasmEdge::Byte> &Content) {
  if (Content.size() < 28)
    return std::nullopt;
  const size_t Index = read32le(Content, 20);
  const uint32_t IndexCount = read32le(Content, 24);
  if (IndexCount == 0 || Index > Content.size() ||
      IndexCount > (Content.size() - Index) / 12)
    return std::nullopt;
  std::vector<uint32_t> Result;
  for (uint32_t I = 0; I + 1 < IndexCount; ++I) {
    const uint32_t Base = read32le(Content, Index + I * 12);
    const size_t Page = read32le(Content, Index + I * 12 + 4);
    if (Page > Content.size() || Content.size() - Page < 8)
      return std::nullopt;
    const uint32_t Kind = read32le(Content, Page);
    const auto Entries = static_cast<uint16_t>(read32le(Content, Page + 4));
    const auto Count = static_cast<uint16_t>(read32le(Content, Page + 4) >> 16);
    const size_t Width = Kind == 2 ? 8 : Kind == 3 ? 4 : 0;
    if (Width == 0 || Entries > Content.size() - Page ||
        Count > (Content.size() - Page - Entries) / Width)
      return std::nullopt;
    for (uint16_t J = 0; J < Count; ++J) {
      const uint32_t Entry = read32le(Content, Page + Entries + J * Width);
      Result.push_back(Kind == 2 ? Entry
                                 : Base + (Entry & UINT32_C(0x00FFFFFF)));
    }
  }
  return Result;
}

class GeneratedMachOLinkerTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

class GeneratedWindowsLinkerTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

class MSVCFltusedPolicyTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

LinkGraph makeUniversalCOFFPDataGraph(size_t FirstSize = 12,
                                      size_t SecondSize = 12) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  EXPECT_TRUE(Graph.beginInput("runtime-functions.obj"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 16, 64, 0x100, 0,
                               std::vector<WasmEdge::Byte>(64)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 2, 0x200, 0, {0, 0}});
  EXPECT_TRUE(Graph.addSection(Section{".xdata", SectionKind::Unwind, 4, 8,
                                       0x300, 0, std::vector<WasmEdge::Byte>(8),
                                       SectionPurpose::XData}));
  EXPECT_TRUE(Graph.addSection(
      Section{".pdata$a", SectionKind::Unwind, 4, FirstSize, 0x400, 0,
              std::vector<WasmEdge::Byte>(FirstSize), SectionPurpose::PData}));
  EXPECT_TRUE(Graph.addSection(
      Section{".pdata$b", SectionKind::Unwind, 4, SecondSize, 0x500, 0,
              std::vector<WasmEdge::Byte>(SecondSize), SectionPurpose::PData}));
  EXPECT_TRUE(Text && Data);
  EXPECT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  return Graph;
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsMalformedCOFFPData) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  for (auto Graph :
       {makeUniversalCOFFPDataGraph(13, 12), makeUniversalCOFFPDataGraph()}) {
    if (Graph.sections()[3].VirtualSize == 12) {
      for (const SectionId Id : {SectionId{3}, SectionId{4}}) {
        write32le(const_cast<Section &>(Graph.sections()[Id]).Content, 0,
                  0x100);
        write32le(const_cast<Section &>(Graph.sections()[Id]).Content, 4,
                  0x120);
        write32le(const_cast<Section &>(Graph.sections()[Id]).Content, 8,
                  0x300);
      }
    }
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
    EXPECT_TRUE(Bytes.empty());
  }
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsOverlappingCOFFPData) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  auto Graph = makeUniversalCOFFPDataGraph();
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 0, 0x100);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 4, 0x130);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 8, 0x300);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 0, 0x120);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 4, 0x140);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 8, 0x304);
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsAppliedGraphWithRebases) {
  REQUIRE_NATIVE_TARGET(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("rebases.o"));
  auto Text = Graph.addSection(Section{".text", SectionKind::Text, 8, 8, 0x1000,
                                       0, std::vector<WasmEdge::Byte>(8)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x2000, 8, {0, 0}});
  ASSERT_TRUE(Text && Data);
  auto Version = Graph.addSymbol(Symbol{"version", *Data, 0, 1, true});
  ASSERT_TRUE(Version);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(
      Graph.addRelocation(Relocation{*Text, 0, llvm::ELF::R_X86_64_64, *Version,
                                     0, false, ObjectFormat::ELF, 8}));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_FALSE(Graph.rebases().empty());
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  auto Result = UniversalWasmWriter::write(Graph, EmptyWasm, Output);
  ASSERT_FALSE(Result);
  EXPECT_TRUE(Bytes.empty());
  EXPECT_EQ(Result.error().SectionName, ".text");
  EXPECT_EQ(Result.error().Section, 0U);
  EXPECT_EQ(Result.error().Offset, 0U);
  EXPECT_EQ(Result.error().RelocationType, 1U);
  EXPECT_NE(Result.error().Message.find("width=8"), std::string::npos);
  EXPECT_NE(Result.error().Message.find("format=ELF"), std::string::npos);
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsMixedCOFFImageBases) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("mixed-image-base.obj"));
  ASSERT_TRUE(Graph.setImageBase(PEImageBase));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 1, 1, PEImageBase + 0x100, 0, {0}});
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 2, PEImageBase + 0x200, 0, {0, 0}});
  ASSERT_TRUE(
      Graph.addSection(Section{".bss", SectionKind::BSS, 1, 1, 0x300, 0, {}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsSectionAddressOverflow) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("section-overflow.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 1, 1, 0x100, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x200, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".bss", SectionKind::BSS, 1, 2, UINT64_MAX, 0, {}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsPDataAddressOverflow) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("pdata-overflow.obj"));
  auto Text = Graph.addSection(Section{".text", SectionKind::Text, 1, 0x20,
                                       PEImageBase + 0x100, 0,
                                       std::vector<WasmEdge::Byte>(0x20)});
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 2, PEImageBase + 0x200, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".xdata", SectionKind::Unwind, 1, 4, PEImageBase + 0x300, 0,
              std::vector<WasmEdge::Byte>(4), SectionPurpose::XData}));
  std::vector<WasmEdge::Byte> PData(12);
  write32le(PData, 0, 0x100);
  write32le(PData, 4, 0x120);
  write32le(PData, 8, 0x300);
  ASSERT_TRUE(Graph.addSection(Section{".pdata", SectionKind::Unwind, 1, 12,
                                       UINT64_MAX - 5, 0, std::move(PData),
                                       SectionPurpose::PData}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, RejectsInvalidSemanticSymbolTables) {
  REQUIRE_NATIVE_TARGET(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  auto MakeGraph = [] {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    EXPECT_TRUE(Graph.beginInput("symbols.o"));
    EXPECT_TRUE(
        Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
    EXPECT_TRUE(Graph.addSection(
        Section{".data", SectionKind::Data, 1, 3, 1, 1, {1, 2, 3}}));
    return Graph;
  };
  auto Write = [&](LinkGraph &Graph) {
    EXPECT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    return UniversalWasmWriter::write(Graph, EmptyWasm, Output);
  };

  auto Missing = MakeGraph();
  EXPECT_FALSE(Write(Missing));

  auto LocalShadow = MakeGraph();
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"local_version", 1, 0, 1, false}));
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"version", 1, 1, 1, true}));
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"intrinsics", 1, 2, 1, true}));
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  EXPECT_TRUE(Write(LocalShadow));

  auto Duplicate = MakeGraph();
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"version", 1, 0, 1, true}));
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"other", 0, 0, 1, true, "f0"}));
  EXPECT_FALSE(Write(Duplicate));

  auto Sparse = MakeGraph();
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"version", 1, 0, 1, true}));
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"f2", 0, 0, 1, true}));
  EXPECT_FALSE(Write(Sparse));

  auto NonCanonical = MakeGraph();
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"version", 1, 0, 1, true}));
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"f01", 0, 0, 1, true}));
  EXPECT_TRUE(Write(NonCanonical));

  LinkGraph Darwin(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Darwin.beginInput("symbols.o"));
  ASSERT_TRUE(
      Darwin.addSection(Section{"__text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  ASSERT_TRUE(Darwin.addSection(
      Section{"__data", SectionKind::Data, 1, 2, 1, 1, {1, 2}}));
  ASSERT_TRUE(Darwin.addSymbol(Symbol{"_version", 1, 0, 1, true}));
  ASSERT_TRUE(Darwin.addSymbol(Symbol{"_intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(Darwin.addSymbol(Symbol{"_f0", 0, 0, 1, true}));
  EXPECT_TRUE(Write(Darwin));
}

TEST(LinkerWriterTest, RejectsSemanticIndicesBeyondVectorCapacity) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Capacity = std::vector<uint64_t>().max_size();
  for (const char Prefix : {'t', 'f'}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("oversized-symbol.o"));
    auto Text =
        Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 1, 2, 1, 1, {1, 2}});
    ASSERT_TRUE(Text && Data);
    ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
    ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
    ASSERT_TRUE(Graph.addSymbol(Symbol{
        std::string(1, Prefix) + std::to_string(Capacity), *Text, 0, 1, true}));
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    bool Written = true;

    EXPECT_NO_THROW(Written = static_cast<bool>(
                        UniversalWasmWriter::write(Graph, EmptyWasm, Output)));
    EXPECT_FALSE(Written);
    EXPECT_TRUE(Bytes.empty());
  }
}

#if WASMEDGE_OS_LINUX && (defined(__x86_64__) || defined(_M_X64))
#endif

#if WASMEDGE_OS_LINUX
#endif

#if WASMEDGE_OS_MACOS

#endif

#if WASMEDGE_OS_WINDOWS
#endif

#if !WASMEDGE_OS_WINDOWS

#endif

TEST(GeneratedMachOLinkerTest, LinksMixedRawDwarfAndCompactX86_64Unwind) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(true, false, true);
  const auto Input = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Input.Content, 0U);
  ASSERT_TRUE(Input.Relocations.empty());
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->ehFrameReferences()[0].Symbol].Name, "_f0");
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->compactUnwind()[0].Function].Name, "_t0");

  ASSERT_TRUE(Graph->pruneUnreferencedMachOEHFrame());
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->ehFrameReferences()[0].Symbol].Name, "_f0");
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->compactUnwind()[0].Function].Name, "_t0");
  EXPECT_TRUE(std::any_of(Graph->sections().begin(), Graph->sections().end(),
                          [](const auto &Section) {
                            return Section.Purpose == SectionPurpose::EHFrame;
                          }));
  ASSERT_TRUE(reserveMachOUnwindInfo(*Graph));
  ASSERT_TRUE(MachOWriter::layout(*Graph));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph));
  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 1U);
  const auto F0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_f0"; });
  const auto T0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_t0"; });
  ASSERT_NE(F0, Graph->symbols().end());
  ASSERT_NE(T0, Graph->symbols().end());
  const uint64_t F0Address =
      Graph->sections()[F0->Section].Address + F0->Offset;
  const uint64_t T0Address =
      Graph->sections()[T0->Section].Address + T0->Offset;
  EXPECT_NE(std::find(Starts->begin(), Starts->end(), F0Address),
            Starts->end());
  EXPECT_EQ(
      Graph->sections()[Graph->symbols()[Graph->compactUnwind()[0].Function]
                            .Section]
              .Address +
          Graph->symbols()[Graph->compactUnwind()[0].Function].Offset,
      T0Address);
  ASSERT_TRUE(applyRelocations(*Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(*Graph));
  const auto Unwind = std::find_if(
      Graph->sections().begin(), Graph->sections().end(),
      [](const auto &S) { return S.Purpose == SectionPurpose::UnwindInfo; });
  ASSERT_NE(Unwind, Graph->sections().end());
  const auto CompactStarts = machOUnwindFunctionStarts(Unwind->Content);
  ASSERT_TRUE(CompactStarts);
  EXPECT_NE(std::find(CompactStarts->begin(), CompactStarts->end(), T0Address),
            CompactStarts->end());

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(*Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "mixed-x86_64.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object));
  std::set<std::string> Sections;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    Sections.emplace(Name->str());
  }
  EXPECT_TRUE(Sections.count("__eh_frame"));
  EXPECT_TRUE(Sections.count("__unwind_info"));
  EXPECT_FALSE(Sections.count("__compact_unwind"));
}

TEST_P(GeneratedMachOLinkerTest, LinksGeneratedMacOSObjects) {
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  const auto Architecture = Test.Architecture;
  REQUIRE_LLVM_TARGET(Triple);
  const auto Object =
      makeObject(llvm::Triple(Triple), false, false, "f0", {}, false, false,
                 "generic", {}, true, false, false, false, false, false, {},
                 true, true, false, false, false, true);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "generated.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  std::set<std::string> InputSections;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    InputSections.emplace(Name->str());
  }
  EXPECT_EQ(InputSections.count("__compact_unwind") != 0,
            Architecture == Target::AArch64)
      << Triple;
  EXPECT_EQ(InputSections.count("__eh_frame") != 0,
            Architecture == Target::X86_64)
      << Triple;
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph) << Triple;
  const auto InputBuildVersions = Graph->machOBuildVersions();
#if LLVM_VERSION_MAJOR < 11
#else
  ASSERT_FALSE(InputBuildVersions.empty()) << Triple;
#endif
  const bool HasCompact = !Graph->compactUnwind().empty();
  const bool NeedsDwarf =
      std::any_of(Graph->compactUnwind().begin(), Graph->compactUnwind().end(),
                  [](const auto &Record) { return Record.FDE.has_value(); });
  const bool HasInputEHFrame =
      std::any_of(Graph->sections().begin(), Graph->sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::EHFrame;
                  });
  if (HasCompact) {
    ASSERT_TRUE(Graph->pruneUnreferencedMachOEHFrame()) << Triple;
  }
  if (!Graph->compactUnwind().empty()) {
    ASSERT_TRUE(reserveMachOUnwindInfo(*Graph)) << Triple;
  }
  ASSERT_TRUE(MachOWriter::layout(*Graph)) << Triple;
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph)) << Triple;
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph)) << Triple;
  ASSERT_TRUE(applyRelocations(*Graph)) << Triple;
  if (!Graph->compactUnwind().empty()) {
    ASSERT_TRUE(populateMachOUnwindInfo(*Graph)) << Triple;
    const auto Unwind =
        std::find_if(Graph->sections().begin(), Graph->sections().end(),
                     [](const auto &Section) {
                       return Section.Purpose == SectionPurpose::UnwindInfo;
                     });
    ASSERT_NE(Unwind, Graph->sections().end()) << Triple;
  }
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(*Graph, Output)) << Triple;
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "generated.dylib"));
  ASSERT_TRUE(static_cast<bool>(Image))
      << Triple << " " << llvm::toString(Image.takeError());
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Image);
  ASSERT_NE(MachO, nullptr) << Triple;
  std::vector<MachOBuildVersion> OutputBuildVersions;
  for (const auto &Command : MachO->load_commands()) {
    if (Command.C.cmd != llvm::MachO::LC_BUILD_VERSION)
      continue;
    const WasmEdge::Span<const WasmEdge::Byte> CommandBytes(
        reinterpret_cast<const WasmEdge::Byte *>(Command.Ptr),
        Command.C.cmdsize);
    auto Version =
        Internal::parseMachOBuildVersion(CommandBytes, Endianness::Little);
    ASSERT_TRUE(Version) << Triple;
    OutputBuildVersions.push_back(std::move(*Version));
  }
  expectMachOBuildVersionsEqual(OutputBuildVersions, InputBuildVersions);
  bool HasEHFrame = false;
  std::vector<WasmEdge::Byte> FinalUnwind;
  for (const auto &Section : (*Image)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    HasEHFrame |= *Name == "__eh_frame";
    if (*Name == "__unwind_info") {
      auto Content = Section.getContents();
      ASSERT_TRUE(static_cast<bool>(Content));
      FinalUnwind.assign(Content->bytes_begin(), Content->bytes_end());
    }
  }
  EXPECT_EQ(HasEHFrame, HasInputEHFrame && (!HasCompact || NeedsDwarf))
      << Triple;
  EXPECT_EQ(!FinalUnwind.empty(), HasCompact) << Triple;
  if (HasCompact) {
    const auto FunctionOffsets = machOUnwindFunctionStarts(FinalUnwind);
    ASSERT_TRUE(FunctionOffsets) << Triple;
    for (const std::string_view Name : {"_f0", "_t0"}) {
      const auto Symbol =
          std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                       [&](const auto &Value) { return Value.Name == Name; });
      ASSERT_NE(Symbol, Graph->symbols().end()) << Triple << " " << Name;
      const auto Address = static_cast<uint32_t>(
          Graph->sections()[Symbol->Section].Address + Symbol->Offset);
      EXPECT_NE(
          std::find(FunctionOffsets->begin(), FunctionOffsets->end(), Address),
          FunctionOffsets->end())
          << Triple << " " << Name;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedMachOLinkerTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx10.15.0", Target::X86_64},
        ObjectArchitectureCase{"arm64-apple-macosx10.15.0", Target::AArch64}),
    objectArchitectureCaseName);

TEST_P(GeneratedWindowsLinkerTest, LinksGeneratedWindowsObjects) {
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  const auto Architecture = Test.Architecture;
  REQUIRE_LLVM_TARGET(Triple);
  const auto Object = makeObject(llvm::Triple(Triple), false, false, "f0",
                                 "/EXPORT:f0 /EXPORT:value", false, false,
                                 "generic", {}, true, true, false, false, true);
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph) << Triple;
  const std::set<uint32_t> Allowed =
      Architecture == Target::X86_64
          ? std::set<uint32_t>{llvm::COFF::IMAGE_REL_AMD64_ADDR64,
                               llvm::COFF::IMAGE_REL_AMD64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_AMD64_REL32,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_1,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_2,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_3,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_4,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_5}
          : std::set<uint32_t>{llvm::COFF::IMAGE_REL_ARM64_ADDR64,
                               llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L};
  const std::set<uint32_t> Core =
      Architecture == Target::X86_64
          ? std::set<uint32_t>{llvm::COFF::IMAGE_REL_AMD64_ADDR64,
                               llvm::COFF::IMAGE_REL_AMD64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_AMD64_REL32}
          : std::set<uint32_t>{llvm::COFF::IMAGE_REL_ARM64_ADDR64,
                               llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21};
  const std::set<uint32_t> Expected =
      Architecture == Target::X86_64
          ? Core
          : std::set<uint32_t>{llvm::COFF::IMAGE_REL_ARM64_ADDR64,
                               llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L};
  std::set<uint32_t> Actual;
  for (const auto &Relocation : Graph->relocations()) {
    Actual.insert(Relocation.Type);
    EXPECT_EQ(Relocation.PatchSize,
              Relocation.Type == llvm::COFF::IMAGE_REL_AMD64_ADDR64 ||
                      Relocation.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR64
                  ? 8
                  : 4);
  }
  EXPECT_TRUE(std::includes(Allowed.begin(), Allowed.end(), Actual.begin(),
                            Actual.end()))
      << Triple;
  EXPECT_TRUE(
      std::includes(Actual.begin(), Actual.end(), Core.begin(), Core.end()))
      << Triple;
  EXPECT_EQ(Actual, Expected) << Triple;
  auto RawObject =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "generated.obj"));
  ASSERT_TRUE(static_cast<bool>(RawObject));
  for (const auto &Symbol : (*RawObject)->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    EXPECT_NE(*Name, "_fltused");
    EXPECT_NE(*Name, "_DllMainCRTStartup");
  }
  ASSERT_TRUE(PEWriter::layout(*Graph)) << Triple;
  ASSERT_TRUE(applyRelocations(*Graph)) << Triple;
  ASSERT_FALSE(Graph->rebases().empty()) << Triple;
  for (const auto &Rebase : Graph->rebases()) {
    EXPECT_EQ(Rebase.Width, 8U);
    EXPECT_EQ(Rebase.Type,
              Architecture == Target::X86_64
                  ? static_cast<uint32_t>(llvm::COFF::IMAGE_REL_AMD64_ADDR64)
                  : static_cast<uint32_t>(llvm::COFF::IMAGE_REL_ARM64_ADDR64));
  }
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(*Graph, "generated.dll", Output)) << Triple;
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "generated.dll"));
  ASSERT_TRUE(static_cast<bool>(Image))
      << Triple << " " << llvm::toString(Image.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  EXPECT_EQ(PE->import_directory_begin(), PE->import_directory_end());
  auto Read = [&](size_t Offset, uint8_t Width) {
    uint64_t Result = 0;
    for (uint8_t I = 0; I < Width; ++I)
      Result |= static_cast<uint64_t>(Bytes[Offset + I]) << (I * 8);
    return Result;
  };
  const uint32_t HeaderOffset = static_cast<uint32_t>(Read(0x3C, 4));
  const size_t Optional = HeaderOffset + 24;
  EXPECT_EQ(Read(Optional + 16, 4), 0U);
  for (const size_t Directory : {size_t{1}, size_t{12}})
    EXPECT_EQ(Read(Optional + 112 + Directory * 8, 8), 0U);
  std::set<std::string> Exports;
  for (const auto &Export : PE->export_directories()) {
    llvm::StringRef Name;
    ASSERT_FALSE(Export.getSymbolName(Name));
    Exports.emplace(Name.str());
  }
  EXPECT_EQ(Exports, (std::set<std::string>{"f0", "value"}));
  EXPECT_FALSE(Exports.count("_fltused"));
  EXPECT_FALSE(Exports.count("_DllMainCRTStartup"));
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedWindowsLinkerTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64},
        ObjectArchitectureCase{"aarch64-pc-windows-msvc", Target::AArch64}),
    objectArchitectureCaseName);

const Symbol *findGraphSymbol(const LinkGraph &Graph, std::string_view Name) {
  const auto Found = std::find_if(
      Graph.symbols().begin(), Graph.symbols().end(), [&](const auto &Value) {
        return Value.Name == Name ||
               (!Value.Name.empty() && Value.Name[0] == '_' &&
                std::string_view(Value.Name).substr(1) == Name);
      });
  return Found == Graph.symbols().end() ? nullptr : &*Found;
}

std::string segmentName(const llvm::MachO::segment_command_64 &Segment) {
  return std::string(Segment.segname,
                     strnlen(Segment.segname, sizeof(Segment.segname)));
}

std::vector<WasmEdge::Byte> makeConstantDataObject(const llvm::Triple &Triple) {
  return makeObject(
      Triple, false, false, "f0",
      Triple.isOSBinFormatCOFF() ? "/EXPORT:f0 /EXPORT:value" : "", false,
      false, "generic", {}, true, true, false, false, true, false, {}, true);
}

TEST_P(GeneratedWindowsLinkerTest, KeepsGeneratedConstantsReadOnly) {
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  REQUIRE_LLVM_TARGET(Triple);
  auto Graph = ObjectReader::read(makeConstantDataObject(llvm::Triple(Triple)),
                                  Test.Architecture);
  ASSERT_TRUE(Graph) << Triple;
  const auto *Version = findGraphSymbol(*Graph, "version");
  const auto *Table = findGraphSymbol(*Graph, "table");
  const auto *Value = findGraphSymbol(*Graph, "value");
  ASSERT_NE(Version, nullptr) << Triple;
  ASSERT_NE(Table, nullptr) << Triple;
  ASSERT_NE(Value, nullptr) << Triple;
  EXPECT_EQ(Graph->sections()[Version->Section].Kind, SectionKind::ReadOnly)
      << Triple;
  EXPECT_EQ(Graph->sections()[Table->Section].Kind, SectionKind::ReadOnly)
      << Triple;
  EXPECT_EQ(Graph->sections()[Value->Section].Kind, SectionKind::Data)
      << Triple;

  ASSERT_TRUE(PEWriter::layout(*Graph)) << Triple;
  ASSERT_TRUE(applyRelocations(*Graph)) << Triple;
  EXPECT_TRUE(std::any_of(Graph->rebases().begin(), Graph->rebases().end(),
                          [&](const auto &Rebase) {
                            return Rebase.Section == Table->Section &&
                                   Rebase.Offset == Table->Offset;
                          }))
      << Triple;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(*Graph, "constants.dll", Output)) << Triple;
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "constants.dll"));
  ASSERT_TRUE(static_cast<bool>(Image))
      << Triple << " " << llvm::toString(Image.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr) << Triple;
  const uint64_t ImageBase = PE->getImageBase();
  const auto RVAOf = [&](const Symbol &Item) {
    return static_cast<uint32_t>(Graph->sections()[Item.Section].Address +
                                 Item.Offset - ImageBase);
  };
  const auto OutputSection =
      [&](uint32_t RVA) -> const llvm::object::coff_section * {
    for (const auto &Section : (*Image)->sections()) {
      const auto *Header = PE->getCOFFSection(Section);
      if (RVA >= Header->VirtualAddress &&
          RVA - Header->VirtualAddress < Header->VirtualSize)
        return Header;
    }
    return nullptr;
  };
  constexpr uint32_t Writable = llvm::COFF::IMAGE_SCN_MEM_WRITE;
  for (const auto *Constant : {Version, Table}) {
    const auto *Header = OutputSection(RVAOf(*Constant));
    ASSERT_NE(Header, nullptr) << Triple << " " << Constant->Name;
    EXPECT_EQ(llvm::StringRef(Header->Name, 6), ".rdata")
        << Triple << " " << Constant->Name;
    EXPECT_EQ(Header->Characteristics & Writable, 0U)
        << Triple << " " << Constant->Name;
    EXPECT_NE(Header->Characteristics & llvm::COFF::IMAGE_SCN_MEM_READ, 0U)
        << Triple << " " << Constant->Name;
  }
  const auto *DataHeader = OutputSection(RVAOf(*Value));
  ASSERT_NE(DataHeader, nullptr) << Triple;
  EXPECT_NE(DataHeader->Characteristics & Writable, 0U) << Triple;

  auto Read = [&](size_t Offset, uint8_t Width) {
    uint64_t Result = 0;
    for (uint8_t I = 0; I < Width; ++I)
      Result |= static_cast<uint64_t>(Bytes[Offset + I]) << (I * 8);
    return Result;
  };
  const size_t Optional = static_cast<size_t>(Read(0x3C, 4)) + 24;
  constexpr size_t BaseRelocationDirectory = 5;
  const auto RelocationRVA = static_cast<uint32_t>(
      Read(Optional + 112 + BaseRelocationDirectory * 8, 4));
  const auto RelocationSize = static_cast<uint32_t>(
      Read(Optional + 112 + BaseRelocationDirectory * 8 + 4, 4));
  const auto *RelocationHeader = OutputSection(RelocationRVA);
  ASSERT_NE(RelocationHeader, nullptr) << Triple;
  const size_t RelocationOffset = RelocationHeader->PointerToRawData +
                                  RelocationRVA -
                                  RelocationHeader->VirtualAddress;
  ASSERT_LE(RelocationOffset + RelocationSize, Bytes.size()) << Triple;
  std::set<uint32_t> Rebased;
  for (size_t Block = RelocationOffset;
       Block + 8 <= RelocationOffset + RelocationSize;) {
    const auto Page = static_cast<uint32_t>(Read(Block, 4));
    const auto BlockSize = static_cast<uint32_t>(Read(Block + 4, 4));
    ASSERT_GE(BlockSize, 8U) << Triple;
    for (size_t Entry = Block + 8; Entry < Block + BlockSize; Entry += 2) {
      const auto Raw = static_cast<uint16_t>(Read(Entry, 2));
      if ((Raw >> 12) == llvm::COFF::IMAGE_REL_BASED_DIR64)
        Rebased.insert(Page + (Raw & 0xFFF));
    }
    Block += BlockSize;
  }
  EXPECT_EQ(Rebased.count(RVAOf(*Table)), 1U) << Triple;
}

TEST_P(GeneratedMachOLinkerTest, KeepsGeneratedConstantsReadOnly) {
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  REQUIRE_LLVM_TARGET(Triple);
  const auto Object = makeConstantDataObject(llvm::Triple(Triple));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "constants.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  std::set<std::string> InputSections;
  for (const auto &Section : (*Parsed)->sections()) {
    const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&**Parsed);
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    InputSections.emplace(
        MachO->getSectionFinalSegmentName(Section.getRawDataRefImpl()).str() +
        "," + Name->str());
  }
  EXPECT_TRUE(InputSections.count("__TEXT,__const")) << Triple;
  EXPECT_TRUE(InputSections.count("__DATA,__const")) << Triple;

  auto Graph = ObjectReader::read(Object, Test.Architecture);
  ASSERT_TRUE(Graph) << Triple;
  const auto *Version = findGraphSymbol(*Graph, "version");
  const auto *Table = findGraphSymbol(*Graph, "table");
  const auto *Value = findGraphSymbol(*Graph, "value");
  ASSERT_NE(Version, nullptr) << Triple;
  ASSERT_NE(Table, nullptr) << Triple;
  ASSERT_NE(Value, nullptr) << Triple;
  EXPECT_EQ(Graph->sections()[Version->Section].Kind, SectionKind::ReadOnly)
      << Triple;
  EXPECT_EQ(Graph->sections()[Table->Section].Kind, SectionKind::ReadOnly)
      << Triple;
  EXPECT_EQ(Graph->sections()[Value->Section].Kind, SectionKind::Data)
      << Triple;

  if (!Graph->compactUnwind().empty()) {
    ASSERT_TRUE(Graph->pruneUnreferencedMachOEHFrame()) << Triple;
    ASSERT_TRUE(reserveMachOUnwindInfo(*Graph)) << Triple;
  }
  ASSERT_TRUE(MachOWriter::layout(*Graph)) << Triple;
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph)) << Triple;
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph)) << Triple;
  ASSERT_TRUE(applyRelocations(*Graph)) << Triple;
  if (!Graph->compactUnwind().empty()) {
    ASSERT_TRUE(populateMachOUnwindInfo(*Graph)) << Triple;
  }
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(*Graph, Output)) << Triple;
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "constants.dylib"));
  ASSERT_TRUE(static_cast<bool>(Image))
      << Triple << " " << llvm::toString(Image.takeError());
  auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Image);
  ASSERT_NE(MachO, nullptr) << Triple;
  const auto AddressOf = [&](const Symbol &Item) {
    return Graph->sections()[Item.Section].Address + Item.Offset;
  };
  const auto SegmentOf =
      [&](uint64_t Address) -> std::optional<llvm::MachO::segment_command_64> {
    for (const auto &Command : MachO->load_commands()) {
      if (Command.C.cmd != llvm::MachO::LC_SEGMENT_64)
        continue;
      const auto Segment = MachO->getSegment64LoadCommand(Command);
      if (Address >= Segment.vmaddr &&
          Address - Segment.vmaddr < Segment.vmsize)
        return Segment;
    }
    return std::nullopt;
  };
  constexpr uint32_t SegmentReadOnly = 0x10;
  for (const auto *Constant : {Version, Table}) {
    const auto Segment = SegmentOf(AddressOf(*Constant));
    ASSERT_TRUE(Segment) << Triple << " " << Constant->Name;
    EXPECT_EQ(segmentName(*Segment), "__DATA_CONST")
        << Triple << " " << Constant->Name;
    EXPECT_EQ(Segment->flags & SegmentReadOnly, SegmentReadOnly)
        << Triple << " " << Constant->Name;
    EXPECT_EQ(Segment->initprot & llvm::MachO::VM_PROT_EXECUTE, 0U)
        << Triple << " " << Constant->Name;
  }
  const auto DataSegment = SegmentOf(AddressOf(*Value));
  ASSERT_TRUE(DataSegment) << Triple;
  EXPECT_EQ(segmentName(*DataSegment), "__DATA") << Triple;
  EXPECT_EQ(DataSegment->flags & SegmentReadOnly, 0U) << Triple;

  llvm::Error Error = llvm::Error::success();
  std::set<std::pair<std::string, uint64_t>> Rebased;
  for (const auto &Entry : MachO->rebaseTable(Error))
    Rebased.emplace(Entry.segmentName().str(), Entry.address());
  ASSERT_FALSE(static_cast<bool>(Error))
      << Triple << " " << llvm::toString(std::move(Error));
  EXPECT_EQ(Rebased.count({"__DATA_CONST", AddressOf(*Table)}), 1U) << Triple;
}

TEST(PEWriterTest, DiscardsUnreferencedCompilerRequiredFltusedMarker) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Object =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "f0",
                 "/EXPORT:f0", false, false, "generic", {}, true, true, false,
                 false, false, false, {}, false, false, true);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "floating.obj"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  bool HasUndefinedFltused = false;
  for (const auto &Symbol : (*Parsed)->symbols()) {
    auto Name = Symbol.getName();
    llvm::Expected<uint32_t> Flags = Symbol.getFlags();
    ASSERT_TRUE(Name && Flags);
    HasUndefinedFltused |=
        *Name == "_fltused" &&
        (*Flags & llvm::object::SymbolRef::SF_Undefined) != 0;
  }
  EXPECT_TRUE(HasUndefinedFltused);
  EXPECT_FALSE(ObjectReader::read(Object, Target::X86_64));
  auto Graph =
      ObjectReader::read(Object, Target::X86_64,
                         ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(std::none_of(
      Graph->symbols().begin(), Graph->symbols().end(), [](const auto &Symbol) {
        return Symbol.Name == "_fltused" || Symbol.Name == "_DllMainCRTStartup";
      }));
  const auto ResolvedObject =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "f0",
                 "/EXPORT:f0", false, false, "generic", {}, true, true, false,
                 false, false, false, {}, false, false, true, true);
  auto Resolved = ObjectReader::read(ResolvedObject, Target::X86_64);
  ASSERT_TRUE(Resolved);
  const auto TextContent = [](const LinkGraph &Value) {
    std::vector<std::vector<WasmEdge::Byte>> Result;
    for (const auto &Section : Value.sections())
      if (Section.Kind == SectionKind::Text)
        Result.push_back(Section.Content);
    return Result;
  };
  EXPECT_EQ(TextContent(*Graph), TextContent(*Resolved));
  const auto RealSymbols = [](const LinkGraph &Value) {
    std::set<std::string> Result;
    for (const auto &Symbol : Value.symbols())
      if (Symbol.Name != "_fltused")
        Result.emplace(Symbol.Name);
    return Result;
  };
  EXPECT_EQ(RealSymbols(*Graph), RealSymbols(*Resolved));
  ASSERT_EQ(Graph->relocations().size(), Resolved->relocations().size());
  for (size_t I = 0; I < Graph->relocations().size(); ++I) {
    const auto &Left = Graph->relocations()[I];
    const auto &Right = Resolved->relocations()[I];
    EXPECT_EQ(Left.Section, Right.Section);
    EXPECT_EQ(Left.Offset, Right.Offset);
    EXPECT_EQ(Left.Type, Right.Type);
    EXPECT_EQ(Left.Addend, Right.Addend);
    EXPECT_EQ(Graph->symbols()[Left.Symbol].Name,
              Resolved->symbols()[Right.Symbol].Name);
  }
  ASSERT_TRUE(PEWriter::layout(*Graph));
  ASSERT_TRUE(applyRelocations(*Graph));
  EXPECT_TRUE(std::none_of(Graph->relocations().begin(),
                           Graph->relocations().end(), [&](const auto &Rel) {
                             return Graph->symbols()[Rel.Symbol].Name ==
                                    "_fltused";
                           }));
  std::vector<WasmEdge::Byte> ImageBytes;
  Writer ImageWriter(ImageBytes);
  ASSERT_TRUE(PEWriter::write(*Graph, "floating.dll", ImageWriter));
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(ImageBytes.data()),
                      ImageBytes.size()),
      "floating.dll"));
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  EXPECT_EQ(pe32PlusHeader(*PE)->AddressOfEntryPoint, 0U);
  EXPECT_EQ(PE->import_directory_begin(), PE->import_directory_end());
  auto Read = [&](size_t Offset, uint8_t Width) {
    uint64_t Result = 0;
    for (uint8_t I = 0; I < Width; ++I)
      Result |= static_cast<uint64_t>(ImageBytes[Offset + I]) << (I * 8);
    return Result;
  };
  const uint32_t PEOffset = static_cast<uint32_t>(Read(0x3C, 4));
  const size_t Optional = PEOffset + 24;
  for (const size_t Directory : {size_t{1}, size_t{12}})
    EXPECT_EQ(Read(Optional + 112 + Directory * 8, 8), 0U);
  std::set<std::string> Exports;
  for (const auto &Export : PE->export_directories()) {
    llvm::StringRef Name;
    ASSERT_FALSE(Export.getSymbolName(Name));
    Exports.emplace(Name.str());
  }
  EXPECT_EQ(Exports, (std::set<std::string>{"f0"}));
}

TEST_P(MSVCFltusedPolicyTest, AllowsOnlyUnreferencedMarker) {
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Object =
      makeObject(llvm::Triple(Test.Triple), false, false, "f0", {}, false,
                 false, "generic", {}, false, false, false, false, false, false,
                 std::string(Marker));
  EXPECT_FALSE(ObjectReader::read(Object, Test.Architecture)) << Test.Triple;
  auto Graph =
      ObjectReader::read(Object, Test.Architecture,
                         ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused);
  ASSERT_TRUE(Graph) << Test.Triple;
  EXPECT_TRUE(std::none_of(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_fltused"; }));
}

TEST_P(MSVCFltusedPolicyTest, RejectsReferencedMarker) {
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  constexpr std::string_view Referenced = R"(
.data
.p2align 3
.quad _fltused
)";
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Object =
      makeObject(llvm::Triple(Test.Triple), false, false, "f0", {}, false,
                 false, "generic", {}, false, false, false, false, false, false,
                 std::string(Marker) + std::string(Referenced));
  EXPECT_FALSE(
      ObjectReader::read(Object, Test.Architecture,
                         ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused))
      << Test.Triple;
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, MSVCFltusedPolicyTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64},
        ObjectArchitectureCase{"aarch64-pc-windows-msvc", Target::AArch64}),
    objectArchitectureCaseName);

TEST(LinkerHostTest, MapsEveryTargetToAOTArchitecture) {
  using WasmEdge::AOT::Architecture;
  const std::array<std::pair<Target, Architecture>, 5> Pairs{
      {{Target::X86_64, Architecture::X86_64},
       {Target::AArch64, Architecture::AArch64},
       {Target::ARM, Architecture::ARMv7},
       {Target::RISCV64, Architecture::RISCV64},
       {Target::S390X, Architecture::S390X}}};
  for (const auto &[TargetValue, ArchitectureValue] : Pairs) {
    EXPECT_EQ(Internal::architecture(TargetValue), ArchitectureValue);
    EXPECT_EQ(Internal::target(ArchitectureValue), TargetValue);
  }
  EXPECT_EQ(
      Internal::architecture(Target::ARM, llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
      Architecture::ARMv7HardFloat);
  EXPECT_EQ(
      Internal::architecture(Target::ARM, llvm::ELF::EF_ARM_ABI_FLOAT_SOFT),
      Architecture::ARMv7);
  EXPECT_EQ(Internal::target(Architecture::ARMv7HardFloat), Target::ARM);
  EXPECT_FALSE(Internal::target(Architecture::Unsupported));
  EXPECT_FALSE(Internal::target(static_cast<Architecture>(0)));
  EXPECT_FALSE(Internal::architecture(static_cast<Target>(UINT8_MAX)));
}

TEST(LinkerWriterTest, RejectsWritesAfterCloseAndDuplicateExports) {
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(Output.writeName("a"));
  ASSERT_TRUE(Output.close());
  EXPECT_FALSE(Output.writeLengthPrefixed({}));
  EXPECT_FALSE(Output.writeByte(1));
  EXPECT_EQ(Bytes, (std::vector<WasmEdge::Byte>{0x01, 'a'}));

  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"first", *Text, 0, 0, true, "same"}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"second", *Text, 0, 0, true, "same"}));
  auto Exports = sortedUniqueExports(Graph);
  ASSERT_FALSE(Exports);
  EXPECT_EQ(Exports.error().Message, "duplicate exported symbol 'same'");
}

TEST(LinkerWriterTest, CounterMatchesBufferedOutput) {
  std::vector<WasmEdge::Byte> Bytes;
  uint64_t Count = 0;
  Writer Buffered(Bytes);
  Writer Counter(Count);
  for (auto *Output : {&Buffered, &Counter}) {
    ASSERT_TRUE(Output->writeULEB(300));
    ASSERT_TRUE(Output->writeZeros(5000));
    ASSERT_TRUE(Output->writeByte(7));
    ASSERT_TRUE(Output->flush());
  }
  ASSERT_EQ(Bytes.size(), 5003U);
  EXPECT_EQ(Count, Bytes.size());
  EXPECT_EQ(Bytes[0], 0xACU);
  EXPECT_EQ(Bytes[1], 0x02U);
  EXPECT_TRUE(std::all_of(Bytes.begin() + 2, Bytes.end() - 1,
                          [](WasmEdge::Byte Value) { return Value == 0; }));
  EXPECT_EQ(Bytes.back(), 7U);
}

#if !WASMEDGE_OS_WINDOWS
#endif

#if !WASMEDGE_OS_WINDOWS
#endif

TEST(PEWriterTest, MergesIslandsPlacedBeforeTheirSection) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("island.obj"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}});
  ASSERT_TRUE(Text);
  Section Island{".text$thunks",
                 SectionKind::Text,
                 4,
                 4,
                 0,
                 0,
                 std::vector<WasmEdge::Byte>{0x1F, 0x20, 0x03, 0xD5}};
  Island.Anchor = SectionAnchor{*Text, false};
  auto IslandId = Graph.addSection(std::move(Island));
  ASSERT_TRUE(IslandId);
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_LT(Graph.sections()[*IslandId].Address,
            Graph.sections()[*Text].Address);
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "island.dll", Output));
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "island.dll"));
  ASSERT_TRUE(static_cast<bool>(Image)) << llvm::toString(Image.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  const uint64_t IslandRVA = Graph.sections()[*IslandId].Address - PEImageBase;
  const uint64_t TextRVA = Graph.sections()[*Text].Address - PEImageBase;
  bool FoundText = false;
  for (const auto &Value : PE->sections()) {
    const auto *Header = PE->getCOFFSection(Value);
    if (Header->VirtualAddress != IslandRVA)
      continue;
    FoundText = true;
    auto Contents = Value.getContents();
    ASSERT_TRUE(static_cast<bool>(Contents))
        << llvm::toString(Contents.takeError());
    const uint64_t TextOffset = TextRVA - IslandRVA;
    ASSERT_GE(Contents->size(), TextOffset + 4);
    EXPECT_EQ(Contents->substr(0, 4), llvm::StringRef("\x1F\x20\x03\xD5", 4));
    EXPECT_EQ(Contents->substr(TextOffset, 4),
              llvm::StringRef("\xC0\x03\x5F\xD6", 4));
  }
  EXPECT_TRUE(FoundText);
}

TEST(PEWriterTest, OmitsEmptyOutputSections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("empty.obj"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(
      Graph.addSection(Section{".data", SectionKind::Data, 4, 0, 0, 0, {}}));
  ASSERT_TRUE(
      Graph.addSection(Section{".bss", SectionKind::BSS, 8, 8, 0, 0, {}}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "empty.dll", Output));
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "empty.dll"));
  ASSERT_TRUE(static_cast<bool>(Image)) << llvm::toString(Image.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  uint64_t PreviousEnd = 0;
  std::vector<std::string> Names;
  for (const auto &Value : PE->sections()) {
    auto Name = sectionName(Value);
    ASSERT_TRUE(static_cast<bool>(Name)) << llvm::toString(Name.takeError());
    Names.push_back(Name->str());
    const auto *Header = PE->getCOFFSection(Value);
    EXPECT_NE(Header->VirtualSize, 0U) << Names.back();
    EXPECT_GE(Header->VirtualAddress, PreviousEnd) << Names.back();
    PreviousEnd =
        static_cast<uint64_t>(Header->VirtualAddress) + Header->VirtualSize;
  }
  EXPECT_EQ(Names, (std::vector<std::string>{".text", ".bss", ".edata"}));
}

TEST(PEWriterTest, KeepsHeaderSizeWhenLeadingSectionIsOmitted) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("leading.obj"));
  ASSERT_TRUE(
      Graph.addSection(Section{".text", SectionKind::Text, 16, 0, 0, 0, {}}));
  ASSERT_TRUE(
      Graph.addSection(Section{".bss", SectionKind::BSS, 8, 8, 0, 0, {}}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "leading.dll", Output));
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "leading.dll"));
  ASSERT_TRUE(static_cast<bool>(Image)) << llvm::toString(Image.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  const auto *Header = pe32PlusHeader(*PE);
  ASSERT_NE(Header, nullptr);
  EXPECT_EQ(Header->SizeOfHeaders, Graph.sections().front().FileOffset);
}

} // namespace
