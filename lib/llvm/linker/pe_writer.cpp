// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/pe_writer.h"

#include "linker/byte_io.h"
#include "linker/layout.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/Object/COFF.h>

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using Internal::addUnsigned;
using Internal::checkedAlign;
using Internal::checkedMul;
using Internal::fitsSizeT;
using Internal::getInteger;
using Internal::putInteger;

// Image, optional-header, section, export, exception, and base-relocation
// layouts follow the Microsoft PE/COFF specification for PE32+ images.
constexpr size_t MaximumExportCount = UINT16_MAX + size_t{1};
constexpr uint32_t SectionAlignment = 4096;
constexpr uint32_t FileAlignment = 512;
constexpr uint32_t PEOffset = 0x80;
constexpr uint32_t DataDirectoryCount = 16;
constexpr uint32_t DataDirectoriesSize =
    DataDirectoryCount * sizeof(llvm::object::data_directory);
constexpr uint32_t OptionalHeaderSize =
    sizeof(llvm::object::pe32plus_header) + DataDirectoriesSize;
constexpr uint32_t SectionHeaderSize = llvm::COFF::SectionSize;
constexpr uint32_t FixedHeaderSize = PEOffset + sizeof(llvm::COFF::PEMagic) +
                                     llvm::COFF::Header16Size +
                                     OptionalHeaderSize;
constexpr uint16_t OperatingSystemMajorVersion = 6;
constexpr uint16_t SubsystemMajorVersion = 6;
constexpr uint64_t StackReserveSize = UINT64_C(0x100000);
constexpr uint64_t StackCommitSize = UINT64_C(0x1000);
constexpr uint64_t HeapReserveSize = UINT64_C(0x100000);
constexpr uint64_t HeapCommitSize = UINT64_C(0x1000);
constexpr uint64_t ExportDirectorySize =
    sizeof(llvm::object::export_directory_table_entry);
constexpr uint64_t ExportAddressSize = 4;
constexpr uint64_t ExportNamePointerSize = 4;
constexpr uint64_t ExportOrdinalSize = 2;
constexpr uint64_t ExportEntrySize =
    ExportAddressSize + ExportNamePointerSize + ExportOrdinalSize;
constexpr uint32_t BaseRelocationPageMask = 0xFFF;
constexpr uint8_t BaseRelocationTypeShift = 12;
constexpr uint64_t BaseRelocationBlockHeaderSize =
    sizeof(llvm::object::coff_base_reloc_block_header);
constexpr uint64_t BaseRelocationEntrySize =
    sizeof(llvm::object::coff_base_reloc_block_entry);
constexpr size_t X86_64RuntimeFunctionSize = 12;
constexpr size_t AArch64RuntimeFunctionSize = 8;
constexpr uint32_t ARM64UnwindFlagMask = 0x3;
constexpr uint32_t ARM64UnwindFlagXData = 0x0;
constexpr uint32_t ARM64UnwindFlagReserved = 0x3;
constexpr uint8_t ARM64PackedFunctionLengthShift = 2;
constexpr uint32_t ARM64PackedFunctionLengthMask = 0x7FF;
constexpr uint32_t ARM64XDataFunctionLengthMask = 0x3FFFF;
constexpr uint32_t ARM64XDataHeaderSize = 4;
constexpr uint32_t ARM64InstructionSize = 4;

enum class OutputGroup : uint8_t { Text, ReadOnly, Data, BSS, PData, XData };

size_t runtimeFunctionSize(Target Architecture) noexcept {
  return Architecture == Target::X86_64 ? X86_64RuntimeFunctionSize
                                        : AArch64RuntimeFunctionSize;
}

uint32_t get32(Span<const Byte> Bytes, size_t Offset) noexcept {
  return static_cast<uint32_t>(getInteger(Bytes, Offset, 4));
}

OutputGroup outputGroup(const Section &Value) noexcept {
  if (Value.Purpose == SectionPurpose::PData)
    return OutputGroup::PData;
  if (Value.Purpose == SectionPurpose::XData)
    return OutputGroup::XData;
  switch (Value.Kind) {
  case SectionKind::Text:
    return OutputGroup::Text;
  case SectionKind::Data:
    return OutputGroup::Data;
  case SectionKind::BSS:
    return OutputGroup::BSS;
  case SectionKind::ReadOnly:
  case SectionKind::Unwind:
    break;
  }
  return OutputGroup::ReadOnly;
}

std::string_view groupName(OutputGroup Group) noexcept {
  switch (Group) {
  case OutputGroup::Text:
    return ".text";
  case OutputGroup::ReadOnly:
    return ".rdata";
  case OutputGroup::Data:
    return ".data";
  case OutputGroup::BSS:
    return ".bss";
  case OutputGroup::PData:
    return ".pdata";
  case OutputGroup::XData:
    return ".xdata";
  }
  return {};
}

