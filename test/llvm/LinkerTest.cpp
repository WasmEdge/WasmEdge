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
#include "linker/native_linker.h"
#include "linker/object_reader.h"
#include "linker/output_file.h"
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

constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
    0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
    0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};

TEST(NativeLinkerTest, AcceptsMatchingARMFloatABI) {
  EXPECT_TRUE(Internal::armFloatABICompatible(
      ObjectFormat::ELF,
      llvm::ELF::EF_ARM_EABI_VER5 | llvm::ELF::EF_ARM_ABI_FLOAT_SOFT, false));
  EXPECT_TRUE(Internal::armFloatABICompatible(
      ObjectFormat::ELF,
      llvm::ELF::EF_ARM_EABI_VER5 | llvm::ELF::EF_ARM_ABI_FLOAT_HARD, true));
}

TEST(NativeLinkerTest, RejectsMismatchedARMFloatABI) {
  EXPECT_FALSE(Internal::armFloatABICompatible(
      ObjectFormat::ELF, llvm::ELF::EF_ARM_ABI_FLOAT_HARD, false));
  EXPECT_FALSE(Internal::armFloatABICompatible(
      ObjectFormat::ELF, llvm::ELF::EF_ARM_ABI_FLOAT_SOFT, true));
}

TEST(NativeLinkerTest, RejectsMissingOrInvalidARMFloatABI) {
  EXPECT_FALSE(Internal::armFloatABICompatible(ObjectFormat::ELF, 0, false));
  EXPECT_FALSE(Internal::armFloatABICompatible(ObjectFormat::ELF, 0, true));
  constexpr uint32_t Both =
      llvm::ELF::EF_ARM_ABI_FLOAT_SOFT | llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  EXPECT_FALSE(Internal::armFloatABICompatible(ObjectFormat::ELF, Both, false));
  EXPECT_FALSE(Internal::armFloatABICompatible(ObjectFormat::ELF, Both, true));
}

TEST(NativeLinkerTest, BypassesARMFloatABIForNonELFFormats) {
  for (const auto Format : {ObjectFormat::MachO, ObjectFormat::COFF}) {
    EXPECT_TRUE(Internal::armFloatABICompatible(Format, 0, false));
    EXPECT_TRUE(Internal::armFloatABICompatible(
        Format, llvm::ELF::EF_ARM_ABI_FLOAT_HARD, false));
  }
}

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

std::vector<WasmEdge::Byte> makeUnusedNativeObject() {
  return makeObject(llvm::Triple(llvm::Triple::normalize(
                        llvm::sys::getDefaultTargetTriple())),
                    false, false, "f0", {}, false, true, "generic", {}, true,
                    false, false, false, false, false, {}, true, false, false,
                    false, true);
}

struct AOTMetadata {
  using SectionTuple =
      std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>;

  uint32_t Version;
  uint8_t OS;
  uint8_t Arch;
  uint64_t VersionAddress;
  uint64_t IntrinsicsAddress;
  std::vector<uintptr_t> Types;
  std::vector<uintptr_t> Codes;
  std::vector<SectionTuple> Sections;
};

AOTMetadata parseAOTMetadata(const std::filesystem::path &Path) {
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  WasmEdge::Loader::Loader Loader(Conf);
  auto Module = Loader.parseModule(Path);
  EXPECT_TRUE(Module);
  if (!Module) {
    return {};
  }
  const auto &AOT = (*Module)->getAOTSection();
  return {AOT.getVersion(),           AOT.getOSType(),
          AOT.getArchType(),          AOT.getVersionAddress(),
          AOT.getIntrinsicsAddress(), AOT.getTypesAddress(),
          AOT.getCodesAddress(),      AOT.getSections()};
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

TEST_F(LinkerOutputTest, UniversalWasmWriterSerializesLoaderSchema) {
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("golden.o"));
  ASSERT_TRUE(setHostELFFlags(Graph));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0x20, 0, {1, 2, 3, 4}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 3, 0x28, 4, {5, 6, 7}});
  auto Bss =
      Graph.addSection(Section{".bss", SectionKind::BSS, 16, 9, 0x30, 0, {}});
  auto Unwind = Graph.addSection(Section{".eh_frame",
                                         SectionKind::Unwind,
                                         8,
                                         2,
                                         0x40,
                                         7,
                                         {8, 9},
                                         SectionPurpose::EHFrame});
  ASSERT_TRUE(Text && Data && Bss && Unwind);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 2, 1, true}));
  ASSERT_TRUE(
      Graph.addSymbol(Symbol{"f0", *Text, 2, 1, true, {}, false, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "golden.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  WasmEdge::Loader::Loader Loader(Conf);
  auto Module = Loader.parseModule(Output);
  ASSERT_TRUE(Module);
  const auto &AOT = (*Module)->getAOTSection();
  EXPECT_EQ(AOT.getVersion(), WasmEdge::AOT::kBinaryVersion);
#if WASMEDGE_OS_LINUX
  EXPECT_EQ(AOT.getOSType(), 1U);
#elif WASMEDGE_OS_MACOS
  EXPECT_EQ(AOT.getOSType(), 2U);
#elif WASMEDGE_OS_WINDOWS
  EXPECT_EQ(AOT.getOSType(), 3U);
#endif
#if defined(__x86_64__) || defined(_M_X64)
  EXPECT_EQ(AOT.getArchType(), 1U);
#elif defined(__aarch64__) || defined(_M_ARM64)
  EXPECT_EQ(AOT.getArchType(), 2U);
#elif defined(__riscv) && __riscv_xlen == 64
  EXPECT_EQ(AOT.getArchType(), 3U);
#elif defined(__arm__) && defined(__ARM_PCS_VFP)
  EXPECT_EQ(AOT.getArchType(), 6U);
#elif defined(__arm__) || defined(_M_ARM)
  EXPECT_EQ(AOT.getArchType(), 4U);
#elif defined(__s390x__)
  EXPECT_EQ(AOT.getArchType(), 5U);
#endif
  EXPECT_EQ(AOT.getVersionAddress(), 0x29U);
  EXPECT_EQ(AOT.getIntrinsicsAddress(), 0x2AU);
  EXPECT_TRUE(AOT.getTypesAddress().empty());
  EXPECT_EQ(AOT.getCodesAddress(), (std::vector<uintptr_t>{0x23}));
  EXPECT_EQ(AOT.getSections().size(), 4U);
  EXPECT_EQ(
      AOT.getSections()[0],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          1, 0x20, 4, {1, 2, 3, 4}}));
  EXPECT_EQ(
      AOT.getSections()[1],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          2, 0x28, 3, {5, 6, 7}}));
  EXPECT_EQ(
      AOT.getSections()[2],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          3, 0x30, 9, {}}));
  EXPECT_EQ(
      AOT.getSections()[3],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          4, 0x40, 2, {8, 9}}));
}

