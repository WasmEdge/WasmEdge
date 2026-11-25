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
#include "linker/elf_writer.h"
#include "linker/layout.h"
#include "linker/link_graph.h"
#include "linker/object_reader.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"

#include <gtest/gtest.h>

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Config/llvm-config.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

using namespace WasmEdge::LLVM::Linker;
using namespace WasmEdge::LLVM::Test;

struct FailingRelocationCase {
  const char *Triple;
  Target Architecture;
  Endianness WrongEndian;
  uint32_t AbsoluteType;
  uint8_t Width;
  const char *Features;
};

struct PortableRelocationCase {
  const char *Triple;
  Target Architecture;
  ObjectFormat Format;
  uint32_t Type;
};

[[maybe_unused]] std::string failingRelocationCaseName(
    const testing::TestParamInfo<FailingRelocationCase> &Info) {
  return architectureName(Info.param.Architecture);
}

[[maybe_unused]] std::string portableRelocationCaseName(
    const testing::TestParamInfo<PortableRelocationCase> &Info) {
  return architectureName(Info.param.Architecture);
}

std::vector<WasmEdge::Byte> makeX86_64AssemblyObject(std::string Assembly) {
  return makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                            std::move(Assembly));
}

TEST(RelocationFieldTest, ReadsAndWritesUnsignedBoundariesWithExactBytes) {
  struct Case {
    uint8_t Width;
    uint64_t Maximum;
  };
  const std::array<Case, 4> Cases{
      {{1, UINT8_MAX}, {2, UINT16_MAX}, {4, UINT32_MAX}, {8, UINT64_MAX}}};
  for (const auto Endian : {Endianness::Little, Endianness::Big}) {
    for (const auto &Test : Cases) {
      for (const uint64_t Value : {UINT64_C(0), Test.Maximum}) {
        std::array<WasmEdge::Byte, 10> Bytes{};
        ASSERT_TRUE(
            Internal::writeUnsigned(Bytes, 1, Test.Width, Endian, Value));
        for (uint8_t I = 0; I < Test.Width; ++I) {
          EXPECT_EQ(Bytes[1 + I], Value == 0 ? 0 : 0xFF);
        }
        auto Read = Internal::readUnsigned(Bytes, 1, Test.Width, Endian);
        ASSERT_TRUE(Read);
        EXPECT_EQ(*Read, Value);
      }
    }
  }
  std::array<WasmEdge::Byte, 4> Bytes{};
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x12345678)));
  EXPECT_EQ(Bytes, (std::array<WasmEdge::Byte, 4>{0x78, 0x56, 0x34, 0x12}));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Big,
                                      UINT32_C(0x12345678)));
  EXPECT_EQ(Bytes, (std::array<WasmEdge::Byte, 4>{0x12, 0x34, 0x56, 0x78}));
}

TEST(RelocationFieldTest, ReadsAndWritesSignedBoundariesInEitherEndianness) {
  struct Case {
    uint8_t Width;
    int64_t Minimum;
    int64_t Maximum;
  };
  const std::array<Case, 4> Cases{{{1, INT8_MIN, INT8_MAX},
                                   {2, INT16_MIN, INT16_MAX},
                                   {4, INT32_MIN, INT32_MAX},
                                   {8, INT64_MIN, INT64_MAX}}};
  for (const auto &Test : Cases) {
    for (const auto Endian : {Endianness::Little, Endianness::Big}) {
      for (const auto Value : {Test.Minimum, Test.Maximum}) {
        std::array<WasmEdge::Byte, 10> Bytes{};
        ASSERT_TRUE(Internal::writeSigned(Bytes, 1, Test.Width, Endian, Value));
        for (uint8_t I = 0; I < Test.Width; ++I) {
          const bool SignByte =
              Endian == Endianness::Little ? I == Test.Width - 1 : I == 0;
          const uint8_t Expected = Value == Test.Minimum
                                       ? SignByte ? 0x80 : 0x00
                                   : SignByte ? 0x7F
                                              : 0xFF;
          EXPECT_EQ(Bytes[1 + I], Expected);
        }
        auto Read = Internal::readSigned(Bytes, 1, Test.Width, Endian);
        ASSERT_TRUE(Read);
        EXPECT_EQ(*Read, Value);
      }
    }
  }
}

TEST(RelocationFieldTest, AllowsMisalignmentAndRejectsBoundsAndOverflow) {
  std::array<WasmEdge::Byte, 8> Bytes{};
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 1, 4, Endianness::Little,
                                      UINT32_C(0x12345678)));
  EXPECT_EQ(*Internal::readUnsigned(Bytes, 1, 4, Endianness::Little),
            UINT32_C(0x12345678));
  EXPECT_FALSE(Internal::readUnsigned(Bytes, 0, 3, Endianness::Little));
  EXPECT_FALSE(
      Internal::readUnsigned(Bytes, UINT64_MAX, 1, Endianness::Little));
  for (const uint8_t Width : std::array<uint8_t, 4>{1, 2, 4, 8}) {
    const uint64_t Offset = Bytes.size() - Width + 1;
    EXPECT_FALSE(
        Internal::readUnsigned(Bytes, Offset, Width, Endianness::Little));
    EXPECT_FALSE(Internal::readSigned(Bytes, Offset, Width, Endianness::Big));
    EXPECT_FALSE(
        Internal::writeUnsigned(Bytes, Offset, Width, Endianness::Little, 0));
    EXPECT_FALSE(
        Internal::writeSigned(Bytes, Offset, Width, Endianness::Big, 0));
  }
  EXPECT_FALSE(Internal::writeUnsigned(Bytes, 0, 1, Endianness::Little, 256));
  EXPECT_FALSE(Internal::writeSigned(Bytes, 0, 1, Endianness::Little, 128));
  EXPECT_FALSE(Internal::writeSigned(Bytes, 0, 1, Endianness::Little, -129));
  for (const uint8_t Width : std::array<uint8_t, 3>{1, 2, 4}) {
    const uint8_t Bits =
        static_cast<uint8_t>(static_cast<uint32_t>(Width) * 8U);
    EXPECT_FALSE(Internal::writeUnsigned(Bytes, 0, Width, Endianness::Little,
                                         UINT64_C(1) << Bits));
    const int64_t Maximum = (INT64_C(1) << (Bits - 1)) - 1;
    const int64_t Minimum = -(INT64_C(1) << (Bits - 1));
    EXPECT_FALSE(
        Internal::writeSigned(Bytes, 0, Width, Endianness::Big, Maximum + 1));
    EXPECT_FALSE(
        Internal::writeSigned(Bytes, 0, Width, Endianness::Big, Minimum - 1));
  }
}

LinkGraph makeRelocationGraph(
    uint32_t Type, int64_t Addend, bool Implicit = false,
    uint64_t TargetAddress = 0x1100, uint64_t PatchAddress = 0x1000,
    ObjectFormat Format = ObjectFormat::ELF,
    std::vector<WasmEdge::Byte> Bytes = std::vector<WasmEdge::Byte>(24)) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  EXPECT_TRUE(Graph.beginInput("input.o"));
  auto Patch =
      Graph.addSection(Section{".text", SectionKind::Text, 1, Bytes.size(),
                               PatchAddress, 0, std::move(Bytes)});
  auto TargetSection = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, TargetAddress, 0, {0}});
  EXPECT_TRUE(Patch && TargetSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *TargetSection, 0, 1, false});
  EXPECT_TRUE(TargetSymbol);
  Relocation Value{*Patch, 1, Type, *TargetSymbol, Addend, Implicit, Format};
  Value.PatchSize = Type == 1 ? 8 : 4;
  EXPECT_TRUE(Graph.addRelocation(Value));
  return Graph;
}

TEST(RelocationTest, AppliesX86_64AbsoluteAndRecordsOneRebase) {
  auto Graph = makeRelocationGraph(1, 5);
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 8,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x1105U);
  ASSERT_EQ(Graph.rebases().size(), 1U);
  EXPECT_EQ(Graph.rebases()[0].Section, 0U);
  EXPECT_EQ(Graph.rebases()[0].Offset, 1U);
  EXPECT_EQ(Graph.rebases()[0].Width, 8U);
  EXPECT_EQ(Graph.rebases()[0].Type, 1U);
  EXPECT_EQ(Graph.rebases()[0].Format, ObjectFormat::ELF);
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsBigEndianX86_64WithoutMutation) {
  LinkGraph Graph(Target::X86_64, Endianness::Big);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto DataSection = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 8, 0x1000, 0, {1, 2, 3, 4, 5, 6, 7, 8}});
  ASSERT_TRUE(DataSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *DataSection, 0, 8, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph.addRelocation(Relocation{*DataSection, 0, 1, *TargetSymbol,
                                             0, false, ObjectFormat::ELF, 8}));
  const auto Content = Graph.sections()[0].Content;
  EXPECT_FALSE(applyRelocations(Graph));
  EXPECT_EQ(Graph.sections()[0].Content, Content);
  EXPECT_TRUE(Graph.rebases().empty());
  EXPECT_FALSE(Graph.relocationsApplied());
}

TEST(RelocationTest, ChecksX86_64AbsoluteOverflowAndUnderflow) {
  for (const auto &[Address, Addend] :
       std::array<std::pair<uint64_t, int64_t>, 2>{
           {{0, -1}, {UINT64_MAX, 1}}}) {
    auto Graph = makeRelocationGraph(1, Addend, false, Address);
    const auto Content = Graph.sections()[0].Content;
    EXPECT_FALSE(applyRelocations(Graph));
    EXPECT_EQ(Graph.sections()[0].Content, Content);
    EXPECT_TRUE(Graph.rebases().empty());
  }
}

TEST(RelocationTest, ChecksPC32AndPLT32SignedBoundaries) {
  struct Case {
    int64_t Displacement;
    bool Accepted;
  };
  const std::array<Case, 6> Cases{{{INT32_MIN, true},
                                   {INT32_MAX, true},
                                   {static_cast<int64_t>(INT32_MIN) - 1, false},
                                   {static_cast<int64_t>(INT32_MAX) + 1, false},
                                   {-256, true},
                                   {256, true}}};
  constexpr uint64_t PatchAddress = UINT64_C(0x100000000);
  constexpr uint64_t Place = PatchAddress + 1;
  for (const uint32_t Type : {2U, 4U}) {
    for (const auto &Test : Cases) {
      const uint64_t Target =
          Test.Displacement < 0
              ? Place - static_cast<uint64_t>(-Test.Displacement)
              : Place + static_cast<uint64_t>(Test.Displacement);
      auto Graph = makeRelocationGraph(Type, 0, false, Target, PatchAddress);
      auto Result = applyRelocations(Graph);
      EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
      if (Test.Accepted) {
        auto Value = Internal::readSigned(Graph.sections()[0].Content, 1, 4,
                                          Endianness::Little);
        ASSERT_TRUE(Value);
        EXPECT_EQ(*Value, Test.Displacement);
      }
    }
  }
}

TEST(RelocationTest, AppliesExplicitAndImplicitPC32AndPLT32) {
  struct Case {
    uint32_t Type;
    int64_t Addend;
    bool Implicit;
    uint64_t Target;
    int32_t Expected;
  };
  const std::array<Case, 4> Cases{{{2, -4, false, 0x1100, 0xFB},
                                   {4, -4, false, 0x0F00, -0x105},
                                   {2, 0, true, 0x1100, 0xFB},
                                   {4, 0, true, 0x0F00, -0x105}}};
  for (const auto &Test : Cases) {
    auto Graph = [&] {
      std::vector<WasmEdge::Byte> Bytes(24);
      if (Test.Implicit) {
        EXPECT_TRUE(Internal::writeSigned(
            WasmEdge::Span<WasmEdge::Byte>(Bytes).subspan(1), 0, 4,
            Endianness::Little, -4));
      }
      return makeRelocationGraph(Test.Type, Test.Addend, Test.Implicit,
                                 Test.Target, 0x1000, ObjectFormat::ELF,
                                 std::move(Bytes));
    }();
    ASSERT_TRUE(applyRelocations(Graph));
    auto Value = Internal::readSigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, Test.Expected);
  }
}

TEST(RelocationTest, AppliesMachOSignedSuffixBiasExactly) {
  struct Case {
    uint32_t Type;
    int64_t Suffix;
  };
  const std::array<Case, 4> Cases{{
      {llvm::MachO::X86_64_RELOC_SIGNED, 0},
      {llvm::MachO::X86_64_RELOC_SIGNED_1, 1},
      {llvm::MachO::X86_64_RELOC_SIGNED_2, 2},
      {llvm::MachO::X86_64_RELOC_SIGNED_4, 4},
  }};
  for (const auto &Test : Cases) {
    for (const uint64_t Target : {UINT64_C(0x1100), UINT64_C(0x0F00)}) {
      LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
      ASSERT_TRUE(Graph.beginInput("signed.o"));
      auto Patch = Graph.addSection(
          Section{"__text", SectionKind::Text, 1, 4, 0x1000, 0, {0, 0, 0, 0}});
      auto TargetSection = Graph.addSection(
          Section{"__target", SectionKind::Text, 1, 1, Target, 0, {0}});
      ASSERT_TRUE(Patch && TargetSection);
      auto TargetSymbol =
          Graph.addSymbol(Symbol{"_target", *TargetSection, 0, 1, false});
      ASSERT_TRUE(TargetSymbol);
      ASSERT_TRUE(
          Graph.addRelocation(Relocation{*Patch, 0, Test.Type, *TargetSymbol, 0,
                                         true, ObjectFormat::MachO, 4}));
      ASSERT_TRUE(applyRelocations(Graph));
      auto Value = Internal::readSigned(Graph.sections()[*Patch].Content, 0, 4,
                                        Endianness::Little);
      ASSERT_TRUE(Value);
      EXPECT_EQ(*Value, static_cast<int64_t>(Target) - 0x1000 - 4)
          << Test.Suffix;

      LinkGraph Explicit(Target::X86_64, Endianness::Little,
                         ObjectFormat::MachO);
      ASSERT_TRUE(Explicit.beginInput("explicit-signed.o"));
      auto ExplicitPatch = Explicit.addSection(
          Section{"__text", SectionKind::Text, 1, 4, 0x1000, 0, {0, 0, 0, 0}});
      auto ExplicitTarget = Explicit.addSection(
          Section{"__target", SectionKind::Text, 1, 1, Target, 0, {0}});
      ASSERT_TRUE(ExplicitPatch && ExplicitTarget);
      auto ExplicitSymbol =
          Explicit.addSymbol(Symbol{"_target", *ExplicitTarget, 0, 1, false});
      ASSERT_TRUE(ExplicitSymbol);
      ASSERT_TRUE(Explicit.addRelocation(
          Relocation{*ExplicitPatch, 0, Test.Type, *ExplicitSymbol, 7, false,
                     ObjectFormat::MachO, 4}));
      ASSERT_TRUE(applyRelocations(Explicit));
      auto ExplicitValue =
          Internal::readSigned(Explicit.sections()[*ExplicitPatch].Content, 0,
                               4, Endianness::Little);
      ASSERT_TRUE(ExplicitValue);
      EXPECT_EQ(*ExplicitValue,
                static_cast<int64_t>(Target) + 7 - 0x1000 - 4 - Test.Suffix);
    }
  }
}

