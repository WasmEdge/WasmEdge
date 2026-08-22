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

#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/elf_writer.h"
#include "linker/macho_writer.h"
#include "linker/native_linker.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"
#include "linker/writer.h"

#include <gtest/gtest.h>

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/Process.h>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>
#if WASMEDGE_OS_WINDOWS
#include <io.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

struct EHFrameCommitTestAccess {
  static LinkExpect<void>
  commit(LinkGraph &Graph,
         std::vector<std::pair<SectionId, std::vector<Byte>>> Content,
         Span<const uint8_t> RemoveRelocations) {
    return Graph.commitNormalizedEHFrame(std::move(Content), RemoveRelocations);
  }
};

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge

namespace {

using namespace WasmEdge::LLVM::Linker;

static_assert(std::is_move_constructible_v<Writer>);
static_assert(!std::is_nothrow_move_constructible_v<Writer>);

struct ArchitectureCase {
  Target Architecture;
};

std::string architectureName(Target Architecture) {
  switch (Architecture) {
  case Target::ARM:
    return "ARM";
  case Target::X86_64:
    return "X86_64";
  case Target::AArch64:
    return "AArch64";
  case Target::RISCV64:
    return "RISCV64";
  case Target::S390X:
    return "S390X";
  }
  return "Unknown";
}

[[maybe_unused]] std::string
architectureCaseName(const testing::TestParamInfo<ArchitectureCase> &Info) {
  return architectureName(Info.param.Architecture);
}

#if WASMEDGE_OS_WINDOWS
class ScopedInvalidParameterHandler {
public:
  ScopedInvalidParameterHandler()
      : Previous(_set_thread_local_invalid_parameter_handler(ignore)) {}

  ~ScopedInvalidParameterHandler() {
    _set_thread_local_invalid_parameter_handler(Previous);
  }

private:
  static void ignore(const wchar_t *, const wchar_t *, const wchar_t *,
                     unsigned int, uintptr_t) {}

  _invalid_parameter_handler Previous;
};
#endif

bool hasRelocationHandler(Target Architecture) {
  switch (Architecture) {
  case Target::X86_64:
    return WASMEDGE_LINKER_HAS_X86_64;
  case Target::AArch64:
    return WASMEDGE_LINKER_HAS_AARCH64;
  case Target::ARM:
    return WASMEDGE_LINKER_HAS_ARM;
  case Target::RISCV64:
    return WASMEDGE_LINKER_HAS_RISCV64;
  case Target::S390X:
    return WASMEDGE_LINKER_HAS_S390X;
  }
  return false;
}

#define REQUIRE_RELOCATION_HANDLER(TARGET)                                     \
  do {                                                                         \
    if (!hasRelocationHandler(TARGET)) {                                       \
      GTEST_SKIP() << architectureName(TARGET)                                 \
                   << " relocation handler is not compiled";                   \
    }                                                                          \
  } while (false)

template <typename F>
LinkGraph rebuildWithSectionContent(const LinkGraph &Source, SectionId Id,
                                    F &&Mutate) {
  LinkGraph Result(Source.target(), Source.endianness(), Source.format());
  EXPECT_TRUE(Result.beginInput("rebuilt-test-graph"));
  for (SectionId I = 0; I < Source.sections().size(); ++I) {
    auto Value = Source.sections()[I];
    if (I == Id)
      Mutate(Value.Content);
    EXPECT_TRUE(Result.addSection(std::move(Value)));
  }
  for (const auto &Value : Source.symbols())
    EXPECT_TRUE(Result.addSymbol(Value));
  for (const auto &Value : Source.relocations())
    EXPECT_TRUE(Result.addRelocation(Value));
  for (const auto &Value : Source.rebases())
    EXPECT_TRUE(Result.addRebase(Value));
  for (const auto &Value : Source.ehFrameReferences())
    EXPECT_TRUE(Result.addEHFrameReference(Value));
  for (const auto &Value : Source.compactUnwind())
    EXPECT_TRUE(Result.addCompactUnwind(Value));
  if (Source.format() == ObjectFormat::ELF) {
    EXPECT_TRUE(Result.setELFFlags(Source.elfFlags()));
  }
  return Result;
}

LinkGraph makePEGraph(Target Architecture) {
  LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::COFF);
  EXPECT_TRUE(Graph.beginInput("writer.obj"));
  auto Text = Graph.addSection(
      Section{".text$f", SectionKind::Text, 16, 4, 0, 0,
              Architecture == Target::X86_64
                  ? std::vector<WasmEdge::Byte>{0xC3, 0, 0, 0}
                  : std::vector<WasmEdge::Byte>{0xC0, 0x03, 0x5F, 0xD6}});
  auto RData = Graph.addSection(
      Section{".rdata", SectionKind::ReadOnly, 8, 4, 0, 0, {1, 2, 3, 4}});
  auto Data = Graph.addSection(Section{".data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)});
  auto BSS = Graph.addSection(Section{".bss", SectionKind::BSS, 8, 16});
  const size_t PDataSize = Architecture == Target::X86_64 ? 12 : 8;
  auto PData = Graph.addSection(
      Section{".pdata", SectionKind::Unwind, 4, PDataSize, 0, 0,
              std::vector<WasmEdge::Byte>(PDataSize), SectionPurpose::PData});
  auto XData = Graph.addSection(Section{".xdata",
                                        SectionKind::Unwind,
                                        4,
                                        4,
                                        0,
                                        0,
                                        {1, 0, 0, 0},
                                        SectionPurpose::XData});
  EXPECT_TRUE(Text && RData && Data && BSS && PData && XData);
  EXPECT_TRUE(
      Graph.addSymbol(Symbol{"z_impl", *Text, 0, 4, true, "alias", true}));
  EXPECT_TRUE(
      Graph.addSymbol(Symbol{"alpha", *Text, 0, 4, true, std::nullopt, true}));
  EXPECT_TRUE(Graph.addRebase(
      Rebase{*Data, 0,
             Architecture == Target::X86_64
                 ? static_cast<uint32_t>(llvm::COFF::IMAGE_REL_AMD64_ADDR64)
                 : static_cast<uint32_t>(llvm::COFF::IMAGE_REL_ARM64_ADDR64),
             0, 8, ObjectFormat::COFF}));
  return Graph;
}

struct ELFCase {
  Target Architecture;
  Endianness Endian;
  uint16_t Machine;
  uint32_t RelativeType;
  bool Is64;
};

[[maybe_unused]] std::string
elfCaseName(const testing::TestParamInfo<ELFCase> &Info) {
  return architectureName(Info.param.Architecture);
}

class PEArchitectureTest
    : public testing::Test,
      public testing::WithParamInterface<ArchitectureCase> {};

class ELFArchitectureTest : public testing::Test,
                            public testing::WithParamInterface<ELFCase> {};

class MachOArchitectureTest
    : public testing::Test,
      public testing::WithParamInterface<ArchitectureCase> {};

void writeInteger(std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                  uint64_t Value, uint8_t Width, Endianness Endian) {
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift = Endian == Endianness::Little ? I : Width - I - 1;
    Bytes[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (Shift * 8));
  }
}

std::vector<WasmEdge::Byte> makeEHFrame(Endianness Endian) {
  constexpr size_t CIERecordSize = 17;
  constexpr size_t FDERecordSize = 17;
  constexpr size_t TerminatorSize = 4;
  std::vector<WasmEdge::Byte> Bytes(CIERecordSize + FDERecordSize * 2 +
                                    TerminatorSize);
  writeInteger(Bytes, 0, 13, 4, Endian);
  Bytes[8] = 1;
  Bytes[9] = 'z';
  Bytes[10] = 'R';
  Bytes[12] = 1;
  Bytes[13] = 0x78;
  Bytes[14] = 16;
  Bytes[15] = 1;
  Bytes[16] = 0x1B;
  for (size_t I = 0; I < 2; ++I) {
    const size_t Offset = CIERecordSize + I * FDERecordSize;
    writeInteger(Bytes, Offset, 13, 4, Endian);
    writeInteger(Bytes, Offset + 4, Offset + 4, 4, Endian);
    writeInteger(Bytes, Offset + 12, 2, 4, Endian);
  }
  return Bytes;
}

std::vector<WasmEdge::Byte> makePersonalityEHFrame(Endianness Endian,
                                                   uint8_t Encoding) {
  constexpr size_t CIERecordSize = 23;
  constexpr size_t FDERecordSize = 17;
  constexpr size_t TerminatorSize = 4;
  std::vector<WasmEdge::Byte> Bytes(CIERecordSize + FDERecordSize +
                                    TerminatorSize);
  writeInteger(Bytes, 0, 19, 4, Endian);
  Bytes[8] = 1;
  Bytes[9] = 'z';
  Bytes[10] = 'P';
  Bytes[11] = 'R';
  Bytes[13] = 1;
  Bytes[14] = 0x78;
  Bytes[15] = 16;
  Bytes[16] = 6;
  Bytes[17] = Encoding;
  Bytes[22] = 0x1B;
  writeInteger(Bytes, CIERecordSize, 13, 4, Endian);
  writeInteger(Bytes, CIERecordSize + 4, CIERecordSize + 4, 4, Endian);
  writeInteger(Bytes, CIERecordSize + 12, 2, 4, Endian);
  return Bytes;
}

LinkGraph makeELFGraph(const ELFCase &Test) {
  LinkGraph Graph(Test.Architecture, Test.Endian);
  EXPECT_TRUE(Graph.beginInput("writer.o"));
  if (Test.Architecture == Target::ARM) {
    EXPECT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                  llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  }
  const uint8_t PointerSize = Test.Is64 ? 8 : 4;
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 16, 4, 0, 0, {0xC3, 0, 0, 0}});
  auto Rodata = Graph.addSection(
      Section{".rodata", SectionKind::ReadOnly, 8, 4, 0, 0, {1, 2, 3, 4}});
  auto EHFrame = Graph.addSection(Section{
      ".eh_frame", SectionKind::Unwind, 8, makeEHFrame(Test.Endian).size(), 0,
      0, makeEHFrame(Test.Endian), SectionPurpose::EHFrame});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, PointerSize, PointerSize, 0, 0,
              std::vector<WasmEdge::Byte>(PointerSize)});
  auto BSS = Graph.addSection(
      Section{".bss", SectionKind::BSS, PointerSize, PointerSize});
  EXPECT_TRUE(Text && Rodata && EHFrame && Data && BSS);
  EXPECT_TRUE(
      Graph.addSymbol(Symbol{"f0", *Text, 0, 4, true, std::nullopt, true}));
  EXPECT_TRUE(
      Graph.addSymbol(Symbol{"f1", *Text, 2, 2, true, std::nullopt, true}));
  EXPECT_TRUE(Graph.addSymbol(
      Symbol{"value", *Data, 0, PointerSize, true, std::nullopt, true}));
  EXPECT_TRUE(Graph.addRebase(
      Rebase{*Data, 0, Test.RelativeType, 0, PointerSize, ObjectFormat::ELF}));
  return Graph;
}

uint64_t readInteger(const std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                     uint8_t Width, Endianness Endian) {
  uint64_t Result = 0;
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift = Endian == Endianness::Little ? I : Width - I - 1;
    Result |= static_cast<uint64_t>(Bytes[Offset + I]) << (Shift * 8);
  }
  return Result;
}

uint64_t readULEB(const std::vector<WasmEdge::Byte> &Bytes, size_t &Offset,
                  size_t End) {
  uint64_t Result = 0;
  for (uint8_t Shift = 0; Shift < 64 && Offset < End; Shift += 7) {
    const uint8_t Value = Bytes[Offset++];
    Result |= static_cast<uint64_t>(Value & 0x7F) << Shift;
    if ((Value & 0x80) == 0)
      return Result;
  }
  ADD_FAILURE() << "malformed ULEB";
  return 0;
}

void readExportNode(const std::vector<WasmEdge::Byte> &Bytes, size_t Base,
                    size_t Start, size_t End, const std::string &Prefix,
                    std::map<std::string, uint64_t> &Exports,
                    std::set<size_t> &Visited) {
  ASSERT_LT(Start, End);
  ASSERT_TRUE(Visited.insert(Start).second);
  size_t Offset = Start;
  const uint64_t TerminalSize = readULEB(Bytes, Offset, End);
  ASSERT_LE(TerminalSize, End - Offset);
  const size_t TerminalEnd = Offset + TerminalSize;
  if (TerminalSize != 0) {
    const uint64_t Flags = readULEB(Bytes, Offset, TerminalEnd);
    EXPECT_EQ(Flags, llvm::MachO::EXPORT_SYMBOL_FLAGS_KIND_REGULAR);
    const uint64_t Address = readULEB(Bytes, Offset, TerminalEnd);
    EXPECT_EQ(Offset, TerminalEnd);
    Exports.emplace(Prefix, Address);
  }
  Offset = TerminalEnd;
  ASSERT_LT(Offset, End);
  const uint8_t ChildCount = Bytes[Offset++];
  std::set<char> ChildPrefixes;
  for (uint8_t I = 0; I < ChildCount; ++I) {
    std::string Suffix;
    while (Offset < End && Bytes[Offset] != 0)
      Suffix.push_back(static_cast<char>(Bytes[Offset++]));
    ASSERT_LT(Offset, End);
    ++Offset;
    ASSERT_FALSE(Suffix.empty());
    EXPECT_TRUE(ChildPrefixes.insert(Suffix.front()).second);
    const uint64_t Child = readULEB(Bytes, Offset, End);
    ASSERT_LT(Child, End - Base);
    readExportNode(Bytes, Base, Base + Child, End, Prefix + Suffix, Exports,
                   Visited);
  }
}

const llvm::object::SectionRef *
findSection(const llvm::object::ObjectFile &Object, std::string_view Wanted,
            llvm::object::SectionRef &Storage) {
  for (const auto &Section : Object.sections()) {
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (Name && *Name == llvm::StringRef(Wanted.data(), Wanted.size())) {
      Storage = Section;
      return &Storage;
    }
  }
  return nullptr;
}

uint32_t elfHash(std::string_view Name) {
  uint32_t Result = 0;
  for (const char Value : Name) {
    const auto Character = static_cast<unsigned char>(Value);
    Result = (Result << 4) + Character;
    const uint32_t High = Result & UINT32_C(0xF0000000);
    if (High != 0)
      Result ^= High >> 24;
    Result &= ~High;
  }
  return Result;
}

TEST_P(PEArchitectureTest, WritesDeterministicPE32PlusDLLs) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  const auto Architecture = GetParam().Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  auto Graph = makePEGraph(Architecture);
  ASSERT_TRUE(PEWriter::layout(Graph));
  Graph = rebuildWithSectionContent(Graph, 2, [&](auto &Data) {
    for (uint8_t I = 0; I < 8; ++I)
      Data[I] =
          static_cast<WasmEdge::Byte>(Graph.sections()[0].Address >> (I * 8));
  });
  Graph = rebuildWithSectionContent(Graph, 4, [&](auto &PData) {
    auto WritePData = [&](size_t Offset, uint64_t Value) {
      for (uint8_t I = 0; I < 4; ++I)
        PData[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
    };
    WritePData(0, Graph.sections()[0].Address - ImageBase);
    WritePData(4,
               (Architecture == Target::X86_64 ? Graph.sections()[0].Address + 4
                                               : Graph.sections()[5].Address) -
                   ImageBase);
    if (Architecture == Target::X86_64)
      WritePData(8, Graph.sections()[5].Address - ImageBase);
  });
  ASSERT_TRUE(applyRelocations(Graph));

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "writer.dll", Output));
  std::vector<WasmEdge::Byte> Again;
  Writer Second(Again);
  ASSERT_TRUE(PEWriter::write(Graph, "writer.dll", Second));
  EXPECT_EQ(Bytes, Again);
  ASSERT_GE(Bytes.size(), 512U);
  EXPECT_EQ(Bytes[0], 'M');
  EXPECT_EQ(Bytes[1], 'Z');
  const uint32_t PEOffset =
      static_cast<uint32_t>(readInteger(Bytes, 0x3C, 4, Endianness::Little));
  EXPECT_GE(PEOffset, 0x40U);
  EXPECT_EQ(readInteger(Bytes, PEOffset, 4, Endianness::Little),
            UINT32_C(0x00004550));
  const size_t COFF = PEOffset + 4;
  EXPECT_EQ(readInteger(Bytes, COFF, 2, Endianness::Little),
            Architecture == Target::X86_64
                ? llvm::COFF::IMAGE_FILE_MACHINE_AMD64
                : llvm::COFF::IMAGE_FILE_MACHINE_ARM64);
  EXPECT_EQ(readInteger(Bytes, COFF + 16, 2, Endianness::Little), 240U);
  const uint16_t Characteristics = static_cast<uint16_t>(
      readInteger(Bytes, COFF + 18, 2, Endianness::Little));
  EXPECT_EQ(Characteristics & (llvm::COFF::IMAGE_FILE_DLL |
                               llvm::COFF::IMAGE_FILE_EXECUTABLE_IMAGE |
                               llvm::COFF::IMAGE_FILE_LARGE_ADDRESS_AWARE),
            llvm::COFF::IMAGE_FILE_DLL |
                llvm::COFF::IMAGE_FILE_EXECUTABLE_IMAGE |
                llvm::COFF::IMAGE_FILE_LARGE_ADDRESS_AWARE);
  const size_t Optional = COFF + 20;
  EXPECT_EQ(readInteger(Bytes, Optional, 2, Endianness::Little), 0x20BU);
  EXPECT_NE(readInteger(Bytes, Optional + 4, 4, Endianness::Little), 0U);
  EXPECT_NE(readInteger(Bytes, Optional + 8, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, Optional + 12, 4, Endianness::Little), 16U);
  EXPECT_EQ(readInteger(Bytes, Optional + 16, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, Optional + 20, 4, Endianness::Little),
            Graph.sections()[0].Address - ImageBase);
  EXPECT_EQ(readInteger(Bytes, Optional + 24, 8, Endianness::Little),
            ImageBase);
  EXPECT_EQ(readInteger(Bytes, Optional + 32, 4, Endianness::Little), 4096U);
  EXPECT_EQ(readInteger(Bytes, Optional + 36, 4, Endianness::Little), 512U);
  EXPECT_EQ(readInteger(Bytes, Optional + 64, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, Optional + 56, 4, Endianness::Little) % 4096,
            0U);
  EXPECT_EQ(readInteger(Bytes, Optional + 60, 4, Endianness::Little) % 512, 0U);
  EXPECT_EQ(readInteger(Bytes, Optional + 68, 2, Endianness::Little),
            llvm::COFF::IMAGE_SUBSYSTEM_WINDOWS_CUI);
  const uint16_t DLLCharacteristics = static_cast<uint16_t>(
      readInteger(Bytes, Optional + 70, 2, Endianness::Little));
  EXPECT_EQ(DLLCharacteristics &
                (llvm::COFF::IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE |
                 llvm::COFF::IMAGE_DLL_CHARACTERISTICS_NX_COMPAT |
                 llvm::COFF::IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA),
            llvm::COFF::IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE |
                llvm::COFF::IMAGE_DLL_CHARACTERISTICS_NX_COMPAT |
                llvm::COFF::IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA);
  EXPECT_EQ(readInteger(Bytes, Optional + 108, 4, Endianness::Little), 16U);

  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "writer.dll"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Object);
  ASSERT_NE(PE, nullptr);
  std::map<std::string, uint32_t> Exports;
  for (const auto &Export : PE->export_directories()) {
    llvm::StringRef Name;
    llvm::StringRef DLL;
    uint32_t RVA = 0;
    uint32_t Base = 0;
    uint32_t Ordinal = 0;
    bool Forwarder = true;
    ASSERT_FALSE(Export.getSymbolName(Name));
    ASSERT_FALSE(Export.getDllName(DLL));
    ASSERT_FALSE(Export.getExportRVA(RVA));
    ASSERT_FALSE(Export.getOrdinalBase(Base));
    ASSERT_FALSE(Export.getOrdinal(Ordinal));
    ASSERT_FALSE(Export.isForwarder(Forwarder));
    EXPECT_EQ(DLL, "writer.dll");
    EXPECT_EQ(Base, 1U);
    EXPECT_FALSE(Forwarder);
    Exports.emplace(Name.str(), RVA);
  }
  EXPECT_EQ(Exports,
            (std::map<std::string, uint32_t>{
                {"alias", static_cast<uint32_t>(Graph.sections()[0].Address -
                                                ImageBase)},
                {"alpha", static_cast<uint32_t>(Graph.sections()[0].Address -
                                                ImageBase)}}));
  std::map<std::string, uint32_t> Sections;
  for (const auto &Section : PE->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    const auto *Header = PE->getCOFFSection(Section);
    Sections.emplace(Name->str(), Header->Characteristics);
    EXPECT_FALSE(
        (Header->Characteristics & llvm::COFF::IMAGE_SCN_MEM_WRITE) != 0 &&
        (Header->Characteristics & llvm::COFF::IMAGE_SCN_MEM_EXECUTE) != 0);
  }
  EXPECT_EQ(Sections[".text"], llvm::COFF::IMAGE_SCN_CNT_CODE |
                                   llvm::COFF::IMAGE_SCN_MEM_EXECUTE |
                                   llvm::COFF::IMAGE_SCN_MEM_READ);
  EXPECT_NE(Sections[".data"] & llvm::COFF::IMAGE_SCN_MEM_WRITE, 0U);
  EXPECT_NE(Sections[".bss"] & llvm::COFF::IMAGE_SCN_CNT_UNINITIALIZED_DATA,
            0U);
  for (const char *Name : {".rdata", ".pdata", ".xdata", ".edata", ".reloc"})
    EXPECT_NE(Sections.count(Name), 0U) << Name;

  const std::array<size_t, 8> EmptyDirectories{1, 6, 9, 10, 11, 12, 13, 14};
  for (const size_t Index : EmptyDirectories) {
    const size_t Directory = Optional + 112 + Index * 8;
    EXPECT_EQ(readInteger(Bytes, Directory, 8, Endianness::Little), 0U)
        << Index;
  }
  const size_t ExportDirectory = Optional + 112;
  const size_t ExceptionDirectory = Optional + 112 + 3 * 8;
  const size_t RelocDirectory = Optional + 112 + 5 * 8;
  EXPECT_NE(readInteger(Bytes, ExportDirectory, 4, Endianness::Little), 0U);
  EXPECT_NE(readInteger(Bytes, RelocDirectory, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, ExceptionDirectory + 4, 4, Endianness::Little),
            Architecture == Target::X86_64 ? 12U : 8U);
  llvm::object::SectionRef RelocStorage;
  const auto *RelocSection = findSection(*PE, ".reloc", RelocStorage);
  ASSERT_NE(RelocSection, nullptr);
  auto RelocContent = RelocSection->getContents();
  ASSERT_TRUE(static_cast<bool>(RelocContent));
  const std::vector<WasmEdge::Byte> RelocBytes(RelocContent->bytes_begin(),
                                               RelocContent->bytes_end());
  ASSERT_GE(RelocBytes.size(), 12U);
  const uint32_t DataRVA =
      static_cast<uint32_t>(Graph.sections()[2].Address - ImageBase);
  EXPECT_EQ(readInteger(RelocBytes, 0, 4, Endianness::Little),
            DataRVA & ~UINT32_C(0xFFF));
  EXPECT_EQ(readInteger(RelocBytes, 4, 4, Endianness::Little), 12U);
  EXPECT_EQ(readInteger(RelocBytes, 8, 2, Endianness::Little),
            (llvm::COFF::IMAGE_REL_BASED_DIR64 << 12) | (DataRVA & 0xFFF));
  EXPECT_EQ(readInteger(RelocBytes, 10, 2, Endianness::Little), 0U);
}