TEST_F(LinkerOutputTest, UniversalWasmWriterMergesOnlySmallSameKindGaps) {
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("merge.o"));
  ASSERT_TRUE(setHostELFFlags(Graph));
  ASSERT_TRUE(Graph.addSection(
      Section{".text.a", SectionKind::Text, 1, 2, 0x10, 0, {1, 2}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".text.b", SectionKind::Text, 8, 2, 0x18, 2, {3, 4}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".data.a", SectionKind::Data, 1, 2, 0x1000, 4, {5, 6}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".data.b", SectionKind::Data, 4, 1, 0x1004, 6, {7}}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", 2, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", 3, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "merged.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  ASSERT_EQ(Metadata.Sections.size(), 3U);
  EXPECT_EQ(Metadata.Sections[0],
            (decltype(Metadata.Sections)::value_type{1, 0x10, 2, {1, 2}}));
  EXPECT_EQ(Metadata.Sections[1],
            (decltype(Metadata.Sections)::value_type{1, 0x18, 2, {3, 4}}));
  EXPECT_EQ(Metadata.Sections[2], (decltype(Metadata.Sections)::value_type{
                                      2, 0x1000, 5, {5, 6, 0, 0, 7}}));
}

TEST_F(LinkerOutputTest, UniversalWasmWriterClassifiesUnwindByPurpose) {
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("unwind-kinds.o"));
  ASSERT_TRUE(setHostELFFlags(Graph));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x1000, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(Section{".ARM.exidx",
                                       SectionKind::Unwind,
                                       4,
                                       4,
                                       0x2000,
                                       0,
                                       {1, 2, 3, 4},
                                       SectionPurpose::ARMExidx}));
  ASSERT_TRUE(Graph.addSection(Section{".eh_frame",
                                       SectionKind::Unwind,
                                       4,
                                       4,
                                       0x3000,
                                       0,
                                       {0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Graph.addSection(
      Section{".unwind", SectionKind::Unwind, 4, 4, 0x4000, 0, {5, 6, 7, 8}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "unwind-kinds.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto Exidx = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        const auto Address = std::get<1>(Value);
        const auto &Content = std::get<3>(Value);
        return Address <= 0x2000 && 0x2004 <= Address + Content.size() &&
               std::equal(Content.begin() +
                              static_cast<std::ptrdiff_t>(0x2000 - Address),
                          Content.begin() +
                              static_cast<std::ptrdiff_t>(0x2004 - Address),
                          std::array<WasmEdge::Byte, 4>{1, 2, 3, 4}.begin());
      });
  const auto EHFrame = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) { return std::get<1>(Value) == 0x3000; });
  const auto Generic = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        const auto Address = std::get<1>(Value);
        const auto &Content = std::get<3>(Value);
        return Address <= 0x4000 && 0x4004 <= Address + Content.size() &&
               std::equal(Content.begin() +
                              static_cast<std::ptrdiff_t>(0x4000 - Address),
                          Content.begin() +
                              static_cast<std::ptrdiff_t>(0x4004 - Address),
                          std::array<WasmEdge::Byte, 4>{5, 6, 7, 8}.begin());
      });
  ASSERT_NE(Exidx, Metadata.Sections.end());
  ASSERT_NE(EHFrame, Metadata.Sections.end());
  ASSERT_NE(Generic, Metadata.Sections.end());
  EXPECT_EQ(std::get<0>(*Exidx), 2U);
  EXPECT_EQ(std::get<0>(*EHFrame), 4U);
  EXPECT_EQ(std::get<0>(*Generic), 2U);
}

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

TEST_F(LinkerOutputTest,
       UniversalWasmWriterKeepsCOFFPDataOutsideMergedSections) {
  REQUIRE_NATIVE_TARGET(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("straddled-pdata.obj"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 16, 64, 0x100, 0,
                               std::vector<WasmEdge::Byte>(64)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 2, 0x200, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".pdata", SectionKind::Unwind, 4, 12, 0x400, 0,
              std::vector<WasmEdge::Byte>(12), SectionPurpose::PData}));
  ASSERT_TRUE(Graph.addSection(Section{".xdata", SectionKind::Unwind, 4, 8,
                                       0x600, 0, std::vector<WasmEdge::Byte>(8),
                                       SectionPurpose::XData}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  write32le(const_cast<Section &>(Graph.sections()[2]).Content, 0, 0x100);
  write32le(const_cast<Section &>(Graph.sections()[2]).Content, 4, 0x110);
  write32le(const_cast<Section &>(Graph.sections()[2]).Content, 8, 0x600);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "straddled-pdata.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; });
  ASSERT_NE(PData, Metadata.Sections.end());
  EXPECT_EQ(std::get<1>(*PData), 0x400U);
  EXPECT_TRUE(std::none_of(Metadata.Sections.begin(), Metadata.Sections.end(),
                           [](const auto &Value) {
                             return std::get<0>(Value) != 4 &&
                                    std::get<1>(Value) <= 0x400 &&
                                    0x400 <
                                        std::get<1>(Value) + std::get<2>(Value);
                           }));
}

TEST_F(LinkerOutputTest, UniversalWasmWriterCompactsAndSortsCOFFPData) {
  REQUIRE_NATIVE_TARGET(Target::X86_64);
  auto Graph = makeUniversalCOFFPDataGraph();
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 0, 0x120);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 4, 0x130);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 8, 0x304);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 0, 0x100);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 4, 0x110);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 8, 0x300);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "sorted-pdata.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; });
  ASSERT_NE(PData, Metadata.Sections.end());
  EXPECT_EQ(std::get<1>(*PData), 0x400U);
  EXPECT_EQ(std::get<2>(*PData), 24U);
  ASSERT_EQ(std::get<3>(*PData).size(), 24U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 0), 0x100U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 12), 0x120U);
  const auto XData = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        return std::get<1>(Value) <= 0x300 &&
               0x308 <= std::get<1>(Value) + std::get<3>(Value).size();
      });
  ASSERT_NE(XData, Metadata.Sections.end());
  EXPECT_EQ(std::get<0>(*XData), 2U);
}

TEST_F(LinkerOutputTest,
       UniversalWasmWriterPlacesCOFFPDataAtLowestAddressWhenInsertedReverse) {
  REQUIRE_NATIVE_TARGET(Target::X86_64);
  auto Graph = makeUniversalCOFFPDataGraph();
  auto &First = const_cast<Section &>(Graph.sections()[3]);
  auto &Second = const_cast<Section &>(Graph.sections()[4]);
  std::swap(First.Address, Second.Address);
  write32le(First.Content, 0, 0x120);
  write32le(First.Content, 4, 0x130);
  write32le(First.Content, 8, 0x304);
  write32le(Second.Content, 0, 0x100);
  write32le(Second.Content, 4, 0x110);
  write32le(Second.Content, 8, 0x300);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "reverse-pdata.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; });
  ASSERT_NE(PData, Metadata.Sections.end());
  EXPECT_EQ(std::get<1>(*PData), 0x400U);
  EXPECT_EQ(std::get<2>(*PData), 24U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 0), 0x100U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 12), 0x120U);
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