TEST(RelocationTest, RelocatesGeneratedMachOSignedSuffixExactly) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Original = makeObject(llvm::Triple("x86_64-apple-macosx"));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Original.data()),
                          Original.size()),
          "signed.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  size_t RelocationOffset = 0;
  for (const auto &Section : MachO->sections()) {
    if (Section.relocation_begin() == Section.relocation_end())
      continue;
    const auto Ref = Section.relocation_begin()->getRawDataRefImpl();
    llvm::object::DataRefImpl SectionRef;
    SectionRef.d.a = Ref.d.a;
    RelocationOffset = MachO->getSection64(SectionRef).reloff + Ref.d.b * 8;
    break;
  }
  ASSERT_NE(RelocationOffset, 0U);
  for (const uint32_t Type :
       {llvm::MachO::X86_64_RELOC_SIGNED_1, llvm::MachO::X86_64_RELOC_SIGNED_2,
        llvm::MachO::X86_64_RELOC_SIGNED_4}) {
    auto Object = Original;
    const uint32_t Word = read32le(Object, RelocationOffset + 4);
    write32le(Object, RelocationOffset + 4,
              (Word & UINT32_C(0x0FFFFFFF)) | (Type << 28));
    auto Graph = ObjectReader::read(Object, Target::X86_64);
    ASSERT_TRUE(Graph) << Type;
    ASSERT_EQ(Graph->relocations().size(), 1U);
    EXPECT_EQ(Graph->relocations()[0].Type, Type);
    ASSERT_TRUE(layout(*Graph, 0, 4096));
    const auto Relocation = Graph->relocations()[0];
    ASSERT_TRUE(applyRelocations(*Graph));
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    const uint64_t S =
        Graph->sections()[Symbol.Section].Address + Symbol.Offset;
    const uint64_t P =
        Graph->sections()[Relocation.Section].Address + Relocation.Offset;
    auto Value =
        Internal::readSigned(Graph->sections()[Relocation.Section].Content,
                             Relocation.Offset, 4, Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, static_cast<int64_t>(S) - static_cast<int64_t>(P) - 4);
  }
}

TEST(RelocationTest, DecodesImplicitAddendForX86_64Absolute) {
  std::vector<WasmEdge::Byte> Bytes(24);
  ASSERT_TRUE(
      Internal::writeSigned(WasmEdge::Span<WasmEdge::Byte>(Bytes).subspan(1), 0,
                            8, Endianness::Little, -5));
  auto Graph = makeRelocationGraph(1, 0, true, 0x1100, 0x1000,
                                   ObjectFormat::ELF, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 8,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x10FBU);
}

TEST(RelocationTest, RejectsOverflowAndDoesNotPartiallyModifyGraph) {
  auto Graph = makeRelocationGraph(1, 5);
  auto FarSection = Graph.addSection(Section{
      ".far", SectionKind::Data, 1, 1, UINT64_C(0x8000000000000001), 0, {0}});
  ASSERT_TRUE(FarSection);
  auto SecondSymbol =
      Graph.addSymbol(Symbol{"second", *FarSection, 0, 1, false});
  ASSERT_TRUE(SecondSymbol);
  ASSERT_TRUE(Graph.addRelocation(Relocation{0, 12, 1, *SecondSymbol, INT64_MAX,
                                             false, ObjectFormat::ELF, 8}));
  const auto Snapshot = Graph;
  auto Result = applyRelocations(Graph);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("overflows"), std::string::npos);
  EXPECT_TRUE(Result.error().Section);
  EXPECT_TRUE(Result.error().Symbol);
  EXPECT_TRUE(Result.error().RelocationType);
  EXPECT_TRUE(Result.error().Offset);
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsGeneratedRebaseOverlappingExistingRebase) {
  auto Graph = makeRelocationGraph(1, 5);
  ASSERT_TRUE(Graph.addRebase(Rebase{0, 4, 1, 0, 8}));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsInvalidGraphUnsupportedTargetTypeAndFormat) {
  auto InvalidSection = makeRelocationGraph(2, -4);
  const_cast<std::vector<Relocation> &>(InvalidSection.relocations())[0]
      .Section = InvalidSectionId;
  EXPECT_FALSE(applyRelocations(InvalidSection));

  auto InvalidSymbol = makeRelocationGraph(2, -4);
  const_cast<std::vector<Relocation> &>(InvalidSymbol.relocations())[0].Symbol =
      InvalidSymbolId;
  EXPECT_FALSE(applyRelocations(InvalidSymbol));

  auto UnsupportedType = makeRelocationGraph(2, 0);
  auto &UnsupportedTypeValue =
      const_cast<std::vector<Relocation> &>(UnsupportedType.relocations())[0];
  UnsupportedTypeValue.Type = 0xFFFF;
  auto UnsupportedTypeResult = applyRelocations(UnsupportedType);
  ASSERT_FALSE(UnsupportedTypeResult);
  EXPECT_EQ(UnsupportedTypeResult.error().Kind, DiagnosticKind::Malformed);
  auto UnsupportedFormat = makeRelocationGraph(2, -4);
  const_cast<std::vector<Relocation> &>(UnsupportedFormat.relocations())[0]
      .Format = ObjectFormat::COFF;
  EXPECT_FALSE(applyRelocations(UnsupportedFormat));
}

TEST(RelocationTest, RelaxesX86_64RexGotpcrelxForDefinedSymbol) {
  std::vector<WasmEdge::Byte> Content(24);
  Content[0] = 0x48;
  Content[1] = 0x8B;
  Content[2] = 0x05;
  auto Graph = makeRelocationGraph(42, -4, false, 0x1100, 0x1000,
                                   ObjectFormat::ELF, std::move(Content));
  auto &RelocationValue =
      const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
  RelocationValue.PatchSize = 4;
  RelocationValue.Offset = 3;
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(Graph.sections()[0].Content[1], 0x8DU);
  auto Value = Internal::readSigned(Graph.sections()[0].Content, 3, 4,
                                    Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0xF9);
}

TEST(RelocationTest, ValidatesGotpcrelxInstructionAndAddend) {
  struct Case {
    uint32_t Type;
    int64_t Addend;
    std::array<WasmEdge::Byte, 3> Prefix;
    uint64_t Offset;
    bool Accepted;
  };
  const std::array<Case, 7> Cases{{
      {41, -4, {0, 0x8B, 0x05}, 3, true},
      {42, -4, {0x48, 0x8B, 0x05}, 3, true},
      {42, 0, {0x48, 0x8B, 0x05}, 3, false},
      {42, -4, {0x48, 0x89, 0x05}, 3, false},
      {42, -4, {0x48, 0x8B, 0x04}, 3, false},
      {42, -4, {0x90, 0x8B, 0x05}, 3, false},
      {41, -4, {0x8B, 0x05, 0x90}, 2, true},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(24);
    std::copy(Test.Prefix.begin(), Test.Prefix.end(), Bytes.begin());
    auto Graph =
        makeRelocationGraph(Test.Type, Test.Addend, false, 0x1100, 0x1000,
                            ObjectFormat::ELF, std::move(Bytes));
    auto &RelocationValue =
        const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
    RelocationValue.Offset = Test.Offset;
    RelocationValue.PatchSize = 4;
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
  }
}

TEST(RelocationTest, RelaxesExactGotpcrelxIndirectCallAndJump) {
  struct Case {
    WasmEdge::Byte ModRM;
    std::array<WasmEdge::Byte, 6> ExpectedPrefixAndPatch;
  };
  const std::array<Case, 2> Cases{{
      {0x15, {0x67, 0xE8, 0xFA, 0x00, 0x00, 0x00}},
      {0x25, {0xE9, 0xFB, 0x00, 0x00, 0x00, 0x90}},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(24);
    Bytes[0] = 0xFF;
    Bytes[1] = Test.ModRM;
    auto Graph = makeRelocationGraph(41, -4, false, 0x1100, 0x1000,
                                     ObjectFormat::ELF, std::move(Bytes));
    auto &RelocationValue =
        const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
    RelocationValue.Offset = 2;
    RelocationValue.PatchSize = 4;
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_TRUE(std::equal(Test.ExpectedPrefixAndPatch.begin(),
                           Test.ExpectedPrefixAndPatch.end(),
                           Graph.sections()[0].Content.begin()));
  }
}

TEST(RelocationTest, RelaxesGotpcrelxIndirectBranchesAboveFourGiB) {
  struct Case {
    WasmEdge::Byte ModRM;
    std::array<WasmEdge::Byte, 6> Expected;
  };
  const std::array<Case, 2> Cases{{
      {0x15, {0x67, 0xE8, 0xFA, 0x00, 0x00, 0x00}},
      {0x25, {0xE9, 0xFB, 0x00, 0x00, 0x00, 0x90}},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(24);
    Bytes[0] = 0xFF;
    Bytes[1] = Test.ModRM;
    auto Graph = makeRelocationGraph(41, -4, false, UINT64_C(0x100000100),
                                     UINT64_C(0x100000000), ObjectFormat::ELF,
                                     std::move(Bytes));
    auto &RelocationValue =
        const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
    RelocationValue.Offset = 2;
    RelocationValue.PatchSize = 4;
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_TRUE(std::equal(Test.Expected.begin(), Test.Expected.end(),
                           Graph.sections()[0].Content.begin()));
  }
}

TEST(RelocationTest, RejectsGotpcrelxIndirectBranchDisplacementOverflow) {
  std::vector<WasmEdge::Byte> Bytes(24);
  Bytes[0] = 0xFF;
  Bytes[1] = 0x15;
  auto Graph = makeRelocationGraph(41, -4, false, UINT64_C(0x80001007), 0x1000,
                                   ObjectFormat::ELF, std::move(Bytes));
  auto &RelocationValue =
      const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
  RelocationValue.Offset = 2;
  RelocationValue.PatchSize = 4;
  EXPECT_FALSE(applyRelocations(Graph));
}

TEST(RelocationTest, RejectsMutationAndLayoutAfterRelocation) {
  auto Graph = makeRelocationGraph(2, -4);
  auto EHFrame = Graph.addSection(
      Section{".eh_frame", SectionKind::Unwind, 8, 8, 0, 0,
              std::vector<WasmEdge::Byte>(8), SectionPurpose::EHFrame});
  ASSERT_TRUE(EHFrame);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto ExpectRelocated = [](const auto &Result) {
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "link graph relocations already applied");
  };
  ExpectRelocated(Graph.beginInput("again.o"));
  ExpectRelocated(Graph.addSection(Section{"new", SectionKind::Data, 1, 0}));
  ExpectRelocated(Graph.addSymbol(Symbol{"new", 0, 0, 0, false}));
  ExpectRelocated(Graph.addRelocation(Relocation{0, 8, 2, 0, -4}));
  ExpectRelocated(Graph.addRebase(Rebase{0, 8, 1, 0, 8}));
  const auto EHFrameReferences = Graph.ehFrameReferences();
  ExpectRelocated(Graph.addEHFrameReference(EHFrameReference{*EHFrame, 0, 0}));
  EXPECT_EQ(Graph.ehFrameReferences().size(), EHFrameReferences.size());
  ExpectRelocated(Graph.setSectionAddress(0, 3));
  ExpectRelocated(Graph.setSectionFileOffset(0, 3));
  auto LayoutResult = layout(Graph);
  ASSERT_FALSE(LayoutResult);
  EXPECT_EQ(LayoutResult.error().Message, "cannot layout relocated link graph");
}

TEST(RelocationTest, ReadsLayoutsAndRelocatesX86_64ObjectEndToEnd) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Object = makeX86_64ELFReaderFixture();
  ASSERT_EQ(Object.size(), 848U);
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph) << Graph.error().Message;
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto Text =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const Section &Value) { return Value.Name == ".text"; });
  ASSERT_NE(Text, Graph->sections().end());
  ASSERT_GE(Text->Content.size(), 7U);
  EXPECT_EQ(Text->Content[1], 0x8DU);
  auto Displacement =
      Internal::readSigned(Text->Content, 3, 4, Endianness::Little);
  ASSERT_TRUE(Displacement);
  const auto &Relocation = Graph->relocations()[0];
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  EXPECT_EQ(*Displacement,
            static_cast<int64_t>(Graph->sections()[Symbol.Section].Address +
                                 Symbol.Offset) -
                static_cast<int64_t>(Text->Address + Relocation.Offset) - 4);
}

LinkGraph makeELFRelocationGraph(
    Target Architecture, Endianness Endian, uint32_t Type, uint8_t Width,
    uint64_t TargetAddress, uint64_t PatchAddress = 0x1000, uint64_t Offset = 0,
    int64_t Addend = 0, bool Implicit = false,
    std::vector<WasmEdge::Byte> Bytes = std::vector<WasmEdge::Byte>(16),
    ObjectFormat Format = ObjectFormat::ELF) {
  LinkGraph Graph(Architecture, Endian, Format);
  EXPECT_TRUE(Graph.beginInput("input.o"));
  auto Patch =
      Graph.addSection(Section{".text", SectionKind::Text, 4, Bytes.size(),
                               PatchAddress, 0, std::move(Bytes)});
  auto TargetSection = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, TargetAddress, 0, {0}});
  EXPECT_TRUE(Patch && TargetSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *TargetSection, 0, 1, false});
  EXPECT_TRUE(TargetSymbol);
  EXPECT_TRUE(Graph.addRelocation(Relocation{
      *Patch, Offset, Type, *TargetSymbol, Addend, Implicit, Format, Width}));
  return Graph;
}