TEST(PEWriterTest, OmitsRelocationsWhenImageHasNoRebases) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("writer.obj"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(
      Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true, std::nullopt, true}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "writer.dll", Output));

  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "writer.dll"));
  ASSERT_TRUE(static_cast<bool>(Object));
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Object);
  ASSERT_NE(PE, nullptr);
  llvm::object::SectionRef Storage;
  EXPECT_EQ(findSection(*PE, ".reloc", Storage), nullptr);
  const auto *Header = PE->getPE32PlusHeader();
  ASSERT_NE(Header, nullptr);
  const auto *COFFHeader = PE->getCOFFHeader();
  ASSERT_NE(COFFHeader, nullptr);
  EXPECT_EQ(
      COFFHeader->Characteristics & llvm::COFF::IMAGE_FILE_RELOCS_STRIPPED, 0U);
  const size_t Optional = readInteger(Bytes, 0x3C, 4, Endianness::Little) + 24;
  EXPECT_EQ(readInteger(Bytes, Optional + 112 + 5 * 8, 8, Endianness::Little),
            0U);
  EXPECT_EQ(Header->DLLCharacteristics &
                (llvm::COFF::IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE |
                 llvm::COFF::IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA),
            llvm::COFF::IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE |
                llvm::COFF::IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA);
}

TEST(PEWriterTest, RejectsInvalidRebasesAndOverflowAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makePEGraph(Target::X86_64);
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  auto &RebaseValue = const_cast<std::vector<Rebase> &>(Graph.rebases())[0];
  RebaseValue.Width = 4;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(PEWriter::write(Graph, "invalid.dll", Output));
  EXPECT_TRUE(Bytes.empty());

  LinkGraph Overflow(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Overflow.beginInput("overflow.obj"));
  ASSERT_TRUE(Overflow.addSection(
      Section{".text", SectionKind::Text, 1, UINT64_C(1) << 32, 0, 0, {0}}));
  EXPECT_FALSE(PEWriter::layout(Overflow));
  EXPECT_EQ(Overflow.sections()[0].Address, 0U);
  EXPECT_EQ(Overflow.sections()[0].FileOffset, 0U);
}

TEST(PEWriterTest, HonorsInputAlignmentAboveSectionAlignment) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("aligned.obj"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text$a", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".text$b", SectionKind::Text, 8192, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  EXPECT_EQ(Graph.sections()[1].Address % 8192, 0U);
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "aligned.dll", Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "aligned.dll"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
  llvm::object::SectionRef Storage;
  const auto *Text = findSection(**Object, ".text", Storage);
  ASSERT_NE(Text, nullptr);
  EXPECT_EQ(Text->getAddress() % 8192, 0U);
}

TEST_P(PEArchitectureTest, SortsRuntimeFunctionTables) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  const auto Architecture = GetParam().Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("unwind.obj"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 64, 0, 0,
              std::vector<WasmEdge::Byte>(64, Architecture == Target::X86_64
                                                  ? WasmEdge::Byte{0x90}
                                                  : WasmEdge::Byte{0})});
  auto XData = Graph.addSection(Section{".xdata", SectionKind::Unwind, 4, 8, 0,
                                        0, std::vector<WasmEdge::Byte>(8),
                                        SectionPurpose::XData});
  const size_t EntrySize = Architecture == Target::X86_64 ? 12 : 8;
  auto PDataA = Graph.addSection(
      Section{".pdata$a", SectionKind::Unwind, 4, EntrySize, 0, 0,
              std::vector<WasmEdge::Byte>(EntrySize), SectionPurpose::PData});
  auto PDataB = Graph.addSection(
      Section{".pdata$b", SectionKind::Unwind, 16, EntrySize, 0, 0,
              std::vector<WasmEdge::Byte>(EntrySize), SectionPurpose::PData});
  ASSERT_TRUE(Text && XData && PDataA && PDataB);
  ASSERT_TRUE(PEWriter::layout(Graph));
  auto Write32 = [&](SectionId Section, size_t Offset, uint32_t Value) {
    std::array<WasmEdge::Byte, 4> Content{};
    for (uint8_t I = 0; I < 4; ++I)
      Content[I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
    Graph = rebuildWithSectionContent(Graph, Section, [&](auto &Data) {
      std::copy(Content.begin(), Content.end(), Data.begin() + Offset);
    });
  };
  const uint32_t TextRVA =
      static_cast<uint32_t>(Graph.sections()[*Text].Address - ImageBase);
  const uint32_t XDataRVA =
      static_cast<uint32_t>(Graph.sections()[*XData].Address - ImageBase);
  if (Architecture == Target::X86_64) {
    Write32(*PDataA, 0, TextRVA + 32);
    Write32(*PDataA, 4, TextRVA + 48);
    Write32(*PDataA, 8, XDataRVA + 4);
    Write32(*PDataB, 0, TextRVA);
    Write32(*PDataB, 4, TextRVA + 16);
    Write32(*PDataB, 8, XDataRVA);
  } else {
    Write32(*XData, 0, 4);
    Write32(*PDataA, 0, TextRVA + 32);
    Write32(*PDataA, 4, (4U << 2) | 1U);
    Write32(*PDataB, 0, TextRVA);
    Write32(*PDataB, 4, XDataRVA);
  }
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(Graph, "sorted.dll", Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "sorted.dll"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef Storage;
  const auto *PData = findSection(**Object, ".pdata", Storage);
  ASSERT_NE(PData, nullptr);
  const uint32_t PEOffset =
      static_cast<uint32_t>(readInteger(Bytes, 0x3C, 4, Endianness::Little));
  const size_t ExceptionDirectory = PEOffset + 24 + 112 + 3 * 8;
  EXPECT_EQ(readInteger(Bytes, ExceptionDirectory, 4, Endianness::Little),
            PData->getAddress() - ImageBase);
  EXPECT_EQ(readInteger(Bytes, ExceptionDirectory + 4, 4, Endianness::Little),
            EntrySize * 2);
  auto Content = PData->getContents();
  ASSERT_TRUE(static_cast<bool>(Content));
  std::vector<WasmEdge::Byte> PDataBytes(Content->bytes_begin(),
                                         Content->bytes_end());
  EXPECT_EQ(readInteger(PDataBytes, 0, 4, Endianness::Little), TextRVA);
  EXPECT_EQ(readInteger(PDataBytes, EntrySize, 4, Endianness::Little),
            TextRVA + 32);
}

TEST(PEWriterTest, RejectsSymbolsReferencingSortedRuntimeFunctions) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makePEGraph(Target::X86_64);
  ASSERT_TRUE(
      Graph.addSymbol(Symbol{"bad_pdata", 4, 0, 12, true, std::nullopt, true}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  const uint32_t TextRVA =
      static_cast<uint32_t>(Graph.sections()[0].Address - ImageBase);
  const uint32_t XDataRVA =
      static_cast<uint32_t>(Graph.sections()[5].Address - ImageBase);
  Graph = rebuildWithSectionContent(Graph, 4, [&](auto &PData) {
    for (const auto &[Offset, Value] :
         std::array<std::pair<size_t, uint32_t>, 3>{
             {{0, TextRVA}, {4, TextRVA + 4}, {8, XDataRVA}}})
      for (uint8_t I = 0; I < 4; ++I)
        PData[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
  });
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(PEWriter::write(Graph, "bad-symbol.dll", Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(PEWriterTest, ReportsRelocationSourceTargetingRuntimeFunctions) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makePEGraph(Target::X86_64);
  auto Target = Graph.addSymbol(
      Symbol{"pdata_target", 4, 0, 0, false, std::nullopt, false});
  ASSERT_TRUE(Target);
  constexpr uint32_t Type = llvm::COFF::IMAGE_REL_AMD64_ADDR32NB;
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{0, 0, Type, *Target, 0, false, ObjectFormat::COFF, 4, false}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  auto Result = PEWriter::write(Graph, "pdata-relocation.dll", Output);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("target section '.pdata'"),
            std::string::npos);
  EXPECT_EQ(Result.error().Section, 0U);
  EXPECT_EQ(Result.error().SectionName, ".text$f");
  EXPECT_EQ(Result.error().Offset, 0U);
  EXPECT_EQ(Result.error().RelocationType, Type);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
  EXPECT_TRUE(Bytes.empty());
}

TEST_P(PEArchitectureTest, RejectsInvalidRuntimeFunctionTables) {
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  const auto Architecture = GetParam().Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  auto Graph = makePEGraph(Architecture);
  ASSERT_TRUE(PEWriter::layout(Graph));
  const uint32_t TextRVA =
      static_cast<uint32_t>(Graph.sections()[0].Address - ImageBase);
  const uint32_t XDataRVA =
      static_cast<uint32_t>(Graph.sections()[5].Address - ImageBase);
  Graph = rebuildWithSectionContent(Graph, 4, [&](auto &PData) {
    auto Write = [&](size_t Offset, uint32_t Value) {
      for (uint8_t I = 0; I < 4; ++I)
        PData[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
    };
    Write(0, TextRVA);
    if (Architecture == Target::X86_64) {
      Write(4, TextRVA);
      Write(8, XDataRVA + 8);
    } else {
      Write(4, XDataRVA + 8);
    }
  });
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  auto Result = PEWriter::write(Graph, "invalid-unwind.dll", Output);
  ASSERT_FALSE(Result);
  const auto Message =
      Architecture == Target::X86_64 ? "runtime function" : "PE unwind";
  EXPECT_NE(Result.error().Message.find(Message), std::string::npos);
  EXPECT_EQ(Result.error().Section, 4U);
  EXPECT_EQ(Result.error().SectionName, ".pdata");
  EXPECT_EQ(Result.error().Offset, 0U);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
  EXPECT_TRUE(Bytes.empty());
}

TEST(PEWriterTest, RejectsDuplicateAndOverlappingX86_64RuntimeFunctions) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  for (const bool Duplicate : {true, false}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
    ASSERT_TRUE(Graph.beginInput("overlap.obj"));
    auto Text = Graph.addSection(Section{".text", SectionKind::Text, 4, 64, 0,
                                         0, std::vector<WasmEdge::Byte>(64)});
    auto XData = Graph.addSection(Section{".xdata",
                                          SectionKind::Unwind,
                                          4,
                                          4,
                                          0,
                                          0,
                                          {1, 0, 0, 0},
                                          SectionPurpose::XData});
    auto PData = Graph.addSection(Section{".pdata", SectionKind::Unwind, 4, 24,
                                          0, 0, std::vector<WasmEdge::Byte>(24),
                                          SectionPurpose::PData});
    ASSERT_TRUE(Text && XData && PData);
    ASSERT_TRUE(PEWriter::layout(Graph));
    const uint32_t TextRVA =
        static_cast<uint32_t>(Graph.sections()[*Text].Address - ImageBase);
    const uint32_t XDataRVA =
        static_cast<uint32_t>(Graph.sections()[*XData].Address - ImageBase);
    Graph = rebuildWithSectionContent(Graph, *PData, [&](auto &Content) {
      auto Write = [&](size_t Offset, uint32_t Value) {
        for (uint8_t I = 0; I < 4; ++I)
          Content[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
      };
      Write(0, TextRVA);
      Write(4, TextRVA + 32);
      Write(8, XDataRVA);
      Write(12, TextRVA + (Duplicate ? 0 : 16));
      Write(16, TextRVA + 48);
      Write(20, XDataRVA);
    });
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    EXPECT_FALSE(PEWriter::write(Graph, "overlap.dll", Output));
    EXPECT_TRUE(Bytes.empty());
  }
}

TEST(PEWriterTest, RejectsMisSizedAArch64RuntimeFunctionTable) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  LinkGraph MisSized(Target::AArch64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(MisSized.beginInput("mis-sized.obj"));
  ASSERT_TRUE(MisSized.addSection(Section{".text", SectionKind::Text, 4, 16, 0,
                                          0, std::vector<WasmEdge::Byte>(16)}));
  ASSERT_TRUE(MisSized.addSection(Section{".pdata", SectionKind::Unwind, 4, 9,
                                          0, 0, std::vector<WasmEdge::Byte>(9),
                                          SectionPurpose::PData}));
  ASSERT_TRUE(PEWriter::layout(MisSized));
  ASSERT_TRUE(applyRelocations(MisSized));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  auto Result = PEWriter::write(MisSized, "mis-sized.dll", Output);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("runtime function table size"),
            std::string::npos);
  EXPECT_EQ(Result.error().Section, 1U);
  EXPECT_EQ(Result.error().SectionName, ".pdata");
  EXPECT_EQ(Result.error().Offset, 8U);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
  EXPECT_TRUE(Bytes.empty());
}

INSTANTIATE_TEST_SUITE_P(Architectures, PEArchitectureTest,
                         testing::Values(ArchitectureCase{Target::X86_64},
                                         ArchitectureCase{Target::AArch64}),
                         architectureCaseName);

TEST_P(ELFArchitectureTest, LaysOutImage) {
  auto Graph = makeELFGraph(GetParam());
  EXPECT_TRUE(ELFWriter::layout(Graph));
}

TEST_P(ELFArchitectureTest, WritesLoadableImage) {
  const auto &Test = GetParam();
  if (!hasRelocationHandler(Test.Architecture))
    GTEST_SKIP() << architectureName(Test.Architecture)
                 << " relocation handler is not compiled";
  auto Graph = makeELFGraph(Test);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  Graph = rebuildWithSectionContent(Graph, 2, [&](auto &EHContent) {
    for (size_t I = 0; I < 2; ++I) {
      const size_t FieldOffset = 17 + I * 17 + 8;
      const uint64_t FieldAddress = Graph.sections()[2].Address + FieldOffset;
      const uint64_t FunctionAddress = Graph.sections()[0].Address + I * 2;
      const int64_t Delta = static_cast<int64_t>(FunctionAddress) -
                            static_cast<int64_t>(FieldAddress);
      for (uint8_t Byte = 0; Byte < 4; ++Byte) {
        const uint8_t Shift =
            Test.Endian == Endianness::Little ? Byte : 3 - Byte;
        EHContent[FieldOffset + Byte] = static_cast<WasmEdge::Byte>(
            static_cast<uint32_t>(Delta) >> (Shift * 8));
      }
    }
  });
  Graph = rebuildWithSectionContent(Graph, 3, [&](auto &Data) {
    for (uint8_t I = 0; I < (Test.Is64 ? 8 : 4); ++I) {
      const uint8_t Shift =
          Test.Endian == Endianness::Little ? I : (Test.Is64 ? 7 - I : 3 - I);
      Data[I] = static_cast<WasmEdge::Byte>(Graph.sections()[3].Address >>
                                            (Shift * 8));
    }
  });
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  std::vector<WasmEdge::Byte> SecondBytes;
  Writer SecondOutput(SecondBytes);
  ASSERT_TRUE(ELFWriter::write(Graph, SecondOutput));
  EXPECT_EQ(Bytes, SecondBytes);

  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "writer.so"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
  EXPECT_TRUE((*Object)->isELF());
  EXPECT_EQ(readInteger(Bytes, 16, 2, Test.Endian), llvm::ELF::ET_DYN);
  EXPECT_EQ((*Object)->getArch(),
            Test.Architecture == Target::ARM       ? llvm::Triple::arm
            : Test.Architecture == Target::X86_64  ? llvm::Triple::x86_64
            : Test.Architecture == Target::AArch64 ? llvm::Triple::aarch64
            : Test.Architecture == Target::RISCV64 ? llvm::Triple::riscv64
                                                   : llvm::Triple::systemz);
  std::set<std::string> Sections;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    Sections.emplace(Name->str());
  }
  for (const char *Name :
       {".text", ".rodata", ".eh_frame", ".eh_frame_hdr", ".data", ".bss",
        ".dynsym", ".dynstr", ".hash", ".dynamic", ".shstrtab"}) {
    EXPECT_TRUE(Sections.count(Name)) << Name;
  }
  EXPECT_TRUE(Sections.count(Test.Is64 ? ".rela.dyn" : ".rel.dyn"));
  std::set<std::string> Symbols;
  const auto *ELF = llvm::dyn_cast<llvm::object::ELFObjectFileBase>(&**Object);
  ASSERT_NE(ELF, nullptr);
  for (const auto &Symbol : ELF->getDynamicSymbolIterators()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (!Name->empty())
      Symbols.emplace(Name->str());
  }
  EXPECT_EQ(Symbols, (std::set<std::string>{"f0", "f1", "value"}));

  const uint64_t ProgramHeaderOffset =
      readInteger(Bytes, Test.Is64 ? 32 : 28, Test.Is64 ? 8 : 4, Test.Endian);
  const uint16_t ProgramHeaderSize = static_cast<uint16_t>(
      readInteger(Bytes, Test.Is64 ? 54 : 42, 2, Test.Endian));
  const uint16_t ProgramHeaderCount = static_cast<uint16_t>(
      readInteger(Bytes, Test.Is64 ? 56 : 44, 2, Test.Endian));
  bool HasDynamic = false;
  bool HasEHFrame = false;
  bool HasNonExecutableStack = false;
  bool HasInterpreter = false;
  bool HasRXLoad = false;
  bool HasReadOnlyLoad = false;
  bool HasRWLoad = false;
  for (uint16_t I = 0; I < ProgramHeaderCount; ++I) {
    const uint64_t Offset = ProgramHeaderOffset + I * ProgramHeaderSize;
    const uint32_t Type =
        static_cast<uint32_t>(readInteger(Bytes, Offset, 4, Test.Endian));
    const uint32_t Flags = static_cast<uint32_t>(
        readInteger(Bytes, Offset + (Test.Is64 ? 4 : 24), 4, Test.Endian));
    HasDynamic |= Type == llvm::ELF::PT_DYNAMIC &&
                  Flags == (llvm::ELF::PF_R | llvm::ELF::PF_W);
    HasEHFrame |= Type == llvm::ELF::PT_GNU_EH_FRAME;
    HasNonExecutableStack |=
        Type == llvm::ELF::PT_GNU_STACK && (Flags & llvm::ELF::PF_X) == 0;
    HasInterpreter |= Type == llvm::ELF::PT_INTERP;
    if (Type == llvm::ELF::PT_LOAD) {
      const uint64_t FileOffset = readInteger(
          Bytes, Offset + (Test.Is64 ? 8 : 4), Test.Is64 ? 8 : 4, Test.Endian);
      const uint64_t Address = readInteger(Bytes, Offset + (Test.Is64 ? 16 : 8),
                                           Test.Is64 ? 8 : 4, Test.Endian);
      EXPECT_EQ(FileOffset % 4096, Address % 4096);
      EXPECT_NE(Flags & llvm::ELF::PF_R, 0U);
      EXPECT_FALSE((Flags & llvm::ELF::PF_X) != 0 &&
                   (Flags & llvm::ELF::PF_W) != 0);
      HasRXLoad |= Flags == (llvm::ELF::PF_R | llvm::ELF::PF_X);
      HasReadOnlyLoad |= Flags == llvm::ELF::PF_R;
      HasRWLoad |= Flags == (llvm::ELF::PF_R | llvm::ELF::PF_W);
    }
  }
  EXPECT_TRUE(HasDynamic);
  EXPECT_TRUE(HasEHFrame);
  EXPECT_TRUE(HasNonExecutableStack);
  EXPECT_FALSE(HasInterpreter);
  EXPECT_TRUE(HasRXLoad);
  EXPECT_TRUE(HasReadOnlyLoad);
  EXPECT_TRUE(HasRWLoad);

  llvm::object::SectionRef DynamicStorage;
  const auto *DynamicSection =
      findSection(**Object, ".dynamic", DynamicStorage);
  ASSERT_NE(DynamicSection, nullptr);
  EXPECT_NE(llvm::object::ELFSectionRef(*DynamicSection).getFlags() &
                llvm::ELF::SHF_WRITE,
            0U);
  auto DynamicContent = DynamicSection->getContents();
  ASSERT_TRUE(static_cast<bool>(DynamicContent));
  std::map<uint64_t, uint64_t> DynamicTags;
  const uint8_t AddressSize = Test.Is64 ? 8 : 4;
  const uint8_t DynamicEntrySize = AddressSize * 2;
  const std::vector<WasmEdge::Byte> DynamicBytes(DynamicContent->bytes_begin(),
                                                 DynamicContent->bytes_end());
  for (size_t Offset = 0; Offset < DynamicBytes.size();
       Offset += DynamicEntrySize) {
    const uint64_t Tag =
        readInteger(DynamicBytes, Offset, AddressSize, Test.Endian);
    const uint64_t Value = readInteger(DynamicBytes, Offset + AddressSize,
                                       AddressSize, Test.Endian);
    if (Tag == llvm::ELF::DT_NULL)
      break;
    EXPECT_NE(Tag, llvm::ELF::DT_NEEDED);
    DynamicTags.emplace(Tag, Value);
  }
  struct DynamicSectionCase {
    uint64_t AddressTag;
    uint64_t SizeTag;
    uint64_t EntryTag;
    const char *Name;
    uint64_t EntrySize;
  };
  const std::array<DynamicSectionCase, 4> DynamicSections{{
      {static_cast<uint64_t>(llvm::ELF::DT_HASH), 0, 0, ".hash", 4},
      {static_cast<uint64_t>(llvm::ELF::DT_STRTAB),
       static_cast<uint64_t>(llvm::ELF::DT_STRSZ), 0, ".dynstr", 1},
      {static_cast<uint64_t>(llvm::ELF::DT_SYMTAB), 0,
       static_cast<uint64_t>(llvm::ELF::DT_SYMENT), ".dynsym",
       Test.Is64 ? 24U : 16U},
      {Test.Is64 ? static_cast<uint64_t>(llvm::ELF::DT_RELA)
                 : static_cast<uint64_t>(llvm::ELF::DT_REL),
       Test.Is64 ? static_cast<uint64_t>(llvm::ELF::DT_RELASZ)
                 : static_cast<uint64_t>(llvm::ELF::DT_RELSZ),
       Test.Is64 ? static_cast<uint64_t>(llvm::ELF::DT_RELAENT)
                 : static_cast<uint64_t>(llvm::ELF::DT_RELENT),
       Test.Is64 ? ".rela.dyn" : ".rel.dyn", Test.Is64 ? 24U : 8U},
  }};
  for (const auto &Expected : DynamicSections) {
    llvm::object::SectionRef Storage;
    const auto *Section = findSection(**Object, Expected.Name, Storage);
    ASSERT_NE(Section, nullptr);
    EXPECT_EQ(DynamicTags[Expected.AddressTag], Section->getAddress());
    if (Expected.SizeTag != 0) {
      EXPECT_EQ(DynamicTags[Expected.SizeTag], Section->getSize());
    }
    if (Expected.EntryTag != 0) {
      EXPECT_EQ(DynamicTags[Expected.EntryTag], Expected.EntrySize);
    }
  }

  llvm::object::SectionRef HashStorage;
  const auto *HashSection = findSection(**Object, ".hash", HashStorage);
  ASSERT_NE(HashSection, nullptr);
  auto HashContent = HashSection->getContents();
  ASSERT_TRUE(static_cast<bool>(HashContent));
  const std::vector<WasmEdge::Byte> HashBytes(HashContent->bytes_begin(),
                                              HashContent->bytes_end());
  const uint32_t BucketCount =
      static_cast<uint32_t>(readInteger(HashBytes, 0, 4, Test.Endian));
  const uint32_t ChainCount =
      static_cast<uint32_t>(readInteger(HashBytes, 4, 4, Test.Endian));
  ASSERT_EQ(ChainCount, Symbols.size() + 1);
  std::vector<std::string> SymbolNames(ChainCount);
  uint32_t SymbolIndex = 1;
  for (const auto &Symbol : ELF->getDynamicSymbolIterators()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (!Name->empty())
      SymbolNames[SymbolIndex] = Name->str();
    ++SymbolIndex;
  }
  ASSERT_EQ(SymbolIndex, ChainCount);
  for (uint32_t Wanted = 1; Wanted < ChainCount; ++Wanted) {
    uint32_t Index = static_cast<uint32_t>(readInteger(
        HashBytes, (2 + elfHash(SymbolNames[Wanted]) % BucketCount) * 4, 4,
        Test.Endian));
    std::set<uint32_t> Visited;
    while (Index != 0 && Index != Wanted) {
      ASSERT_LT(Index, ChainCount);
      ASSERT_TRUE(Visited.insert(Index).second);
      Index = static_cast<uint32_t>(readInteger(
          HashBytes, (2 + BucketCount + Index) * 4, 4, Test.Endian));
    }
    EXPECT_EQ(Index, Wanted) << SymbolNames[Wanted];
  }

  bool CheckedRelocation = false;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != (Test.Is64 ? ".rela.dyn" : ".rel.dyn"))
      continue;
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    ASSERT_EQ(Content->size(), Test.Is64 ? 24U : 8U);
    const auto *RelocationBytes =
        reinterpret_cast<const WasmEdge::Byte *>(Content->data());
    const uint64_t Info =
        readInteger(std::vector<WasmEdge::Byte>(
                        RelocationBytes, RelocationBytes + Content->size()),
                    Test.Is64 ? 8 : 4, Test.Is64 ? 8 : 4, Test.Endian);
    EXPECT_EQ(Test.Is64 ? static_cast<uint32_t>(Info)
                        : static_cast<uint32_t>(Info & 0xFF),
              Test.RelativeType);
    EXPECT_EQ(Test.Is64 ? Info >> 32 : Info >> 8, 0U);
    EXPECT_EQ(
        readInteger(std::vector<WasmEdge::Byte>(
                        RelocationBytes, RelocationBytes + Content->size()),
                    0, AddressSize, Test.Endian),
        Graph.sections()[3].Address);
    if (Test.Is64) {
      EXPECT_EQ(
          readInteger(std::vector<WasmEdge::Byte>(
                          RelocationBytes, RelocationBytes + Content->size()),
                      16, 8, Test.Endian),
          Graph.sections()[3].Address);
    }
    if (!Test.Is64) {
      EXPECT_EQ(
          readInteger(Bytes, Graph.sections()[3].FileOffset, 4, Test.Endian),
          Graph.sections()[3].Address);
    }
    CheckedRelocation = true;
  }
  EXPECT_TRUE(CheckedRelocation);

  llvm::object::SectionRef HeaderStorage;
  const auto *HeaderSection =
      findSection(**Object, ".eh_frame_hdr", HeaderStorage);
  ASSERT_NE(HeaderSection, nullptr);
  EXPECT_EQ(llvm::object::ELFSectionRef(*HeaderSection).getFlags() &
                llvm::ELF::SHF_WRITE,
            0U);
  auto HeaderContent = HeaderSection->getContents();
  ASSERT_TRUE(static_cast<bool>(HeaderContent));
  const std::vector<WasmEdge::Byte> HeaderBytes(HeaderContent->bytes_begin(),
                                                HeaderContent->bytes_end());
  ASSERT_EQ(HeaderBytes.size(), 12U + 2U * 8U);
  EXPECT_EQ(HeaderBytes[0], 1);
  EXPECT_EQ(HeaderBytes[1], 0x1B);
  EXPECT_EQ(HeaderBytes[2], 0x03);
  EXPECT_EQ(HeaderBytes[3], 0x3B);
  EXPECT_EQ(readInteger(HeaderBytes, 8, 4, Test.Endian), 2U);
  uint64_t Previous = 0;
  for (size_t I = 0; I < 2; ++I) {
    const auto HeaderAddress =
        static_cast<int64_t>(HeaderSection->getAddress());
    const uint64_t FunctionAddress = static_cast<uint64_t>(
        HeaderAddress + static_cast<int32_t>(readInteger(
                            HeaderBytes, 12 + I * 8, 4, Test.Endian)));
    const uint64_t FDEAddress = static_cast<uint64_t>(
        HeaderAddress + static_cast<int32_t>(readInteger(
                            HeaderBytes, 16 + I * 8, 4, Test.Endian)));
    EXPECT_EQ(FunctionAddress, Graph.sections()[0].Address + I * 2);
    EXPECT_EQ(FDEAddress, Graph.sections()[2].Address + 17 + I * 17);
    if (I != 0) {
      EXPECT_LT(Previous, FunctionAddress);
    }
    Previous = FunctionAddress;
  }
}

TEST(ELFWriterTest, UsesTargetMaximumPageSizeForLoadSegments) {
  for (const auto &Test :
       {ELFCase{Target::ARM, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_ARM),
                static_cast<uint32_t>(llvm::ELF::R_ARM_RELATIVE), false},
        ELFCase{Target::AArch64, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_AARCH64),
                static_cast<uint32_t>(llvm::ELF::R_AARCH64_RELATIVE), true}}) {
    REQUIRE_RELOCATION_HANDLER(Test.Architecture);
    auto Graph = makeELFGraph(Test);
    ASSERT_TRUE(ELFWriter::layout(Graph));
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    ASSERT_TRUE(ELFWriter::write(Graph, Output));

    const uint8_t Width = Test.Is64 ? 8 : 4;
    const uint64_t ProgramHeaderOffset =
        readInteger(Bytes, Test.Is64 ? 32 : 28, Width, Test.Endian);
    const uint16_t ProgramHeaderSize = static_cast<uint16_t>(
        readInteger(Bytes, Test.Is64 ? 54 : 42, 2, Test.Endian));
    const uint16_t ProgramHeaderCount = static_cast<uint16_t>(
        readInteger(Bytes, Test.Is64 ? 56 : 44, 2, Test.Endian));
    size_t LoadCount = 0;
    for (uint16_t I = 0; I < ProgramHeaderCount; ++I) {
      const uint64_t Header = ProgramHeaderOffset + I * ProgramHeaderSize;
      if (readInteger(Bytes, Header, 4, Test.Endian) != llvm::ELF::PT_LOAD)
        continue;
      ++LoadCount;
      const uint64_t FileOffset =
          readInteger(Bytes, Header + (Test.Is64 ? 8 : 4), Width, Test.Endian);
      const uint64_t Address =
          readInteger(Bytes, Header + (Test.Is64 ? 16 : 8), Width, Test.Endian);
      const uint64_t Alignment = readInteger(
          Bytes, Header + (Test.Is64 ? 48 : 28), Width, Test.Endian);
      EXPECT_EQ(Alignment, UINT64_C(65536))
          << architectureName(Test.Architecture);
      EXPECT_EQ(FileOffset % Alignment, Address % Alignment);
    }
    EXPECT_GE(LoadCount, 3U);
  }
}