TEST_F(LinkerOutputTest,
       UniversalWasmWriterDoesNotNormalizeNonPDataCOFFUnwind) {
  REQUIRE_NATIVE_TARGET(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("non-pdata.obj"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x100, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(Section{".xdata",
                                       SectionKind::Unwind,
                                       1,
                                       3,
                                       0x200,
                                       0,
                                       {1, 2, 3},
                                       SectionPurpose::XData}));
  ASSERT_TRUE(Graph.addSection(Section{".eh_frame",
                                       SectionKind::Unwind,
                                       1,
                                       3,
                                       0x300,
                                       0,
                                       {4, 5, 6},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "non-pdata.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  EXPECT_TRUE(
      std::none_of(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; }));
  const auto HasDataSection = [&Metadata](uint64_t Address,
                                          std::vector<WasmEdge::Byte> Bytes) {
    return std::find(Metadata.Sections.begin(), Metadata.Sections.end(),
                     decltype(Metadata.Sections)::value_type{
                         2, Address, Bytes.size(), Bytes}) !=
           Metadata.Sections.end();
  };
  EXPECT_TRUE(HasDataSection(0x200, {1, 2, 3}));
  EXPECT_TRUE(HasDataSection(0x300, {4, 5, 6}));
}

TEST_F(LinkerOutputTest, UniversalWasmWriterTreatsELFPDataAsOrdinaryData) {
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("ordinary-pdata.o"));
  ASSERT_TRUE(setHostELFFlags(Graph));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x100, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".pdata", SectionKind::ReadOnly, 1, 4, 0x200, 0, {1, 2, 3, 4}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "ordinary-pdata.wasm";

  ASSERT_TRUE(writeUniversalWasmFile(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  EXPECT_TRUE(
      std::none_of(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; }));
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) {
                     const auto Address = std::get<1>(Value);
                     const auto &Content = std::get<3>(Value);
                     return std::get<0>(Value) == 2 && Address <= 0x200 &&
                            0x204 <= Address + Content.size();
                   });
  ASSERT_NE(PData, Metadata.Sections.end());
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

TEST_F(LinkerOutputTest,
       UniversalWasmWriterPreservesDestinationOnValidationFailure) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  constexpr std::array<WasmEdge::Byte, 8> InvalidWasm{0x01, 0x61, 0x73, 0x6D,
                                                      0x01, 0x00, 0x00, 0x00};
  const std::vector<WasmEdge::Byte> Original{9, 8, 7, 6};
  const auto Output = Directory / "preserved.wasm";
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("invalid-graph.o"));

  for (const auto Wasm : {WasmEdge::Span<const WasmEdge::Byte>(InvalidWasm),
                          WasmEdge::Span<const WasmEdge::Byte>(EmptyWasm)}) {
    {
      std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
      ASSERT_TRUE(File);
      File.write(reinterpret_cast<const char *>(Original.data()),
                 static_cast<std::streamsize>(Original.size()));
    }
    EXPECT_FALSE(writeUniversalWasmFile(Graph, Wasm, Output));
    EXPECT_EQ(readFile(Output), Original);
  }
}

TEST_F(LinkerOutputTest,
       UniversalWasmWriterPropagatesOpenFailureAfterValidation) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("valid.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data =
      Graph.addSection(Section{".data", SectionKind::Data, 1, 2, 1, 1, {1, 2}});
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "missing" / "output.wasm";

  EXPECT_FALSE(writeUniversalWasmFile(Graph, EmptyWasm, Output));
  EXPECT_FALSE(std::filesystem::exists(Output));
}

TEST_F(LinkerOutputTest, UniversalCodegenExecutesTinyFixture) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      WasmEdge::CompilerConfigure::OutputFormat::Wasm);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator Validator(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  WasmEdge::VM::VM VM(Conf);
  auto Module = Loader.parseModule(TinyWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(Validator.validate(**Module));
  auto Data = Compiler.compile(**Module);
  ASSERT_TRUE(Data);
  const auto Output = Directory / "execute.wasm";

  ASSERT_TRUE(CodeGen.codegen(TinyWasm, std::move(*Data), Output));
  ASSERT_TRUE(VM.loadWasm(Output));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto Result = VM.execute("f");
  ASSERT_TRUE(Result);
  ASSERT_EQ(Result->size(), 1U);
  EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 7U);
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsUnsupportedOutputAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Existing{1, 2, 3};
  {
    std::ofstream File(Output, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Existing.data()),
               Existing.size());
  }
  const auto Object = makeNativeObject();
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};

#if WASMEDGE_OS_LINUX
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::MachO));
#else
  EXPECT_FALSE(NativeLinker::link(Object, EmptyWasm, Output, OutputKind::ELF));
#endif
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Existing.begin(), Existing.end())));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, ReportsRealObjectDiagnosticOnceAtBoundary) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Object = makeNativeObject(true);
#if WASMEDGE_OS_MACOS
  const std::string UndefinedName = "_value";
#else
  const std::string UndefinedName = "value";
#endif
  auto ReadResult = ObjectReader::read(Object, nativeTarget());
  ASSERT_FALSE(ReadResult);
  EXPECT_EQ(ReadResult.error().Message, "undefined symbol");
  EXPECT_EQ(ReadResult.error().SymbolName, UndefinedName);
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result =
      NativeLinker::link(Object, EmptyWasm, Directory / "diagnostic.wasm",
                         OutputKind::UniversalWasm);

  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MalformedSection);
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_NE(Logs[0].find("native linker: undefined symbol"), std::string::npos);
  EXPECT_NE(Logs[0].find("symbol name='" + UndefinedName + "'"),
            std::string::npos);
}