constexpr uint32_t ReadOnlyCharacteristics =
    llvm::COFF::IMAGE_SCN_CNT_INITIALIZED_DATA | llvm::COFF::IMAGE_SCN_MEM_READ;

uint32_t characteristics(OutputGroup Group) noexcept {
  switch (Group) {
  case OutputGroup::Text:
    return llvm::COFF::IMAGE_SCN_CNT_CODE | llvm::COFF::IMAGE_SCN_MEM_EXECUTE |
           llvm::COFF::IMAGE_SCN_MEM_READ;
  case OutputGroup::Data:
    return llvm::COFF::IMAGE_SCN_CNT_INITIALIZED_DATA |
           llvm::COFF::IMAGE_SCN_MEM_READ | llvm::COFF::IMAGE_SCN_MEM_WRITE;
  case OutputGroup::BSS:
    return llvm::COFF::IMAGE_SCN_CNT_UNINITIALIZED_DATA |
           llvm::COFF::IMAGE_SCN_MEM_READ | llvm::COFF::IMAGE_SCN_MEM_WRITE;
  case OutputGroup::ReadOnly:
  case OutputGroup::PData:
  case OutputGroup::XData:
    break;
  }
  return ReadOnlyCharacteristics;
}

struct OutputSection {
  std::string Name;
  uint32_t RVA = 0;
  uint32_t VirtualSize = 0;
  uint32_t FileOffset = 0;
  uint32_t FileSize = 0;
  uint32_t Characteristics = 0;
  std::vector<Byte> Content;
};

template <typename Filter, typename Extent>
const Section *findSectionAt(const LinkGraph &Graph, uint64_t RuntimeImageBase,
                             uint32_t RVA, Filter &&Matches,
                             Extent &&Covers) noexcept {
  for (const auto &Value : Graph.sections()) {
    if (!Matches(Value) || Value.Address < RuntimeImageBase)
      continue;
    const uint64_t Start = Value.Address - RuntimeImageBase;
    if (RVA >= Start && Covers(Value, RVA - Start))
      return &Value;
  }
  return nullptr;
}

bool containsRVA(const LinkGraph &Graph, uint64_t RuntimeImageBase,
                 SectionKind Kind, uint32_t RVA,
                 bool AllowEnd = false) noexcept {
  return findSectionAt(
             Graph, RuntimeImageBase, RVA,
             [Kind](const Section &Value) { return Value.Kind == Kind; },
             [AllowEnd](const Section &Value, uint64_t Offset) {
               return AllowEnd ? Offset <= Value.VirtualSize
                               : Offset < Value.VirtualSize;
             }) != nullptr;
}

bool isXData(const Section &Value) noexcept {
  return Value.Purpose == SectionPurpose::XData;
}

bool containsXDataRVA(const LinkGraph &Graph, uint64_t RuntimeImageBase,
                      uint32_t RVA) noexcept {
  return findSectionAt(Graph, RuntimeImageBase, RVA, isXData,
                       [](const Section &Value, uint64_t Offset) {
                         return Offset < Value.VirtualSize;
                       }) != nullptr;
}

std::optional<uint32_t> arm64UnwindLength(const LinkGraph &Graph,
                                          uint64_t RuntimeImageBase,
                                          uint32_t RVA) noexcept {
  const auto *Owner = findSectionAt(Graph, RuntimeImageBase, RVA, isXData,
                                    [](const Section &Value, uint64_t Offset) {
                                      return Offset <= Value.Content.size() &&
                                             ARM64XDataHeaderSize <=
                                                 Value.Content.size() - Offset;
                                    });
  if (Owner == nullptr)
    return std::nullopt;
  const uint64_t Offset = RVA - (Owner->Address - RuntimeImageBase);
  return get32(Owner->Content, static_cast<size_t>(Offset)) &
         ARM64XDataFunctionLengthMask;
}

} // namespace