void setTargetThumb(LinkGraph &Graph, bool Thumb = true) {
  ASSERT_FALSE(Graph.symbols().empty());
  const_cast<Symbol &>(Graph.symbols().back()).Thumb = Thumb;
}

TEST(ARMRelocationTest, AppliesDataRelocationsAndPreservesPrel31TopBit) {
  auto Absolute = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, 2, 4, 0x2000, 0x1000, 0, 0, true, [] {
        std::vector<WasmEdge::Byte> Bytes(16);
        EXPECT_TRUE(Internal::writeSigned(Bytes, 0, 4, Endianness::Little, -4));
        return Bytes;
      }());
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            0x1FFCU);
  ASSERT_EQ(Absolute.rebases().size(), 1U);
  EXPECT_EQ(Absolute.rebases()[0].Width, 4U);

  auto Relative = makeELFRelocationGraph(Target::ARM, Endianness::Little, 3, 4,
                                         0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(Relative));
  EXPECT_EQ(*Internal::readSigned(Relative.sections()[0].Content, 0, 4,
                                  Endianness::Little),
            -0xFC);

  std::vector<WasmEdge::Byte> PrelBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(PrelBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x80000004)));
  auto Prel =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 42, 4, 0x1100,
                             0x1000, 0, 0, true, std::move(PrelBytes));
  ASSERT_TRUE(applyRelocations(Prel));
  EXPECT_EQ(*Internal::readUnsigned(Prel.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x80000104));
}

TEST(ARMRelocationTest, AppliesThumbStateToDataAddresses) {
  for (const auto Type : {llvm::ELF::R_ARM_ABS32, llvm::ELF::R_ARM_REL32,
                          llvm::ELF::R_ARM_PREL31}) {
    auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little, Type,
                                        4, 0x2000, 0x1000);
    setTargetThumb(Graph);
    ASSERT_TRUE(applyRelocations(Graph));
    const auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                              Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, Type == llvm::ELF::R_ARM_ABS32 ? 0x2001U : 0x1001U);
  }
}

TEST(ARMRelocationTest, ConvertsCrossStateCallsAndRejectsJump) {
  std::vector<WasmEdge::Byte> ArmBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(ArmBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xEB000000)));
  auto ArmCall = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                        llvm::ELF::R_ARM_CALL, 4, 0x1004,
                                        0x1000, 0, 0, false, ArmBytes);
  setTargetThumb(ArmCall);
  ASSERT_TRUE(applyRelocations(ArmCall));
  EXPECT_EQ(*Internal::readUnsigned(ArmCall.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xFA000001));

  auto ArmJump = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_JUMP24, 4, 0x1004,
      0x1000, 0, 0, false, std::move(ArmBytes));
  setTargetThumb(ArmJump);
  const auto JumpSnapshot = ArmJump;
  auto JumpResult = applyRelocations(ArmJump);
  ASSERT_FALSE(JumpResult);
  EXPECT_EQ(JumpResult.error().Kind, DiagnosticKind::Unsupported);
  expectGraphStateEquals(ArmJump, JumpSnapshot);

  std::vector<WasmEdge::Byte> ThumbBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(ThumbBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xF800F000)));
  auto ThumbCall = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
      0x1000, 0, 0, false, std::move(ThumbBytes));
  ASSERT_TRUE(applyRelocations(ThumbCall));
  EXPECT_EQ(*Internal::readUnsigned(ThumbCall.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xE802F000));

  std::vector<WasmEdge::Byte> MisalignedBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(MisalignedBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xF800F000)));
  auto Misaligned = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4, 0x1002,
      0x1000, 0, 0, false, std::move(MisalignedBytes));
  EXPECT_FALSE(applyRelocations(Misaligned));
}

TEST(ARMRelocationTest, EncodesNegativeCrossStateHalfwordOffset) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xEB000000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_CALL, 4, 0x0FFA, 0x1000,
                                      0, 0, false, std::move(Bytes));
  setTargetThumb(Graph);
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xFBFFFFFE));
}

TEST(ARMRelocationTest, NormalizesArmBlxToBlForArmTarget) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFA000000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_CALL, 4, 0x1004, 0x1000,
                                      0, 0, true, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xEB000001));
}

TEST(ARMRelocationTest, DecodesArmBlxImplicitHBit) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFB000000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_CALL, 4, 0x1006, 0x1000,
                                      0, 0, true, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xEB000002));
}

TEST(ARMRelocationTest, NormalizesThumbBlxToBlForThumbTarget) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xE800F000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
                                      0x1000, 0, 0, true, std::move(Bytes));
  setTargetThumb(Graph);
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xF802F000));
}

TEST(ARMRelocationTest, EncodesArmCallAndChecksRangeAlignmentAndEndianness) {
  for (const auto &[Delta, Accepted] :
       std::array<std::pair<int64_t, bool>, 6>{{{4, true},
                                                {-4, true},
                                                {33554428, true},
                                                {-33554432, true},
                                                {2, false},
                                                {33554432, false}}}) {
    const uint64_t Patch = UINT64_C(0x4000000);
    const uint64_t Target = Delta < 0 ? Patch - static_cast<uint64_t>(-Delta)
                                      : Patch + static_cast<uint64_t>(Delta);
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0xEB000000)));
    auto Graph =
        makeELFRelocationGraph(Target::ARM, Endianness::Little, 28, 4, Target,
                               Patch, 0, 0, false, std::move(Bytes));
    const auto Snapshot = Graph;
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Accepted);
    if (!Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
    }
  }

  auto Big = makeELFRelocationGraph(Target::ARM, Endianness::Big, 3, 4, 0x1100);
  const auto Snapshot = Big;
  EXPECT_FALSE(applyRelocations(Big));
  expectGraphStateEquals(Big, Snapshot);
}

TEST(ARMRelocationTest, DecodesImplicitBranchAddendFromImm24) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xEBFFFFFE)));
  auto Graph =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 28, 4, 0x1004,
                             0x1000, 0, 0, true, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xEBFFFFFF));
}

TEST(ARMRelocationTest, EncodesThumbCallBoundariesAndImplicitAddend) {
  constexpr uint32_t ThumbBl = UINT32_C(0xF800F000);
  constexpr uint64_t PatchAddress = UINT64_C(0x2000000);
  struct Case {
    int64_t Delta;
    bool Accepted;
  };
  const std::array<Case, 6> Cases{{
      {-INT64_C(16777216), true},
      {INT64_C(16777214), true},
      {-INT64_C(16777218), false},
      {INT64_C(16777216), false},
      {-2, true},
      {1, false},
  }};
  for (const auto &Test : Cases) {
    const uint64_t TargetAddress =
        Test.Delta < 0 ? PatchAddress - static_cast<uint64_t>(-Test.Delta)
                       : PatchAddress + static_cast<uint64_t>(Test.Delta);
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(
        Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little, ThumbBl));
    auto Graph = makeELFRelocationGraph(
        Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4,
        TargetAddress, PatchAddress, 0, 0, false, std::move(Bytes));
    setTargetThumb(Graph);
    const auto Snapshot = Graph;
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
    }
  }

  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFFFFF7FF)));
  auto Implicit = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                         llvm::ELF::R_ARM_THM_CALL, 4, 0x1002,
                                         0x1000, 0, 0, true, std::move(Bytes));
  setTargetThumb(Implicit);
  ASSERT_TRUE(applyRelocations(Implicit));
  EXPECT_EQ(*Internal::readUnsigned(Implicit.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xF800F000));
}

TEST(ARMRelocationTest, RejectsMalformedThumbCallInstructionAtomically) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x8000F000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
                                      0x1000, 0, 0, false, std::move(Bytes));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);

  std::vector<WasmEdge::Byte> MisalignedBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(MisalignedBytes, 1, 4, Endianness::Little,
                                      UINT32_C(0xF800F000)));
  auto Misaligned = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
      0x1000, 1, 0, false, std::move(MisalignedBytes));
  const auto MisalignedSnapshot = Misaligned;
  EXPECT_FALSE(applyRelocations(Misaligned));
  expectGraphStateEquals(Misaligned, MisalignedSnapshot);
}

TEST(ARMRelocationTest, ReadsGeneratedThumbAndCantUnwindObject) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabihf"),
                         R"(.syntax unified
.thumb
.section .text,"ax",%progbits
.globl caller
.thumb_func
.type caller,%function
caller:
.fnstart
  bl target
  bx lr
.cantunwind
.fnend
.section .text.target,"ax",%progbits
.thumb_func
.type target,%function
target:
  bx lr
)",
                         "+thumb-mode");
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(ObjectBytes.data()),
                          ObjectBytes.size()),
          "thumb.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  std::set<uint64_t> Types;
  for (const auto &Section : (*Object)->sections()) {
    for (const auto &Relocation : Section.relocations()) {
      Types.insert(Relocation.getType());
    }
  }
  EXPECT_EQ(Types, (std::set<uint64_t>{llvm::ELF::R_ARM_THM_CALL,
                                       llvm::ELF::R_ARM_PREL31}));
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  const auto TargetSymbol =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Value) { return Value.Name == "target"; });
  ASSERT_NE(TargetSymbol, Graph->symbols().end());
  EXPECT_TRUE(TargetSymbol->Thumb);
  EXPECT_EQ(TargetSymbol->Offset & 1, 0U);
  const auto Exidx = std::find_if(
      Graph->sections().begin(), Graph->sections().end(),
      [](const auto &Section) { return Section.Name == ".ARM.exidx"; });
  ASSERT_NE(Exidx, Graph->sections().end());
  EXPECT_EQ(Exidx->Kind, SectionKind::Unwind);
  EXPECT_FALSE(Exidx->Content.empty());
  ASSERT_TRUE(Exidx->LinkedSection);
  EXPECT_EQ(Graph->sections()[*Exidx->LinkedSection].Name, ".text");
  const auto ExidxId =
      static_cast<SectionId>(Exidx - Graph->sections().begin());
  EXPECT_TRUE(std::any_of(Graph->relocations().begin(),
                          Graph->relocations().end(), [&](const auto &Value) {
                            return Value.Section == ExidxId &&
                                   Value.Type == llvm::ELF::R_ARM_PREL31;
                          }));
}

TEST(ARMRelocationTest, RelocatesGeneratedThumbAndCantUnwindObject) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabihf"),
                         R"(.syntax unified
.thumb
.section .text,"ax",%progbits
.globl caller
.thumb_func
.type caller,%function
caller:
.fnstart
  bl target
  bx lr
.cantunwind
.fnend
.section .text.target,"ax",%progbits
.thumb_func
.type target,%function
target:
  bx lr
)",
                         "+thumb-mode");
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(ARMRelocationTest, AddsCantUnwindBoundariesForUnindexedText) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabihf"),
                         R"(.syntax unified
.arm
.section .text.a,"ax",%progbits
.globl a
.type a,%function
a:
.fnstart
  push {r4, lr}
  pop {r4, pc}
.fnend
.section .text.b,"ax",%progbits
.globl b
.type b,%function
b:
  bx lr
.section .text.c,"ax",%progbits
.globl c
.type c,%function
c:
  bx lr
.section .text.d,"ax",%progbits
.globl d
.type d,%function
d:
.fnstart
  push {r4, lr}
  pop {r4, pc}
.fnend
)",
                         "");
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(addARMExidxBoundaries(*Graph));
  ASSERT_TRUE(ELFWriter::layout(*Graph));
  ASSERT_TRUE(applyRelocations(*Graph));
  std::map<uint64_t, uint32_t> Entries;
  for (const auto &Value : Graph->sections()) {
    if (Value.Purpose != SectionPurpose::ARMExidx)
      continue;
    for (uint64_t Offset = 0; Offset < Value.Content.size(); Offset += 8) {
      const auto Function =
          Internal::readUnsigned(Value.Content, Offset, 4, Endianness::Little);
      const auto Data = Internal::readUnsigned(Value.Content, Offset + 4, 4,
                                               Endianness::Little);
      ASSERT_TRUE(Function && Data);
      const int64_t Delta = llvm::SignExtend64<31>(*Function);
      Entries.emplace(static_cast<uint64_t>(
                          static_cast<int64_t>(Value.Address + Offset) + Delta),
                      static_cast<uint32_t>(*Data));
    }
  }
  for (const char *Name : {"a", "b", "c", "d"}) {
    const auto Found =
        std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                     [&](const auto &Value) { return Value.Name == Name; });
    ASSERT_NE(Found, Graph->symbols().end()) << Name;
    const uint64_t Address =
        Graph->sections()[Found->Section].Address + Found->Offset;
    const auto Entry = Entries.find(Address);
    ASSERT_NE(Entry, Entries.end()) << Name;
    const bool Unindexed = Name[0] == 'b' || Name[0] == 'c';
    EXPECT_EQ(Entry->second == 1U, Unindexed) << Name;
  }
  const auto Last =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Value) { return Value.Name == ".text.d"; });
  ASSERT_NE(Last, Graph->sections().end());
  const auto End = Entries.find(Last->Address + Last->VirtualSize);
  ASSERT_NE(End, Entries.end());
  EXPECT_EQ(End->second, 1U);
  EXPECT_EQ(Entries.size(), 5U);
}

TEST(ARMRelocationTest, KeepsLinkBitForGeneratedConditionalCall) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabihf"),
                         R"(.syntax unified
.arm
.section .text,"ax",%progbits
.globl caller
.type caller,%function
caller:
  cmp r0, #0
  bleq target
  bne target
  bx lr
.section .text.target,"ax",%progbits
.type target,%function
target:
  bx lr
)",
                         "");
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  std::vector<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations())
    Types.push_back(Relocation.Type);
  ASSERT_EQ(Types, (std::vector<uint32_t>{llvm::ELF::R_ARM_JUMP24,
                                          llvm::ELF::R_ARM_JUMP24}));
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto Text =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Section) { return Section.Name == ".text"; });
  ASSERT_NE(Text, Graph->sections().end());
  const auto Call =
      Internal::readUnsigned(Text->Content, 4, 4, Endianness::Little);
  const auto Jump =
      Internal::readUnsigned(Text->Content, 8, 4, Endianness::Little);
  ASSERT_TRUE(Call);
  ASSERT_TRUE(Jump);
  EXPECT_EQ(*Call & UINT32_C(0xFF000000), UINT32_C(0x0B000000));
  EXPECT_EQ(*Jump & UINT32_C(0xFF000000), UINT32_C(0x1A000000));
}