#if defined(__arm__)
TEST_F(LinkerOutputTest, LoaderRejectsUniversalWasmFromOtherARMFloatABI) {
  using WasmEdge::AOT::Architecture;
  const auto Output = Directory / "float-abi.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_FALSE(parseAOTMetadata(Output).Sections.empty());

  auto Bytes = readFile(Output);
  const std::array<WasmEdge::Byte, 11> Header{
      8,
      'w',
      'a',
      's',
      'm',
      'e',
      'd',
      'g',
      'e',
      0x03,
      static_cast<WasmEdge::Byte>(WasmEdge::AOT::kHostOSType)};
  const auto Found =
      std::search(Bytes.begin(), Bytes.end(), Header.begin(), Header.end());
  ASSERT_NE(Found, Bytes.end());
  const auto Arch = Found + static_cast<std::ptrdiff_t>(Header.size());
  ASSERT_NE(Arch, Bytes.end());
  ASSERT_EQ(*Arch,
            static_cast<WasmEdge::Byte>(WasmEdge::AOT::kHostArchitecture));
  *Arch = static_cast<WasmEdge::Byte>(WasmEdge::AOT::kHostArchitecture ==
                                              Architecture::ARMv7HardFloat
                                          ? Architecture::ARMv7
                                          : Architecture::ARMv7HardFloat);
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Bytes.data()),
               static_cast<std::streamsize>(Bytes.size()));
  }
  EXPECT_TRUE(parseAOTMetadata(Output).Sections.empty());
}
#endif

TEST_F(LinkerOutputTest, NativeLinkerRejectsLibraryWithoutLoaderExports) {
#if WASMEDGE_OS_LINUX
  const auto Output = Directory / "missing-exports.so";
  constexpr auto Kind = OutputKind::ELF;
#elif WASMEDGE_OS_MACOS
  const auto Output = Directory / "missing-exports.dylib";
  constexpr auto Kind = OutputKind::MachO;
#else
  const auto Output = Directory / "missing-exports.dll";
  constexpr auto Kind = OutputKind::PE;
#endif
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result = NativeLinker::link(Object, TinyWasm, Output, Kind);

  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);
  ASSERT_FALSE(Result);
  EXPECT_FALSE(std::filesystem::exists(Output));
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_EQ(Logs[0].rfind("native linker: object does not export ", 0), 0U)
      << Logs[0];
  expectNoTemporaryFiles();
}

#if WASMEDGE_OS_LINUX && (defined(__x86_64__) || defined(_M_X64))
TEST_F(LinkerOutputTest, ReportsRealRelocationFailureOnceAtBoundary) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  auto Object = makeNativeObject();
  const auto Header = elf64RelocationSectionHeader(Object);
  const auto RelocationOffset = read64le(Object, Header + 24);
  write64le(Object, RelocationOffset + 16, static_cast<uint64_t>(INT64_MAX));
  auto ReadResult = ObjectReader::read(Object, nativeTarget());
  ASSERT_TRUE(ReadResult);
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result = NativeLinker::link(
      Object, EmptyWasm, Directory / "relocation-diagnostic.wasm",
      OutputKind::UniversalWasm);

  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::AOTNotImpl);
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_NE(Logs[0].find("native linker:"), std::string::npos);
  EXPECT_NE(Logs[0].find("unsupported"), std::string::npos);
  EXPECT_NE(Logs[0].find("section="), std::string::npos);
  EXPECT_NE(Logs[0].find("symbol="), std::string::npos);
  EXPECT_NE(Logs[0].find("relocation="), std::string::npos);
  EXPECT_NE(Logs[0].find("offset="), std::string::npos);
}
#endif

#if WASMEDGE_OS_LINUX
TEST_F(LinkerOutputTest, NativeAOTWriterLoadsAndExecutesWithoutImports) {
  const auto Output = Directory / "native.so";
  const auto SecondOutput = Directory / "native-second.so";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.so", true);
  ASSERT_FALSE(std::filesystem::exists(Directory / "seed.so"));

  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Output, OutputKind::ELF));
  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, SecondOutput, OutputKind::ELF));
  EXPECT_EQ(readFile(Output), readFile(SecondOutput));
  auto Image = llvm::object::ObjectFile::createObjectFile(Output.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *ELF =
      llvm::dyn_cast<llvm::object::ELFObjectFileBase>(Image->getBinary());
  ASSERT_NE(ELF, nullptr);
  std::set<std::string> DynamicSymbols;
  for (const auto &Symbol : ELF->getDynamicSymbolIterators()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
#if LLVM_VERSION_MAJOR < 11
    if (Name->empty())
      continue;
#endif
    if (!Name->empty())
      DynamicSymbols.emplace(Name->str());
    llvm::Expected<uint32_t> Flags = Symbol.getFlags();
    ASSERT_TRUE(static_cast<bool>(Flags));
    EXPECT_EQ(*Flags & llvm::object::SymbolRef::SF_Undefined, 0U);
  }
  for (const char *Name :
       {"f0", "version", "intrinsics", "wasm.code", "wasm.size"})
    EXPECT_TRUE(DynamicSymbols.count(Name)) << Name;

  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  EXPECT_TRUE(Library->get<uint32_t>("version"));
  EXPECT_TRUE(Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
      "intrinsics"));
  EXPECT_TRUE(Library->get<uint8_t>("wasm.code"));
  EXPECT_TRUE(Library->get<uint32_t>("wasm.size"));
  EXPECT_EQ(execute(Output), 7U);
}
#endif

#if WASMEDGE_OS_MACOS
TEST_F(LinkerOutputTest, ReportsFailedMachOLinkerStage) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Output = Directory / "failed.dylib";
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result =
      NativeLinker::link({}, EmptyWasm, Output, OutputKind::MachO);
  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MalformedSection);
  EXPECT_FALSE(std::filesystem::exists(Output));
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_EQ(Logs[0], "native linker: empty object buffer");
}

TEST_F(LinkerOutputTest, NativeMachOWriterSupportsEmptyModule) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Output = Directory / "empty.dylib";
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      WasmEdge::CompilerConfigure::OutputFormat::Native);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator Validator(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Module = Loader.parseModule(EmptyWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(Validator.validate(**Module));
  auto Data = Compiler.compile(**Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(EmptyWasm, std::move(*Data), Output));

  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  EXPECT_TRUE(Library->get<uint32_t>("version"));
  EXPECT_TRUE(Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
      "intrinsics"));
  EXPECT_TRUE(Library->get<uint8_t>("wasm.code"));
  EXPECT_TRUE(Library->get<uint32_t>("wasm.size"));
}