LinkExpect<std::vector<Byte>>
normalizePERuntimeFunctions(const LinkGraph &Graph, uint64_t RuntimeImageBase) {
  const size_t EntrySize = runtimeFunctionSize(Graph.target());
  std::vector<SectionId> PDataSections;
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    const auto &Value = Graph.sections()[I];
    if (Value.Purpose != SectionPurpose::PData)
      continue;
    if (Value.VirtualSize % EntrySize != 0)
      return sectionError("invalid PE runtime function table size", Value, I,
                          Value.VirtualSize - Value.VirtualSize % EntrySize);
    if (Value.Content.size() != Value.VirtualSize)
      return sectionError("PE runtime function table content size mismatch",
                          Value, I, Value.Content.size());
    PDataSections.push_back(I);
  }
  auto IsPData = [&](SectionId Id) {
    return std::find(PDataSections.begin(), PDataSections.end(), Id) !=
           PDataSections.end();
  };
  for (const auto &Symbol : Graph.symbols())
    if (IsPData(Symbol.Section) && (Symbol.Exported || Symbol.Size != 0))
      return sectionError("symbol references PE runtime function table",
                          Graph.sections()[Symbol.Section], Symbol.Section,
                          Symbol.Offset);
  for (const auto &Relocation : Graph.relocations())
    if (IsPData(Graph.symbols()[Relocation.Symbol].Section)) {
      const auto &TargetSection =
          Graph.sections()[Graph.symbols()[Relocation.Symbol].Section];
      Diagnostic Diag(
          "relocation targets PE runtime function target section '" +
          TargetSection.Name + "'");
      Diag.Section = Relocation.Section;
      Diag.Offset = Relocation.Offset;
      Diag.RelocationType = Relocation.Type;
      Diag.SectionName = Graph.sections()[Relocation.Section].Name;
      return diagnosticError(std::move(Diag));
    }
  struct Entry {
    uint32_t Begin;
    uint32_t End;
    SectionId Section;
    size_t Offset;
    std::vector<Byte> Bytes;
  };
  std::vector<Entry> Entries;
  for (const SectionId Id : PDataSections) {
    const auto &Section = Graph.sections()[Id];
    for (size_t Offset = 0; Offset < Section.Content.size();
         Offset += EntrySize) {
      const Span<const Byte> Bytes(Section.Content.data() + Offset, EntrySize);
      const uint32_t Begin = get32(Bytes, 0);
      uint32_t End = Begin;
      if (!containsRVA(Graph, RuntimeImageBase, SectionKind::Text, Begin))
        return sectionError("PE runtime function start is outside code",
                            Section, Id, Offset);
      if (Graph.target() == Target::X86_64) {
        End = get32(Bytes, 4);
        const uint32_t Unwind = get32(Bytes, 8);
        if (End <= Begin ||
            !containsRVA(Graph, RuntimeImageBase, SectionKind::Text, End,
                         true) ||
            !containsXDataRVA(Graph, RuntimeImageBase, Unwind))
          return sectionError("invalid x86_64 PE runtime function entry",
                              Section, Id, Offset);
      } else {
        const uint32_t Unwind = get32(Bytes, 4);
        const uint32_t Flag = Unwind & ARM64UnwindFlagMask;
        uint32_t FunctionLength = 0;
        if (Flag == ARM64UnwindFlagXData) {
          const auto Length =
              arm64UnwindLength(Graph, RuntimeImageBase, Unwind);
          if (!Length || *Length == 0 ||
              Begin > UINT32_MAX - *Length * ARM64InstructionSize)
            return sectionError("invalid AArch64 PE unwind record", Section, Id,
                                Offset);
          FunctionLength = *Length;
        } else {
          if (Flag == ARM64UnwindFlagReserved)
            return sectionError("unsupported AArch64 PE unwind encoding",
                                Section, Id, Offset + 4,
                                DiagnosticKind::Unsupported);
          FunctionLength = (Unwind >> ARM64PackedFunctionLengthShift) &
                           ARM64PackedFunctionLengthMask;
          if (FunctionLength == 0 ||
              Begin > UINT32_MAX - FunctionLength * ARM64InstructionSize)
            return sectionError("invalid packed AArch64 PE unwind length",
                                Section, Id, Offset + 4);
        }
        End = Begin + FunctionLength * ARM64InstructionSize;
        if (!containsRVA(Graph, RuntimeImageBase, SectionKind::Text, End, true))
          return sectionError("AArch64 PE runtime function ends outside code",
                              Section, Id, Offset);
      }
      Entries.push_back(Entry{Begin, End, Id, Offset,
                              std::vector<Byte>(Bytes.begin(), Bytes.end())});
    }
  }
  std::sort(Entries.begin(), Entries.end(),
            [](const auto &Left, const auto &Right) {
              return Left.Begin < Right.Begin;
            });
  for (size_t I = 1; I < Entries.size(); ++I)
    if (Entries[I - 1].Begin == Entries[I].Begin ||
        Entries[I - 1].End > Entries[I].Begin)
      return sectionError("duplicate or overlapping PE runtime functions",
                          Graph.sections()[Entries[I].Section],
                          Entries[I].Section, Entries[I].Offset);
  std::vector<Byte> Content(Entries.size() * EntrySize);
  const Span<Byte> Target(Content.data(), Content.size());
  size_t Offset = 0;
  for (const auto &Entry : Entries) {
    std::copy(Entry.Bytes.begin(), Entry.Bytes.end(),
              Target.subspan(Offset, Entry.Bytes.size()).begin());
    Offset += EntrySize;
  }
  return Content;
}