TEST(ARMRelocationTest, AcceptsGeneratedPersonalityMarker) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeObject(llvm::Triple("armv7-unknown-linux-gnueabihf"), false, false,
                 "f0", {}, true, true, "generic", "+thumb-mode", true);
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  for (const auto &Relocation : Graph->relocations())
    EXPECT_NE(Relocation.Type, llvm::ELF::R_ARM_NONE);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(ARMRelocationTest, RejectsGeneratedPersonalityReference) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"), R"(.syntax unified
.thumb
.text
.globl handler
.thumb_func
handler:
  bx lr
.section .ARM.extab,"a",%progbits
.align 2
  .word __aeabi_unwind_cpp_pr0(prel31)
)",
      "+thumb-mode");
  auto Result = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "undefined symbol");
  EXPECT_EQ(Result.error().SymbolName, "__aeabi_unwind_cpp_pr0");
}

TEST(ARMRelocationTest, RejectsMarkersOutsideTheEHABIPersonalities) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
#if LLVM_VERSION_MAJOR < 11
  GTEST_SKIP() << "the ARM assembler does not accept relocation names in "
                  ".reloc before LLVM 11";
#endif
  for (const char *Name : {"other_marker", "__aeabi_unwind_cpp_pr0"}) {
    SCOPED_TRACE(Name);
    const auto ObjectBytes = makeAssemblyObject(
        llvm::Triple("armv7-unknown-linux-gnueabihf"),
        std::string(".syntax unified\n.text\n.globl handler\nhandler:\n"
                    "  .reloc ., R_ARM_NONE, ") +
            Name + "\n  bx lr\n");
    auto Result = ObjectReader::read(ObjectBytes, Target::ARM);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "undefined symbol");
    EXPECT_EQ(Result.error().SymbolName, Name);
  }
}

TEST(ARMRelocationTest, RejectsGeneratedDivisionImport) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"), R"(.syntax unified
.thumb
.text
.globl divide
.thumb_func
divide:
  bl __aeabi_uldivmod
  bx lr
)",
      "+thumb-mode");
  auto Result = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "undefined symbol");
  EXPECT_EQ(Result.error().SymbolName, "__aeabi_uldivmod");
}

TEST(ARMRelocationTest, RelocatesSelfCallAndPackedDataAtZeroImageBase) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"), R"(.syntax unified
.arm
.text
.globl self
.type self,%function
self:
  bl self
.section .data.packed,"aw",%progbits
.byte 0
.long self
.long self-.
)");
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(layout(*Graph));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(ARMRelocationTest, RelocatesThumbSelfCallAtZeroImageBase) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"), R"(.syntax unified
.thumb
.text
.globl self
.thumb_func
self:
  bl self
)",
      "+thumb-mode");
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(layout(*Graph));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(ARMRelocationTest, AcceptsNoneOnlyAtZeroWidth) {
  auto None =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 0, 0, 0x1000);
  ASSERT_TRUE(applyRelocations(None));
}

TEST(ARMRelocationTest, RejectsUnsupportedAtomically) {
  auto Unsupported =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 2, 4, 0x1100);
  const_cast<std::vector<Relocation> &>(Unsupported.relocations())[0].Type = 99;
  const auto Snapshot = Unsupported;
  auto Result = applyRelocations(Unsupported);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
  expectGraphStateEquals(Unsupported, Snapshot);
}

TEST(AArch64RelocationTest, AppliesAbsolutePrelAndCallRelocations) {
  auto Absolute = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                         0x101, 8, 0x2000, 0x1000, 0, -8);
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 8,
                                    Endianness::Little),
            0x1FF8U);
  ASSERT_EQ(Absolute.rebases().size(), 1U);

  auto Prel = makeELFRelocationGraph(Target::AArch64, Endianness::Little, 0x105,
                                     4, 0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(Prel));
  EXPECT_EQ(*Internal::readSigned(Prel.sections()[0].Content, 0, 4,
                                  Endianness::Little),
            -0xFC);

  for (const auto &[Delta, Accepted] :
       std::array<std::pair<int64_t, bool>, 6>{{{4, true},
                                                {-4, true},
                                                {134217724, true},
                                                {-134217728, true},
                                                {2, false},
                                                {134217728, false}}}) {
    const uint64_t Patch = UINT64_C(0x20000000);
    const uint64_t Target = Delta < 0 ? Patch - static_cast<uint64_t>(-Delta)
                                      : Patch + static_cast<uint64_t>(Delta);
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x94000000)));
    auto Graph =
        makeELFRelocationGraph(Target::AArch64, Endianness::Little, 0x11B, 4,
                               Target, Patch, 0, 0, false, std::move(Bytes));
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Accepted);
  }
}

TEST(AArch64RelocationTest, EncodesPageAndScaledLow12Relocations) {
  std::vector<WasmEdge::Byte> AdrBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(AdrBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x90000000)));
  auto Adr = makeELFRelocationGraph(Target::AArch64, Endianness::Little, 0x113,
                                    4, 0x201000, 0x1FFFFC, 0, 0, false,
                                    std::move(AdrBytes));
  ASSERT_TRUE(applyRelocations(Adr));
  EXPECT_EQ(*Internal::readUnsigned(Adr.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xD0000000));

  struct Case {
    uint32_t Type;
    uint32_t Instruction;
    uint64_t Address;
    uint32_t Expected;
    bool Accepted;
  };
  const std::array<Case, 10> Cases{{
      {0x115, 0x91000000, 0x1ABC, 0x912AF000, true},
      {0x116, 0x39000000, 0x1ABC, 0x392AF000, true},
      {0x11C, 0x79000000, 0x1ABC, 0x79157800, true},
      {0x11D, 0xB9000000, 0x1ABC, 0xB90ABC00, true},
      {0x11E, 0xF9000000, 0x1AB8, 0xF9055C00, true},
      {0x12B, 0x3D800000, 0x1AB0, 0x3D82AC00, true},
      {0x11E, 0xF9000000, 0x1ABC, 0, false},
      {0x11E, 0xD503201F, 0x1AB8, 0, false},
      {0x11E, 0xBD400000, 0x1AB8, 0, false},
      {0x11E, 0x3DC00000, 0x1AB8, 0, false},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        Test.Instruction));
    auto Graph = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                        Test.Type, 4, Test.Address, 0x1000, 0,
                                        0, false, std::move(Bytes));
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
    if (Test.Accepted) {
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(AArch64RelocationTest, EncodesMachOPageOff12InstructionClasses) {
  struct Case {
    uint32_t Instruction;
    uint64_t Address;
    uint32_t Expected;
    bool Accepted;
  };
  const std::array<Case, 8> Cases{{
      {0x39000000, 0x1ABC, 0x392AF000, true},
      {0x79000000, 0x1ABC, 0x79157800, true},
      {0xB9000000, 0x1ABC, 0xB90ABC00, true},
      {0xF9000000, 0x1AB8, 0xF9055C00, true},
      {0x3D800000, 0x1AB0, 0x3D82AC00, true},
      {0xF9000000, 0x1ABC, 0, false},
      {0x3D800000, 0x1AB8, 0, false},
      {0xD503201F, 0x1ABC, 0, false},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        Test.Instruction));
    auto Graph = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                        llvm::MachO::ARM64_RELOC_PAGEOFF12, 4,
                                        Test.Address, 0x1000, 0, 0, false,
                                        std::move(Bytes), ObjectFormat::MachO);
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
    if (Test.Accepted) {
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(AArch64RelocationTest, AppliesSignedMachOUnsignedImplicitAddends) {
  struct Case {
    uint64_t Target;
    int64_t Addend;
    bool Accepted;
    uint64_t Expected;
  };
  const std::array<Case, 4> Cases{{
      {0x2000, -1, true, 0x1FFF},
      {UINT64_C(0x8000000000001000), INT64_MIN, true, 0x1000},
      {0, -1, false, 0},
      {UINT64_MAX, 1, false, 0},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(
        Internal::writeSigned(Bytes, 0, 8, Endianness::Little, Test.Addend));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little, llvm::MachO::ARM64_RELOC_UNSIGNED,
        8, Test.Target, 0x1000, 0, Test.Addend == 1 ? 1 : 0, Test.Addend != 1,
        std::move(Bytes), ObjectFormat::MachO);
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
    if (Test.Accepted) {
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 8,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(AArch64RelocationTest, RejectsShiftedAddLow12InstructionAtomically) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x91400000)));
  auto Graph = makeELFRelocationGraph(
      Target::AArch64, Endianness::Little, llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
      4, 0x1ABC, 0x1000, 0, 0, false, std::move(Bytes));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(AArch64RelocationTest, AppliesLoadStore64Low12ToDRegisterInstruction) {
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFD400000)));
  auto Graph =
      makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                             llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC, 4, 0x1AB8,
                             0x1000, 0, 0, false, std::move(Bytes));

  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xFD455C00));
}

TEST(AArch64RelocationTest, AppliesLoadStoreLow12ToHAndSRegisters) {
  struct Case {
    uint32_t Type;
    uint32_t Instruction;
    uint64_t Target;
    uint32_t Expected;
  };
  const std::array<Case, 2> Cases{{
      {llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC, UINT32_C(0x7D400000),
       UINT64_C(0x1AB8), UINT32_C(0x7D557000)},
      {llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC, UINT32_C(0xBD400000),
       UINT64_C(0x1ABC), UINT32_C(0xBD4ABC00)},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        Test.Instruction));
    auto Graph = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                        Test.Type, 4, Test.Target, 0x1000, 0, 0,
                                        false, std::move(Bytes));

    auto Result = applyRelocations(Graph);
    ASSERT_TRUE(Result) << Result.error().Message;
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                      Endianness::Little),
              Test.Expected);
  }
}

TEST(AArch64RelocationTest, RejectsInvalidLoadStoreLow12Atomically) {
  struct Case {
    uint32_t Type;
    uint32_t Instruction;
  };
  const std::array<Case, 4> Cases{{
      {llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC, UINT32_C(0xB9400000)},
      {llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC, UINT32_C(0x79400000)},
      {llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC, UINT32_C(0x78400000)},
      {llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC, UINT32_C(0xB8400000)},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        Test.Instruction));
    auto Graph = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                        Test.Type, 4, 0x1AB8, 0x1000, 0, 0,
                                        false, std::move(Bytes));
    const auto Snapshot = Graph;

    EXPECT_FALSE(applyRelocations(Graph));
    expectGraphStateEquals(Graph, Snapshot);
  }
}

TEST(AArch64RelocationTest, ChecksAdrpPageDeltaBoundaries) {
  constexpr uint64_t PageSize = UINT64_C(1) << 12;
  constexpr uint64_t PatchAddress = UINT64_C(0x200000000);
  struct Case {
    int64_t Pages;
    bool Accepted;
  };
  const std::array<Case, 4> Cases{{
      {-INT64_C(1048576), true},
      {INT64_C(1048575), true},
      {-INT64_C(1048577), false},
      {INT64_C(1048576), false},
  }};
  for (const auto &Test : Cases) {
    const uint64_t TargetAddress =
        Test.Pages < 0
            ? PatchAddress - static_cast<uint64_t>(-Test.Pages) * PageSize
            : PatchAddress + static_cast<uint64_t>(Test.Pages) * PageSize;
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x90000000)));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little,
        llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21, 4, TargetAddress,
        PatchAddress + PageSize - 4, 0, 0, false, std::move(Bytes));
    const auto Snapshot = Graph;
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
    }
  }
}

TEST(AArch64RelocationTest, EncodesAdrpAddAndLoadPairsExactly) {
  constexpr uint64_t PatchAddress = UINT64_C(0x1000);
  constexpr uint64_t TargetAddress = UINT64_C(0x3AB8);
  for (const auto &[LowType, LowInstruction, ExpectedLow] :
       std::array<std::tuple<uint32_t, uint32_t, uint32_t>, 2>{{
           {llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC, UINT32_C(0x91000000),
            UINT32_C(0x912AE000)},
           {llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC, UINT32_C(0xF9400000),
            UINT32_C(0xF9455C00)},
       }}) {
    LinkGraph Graph(Target::AArch64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("pair.o"));
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x90000000)));
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                        LowInstruction));
    auto Text = Graph.addSection(Section{".text", SectionKind::Text, 4, 8,
                                         PatchAddress, 0, std::move(Bytes)});
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 8, 8, TargetAddress, 0, {0}});
    ASSERT_TRUE(Text && Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 8, false});
    ASSERT_TRUE(TargetSymbol);
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Text, 0, llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
                   *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Text, 4, LowType, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                      Endianness::Little),
              UINT32_C(0xD0000000));
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                      Endianness::Little),
              ExpectedLow);
  }
}

TEST(AArch64RelocationTest, RelocatesGeneratedAdrpLow12PairsExactly) {
  REQUIRE_LLVM_TARGET("aarch64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeAssemblyObject(llvm::Triple("aarch64-unknown-linux-gnu"),
                         R"(.text
.globl f
.type f,%function
f:
  adrp x0, data
  add x0, x0, :lo12:data
  adrp x1, data
  ldr x1, [x1, :lo12:data]
  ret
.data
.p2align 3
data:
  .xword 0
)"),
      Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 4U);
  const std::array<uint32_t, 4> Types{{
      llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
      llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
      llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
      llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC,
  }};
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECT_EQ(Graph->relocations()[I].Type, Types[I]);
  }
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto &First = Graph->relocations()[0];
  const auto &Symbol = Graph->symbols()[First.Symbol];
  const uint64_t TargetAddress =
      Graph->sections()[Symbol.Section].Address + Symbol.Offset;
  const uint32_t Low12 = static_cast<uint32_t>(TargetAddress & 0xFFF);
  const auto &Text = Graph->sections()[First.Section];
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 4, 4, Endianness::Little),
            UINT32_C(0x91000000) | (Low12 << 10));
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 12, 4, Endianness::Little),
            UINT32_C(0xF9400021) | ((Low12 >> 3) << 10));
}