TEST_F(LinkerOutputTest, NativeMachOWriterLoadsAndExecutesSignedLibrary) {
  const auto Output = Directory / "native.dylib";
  const auto Object =
      compileTinyObject(TinyWasm, Directory / "seed.wasm", true);
  auto Input = ObjectReader::read(Object, nativeTarget());
  ASSERT_TRUE(Input);
  const bool HasCompact = !Input->compactUnwind().empty();
  const bool NeedsDwarf =
      std::any_of(Input->compactUnwind().begin(), Input->compactUnwind().end(),
                  [](const auto &Record) { return Record.FDE.has_value(); });
  const bool HasDwarfReferences = !Input->ehFrameReferences().empty();
  const bool HasInputEHFrame =
      std::any_of(Input->sections().begin(), Input->sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::EHFrame;
                  });
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Output, OutputKind::MachO));

  auto Image = llvm::object::ObjectFile::createObjectFile(Output.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *MachO =
      llvm::dyn_cast<llvm::object::MachOObjectFile>(Image->getBinary());
  ASSERT_NE(MachO, nullptr);
  std::set<std::string> Symbols;
  std::set<std::string> Sections;
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    Sections.emplace(Name->str());
  }
  EXPECT_EQ(Sections.count("__unwind_info") != 0, HasCompact);
  EXPECT_EQ(Sections.count("__eh_frame") != 0,
            HasInputEHFrame &&
                (!HasCompact || NeedsDwarf || HasDwarfReferences));
  EXPECT_FALSE(Sections.count("__compact_unwind"));
  for (const auto &Symbol : MachO->symbols()) {
    auto Name = Symbol.getName();
    llvm::Expected<uint32_t> Flags = Symbol.getFlags();
    ASSERT_TRUE(Name && Flags);
    EXPECT_EQ(*Flags & llvm::object::SymbolRef::SF_Undefined, 0U);
    Symbols.emplace(Name->str());
  }
  for (const char *Name :
       {"_f0", "_version", "_intrinsics", "_wasm.code", "_wasm.size"})
    EXPECT_TRUE(Symbols.count(Name)) << Name;
  const auto Bytes = readFile(Output);
  size_t Command = sizeof(llvm::MachO::mach_header_64);
  bool HasDyldInfo = false;
  auto Read32 = [&](size_t Offset) { return read32le(Bytes, Offset); };
  for (uint32_t I = 0; I < Read32(16); ++I) {
    const uint32_t Type = Read32(Command);
    const uint32_t Size = Read32(Command + 4);
    ASSERT_GE(Size, 8U);
    if (Type == llvm::MachO::LC_DYLD_INFO_ONLY) {
      HasDyldInfo = true;
      for (const size_t Offset : {size_t{16}, size_t{20}, size_t{24},
                                  size_t{28}, size_t{32}, size_t{36}})
        EXPECT_EQ(Read32(Command + Offset), 0U);
    }
    Command += Size;
  }
  EXPECT_TRUE(HasDyldInfo);

  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  {
    auto Version = Library->get<uint32_t>("version");
    auto Intrinsics =
        Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
            "intrinsics");
    auto F0 = Library->get<WasmEdge::Executable::Wrapper>("f0");
    auto WasmCode = Library->get<uint8_t>("wasm.code");
    auto WasmSize = Library->get<uint32_t>("wasm.size");
    EXPECT_TRUE(Version);
    EXPECT_TRUE(Intrinsics);
    EXPECT_TRUE(F0);
    EXPECT_TRUE(WasmCode);
    EXPECT_TRUE(WasmSize);
  }
  EXPECT_EQ(execute(Output), 7U);
  Library->unload();
  EXPECT_FALSE(Library->get<uint32_t>("version"));
}
#endif

#if WASMEDGE_OS_WINDOWS
TEST_F(LinkerOutputTest, NativePEWriterLoadsAndExecutesWithoutImports) {
  const auto Output = Directory / "native.dll";
  const auto Object =
      compileTinyObject(TinyWasm, Directory / "seed.wasm", true);
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Output, OutputKind::PE));
  auto Image = llvm::object::ObjectFile::createObjectFile(Output.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *PE =
      llvm::dyn_cast<llvm::object::COFFObjectFile>(Image->getBinary());
  ASSERT_NE(PE, nullptr);
  EXPECT_EQ(PE->import_directory_begin(), PE->import_directory_end());
  EXPECT_EQ(pe32PlusHeader(*PE)->AddressOfEntryPoint, 0U);
  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  EXPECT_TRUE(Library->get<uint32_t>("version"));
  EXPECT_TRUE(Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
      "intrinsics"));
  EXPECT_TRUE(Library->get<WasmEdge::Executable::Wrapper>("f0"));
  EXPECT_TRUE(Library->get<uint8_t>("wasm.code"));
  EXPECT_TRUE(Library->get<uint32_t>("wasm.size"));
  EXPECT_EQ(execute(Output), 7U);
}
#endif

TEST_F(LinkerOutputTest, NativeLinkerCreatesNoNativeTemporary) {
  const auto Output = Directory / "output.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  ASSERT_TRUE(std::filesystem::is_regular_file(Output));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerKeepsPercentSignsInOutputPath) {
  const auto Parent = Directory / "100%";
  ASSERT_TRUE(std::filesystem::create_directory(Parent));
  const auto Output = Parent / "a%%b.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  ASSERT_TRUE(std::filesystem::is_regular_file(Output));
  std::vector<std::filesystem::path> Entries;
  for (const auto &Entry : std::filesystem::directory_iterator(Parent))
    Entries.push_back(Entry.path().filename());
  EXPECT_EQ(Entries, std::vector<std::filesystem::path>{"a%%b.wasm"});
}

TEST_F(LinkerOutputTest, NativeLinkerReplacesExistingOutputAtomically) {
  const auto Output = Directory / "replace.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const std::array<WasmEdge::Byte, 4> Sentinel{1, 2, 3, 4};
  {
    std::ofstream File(Output, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
#if !WASMEDGE_OS_WINDOWS
  constexpr std::filesystem::perms SentinelMode =
      std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
      std::filesystem::perms::group_read;
  std::filesystem::permissions(Output, SentinelMode,
                               std::filesystem::perm_options::replace);
#endif

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_NE(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  EXPECT_EQ(execute(Output), 7U);
#if !WASMEDGE_OS_WINDOWS
  EXPECT_EQ(std::filesystem::status(Output).permissions() &
                std::filesystem::perms::mask,
            SentinelMode);
#endif
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerOverwritesHardLinkedUniversalWasm) {
  const auto Output = Directory / "output.wasm";
  const auto Alias = Directory / "alias.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  {
    std::ofstream File(Output, std::ios_base::binary);
    File.write("sentinel-longer-than-nothing", 28);
    ASSERT_TRUE(File);
  }
  std::filesystem::create_hard_link(Output, Alias);

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(std::filesystem::hard_link_count(Output), 2U);
  EXPECT_EQ(readFile(Alias), readFile(Output));
  EXPECT_EQ(execute(Alias), 7U);
  expectNoTemporaryFiles();
}

#if !WASMEDGE_OS_WINDOWS
TEST_F(LinkerOutputTest, NativeLinkerWritesThroughOutputSymlink) {
  const auto Target = Directory / "target.wasm";
  const auto Link = Directory / "link.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  {
    std::ofstream File(Target, std::ios_base::binary);
    File.put(1);
    ASSERT_TRUE(File);
  }
  std::filesystem::create_symlink("target.wasm", Link);

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Link, OutputKind::UniversalWasm));
  EXPECT_TRUE(std::filesystem::is_symlink(Link));
  EXPECT_EQ(std::filesystem::read_symlink(Link), "target.wasm");
  EXPECT_EQ(execute(Target), 7U);
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerCreatesMissingTargetOfSymlinkChain) {
  const auto First = Directory / "first.wasm";
  const auto Second = Directory / "second.wasm";
  const auto Target = Directory / "target.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  std::filesystem::create_symlink("second.wasm", First);
  std::filesystem::create_symlink("target.wasm", Second);

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, First, OutputKind::UniversalWasm));
  EXPECT_EQ(std::filesystem::read_symlink(First), "second.wasm");
  EXPECT_EQ(std::filesystem::read_symlink(Second), "target.wasm");
  EXPECT_TRUE(std::filesystem::is_regular_file(
      std::filesystem::symlink_status(Target)));
  EXPECT_EQ(execute(Target), 7U);
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerRefusesReadOnlyOutput) {
  if (::geteuid() == 0)
    GTEST_SKIP() << "root bypasses file permissions";
  const auto Output = Directory / "readonly.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const std::vector<WasmEdge::Byte> Sentinel{1, 2, 3, 4};
  {
    std::ofstream File(Output, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               static_cast<std::streamsize>(Sentinel.size()));
    ASSERT_TRUE(File);
  }
  std::filesystem::permissions(Output, std::filesystem::perms::owner_read,
                               std::filesystem::perm_options::replace);

  EXPECT_FALSE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output), Sentinel);
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerWritesSpecialOutputInPlace) {
  const std::filesystem::path Output = "/dev/null";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_TRUE(std::filesystem::is_character_file(Output));
}