TEST(ELFWriterTest, KeepsRoundedLoadRangesDisjointWithOverAlignment) {
  for (const auto &Test :
       {ELFCase{Target::ARM, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_ARM),
                static_cast<uint32_t>(llvm::ELF::R_ARM_RELATIVE), false},
        ELFCase{Target::AArch64, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_AARCH64),
                static_cast<uint32_t>(llvm::ELF::R_AARCH64_RELATIVE), true}}) {
    REQUIRE_RELOCATION_HANDLER(Test.Architecture);
    LinkGraph Graph(Test.Architecture, Test.Endian);
    ASSERT_TRUE(Graph.beginInput("over-aligned.o"));
    if (Test.Architecture == Target::ARM) {
      ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                    llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
    }
    ASSERT_TRUE(Graph.addSection(
        Section{".text", SectionKind::Text, 16, 1, 0, 0, {0}}));
    ASSERT_TRUE(
        Graph.addSection(Section{".rodata.a", SectionKind::ReadOnly, 8, 8, 0, 0,
                                 std::vector<WasmEdge::Byte>(8)}));
    constexpr uint64_t OverAlignment = UINT64_C(1) << 20;
    ASSERT_TRUE(Graph.addSection(Section{".rodata.z", SectionKind::ReadOnly,
                                         OverAlignment, 8, 0, 0,
                                         std::vector<WasmEdge::Byte>(8)}));
    ASSERT_TRUE(ELFWriter::layout(Graph));
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    ASSERT_TRUE(ELFWriter::write(Graph, Output));

    const uint8_t Width = Test.Is64 ? 8 : 4;
    const uint64_t ProgramHeaderOffset =
        readInteger(Bytes, Test.Is64 ? 32 : 28, Width, Test.Endian);
    const uint16_t ProgramHeaderSize = static_cast<uint16_t>(
        readInteger(Bytes, Test.Is64 ? 54 : 42, 2, Test.Endian));
    const uint16_t ProgramHeaderCount = static_cast<uint16_t>(
        readInteger(Bytes, Test.Is64 ? 56 : 44, 2, Test.Endian));
    uint64_t PreviousRoundedEnd = 0;
    bool FoundOverAlignedLoad = false;
    for (uint16_t I = 0; I < ProgramHeaderCount; ++I) {
      const uint64_t Header = ProgramHeaderOffset + I * ProgramHeaderSize;
      if (readInteger(Bytes, Header, 4, Test.Endian) != llvm::ELF::PT_LOAD)
        continue;
      const uint64_t FileOffset =
          readInteger(Bytes, Header + (Test.Is64 ? 8 : 4), Width, Test.Endian);
      const uint64_t Address =
          readInteger(Bytes, Header + (Test.Is64 ? 16 : 8), Width, Test.Endian);
      const uint64_t MemorySize = readInteger(
          Bytes, Header + (Test.Is64 ? 40 : 20), Width, Test.Endian);
      const uint64_t Alignment = readInteger(
          Bytes, Header + (Test.Is64 ? 48 : 28), Width, Test.Endian);
      const uint64_t RoundedStart = Address & ~UINT64_C(65535);
      const uint64_t RoundedEnd =
          (Address + MemorySize + UINT64_C(65535)) & ~UINT64_C(65535);
      EXPECT_GE(RoundedStart, PreviousRoundedEnd)
          << architectureName(Test.Architecture);
      EXPECT_EQ(FileOffset % Alignment, Address % Alignment);
      PreviousRoundedEnd = RoundedEnd;
      FoundOverAlignedLoad |= Alignment >= OverAlignment;
    }
    EXPECT_TRUE(FoundOverAlignedLoad);
  }
}

TEST(ELFWriterTest, RejectsRebaseIntoReadOnlySectionBeforeEmission) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("read-only-rebase.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}});
  auto Rodata = Graph.addSection(Section{".rodata", SectionKind::ReadOnly, 8, 8,
                                         0, 0, std::vector<WasmEdge::Byte>(8)});
  ASSERT_TRUE(Text && Rodata);
  auto TargetSymbol = Graph.addSymbol(
      Symbol{"target", *Text, 0, 1, false, std::nullopt, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph.addRelocation(Relocation{*Rodata, 0, llvm::ELF::R_X86_64_64,
                                             *TargetSymbol, 0, false,
                                             ObjectFormat::ELF, 8, false}));
  ASSERT_TRUE(Graph.rebases().empty());
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_EQ(Graph.rebases().size(), 1U);

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  auto Result = ELFWriter::write(Graph, Output);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("non-writable"), std::string::npos);
  EXPECT_EQ(Result.error().Section, *Rodata);
  EXPECT_EQ(Result.error().SectionName, ".rodata");
  EXPECT_EQ(Result.error().Offset, 0U);
  EXPECT_EQ(Result.error().RelocationType, llvm::ELF::R_X86_64_64);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
  EXPECT_TRUE(Bytes.empty());
}

TEST(ELFWriterTest, KeepsLargeBSSOutOfFileAndInLoadMemorySize) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint64_t BSSSize = UINT64_C(4) * 1024 * 1024;
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("large-bss.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(Graph.addSection(Section{".data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)}));
  auto BSS = Graph.addSection(Section{".bss", SectionKind::BSS, 4096, BSSSize});
  ASSERT_TRUE(BSS);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  EXPECT_LT(Bytes.size(), 128U * 1024U);

  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "large-bss.so"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
  llvm::object::SectionRef Storage;
  const auto *BSSSection = findSection(**Object, ".bss", Storage);
  ASSERT_NE(BSSSection, nullptr);
  EXPECT_EQ(llvm::object::ELFSectionRef(*BSSSection).getType(),
            llvm::ELF::SHT_NOBITS);
  llvm::object::SectionRef DynamicStorage;
  const auto *DynamicSection =
      findSection(**Object, ".dynamic", DynamicStorage);
  ASSERT_NE(DynamicSection, nullptr);
  llvm::object::SectionRef DataStorage;
  const auto *DataSection = findSection(**Object, ".data", DataStorage);
  ASSERT_NE(DataSection, nullptr);
  llvm::object::SectionRef DynStrStorage;
  const auto *DynStrSection = findSection(**Object, ".dynstr", DynStrStorage);
  ASSERT_NE(DynStrSection, nullptr);

  const uint64_t ProgramHeaderOffset =
      readInteger(Bytes, 32, 8, Endianness::Little);
  const uint16_t ProgramHeaderSize =
      static_cast<uint16_t>(readInteger(Bytes, 54, 2, Endianness::Little));
  const uint16_t ProgramHeaderCount =
      static_cast<uint16_t>(readInteger(Bytes, 56, 2, Endianness::Little));
  const uint64_t SectionHeaderOffset =
      readInteger(Bytes, 40, 8, Endianness::Little);
  const uint64_t DynamicFileOffset = readInteger(
      Bytes, SectionHeaderOffset + DynamicSection->getIndex() * 64 + 24, 8,
      Endianness::Little);
  const auto SectionOffset = [&](const llvm::object::SectionRef &Section) {
    return readInteger(Bytes,
                       SectionHeaderOffset + Section.getIndex() * 64 + 24, 8,
                       Endianness::Little);
  };
  const uint64_t DataFileOffset = SectionOffset(*DataSection);
  const uint64_t BSSFileOffset = SectionOffset(*BSSSection);
  const uint64_t DynStrFileOffset = SectionOffset(*DynStrSection);
  EXPECT_EQ(BSSFileOffset,
            (DataFileOffset + DataSection->getSize() + 4095) & ~UINT64_C(4095));
  EXPECT_EQ(DynStrFileOffset, BSSFileOffset);
  EXPECT_GE(DynStrSection->getAddress(),
            BSSSection->getAddress() + BSSSection->getSize());
  EXPECT_LT(DynStrFileOffset, DynStrSection->getAddress());
  bool FoundBSSLoad = false;
  bool FoundDynamic = false;
  for (uint16_t I = 0; I < ProgramHeaderCount; ++I) {
    const uint64_t Header = ProgramHeaderOffset + I * ProgramHeaderSize;
    const uint64_t Type = readInteger(Bytes, Header, 4, Endianness::Little);
    if (Type == llvm::ELF::PT_DYNAMIC) {
      FoundDynamic = true;
      EXPECT_EQ(readInteger(Bytes, Header + 8, 8, Endianness::Little),
                DynamicFileOffset);
      EXPECT_EQ(readInteger(Bytes, Header + 16, 8, Endianness::Little),
                DynamicSection->getAddress());
    }
    if (Type != llvm::ELF::PT_LOAD)
      continue;
    const uint64_t FileOffset =
        readInteger(Bytes, Header + 8, 8, Endianness::Little);
    const uint64_t Address =
        readInteger(Bytes, Header + 16, 8, Endianness::Little);
    const uint64_t FileSize =
        readInteger(Bytes, Header + 32, 8, Endianness::Little);
    const uint64_t MemorySize =
        readInteger(Bytes, Header + 40, 8, Endianness::Little);
    const uint64_t Alignment =
        readInteger(Bytes, Header + 48, 8, Endianness::Little);
    EXPECT_EQ(FileOffset % Alignment, Address % Alignment);
    if (BSSSection->getAddress() >= Address &&
        BSSSection->getAddress() < Address + MemorySize) {
      FoundBSSLoad = true;
      EXPECT_EQ(FileOffset + FileSize, DataFileOffset + DataSection->getSize());
      EXPECT_EQ(Address + MemorySize,
                BSSSection->getAddress() + BSSSection->getSize());
      EXPECT_EQ(MemorySize - FileSize,
                BSSSize + BSSSection->getAddress() - (Address + FileSize));
      EXPECT_LT(FileOffset + FileSize, DynStrFileOffset);
    }
  }
  EXPECT_TRUE(FoundBSSLoad);
  EXPECT_TRUE(FoundDynamic);
}