TEST(AArch64RelocationTest, RelocatesGeneratedFPSIMDLow12Loads) {
  REQUIRE_LLVM_TARGET("aarch64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeAssemblyObject(llvm::Triple("aarch64-unknown-linux-gnu"),
                         R"(.text
.globl f
.type f,%function
f:
  adrp x0, hdata
  ldr h0, [x0, :lo12:hdata]
  adrp x1, sdata
  ldr s0, [x1, :lo12:sdata]
  ret
.data
.p2align 1
hdata:
  .hword 0
.p2align 2
sdata:
  .word 0
)"),
      Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 4U);
  EXPECT_EQ(Graph->relocations()[1].Type,
            llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC);
  EXPECT_EQ(Graph->relocations()[3].Type,
            llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  auto Result = applyRelocations(*Graph);
  ASSERT_TRUE(Result) << Result.error().Message;
}

TEST(AArch64RelocationTest, DecodesCOFFImplicitAddendsExactly) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  struct Case {
    uint32_t Type;
    uint32_t Initial;
    uint64_t Target;
    uint64_t Patch;
    uint32_t Expected;
  };
  const std::array<Case, 5> Cases{{
      {llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, 5, ImageBase + 0x2000,
       ImageBase + 0x1000, 0x2005},
      {llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 0x94000002, ImageBase + 0x1100,
       ImageBase + 0x1000, 0x94000042},
      {llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 0x97FFFFFF, ImageBase + 0x1100,
       ImageBase + 0x1000, 0x9400003F},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A, 0x91001400,
       ImageBase + 0x1FFC, ImageBase + 0x1000, 0x91000400},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L, 0xF9400800,
       ImageBase + 0x1FF8, ImageBase + 0x1000, 0xF9400400},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(
        Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little, Test.Initial));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little, Test.Type, 4, Test.Target,
        Test.Patch, 0, 0, true, std::move(Bytes), ObjectFormat::COFF);
    ASSERT_TRUE(Graph.setImageBase(ImageBase));
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                      Endianness::Little),
              Test.Expected)
        << Test.Type;
  }

  std::vector<WasmEdge::Byte> SIMD(8);
  ASSERT_TRUE(Internal::writeUnsigned(SIMD, 0, 4, Endianness::Little,
                                      UINT32_C(0x3DC00400)));
  auto SIMDGraph = makeELFRelocationGraph(
      Target::AArch64, Endianness::Little,
      llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L, 4, ImageBase + 0x1FF0,
      ImageBase + 0x1000, 0, 0, true, std::move(SIMD), ObjectFormat::COFF);
  ASSERT_TRUE(applyRelocations(SIMDGraph));
  EXPECT_EQ(*Internal::readUnsigned(SIMDGraph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x3DC00000));
}

TEST(AArch64RelocationTest, ResolvesCOFFPageBaseImplicitByteAddends) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  struct Case {
    uint32_t Initial;
    uint64_t Target;
    uint64_t Patch;
    uint64_t ExpectedPage;
    bool Accepted;
  };
  const std::array<Case, 8> Cases{{
      {UINT32_C(0xB0000000), ImageBase + 0x5000, ImageBase + 0x1000,
       ImageBase + 0x5000, true},
      {UINT32_C(0xF0FFFFE0), ImageBase + 0x5001, ImageBase + 0x1000,
       ImageBase + 0x5000, true},
      {UINT32_C(0xB0000000), ImageBase + (UINT64_C(1048575) << 12), ImageBase,
       ImageBase + (UINT64_C(1048575) << 12), true},
      {UINT32_C(0xF0FFFFE0), ImageBase - (UINT64_C(1048576) << 12) + 1,
       ImageBase, ImageBase - (UINT64_C(1048576) << 12), true},
      {UINT32_C(0xB0008000), ImageBase + (UINT64_C(1048575) << 12), ImageBase,
       0, false},
      {UINT32_C(0xF0FF7FE0), ImageBase - (UINT64_C(1048576) << 12), ImageBase,
       0, false},
      {UINT32_C(0xB0000000), UINT64_MAX, ImageBase, 0, false},
      {UINT32_C(0xF0FFFFE0), 0, ImageBase, 0, false},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(
        Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little, Test.Initial));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little,
        llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21, 4, Test.Target, Test.Patch,
        0, 0, true, std::move(Bytes), ObjectFormat::COFF);
    const auto Snapshot = Graph;
    ASSERT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted)
        << Test.Initial;
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
      continue;
    }
    const auto Instruction = Internal::readUnsigned(Graph.sections()[0].Content,
                                                    0, 4, Endianness::Little);
    ASSERT_TRUE(Instruction);
    EXPECT_EQ(
        decodeAdrpTargetPage(static_cast<uint32_t>(*Instruction), Test.Patch),
        Test.ExpectedPage);
  }
}

TEST(AArch64RelocationTest, RejectsMalformedCOFFImplicitInstructions) {
  for (const auto Type : {llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                          llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                          llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A,
                          llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L}) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0xD503201F)));
    auto Graph =
        makeELFRelocationGraph(Target::AArch64, Endianness::Little, Type, 4,
                               UINT64_C(0x180002000), UINT64_C(0x180001000), 0,
                               0, true, std::move(Bytes), ObjectFormat::COFF);
    const auto Snapshot = Graph;
    EXPECT_FALSE(applyRelocations(Graph)) << Type;
    expectGraphStateEquals(Graph, Snapshot);
  }
}

TEST(AArch64RelocationTest, RelocatesGeneratedCOFFAddendsExactly) {
  REQUIRE_LLVM_TARGET("aarch64-pc-windows-msvc");
  const auto Object =
      makeAssemblyObject(llvm::Triple("aarch64-pc-windows-msvc"), R"(
.text
.globl f
.globl target
f:
  adrp x0, data+4097
  add x0, x0, :lo12:data+4097
  adrp x1, data-1
  add x1, x1, :lo12:data-1
  bl target+8
target:
  ret
.data
.p2align 4
.globl data
data:
  .xword 0
)");
  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 4U);
  ASSERT_TRUE(PEWriter::layout(*Graph));
  const auto Before =
      Graph->sections()[Graph->relocations()[0].Section].Content;
  EXPECT_TRUE(std::any_of(Before.begin(), Before.end(),
                          [](WasmEdge::Byte Value) { return Value != 0; }));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto &Text = Graph->sections()[Graph->relocations()[0].Section];
  const auto DataSymbol =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "data"; });
  ASSERT_NE(DataSymbol, Graph->symbols().end());
  const uint64_t DataAddress =
      Graph->sections()[DataSymbol->Section].Address + DataSymbol->Offset;
  const auto FirstADRP =
      Internal::readUnsigned(Text.Content, 0, 4, Endianness::Little);
  const auto SecondADRP =
      Internal::readUnsigned(Text.Content, 8, 4, Endianness::Little);
  ASSERT_TRUE(FirstADRP && SecondADRP);
  EXPECT_EQ(
      decodeAdrpTargetPage(static_cast<uint32_t>(*FirstADRP), Text.Address),
      (DataAddress + 4097) & ~UINT64_C(0xFFF));
  EXPECT_EQ(decodeAdrpTargetPage(static_cast<uint32_t>(*SecondADRP),
                                 Text.Address + 8),
            (DataAddress - 1) & ~UINT64_C(0xFFF));
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 4, 4, Endianness::Little),
            UINT32_C(0x91000000) | (((DataAddress + 4097) & 0xFFF) << 10));
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 12, 4, Endianness::Little),
            UINT32_C(0x91000021) | (((DataAddress - 1) & 0xFFF) << 10));
}

TEST(RISCVRelocationTest, AppliesAbsoluteCallAndUnwindRelocations) {
  auto Absolute = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 2,
                                         8, 0x2000, 0x1000, 0, -8);
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 8,
                                    Endianness::Little),
            0x1FF8U);
  ASSERT_EQ(Absolute.rebases().size(), 1U);

  std::vector<WasmEdge::Byte> CallBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000097)));
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x000080E7)));
  auto Call = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 18, 8,
                                     0x11234, 0x10000, 0, 0, false,
                                     std::move(CallBytes));
  ASSERT_TRUE(applyRelocations(Call));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001097));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x234080E7));

  auto Unwind = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 57,
                                       4, 0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(Unwind));
  EXPECT_EQ(*Internal::readSigned(Unwind.sections()[0].Content, 0, 4,
                                  Endianness::Little),
            -0xFC);
}

TEST(RISCVRelocationTest, ReadsGeneratedSymbolDifferenceObject) {
#if LLVM_VERSION_MAJOR < 15
  GTEST_SKIP() << "the RISC-V assembler cannot represent cross-section "
                  "symbol differences before LLVM 15; direct relocation logic "
                  "is tested separately";
#endif
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  constexpr std::string_view Assembly = R"(
    .pushsection .rodata.symbol_begin,"a",@progbits
  begin:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_end,"a",@progbits
  end:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_difference,"a",@progbits
    .globl symbol_difference
  symbol_difference:
    .word end - begin
    .popsection
  )";
  const auto Object =
      makeObject(llvm::Triple("riscv64-unknown-linux-gnu"), false, false, "f0",
                 {}, true, true, "generic-rv64", "+a", false, false, false,
                 false, false, false, Assembly.data());
  auto Graph = ObjectReader::read(Object, Target::RISCV64);
  ASSERT_TRUE(Graph);
  std::set<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations()) {
    if (Relocation.Type == llvm::ELF::R_RISCV_ADD32 ||
        Relocation.Type == llvm::ELF::R_RISCV_SUB32) {
      Types.insert(Relocation.Type);
      EXPECT_EQ(Relocation.PatchSize, 4U);
    }
  }
  EXPECT_EQ(Types, (std::set<uint32_t>{llvm::ELF::R_RISCV_ADD32,
                                       llvm::ELF::R_RISCV_SUB32}));
}

TEST(RISCVRelocationTest, ReadsGeneratedByteSymbolDifferenceObject) {
#if LLVM_VERSION_MAJOR < 15
  GTEST_SKIP() << "the RISC-V assembler cannot represent cross-section "
                  "symbol differences before LLVM 15; direct relocation logic "
                  "is tested separately";
#endif
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  constexpr std::string_view Assembly = R"(
    .pushsection .rodata.symbol_begin,"a",@progbits
  begin:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_end,"a",@progbits
  end:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_difference,"a",@progbits
  symbol_difference:
    .byte end - begin
    .popsection
  )";
  const auto Object =
      makeObject(llvm::Triple("riscv64-unknown-linux-gnu"), false, false, "f0",
                 {}, true, true, "generic-rv64", "+a", false, false, false,
                 false, false, false, Assembly.data());
  auto Graph = ObjectReader::read(Object, Target::RISCV64);
  ASSERT_TRUE(Graph);
  std::set<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations()) {
    if (Relocation.Type == llvm::ELF::R_RISCV_ADD8 ||
        Relocation.Type == llvm::ELF::R_RISCV_SET8 ||
        Relocation.Type == llvm::ELF::R_RISCV_SUB8) {
      Types.insert(Relocation.Type);
      EXPECT_EQ(Relocation.PatchSize, 1U);
    }
  }
  EXPECT_TRUE(Types == (std::set<uint32_t>{llvm::ELF::R_RISCV_ADD8,
                                           llvm::ELF::R_RISCV_SUB8}) ||
              Types == (std::set<uint32_t>{llvm::ELF::R_RISCV_SET8,
                                           llvm::ELF::R_RISCV_SUB8}));
}

TEST(RISCVRelocationTest, AppliesSymbolDifferencePairsModulo32InEitherOrder) {
  struct Case {
    uint64_t AddOffset;
    uint64_t SubOffset;
    uint32_t Expected;
  };
  const std::array<Case, 2> Cases{{
      {31, 4, UINT32_C(0x00000015)},
      {4, 31, UINT32_C(0xFFFFFFDF)},
  }};
  for (const auto &Test : Cases) {
    for (const bool Reverse : {false, true}) {
      LinkGraph Graph(Target::RISCV64, Endianness::Little);
      ASSERT_TRUE(Graph.beginInput("difference.o"));
      std::vector<WasmEdge::Byte> Bytes(4);
      ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                          UINT32_C(0xFFFFFFF0)));
      auto Field = Graph.addSection(Section{".data", SectionKind::Data, 4, 4,
                                            0x1000, 0, std::move(Bytes)});
      auto Symbols = Graph.addSection(Section{".symbols", SectionKind::Data, 1,
                                              32, UINT64_MAX - 31, 0,
                                              std::vector<WasmEdge::Byte>(32)});
      ASSERT_TRUE(Field && Symbols);
      auto AddSymbol =
          Graph.addSymbol(Symbol{"add", *Symbols, Test.AddOffset, 0, false});
      auto SubSymbol =
          Graph.addSymbol(Symbol{"sub", *Symbols, Test.SubOffset, 0, false});
      ASSERT_TRUE(AddSymbol && SubSymbol);
      const Relocation Add{*Field, 0,     llvm::ELF::R_RISCV_ADD32, *AddSymbol,
                           7,      false, ObjectFormat::ELF,        4};
      const Relocation Sub{*Field, 0,     llvm::ELF::R_RISCV_SUB32, *SubSymbol,
                           -3,     false, ObjectFormat::ELF,        4};
      ASSERT_TRUE(Graph.addRelocation(Reverse ? Sub : Add));
      ASSERT_TRUE(Graph.addRelocation(Reverse ? Add : Sub));
      ASSERT_TRUE(applyRelocations(Graph));
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[*Field].Content, 0, 4,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(RISCVRelocationTest, AppliesSixBitSymbolDifferenceKeepingHighBits) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("advance.o"));
    auto Field = Graph.addSection(
        Section{".eh_frame", SectionKind::Data, 1, 1, 0x1000, 0, {0x7F}});
    auto Symbols =
        Graph.addSection(Section{".text", SectionKind::Text, 4, 128, 0x2000, 0,
                                 std::vector<WasmEdge::Byte>(128)});
    ASSERT_TRUE(Field && Symbols);
    auto Begin = Graph.addSymbol(Symbol{"begin", *Symbols, 4, 0, false});
    auto End = Graph.addSymbol(Symbol{"end", *Symbols, 90, 0, false});
    ASSERT_TRUE(Begin && End);
    const Relocation Set{*Field, 0,     llvm::ELF::R_RISCV_SET6, *End,
                         0,      false, ObjectFormat::ELF,       1};
    const Relocation Sub{*Field, 0,     llvm::ELF::R_RISCV_SUB6, *Begin,
                         0,      false, ObjectFormat::ELF,       1};
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Sub : Set));
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Set : Sub));
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_EQ(Graph.sections()[*Field].Content[0], 0x40 | ((90 - 4) & 0x3F));
  }
}