#if WASMEDGE_OS_MACOS
TEST_F(LinkerOutputTest, NativeLinkerRejectsUnsignableMachOOutput) {
  const auto Object =
      compileTinyObject(TinyWasm, Directory / "seed.wasm", true);

  EXPECT_FALSE(
      NativeLinker::link(Object, TinyWasm, "/dev/null", OutputKind::MachO));
  expectNoTemporaryFiles();
}
#endif
#endif

TEST_F(LinkerOutputTest, NativeLinkerRejectsBadObjectsAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Existing{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  for (const auto &Object :
       {std::vector<WasmEdge::Byte>{0, 1, 2}, makeNativeObject(true)}) {
    {
      std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
      File.write(reinterpret_cast<const char *>(Existing.data()),
                 Existing.size());
    }
    EXPECT_FALSE(NativeLinker::link(Object, EmptyWasm, Output,
                                    OutputKind::UniversalWasm));
    EXPECT_EQ(readFile(Output),
              (std::vector<WasmEdge::Byte>(Existing.begin(), Existing.end())));
    expectNoTemporaryFiles();
  }
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsWindowsCOFFAtomically) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto COFF = makeAssemblyObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                       ".text\nretq\n");
  auto Concatenated = COFF;
  Concatenated.insert(Concatenated.end(), COFF.begin(), COFF.end());
  std::vector<std::vector<WasmEdge::Byte>> Invalid;
#if !WASMEDGE_OS_WINDOWS
  Invalid.push_back(COFF);
#endif
  Invalid.push_back(Concatenated);

  for (const auto &Object : Invalid) {
    {
      std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
      File.write(reinterpret_cast<const char *>(Sentinel.data()),
                 Sentinel.size());
      ASSERT_TRUE(File);
    }
    EXPECT_FALSE(NativeLinker::link(Object, EmptyWasm, Output,
                                    OutputKind::UniversalWasm));
    EXPECT_EQ(readFile(Output),
              (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
    expectNoTemporaryFiles();
  }
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsLinuxELFOnWindowsAtomically) {
#if WASMEDGE_OS_WINDOWS
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto ELF = makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                                      ".text\nretq\n");
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(
      NativeLinker::link(ELF, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
#else
  GTEST_SKIP() << "Linux ELF is the host object format";
#endif
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsArchiveAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const std::string ArchiveMagic = "!<arch>\n";
  const std::vector<WasmEdge::Byte> Archive(ArchiveMagic.begin(),
                                            ArchiveMagic.end());
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(NativeLinker::link(Archive, EmptyWasm, Output,
                                  OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsUndefinedHostObjectAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Object =
      makeSemanticObject(llvm::Triple(llvm::Triple::normalize(
                             llvm::sys::getDefaultTargetTriple())),
                         true);
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, RetainsUnusedAllocatableSections) {
  const auto Object = makeUnusedNativeObject();
  auto Graph = ObjectReader::read(Object, nativeTarget());
  ASSERT_TRUE(Graph);
  SectionId UnusedTextSection = InvalidSectionId;
  SectionId UnusedDataSection = InvalidSectionId;
  uint64_t UnusedDataOffset = 0;
  uint64_t UnusedDataSize = 0;
  for (const auto &Symbol : Graph->symbols()) {
    if (!Symbol.Exported && !Symbol.Global)
      continue;
    if (Symbol.Name == "unused_function" || Symbol.Name == "_unused_function")
      UnusedTextSection = Symbol.Section;
    if (Symbol.Name == "unused_data" || Symbol.Name == "_unused_data") {
      UnusedDataSection = Symbol.Section;
      UnusedDataOffset = Symbol.Offset;
      UnusedDataSize = Symbol.Size;
    }
  }
  ASSERT_LT(UnusedTextSection, Graph->sections().size());
  ASSERT_LT(UnusedDataSection, Graph->sections().size());
  const auto &UnusedText = Graph->sections()[UnusedTextSection].Content;
  const std::vector<WasmEdge::Byte> UnusedData{0x67, 0x63, 0x6F, 0x6E};
  EXPECT_EQ(Graph->sections()[UnusedTextSection].Kind, SectionKind::Text);
  EXPECT_TRUE(Graph->sections()[UnusedDataSection].Kind == SectionKind::Data ||
              Graph->sections()[UnusedDataSection].Kind ==
                  SectionKind::ReadOnly);
  ASSERT_FALSE(UnusedText.empty());
  auto Contains = [](const auto &Bytes, const auto &Marker) {
    return std::search(Bytes.begin(), Bytes.end(), Marker.begin(),
                       Marker.end()) != Bytes.end();
  };
  const auto &UnusedDataContent = Graph->sections()[UnusedDataSection].Content;
  ASSERT_EQ(UnusedDataSize, UnusedData.size());
  ASSERT_LE(UnusedDataOffset, UnusedDataContent.size());
  ASSERT_LE(UnusedDataSize, UnusedDataContent.size() - UnusedDataOffset);
  using Difference = std::vector<WasmEdge::Byte>::difference_type;
  ASSERT_LE(UnusedDataOffset,
            static_cast<uint64_t>(std::numeric_limits<Difference>::max()));
  EXPECT_TRUE(std::equal(UnusedData.begin(), UnusedData.end(),
                         UnusedDataContent.begin() +
                             static_cast<Difference>(UnusedDataOffset)));

  if (Graph->format() == ObjectFormat::MachO) {
    ASSERT_TRUE(compactUnwindToEHFrame(*Graph));
    ASSERT_TRUE(layout(*Graph, 0, 0x4000));
    EXPECT_TRUE(normalizeMachOEHFrame(*Graph));
    EXPECT_TRUE(validateMachOEHFrameCoverage(*Graph));
  }

  const auto Universal = Directory / "retained.wasm";
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Universal,
                                 OutputKind::UniversalWasm));
  const auto Metadata = parseAOTMetadata(Universal);
  bool UniversalText = false;
  bool UniversalData = false;
  for (const auto &Section : Metadata.Sections) {
    const auto &Content = std::get<3>(Section);
    UniversalText |= Contains(Content, UnusedText);
    UniversalData |= Contains(Content, UnusedData);
  }
  EXPECT_TRUE(UniversalText);
  EXPECT_TRUE(UniversalData);

#if WASMEDGE_OS_LINUX
  const auto Native = Directory / "retained.so";
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Native, OutputKind::ELF));
  auto Image = llvm::object::ObjectFile::createObjectFile(Native.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  bool NativeText = false;
  bool NativeData = false;
  for (const auto &Section : Image->getBinary()->sections()) {
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    const std::vector<WasmEdge::Byte> Bytes(Content->bytes_begin(),
                                            Content->bytes_end());
    const llvm::object::ELFSectionRef ELFSection(Section);
    NativeText |= Contains(Bytes, UnusedText) &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_ALLOC) != 0 &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_EXECINSTR) != 0;
    NativeData |= Contains(Bytes, UnusedData) &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_ALLOC) != 0 &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_WRITE) != 0;
  }
  EXPECT_TRUE(NativeText);
  EXPECT_TRUE(NativeData);