namespace {

bool appendSection(std::vector<OutputSection> &Sections, std::string Name,
                   uint64_t &RVA, uint64_t &FileOffset,
                   std::vector<Byte> Content, uint64_t VirtualSize) {
  uint64_t RawSize = 0;
  uint64_t NextRVA = 0;
  uint64_t NextFile = 0;
  if (!checkedAlign(Content.size(), FileAlignment, RawSize) ||
      !addUnsigned(RVA, VirtualSize, NextRVA) ||
      !checkedAlign(NextRVA, SectionAlignment, NextRVA) ||
      !addUnsigned(FileOffset, RawSize, NextFile) || RVA > UINT32_MAX ||
      FileOffset > UINT32_MAX || VirtualSize > UINT32_MAX ||
      RawSize > UINT32_MAX || NextRVA > UINT32_MAX || NextFile > UINT32_MAX)
    return false;
  OutputSection Value;
  Value.Name = std::move(Name);
  Value.RVA = static_cast<uint32_t>(RVA);
  Value.VirtualSize = static_cast<uint32_t>(VirtualSize);
  Value.FileOffset = static_cast<uint32_t>(FileOffset);
  Value.FileSize = static_cast<uint32_t>(RawSize);
  Value.Characteristics = ReadOnlyCharacteristics;
  Value.Content = std::move(Content);
  Sections.push_back(std::move(Value));
  RVA = NextRVA;
  FileOffset = NextFile;
  return true;
}

LinkExpect<std::map<OutputGroup, OutputSection>>
mergeInputSections(const LinkGraph &Graph) {
  std::map<OutputGroup, OutputSection> Inputs;
  for (const auto &Input : Graph.sections()) {
    const auto Group = outputGroup(Input);
    const uint64_t InputRVA64 = Input.Address - PEImageBase;
    if (Input.Address < PEImageBase || InputRVA64 > UINT32_MAX ||
        Input.VirtualSize > UINT32_MAX)
      return sectionError("PE input section placement is invalid", Input,
                          std::nullopt);
    auto &Section = Inputs[Group];
    Section.Name = std::string(groupName(Group));
    if (Section.RVA == 0) {
      Section.RVA = static_cast<uint32_t>(InputRVA64);
      Section.FileOffset = static_cast<uint32_t>(Input.FileOffset);
    }
    const uint64_t Offset = InputRVA64 - Section.RVA;
    uint64_t End = 0;
    if (InputRVA64 < Section.RVA ||
        !addUnsigned(Offset, Input.VirtualSize, End) || End > UINT32_MAX ||
        !fitsSizeT(End))
      return sectionError("PE output section size overflows", Input,
                          std::nullopt);
    Section.VirtualSize =
        std::max(Section.VirtualSize, static_cast<uint32_t>(End));
    if (Input.Kind != SectionKind::BSS) {
      Section.Content.resize(static_cast<size_t>(End));
      const Span<Byte> Target(Section.Content.data(), Section.Content.size());
      std::copy(
          Input.Content.begin(), Input.Content.end(),
          Target.subspan(static_cast<size_t>(Offset), Input.Content.size())
              .begin());
    }
    Section.Characteristics = characteristics(Group);
  }
  return Inputs;
}

struct ExportTable {
  uint32_t RVA;
  std::vector<Byte> Bytes;
};

LinkExpect<ExportTable> buildExportTable(const LinkGraph &Graph,
                                         std::string_view DLLName,
                                         uint64_t RVA) {
  using Directory = llvm::object::export_directory_table_entry;
  EXPECTED_TRY(auto Exports, sortedUniqueExports(Graph));
  if (Exports.size() > MaximumExportCount)
    return diagnosticError("PE image has too many exports");
  uint64_t TableSize = 0;
  uint64_t Size = 0;
  if (!checkedMul(Exports.size(), ExportEntrySize, TableSize) ||
      !addUnsigned(ExportDirectorySize, TableSize, Size) ||
      !addUnsigned(Size, DLLName.size(), Size) || !addUnsigned(Size, 1, Size))
    return diagnosticError("PE export table size overflows");
  for (const auto *Symbol : Exports)
    if (!addUnsigned(Size, Symbol->exportedName().size() + 1, Size))
      return diagnosticError("PE export table size overflows");
  if (Size > UINT32_MAX || !fitsSizeT(Size))
    return diagnosticError("PE export table size overflows");
  std::vector<Byte> Bytes(static_cast<size_t>(Size));
  if (RVA > UINT32_MAX)
    return diagnosticError("PE export table address overflows");
  const uint32_t ExportRVA = static_cast<uint32_t>(RVA);
  uint64_t Cursor64 = 0;
  uint64_t DLLNameRVA64 = 0;
  if (!addUnsigned(ExportDirectorySize, TableSize, Cursor64) ||
      Cursor64 > UINT32_MAX ||
      !addUnsigned(ExportRVA, Cursor64, DLLNameRVA64) ||
      DLLNameRVA64 > UINT32_MAX)
    return diagnosticError("PE export name address overflows");
  uint32_t Cursor = static_cast<uint32_t>(Cursor64);
  const Span<Byte> Data(Bytes.data(), Bytes.size());
  std::copy(DLLName.begin(), DLLName.end(),
            Data.subspan(Cursor, DLLName.size()).begin());
  Cursor += static_cast<uint32_t>(DLLName.size()) + 1;
  uint64_t AddressTableRVA = 0;
  uint64_t NameTableRVA = 0;
  uint64_t OrdinalTableRVA = 0;
  uint64_t TableBytes = 0;
  if (!addUnsigned(ExportRVA, ExportDirectorySize, AddressTableRVA) ||
      !checkedMul(Exports.size(), ExportAddressSize, TableBytes) ||
      !addUnsigned(AddressTableRVA, TableBytes, NameTableRVA) ||
      !addUnsigned(NameTableRVA, TableBytes, OrdinalTableRVA) ||
      OrdinalTableRVA > UINT32_MAX)
    return diagnosticError("PE export table address overflows");
  putInteger(Bytes, offsetof(Directory, NameRVA), DLLNameRVA64, 4);
  putInteger(Bytes, offsetof(Directory, OrdinalBase), 1, 4);
  putInteger(Bytes, offsetof(Directory, AddressTableEntries), Exports.size(),
             4);
  putInteger(Bytes, offsetof(Directory, NumberOfNamePointers), Exports.size(),
             4);
  putInteger(Bytes, offsetof(Directory, ExportAddressTableRVA), AddressTableRVA,
             4);
  putInteger(Bytes, offsetof(Directory, NamePointerRVA), NameTableRVA, 4);
  putInteger(Bytes, offsetof(Directory, OrdinalTableRVA), OrdinalTableRVA, 4);
  const uint64_t NamePointers =
      ExportDirectorySize + Exports.size() * ExportAddressSize;
  const uint64_t Ordinals =
      NamePointers + Exports.size() * ExportNamePointerSize;
  for (size_t I = 0; I < Exports.size(); ++I) {
    const auto &Symbol = *Exports[I];
    const uint64_t Address =
        Graph.sections()[Symbol.Section].Address + Symbol.Offset;
    if (Address < PEImageBase || Address - PEImageBase > UINT32_MAX)
      return diagnosticError("PE export address is outside the image");
    uint64_t NameRVA = 0;
    if (!addUnsigned(ExportRVA, Cursor, NameRVA) || NameRVA > UINT32_MAX)
      return diagnosticError("PE export name address overflows");
    putInteger(Bytes, ExportDirectorySize + I * ExportAddressSize,
               Address - PEImageBase, ExportAddressSize);
    putInteger(Bytes, NamePointers + I * ExportNamePointerSize, NameRVA,
               ExportNamePointerSize);
    putInteger(Bytes, Ordinals + I * ExportOrdinalSize, I, ExportOrdinalSize);
    const auto &Name = Symbol.exportedName();
    std::copy(Name.begin(), Name.end(),
              Data.subspan(Cursor, Name.size()).begin());
    Cursor += static_cast<uint32_t>(Name.size()) + 1;
  }
  return ExportTable{ExportRVA, std::move(Bytes)};
}

LinkExpect<std::vector<Byte>> buildBaseRelocations(const LinkGraph &Graph) {
  const uint32_t Required =
      Graph.target() == Target::X86_64
          ? static_cast<uint32_t>(llvm::COFF::IMAGE_REL_AMD64_ADDR64)
          : static_cast<uint32_t>(llvm::COFF::IMAGE_REL_ARM64_ADDR64);
  std::map<uint32_t, std::vector<uint16_t>> Pages;
  for (const auto &Rebase : Graph.rebases()) {
    if (Rebase.Format != ObjectFormat::COFF || Rebase.Type != Required ||
        Rebase.Width != 8 || Rebase.Section >= Graph.sections().size() ||
        Rebase.Offset > Graph.sections()[Rebase.Section].Content.size() ||
        8 > Graph.sections()[Rebase.Section].Content.size() - Rebase.Offset)
      return diagnosticError("unsupported PE rebase");
    const uint64_t Address =
        Graph.sections()[Rebase.Section].Address + Rebase.Offset;
    if (Address < PEImageBase || Address - PEImageBase > UINT32_MAX)
      return diagnosticError("PE rebase is outside the image");
    const uint32_t Slot = static_cast<uint32_t>(Address - PEImageBase);
    Pages[Slot & ~BaseRelocationPageMask].push_back(static_cast<uint16_t>(
        (llvm::COFF::IMAGE_REL_BASED_DIR64 << BaseRelocationTypeShift) |
        (Slot & BaseRelocationPageMask)));
  }
  std::vector<Byte> Result;
  for (auto &[Page, Entries] : Pages) {
    std::sort(Entries.begin(), Entries.end());
    if (Entries.size() % 2 != 0)
      Entries.push_back(llvm::COFF::IMAGE_REL_BASED_ABSOLUTE
                        << BaseRelocationTypeShift);
    const size_t Start = Result.size();
    const size_t BlockSize = BaseRelocationBlockHeaderSize +
                             Entries.size() * BaseRelocationEntrySize;
    Result.resize(Start + BlockSize);
    putInteger(
        Result,
        Start + offsetof(llvm::object::coff_base_reloc_block_header, PageRVA),
        Page, 4);
    putInteger(
        Result,
        Start + offsetof(llvm::object::coff_base_reloc_block_header, BlockSize),
        BlockSize, 4);
    for (size_t I = 0; I < Entries.size(); ++I)
      putInteger(Result,
                 Start + BaseRelocationBlockHeaderSize +
                     I * BaseRelocationEntrySize,
                 Entries[I], BaseRelocationEntrySize);
  }
  return Result;
}

void putDataDirectory(Span<Byte> Bytes, size_t Optional,
                      llvm::COFF::DataDirectoryIndex Index, uint32_t RVA,
                      uint32_t Size) {
  const size_t Entry = Optional + sizeof(llvm::object::pe32plus_header) +
                       Index * sizeof(llvm::object::data_directory);
  putInteger(Bytes,
             Entry +
                 offsetof(llvm::object::data_directory, RelativeVirtualAddress),
             RVA, 4);
  putInteger(Bytes, Entry + offsetof(llvm::object::data_directory, Size), Size,
             4);
}

} // namespace