TEST(RISCVRelocationTest, RejectsSixBitDifferencePairedWithByteDifference) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("advance.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x1000, 0, {0x40}});
  ASSERT_TRUE(Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph.addRelocation(Relocation{*Data, 0, llvm::ELF::R_RISCV_SET6,
                                             *TargetSymbol, 0, false,
                                             ObjectFormat::ELF, 1}));
  ASSERT_TRUE(Graph.addRelocation(Relocation{*Data, 0, llvm::ELF::R_RISCV_SUB8,
                                             *TargetSymbol, 0, false,
                                             ObjectFormat::ELF, 1}));
  EXPECT_FALSE(Graph.validate());
}

TEST(RISCVRelocationTest, RejectsMalformedSymbolDifferencePairsAtomically) {
  for (const uint32_t SecondType :
       {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_32_PCREL}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("malformed.o"));
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 4, 4, 0x1000, 0, {1, 2, 3, 4}});
    ASSERT_TRUE(Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Data, 0, llvm::ELF::R_RISCV_ADD32, *TargetSymbol, 0, false,
                   ObjectFormat::ELF, 4}));
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Data, 0, SecondType, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
    EXPECT_FALSE(Graph.validate());
    const auto Snapshot = Graph;
    EXPECT_FALSE(applyRelocations(Graph));
    expectGraphStateEquals(Graph, Snapshot);
  }
}

TEST(RISCVRelocationTest, PairsPcrelLowWithMarkedHighSiteAndAllowsRelax) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  std::vector<WasmEdge::Byte> Bytes(12);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000297)));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x00028293)));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 12, 0x1000, 0, std::move(Bytes)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, 23, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, 24, *HighSite, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, 51, *TargetSymbol, 0, false, ObjectFormat::ELF, 0}));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001297));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x23428293));
}

TEST(RISCVRelocationTest, PairsPcrelLowBeforeHighRelocation) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000297)));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x00028293)));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0, std::move(Bytes)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_LO12_I, *HighSite, 0, false,
                 ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001297));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x23428293));
}

TEST(RISCVRelocationTest, RejectsMalformedHighBeforeMutation) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x00028293)));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0, std::move(Bytes)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_LO12_I, *HighSite, 0, false,
                 ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RISCVRelocationTest, RejectsMissingLowPairAndInvalidOpcodes) {
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000293)));
  auto Missing =
      makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 24, 4, 0x1000,
                             0x1000, 0, 0, false, std::move(Bytes));
  EXPECT_FALSE(applyRelocations(Missing));

  auto BadCall = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 18,
                                        8, 0x1100, 0x1000);
  EXPECT_FALSE(applyRelocations(BadCall));
}

TEST(RISCVRelocationTest, EncodesCallPltAndPcrelLo12SExactly) {
  std::vector<WasmEdge::Byte> CallBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000097)));
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x000080E7)));
  auto Call = makeELFRelocationGraph(
      Target::RISCV64, Endianness::Little, llvm::ELF::R_RISCV_CALL_PLT, 8,
      0x11234, 0x10000, 0, 0, false, std::move(CallBytes));
  ASSERT_TRUE(applyRelocations(Call));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001097));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x234080E7));

  LinkGraph Store(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Store.beginInput("store.o"));
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000297)));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x0052A023)));
  auto Text = Store.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0, std::move(Bytes)});
  auto Data = Store.addSection(
      Section{".data", SectionKind::Data, 4, 4, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Store.addSymbol(Symbol{"target", *Data, 0, 4, false});
  auto HighSite = Store.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Store.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Store.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_LO12_S, *HighSite, 0, false,
                 ObjectFormat::ELF, 4}));
  ASSERT_TRUE(applyRelocations(Store));
  EXPECT_EQ(*Internal::readUnsigned(Store.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x2252AA23));
}

TEST(RISCVRelocationTest, RelocatesCompressedMisalignedSites) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
#if LLVM_VERSION_MAJOR < 11
  GTEST_SKIP() << "the RISC-V assembler resolves local calls without a "
                  "relocation before LLVM 11";
#endif
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("riscv64-unknown-linux-gnu"), R"(
.text
.globl caller
caller:
  c.nop
  call target
.Lhi:
  auipc a0, %pcrel_hi(gvar)
  lw a1, %pcrel_lo(.Lhi)(a0)
  ret
.globl target
target:
  ret
.data
.globl gvar
gvar:
  .word 0
)",
                         "+c");
  auto Graph = ObjectReader::read(ObjectBytes, Target::RISCV64);
  ASSERT_TRUE(Graph);
  std::set<uint64_t> Offsets;
  for (const auto &Relocation : Graph->relocations())
    Offsets.insert(Relocation.Offset);
  EXPECT_NE(Offsets.find(2), Offsets.end());
  ASSERT_TRUE(layout(*Graph, 0x1000));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(RISCVRelocationTest, RelocatesFloatingPointLow12LoadsAndStores) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("riscv64-unknown-linux-gnu"), R"(
.text
.globl fpaccess
fpaccess:
.Lstore:
  auipc a0, %pcrel_hi(gvar)
  fsd fa0, %pcrel_lo(.Lstore)(a0)
.Lload:
  auipc a1, %pcrel_hi(gvar)
  fld fa1, %pcrel_lo(.Lload)(a1)
  ret
.data
.globl gvar
gvar:
  .dword 0
)",
                         "+d");
  auto Graph = ObjectReader::read(ObjectBytes, Target::RISCV64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(RISCVRelocationTest, ChecksRoundedHighSignedRange) {
  constexpr uint64_t PatchAddress = UINT64_C(0x200000000);
  struct Case {
    int64_t Delta;
    bool Accepted;
    int64_t RoundedDelta;
  };
  const std::array<Case, 4> Cases{{
      {-INT64_C(0x80000800), true, -INT64_C(0x80000000)},
      {-INT64_C(0x80000801), false, 0},
      {INT64_C(0x7FFFF7FF), true, INT64_C(0x7FFFF000)},
      {INT64_C(0x7FFFF800), false, 0},
  }};
  for (const uint32_t Type :
       {llvm::ELF::R_RISCV_CALL, llvm::ELF::R_RISCV_CALL_PLT}) {
    for (const auto &Test : Cases) {
      std::vector<WasmEdge::Byte> Bytes(16);
      ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                          UINT32_C(0x00000097)));
      ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                          UINT32_C(0x000080E7)));
      const uint64_t TargetAddress =
          Test.Delta < 0 ? PatchAddress - static_cast<uint64_t>(-Test.Delta)
                         : PatchAddress + static_cast<uint64_t>(Test.Delta);
      auto Graph = makeELFRelocationGraph(Target::RISCV64, Endianness::Little,
                                          Type, 8, TargetAddress, PatchAddress,
                                          0, 0, false, std::move(Bytes));
      const auto Snapshot = Graph;
      ASSERT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted)
          << Type << " " << Test.Delta;
      if (!Test.Accepted) {
        expectGraphStateEquals(Graph, Snapshot);
        continue;
      }
      const auto High = Internal::readUnsigned(Graph.sections()[0].Content, 0,
                                               4, Endianness::Little);
      const auto Low = Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                              Endianness::Little);
      ASSERT_TRUE(High && Low);
      const int64_t HighDelta = static_cast<int32_t>(
          static_cast<uint32_t>(*High) & UINT32_C(0xFFFFF000));
      const int64_t LowDelta =
          static_cast<int32_t>(static_cast<uint32_t>(*Low) &
                               UINT32_C(0xFFF00000)) >>
          20;
      EXPECT_EQ(PatchAddress + static_cast<uint64_t>(HighDelta) +
                    static_cast<uint64_t>(LowDelta),
                TargetAddress);
    }
  }

  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x00000297)));
    const uint64_t TargetAddress =
        Test.Delta < 0 ? PatchAddress - static_cast<uint64_t>(-Test.Delta)
                       : PatchAddress + static_cast<uint64_t>(Test.Delta);
    auto Graph = makeELFRelocationGraph(
        Target::RISCV64, Endianness::Little, llvm::ELF::R_RISCV_PCREL_HI20, 4,
        TargetAddress, PatchAddress, 0, 0, false, std::move(Bytes));
    const auto Snapshot = Graph;
    ASSERT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted)
        << Test.Delta;
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
      continue;
    }
    const auto High = Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                             Endianness::Little);
    ASSERT_TRUE(High);
    const int64_t HighDelta = static_cast<int32_t>(
        static_cast<uint32_t>(*High) & UINT32_C(0xFFFFF000));
    EXPECT_EQ(PatchAddress + static_cast<uint64_t>(HighDelta),
              static_cast<uint64_t>(static_cast<int64_t>(PatchAddress) +
                                    Test.RoundedDelta));
  }
}

TEST(S390XRelocationTest, AppliesBigEndianAbsoluteAndPcRelativeRelocations) {
  auto Absolute = makeELFRelocationGraph(Target::S390X, Endianness::Big, 22, 8,
                                         0x2000, 0x1000, 0, -8);
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 8,
                                    Endianness::Big),
            0x1FF8U);
  ASSERT_EQ(Absolute.rebases().size(), 1U);

  auto PC32 = makeELFRelocationGraph(Target::S390X, Endianness::Big, 5, 4,
                                     0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(PC32));
  EXPECT_EQ(
      *Internal::readSigned(PC32.sections()[0].Content, 0, 4, Endianness::Big),
      -0xFC);
}

void expectGeneratedRebaseOverlapRejected(Target Architecture,
                                          Endianness Endian, uint32_t Type,
                                          uint8_t Width) {
  auto Graph =
      makeELFRelocationGraph(Architecture, Endian, Type, Width, 0x2000);
  ASSERT_TRUE(Graph.addRebase(Rebase{0, 0, Type, 0, Width}));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsGeneratedARMRebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::ARM, Endianness::Little, 2, 4);
}

TEST(RelocationTest, RejectsGeneratedAArch64RebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::AArch64, Endianness::Little,
                                       0x101, 8);
}

TEST(RelocationTest, RejectsGeneratedRISCV64RebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::RISCV64, Endianness::Little, 2,
                                       8);
}

TEST(RelocationTest, RejectsGeneratedS390XRebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::S390X, Endianness::Big, 22, 8);
}

void expectManyAdjacentAbsoluteRelocations(Target Architecture,
                                           Endianness Endian, uint32_t Type,
                                           uint8_t Width) {
  constexpr uint32_t RelocationCount = 20000;
  LinkGraph Graph(Architecture, Endian);
  ASSERT_TRUE(Graph.beginInput("many-absolute.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, Width,
              static_cast<uint64_t>(RelocationCount) * Width, 0x1000, 0,
              std::vector<WasmEdge::Byte>(static_cast<size_t>(RelocationCount) *
                                          Width)});
  auto TargetSection = Graph.addSection(
      Section{".target", SectionKind::Data, 1, 1, 0x2000, 0, {0}});
  ASSERT_TRUE(Data && TargetSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *TargetSection, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  for (uint32_t I = 0; I < RelocationCount; ++I) {
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Data, static_cast<uint64_t>(I) * Width, Type, *TargetSymbol,
                   0, false, ObjectFormat::ELF, Width}));
  }

  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_EQ(Graph.rebases().size(), RelocationCount);
  for (uint32_t I = 0; I < RelocationCount; ++I) {
    EXPECT_EQ(Graph.rebases()[I].Section, *Data);
    EXPECT_EQ(Graph.rebases()[I].Offset, static_cast<uint64_t>(I) * Width);
    EXPECT_EQ(Graph.rebases()[I].Width, Width);
  }
}

TEST(RelocationTest, AppliesManyAdjacentX86_64AbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::X86_64, Endianness::Little,
                                        llvm::ELF::R_X86_64_64, 8);
}

TEST(RelocationTest, AppliesManyAdjacentARMAbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::ARM, Endianness::Little,
                                        llvm::ELF::R_ARM_ABS32, 4);
}

TEST(RelocationTest, AppliesManyAdjacentAArch64AbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::AArch64, Endianness::Little,
                                        llvm::ELF::R_AARCH64_ABS64, 8);
}

TEST(RelocationTest, AppliesManyAdjacentRISCV64AbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::RISCV64, Endianness::Little,
                                        llvm::ELF::R_RISCV_64, 8);
}

TEST(RelocationTest, AppliesManyAdjacentS390XAbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::S390X, Endianness::Big,
                                        llvm::ELF::R_390_64, 8);
}

TEST(S390XRelocationTest, ChecksDoubledDisplacementRangeAndAlignment) {
  struct Case {
    int64_t Delta;
    bool Accepted;
  };
  const std::array<Case, 6> Cases{{
      {-INT64_C(4294967296), true},
      {INT64_C(4294967294), true},
      {-INT64_C(4294967298), false},
      {INT64_C(4294967296), false},
      {2, true},
      {1, false},
  }};
  constexpr uint64_t Patch = UINT64_C(0x200000000);
  for (const uint32_t Type : {19U, 20U}) {
    for (const auto &Test : Cases) {
      const uint64_t Target = Test.Delta < 0
                                  ? Patch - static_cast<uint64_t>(-Test.Delta)
                                  : Patch + static_cast<uint64_t>(Test.Delta);
      auto Graph = makeELFRelocationGraph(Target::S390X, Endianness::Big, Type,
                                          4, Target, Patch);
      EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
    }
  }

  auto Little =
      makeELFRelocationGraph(Target::S390X, Endianness::Little, 5, 4, 0x1100);
  EXPECT_FALSE(applyRelocations(Little));
}

class FailingGeneratedRelocationTest
    : public testing::Test,
      public testing::WithParamInterface<FailingRelocationCase> {};