TEST(ELFWriterTest, ReservesZeroFilledFileRangeForShortPROGBITSContent) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("short-progbits.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  auto Short = Graph.addSection(
      Section{".data.a", SectionKind::Data, 1, 16, 0, 0, {1, 2, 3, 4}});
  auto Following = Graph.addSection(
      Section{".data.b", SectionKind::Data, 1, 4, 0, 0, {5, 6, 7, 8}});
  ASSERT_TRUE(Short && Following);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  EXPECT_EQ(Graph.sections()[*Following].FileOffset,
            Graph.sections()[*Short].FileOffset +
                Graph.sections()[*Short].VirtualSize);
  ASSERT_TRUE(applyRelocations(Graph));

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "short-progbits.so"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
  llvm::object::SectionRef ShortStorage;
  const auto *ShortSection = findSection(**Object, ".data.a", ShortStorage);
  ASSERT_NE(ShortSection, nullptr);
  llvm::object::SectionRef FollowingStorage;
  const auto *FollowingSection =
      findSection(**Object, ".data.b", FollowingStorage);
  ASSERT_NE(FollowingSection, nullptr);
  llvm::object::SectionRef DynStrStorage;
  const auto *DynStrSection = findSection(**Object, ".dynstr", DynStrStorage);
  ASSERT_NE(DynStrSection, nullptr);

  const uint64_t SectionHeaderOffset =
      readInteger(Bytes, 40, 8, Endianness::Little);
  const auto SectionOffset = [&](const llvm::object::SectionRef &Section) {
    return readInteger(Bytes,
                       SectionHeaderOffset + Section.getIndex() * 64 + 24, 8,
                       Endianness::Little);
  };
  const uint64_t ShortOffset = SectionOffset(*ShortSection);
  const uint64_t FollowingOffset = SectionOffset(*FollowingSection);
  const uint64_t DynStrOffset = SectionOffset(*DynStrSection);
  EXPECT_EQ(FollowingOffset, ShortOffset + ShortSection->getSize());
  EXPECT_GE(DynStrOffset, FollowingOffset + FollowingSection->getSize());
  ASSERT_LE(ShortOffset + ShortSection->getSize(), Bytes.size());
  EXPECT_EQ(std::vector<WasmEdge::Byte>(Bytes.begin() + ShortOffset,
                                        Bytes.begin() + ShortOffset + 4),
            (std::vector<WasmEdge::Byte>{1, 2, 3, 4}));
  EXPECT_TRUE(std::all_of(Bytes.begin() + ShortOffset + 4,
                          Bytes.begin() + ShortOffset + ShortSection->getSize(),
                          [](WasmEdge::Byte Value) { return Value == 0; }));
  EXPECT_EQ(std::vector<WasmEdge::Byte>(Bytes.begin() + FollowingOffset,
                                        Bytes.begin() + FollowingOffset + 4),
            (std::vector<WasmEdge::Byte>{5, 6, 7, 8}));
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, ELFArchitectureTest,
    testing::Values(
        ELFCase{Target::ARM, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_ARM),
                static_cast<uint32_t>(llvm::ELF::R_ARM_RELATIVE), false},
        ELFCase{Target::X86_64, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_X86_64),
                static_cast<uint32_t>(llvm::ELF::R_X86_64_RELATIVE), true},
        ELFCase{Target::AArch64, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_AARCH64),
                static_cast<uint32_t>(llvm::ELF::R_AARCH64_RELATIVE), true},
        ELFCase{Target::RISCV64, Endianness::Little,
                static_cast<uint16_t>(llvm::ELF::EM_RISCV),
                static_cast<uint32_t>(llvm::ELF::R_RISCV_RELATIVE), true},
        ELFCase{Target::S390X, Endianness::Big,
                static_cast<uint16_t>(llvm::ELF::EM_S390),
                static_cast<uint32_t>(llvm::ELF::R_390_RELATIVE), true}),
    elfCaseName);

TEST(ELFWriterTest, ExportsThumbFunctionWithStateBit) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("thumb.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 2, 0, 0, {0x70, 0x47}});
  auto Exidx = Graph.addSection(Section{".ARM.exidx",
                                        SectionKind::Unwind,
                                        4,
                                        8,
                                        0,
                                        0,
                                        {0, 0, 0, 0, 1, 0, 0, 0},
                                        SectionPurpose::ARMExidx,
                                        0,
                                        *Text});
  ASSERT_TRUE(Text && Exidx);
  ASSERT_TRUE(Graph.addSymbol(
      Symbol{"thumb", *Text, 0, 2, true, std::nullopt, true, true}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_EQ(Graph.sections()[*Text].Address & 1, 0U);
  ASSERT_TRUE(applyRelocations(Graph));

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "thumb.so"));
  ASSERT_TRUE(static_cast<bool>(Object));
  const auto *ELF = llvm::dyn_cast<llvm::object::ELFObjectFileBase>(&**Object);
  ASSERT_NE(ELF, nullptr);
  bool Found = false;
  llvm::object::SectionRef DynamicSymbols;
  const auto *DynamicSymbolSection =
      findSection(**Object, ".dynsym", DynamicSymbols);
  ASSERT_NE(DynamicSymbolSection, nullptr);
  auto DynamicSymbolContent = DynamicSymbolSection->getContents();
  ASSERT_TRUE(static_cast<bool>(DynamicSymbolContent));
  ASSERT_EQ(DynamicSymbolContent->size(), 32U);
  for (const auto &Symbol : ELF->getDynamicSymbolIterators()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "thumb")
      continue;
    const std::vector<WasmEdge::Byte> Content(
        DynamicSymbolContent->bytes_begin(), DynamicSymbolContent->bytes_end());
    EXPECT_EQ(readInteger(Content, 16 + 4, 4, Endianness::Little),
              Graph.sections()[*Text].Address | UINT64_C(1));
    Found = true;
  }
  EXPECT_TRUE(Found);
}

TEST(ELFWriterTest, OmitsEHFrameHeaderWithoutEHFrame) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("no-eh.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "no-eh.so"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef Storage;
  EXPECT_EQ(findSection(**Object, ".eh_frame_hdr", Storage), nullptr);
  const uint64_t ProgramHeaderOffset =
      readInteger(Bytes, 32, 8, Endianness::Little);
  const uint16_t ProgramHeaderSize =
      static_cast<uint16_t>(readInteger(Bytes, 54, 2, Endianness::Little));
  const uint16_t ProgramHeaderCount =
      static_cast<uint16_t>(readInteger(Bytes, 56, 2, Endianness::Little));
  for (uint16_t I = 0; I < ProgramHeaderCount; ++I)
    EXPECT_NE(readInteger(Bytes, ProgramHeaderOffset + I * ProgramHeaderSize, 4,
                          Endianness::Little),
              llvm::ELF::PT_GNU_EH_FRAME);
}

TEST(ELFWriterTest, WritesARMExidxAndHardFloatABI) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("arm.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  auto Exidx = Graph.addSection(Section{".ARM.exidx",
                                        SectionKind::Unwind,
                                        4,
                                        8,
                                        0,
                                        0,
                                        {0, 0, 0, 0, 1, 0, 0, 0},
                                        SectionPurpose::ARMExidx,
                                        0,
                                        *Text});
  ASSERT_TRUE(Text && Exidx);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(Graph.setSectionAddress(*Exidx, Graph.sections()[*Exidx].Address +
                                                  UINT64_C(65536)));
  ASSERT_NE(Graph.sections()[*Exidx].FileOffset,
            Graph.sections()[*Exidx].Address);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto ExidxContent = Graph.sections()[*Exidx].Content;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  if (const char *Fixture = std::getenv("WASMEDGE_ARM_ELF_FIXTURE")) {
    std::ofstream File(Fixture, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Bytes.data()),
               static_cast<std::streamsize>(Bytes.size()));
    ASSERT_TRUE(File);
  }
  EXPECT_EQ(readInteger(Bytes, 36, 4, Endianness::Little),
            llvm::ELF::EF_ARM_EABI_VER5 | llvm::ELF::EF_ARM_ABI_FLOAT_HARD);

  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm.so"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef ExidxStorage;
  const auto *ExidxSection = findSection(**Object, ".ARM.exidx", ExidxStorage);
  ASSERT_NE(ExidxSection, nullptr);
  const llvm::object::ELFSectionRef ELFExidx(*ExidxSection);
  EXPECT_EQ(ELFExidx.getType(), llvm::ELF::SHT_ARM_EXIDX);
  EXPECT_EQ(ELFExidx.getFlags(),
            llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_LINK_ORDER);
  const uint64_t SectionHeaderOffset =
      readInteger(Bytes, 32, 4, Endianness::Little);
  const uint64_t ExidxHeader =
      SectionHeaderOffset + ExidxSection->getIndex() * 40;
  EXPECT_EQ(readInteger(Bytes, ExidxHeader + 24, 4, Endianness::Little), 1U);
  auto Content = ExidxSection->getContents();
  ASSERT_TRUE(static_cast<bool>(Content));
  EXPECT_EQ(
      std::vector<WasmEdge::Byte>(Content->bytes_begin(), Content->bytes_end()),
      ExidxContent);
  const uint64_t ProgramHeaderOffset =
      readInteger(Bytes, 28, 4, Endianness::Little);
  const uint16_t ProgramHeaderSize =
      static_cast<uint16_t>(readInteger(Bytes, 42, 2, Endianness::Little));
  const uint16_t ProgramHeaderCount =
      static_cast<uint16_t>(readInteger(Bytes, 44, 2, Endianness::Little));
  bool HasExidx = false;
  for (uint16_t I = 0; I < ProgramHeaderCount; ++I) {
    const uint64_t Offset = ProgramHeaderOffset + I * ProgramHeaderSize;
    if (readInteger(Bytes, Offset, 4, Endianness::Little) ==
        llvm::ELF::PT_ARM_EXIDX) {
      HasExidx = true;
      EXPECT_EQ(readInteger(Bytes, Offset + 4, 4, Endianness::Little),
                Graph.sections()[*Exidx].FileOffset);
      EXPECT_EQ(readInteger(Bytes, Offset + 8, 4, Endianness::Little),
                Graph.sections()[*Exidx].Address);
      EXPECT_EQ(readInteger(Bytes, Offset + 16, 4, Endianness::Little),
                ExidxSection->getSize());
      EXPECT_EQ(readInteger(Bytes, Offset + 20, 4, Endianness::Little),
                ExidxSection->getSize());
      EXPECT_EQ(readInteger(Bytes, Offset + 28, 4, Endianness::Little), 4U);
    }
  }
  EXPECT_TRUE(HasExidx);
  llvm::object::SectionRef HeaderStorage;
  EXPECT_EQ(findSection(**Object, ".eh_frame_hdr", HeaderStorage), nullptr);
}

TEST(ELFWriterTest, PreservesARMExidxAssociationsAndUsesOneSegment) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("many-arm.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  constexpr size_t SectionCount = 10;
  std::array<SectionId, SectionCount> Texts{};
  std::array<SectionId, SectionCount> Exidxs{};
  for (size_t I = 0; I < SectionCount; ++I) {
    const size_t NameOrdinal = SectionCount - I - 1;
    auto Text =
        Graph.addSection(Section{".text." + std::to_string(NameOrdinal),
                                 SectionKind::Text,
                                 4,
                                 4,
                                 0,
                                 0,
                                 {static_cast<WasmEdge::Byte>(I), 0, 0, 0}});
    ASSERT_TRUE(Text);
    Texts[I] = *Text;
    auto Exidx = Graph.addSection(
        Section{".ARM.exidx." + std::to_string(I),
                SectionKind::Unwind,
                4,
                8,
                0,
                0,
                {static_cast<WasmEdge::Byte>(I), 0, 0, 0, 1, 0, 0, 0},
                SectionPurpose::ARMExidx,
                0,
                *Text});
    ASSERT_TRUE(Exidx);
    Exidxs[I] = *Exidx;
  }
  ASSERT_TRUE(ELFWriter::layout(Graph));
  for (const auto Exidx : Exidxs)
    ASSERT_TRUE(Graph.setSectionAddress(Exidx, Graph.sections()[Exidx].Address +
                                                   UINT64_C(65536)));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "many-arm.so"));
  ASSERT_TRUE(static_cast<bool>(Object));

  const uint64_t SectionHeaderOffset =
      readInteger(Bytes, 32, 4, Endianness::Little);
  uint64_t FirstAddress = UINT64_MAX;
  uint64_t FirstFileOffset = UINT64_MAX;
  uint64_t LastEnd = 0;
  std::vector<std::pair<uint64_t, uint64_t>> ExidxOrder;
  for (size_t I = 0; I < SectionCount; ++I) {
    llvm::object::SectionRef Storage;
    const auto *Exidx =
        findSection(**Object, ".ARM.exidx." + std::to_string(I), Storage);
    ASSERT_NE(Exidx, nullptr);
    const uint64_t Header = SectionHeaderOffset + Exidx->getIndex() * 40;
    EXPECT_EQ(readInteger(Bytes, Header + 24, 4, Endianness::Little),
              Texts[I] + 1);
    FirstAddress = std::min(FirstAddress, Exidx->getAddress());
    FirstFileOffset =
        std::min(FirstFileOffset, Graph.sections()[Exidxs[I]].FileOffset);
    LastEnd = std::max(LastEnd, Exidx->getAddress() + Exidx->getSize());
    ExidxOrder.emplace_back(Exidx->getAddress(),
                            Graph.sections()[Texts[I]].Address);
  }
  std::sort(ExidxOrder.begin(), ExidxOrder.end());
  for (size_t I = 1; I < ExidxOrder.size(); ++I)
    EXPECT_LT(ExidxOrder[I - 1].second, ExidxOrder[I].second);
  const uint64_t ProgramHeaderOffset =
      readInteger(Bytes, 28, 4, Endianness::Little);
  const uint16_t ProgramHeaderSize =
      static_cast<uint16_t>(readInteger(Bytes, 42, 2, Endianness::Little));
  const uint16_t ProgramHeaderCount =
      static_cast<uint16_t>(readInteger(Bytes, 44, 2, Endianness::Little));
  size_t ExidxSegmentCount = 0;
  for (uint16_t I = 0; I < ProgramHeaderCount; ++I) {
    const uint64_t Offset = ProgramHeaderOffset + I * ProgramHeaderSize;
    if (readInteger(Bytes, Offset, 4, Endianness::Little) !=
        llvm::ELF::PT_ARM_EXIDX)
      continue;
    ++ExidxSegmentCount;
    EXPECT_EQ(readInteger(Bytes, Offset + 4, 4, Endianness::Little),
              FirstFileOffset);
    EXPECT_EQ(readInteger(Bytes, Offset + 8, 4, Endianness::Little),
              FirstAddress);
    EXPECT_EQ(readInteger(Bytes, Offset + 16, 4, Endianness::Little),
              LastEnd - FirstAddress);
    EXPECT_EQ(readInteger(Bytes, Offset + 20, 4, Endianness::Little),
              LastEnd - FirstAddress);
    EXPECT_EQ(readInteger(Bytes, Offset + 28, 4, Endianness::Little), 4U);
  }
  EXPECT_EQ(ExidxSegmentCount, 1U);
}

TEST(ELFWriterTest, RejectsMalformedARMExidxContent) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  for (const auto &[VirtualSize, ContentSize] :
       std::array<std::pair<uint64_t, size_t>, 2>{{{8, 4}, {4, 4}}}) {
    LinkGraph Graph(Target::ARM, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("malformed-arm.o"));
    ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                  llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
    auto Text = Graph.addSection(
        Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Text);
    ASSERT_TRUE(Graph.addSection(
        Section{".ARM.exidx", SectionKind::Unwind, 4, VirtualSize, 0, 0,
                std::vector<WasmEdge::Byte>(ContentSize),
                SectionPurpose::ARMExidx, 0, *Text}));
    ASSERT_TRUE(ELFWriter::layout(Graph));
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    EXPECT_FALSE(ELFWriter::write(Graph, Output));
    EXPECT_TRUE(Bytes.empty());
  }
}

TEST(ELFWriterTest, RejectsContentForEmptyARMExidx) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("malformed-empty-arm.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto Exidx = Graph.addSection(Section{".ARM.exidx", SectionKind::Unwind, 4, 8,
                                        0, 0, std::vector<WasmEdge::Byte>(8),
                                        SectionPurpose::ARMExidx, 0, *Text});
  ASSERT_TRUE(Exidx);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  Sections[*Exidx].VirtualSize = 0;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  auto Result = ELFWriter::write(Graph, Output);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "invalid ARM exception index section");
  EXPECT_TRUE(Bytes.empty());
}

TEST(ELFWriterTest, OmitsEmptyARMExidxFromProgramHeaders) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("empty-arm.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(Graph.addSection(Section{".ARM.exidx",
                                       SectionKind::Unwind,
                                       4,
                                       0,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::ARMExidx,
                                       0,
                                       *Text}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  const uint64_t ProgramHeaderOffset =
      readInteger(Bytes, 28, 4, Endianness::Little);
  const uint16_t ProgramHeaderSize =
      static_cast<uint16_t>(readInteger(Bytes, 42, 2, Endianness::Little));
  const uint16_t ProgramHeaderCount =
      static_cast<uint16_t>(readInteger(Bytes, 44, 2, Endianness::Little));
  for (uint16_t I = 0; I < ProgramHeaderCount; ++I)
    EXPECT_NE(readInteger(Bytes, ProgramHeaderOffset + I * ProgramHeaderSize, 4,
                          Endianness::Little),
              llvm::ELF::PT_ARM_EXIDX);
}

TEST(ELFWriterTest, RejectsConflictingARMABIFlags) {
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("arm.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD |
                                llvm::ELF::EF_ARM_ABI_FLOAT_SOFT));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  if (!hasRelocationHandler(Target::ARM)) {
    GTEST_SKIP() << "ARM relocation handler is not compiled";
  }
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(ELFWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(ELFWriterTest, AcceptsDefinedPCRelativePersonality) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("personality.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 16, 4, 0, 0, {0xC3, 0, 0xC3, 0}});
  auto EH = Graph.addSection(
      Section{".eh_frame", SectionKind::Unwind, 8,
              makePersonalityEHFrame(Endianness::Little, 0x1B).size(), 0, 0,
              makePersonalityEHFrame(Endianness::Little, 0x1B),
              SectionPurpose::EHFrame});
  ASSERT_TRUE(Text && EH);
  ASSERT_TRUE(
      Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true, std::nullopt, true}));
  ASSERT_TRUE(Graph.addSymbol(
      Symbol{"personality", *Text, 2, 1, false, std::nullopt, false}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  const uint64_t PersonalityField = Graph.sections()[*EH].Address + 18;
  const int64_t PersonalityDelta =
      static_cast<int64_t>(Graph.sections()[*Text].Address + 2) -
      static_cast<int64_t>(PersonalityField);
  const uint64_t FunctionField = Graph.sections()[*EH].Address + 31;
  const int64_t FunctionDelta =
      static_cast<int64_t>(Graph.sections()[*Text].Address) -
      static_cast<int64_t>(FunctionField);
  Graph = rebuildWithSectionContent(Graph, *EH, [&](auto &Content) {
    for (uint8_t I = 0; I < 4; ++I) {
      Content[18 + I] = static_cast<WasmEdge::Byte>(
          static_cast<uint32_t>(PersonalityDelta) >> (I * 8));
      Content[31 + I] = static_cast<WasmEdge::Byte>(
          static_cast<uint32_t>(FunctionDelta) >> (I * 8));
    }
  });
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_TRUE(ELFWriter::write(Graph, Output));
}

TEST(ELFWriterTest, RejectsUnsupportedPersonalityEncodings) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  for (const uint8_t Encoding : {uint8_t{0x9B}, uint8_t{0x03}}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("personality.o"));
    ASSERT_TRUE(Graph.addSection(
        Section{".text", SectionKind::Text, 16, 4, 0, 0, {0xC3, 0, 0xC3, 0}}));
    ASSERT_TRUE(Graph.addSection(
        Section{".eh_frame", SectionKind::Unwind, 8,
                makePersonalityEHFrame(Endianness::Little, Encoding).size(), 0,
                0, makePersonalityEHFrame(Endianness::Little, Encoding),
                SectionPurpose::EHFrame}));
    ASSERT_TRUE(ELFWriter::layout(Graph));
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    auto Result = ELFWriter::write(Graph, Output);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Section, 1U);
    EXPECT_EQ(Result.error().SectionName, ".eh_frame");
    if (Encoding == 0x03) {
      EXPECT_NE(Result.error().Message.find("personality encoding"),
                std::string::npos);
      EXPECT_EQ(Result.error().Offset, 17U);
      EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
    } else {
      EXPECT_NE(Result.error().Message.find("personality target"),
                std::string::npos);
      EXPECT_EQ(Result.error().Offset, 18U);
      EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
    }
    EXPECT_TRUE(Bytes.empty());
  }
}