Expect<void> PEWriter::layout(LinkGraph &Graph) noexcept {
  auto Fail = []() { return Unexpect(ErrCode::Value::IllegalPath); };
  try {
    if (Graph.format() != ObjectFormat::COFF || Graph.relocationsApplied() ||
        !Graph.validate() || Graph.endianness() != Endianness::Little ||
        (Graph.target() != Target::X86_64 && Graph.target() != Target::AArch64))
      return Fail();
    std::map<OutputGroup, std::vector<SectionId>> Groups;
    for (SectionId I = 0; I < Graph.sections().size(); ++I)
      Groups[outputGroup(Graph.sections()[I])].push_back(I);
    uint64_t HeaderBytes = 0;
    if (!addUnsigned(FixedHeaderSize,
                     (Groups.size() + 2) *
                         static_cast<uint64_t>(SectionHeaderSize),
                     HeaderBytes) ||
        !checkedAlign(HeaderBytes, FileAlignment, HeaderBytes) ||
        HeaderBytes > UINT32_MAX)
      return Fail();
    uint64_t RVA = SectionAlignment;
    uint64_t FileOffset = HeaderBytes;
    std::vector<Internal::Placement> Placements(Graph.sections().size());
    for (const auto &[Group, Members] : Groups) {
      uint64_t GroupAlignment = SectionAlignment;
      for (const SectionId Id : Members)
        GroupAlignment =
            std::max(GroupAlignment, Graph.sections()[Id].Alignment);
      if (!checkedAlign(RVA, GroupAlignment, RVA))
        return Fail();
      uint64_t GroupOffset = 0;
      for (const SectionId Id : Members) {
        const auto &Input = Graph.sections()[Id];
        if (!checkedAlign(GroupOffset, Input.Alignment, GroupOffset))
          return Fail();
        uint64_t Address = 0;
        uint64_t End = 0;
        if (!addUnsigned(PEImageBase, RVA, Address) ||
            !addUnsigned(Address, GroupOffset, Address) ||
            !addUnsigned(GroupOffset, Input.VirtualSize, End) ||
            End > UINT32_MAX)
          return Fail();
        Placements[Id] = {Address, Input.Kind == SectionKind::BSS
                                       ? 0
                                       : FileOffset + GroupOffset};
        GroupOffset = End;
      }
      if (!addUnsigned(RVA, GroupOffset, RVA) ||
          !checkedAlign(RVA, SectionAlignment, RVA) || RVA > UINT32_MAX)
        return Fail();
      if (Group != OutputGroup::BSS) {
        uint64_t RawSize = 0;
        if (!checkedAlign(GroupOffset, FileAlignment, RawSize) ||
            !addUnsigned(FileOffset, RawSize, FileOffset) ||
            FileOffset > UINT32_MAX)
          return Fail();
      }
    }
    if (!Internal::applyPlacements(Graph, Placements))
      return Fail();
    return {};
  } catch (...) {
    return Fail();
  }
}