#endif
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsInvalidWasmFramingAtomically) {
  const auto Output = Directory / "malformed.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const std::array<std::vector<WasmEdge::Byte>, 5> Invalid{{
      {0x01, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x02, 0x00, 0x00, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x80, 0x80, 0x80,
       0x80, 0x10},
  }};
  for (const auto &Wasm : Invalid) {
    EXPECT_FALSE(
        NativeLinker::link(Object, Wasm, Output, OutputKind::UniversalWasm));
    EXPECT_FALSE(std::filesystem::exists(Output));
    expectNoTemporaryFiles();
  }
}

TEST_F(LinkerOutputTest, NativeLinkerDoesNotSemanticallyReparseWasm) {
  constexpr std::array<WasmEdge::Byte, 14> DuplicateTypeSections{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00,
      0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00};
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const auto Output = Directory / "validated-input.wasm";

  ASSERT_TRUE(NativeLinker::link(Object, DuplicateTypeSections, Output,
                                 OutputKind::UniversalWasm));
  const auto Bytes = readFile(Output);
  ASSERT_GE(Bytes.size(), DuplicateTypeSections.size());
  EXPECT_TRUE(std::equal(DuplicateTypeSections.begin(),
                         DuplicateTypeSections.end(), Bytes.begin()));
}

TEST_F(LinkerOutputTest,
       NativeLinkerPublishesConcurrentlyWithoutTempSurvivors) {
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  std::promise<void> Start;
  const auto Ready = Start.get_future().share();
  auto Link = [&](const char *Name) {
    Ready.wait();
    return NativeLinker::link(Object, TinyWasm, Directory / Name,
                              OutputKind::UniversalWasm);
  };
  auto First = std::async(std::launch::async, Link, "first.wasm");
  auto Second = std::async(std::launch::async, Link, "second.wasm");
  Start.set_value();

  EXPECT_TRUE(First.get());
  EXPECT_TRUE(Second.get());
  EXPECT_EQ(parseAOTMetadata(Directory / "first.wasm").Sections,
            parseAOTMetadata(Directory / "second.wasm").Sections);
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerPublishesConcurrentlyToSameDestination) {
  const auto Output = Directory / "shared.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  std::promise<void> Start;
  const auto Ready = Start.get_future().share();
  auto Link = [&] {
    Ready.wait();
    return NativeLinker::link(Object, TinyWasm, Output,
                              OutputKind::UniversalWasm);
  };
  auto First = std::async(std::launch::async, Link);
  auto Second = std::async(std::launch::async, Link);
  Start.set_value();

  EXPECT_TRUE(First.get());
  EXPECT_TRUE(Second.get());
  EXPECT_EQ(execute(Output), 7U);
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, WritesConvertedCompactUnwindToUniversalWasm) {
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__aarch64__) &&       \
    !defined(_M_ARM64)
  GTEST_SKIP() << "compact unwind conversion requires x86_64 or AArch64";
#else
#if defined(__x86_64__) || defined(_M_X64)
  constexpr const char *Triple = "x86_64-apple-macosx";
  constexpr std::string_view Instructions = "retq\n";
  constexpr uint32_t Encoding = UINT32_C(0x01000000);
  constexpr WasmEdge::Byte AOTArchitecture = 1;
  const auto Architecture = Target::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
  constexpr const char *Triple = "arm64-apple-macosx";
  constexpr std::string_view Instructions = "ret\n";
  constexpr uint32_t Encoding = UINT32_C(0x02000000);
  constexpr WasmEdge::Byte AOTArchitecture = 2;
  const auto Architecture = Target::AArch64;