TEST(ELFWriterTest, AcceptsIndirectPersonalityWithRelativeSlot) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  for (const bool HasRebase : {false, true}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("indirect-personality.o"));
    auto Text = Graph.addSection(
        Section{".text", SectionKind::Text, 16, 4, 0, 0, {0xC3, 0, 0xC3, 0}});
    auto Data = Graph.addSection(Section{".data", SectionKind::Data, 8, 8, 0, 0,
                                         std::vector<WasmEdge::Byte>(8)});
    auto EH = Graph.addSection(
        Section{".eh_frame", SectionKind::Unwind, 8,
                makePersonalityEHFrame(Endianness::Little, 0x9B).size(), 0, 0,
                makePersonalityEHFrame(Endianness::Little, 0x9B),
                SectionPurpose::EHFrame});
    ASSERT_TRUE(Text && Data && EH);
    ASSERT_TRUE(
        Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true, std::nullopt, true}));
    ASSERT_TRUE(Graph.addSymbol(
        Symbol{"personality", *Text, 2, 1, false, std::nullopt, false}));
    ASSERT_TRUE(ELFWriter::layout(Graph));
    Graph = rebuildWithSectionContent(Graph, *Data, [&](auto &DataContent) {
      for (uint8_t I = 0; I < 8; ++I)
        DataContent[I] = static_cast<WasmEdge::Byte>(
            (Graph.sections()[*Text].Address + 2) >> (I * 8));
    });
    const int64_t SlotDelta =
        static_cast<int64_t>(Graph.sections()[*Data].Address) -
        static_cast<int64_t>(Graph.sections()[*EH].Address + 18);
    const int64_t FunctionDelta =
        static_cast<int64_t>(Graph.sections()[*Text].Address) -
        static_cast<int64_t>(Graph.sections()[*EH].Address + 31);
    Graph = rebuildWithSectionContent(Graph, *EH, [&](auto &EHContent) {
      for (uint8_t I = 0; I < 4; ++I) {
        EHContent[18 + I] = static_cast<WasmEdge::Byte>(
            static_cast<uint32_t>(SlotDelta) >> (I * 8));
        EHContent[31 + I] = static_cast<WasmEdge::Byte>(
            static_cast<uint32_t>(FunctionDelta) >> (I * 8));
      }
    });
    if (HasRebase) {
      ASSERT_TRUE(Graph.addRebase(Rebase{
          *Data, 0, static_cast<uint32_t>(llvm::ELF::R_X86_64_64), 0, 8}));
    }
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    EXPECT_EQ(static_cast<bool>(ELFWriter::write(Graph, Output)), HasRebase);
    if (HasRebase) {
      auto Object =
          llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
              llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                              Bytes.size()),
              "indirect.so"));
      ASSERT_TRUE(static_cast<bool>(Object));
      llvm::object::SectionRef Storage;
      const auto *Relocations = findSection(**Object, ".rela.dyn", Storage);
      ASSERT_NE(Relocations, nullptr);
      EXPECT_EQ(Relocations->getSize(), 24U);
    }
  }
}

TEST(ELFWriterTest, RejectsCIEFieldsCrossingRecordBoundary) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Bytes = makeEHFrame(Endianness::Little);
  Bytes[12] = 0x80;
  Bytes[13] = 0x80;
  Bytes[14] = 0x80;
  Bytes[15] = 0x80;
  Bytes[16] = 0x80;
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("bounded-eh.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 4, 0, 0, {0xC3, 0, 0, 0}}));
  ASSERT_TRUE(Graph.addSection(Section{".eh_frame", SectionKind::Unwind, 8,
                                       Bytes.size(), 0, 0, std::move(Bytes),
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> OutputBytes;
  Writer Output(OutputBytes);
  auto Result = ELFWriter::write(Graph, Output);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("alignment fields"), std::string::npos);
  EXPECT_EQ(Result.error().Section, 1U);
  EXPECT_EQ(Result.error().SectionName, ".eh_frame");
  EXPECT_EQ(Result.error().Offset, 17U);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
  EXPECT_TRUE(OutputBytes.empty());
}

TEST(ELFWriterTest, AggregatesMultipleEHFrameSections) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("multiple-eh.o"));
  auto Text = Graph.addSection(Section{".text",
                                       SectionKind::Text,
                                       16,
                                       8,
                                       0,
                                       0,
                                       {0xC3, 0, 0xC3, 0, 0xC3, 0, 0xC3}});
  ASSERT_TRUE(Text);
  std::array<SectionId, 2> EHSections{};
  for (size_t I = 0; I < EHSections.size(); ++I) {
    auto EH = Graph.addSection(
        Section{".eh_frame." + std::to_string(I), SectionKind::Unwind, 8,
                makeEHFrame(Endianness::Little).size(), 0, 0,
                makeEHFrame(Endianness::Little), SectionPurpose::EHFrame});
    ASSERT_TRUE(EH);
    EHSections[I] = *EH;
  }
  ASSERT_TRUE(ELFWriter::layout(Graph));
  for (size_t SectionIndex = 0; SectionIndex < EHSections.size();
       ++SectionIndex) {
    Graph = rebuildWithSectionContent(
        Graph, EHSections[SectionIndex], [&](auto &Content) {
          for (size_t I = 0; I < 2; ++I) {
            const size_t FieldOffset = 17 + I * 17 + 8;
            const uint64_t FieldAddress =
                Graph.sections()[EHSections[SectionIndex]].Address +
                FieldOffset;
            const uint64_t FunctionAddress =
                Graph.sections()[*Text].Address + (SectionIndex * 2 + I) * 2;
            const int64_t Delta = static_cast<int64_t>(FunctionAddress) -
                                  static_cast<int64_t>(FieldAddress);
            for (uint8_t Byte = 0; Byte < 4; ++Byte)
              Content[FieldOffset + Byte] = static_cast<WasmEdge::Byte>(
                  static_cast<uint32_t>(Delta) >> (Byte * 8));
          }
        });
  }
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(ELFWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "multiple-eh.so"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef Storage;
  const auto *Header = findSection(**Object, ".eh_frame_hdr", Storage);
  ASSERT_NE(Header, nullptr);
  auto Content = Header->getContents();
  ASSERT_TRUE(static_cast<bool>(Content));
  const std::vector<WasmEdge::Byte> HeaderBytes(Content->bytes_begin(),
                                                Content->bytes_end());
  EXPECT_EQ(readInteger(HeaderBytes, 8, 4, Endianness::Little), 4U);
  EXPECT_EQ(HeaderBytes.size(), 12U + 4U * 8U);
}

TEST(ELFWriterTest, RejectsELF32LayoutOverflowAtomically) {
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("overflow.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 1, UINT64_C(1) << 32, 0, 0, {0}}));
  EXPECT_FALSE(ELFWriter::layout(Graph));
  EXPECT_EQ(Graph.sections()[0].Address, 0U);
  EXPECT_EQ(Graph.sections()[0].FileOffset, 0U);
}

TEST(ELFWriterTest, RejectsELF32GeneratedMetadataOverflowAtomically) {
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("metadata-overflow.o"));
  ASSERT_TRUE(Graph.setELFFlags(llvm::ELF::EF_ARM_EABI_VER5 |
                                llvm::ELF::EF_ARM_ABI_FLOAT_HARD));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(Graph.setSectionAddress(*Text, UINT32_MAX - 2047));
  ASSERT_TRUE(Graph.setSectionFileOffset(*Text, UINT32_MAX - 2047));
  if (!hasRelocationHandler(Target::ARM)) {
    GTEST_SKIP() << "ARM relocation handler is not compiled";
  }
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(ELFWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(ELFWriterTest, RejectsUnsupportedRebasesAndInvalidState) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  const ELFCase Test{Target::X86_64, Endianness::Little,
                     static_cast<uint16_t>(llvm::ELF::EM_X86_64),
                     static_cast<uint32_t>(llvm::ELF::R_X86_64_RELATIVE), true};
  auto Graph = makeELFGraph(Test);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  auto &RebaseValue = const_cast<std::vector<Rebase> &>(Graph.rebases())[0];
  RebaseValue.Width = 4;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(ELFWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());

  LinkGraph WrongEndian(Target::X86_64, Endianness::Big);
  ASSERT_TRUE(WrongEndian.beginInput("wrong.o"));
  ASSERT_TRUE(WrongEndian.addSection(
      Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  EXPECT_FALSE(ELFWriter::layout(WrongEndian));
}

TEST(ELFWriterTest, RejectsSectionContentOutsideOutput) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  const ELFCase Test{Target::X86_64, Endianness::Little,
                     static_cast<uint16_t>(llvm::ELF::EM_X86_64),
                     static_cast<uint32_t>(llvm::ELF::R_X86_64_RELATIVE), true};
  auto Graph = makeELFGraph(Test);
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  const_cast<Section &>(Graph.sections()[0]).FileOffset = UINT64_MAX;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(ELFWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());
}

LinkGraph makeMachOGraph(Target Architecture) {
  LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::MachO);
  EXPECT_TRUE(Graph.beginInput("writer.o"));
  auto Text = Graph.addSection(
      Section{"__text", SectionKind::Text, 16, 4, 0, 0,
              Architecture == Target::X86_64
                  ? std::vector<WasmEdge::Byte>{0xC3, 0, 0, 0}
                  : std::vector<WasmEdge::Byte>{0xC0, 0x03, 0x5F, 0xD6}});
  auto Constant =
      Graph.addSection(Section{"__const", SectionKind::ReadOnly, 8, 8, 0, 0,
                               std::vector<WasmEdge::Byte>(8)});
  auto EHFrame = Graph.addSection(Section{"__eh_frame",
                                          SectionKind::Unwind,
                                          8,
                                          4,
                                          0,
                                          0,
                                          {0, 0, 0, 0},
                                          SectionPurpose::EHFrame});
  auto Data = Graph.addSection(Section{"__data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)});
  auto Pointer =
      Graph.addSection(Section{"__pointer", SectionKind::Data, 8, 8, 0, 0,
                               std::vector<WasmEdge::Byte>(8)});
  auto BSS =
      Graph.addSection(Section{"__bss", SectionKind::BSS, 8, 8, 0, 0, {}});
  EXPECT_TRUE(Text && Constant && EHFrame && Data && Pointer && BSS);
  EXPECT_TRUE(
      Graph.addSymbol(Symbol{"_f0", *Text, 0, 4, true, std::nullopt, true}));
  EXPECT_TRUE(
      Graph.addSymbol(Symbol{"_value", *Data, 0, 8, true, std::nullopt, true}));
  EXPECT_TRUE(Graph.addRebase(
      Rebase{*Constant, 0,
             Architecture == Target::X86_64
                 ? static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED)
                 : static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED),
             0, 8, ObjectFormat::MachO}));
  EXPECT_TRUE(Graph.addRebase(
      Rebase{*Data, 0,
             Architecture == Target::X86_64
                 ? static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED)
                 : static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED),
             0, 8, ObjectFormat::MachO}));
  EXPECT_TRUE(Graph.addRebase(
      Rebase{*Pointer, 0,
             Architecture == Target::X86_64
                 ? static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED)
                 : static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED),
             0, 8, ObjectFormat::MachO}));
  return Graph;
}

LinkGraph makeMachOCompactGraph(Target Architecture, size_t FunctionCount) {
  LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::MachO);
  EXPECT_TRUE(Graph.beginInput("compact-writer.o"));
  const uint64_t TextSize = std::max<size_t>(FunctionCount, 1) * 16;
  auto Text = Graph.addSection(
      Section{"__text", SectionKind::Text, 16, TextSize, 0, 0,
              std::vector<WasmEdge::Byte>(static_cast<size_t>(TextSize)),
              SectionPurpose::Default, 0x1000});
  auto EH = Graph.addSection(Section{"__eh_frame", SectionKind::Unwind, 8, 32,
                                     0, 0, std::vector<WasmEdge::Byte>(32),
                                     SectionPurpose::EHFrame});
  auto LSDA =
      Graph.addSection(Section{"__gcc_except_tab", SectionKind::ReadOnly, 4, 4,
                               0, 0, std::vector<WasmEdge::Byte>(4)});
  EXPECT_TRUE(Text && EH && LSDA);
  for (size_t I = 0; I < FunctionCount; ++I) {
    auto Function = Graph.addSymbol(
        Symbol{"_f" + std::to_string(I), *Text, I * 16, 16, false, {}, true});
    EXPECT_TRUE(Function);
  }
  return Graph;
}

const Section *findGraphSection(const LinkGraph &Graph,
                                SectionPurpose Purpose) {
  const auto Result = std::find_if(
      Graph.sections().begin(), Graph.sections().end(),
      [&](const auto &Section) { return Section.Purpose == Purpose; });
  return Result == Graph.sections().end() ? nullptr : &*Result;
}

TEST(MachOWriterTest, BuildsCompressedUnwindInfoAndWritesFinalSection) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  constexpr size_t FunctionCount = 1022;
  auto Graph = makeMachOCompactGraph(Target::AArch64, FunctionCount);
  auto LSDA = Graph.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(LSDA);
  for (size_t I = 0; I < FunctionCount; ++I) {
    const uint32_t Encoding =
        I % 2 == 0 ? UINT32_C(0x02001000) : UINT32_C(0x04000001);
    ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{
        static_cast<SymbolId>(I),
        16,
        I == 7 ? Encoding | UINT32_C(0x40000000) : Encoding,
        {},
        I == 7 ? std::optional<SymbolId>{*LSDA} : std::optional<SymbolId>{},
        {}}));
  }
  auto Size = machOUnwindInfoSize(Graph);
  ASSERT_TRUE(Size);
  EXPECT_EQ((*Size - 28 - 3 * 4 - 3 * 12 - 8) % 4096, 0U);
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  const auto *Unwind = findGraphSection(Graph, SectionPurpose::UnwindInfo);
  ASSERT_NE(Unwind, nullptr);
  const auto &Content = Unwind->Content;
  ASSERT_EQ(Content.size(), *Size);
  EXPECT_EQ(readInteger(Content, 0, 4, Endianness::Little), 1U);
  EXPECT_EQ(readInteger(Content, 4, 4, Endianness::Little), 28U);
  EXPECT_EQ(readInteger(Content, 8, 4, Endianness::Little), 3U);
  EXPECT_EQ(readInteger(Content, 16, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Content, 24, 4, Endianness::Little), 3U);
  const size_t Index = readInteger(Content, 20, 4, Endianness::Little);
  const size_t FirstPage =
      readInteger(Content, Index + 4, 4, Endianness::Little);
  const size_t SecondPage =
      readInteger(Content, Index + 16, 4, Endianness::Little);
  EXPECT_EQ(readInteger(Content, FirstPage, 4, Endianness::Little), 3U);
  EXPECT_EQ(readInteger(Content, SecondPage, 4, Endianness::Little), 2U);
  EXPECT_EQ(SecondPage - FirstPage, 4096U);
  const size_t LSDAIndex =
      readInteger(Content, Index + 8, 4, Endianness::Little);
  EXPECT_EQ(readInteger(Content, LSDAIndex, 4, Endianness::Little),
            Graph.sections()[0].Address + 7 * 16);
  EXPECT_EQ(readInteger(Content, LSDAIndex + 4, 4, Endianness::Little),
            Graph.sections()[2].Address);

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef Storage;
  EXPECT_NE(findSection(**Object, "__unwind_info", Storage), nullptr);
  EXPECT_EQ(findSection(**Object, "__compact_unwind", Storage), nullptr);
}

TEST(MachOWriterTest, SelectsRegularPagesAndPreservesNonMergeableRecords) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr size_t FunctionCount = 300;
  auto Graph = makeMachOCompactGraph(Target::X86_64, FunctionCount);
  for (size_t I = 0; I < FunctionCount; ++I) {
    const uint32_t First = static_cast<uint32_t>(I % 5 + 1);
    uint32_t Second = static_cast<uint32_t>((I / 5) % 4 + 1);
    if (Second >= First)
      ++Second;
    ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{
        static_cast<SymbolId>(I),
        16,
        UINT32_C(0x01000000) | (static_cast<uint32_t>(I / 20 + 1) << 16) |
            First | (Second << 3),
        {},
        {},
        {}}));
  }
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  const auto *Unwind = findGraphSection(Graph, SectionPurpose::UnwindInfo);
  ASSERT_NE(Unwind, nullptr);
  const size_t Index = readInteger(Unwind->Content, 20, 4, Endianness::Little);
  const size_t Page =
      readInteger(Unwind->Content, Index + 4, 4, Endianness::Little);
  EXPECT_EQ(readInteger(Unwind->Content, Page, 4, Endianness::Little), 2U);
  EXPECT_EQ(readInteger(Unwind->Content, Page + 6, 2, Endianness::Little),
            FunctionCount);
}

TEST(MachOWriterTest, EncodesDwarfFallbackFromEHFrameBase) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  auto FDE = Graph.addSymbol(Symbol{"fde", 1, 12, 0, false});
  ASSERT_TRUE(FDE);
  ASSERT_TRUE(Graph.addEHFrameReference(EHFrameReference{1, 20, 0}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x03000000, {}, {}, *FDE}));
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  const auto *Unwind = findGraphSection(Graph, SectionPurpose::UnwindInfo);
  ASSERT_NE(Unwind, nullptr);
  const size_t Index = readInteger(Unwind->Content, 20, 4, Endianness::Little);
  const size_t Page =
      readInteger(Unwind->Content, Index + 4, 4, Endianness::Little);
  ASSERT_EQ(readInteger(Unwind->Content, Page, 4, Endianness::Little), 2U);
  EXPECT_EQ(readInteger(Unwind->Content, Page + 12, 4, Endianness::Little),
            UINT32_C(0x0300000C));
}