LinkExpect<void> PEWriter::write(const LinkGraph &Graph,
                                 std::string_view DLLName,
                                 Writer &Output) noexcept {
  try {
    if (!Graph.relocationsApplied() || Graph.format() != ObjectFormat::COFF)
      return diagnosticError("PE writer requires a relocated COFF link graph");
    EXPECTED_TRY(Graph.validate());
    if (DLLName.empty() || DLLName.find('\0') != DLLName.npos)
      return diagnosticError("invalid PE DLL name");
    EXPECTED_TRY(auto Inputs, mergeInputSections(Graph));
    if (auto PData = Inputs.find(OutputGroup::PData); PData != Inputs.end()) {
      EXPECTED_TRY(PData->second.Content, normalizePERuntimeFunctions(Graph));
      PData->second.VirtualSize =
          static_cast<uint32_t>(PData->second.Content.size());
    }
    std::vector<OutputSection> Sections;
    uint64_t RVA = SectionAlignment;
    uint64_t FileOffset = 0;
    for (auto &Entry : Inputs) {
      auto &Section = Entry.second;
      uint64_t RawSize = 0;
      if (!checkedAlign(Section.Content.size(), FileAlignment, RawSize) ||
          RawSize > UINT32_MAX)
        return diagnosticError("PE section raw size overflows");
      Section.FileSize = static_cast<uint32_t>(RawSize);
      Sections.push_back(std::move(Section));
      RVA = std::max<uint64_t>(RVA, Sections.back().RVA +
                                        Sections.back().VirtualSize);
      FileOffset = std::max<uint64_t>(FileOffset, Sections.back().FileOffset +
                                                      Sections.back().FileSize);
    }
    if (!checkedAlign(RVA, SectionAlignment, RVA) ||
        !checkedAlign(FileOffset, FileAlignment, FileOffset))
      return diagnosticError("PE image size overflows");

    EXPECTED_TRY(auto Exports, buildExportTable(Graph, DLLName, RVA));
    const uint32_t ExportRVA = Exports.RVA;
    const auto ExportSize = static_cast<uint32_t>(Exports.Bytes.size());
    if (!appendSection(Sections, ".edata", RVA, FileOffset,
                       std::move(Exports.Bytes), ExportSize))
      return diagnosticError("PE .edata section overflows");

    EXPECTED_TRY(auto Reloc, buildBaseRelocations(Graph));
    const uint32_t RelocRVA = static_cast<uint32_t>(RVA);
    const uint32_t RelocSize = static_cast<uint32_t>(Reloc.size());
    if (RelocSize != 0 && !appendSection(Sections, ".reloc", RVA, FileOffset,
                                         std::move(Reloc), RelocSize))
      return diagnosticError("PE .reloc section overflows");
    if (Sections.size() > UINT16_MAX || RVA > UINT32_MAX ||
        !fitsSizeT(FileOffset))
      return diagnosticError("PE image is too large");

    const uint32_t HeaderSize = Sections.front().FileOffset;
    if (HeaderSize < FixedHeaderSize + Sections.size() * SectionHeaderSize ||
        HeaderSize % FileAlignment != 0)
      return diagnosticError("PE headers overlap section content");
    std::vector<Byte> Bytes(static_cast<size_t>(FileOffset));
    Bytes[0] = 'M';
    Bytes[1] = 'Z';
    putInteger(Bytes, offsetof(llvm::object::dos_header, AddressOfNewExeHeader),
               PEOffset, 4);
    Bytes[0x40] = 0x0E;
    Bytes[0x41] = 0x1F;
    Bytes[0x42] = 0xBA;
    std::copy(std::begin(llvm::COFF::PEMagic), std::end(llvm::COFF::PEMagic),
              Bytes.begin() + PEOffset);
    using FileHeader = llvm::object::coff_file_header;
    const size_t COFF = PEOffset + sizeof(llvm::COFF::PEMagic);
    putInteger(Bytes, COFF + offsetof(FileHeader, Machine),
               Graph.target() == Target::X86_64
                   ? llvm::COFF::IMAGE_FILE_MACHINE_AMD64
                   : llvm::COFF::IMAGE_FILE_MACHINE_ARM64,
               2);
    putInteger(Bytes, COFF + offsetof(FileHeader, NumberOfSections),
               Sections.size(), 2);
    putInteger(Bytes, COFF + offsetof(FileHeader, SizeOfOptionalHeader),
               OptionalHeaderSize, 2);
    putInteger(Bytes, COFF + offsetof(FileHeader, Characteristics),
               llvm::COFF::IMAGE_FILE_EXECUTABLE_IMAGE |
                   llvm::COFF::IMAGE_FILE_LARGE_ADDRESS_AWARE |
                   llvm::COFF::IMAGE_FILE_DLL,
               2);
    using OptionalHeader = llvm::object::pe32plus_header;
    const size_t Optional = COFF + llvm::COFF::Header16Size;
    putInteger(Bytes, Optional + offsetof(OptionalHeader, Magic),
               llvm::COFF::PE32Header::PE32_PLUS, 2);
    uint64_t CodeSize = 0;
    uint64_t InitializedSize = 0;
    uint64_t UninitializedSize = 0;
    uint32_t CodeRVA = 0;
    for (const auto &Section : Sections) {
      if ((Section.Characteristics & llvm::COFF::IMAGE_SCN_CNT_CODE) != 0) {
        CodeSize += Section.FileSize;
        CodeRVA = Section.RVA;
      } else if ((Section.Characteristics &
                  llvm::COFF::IMAGE_SCN_CNT_UNINITIALIZED_DATA) != 0) {
        UninitializedSize += Section.VirtualSize;
      } else {
        InitializedSize += Section.FileSize;
      }
    }
    if (CodeSize > UINT32_MAX || InitializedSize > UINT32_MAX ||
        UninitializedSize > UINT32_MAX)
      return diagnosticError("PE section size totals overflow");
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfCode), CodeSize,
               4);
    putInteger(Bytes,
               Optional + offsetof(OptionalHeader, SizeOfInitializedData),
               InitializedSize, 4);
    putInteger(Bytes,
               Optional + offsetof(OptionalHeader, SizeOfUninitializedData),
               UninitializedSize, 4);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, BaseOfCode), CodeRVA,
               4);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, ImageBase),
               PEImageBase, 8);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SectionAlignment),
               SectionAlignment, 4);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, FileAlignment),
               FileAlignment, 4);
    putInteger(Bytes,
               Optional + offsetof(OptionalHeader, MajorOperatingSystemVersion),
               OperatingSystemMajorVersion, 2);
    putInteger(Bytes,
               Optional + offsetof(OptionalHeader, MajorSubsystemVersion),
               SubsystemMajorVersion, 2);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfImage), RVA, 4);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfHeaders),
               HeaderSize, 4);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, Subsystem),
               llvm::COFF::IMAGE_SUBSYSTEM_WINDOWS_CUI, 2);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, DLLCharacteristics),
               llvm::COFF::IMAGE_DLL_CHARACTERISTICS_NX_COMPAT |
                   llvm::COFF::IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA |
                   llvm::COFF::IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE,
               2);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfStackReserve),
               StackReserveSize, 8);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfStackCommit),
               StackCommitSize, 8);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfHeapReserve),
               HeapReserveSize, 8);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, SizeOfHeapCommit),
               HeapCommitSize, 8);
    putInteger(Bytes, Optional + offsetof(OptionalHeader, NumberOfRvaAndSize),
               DataDirectoryCount, 4);
    putDataDirectory(Bytes, Optional, llvm::COFF::EXPORT_TABLE, ExportRVA,
                     ExportSize);
    const auto PData =
        std::find_if(Sections.begin(), Sections.end(), [](const auto &Section) {
          return Section.Name == groupName(OutputGroup::PData);
        });
    if (PData != Sections.end()) {
      if (PData->VirtualSize % runtimeFunctionSize(Graph.target()) != 0)
        return diagnosticError("invalid PE runtime function table size");
      putDataDirectory(Bytes, Optional, llvm::COFF::EXCEPTION_TABLE, PData->RVA,
                       PData->VirtualSize);
    }
    if (RelocSize != 0)
      putDataDirectory(Bytes, Optional, llvm::COFF::BASE_RELOCATION_TABLE,
                       RelocRVA, RelocSize);
    using SectionHeader = llvm::object::coff_section;
    const Span<Byte> Target(Bytes.data(), Bytes.size());
    for (size_t I = 0; I < Sections.size(); ++I) {
      const auto &Section = Sections[I];
      const size_t Header =
          Optional + OptionalHeaderSize + I * SectionHeaderSize;
      std::copy(Section.Name.begin(), Section.Name.end(),
                Target.subspan(Header, Section.Name.size()).begin());
      putInteger(Bytes, Header + offsetof(SectionHeader, VirtualSize),
                 Section.VirtualSize, 4);
      putInteger(Bytes, Header + offsetof(SectionHeader, VirtualAddress),
                 Section.RVA, 4);
      putInteger(Bytes, Header + offsetof(SectionHeader, SizeOfRawData),
                 Section.FileSize, 4);
      putInteger(Bytes, Header + offsetof(SectionHeader, PointerToRawData),
                 Section.FileSize == 0 ? 0 : Section.FileOffset, 4);
      putInteger(Bytes, Header + offsetof(SectionHeader, Characteristics),
                 Section.Characteristics, 4);
      if (!Section.Content.empty())
        std::copy(
            Section.Content.begin(), Section.Content.end(),
            Target.subspan(Section.FileOffset, Section.Content.size()).begin());
    }
    return writeImage(Output, Bytes, "PE");
  } catch (...) {
    return diagnosticError("PE writer failed");
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