TEST_P(FailingGeneratedRelocationTest,
       RejectsWrongEndiannessAndArchitectureMismatch) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto ObjectBytes =
      makeObject(llvm::Triple(Test.Triple), false, false, "representative", {},
                 true, true, "generic", Test.Features, false, false, true, true,
                 true, Test.Architecture != Target::ARM);
  LinkGraph WrongEndianGraph(Test.Architecture, Test.WrongEndian);
  ASSERT_TRUE(WrongEndianGraph.beginInput("wrong-endian.o"));
  const auto WrongEndianSnapshot = WrongEndianGraph;
  EXPECT_FALSE(applyRelocations(WrongEndianGraph)) << Test.Triple;
  expectGraphStateEquals(WrongEndianGraph, WrongEndianSnapshot);

  const auto Mismatch =
      Test.Architecture == Target::ARM ? Target::AArch64 : Target::ARM;
  EXPECT_FALSE(ObjectReader::read(ObjectBytes, Mismatch)) << Test.Triple;
}

TEST_P(FailingGeneratedRelocationTest,
       RejectsFailingGeneratedRelocationAtomically) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto ObjectBytes =
      makeObject(llvm::Triple(Test.Triple), false, false, "representative", {},
                 true, true, "generic", Test.Features, false, false, true, true,
                 true, Test.Architecture != Target::ARM);
  auto Graph = ObjectReader::read(ObjectBytes, Test.Architecture);
  ASSERT_TRUE(Graph) << Test.Triple;
  ASSERT_TRUE(layout(*Graph, 0x1000));
  auto Patch = Graph->addSection(
      Section{".failure", SectionKind::Data, Test.Width, Test.Width, 0, 0,
              std::vector<WasmEdge::Byte>(Test.Width)});
  auto TargetSection = Graph->addSection(
      Section{".failure.target", SectionKind::Data, 1, 0, UINT64_MAX});
  ASSERT_TRUE(Patch && TargetSection);
  auto TargetSymbol =
      Graph->addSymbol(Symbol{"failure.target", *TargetSection, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph->addRelocation(Relocation{*Patch, 0, Test.AbsoluteType,
                                              *TargetSymbol, 1, false,
                                              ObjectFormat::ELF, Test.Width}));
  const auto Snapshot = *Graph;
  EXPECT_FALSE(applyRelocations(*Graph)) << Test.Triple;
  expectGraphStateEquals(*Graph, Snapshot);
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, FailingGeneratedRelocationTest,
    testing::Values(
        FailingRelocationCase{"armv7-unknown-linux-gnueabihf", Target::ARM,
                              Endianness::Big, llvm::ELF::R_ARM_ABS32, 4, ""},
        FailingRelocationCase{"aarch64-unknown-linux-gnu", Target::AArch64,
                              Endianness::Big, llvm::ELF::R_AARCH64_ABS64, 8,
                              ""},
        FailingRelocationCase{"riscv64-unknown-linux-gnu", Target::RISCV64,
                              Endianness::Big, llvm::ELF::R_RISCV_64, 8, "+a"},
        FailingRelocationCase{"s390x-unknown-linux-gnu", Target::S390X,
                              Endianness::Little, llvm::ELF::R_390_64, 8, ""}),
    failingRelocationCaseName);

TEST(RelocationTest, AppliesGeneratedELF64PC32AndPLT32RelocationsExactly) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeX86_64AssemblyObject(
          ".data\n.globl target_data\ntarget_data:\n.quad 0\n"
          ".globl absolute\nabsolute:\n.quad target_data + 5\n"
          ".text\n.globl caller\ncaller:\n"
          "movl target_data(%rip), %eax\ncall target_func\nret\n"
          ".section .text.target,\"ax\",@progbits\n"
          ".globl target_func\ntarget_func:\nret\n"),
      Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 3U);
  std::array<bool, 3> Seen{};
  for (const auto &Relocation : Graph->relocations()) {
    ASSERT_TRUE(Relocation.Type == 1 || Relocation.Type == 2 ||
                Relocation.Type == 4);
    Seen[Relocation.Type == 1 ? 0 : Relocation.Type == 2 ? 1 : 2] = true;
    EXPECT_EQ(Relocation.Format, ObjectFormat::ELF);
    EXPECT_EQ(Relocation.PatchSize, Relocation.Type == 1 ? 8U : 4U);
    EXPECT_FALSE(Relocation.AddendIsImplicit);
    EXPECT_EQ(Relocation.Addend, Relocation.Type == 1 ? 5 : -4);
  }
  EXPECT_EQ(Seen, (std::array<bool, 3>{true, true, true}));
  ASSERT_TRUE(layout(*Graph, 0x1000));
  struct ExpectedPatch {
    SectionId Section;
    uint64_t Offset;
    uint8_t Width;
    uint64_t Bits;
  };
  std::vector<ExpectedPatch> Expected;
  for (const auto &Relocation : Graph->relocations()) {
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    const uint64_t S =
        Graph->sections()[Symbol.Section].Address + Symbol.Offset;
    const uint64_t P =
        Graph->sections()[Relocation.Section].Address + Relocation.Offset;
    Expected.push_back(ExpectedPatch{
        Relocation.Section, Relocation.Offset,
        static_cast<uint8_t>(Relocation.Type == 1 ? 8 : 4),
        Relocation.Type == 1 ? S + static_cast<uint64_t>(Relocation.Addend)
                             : static_cast<uint32_t>(static_cast<int64_t>(S) +
                                                     Relocation.Addend -
                                                     static_cast<int64_t>(P))});
  }
  ASSERT_TRUE(applyRelocations(*Graph));
  for (const auto &Patch : Expected) {
    auto Value =
        Internal::readUnsigned(Graph->sections()[Patch.Section].Content,
                               Patch.Offset, Patch.Width, Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, Patch.Bits);
  }
  ASSERT_EQ(Graph->rebases().size(), 1U);
  EXPECT_EQ(Graph->rebases()[0].Width, 8U);
}

class PortableRelocationTest
    : public testing::Test,
      public testing::WithParamInterface<PortableRelocationCase> {};

TEST_P(PortableRelocationTest,
       ReadsLayoutsAndRelocatesCurrentMachOAndCOFFForms) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const llvm::Triple Triple(Test.Triple);
  auto Graph = ObjectReader::read(makeObject(Triple), Test.Architecture);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  EXPECT_EQ(Graph->relocations()[0].Format, Test.Format);
  EXPECT_EQ(Graph->relocations()[0].Type, Test.Type);
  EXPECT_EQ(Graph->relocations()[0].PatchSize, 4U);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  const auto &Relocation = Graph->relocations()[0];
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  const int64_t S = static_cast<int64_t>(
      Graph->sections()[Symbol.Section].Address + Symbol.Offset);
  const int64_t P = static_cast<int64_t>(
      Graph->sections()[Relocation.Section].Address + Relocation.Offset);
  auto Before =
      Internal::readSigned(Graph->sections()[Relocation.Section].Content,
                           Relocation.Offset, 4, Endianness::Little);
  ASSERT_TRUE(Before);
  ASSERT_TRUE(applyRelocations(*Graph));
  auto After =
      Internal::readSigned(Graph->sections()[Relocation.Section].Content,
                           Relocation.Offset, 4, Endianness::Little);
  ASSERT_TRUE(After);
  EXPECT_EQ(*After, S + *Before - P - 4);
}

INSTANTIATE_TEST_SUITE_P(MachO, PortableRelocationTest,
                         testing::Values(PortableRelocationCase{
                             "x86_64-apple-macosx", Target::X86_64,
                             ObjectFormat::MachO, 1}),
                         portableRelocationCaseName);
INSTANTIATE_TEST_SUITE_P(COFF, PortableRelocationTest,
                         testing::Values(PortableRelocationCase{
                             "x86_64-pc-windows-msvc", Target::X86_64,
                             ObjectFormat::COFF, 4}),
                         portableRelocationCaseName);

void expectPortableRelocationForms(
    ObjectFormat Format, Target Architecture,
    WasmEdge::Span<const std::pair<uint32_t, uint8_t>> Cases) {
  for (const auto &[Type, Width] : Cases)
    EXPECT_EQ(relocationPatchSize(Format, Architecture, Type, 4), Width);
}

TEST(RelocationTest, RecognizesObservedX86_64MachORelocationForms) {
  const std::array<std::pair<uint32_t, uint8_t>, 3> Cases{{
      {llvm::MachO::X86_64_RELOC_SIGNED, 4},
      {llvm::MachO::X86_64_RELOC_SIGNED_4, 4},
      {llvm::MachO::X86_64_RELOC_BRANCH, 4},
  }};
  expectPortableRelocationForms(ObjectFormat::MachO, Target::X86_64, Cases);
}

TEST(RelocationTest, RecognizesObservedAArch64MachORelocationForms) {
  const std::array<std::pair<uint32_t, uint8_t>, 3> Cases{{
      {llvm::MachO::ARM64_RELOC_BRANCH26, 4},
      {llvm::MachO::ARM64_RELOC_PAGE21, 4},
      {llvm::MachO::ARM64_RELOC_PAGEOFF12, 4},
  }};
  expectPortableRelocationForms(ObjectFormat::MachO, Target::AArch64, Cases);
}

TEST(RelocationTest, RecognizesObservedX86_64COFFRelocationForms) {
  const std::array<std::pair<uint32_t, uint8_t>, 2> Cases{{
      {llvm::COFF::IMAGE_REL_AMD64_REL32, 4},
      {llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 4},
  }};
  expectPortableRelocationForms(ObjectFormat::COFF, Target::X86_64, Cases);
}

TEST(RelocationTest, RecognizesObservedAArch64COFFRelocationForms) {
  const std::array<std::pair<uint32_t, uint8_t>, 5> Cases{{
      {llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 4},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21, 4},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A, 4},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L, 4},
      {llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, 4},
  }};
  expectPortableRelocationForms(ObjectFormat::COFF, Target::AArch64, Cases);
}

TEST(RelocationTest, DecodesCOFFAddr32NBImplicitAddendAsUnsigned) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  constexpr uint32_t Addend = UINT32_C(0x80000000);
  std::vector<WasmEdge::Byte> Bytes(24);
  write32le(Bytes, 1, Addend);
  auto Graph = makeRelocationGraph(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 0,
                                   true, ImageBase + 0x2000, ImageBase + 0x1000,
                                   ObjectFormat::COFF, std::move(Bytes));
  ASSERT_TRUE(Graph.setImageBase(ImageBase));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, static_cast<uint64_t>(Addend) + 0x2000);
}

TEST(RelocationTest, WrapsCOFFAddr32NBAt32Bits) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  constexpr uint32_t Addend = UINT32_C(0xFFFFF000);
  std::vector<WasmEdge::Byte> Bytes(24);
  write32le(Bytes, 1, Addend);
  auto Graph = makeRelocationGraph(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 0,
                                   true, ImageBase + 0x2000, ImageBase + 0x1000,
                                   ObjectFormat::COFF, std::move(Bytes));
  ASSERT_TRUE(Graph.setImageBase(ImageBase));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x1000U);
}

TEST(RelocationTest, AppliesCOFFAddr32NBToUniversalOffsets) {
  constexpr uint32_t Addend = UINT32_C(0xFFFFF000);
  std::vector<WasmEdge::Byte> Bytes(24);
  write32le(Bytes, 1, Addend);
  auto Graph =
      makeRelocationGraph(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 0, true, 0x2000,
                          0x1000, ObjectFormat::COFF, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x1000U);
}

TEST(RelocationTest, AppliesAArch64COFFAddr32NBFromPEBasedUniversalLayout) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("arm64-universal.obj"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}});
  auto PData = Graph.addSection(Section{".pdata", SectionKind::Unwind, 4, 8, 0,
                                        0, std::vector<WasmEdge::Byte>(8),
                                        SectionPurpose::PData});
  ASSERT_TRUE(Text && PData);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 0, 4, false});
  ASSERT_TRUE(Function);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*PData, 0, llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, *Function, 0,
                 true, ObjectFormat::COFF, 4}));
  write32le(const_cast<Section &>(Graph.sections()[*PData]).Content, 4,
            (1U << 2) | 1U);

  ASSERT_TRUE(layout(Graph, ImageBase, 4096));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(read32le(Graph.sections()[*PData].Content, 0),
            Graph.sections()[*Text].Address - ImageBase);
  auto RuntimeFunctions = normalizePERuntimeFunctions(Graph, ImageBase);
  ASSERT_TRUE(RuntimeFunctions);
  ASSERT_EQ(RuntimeFunctions->size(), 8U);
  EXPECT_EQ(read32le(*RuntimeFunctions, 0),
            Graph.sections()[*Text].Address - ImageBase);
}

TEST(RelocationTest, AcceptsX86_64MachOAbsoluteAndRejectsGOTForm) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::MachO, Target::X86_64,
                                llvm::MachO::X86_64_RELOC_UNSIGNED, 8),
            8);
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::X86_64,
                                   llvm::MachO::X86_64_RELOC_UNSIGNED, 4));
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::X86_64,
                                   llvm::MachO::X86_64_RELOC_GOT, 4));
}

TEST(RelocationTest, AcceptsAArch64MachOAbsoluteAndRejectsGOTForm) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::MachO, Target::AArch64,
                                llvm::MachO::ARM64_RELOC_UNSIGNED, 8),
            8);
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::AArch64,
                                   llvm::MachO::ARM64_RELOC_UNSIGNED, 4));
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::AArch64,
                                   llvm::MachO::ARM64_RELOC_POINTER_TO_GOT, 8));
}

TEST(RelocationTest, RejectsFourByteAArch64MachOUnsignedWithoutOverwrite) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("unsigned.o"));
  std::vector<WasmEdge::Byte> Bytes{0, 0, 0, 0, 0xAA, 0xBB, 0xCC, 0xDD};
  auto Patch = Graph.addSection(
      Section{"__data", SectionKind::Data, 4, 8, 0x1000, 0, Bytes});
  auto TargetSection =
      Graph.addSection(Section{"__target", SectionKind::Data, 8, 8, 0x2000, 0,
                               std::vector<WasmEdge::Byte>(8)});
  ASSERT_TRUE(Patch && TargetSection);
  auto Target = Graph.addSymbol(Symbol{"target", *TargetSection, 0, 8, false});
  ASSERT_TRUE(Target);
  EXPECT_FALSE(Graph.addRelocation(
      Relocation{*Patch, 0, llvm::MachO::ARM64_RELOC_UNSIGNED, *Target, 0,
                 false, ObjectFormat::MachO, 4}));
  EXPECT_EQ(Graph.sections()[*Patch].Content, Bytes);
}