TEST(MachOWriterTest, RejectsInvalidAArch64NativeCompactUnwind) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Personality = makeMachOCompactGraph(Target::AArch64, 1);
  auto PersonalitySymbol =
      Personality.addSymbol(Symbol{"personality", 0, 0, 0, false});
  ASSERT_TRUE(PersonalitySymbol);
  ASSERT_TRUE(Personality.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, *PersonalitySymbol, {}, {}}));
  EXPECT_FALSE(machOUnwindInfoSize(Personality));

  auto TooManyPersonalities = makeMachOCompactGraph(Target::AArch64, 4);
  for (size_t I = 0; I < 4; ++I) {
    auto PersonalityValue = TooManyPersonalities.addSymbol(
        Symbol{"personality" + std::to_string(I), 0, I * 16, 0, false});
    ASSERT_TRUE(PersonalityValue);
    ASSERT_TRUE(TooManyPersonalities.addCompactUnwind(CompactUnwindRecord{
        static_cast<SymbolId>(I), 16, 0x02000000, *PersonalityValue, {}, {}}));
  }
  EXPECT_FALSE(machOUnwindInfoSize(TooManyPersonalities));

  auto MissingLSDA = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(MissingLSDA.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x42000000, {}, {}, {}}));
  EXPECT_FALSE(machOUnwindInfoSize(MissingLSDA));

  auto UnexpectedLSDA = makeMachOCompactGraph(Target::AArch64, 1);
  auto UnexpectedLSDASymbol =
      UnexpectedLSDA.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(UnexpectedLSDASymbol);
  ASSERT_TRUE(UnexpectedLSDA.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, *UnexpectedLSDASymbol, {}}));
  EXPECT_FALSE(machOUnwindInfoSize(UnexpectedLSDA));

  auto MissingFDE = makeMachOCompactGraph(Target::AArch64, 1);
  auto FDE = MissingFDE.addSymbol(Symbol{"fde", 1, 0, 0, false});
  ASSERT_TRUE(FDE);
  ASSERT_TRUE(MissingFDE.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x03000000, {}, {}, *FDE}));
  EXPECT_FALSE(machOUnwindInfoSize(MissingFDE));

  auto DwarfLSDA = makeMachOCompactGraph(Target::AArch64, 1);
  auto DwarfFDE = DwarfLSDA.addSymbol(Symbol{"fde", 1, 12, 0, false});
  auto DwarfLSDASymbol = DwarfLSDA.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(DwarfFDE && DwarfLSDASymbol);
  ASSERT_TRUE(DwarfLSDA.addEHFrameReference(EHFrameReference{1, 20, 0}));
  ASSERT_TRUE(DwarfLSDA.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x03000000, {}, *DwarfLSDASymbol, *DwarfFDE}));
  EXPECT_FALSE(machOUnwindInfoSize(DwarfLSDA));

  auto MultipleEH = makeMachOCompactGraph(Target::AArch64, 1);
  auto MultipleFDE = MultipleEH.addSymbol(Symbol{"fde", 1, 12, 0, false});
  ASSERT_TRUE(MultipleFDE);
  ASSERT_TRUE(MultipleEH.addEHFrameReference(EHFrameReference{1, 20, 0}));
  ASSERT_TRUE(MultipleEH.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x03000000, {}, {}, *MultipleFDE}));
  ASSERT_TRUE(MultipleEH.addSection(
      Section{"__eh_frame", SectionKind::Unwind, 8, 4, 0, 0,
              std::vector<WasmEdge::Byte>(4), SectionPurpose::EHFrame}));
  EXPECT_FALSE(machOUnwindInfoSize(MultipleEH));

  for (const auto Purpose :
       {SectionPurpose::UnwindInfo, SectionPurpose::CompactUnwind}) {
    auto Existing = makeMachOCompactGraph(Target::AArch64, 1);
    ASSERT_TRUE(Existing.addCompactUnwind(
        CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
    ASSERT_TRUE(Existing.addSection(
        Section{Purpose == SectionPurpose::UnwindInfo ? "__unwind_info"
                                                      : "__compact_unwind",
                SectionKind::Unwind, 4, 4, 0, 0, std::vector<WasmEdge::Byte>(4),
                Purpose}));
    EXPECT_FALSE(reserveMachOUnwindInfo(Existing));
  }

  auto Overflow = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Overflow.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(reserveMachOUnwindInfo(Overflow));
  ASSERT_TRUE(MachOWriter::layout(Overflow));
  ASSERT_TRUE(Overflow.setSectionAddress(0, UINT64_C(1) << 32));
  ASSERT_TRUE(applyRelocations(Overflow));
  EXPECT_FALSE(populateMachOUnwindInfo(Overflow));

  auto LSDAOverflow = makeMachOCompactGraph(Target::AArch64, 1);
  auto LSDA = LSDAOverflow.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(LSDA);
  ASSERT_TRUE(LSDAOverflow.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x42000000, {}, *LSDA, {}}));
  ASSERT_TRUE(reserveMachOUnwindInfo(LSDAOverflow));
  ASSERT_TRUE(MachOWriter::layout(LSDAOverflow));
  ASSERT_TRUE(LSDAOverflow.setSectionAddress(2, UINT64_C(1) << 32));
  ASSERT_TRUE(applyRelocations(LSDAOverflow));
  EXPECT_FALSE(populateMachOUnwindInfo(LSDAOverflow));

  auto Malformed = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Malformed.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  auto &Record =
      const_cast<CompactUnwindRecord &>(Malformed.compactUnwind()[0]);
  Record.Encoding = 0x0F000000;
  EXPECT_FALSE(machOUnwindInfoSize(Malformed));

  for (const uint32_t Encoding : {UINT32_C(0x02000020), UINT32_C(0x02000040),
                                  UINT32_C(0x04000020), UINT32_C(0x04001000)}) {
    auto InvalidARM = makeMachOCompactGraph(Target::AArch64, 1);
    ASSERT_TRUE(InvalidARM.addCompactUnwind(
        CompactUnwindRecord{0, 16, Encoding, {}, {}, {}}));
    EXPECT_FALSE(machOUnwindInfoSize(InvalidARM)) << Encoding;
  }
}

TEST(MachOWriterTest, RejectsInvalidX86_64NativeCompactUnwind) {
  for (const uint32_t Encoding : {UINT32_C(0x01000006), UINT32_C(0x01000009),
                                  UINT32_C(0x02000000), UINT32_C(0x03000000)}) {
    auto InvalidX86 = makeMachOCompactGraph(Target::X86_64, 1);
    ASSERT_TRUE(InvalidX86.addCompactUnwind(
        CompactUnwindRecord{0, 16, Encoding, {}, {}, {}}));
    EXPECT_FALSE(machOUnwindInfoSize(InvalidX86)) << Encoding;
  }
  auto ValidHoles = makeMachOCompactGraph(Target::X86_64, 1);
  ASSERT_TRUE(ValidHoles.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x01040081, {}, {}, {}}));
  EXPECT_TRUE(machOUnwindInfoSize(ValidHoles));
}

TEST(MachOWriterTest, RejectsArm64eCompactModeForGenericAArch64) {
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x05000000, {}, {}, {}}));
  EXPECT_FALSE(machOUnwindInfoSize(Graph));
}

TEST(MachOWriterTest, SizesUnwindInfoAcrossRegularPageBoundary) {
  const auto MakeGraph = [](size_t Count) {
    auto Graph = makeMachOCompactGraph(Target::AArch64, Count);
    for (size_t I = 0; I < Count; ++I)
      EXPECT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{
          static_cast<SymbolId>(I), 16, 0x02000000, {}, {}, {}}));
    return Graph;
  };

  auto LastOnePage = MakeGraph(511);
  auto FirstTwoPages = MakeGraph(512);
  auto OnePageSize = machOUnwindInfoSize(LastOnePage);
  auto TwoPageSize = machOUnwindInfoSize(FirstTwoPages);
  ASSERT_TRUE(OnePageSize && TwoPageSize);
  EXPECT_EQ(*OnePageSize, 4152U);
  EXPECT_EQ(*TwoPageSize, 8260U);
  EXPECT_EQ(*TwoPageSize - *OnePageSize, 4096U + 12U);

  ASSERT_TRUE(reserveMachOUnwindInfo(LastOnePage));
  ASSERT_TRUE(reserveMachOUnwindInfo(FirstTwoPages));
  EXPECT_EQ(LastOnePage.sections().back().Content.size(), *OnePageSize);
  EXPECT_EQ(FirstTwoPages.sections().back().Content.size(), *TwoPageSize);
}

TEST(MachOWriterTest, CapsReservedCommonEncodings) {
  const auto MakeGraph = [](size_t Count) {
    auto Graph = makeMachOCompactGraph(Target::AArch64, Count);
    for (size_t I = 0; I < Count; ++I)
      EXPECT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{
          static_cast<SymbolId>(I),
          16,
          UINT32_C(0x02000000) | (static_cast<uint32_t>(I) << 12),
          {},
          {},
          {}}));
    return Graph;
  };

  auto AtLimit = machOUnwindInfoSize(MakeGraph(127));
  auto AboveLimit = machOUnwindInfoSize(MakeGraph(128));
  ASSERT_TRUE(AtLimit && AboveLimit);
  EXPECT_EQ(*AtLimit, 4656U);
  EXPECT_EQ(*AboveLimit, *AtLimit);
}

TEST(MachOWriterTest, AccountsForEveryLSDAEntry) {
  constexpr size_t Count = 511;
  auto WithoutLSDA = makeMachOCompactGraph(Target::AArch64, Count);
  auto WithLSDA = makeMachOCompactGraph(Target::AArch64, Count);
  auto LSDA = WithLSDA.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(LSDA);
  for (size_t I = 0; I < Count; ++I) {
    ASSERT_TRUE(WithoutLSDA.addCompactUnwind(CompactUnwindRecord{
        static_cast<SymbolId>(I), 16, 0x02000000, {}, {}, {}}));
    ASSERT_TRUE(WithLSDA.addCompactUnwind(CompactUnwindRecord{
        static_cast<SymbolId>(I), 16, 0x42000000, {}, *LSDA, {}}));
  }
  auto BaseSize = machOUnwindInfoSize(WithoutLSDA);
  auto LSDASize = machOUnwindInfoSize(WithLSDA);
  ASSERT_TRUE(BaseSize && LSDASize);
  EXPECT_EQ(*LSDASize - *BaseSize, Count * 8U);
  EXPECT_TRUE(reserveMachOUnwindInfo(WithLSDA));
}

TEST(MachOWriterTest, PrunesUnreferencedEHFrameFromCompactOutput) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  auto FDE = Graph.addSymbol(Symbol{"fde", 1, 0, 4, false});
  auto LSDA = Graph.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(FDE && LSDA);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x42000000, {}, *LSDA, {}}));
  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());
  EXPECT_EQ(findGraphSection(Graph, SectionPurpose::EHFrame), nullptr);
  ASSERT_TRUE(Graph.validate());
  EXPECT_FALSE(Graph.addSymbol(Symbol{"lsda", 1, 0, 4, false}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"fde", 1, 0, 4, false}));
  ASSERT_TRUE(Graph.compactUnwind()[0].LSDA);
  EXPECT_EQ(Graph.symbols()[*Graph.compactUnwind()[0].LSDA].Section, 1U);
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-no-eh.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef Storage;
  EXPECT_EQ(findSection(**Object, "__eh_frame", Storage), nullptr);
  EXPECT_NE(findSection(**Object, "__unwind_info", Storage), nullptr);
}

TEST(MachOWriterTest, RebuildsIndexesAfterPruningEHFrame) {
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  auto FDE = Graph.addSymbol(Symbol{"fde", 1, 0, 4, false});
  auto LSDA = Graph.addSymbol(Symbol{"lsda", 2, 0, 4, false});
  ASSERT_TRUE(FDE && LSDA);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x42000000, {}, *LSDA, {}}));
  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());

  EXPECT_FALSE(Graph.addSymbol(Symbol{"lsda", 1, 0, 4, false}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"fde", 1, 0, 4, false}));
  auto AddedSection = Graph.addSection(
      Section{"__data", SectionKind::Data, 8, 8, 0, 0,
              std::vector<WasmEdge::Byte>(8), SectionPurpose::Default});
  ASSERT_TRUE(AddedSection);
  auto Added =
      Graph.addSymbol(Symbol{"after-prune", *AddedSection, 0, 8, false});
  ASSERT_TRUE(Added);
  EXPECT_FALSE(
      Graph.addSymbol(Symbol{"after-prune", *AddedSection, 0, 8, false}));
  ASSERT_TRUE(
      Graph.addRebase(Rebase{*AddedSection, 0, 1, 0, 8, ObjectFormat::MachO}));
  EXPECT_FALSE(
      Graph.addRebase(Rebase{*AddedSection, 6, 1, 0, 2, ObjectFormat::MachO}));
}

TEST(MachOWriterTest, RejectsMoreThanTwoPatchRelocationsWithoutChangingGraph) {
  auto Graph = makeMachOCompactGraph(Target::AArch64, 2);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  for (const uint64_t Offset : {UINT64_C(0), UINT64_C(8), UINT64_C(16)}) {
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{0, Offset, llvm::MachO::ARM64_RELOC_UNSIGNED, 0, 0, false,
                   ObjectFormat::MachO, 8, false, true, false}));
  }
  auto &Relocations =
      const_cast<std::vector<Relocation> &>(Graph.relocations());
  Relocations[1].Offset = 0;
  Relocations[2].Offset = 0;

  const auto Sections = Graph.sections();
  const auto Symbols = Graph.symbols();
  const auto Rebases = Graph.rebases();
  const auto References = Graph.ehFrameReferences();
  const auto CompactUnwind = Graph.compactUnwind();
  const auto State = Graph.machOUnwindInfoState();
  const auto Applied = Graph.relocationsApplied();
  const auto Result = Graph.pruneUnreferencedMachOEHFrame();

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
  EXPECT_EQ(Graph.sections().size(), Sections.size());
  EXPECT_EQ(Graph.symbols().size(), Symbols.size());
  EXPECT_EQ(Graph.relocations().size(), Relocations.size());
  EXPECT_EQ(Graph.rebases().size(), Rebases.size());
  EXPECT_EQ(Graph.ehFrameReferences().size(), References.size());
  EXPECT_EQ(Graph.compactUnwind().size(), CompactUnwind.size());
  EXPECT_EQ(Graph.machOUnwindInfoState(), State);
  EXPECT_EQ(Graph.relocationsApplied(), Applied);
  for (size_t I = 0; I < Sections.size(); ++I) {
    EXPECT_EQ(Graph.sections()[I].Name, Sections[I].Name);
    EXPECT_EQ(Graph.sections()[I].Content, Sections[I].Content);
  }
  for (size_t I = 0; I < Symbols.size(); ++I) {
    EXPECT_EQ(Graph.symbols()[I].Name, Symbols[I].Name);
    EXPECT_EQ(Graph.symbols()[I].Section, Symbols[I].Section);
    EXPECT_EQ(Graph.symbols()[I].Offset, Symbols[I].Offset);
  }
  for (size_t I = 0; I < Relocations.size(); ++I) {
    EXPECT_EQ(Graph.relocations()[I].Section, Relocations[I].Section);
    EXPECT_EQ(Graph.relocations()[I].Offset, Relocations[I].Offset);
    EXPECT_EQ(Graph.relocations()[I].Symbol, Relocations[I].Symbol);
  }
}

TEST(MachOWriterTest, ChecksEHFrameNormalizationCommitBoundary) {
  auto MakeGraph = [] {
    LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
    EXPECT_TRUE(Graph.beginInput("normalize.o"));
    auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 4, 4, 0,
                                         0, std::vector<WasmEdge::Byte>(4)});
    auto EH = Graph.addSection(Section{"__eh_frame", SectionKind::Unwind, 8, 16,
                                       0, 0, std::vector<WasmEdge::Byte>(16),
                                       SectionPurpose::EHFrame});
    EXPECT_TRUE(Text && EH);
    auto Function = Graph.addSymbol(Symbol{"_f", *Text, 0, 4, false});
    EXPECT_TRUE(Function);
    EXPECT_TRUE(Graph.addRelocation(
        Relocation{*EH, 0, llvm::MachO::X86_64_RELOC_SIGNED, *Function, 0,
                   false, ObjectFormat::MachO, 4, true}));
    return Graph;
  };

  for (const size_t MaskSize : {size_t{0}, size_t{2}}) {
    auto Graph = MakeGraph();
    const auto Before = Graph.sections()[1].Content;
    auto Result = Internal::EHFrameCommitTestAccess::commit(
        Graph, {{1, std::vector<WasmEdge::Byte>(16, 1)}},
        std::vector<uint8_t>(MaskSize));
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message,
              "EH frame relocation mask size does not match relocations");
    EXPECT_EQ(Graph.sections()[1].Content, Before);
    EXPECT_EQ(Graph.relocations().size(), 1U);
  }

  auto Graph = MakeGraph();
  const auto Before = Graph.sections()[1].Content;
  const std::vector<uint8_t> Remove{1};
  auto Result = Internal::EHFrameCommitTestAccess::commit(
      Graph, {{1, std::vector<WasmEdge::Byte>(17)}}, Remove);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "normalized EH frame content exceeds virtual size");
  EXPECT_EQ(Result.error().Section, 1U);
  EXPECT_EQ(Result.error().SectionName, "__eh_frame");
  EXPECT_EQ(Result.error().Offset, 17U);
  EXPECT_EQ(Graph.sections()[1].Content, Before);
  EXPECT_EQ(Graph.relocations().size(), 1U);
}

TEST(MachOWriterTest, RetainsIndependentDwarfCoverageWithCompactRecords) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  LinkGraph Source(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Source.beginInput("dwarf-source.o"));
  auto SourceText = Source.addSection(Section{
      "__text", SectionKind::Text, 4, 16, 0, 0, std::vector<WasmEdge::Byte>(16),
      SectionPurpose::Default, 0x1000});
  ASSERT_TRUE(SourceText);
  auto SourceFunction =
      Source.addSymbol(Symbol{"_f1", *SourceText, 0, 16, true, {}, true});
  ASSERT_TRUE(SourceFunction);
  ASSERT_TRUE(Source.addCompactUnwind(
      CompactUnwindRecord{*SourceFunction, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(compactUnwindToEHFrame(Source));
  const auto SourceEH =
      std::find_if(Source.sections().begin(), Source.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(SourceEH, Source.sections().end());
  ASSERT_EQ(Source.ehFrameReferences().size(), 1U);

  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("mixed-coverage.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 4, 32, 0, 0,
                                       std::vector<WasmEdge::Byte>(32),
                                       SectionPurpose::Default, 0x1000});
  auto EH = Graph.addSection(*SourceEH);
  ASSERT_TRUE(Text && EH);
  auto Compact = Graph.addSymbol(Symbol{"_f0", *Text, 0, 16, true, {}, true});
  auto Dwarf = Graph.addSymbol(Symbol{"_f1", *Text, 16, 16, true, {}, true});
  ASSERT_TRUE(Compact && Dwarf);
  ASSERT_TRUE(Graph.addEHFrameReference(
      EHFrameReference{*EH, Source.ehFrameReferences()[0].Offset, *Dwarf}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Compact, 16, 0x02000000, {}, {}, {}}));

  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());
  EXPECT_NE(findGraphSection(Graph, SectionPurpose::EHFrame), nullptr);
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "mixed-coverage.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object));
  llvm::object::SectionRef Storage;
  EXPECT_NE(findSection(**Object, "__eh_frame", Storage), nullptr);
  EXPECT_NE(findSection(**Object, "__unwind_info", Storage), nullptr);
}

TEST(MachOWriterTest, RetainsEHFrameForCompactDwarfFallback) {
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  auto FDE = Graph.addSymbol(Symbol{"fde", 1, 12, 0, false});
  ASSERT_TRUE(FDE);
  ASSERT_TRUE(Graph.addEHFrameReference(EHFrameReference{1, 20, 0}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x03000000, {}, {}, *FDE}));
  const auto Sections = Graph.sections().size();
  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());
  EXPECT_EQ(Graph.sections().size(), Sections);
  EXPECT_NE(findGraphSection(Graph, SectionPurpose::EHFrame), nullptr);
}

TEST(MachOWriterTest, LeavesEHOnlyGraphUnchangedWhenPruning) {
  auto Graph = makeMachOGraph(Target::X86_64);
  const auto Sections = Graph.sections();
  const auto Symbols = Graph.symbols();
  ASSERT_TRUE(Graph.pruneUnreferencedMachOEHFrame());
  ASSERT_EQ(Graph.sections().size(), Sections.size());
  ASSERT_EQ(Graph.symbols().size(), Symbols.size());
  for (size_t I = 0; I < Sections.size(); ++I) {
    EXPECT_EQ(Graph.sections()[I].Name, Sections[I].Name);
    EXPECT_EQ(Graph.sections()[I].Purpose, Sections[I].Purpose);
  }
}

TEST(MachOWriterTest, RejectsUnfinalizedCompactUnwindGraphs) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  EXPECT_FALSE(MachOWriter::layout(Graph));

  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::Reserved);
  ASSERT_TRUE(MachOWriter::layout(Graph));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::Reserved);
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(MachOWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(MachOWriterTest, TracksPopulatedUnwindInfoExplicitly) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::None);
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::Reserved);
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::Populated);
  const auto Unwind =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::UnwindInfo;
                   });
  ASSERT_NE(Unwind, Graph.sections().end());
  const auto UnwindId =
      static_cast<SectionId>(Unwind - Graph.sections().begin());
  const auto Content = Unwind->Content;
  EXPECT_EQ(Graph.sections()[UnwindId].Content, Content);
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::Populated);
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_TRUE(MachOWriter::write(Graph, Output));
}