#endif
  REQUIRE_LLVM_TARGET(Triple);
  const auto Object = makeAssemblyObject(
      llvm::Triple(Triple),
      ".text\n.globl _f0\n_f0:\n" + std::string(Instructions) +
          ".globl _t0\n_t0:\n" + std::string(Instructions) +
          ".section __DATA,__const\n.globl _version\n_version:\n.long 1\n"
          ".globl _intrinsics\n_intrinsics:\n.long 2\n"
          ".section __LD,__compact_unwind,regular,debug\n.p2align 3\n"
          ".quad _f0\n.long 1\n.long " +
          std::to_string(Encoding) +
          "\n.quad 0\n.quad 0\n.quad _t0\n.long 1\n.long " +
          std::to_string(Encoding) + "\n.quad 0\n.quad 0\n");
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 2U);
  ASSERT_TRUE(compactUnwindToEHFrame(*Graph));
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph));
  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 2U);
  for (const std::string_view Name : {"_f0", "_t0"}) {
    const auto Symbol =
        std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                     [&](const auto &Value) { return Value.Name == Name; });
    ASSERT_NE(Symbol, Graph->symbols().end()) << Name;
    const uint64_t Address =
        Graph->sections()[Symbol->Section].Address + Symbol->Offset;
    EXPECT_NE(std::find(Starts->begin(), Starts->end(), Address), Starts->end())
        << Name;
  }
  ASSERT_TRUE(applyRelocations(*Graph));
  ASSERT_TRUE(Graph->rebases().empty());

  const auto Output = Directory / "compact-unwind.wasm";
  ASSERT_TRUE(writeUniversalWasmFile(*Graph, TinyWasm, Output));
  const auto Bytes = readFile(Output);
  ASSERT_GT(Bytes.size(), TinyWasm.size());
  EXPECT_TRUE(std::equal(TinyWasm.begin(), TinyWasm.end(), Bytes.begin()));
  size_t Cursor = TinyWasm.size();
  ASSERT_EQ(Bytes[Cursor++], 0U);
  const auto ReadULEB = [&](size_t &Offset) {
    uint64_t Value = 0;
    unsigned Shift = 0;
    while (Offset < Bytes.size()) {
      const WasmEdge::Byte Part = Bytes[Offset++];
      Value |= static_cast<uint64_t>(Part & 0x7F) << Shift;
      if ((Part & 0x80) == 0)
        return Value;
      Shift += 7;
    }
    return UINT64_MAX;
  };
  const uint64_t PayloadSize = ReadULEB(Cursor);
  ASSERT_EQ(Cursor + PayloadSize, Bytes.size());
  const uint64_t NameSize = ReadULEB(Cursor);
  ASSERT_EQ(
      std::string_view(reinterpret_cast<const char *>(Bytes.data() + Cursor),
                       NameSize),
      "wasmedge");
  Cursor += NameSize;
  EXPECT_EQ(ReadULEB(Cursor), WasmEdge::AOT::kBinaryVersion);
  static_cast<void>(Bytes[Cursor++]);
  EXPECT_EQ(Bytes[Cursor++], AOTArchitecture);
  for (unsigned I = 0; I < 2; ++I)
    static_cast<void>(ReadULEB(Cursor));
  for (unsigned List = 0; List < 2; ++List) {
    const uint64_t Count = ReadULEB(Cursor);
    for (uint64_t I = 0; I < Count; ++I)
      static_cast<void>(ReadULEB(Cursor));
  }
  const uint64_t SectionCount = ReadULEB(Cursor);
  std::vector<WasmEdge::Byte> Unwind;
  for (uint64_t I = 0; I < SectionCount; ++I) {
    const WasmEdge::Byte Kind = Bytes[Cursor++];
    static_cast<void>(ReadULEB(Cursor));
    static_cast<void>(ReadULEB(Cursor));
    const uint64_t ContentSize = ReadULEB(Cursor);
    ASSERT_LE(ContentSize, Bytes.size() - Cursor);
    const auto NativeContentSize = static_cast<size_t>(ContentSize);
    if (Kind == 4)
      Unwind.assign(Bytes.data() + Cursor,
                    Bytes.data() + Cursor + NativeContentSize);
    Cursor += NativeContentSize;
  }
  ASSERT_FALSE(Unwind.empty());
  EXPECT_EQ(std::vector<WasmEdge::Byte>(Unwind.end() - 4, Unwind.end()),
            std::vector<WasmEdge::Byte>(4));
#endif
}

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

TEST_F(LinkerOutputTest, UniqueSiblingKeepsPercentSignsAndSuffix) {
  const auto Parent = Directory / "100%";
  ASSERT_TRUE(std::filesystem::create_directories(Parent));
  const std::string Prefix = ".wasmedge-";
  auto Temp = createUniqueSibling(Parent / "a%%b.so", ".o");
  ASSERT_TRUE(Temp);
  {
    TempGuard Guard(Temp->Path, Temp->File);
    const auto Name = Guard.path().filename().string();
    EXPECT_EQ(Guard.path().parent_path(), Parent);
    EXPECT_EQ(Name.rfind(Prefix, 0), 0U);
    EXPECT_EQ(Name.size(), Prefix.size() + 16 + 2);
    EXPECT_EQ(Name.substr(Name.size() - 2), ".o");
    EXPECT_TRUE(std::filesystem::exists(Guard.path()));
  }
  EXPECT_FALSE(std::filesystem::exists(Temp->Path));
}

#if !WASMEDGE_OS_WINDOWS
TEST(LinkerWriterTest, RunProcessRequiresZeroExitStatus) {
  const std::array<std::string, 2> Success{"-c", "exit 0"};
  const std::array<std::string, 2> Failure{"-c", "exit 3"};
  const std::array<std::string, 2> Killed{"-c", "kill -9 $$"};
  EXPECT_TRUE(runProcess("/bin/sh", Success));
  EXPECT_FALSE(runProcess("/bin/sh", Failure));
  EXPECT_FALSE(runProcess("/bin/sh", Killed));
  EXPECT_FALSE(runProcess("/nonexistent/program", {}));
}
#endif

TEST_F(LinkerOutputTest, NativeLinkerRejectsUnusableOutputPaths) {
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  EXPECT_FALSE(
      NativeLinker::link(Object, TinyWasm, {}, OutputKind::UniversalWasm));
  const auto Missing = Directory / "missing";
  EXPECT_FALSE(NativeLinker::link(Object, TinyWasm, Missing / "output.wasm",
                                  OutputKind::UniversalWasm));
  EXPECT_FALSE(std::filesystem::exists(Missing));
#if !WASMEDGE_OS_WINDOWS
  if (::geteuid() == 0)
    GTEST_SKIP() << "permission checks do not apply to root";
  const auto ReadOnly = Directory / "read-only";
  ASSERT_TRUE(std::filesystem::create_directory(ReadOnly));
  std::filesystem::permissions(ReadOnly,
                               std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_exec,
                               std::filesystem::perm_options::replace);
  EXPECT_FALSE(NativeLinker::link(Object, TinyWasm, ReadOnly / "output.wasm",
                                  OutputKind::UniversalWasm));
  std::filesystem::permissions(ReadOnly, std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace);
  EXPECT_TRUE(std::filesystem::is_empty(ReadOnly));
#endif
}

#if !WASMEDGE_OS_WINDOWS
TEST_F(LinkerOutputTest, MachOPublicationFailsWithoutTemporaryOutput) {
  const auto Destination = Directory / "library.dylib";
  EXPECT_FALSE(Internal::publishMachO(Directory / "missing-temporary",
                                      Destination, "/bin/true"));
  EXPECT_FALSE(Internal::publishMachO(Directory / "missing" / "temporary",
                                      Destination, "/bin/true"));
  EXPECT_FALSE(std::filesystem::exists(Destination));
  EXPECT_TRUE(std::filesystem::is_empty(Directory));
}
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