TEST(RelocationTest, AcceptsX86_64COFFAbsolute) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::COFF, Target::X86_64,
                                llvm::COFF::IMAGE_REL_AMD64_ADDR64, 8),
            8);
}

TEST(RelocationTest, AcceptsAArch64COFFAbsolute) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::COFF, Target::AArch64,
                                llvm::COFF::IMAGE_REL_ARM64_ADDR64, 8),
            8);
}

TEST(RelocationTest, RejectsX86_64MachOPortablePersonalityObject) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  EXPECT_FALSE(
      ObjectReader::read(makeObject(llvm::Triple("x86_64-apple-macosx"), false,
                                    false, "f0", {}, false, false, "generic",
                                    {}, true, false, false, false, false, true),
                         Target::X86_64));
}

TEST(RelocationTest, RejectsAArch64COFFPortablePersonalityObject) {
  REQUIRE_LLVM_TARGET("aarch64-pc-windows-msvc");
  EXPECT_FALSE(ObjectReader::read(
      makeObject(llvm::Triple("aarch64-pc-windows-msvc"), false, false, "f0",
                 {}, false, false, "generic", {}, true, false, false, false,
                 false, true),
      Target::AArch64));
}

void expectRelocationFailure(LinkGraph Graph, std::string_view Message) {
  const auto Snapshot = Graph;
  auto Result = applyRelocations(Graph);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, Message);
  expectGraphStateEquals(Graph, Snapshot);
}

void moveTargetSymbolPastAddressSpace(LinkGraph &Graph) {
  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  Sections[1].Address = UINT64_MAX;
  Sections[1].VirtualSize = 2;
  const_cast<std::vector<Symbol> &>(Graph.symbols()).back().Offset = 1;
}

std::vector<WasmEdge::Byte> littleEndianWords(std::vector<uint32_t> Words,
                                              size_t Size = 16) {
  std::vector<WasmEdge::Byte> Bytes(Size);
  for (size_t I = 0; I < Words.size(); ++I) {
    EXPECT_TRUE(
        Internal::writeUnsigned(Bytes, I * 4, 4, Endianness::Little, Words[I]));
  }
  return Bytes;
}

constexpr uint64_t UnreachableAddress = UINT64_C(0x9000000000000000);

TEST(RelocationTest, RejectsOverflowingSymbolAndPlaceAddresses) {
  auto Symbol = makeELFRelocationGraph(Target::X86_64, Endianness::Little,
                                       llvm::ELF::R_X86_64_64, 8, 0x2000);
  moveTargetSymbolPastAddressSpace(Symbol);
  expectRelocationFailure(std::move(Symbol), "symbol address overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::X86_64, Endianness::Little,
                             llvm::ELF::R_X86_64_64, 8, 0x2000, UINT64_MAX, 1),
      "relocation place overflows");
}

TEST(RelocationTest, RejectsX86_64AddendAndImageRelativeOverflow) {
  expectRelocationFailure(
      makeELFRelocationGraph(
          Target::X86_64, Endianness::Little,
          llvm::MachO::X86_64_RELOC_SIGNED_1, 4, 0x2000, 0x1000, 0, INT64_MIN,
          false, std::vector<WasmEdge::Byte>(16), ObjectFormat::MachO),
      "relocation addend overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::X86_64, Endianness::Little,
                             llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 4,
                             PEImageBase + UINT64_C(0x100000000), 0x1000, 0, 0,
                             false, std::vector<WasmEdge::Byte>(16),
                             ObjectFormat::COFF),
      "absolute relocation overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::X86_64, Endianness::Little,
                             llvm::ELF::R_X86_64_64, 8, UINT64_MAX - 0xF,
                             0x1000, 0, 0x100),
      "absolute relocation overflows");
}

TEST(RelocationTest, AppliesPC32WhenAddendBringsDistantSymbolIntoRange) {
  auto Graph = makeELFRelocationGraph(
      Target::X86_64, Endianness::Little, llvm::ELF::R_X86_64_PC32, 4,
      UINT64_C(0x8000000000001000), 0x1000, 0, INT64_MIN, false,
      std::vector<WasmEdge::Byte>(16, 0xFF));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(Internal::readSigned(Graph.sections()[0].Content, 0, 4,
                                 Endianness::Little),
            0);
  EXPECT_TRUE(Graph.rebases().empty());
}

TEST(ARMRelocationTest, RejectsUnrepresentableDisplacementsAtomically) {
  expectRelocationFailure(
      makeELFRelocationGraph(Target::ARM, Endianness::Little,
                             llvm::ELF::R_ARM_ABS32, 4, UINT64_C(0x100000000)),
      "absolute relocation overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::ARM, Endianness::Little,
                             llvm::ELF::R_ARM_REL32, 4,
                             UINT64_C(0xFFFFFFFFFFFFFF00), 0x1000, 0, 0x1000),
      "relative relocation overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::ARM, Endianness::Little,
                             llvm::ELF::R_ARM_REL32, 4, UnreachableAddress),
      "relative relocation overflows");
  auto Thumb = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4,
      UnreachableAddress, 0x1000, 0, 0, false,
      littleEndianWords({UINT32_C(0xD000F000)}));
  setTargetThumb(Thumb);
  expectRelocationFailure(std::move(Thumb),
                          "Thumb branch displacement overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::ARM, Endianness::Little,
                             llvm::ELF::R_ARM_CALL, 4, UnreachableAddress,
                             0x1000, 0, 0, false,
                             littleEndianWords({UINT32_C(0xEB000000)})),
      "ARM branch displacement overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::ARM, Endianness::Little,
                             llvm::ELF::R_ARM_PREL31, 4, UnreachableAddress),
      "PREL31 relocation overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::ARM, Endianness::Little,
                             llvm::ELF::R_ARM_PREL31, 4, UINT64_C(0x100001000)),
      "PREL31 relocation overflows");
}

TEST(AArch64RelocationTest, RejectsUnrepresentableDisplacementsAtomically) {
  expectRelocationFailure(
      makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                             llvm::ELF::R_AARCH64_CALL26, 4, UnreachableAddress,
                             0x1000, 0, 0, false,
                             littleEndianWords({UINT32_C(0x94000000)})),
      "branch displacement overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(
          Target::AArch64, Endianness::Little,
          llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 4,
          UINT64_C(0x1000) + UINT64_C(0x7FFFFFFFFFFFFF00), 0x1000, 0, 0, true,
          littleEndianWords({UINT32_C(0x94000100)}), ObjectFormat::COFF),
      "branch displacement overflows");
  auto BelowImageBase = makeELFRelocationGraph(
      Target::AArch64, Endianness::Little, llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
      4, 0x2000, 0x1000, 0, 0, false, std::vector<WasmEdge::Byte>(16),
      ObjectFormat::COFF);
  ASSERT_TRUE(BelowImageBase.setImageBase(PEImageBase));
  expectRelocationFailure(std::move(BelowImageBase),
                          "ADDR32NB relocation overflows");
  expectRelocationFailure(makeELFRelocationGraph(Target::AArch64,
                                                 Endianness::Little,
                                                 llvm::ELF::R_AARCH64_PREL64, 8,
                                                 UnreachableAddress),
                          "PREL64 relocation overflows");
  expectRelocationFailure(makeELFRelocationGraph(Target::AArch64,
                                                 Endianness::Little,
                                                 llvm::ELF::R_AARCH64_PREL32, 4,
                                                 UINT64_C(0x100002000)),
                          "PREL32 relocation overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                             llvm::ELF::R_AARCH64_CALL26, 4, 0x2000, 0x1000, 2),
      "invalid relocation field");
}

TEST(S390XRelocationTest, RejectsUnrepresentablePCRelativeDistance) {
  expectRelocationFailure(makeELFRelocationGraph(Target::S390X, Endianness::Big,
                                                 llvm::ELF::R_390_PC32, 4,
                                                 UnreachableAddress),
                          "PC-relative relocation overflows");
}

LinkGraph makeRISCVPcrelPairGraph(uint32_t LowType, uint32_t LowInstruction) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  EXPECT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0,
              littleEndianWords({UINT32_C(0x00000297), LowInstruction}, 8)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  EXPECT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  EXPECT_TRUE(TargetSymbol && HighSite);
  EXPECT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  EXPECT_TRUE(Graph.addRelocation(Relocation{*Text, 4, LowType, *HighSite, 0,
                                             false, ObjectFormat::ELF, 4}));
  return Graph;
}

TEST(RISCVRelocationTest, RejectsMalformedSitesAndDistancesAtomically) {
  const auto Call =
      littleEndianWords({UINT32_C(0x00000097), UINT32_C(0x000080E7)});
  const auto Auipc = littleEndianWords({UINT32_C(0x00000297)});
  expectRelocationFailure(makeELFRelocationGraph(Target::RISCV64,
                                                 Endianness::Little,
                                                 llvm::ELF::R_RISCV_PCREL_HI20,
                                                 4, 0x2000, 0x1000, 1),
                          "invalid relocation field");
  expectRelocationFailure(
      makeELFRelocationGraph(
          Target::RISCV64, Endianness::Little, llvm::ELF::R_RISCV_PCREL_HI20, 4,
          UINT64_C(0x1000) + static_cast<uint64_t>(INT64_MAX), 0x1000, 0, 0,
          false, Auipc),
      "invalid PCREL_HI20 relocation");
  expectRelocationFailure(makeELFRelocationGraph(Target::RISCV64,
                                                 Endianness::Little,
                                                 llvm::ELF::R_RISCV_32_PCREL, 4,
                                                 UINT64_C(0x100002000)),
                          "32_PCREL relocation overflows");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::RISCV64, Endianness::Little,
                             llvm::ELF::R_RISCV_CALL, 8, 0x2000, 0x1000, 1),
      "invalid relocation field");
  expectRelocationFailure(
      makeELFRelocationGraph(Target::RISCV64, Endianness::Little,
                             llvm::ELF::R_RISCV_CALL, 8, UINT64_C(0x100002000),
                             0x1000, 0, 0, false, Call),
      "call displacement overflows");
  expectRelocationFailure(
      makeRISCVPcrelPairGraph(llvm::ELF::R_RISCV_PCREL_LO12_I,
                              UINT32_C(0x00000033)),
      "invalid PCREL_LO12_I instruction");
  expectRelocationFailure(
      makeRISCVPcrelPairGraph(llvm::ELF::R_RISCV_PCREL_LO12_S,
                              UINT32_C(0x00000013)),
      "invalid PCREL_LO12_S instruction");

  auto Difference = makeELFRelocationGraph(Target::RISCV64, Endianness::Little,
                                           llvm::ELF::R_RISCV_ADD32, 4, 0x2000);
  ASSERT_TRUE(Difference.addRelocation(Relocation{
      0, 0, llvm::ELF::R_RISCV_SUB32, 0, 0, false, ObjectFormat::ELF, 4}));
  moveTargetSymbolPastAddressSpace(Difference);
  expectRelocationFailure(std::move(Difference),
                          "invalid symbol difference relocation");
}

TEST(RelocationFieldTest, RejectsCheckedArithmeticOverflow) {
  uint64_t Unsigned = 1;
  EXPECT_TRUE(Internal::checkedMul(0, UINT64_MAX, Unsigned));
  EXPECT_EQ(Unsigned, 0U);
  EXPECT_FALSE(Internal::checkedMul(2, UINT64_MAX / 2 + 1, Unsigned));
  EXPECT_TRUE(Internal::addSigned(UINT64_C(5), INT64_C(-5), Unsigned));
  EXPECT_EQ(Unsigned, 0U);
  EXPECT_FALSE(Internal::addSigned(UINT64_C(5), INT64_C(-6), Unsigned));
  int64_t Signed = 0;
  EXPECT_FALSE(Internal::addSigned(INT64_MAX, INT64_C(1), Signed));
  EXPECT_FALSE(Internal::addSigned(INT64_MIN, INT64_C(-1), Signed));
  EXPECT_FALSE(Internal::signedDelta(UINT64_MAX, 0, Signed));
  EXPECT_FALSE(Internal::signedDelta(0, UINT64_MAX, Signed));
  ASSERT_TRUE(Internal::signedDelta(0, UINT64_C(1) << 63, Signed));
  EXPECT_EQ(Signed, INT64_MIN);
  std::array<WasmEdge::Byte, 8> Bytes{};
  EXPECT_FALSE(Internal::writeSigned(Bytes, 0, 3, Endianness::Little, 0));
  EXPECT_EQ(Bytes, (std::array<WasmEdge::Byte, 8>{}));
}

TEST(RelocationTest, UsesTheLaidOutImageBaseForAddr32NB) {
  constexpr uint64_t ImageBase = UINT64_C(0x10000);
  for (const auto &[Architecture, Type] :
       {std::pair(Target::X86_64,
                  static_cast<uint32_t>(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB)),
        std::pair(
            Target::AArch64,
            static_cast<uint32_t>(llvm::COFF::IMAGE_REL_ARM64_ADDR32NB))}) {
    LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::COFF);
    ASSERT_TRUE(Graph.beginInput("base.obj"));
    auto Text = Graph.addSection(Section{
        ".text", SectionKind::Text, 4, 4, 0, 0, {0xC3, 0x03, 0x5F, 0xD6}});
    auto Data = Graph.addSection(Section{".rdata", SectionKind::ReadOnly, 4, 4,
                                         0, 0, std::vector<WasmEdge::Byte>(4)});
    ASSERT_TRUE(Text && Data);
    auto Function = Graph.addSymbol(Symbol{"function", *Text, 0, 4, false});
    ASSERT_TRUE(Function);
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Data, 0, Type, *Function, 0, false, ObjectFormat::COFF, WordPatch}));
    ASSERT_TRUE(layout(Graph, ImageBase));
    EXPECT_EQ(Graph.imageBase(), ImageBase);
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_EQ(read32le(Graph.sections()[*Data].Content, 0),
              Graph.sections()[*Text].Address - ImageBase);
  }
}

} // namespace