TEST(MachOWriterTest, RejectsGraphMutationAfterUnwindInfoPopulation) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  const auto MakePopulated = [] {
    auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
    EXPECT_TRUE(Graph.addCompactUnwind(
        CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
    EXPECT_TRUE(reserveMachOUnwindInfo(Graph));
    EXPECT_TRUE(MachOWriter::layout(Graph));
    EXPECT_TRUE(applyRelocations(Graph));
    EXPECT_TRUE(populateMachOUnwindInfo(Graph));
    return Graph;
  };

  auto SectionGraph = MakePopulated();
  EXPECT_FALSE(SectionGraph.addSection(
      Section{"__late", SectionKind::Data, 1, 1, 0, 0, {0}}));
  auto SymbolGraph = MakePopulated();
  EXPECT_FALSE(SymbolGraph.addSymbol(Symbol{"late", 0, 0, 0, false}));
  auto CompactGraph = MakePopulated();
  EXPECT_FALSE(CompactGraph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  auto RelocationGraph = MakePopulated();
  EXPECT_FALSE(RelocationGraph.addRelocation(
      Relocation{0, 0, llvm::MachO::ARM64_RELOC_UNSIGNED, 0, 0, false,
                 ObjectFormat::MachO, 8, false, true, false}));
  auto RebaseGraph = MakePopulated();
  EXPECT_FALSE(RebaseGraph.addRebase(Rebase{
      0, 0, llvm::MachO::ARM64_RELOC_UNSIGNED, 0, 8, ObjectFormat::MachO}));
  auto EHReferenceGraph = MakePopulated();
  EXPECT_FALSE(EHReferenceGraph.addEHFrameReference(EHFrameReference{1, 0, 0}));
  auto AddressGraph = MakePopulated();
  EXPECT_FALSE(AddressGraph.setSectionAddress(0, 4096));
  auto FileOffsetGraph = MakePopulated();
  EXPECT_FALSE(FileOffsetGraph.setSectionFileOffset(0, 4096));
  auto LinkedGraph = MakePopulated();
  EXPECT_FALSE(LinkedGraph.setLinkedSection(0, 1));
  auto PruneGraph = MakePopulated();
  EXPECT_FALSE(PruneGraph.pruneUnreferencedMachOEHFrame());

  auto RelocatedGraph = MakePopulated();
  EXPECT_FALSE(applyRelocations(RelocatedGraph));
}

TEST(MachOWriterTest, RejectsEHFrameConversionAfterUnwindInfoPopulation) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  const auto Sections = Graph.sections();
  const auto References = Graph.ehFrameReferences();
  const auto State = Graph.machOUnwindInfoState();

  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  ASSERT_EQ(Graph.sections().size(), Sections.size());
  for (size_t I = 0; I < Sections.size(); ++I) {
    EXPECT_EQ(Graph.sections()[I].Name, Sections[I].Name);
    EXPECT_EQ(Graph.sections()[I].Content, Sections[I].Content);
  }
  EXPECT_EQ(Graph.ehFrameReferences().size(), References.size());
  EXPECT_EQ(Graph.machOUnwindInfoState(), State);
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_TRUE(MachOWriter::write(Graph, Output));
}

TEST(MachOWriterTest, FailedPopulationRemainsReserved) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(Graph.setSectionAddress(0, UINT64_C(1) << 32));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_FALSE(populateMachOUnwindInfo(Graph));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::Reserved);
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(MachOWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(MachOWriterTest, ManualUnwindInfoSectionIsNotFinalized) {
  auto Graph = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  std::vector<WasmEdge::Byte> Content(28);
  Content[0] = 1;
  ASSERT_TRUE(Graph.addSection(Section{"__unwind_info", SectionKind::Unwind, 4,
                                       28, 0, 0, std::move(Content),
                                       SectionPurpose::UnwindInfo}));
  EXPECT_EQ(Graph.machOUnwindInfoState(), MachOUnwindInfoState::None);
  EXPECT_FALSE(Graph.validate());
  EXPECT_FALSE(MachOWriter::layout(Graph));
}

TEST(MachOWriterTest, RejectsCompactSymbolAddressOverflow) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto FunctionOverflow = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(FunctionOverflow.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(reserveMachOUnwindInfo(FunctionOverflow));
  ASSERT_TRUE(MachOWriter::layout(FunctionOverflow));
  ASSERT_TRUE(FunctionOverflow.setSectionAddress(0, UINT64_MAX - 7));
  auto &OverflowSymbol =
      const_cast<Symbol &>(FunctionOverflow.symbols().front());
  OverflowSymbol.Offset = 8;
  EXPECT_FALSE(applyRelocations(FunctionOverflow));

  auto RangeOverflow = makeMachOCompactGraph(Target::AArch64, 1);
  ASSERT_TRUE(RangeOverflow.addCompactUnwind(
      CompactUnwindRecord{0, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(reserveMachOUnwindInfo(RangeOverflow));
  ASSERT_TRUE(MachOWriter::layout(RangeOverflow));
  ASSERT_TRUE(RangeOverflow.setSectionAddress(0, UINT64_MAX - 15));
  ASSERT_TRUE(applyRelocations(RangeOverflow));
  EXPECT_FALSE(populateMachOUnwindInfo(RangeOverflow));
}

TEST_P(MachOArchitectureTest, WritesDeterministicDylib) {
  const auto Architecture = GetParam().Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  auto Graph = makeMachOGraph(Architecture);
  ASSERT_TRUE(MachOWriter::layout(Graph));
  Graph = rebuildWithSectionContent(Graph, 3, [&](auto &Data) {
    for (uint8_t I = 0; I < 8; ++I)
      Data[I] =
          static_cast<WasmEdge::Byte>(Graph.sections()[3].Address >> (I * 8));
  });
  Graph = rebuildWithSectionContent(Graph, 4, [&](auto &Pointer) {
    for (uint8_t I = 0; I < 8; ++I)
      Pointer[I] =
          static_cast<WasmEdge::Byte>(Graph.sections()[4].Address >> (I * 8));
  });
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));
  std::vector<WasmEdge::Byte> Again;
  Writer Second(Again);
  ASSERT_TRUE(MachOWriter::write(Graph, Second));
  EXPECT_EQ(Bytes, Again);

  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "writer.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
  EXPECT_TRUE((*Object)->isMachO());
  EXPECT_EQ(readInteger(Bytes, 0, 4, Endianness::Little),
            llvm::MachO::MH_MAGIC_64);
  EXPECT_EQ(readInteger(Bytes, 12, 4, Endianness::Little),
            llvm::MachO::MH_DYLIB);
  EXPECT_EQ(readInteger(Bytes, 24, 4, Endianness::Little),
            llvm::MachO::MH_NOUNDEFS | llvm::MachO::MH_DYLDLINK |
                llvm::MachO::MH_TWOLEVEL);

  size_t Command = 32;
  uint64_t FirstSectionOffset = UINT64_MAX;
  size_t DyldInfo = 0;
  size_t Dysymtab = 0;
  std::set<uint32_t> Commands;
  std::set<std::string> SegmentNames;
  for (uint32_t I = 0; I < readInteger(Bytes, 16, 4, Endianness::Little); ++I) {
    const uint32_t Type = static_cast<uint32_t>(
        readInteger(Bytes, Command, 4, Endianness::Little));
    const uint32_t Size = static_cast<uint32_t>(
        readInteger(Bytes, Command + 4, 4, Endianness::Little));
    ASSERT_GE(Size, 8U);
    ASSERT_LE(Command + Size, Bytes.size());
    Commands.insert(Type);
    if (Type == llvm::MachO::LC_DYLD_INFO_ONLY)
      DyldInfo = Command;
    if (Type == llvm::MachO::LC_DYSYMTAB)
      Dysymtab = Command;
    if (Type == llvm::MachO::LC_SEGMENT_64) {
      const char *Name =
          reinterpret_cast<const char *>(Bytes.data() + Command + 8);
      SegmentNames.emplace(Name, strnlen(Name, 16));
      const uint32_t MaxProtection = static_cast<uint32_t>(
          readInteger(Bytes, Command + 56, 4, Endianness::Little));
      const uint32_t InitialProtection = static_cast<uint32_t>(
          readInteger(Bytes, Command + 60, 4, Endianness::Little));
      const uint32_t Flags = static_cast<uint32_t>(
          readInteger(Bytes, Command + 68, 4, Endianness::Little));
      EXPECT_EQ(InitialProtection & ~MaxProtection, 0U);
      EXPECT_FALSE((InitialProtection & llvm::MachO::VM_PROT_WRITE) != 0 &&
                   (InitialProtection & llvm::MachO::VM_PROT_EXECUTE) != 0);
      if (std::string_view(Name, strnlen(Name, 16)) == "__DATA_CONST") {
        EXPECT_EQ(MaxProtection,
                  llvm::MachO::VM_PROT_READ | llvm::MachO::VM_PROT_WRITE);
        EXPECT_EQ(InitialProtection,
                  llvm::MachO::VM_PROT_READ | llvm::MachO::VM_PROT_WRITE);
        EXPECT_EQ(Flags, 0x10U);
      }
      const uint32_t SectionCount = static_cast<uint32_t>(
          readInteger(Bytes, Command + 64, 4, Endianness::Little));
      for (uint32_t Section = 0; Section < SectionCount; ++Section) {
        const uint64_t Offset = readInteger(
            Bytes, Command + 72 + Section * 80 + 48, 4, Endianness::Little);
        if (Offset != 0)
          FirstSectionOffset = std::min(FirstSectionOffset, Offset);
      }
    }
    Command += Size;
  }
  EXPECT_EQ(Command, 32U + readInteger(Bytes, 20, 4, Endianness::Little));
  ASSERT_NE(FirstSectionOffset, UINT64_MAX);
  EXPECT_GE(FirstSectionOffset - Command, 16U);
  EXPECT_EQ(SegmentNames, (std::set<std::string>{"__TEXT", "__DATA_CONST",
                                                 "__DATA", "__LINKEDIT"}));
  for (const uint32_t Required :
       {llvm::MachO::LC_DYLD_INFO_ONLY, llvm::MachO::LC_SYMTAB,
        llvm::MachO::LC_DYSYMTAB, llvm::MachO::LC_ID_DYLIB,
        llvm::MachO::LC_BUILD_VERSION})
    EXPECT_TRUE(Commands.count(Required)) << Required;
  EXPECT_FALSE(Commands.count(llvm::MachO::LC_UUID));
  ASSERT_NE(DyldInfo, 0U);
  ASSERT_NE(Dysymtab, 0U);
  EXPECT_EQ(readInteger(Bytes, Dysymtab + 24, 4, Endianness::Little),
            Graph.symbols().size());
  EXPECT_EQ(readInteger(Bytes, Dysymtab + 28, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, DyldInfo + 16, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, DyldInfo + 20, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, DyldInfo + 24, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, DyldInfo + 28, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, DyldInfo + 32, 4, Endianness::Little), 0U);
  EXPECT_EQ(readInteger(Bytes, DyldInfo + 36, 4, Endianness::Little), 0U);

  const size_t ExportOffset = static_cast<size_t>(
      readInteger(Bytes, DyldInfo + 40, 4, Endianness::Little));
  const size_t ExportEnd =
      ExportOffset + static_cast<size_t>(readInteger(Bytes, DyldInfo + 44, 4,
                                                     Endianness::Little));
  std::map<std::string, uint64_t> TrieExports;
  std::set<size_t> Visited;
  readExportNode(Bytes, ExportOffset, ExportOffset, ExportEnd, "", TrieExports,
                 Visited);
  EXPECT_EQ(TrieExports, (std::map<std::string, uint64_t>{
                             {"_f0", Graph.sections()[0].Address},
                             {"_value", Graph.sections()[3].Address}}));

  const size_t RebaseOffset = static_cast<size_t>(
      readInteger(Bytes, DyldInfo + 8, 4, Endianness::Little));
  const size_t RebaseEnd =
      RebaseOffset + static_cast<size_t>(readInteger(Bytes, DyldInfo + 12, 4,
                                                     Endianness::Little));
  size_t RebaseCursor = RebaseOffset;
  ASSERT_LT(RebaseCursor, RebaseEnd);
  EXPECT_EQ(Bytes[RebaseCursor++],
            static_cast<uint8_t>(llvm::MachO::REBASE_OPCODE_SET_TYPE_IMM) |
                static_cast<uint8_t>(llvm::MachO::REBASE_TYPE_POINTER));
  std::set<uint64_t> RebasedAddresses;
  while (RebaseCursor < RebaseEnd &&
         Bytes[RebaseCursor] != llvm::MachO::REBASE_OPCODE_DONE) {
    const uint8_t SegmentOpcode = Bytes[RebaseCursor++];
    EXPECT_EQ(SegmentOpcode & llvm::MachO::REBASE_OPCODE_MASK,
              llvm::MachO::REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB);
    const uint8_t SegmentIndex =
        SegmentOpcode & llvm::MachO::REBASE_IMMEDIATE_MASK;
    ASSERT_LT(SegmentIndex, 4U);
    const uint64_t SegmentOffset = readULEB(Bytes, RebaseCursor, RebaseEnd);
    ASSERT_LT(RebaseCursor, RebaseEnd);
    EXPECT_EQ(Bytes[RebaseCursor++],
              llvm::MachO::REBASE_OPCODE_DO_REBASE_IMM_TIMES | 1);
    const uint64_t SegmentAddress =
        SegmentIndex == 1 ? Graph.sections()[1].Address & ~UINT64_C(0xFFF)
                          : Graph.sections()[3].Address;
    RebasedAddresses.insert(SegmentAddress + SegmentOffset);
  }
  ASSERT_LT(RebaseCursor, RebaseEnd);
  EXPECT_EQ(Bytes[RebaseCursor++], llvm::MachO::REBASE_OPCODE_DONE);
  EXPECT_EQ(RebaseCursor, RebaseEnd);
  std::set<uint64_t> ExpectedRebases;
  for (const auto &Rebase : Graph.rebases()) {
    EXPECT_EQ(Rebase.Width, 8U);
    EXPECT_EQ(Rebase.Format, ObjectFormat::MachO);
    EXPECT_TRUE(Graph.sections()[Rebase.Section].Kind ==
                    SectionKind::ReadOnly ||
                Graph.sections()[Rebase.Section].Kind == SectionKind::Data ||
                Graph.sections()[Rebase.Section].Kind == SectionKind::BSS);
    ExpectedRebases.insert(Graph.sections()[Rebase.Section].Address +
                           Rebase.Offset);
  }
  EXPECT_EQ(RebasedAddresses, ExpectedRebases);

  std::set<std::string> Names;
  for (const auto &Symbol : (*Object)->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    Names.emplace(Name->str());
    auto Flags = Symbol.getFlags();
    ASSERT_TRUE(static_cast<bool>(Flags));
    EXPECT_EQ(*Flags & llvm::object::SymbolRef::SF_Undefined, 0U);
  }
  EXPECT_EQ(Names, (std::set<std::string>{"_f0", "_value"}));
}

TEST(MachOWriterTest, KeepsLargeBSSOutOfLinkEditFileOffsets) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint64_t BSSSize = UINT64_C(8) * 1024 * 1024;
  auto Graph = makeMachOGraph(Target::X86_64);
  auto &BSS = const_cast<Section &>(Graph.sections()[5]);
  BSS.VirtualSize = BSSSize;
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));
  EXPECT_LT(Bytes.size(), BSSSize / 2);

  struct SegmentRange {
    uint64_t Address;
    uint64_t Size;
    uint64_t FileOffset;
    uint64_t FileSize;
  };
  std::map<std::string, SegmentRange> Segments;
  size_t DyldInfo = 0;
  size_t Symtab = 0;
  size_t Command = 32;
  const uint32_t CommandCount =
      static_cast<uint32_t>(readInteger(Bytes, 16, 4, Endianness::Little));
  for (uint32_t I = 0; I < CommandCount; ++I) {
    const uint32_t Type = static_cast<uint32_t>(
        readInteger(Bytes, Command, 4, Endianness::Little));
    const uint32_t Size = static_cast<uint32_t>(
        readInteger(Bytes, Command + 4, 4, Endianness::Little));
    ASSERT_GE(Size, 8U);
    ASSERT_LE(Command + Size, Bytes.size());
    if (Type == llvm::MachO::LC_SEGMENT_64) {
      const char *Name =
          reinterpret_cast<const char *>(Bytes.data() + Command + 8);
      Segments.emplace(
          std::string(Name, strnlen(Name, 16)),
          SegmentRange{
              readInteger(Bytes, Command + 24, 8, Endianness::Little),
              readInteger(Bytes, Command + 32, 8, Endianness::Little),
              readInteger(Bytes, Command + 40, 8, Endianness::Little),
              readInteger(Bytes, Command + 48, 8, Endianness::Little)});
    } else if (Type == llvm::MachO::LC_DYLD_INFO_ONLY) {
      DyldInfo = Command;
    } else if (Type == llvm::MachO::LC_SYMTAB) {
      Symtab = Command;
    }
    Command += Size;
  }

  ASSERT_EQ(Segments.size(), 4U);
  ASSERT_TRUE(Segments.count("__DATA"));
  ASSERT_TRUE(Segments.count("__LINKEDIT"));
  const auto &Data = Segments.at("__DATA");
  const auto &LinkEdit = Segments.at("__LINKEDIT");
  EXPECT_EQ(Data.Size, Graph.sections()[5].Address + BSSSize - Data.Address);
  EXPECT_EQ(Data.FileSize, Graph.sections()[4].FileOffset +
                               Graph.sections()[4].Content.size() -
                               Data.FileOffset);
  EXPECT_GT(Data.Size, Data.FileSize);
  EXPECT_GE(LinkEdit.Address, Data.Address + Data.Size);
  EXPECT_GE(LinkEdit.FileOffset, Data.FileOffset + Data.FileSize);
  EXPECT_LT(LinkEdit.FileOffset, LinkEdit.Address);
  EXPECT_EQ(LinkEdit.Address % 4096, LinkEdit.FileOffset % 4096);
  EXPECT_EQ(LinkEdit.FileOffset + LinkEdit.FileSize, Bytes.size());

  std::vector<SegmentRange> VirtualRanges;
  std::vector<SegmentRange> FileRanges;
  for (const auto &Entry : Segments) {
    const auto &Segment = Entry.second;
    EXPECT_EQ(Segment.Address % 4096, Segment.FileOffset % 4096);
    if (Segment.Size != 0)
      VirtualRanges.push_back(Segment);
    if (Segment.FileSize != 0)
      FileRanges.push_back(Segment);
  }
  std::sort(VirtualRanges.begin(), VirtualRanges.end(),
            [](const auto &L, const auto &R) { return L.Address < R.Address; });
  std::sort(
      FileRanges.begin(), FileRanges.end(),
      [](const auto &L, const auto &R) { return L.FileOffset < R.FileOffset; });
  for (size_t I = 1; I < VirtualRanges.size(); ++I)
    EXPECT_LE(VirtualRanges[I - 1].Address + VirtualRanges[I - 1].Size,
              VirtualRanges[I].Address);
  for (size_t I = 1; I < FileRanges.size(); ++I)
    EXPECT_LE(FileRanges[I - 1].FileOffset + FileRanges[I - 1].FileSize,
              FileRanges[I].FileOffset);

  ASSERT_NE(DyldInfo, 0U);
  ASSERT_NE(Symtab, 0U);
  for (const auto &[Offset, Size] :
       std::array<std::pair<uint64_t, uint64_t>, 4>{
           {{readInteger(Bytes, DyldInfo + 8, 4, Endianness::Little),
             readInteger(Bytes, DyldInfo + 12, 4, Endianness::Little)},
            {readInteger(Bytes, DyldInfo + 40, 4, Endianness::Little),
             readInteger(Bytes, DyldInfo + 44, 4, Endianness::Little)},
            {readInteger(Bytes, Symtab + 8, 4, Endianness::Little),
             readInteger(Bytes, Symtab + 12, 4, Endianness::Little) * 16},
            {readInteger(Bytes, Symtab + 16, 4, Endianness::Little),
             readInteger(Bytes, Symtab + 20, 4, Endianness::Little)}}}) {
    EXPECT_GE(Offset, LinkEdit.FileOffset);
    EXPECT_LE(Offset + Size, Bytes.size());
  }
}

TEST(MachOWriterTest, RejectsSectionContentOutsideOutput) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makeMachOGraph(Target::X86_64);
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  const_cast<Section &>(Graph.sections()[0]).FileOffset = UINT64_MAX;
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  EXPECT_FALSE(MachOWriter::write(Graph, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(MachOWriterTest, RejectsMutatedInvalidLayoutsAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  const std::array<std::function<void(LinkGraph &)>, 3> Mutations{
      {[](LinkGraph &Graph) {
         EXPECT_TRUE(Graph.setSectionAddress(4, Graph.sections()[4].Address +
                                                    UINT64_C(8)));
       },
       [](LinkGraph &Graph) { EXPECT_TRUE(Graph.setSectionFileOffset(1, 0)); },
       [](LinkGraph &Graph) {
         EXPECT_TRUE(Graph.setSectionFileOffset(5, 1));
       }}};
  for (const auto &Mutate : Mutations) {
    auto Graph = makeMachOGraph(Target::X86_64);
    ASSERT_TRUE(MachOWriter::layout(Graph));
    Mutate(Graph);
    ASSERT_TRUE(applyRelocations(Graph));
    const std::vector<WasmEdge::Byte> Existing{1, 2, 3};
    auto Bytes = Existing;
    Writer Output(Bytes);
    EXPECT_FALSE(MachOWriter::write(Graph, Output));
    EXPECT_EQ(Bytes, Existing);
  }
}

TEST(MachOWriterTest, WritesPageBiasedVirtualAddresses) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint64_t Bias = UINT64_C(65536);
  auto Graph = makeMachOGraph(Target::X86_64);
  ASSERT_TRUE(MachOWriter::layout(Graph));
  for (SectionId Id = 0; Id < Graph.sections().size(); ++Id)
    ASSERT_TRUE(
        Graph.setSectionAddress(Id, Graph.sections()[Id].Address + Bias));
  ASSERT_EQ(Graph.sections()[0].Address, Bias + Graph.sections()[0].FileOffset);
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));

  uint64_t TextAddress = 0;
  uint64_t TextFileOffset = UINT64_MAX;
  uint64_t LinkEditAddress = 0;
  uint64_t LinkEditFileOffset = UINT64_MAX;
  size_t Command = 32;
  for (uint32_t I = 0; I < 4; ++I) {
    ASSERT_EQ(readInteger(Bytes, Command, 4, Endianness::Little),
              llvm::MachO::LC_SEGMENT_64);
    const char *Name =
        reinterpret_cast<const char *>(Bytes.data() + Command + 8);
    const std::string SegmentName(Name, strnlen(Name, 16));
    if (SegmentName == "__TEXT") {
      TextAddress = readInteger(Bytes, Command + 24, 8, Endianness::Little);
      TextFileOffset = readInteger(Bytes, Command + 40, 8, Endianness::Little);
    } else if (SegmentName == "__LINKEDIT") {
      LinkEditAddress = readInteger(Bytes, Command + 24, 8, Endianness::Little);
      LinkEditFileOffset =
          readInteger(Bytes, Command + 40, 8, Endianness::Little);
    }
    Command += readInteger(Bytes, Command + 4, 4, Endianness::Little);
  }
  EXPECT_EQ(TextAddress, Bias);
  EXPECT_EQ(TextFileOffset, 0U);
  ASSERT_NE(LinkEditFileOffset, UINT64_MAX);
  EXPECT_EQ(LinkEditAddress, Bias + LinkEditFileOffset);

  ASSERT_EQ(readInteger(Bytes, Command, 4, Endianness::Little),
            llvm::MachO::LC_DYLD_INFO_ONLY);
  const size_t ExportOffset = static_cast<size_t>(
      readInteger(Bytes, Command + 40, 4, Endianness::Little));
  const size_t ExportEnd =
      ExportOffset + static_cast<size_t>(readInteger(Bytes, Command + 44, 4,
                                                     Endianness::Little));
  std::map<std::string, uint64_t> Exports;
  std::set<size_t> Visited;
  readExportNode(Bytes, ExportOffset, ExportOffset, ExportEnd, "", Exports,
                 Visited);
  EXPECT_EQ(Exports["_f0"], Graph.sections()[0].Address - TextAddress);
  EXPECT_EQ(Exports["_value"], Graph.sections()[3].Address - TextAddress);
}

TEST(MachOWriterTest, OrdersEmptyDataConstBeforeData) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("writer.o"));
  ASSERT_TRUE(Graph.addSection(Section{
      "__text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}}));
  ASSERT_TRUE(Graph.addSection(Section{"__eh_frame",
                                       SectionKind::Unwind,
                                       8,
                                       4,
                                       0,
                                       0,
                                       {0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Graph.addSection(Section{"__data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)}));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));

  size_t Command = 32;
  uint64_t PreviousFileOffset = 0;
  for (uint32_t I = 0; I < 4; ++I) {
    ASSERT_EQ(readInteger(Bytes, Command, 4, Endianness::Little),
              llvm::MachO::LC_SEGMENT_64);
    const uint64_t FileOffset =
        readInteger(Bytes, Command + 40, 8, Endianness::Little);
    EXPECT_GE(FileOffset, PreviousFileOffset);
    PreviousFileOffset = FileOffset;
    Command += readInteger(Bytes, Command + 4, 4, Endianness::Little);
  }
}

TEST(MachOWriterTest, RejectsRawCompactUnwind) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("compact.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{"__compact_unwind", SectionKind::Unwind, 8, 32, 0, 0,
              std::vector<WasmEdge::Byte>(32), SectionPurpose::CompactUnwind}));
  EXPECT_FALSE(MachOWriter::layout(Graph));
}

TEST(MachOWriterTest, RejectsResidualCompactUnwindWithEHFrame) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("mixed-unwind.o"));
  ASSERT_TRUE(Graph.addSection(Section{"__eh_frame",
                                       SectionKind::Unwind,
                                       8,
                                       4,
                                       0,
                                       0,
                                       {0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Graph.addSection(
      Section{"__compact_unwind", SectionKind::Unwind, 8, 32, 0, 0,
              std::vector<WasmEdge::Byte>(32), SectionPurpose::CompactUnwind}));
  EXPECT_FALSE(MachOWriter::layout(Graph));
}

TEST(MachOWriterTest, RejectsMissingEHFrame) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("no-eh.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{"__text", SectionKind::Text, 1, 1, 0, 0, {0xC3}}));
  EXPECT_FALSE(MachOWriter::layout(Graph));
}

TEST(MachOWriterTest, WritesBSSOnlyDataSegment) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("bss.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{"__text", SectionKind::Text, 1, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(Graph.addSection(Section{"__eh_frame",
                                       SectionKind::Unwind,
                                       8,
                                       4,
                                       0,
                                       0,
                                       {0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(
      Graph.addSection(Section{"__bss", SectionKind::BSS, 8, 8, 0, 0, {}}));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "bss.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object)) << llvm::toString(Object.takeError());
}

TEST_P(MachOArchitectureTest, AppliesMachOAbsolutePointersAsRebases) {
  const auto Architecture = GetParam().Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("absolute.o"));
  auto Text = Graph.addSection(
      Section{"__text", SectionKind::Text, 4, 4, 0, 0,
              Architecture == Target::X86_64
                  ? std::vector<WasmEdge::Byte>{0xC3, 0, 0, 0}
                  : std::vector<WasmEdge::Byte>{0xC0, 0x03, 0x5F, 0xD6}});
  auto Data = Graph.addSection(Section{"__data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)});
  auto EHFrame = Graph.addSection(Section{"__eh_frame",
                                          SectionKind::Unwind,
                                          8,
                                          4,
                                          0,
                                          0,
                                          {0, 0, 0, 0},
                                          SectionPurpose::EHFrame});
  ASSERT_TRUE(Text && Data && EHFrame);
  auto Function =
      Graph.addSymbol(Symbol{"_f0", *Text, 0, 4, true, std::nullopt, true});
  ASSERT_TRUE(Function);
  const uint32_t Type = Architecture == Target::X86_64
                            ? llvm::MachO::X86_64_RELOC_UNSIGNED
                            : llvm::MachO::ARM64_RELOC_UNSIGNED;
  ASSERT_TRUE(Graph.addRelocation(Relocation{*Data, 0, Type, *Function, 0, true,
                                             ObjectFormat::MachO, 8, false,
                                             false, false}));
  ASSERT_TRUE(MachOWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_EQ(Graph.rebases().size(), 1U);
  EXPECT_EQ(
      readInteger(Graph.sections()[*Data].Content, 0, 8, Endianness::Little),
      Graph.sections()[*Text].Address);
}

INSTANTIATE_TEST_SUITE_P(Architectures, MachOArchitectureTest,
                         testing::Values(ArchitectureCase{Target::X86_64},
                                         ArchitectureCase{Target::AArch64}),
                         architectureCaseName);

TEST(MachOWriterTest, BoundsSectionOrdinalsAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto MakeGraph = [](size_t Count) {
    LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
    EXPECT_TRUE(Graph.beginInput("many-sections.o"));
    EXPECT_TRUE(Graph.addSection(Section{"__eh_frame",
                                         SectionKind::Unwind,
                                         8,
                                         4,
                                         0,
                                         0,
                                         {0, 0, 0, 0},
                                         SectionPurpose::EHFrame}));
    for (size_t I = 1; I < Count; ++I)
      EXPECT_TRUE(Graph.addSection(Section{"__text" + std::to_string(I),
                                           SectionKind::Text,
                                           1,
                                           1,
                                           0,
                                           0,
                                           {0xC3}}));
    return Graph;
  };

  auto Boundary = MakeGraph(UINT8_MAX);
  ASSERT_TRUE(MachOWriter::layout(Boundary));
  ASSERT_TRUE(applyRelocations(Boundary));
  std::vector<WasmEdge::Byte> BoundaryBytes;
  Writer BoundaryOutput(BoundaryBytes);
  EXPECT_TRUE(MachOWriter::write(Boundary, BoundaryOutput));

  auto Overflow = MakeGraph(static_cast<size_t>(UINT8_MAX) + 1);
  ASSERT_TRUE(MachOWriter::layout(Overflow));
  ASSERT_TRUE(applyRelocations(Overflow));
  const std::vector<WasmEdge::Byte> Existing{1, 2, 3};
  auto Bytes = Existing;
  Writer Output(Bytes);
  EXPECT_FALSE(MachOWriter::write(Overflow, Output));
  EXPECT_EQ(Bytes, Existing);
  auto SecondBytes = Existing;
  Writer SecondOutput(SecondBytes);
  EXPECT_FALSE(MachOWriter::write(Overflow, SecondOutput));
  EXPECT_EQ(SecondBytes, Existing);
}

TEST(NativeWriterTest, OwnsDescriptorUntilClose) {
  llvm::SmallString<128> Path;
  int File = -1;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueFile("wasmedge-writer-%%%%%%", File, Path));
  struct Cleanup {
    llvm::SmallString<128> Path;
    ~Cleanup() { llvm::sys::fs::remove(Path); }
  } CleanupGuard{Path};
  const std::array<WasmEdge::Byte, 4> Bytes{0x00, 0x7F, 0x80, 0xFF};

  {
    Writer Output(File);
    ASSERT_TRUE(Output.write(Bytes));
    ASSERT_TRUE(Output.close());
    EXPECT_TRUE(Output.close());
  }

#if WASMEDGE_OS_WINDOWS
  const ScopedInvalidParameterHandler InvalidParameterHandler;
  errno = 0;
  const int Duplicate = ::_dup(File);
  const int Error = errno;
  if (Duplicate != -1)
    EXPECT_EQ(::_close(Duplicate), 0);
  EXPECT_EQ(Duplicate, -1);
  EXPECT_EQ(Error, EBADF);
#else
  EXPECT_EQ(::close(File), -1);
#endif
  std::ifstream Input(std::filesystem::u8path(Path.str().str()),
                      std::ios_base::binary);
  const std::vector<WasmEdge::Byte> Actual{
      std::istreambuf_iterator<char>(Input), std::istreambuf_iterator<char>()};
  EXPECT_EQ(Actual, std::vector<WasmEdge::Byte>(Bytes.begin(), Bytes.end()));
}

TEST(NativeWriterTest, PathCloseIsIdempotentAndRejectsWrites) {
  llvm::SmallString<128> Path;
  int File = -1;
  ASSERT_FALSE(llvm::sys::fs::createUniqueFile("wasmedge-writer-path-%%%%%%",
                                               File, Path));
  llvm::sys::Process::SafelyCloseFileDescriptor(File);
  struct Cleanup {
    llvm::SmallString<128> Path;
    ~Cleanup() { llvm::sys::fs::remove(Path); }
  } CleanupGuard{Path};

  auto Opened = Writer::open(std::filesystem::u8path(Path.str().str()));
  ASSERT_TRUE(Opened);
  Writer Output(std::move(*Opened));
  ASSERT_TRUE(Output.writeByte(0x7F));
  ASSERT_TRUE(Output.close());
  EXPECT_TRUE(Output.close());
  EXPECT_FALSE(Output.writeByte(0x80));
}

TEST(NativeWriterTest, PathOpenRejectsInvalidDestinationsImmediately) {
  llvm::SmallString<128> UniqueRoot;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueDirectory("wasmedge-writer-open", UniqueRoot));
  const auto Root = std::filesystem::u8path(UniqueRoot.str().str());
  struct Cleanup {
    std::filesystem::path Root;
    ~Cleanup() {
      std::error_code Error;
      std::filesystem::remove_all(Root, Error);
    }
  } CleanupGuard{Root};

  const auto MissingOutput = Root / "missing" / "output.wasm";
  auto Missing = Writer::open(MissingOutput);
  ASSERT_FALSE(Missing);
  EXPECT_EQ(Missing.error(), WasmEdge::ErrCode::Value::IllegalPath);
  EXPECT_FALSE(std::filesystem::exists(MissingOutput));

  auto Directory = Writer::open(Root);
  ASSERT_FALSE(Directory);
  EXPECT_EQ(Directory.error(), WasmEdge::ErrCode::Value::IllegalPath);
}

TEST(NativeWriterTest, BufferCloseIsIdempotentAndRejectsWrites) {
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(Output.writeByte(0x7F));
  ASSERT_TRUE(Output.close());
  EXPECT_TRUE(Output.close());
  EXPECT_FALSE(Output.writeByte(0x80));
  EXPECT_EQ(Bytes, (std::vector<WasmEdge::Byte>{0x7F}));
}

#if WASMEDGE_OS_LINUX
TEST(NativeWriterTest, DescriptorRejectsWritesAfterClose) {
  EXPECT_EXIT(
      {
        llvm::SmallString<128> Path;
        int File = -1;
        if (llvm::sys::fs::createUniqueFile("wasmedge-writer-%%%%%%", File,
                                            Path))
          _exit(2);
        llvm::sys::fs::remove(Path);
        Writer Output(File);
        if (!Output.close())
          _exit(3);
        if (Output.writeByte(0xFF))
          _exit(4);
        _exit(0);
      },
      testing::ExitedWithCode(0), "");
}

TEST(NativeWriterTest, CloseReportsBufferedWriteFailure) {
  EXPECT_EXIT(
      {
        const int File = ::open("/dev/full", O_WRONLY);
        if (File == -1)
          _exit(2);
        Writer Output(File);
        if (!Output.writeByte(0xFF))
          _exit(3);
        if (Output.close())
          _exit(4);
        if (!Output.close())
          _exit(5);
        _exit(0);
      },
      testing::ExitedWithCode(0), "");
}

TEST(NativeWriterTest, ELFWriterClassifiesOutputFailureAsIO) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("io.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(ELFWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  const int File = ::open("/dev/full", O_WRONLY);
  ASSERT_NE(File, -1);
  Writer Output(File);
  auto Result = ELFWriter::write(Graph, Output);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("ELF output"), std::string::npos);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::IO);
}

TEST(NativeWriterTest, PEWriterClassifiesOutputFailureAsIO) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("io.obj"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text", SectionKind::Text, 16, 1, 0, 0, {0xC3}}));
  ASSERT_TRUE(PEWriter::layout(Graph));
  ASSERT_TRUE(applyRelocations(Graph));
  const int File = ::open("/dev/full", O_WRONLY);
  ASSERT_NE(File, -1);
  Writer Output(File);
  auto Result = PEWriter::write(Graph, "io.dll", Output);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("PE output"), std::string::npos);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::IO);
}

TEST(NativeWriterTest, BufferedWriteFailureDoesNotAbortOnDestruction) {
  EXPECT_EXIT(
      {
        const int File = ::open("/dev/full", O_WRONLY);
        if (File == -1)
          _exit(2);
        {
          Writer Output(File);
          if (!Output.writeByte(0xFF))
            _exit(3);
        }
        _exit(0);
      },
      testing::ExitedWithCode(0), "");
}
#endif

#if !WASMEDGE_OS_WINDOWS
TEST(NativeWriterTest, PublishFailurePreservesDestination) {
  llvm::SmallString<128> UniqueRoot;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("wasmedge-publish-failure",
                                                    UniqueRoot));
  const auto Root = std::filesystem::u8path(UniqueRoot.str().str());
  struct Cleanup {
    std::filesystem::path Root;
    ~Cleanup() {
      std::error_code Error;
      std::filesystem::remove_all(Root, Error);
    }
  } CleanupGuard{Root};
  const auto Destination = Root / "library.so";
  const auto Temporary = Root / "library.so.tmp-test";
  const std::vector<WasmEdge::Byte> Existing{1, 2, 3};
  const std::vector<WasmEdge::Byte> Replacement{4, 5, 6};
  auto Write = [](const std::filesystem::path &Path,
                  const std::vector<WasmEdge::Byte> &Bytes) {
    std::ofstream File(Path, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Bytes.data()),
               static_cast<std::streamsize>(Bytes.size()));
  };
  auto Read = [](const std::filesystem::path &Path) {
    std::ifstream File(Path, std::ios_base::binary);
    return std::vector<WasmEdge::Byte>(std::istreambuf_iterator<char>(File),
                                       std::istreambuf_iterator<char>());
  };
  Write(Destination, Existing);
  Write(Temporary, Replacement);
  ASSERT_EQ(::chmod(Destination.c_str(), 0751), 0);

  bool Called = false;
  EXPECT_FALSE(Internal::publishAtomically(
      Temporary, Destination,
      [&](const std::filesystem::path &Path) -> WasmEdge::Expect<void> {
        Called = true;
        EXPECT_EQ(Read(Path), Replacement);
        return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
      }));
  EXPECT_TRUE(Called);
  EXPECT_EQ(Read(Destination), Existing);
  struct stat DestinationStat{};
  ASSERT_EQ(::stat(Destination.c_str(), &DestinationStat), 0);
  EXPECT_EQ(DestinationStat.st_mode & 0777, 0751);
  EXPECT_FALSE(std::filesystem::exists(Temporary));
  size_t Entries = 0;
  for (const auto &Entry : std::filesystem::directory_iterator(Root)) {
    static_cast<void>(Entry);
    ++Entries;
  }
  EXPECT_EQ(Entries, 1U);
}

TEST(MachOWriterTest, PublishesOnlyAfterSigning) {
  llvm::SmallString<128> UniqueRoot;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueDirectory("wasmedge-macho-sign", UniqueRoot));
  const auto Root = std::filesystem::u8path(UniqueRoot.str().str());
  struct Cleanup {
    std::filesystem::path Root;
    ~Cleanup() {
      std::error_code Error;
      std::filesystem::remove_all(Root, Error);
    }
  } CleanupGuard{Root};
  const auto Log = Root / "order.log";
  auto MakeHelper = [&](std::string_view Name, std::string_view Action) {
    const auto Path = Root / Name;
    std::ofstream Script(Path);
    Script << "#!/bin/sh\n"
              "printf '%s\\n' \"$@\" >> \""
           << Log.string() << "\"\n"
           << Action << "\n";
    Script.close();
    EXPECT_TRUE(Script);
    EXPECT_EQ(::chmod(Path.c_str(), 0700), 0);
    return Path;
  };
  const auto Success = MakeHelper("success helper", "exit 0");
  const auto Signaled = MakeHelper("signal helper", "kill -TERM $$");
  const auto Failed = MakeHelper("failure helper", "exit 7");

  const auto Destination = Root / "library.dylib";
  const std::vector<WasmEdge::Byte> Existing{1, 2, 3};
  const std::vector<WasmEdge::Byte> Replacement{4, 5, 6};
  auto Write = [](const std::filesystem::path &Path,
                  const std::vector<WasmEdge::Byte> &Bytes) {
    std::ofstream File(Path, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Bytes.data()),
               static_cast<std::streamsize>(Bytes.size()));
  };
  auto Read = [](const std::filesystem::path &Path) {
    std::ifstream File(Path, std::ios_base::binary);
    return std::vector<WasmEdge::Byte>(std::istreambuf_iterator<char>(File),
                                       std::istreambuf_iterator<char>());
  };
  Write(Destination, Existing);

  const std::array<std::filesystem::path, 4> Failures{
      Root / "missing", Signaled, Failed, "/bin/false"};
  for (size_t I = 0; I < Failures.size(); ++I) {
    const auto Temporary = Root / ("temporary-" + std::to_string(I));
    Write(Temporary, Replacement);
    std::filesystem::remove(Log);
    EXPECT_FALSE(Internal::publishMachO(Temporary, Destination, Failures[I]));
    EXPECT_EQ(Read(Destination), Existing);
    EXPECT_FALSE(std::filesystem::exists(Temporary));
    std::string LogValue;
    if (std::filesystem::exists(Log)) {
      const auto LogBytes = Read(Log);
      LogValue.assign(LogBytes.begin(), LogBytes.end());
    }
    const auto Expected = "--force\n--sign\n-\n" + Temporary.string() + "\n";
    EXPECT_EQ(LogValue, I == 0 || I == 3 ? "" : Expected);
  }

  const auto Temporary = Root / "temporary-success";
  Write(Temporary, Replacement);
  std::filesystem::remove(Log);
  EXPECT_TRUE(Internal::publishMachO(Temporary, Destination, Success));
  EXPECT_EQ(Read(Destination), Replacement);
  EXPECT_FALSE(std::filesystem::exists(Temporary));
  const auto LogBytes = Read(Log);
  EXPECT_EQ(std::string(LogBytes.begin(), LogBytes.end()),
            "--force\n--sign\n-\n" + Temporary.string() + "\n");
}
#endif

} // namespace
