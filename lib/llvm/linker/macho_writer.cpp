// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/macho_writer.h"

#include "linker/byte_io.h"
#include "linker/layout.h"

#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/MD5.h>
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using Internal::addUnsigned;
using Internal::checkedAlign;
using Internal::checkedMul;
using Internal::fitsSizeT;
using Internal::putInteger;

constexpr uint32_t SegmentReadOnlyFlag = 0x10;

// Header, load-command, segment, symbol, dyld-info, and version fields follow
// the Mach-O ABI structures represented by llvm/BinaryFormat/MachO.h.
constexpr uint64_t X86PageSize = 4096;
constexpr uint64_t ARMPageSize = 16384;
constexpr uint64_t HeaderSize = sizeof(llvm::MachO::mach_header_64);
constexpr uint64_t SegmentCommandSize = sizeof(llvm::MachO::segment_command_64);
constexpr uint64_t SectionCommandSize = sizeof(llvm::MachO::section_64);
constexpr uint64_t DyldInfoCommandSize = sizeof(llvm::MachO::dyld_info_command);
constexpr uint64_t SymtabCommandSize = sizeof(llvm::MachO::symtab_command);
constexpr uint64_t DysymtabCommandSize = sizeof(llvm::MachO::dysymtab_command);
constexpr uint64_t BuildVersionCommandSize =
    sizeof(llvm::MachO::build_version_command);
constexpr uint64_t BuildToolVersionSize =
    sizeof(llvm::MachO::build_tool_version);
constexpr uint64_t UUIDCommandSize = sizeof(llvm::MachO::uuid_command);
constexpr uint64_t DylibCommandPrefixSize = sizeof(llvm::MachO::dylib_command);
constexpr uint64_t CodeSignatureCommandSize =
    sizeof(llvm::MachO::linkedit_data_command);
constexpr uint64_t SymbolEntrySize = sizeof(llvm::MachO::nlist_64);
constexpr uint64_t LoadCommandAlignment = 8;
constexpr uint64_t SectionStartAlignment = 16;
constexpr uint64_t LinkEditAlignment = 8;
constexpr size_t NameFieldSize = 16;
constexpr uint32_t SegmentCount = 4;
constexpr uint32_t FixedLoadCommandCount = SegmentCount + 6;
constexpr uint8_t DataConstSegmentIndex = 1;
constexpr uint8_t DataSegmentIndex = 2;
constexpr uint32_t DylibVersion1_0_0 = UINT32_C(0x10000);
constexpr uint32_t ReadOnlyProtection =
    static_cast<uint32_t>(llvm::MachO::VM_PROT_READ);
constexpr uint32_t ReadExecuteProtection =
    ReadOnlyProtection | static_cast<uint32_t>(llvm::MachO::VM_PROT_EXECUTE);
constexpr uint32_t ReadWriteProtection =
    ReadOnlyProtection | static_cast<uint32_t>(llvm::MachO::VM_PROT_WRITE);
constexpr std::string_view InstallName = "@rpath/libwasmedge-aot.dylib";
constexpr std::string_view SystemLibraryName = "/usr/lib/libSystem.B.dylib";

struct MachOTargetInfo {
  uint32_t CPUType;
  uint32_t CPUSubtype;
  uint32_t RebaseType;
  uint64_t PageSize;
};

MachOTargetInfo targetInfo(Target Architecture) noexcept {
  if (Architecture == Target::X86_64)
    return {static_cast<uint32_t>(llvm::MachO::CPU_TYPE_X86_64),
            static_cast<uint32_t>(llvm::MachO::CPU_SUBTYPE_X86_64_ALL),
            static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED),
            X86PageSize};
  return {static_cast<uint32_t>(llvm::MachO::CPU_TYPE_ARM64),
          static_cast<uint32_t>(llvm::MachO::CPU_SUBTYPE_ARM64_ALL),
          static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED),
          ARMPageSize};
}

bool supported(const LinkGraph &Graph, bool RequireFinalized) noexcept {
  const bool HasFileBackedSection = std::any_of(
      Graph.sections().begin(), Graph.sections().end(),
      [](const auto &Value) { return Value.Kind != SectionKind::BSS; });
  const bool HasCompact = std::any_of(
      Graph.sections().begin(), Graph.sections().end(), [](const auto &Value) {
        return Value.Purpose == SectionPurpose::CompactUnwind;
      });
  const bool HasUnwindInfo = std::any_of(
      Graph.sections().begin(), Graph.sections().end(), [](const auto &Value) {
        return Value.Purpose == SectionPurpose::UnwindInfo;
      });
  const bool HasCompactRecords = !Graph.compactUnwind().empty();
  const auto UnwindInfoState = Graph.machOUnwindInfoState();
  return HasFileBackedSection && Graph.format() == ObjectFormat::MachO &&
         Graph.endianness() == Endianness::Little &&
         (Graph.target() == Target::X86_64 ||
          Graph.target() == Target::AArch64) &&
         !HasCompact &&
         (!HasCompactRecords ||
          UnwindInfoState != MachOUnwindInfoState::None) &&
         (!RequireFinalized || !HasUnwindInfo ||
          UnwindInfoState == MachOUnwindInfoState::Populated);
}

uint64_t dylibCommandSize(std::string_view Name) noexcept {
  const uint64_t Result = DylibCommandPrefixSize + Name.size() + 1;
  return (Result + LoadCommandAlignment - 1) & ~(LoadCommandAlignment - 1);
}

bool loadCommandSize(const LinkGraph &Graph, uint64_t &Result) noexcept {
  uint64_t SectionsSize = 0;
  if (!checkedMul(SectionCommandSize, Graph.sections().size(), SectionsSize))
    return false;
  Result = SegmentCommandSize * SegmentCount + DyldInfoCommandSize +
           SymtabCommandSize + DysymtabCommandSize +
           dylibCommandSize(InstallName) + dylibCommandSize(SystemLibraryName) +
           UUIDCommandSize;
  if (!addUnsigned(Result, SectionsSize, Result) ||
      Graph.machOBuildVersions().size() > UINT32_MAX - FixedLoadCommandCount)
    return false;
  for (const auto &Version : Graph.machOBuildVersions()) {
    uint32_t Size = 0;
    if (!Internal::machOBuildVersionCommandSize(Version.Tools.size(), Size) ||
        !addUnsigned(Result, Size, Result))
      return false;
  }
  return Result <= UINT32_MAX;
}

template <typename Iterator>
bool copyAt(Span<Byte> Bytes, uint64_t Offset, Iterator Begin, Iterator End) {
  const auto Size = static_cast<uint64_t>(std::distance(Begin, End));
  if (Offset > Bytes.size() || Size > Bytes.size() - Offset)
    return false;
  Bytes = Bytes.subspan(static_cast<size_t>(Offset), static_cast<size_t>(Size));
  std::copy(Begin, End, Bytes.begin());
  return true;
}

bool putName(Span<Byte> Bytes, uint64_t Offset, std::string_view Name) {
  const auto Trimmed = Name.substr(0, NameFieldSize);
  return copyAt(Bytes, Offset, Trimmed.begin(), Trimmed.end());
}

bool putDylibCommand(Span<Byte> Bytes, uint64_t Command, uint32_t Kind,
                     std::string_view Name) {
  using llvm::MachO::dylib;
  using llvm::MachO::dylib_command;
  constexpr uint64_t Dylib = offsetof(dylib_command, dylib);
  putInteger(Bytes, Command + offsetof(dylib_command, cmd), Kind, 4);
  putInteger(Bytes, Command + offsetof(dylib_command, cmdsize),
             dylibCommandSize(Name), 4);
  putInteger(Bytes, Command + Dylib + offsetof(dylib, name),
             DylibCommandPrefixSize, 4);
  putInteger(Bytes, Command + Dylib + offsetof(dylib, current_version),
             DylibVersion1_0_0, 4);
  putInteger(Bytes, Command + Dylib + offsetof(dylib, compatibility_version),
             DylibVersion1_0_0, 4);
  return copyAt(Bytes, Command + DylibCommandPrefixSize, Name.begin(),
                Name.end());
}

void appendULEB(std::vector<Byte> &Bytes, uint64_t Value) {
  do {
    uint8_t Current = static_cast<uint8_t>(Value & 0x7F);
    Value >>= 7;
    if (Value != 0)
      Current |= 0x80;
    Bytes.push_back(Current);
  } while (Value != 0);
}

size_t ulebSize(uint64_t Value) noexcept {
  size_t Result = 1;
  while ((Value >>= 7) != 0)
    ++Result;
  return Result;
}

LinkExpect<std::vector<Byte>> exportTrie(const LinkGraph &Graph,
                                         uint64_t ImageBase) {
  EXPECTED_TRY(auto Exports, sortedUniqueExports(Graph));
  struct Node {
    std::optional<uint64_t> Address;
    std::map<Byte, size_t> Children;
  };
  std::vector<Node> Nodes(1);
  for (const auto *SymbolValue : Exports) {
    const auto &Name = SymbolValue->exportedName();
    uint64_t Address = 0;
    if (!addUnsigned(Graph.sections()[SymbolValue->Section].Address,
                     SymbolValue->Offset, Address) ||
        Address < ImageBase)
      return diagnosticError("Mach-O export address precedes the image");
    if (Name.empty() || Name.find('\0') != Name.npos)
      return diagnosticError("invalid Mach-O export name");
    size_t NodeIndex = 0;
    for (const char Character : Name) {
      const Byte Edge = static_cast<Byte>(Character);
      auto Child = Nodes[NodeIndex].Children.find(Edge);
      if (Child == Nodes[NodeIndex].Children.end()) {
        const size_t NewIndex = Nodes.size();
        Nodes.emplace_back();
        Nodes[NodeIndex].Children.emplace(Edge, NewIndex);
        NodeIndex = NewIndex;
      } else {
        NodeIndex = Child->second;
      }
    }
    Nodes[NodeIndex].Address = Address - ImageBase;
  }

  std::vector<size_t> Offsets(Nodes.size());
  for (;;) {
    std::vector<size_t> NewOffsets(Nodes.size());
    size_t Cursor = 0;
    for (size_t I = 0; I < Nodes.size(); ++I) {
      NewOffsets[I] = Cursor;
      const auto &NodeValue = Nodes[I];
      const size_t TerminalSize =
          NodeValue.Address ? 1 + ulebSize(*NodeValue.Address) : 0;
      Cursor += ulebSize(TerminalSize) + TerminalSize + 1;
      for (const auto &Entry : NodeValue.Children)
        Cursor += 2 + ulebSize(Offsets[Entry.second]);
    }
    if (NewOffsets == Offsets)
      break;
    Offsets = std::move(NewOffsets);
  }

  std::vector<Byte> Result;
  for (const auto &NodeValue : Nodes) {
    const size_t TerminalSize =
        NodeValue.Address ? 1 + ulebSize(*NodeValue.Address) : 0;
    appendULEB(Result, TerminalSize);
    if (NodeValue.Address) {
      Result.push_back(llvm::MachO::EXPORT_SYMBOL_FLAGS_KIND_REGULAR);
      appendULEB(Result, *NodeValue.Address);
    }
    if (NodeValue.Children.size() > UINT8_MAX)
      return diagnosticError("Mach-O export trie node has too many children");
    Result.push_back(static_cast<Byte>(NodeValue.Children.size()));
    for (const auto &[Edge, Child] : NodeValue.Children) {
      Result.push_back(Edge);
      Result.push_back(0);
      appendULEB(Result, Offsets[Child]);
    }
  }
  return Result;
}

struct Segment {
  std::string_view Name;
  uint64_t Address;
  uint64_t Size;
  uint64_t FileOffset;
  uint64_t FileSize;
  uint32_t MaxProtection;
  uint32_t InitialProtection;
  std::vector<SectionId> Sections;
};

std::optional<std::vector<Segment>>
segments(const LinkGraph &Graph, uint64_t TextAddress, uint64_t LinkEditAddress,
         uint64_t LinkEditOffset, uint64_t LinkEditSize) {
  std::vector<SectionId> Text;
  std::vector<SectionId> Constant;
  std::vector<SectionId> Data;
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    const auto Kind = Graph.sections()[I].Kind;
    if (Kind == SectionKind::Data || Kind == SectionKind::BSS)
      Data.push_back(I);
    else if (Kind == SectionKind::ReadOnly)
      Constant.push_back(I);
    else
      Text.push_back(I);
  }
  const uint64_t Page = targetInfo(Graph.target()).PageSize;
  uint64_t CommandsSize = 0;
  uint64_t TextEnd = 0;
  if (!loadCommandSize(Graph, CommandsSize) ||
      !addUnsigned(HeaderSize, CommandsSize, TextEnd) ||
      !addUnsigned(TextAddress, TextEnd, TextEnd))
    return std::nullopt;
  uint64_t DataStart = LinkEditAddress;
  uint64_t DataEnd = 0;
  uint64_t DataFileStart = LinkEditOffset;
  uint64_t DataFileEnd = 0;
  for (const auto Id : Text) {
    const auto &SectionValue = Graph.sections()[Id];
    uint64_t End = 0;
    if (!addUnsigned(SectionValue.Address, SectionValue.VirtualSize, End))
      return std::nullopt;
    TextEnd = std::max(TextEnd, End);
  }
  uint64_t ConstantStart = LinkEditAddress;
  uint64_t ConstantEnd = 0;
  uint64_t ConstantFileStart = LinkEditOffset;
  uint64_t ConstantFileEnd = 0;
  for (const auto Id : Constant) {
    const auto &SectionValue = Graph.sections()[Id];
    uint64_t End = 0;
    uint64_t FileEnd = 0;
    if (!addUnsigned(SectionValue.Address, SectionValue.VirtualSize, End) ||
        !addUnsigned(SectionValue.FileOffset, SectionValue.VirtualSize,
                     FileEnd))
      return std::nullopt;
    ConstantStart = std::min(ConstantStart, SectionValue.Address);
    ConstantEnd = std::max(ConstantEnd, End);
    ConstantFileStart = std::min(ConstantFileStart, SectionValue.FileOffset);
    ConstantFileEnd = std::max(ConstantFileEnd, FileEnd);
  }
  for (const auto Id : Data) {
    const auto &SectionValue = Graph.sections()[Id];
    uint64_t End = 0;
    if (!addUnsigned(SectionValue.Address, SectionValue.VirtualSize, End))
      return std::nullopt;
    DataStart = std::min(DataStart, SectionValue.Address);
    DataEnd = std::max(DataEnd, End);
    if (SectionValue.Kind != SectionKind::BSS) {
      uint64_t FileEnd = 0;
      if (!addUnsigned(SectionValue.FileOffset, SectionValue.VirtualSize,
                       FileEnd))
        return std::nullopt;
      DataFileStart = std::min(DataFileStart, SectionValue.FileOffset);
      DataFileEnd = std::max(DataFileEnd, FileEnd);
    }
  }
  uint64_t TextSize = 0;
  uint64_t AlignedTextEnd = 0;
  if (!checkedAlign(TextEnd, Page, AlignedTextEnd) ||
      AlignedTextEnd < TextAddress)
    return std::nullopt;
  TextSize = AlignedTextEnd - TextAddress;
  if (Data.empty())
    DataStart = AlignedTextEnd;
  return std::vector<Segment>{
      {"__TEXT", TextAddress, TextSize, 0, TextSize, ReadExecuteProtection,
       ReadExecuteProtection, std::move(Text)},
      {"__DATA_CONST", Constant.empty() ? DataStart : ConstantStart,
       Constant.empty() ? 0 : ConstantEnd - ConstantStart,
       Constant.empty() ? DataFileStart : ConstantFileStart,
       Constant.empty() ? 0 : ConstantFileEnd - ConstantFileStart,
       ReadWriteProtection, ReadWriteProtection, std::move(Constant)},
      {"__DATA", DataStart, Data.empty() ? 0 : DataEnd - DataStart,
       DataFileStart,
       Data.empty() || DataFileEnd <= DataFileStart
           ? 0
           : DataFileEnd - DataFileStart,
       ReadWriteProtection, ReadWriteProtection, std::move(Data)},
      {"__LINKEDIT",
       LinkEditAddress,
       LinkEditSize,
       LinkEditOffset,
       LinkEditSize,
       ReadOnlyProtection,
       ReadOnlyProtection,
       {}}};
}

bool validateSegments(const LinkGraph &Graph,
                      const std::vector<Segment> &Segments,
                      uint64_t FileSize) noexcept {
  const uint64_t Page = targetInfo(Graph.target()).PageSize;
  uint64_t PreviousAddressEnd = 0;
  uint64_t PreviousFileEnd = 0;
  for (const auto &SegmentValue : Segments) {
    uint64_t AddressEnd = 0;
    uint64_t FileEnd = 0;
    if (!addUnsigned(SegmentValue.Address, SegmentValue.Size, AddressEnd) ||
        !addUnsigned(SegmentValue.FileOffset, SegmentValue.FileSize, FileEnd) ||
        SegmentValue.FileSize > SegmentValue.Size || FileEnd > FileSize ||
        SegmentValue.Address % Page != SegmentValue.FileOffset % Page ||
        SegmentValue.Address % Page != 0 || SegmentValue.FileOffset % Page != 0)
      return false;
    if (SegmentValue.Size != 0) {
      if (SegmentValue.Address < PreviousAddressEnd)
        return false;
      PreviousAddressEnd = AddressEnd;
    }
    if (SegmentValue.FileSize != 0) {
      if (SegmentValue.FileOffset < PreviousFileEnd)
        return false;
      PreviousFileEnd = FileEnd;
    }

    std::vector<std::pair<uint64_t, uint64_t>> AddressRanges;
    std::vector<std::pair<uint64_t, uint64_t>> FileRanges;
    for (const auto Id : SegmentValue.Sections) {
      const auto &SectionValue = Graph.sections()[Id];
      uint64_t SectionAddressEnd = 0;
      if (!addUnsigned(SectionValue.Address, SectionValue.VirtualSize,
                       SectionAddressEnd) ||
          SectionValue.Address < SegmentValue.Address ||
          SectionAddressEnd > AddressEnd ||
          SectionValue.Address % SectionValue.Alignment != 0)
        return false;
      if (SectionValue.VirtualSize != 0)
        AddressRanges.emplace_back(SectionValue.Address, SectionAddressEnd);
      if (SectionValue.Kind == SectionKind::BSS) {
        if (SectionValue.FileOffset != 0 || !SectionValue.Content.empty())
          return false;
        continue;
      }

      uint64_t SectionFileEnd = 0;
      uint64_t ContentEnd = 0;
      if (!addUnsigned(SectionValue.FileOffset, SectionValue.VirtualSize,
                       SectionFileEnd) ||
          !addUnsigned(SectionValue.FileOffset, SectionValue.Content.size(),
                       ContentEnd) ||
          SectionValue.FileOffset < SegmentValue.FileOffset ||
          SectionFileEnd > FileEnd || ContentEnd > FileSize ||
          SectionValue.FileOffset % SectionValue.Alignment != 0 ||
          SectionValue.Address - SegmentValue.Address !=
              SectionValue.FileOffset - SegmentValue.FileOffset)
        return false;
      if (SectionValue.VirtualSize != 0)
        FileRanges.emplace_back(SectionValue.FileOffset, SectionFileEnd);
    }
    const auto Overlaps = [](auto &Ranges) {
      std::sort(Ranges.begin(), Ranges.end());
      for (size_t I = 1; I < Ranges.size(); ++I)
        if (Ranges[I].first < Ranges[I - 1].second)
          return true;
      return false;
    };
    if (Overlaps(AddressRanges) || Overlaps(FileRanges))
      return false;
  }
  return true;
}

uint32_t sectionFlags(const Section &SectionValue) noexcept {
  if (SectionValue.Kind == SectionKind::BSS)
    return static_cast<uint32_t>(llvm::MachO::S_ZEROFILL);
  if (SectionValue.Kind == SectionKind::Text)
    return static_cast<uint32_t>(llvm::MachO::S_REGULAR) |
           static_cast<uint32_t>(llvm::MachO::S_ATTR_PURE_INSTRUCTIONS) |
           static_cast<uint32_t>(llvm::MachO::S_ATTR_SOME_INSTRUCTIONS);
  return static_cast<uint32_t>(llvm::MachO::S_REGULAR);
}

LinkExpect<std::vector<Byte>>
rebaseStream(const LinkGraph &Graph, const std::vector<Segment> &Segments) {
  std::vector<Byte> Result;
  if (Graph.rebases().empty())
    return Result;
  Result.push_back(static_cast<Byte>(
      static_cast<uint8_t>(llvm::MachO::REBASE_OPCODE_SET_TYPE_IMM) |
      static_cast<uint8_t>(llvm::MachO::REBASE_TYPE_POINTER)));
  std::vector<const Rebase *> Ordered;
  for (const auto &Value : Graph.rebases())
    Ordered.push_back(&Value);
  std::sort(Ordered.begin(), Ordered.end(), [&](const auto *L, const auto *R) {
    return std::tie(Graph.sections()[L->Section].Address, L->Offset) <
           std::tie(Graph.sections()[R->Section].Address, R->Offset);
  });
  for (const auto *Value : Ordered) {
    const uint8_t SegmentIndex =
        Graph.sections()[Value->Section].Kind == SectionKind::ReadOnly
            ? DataConstSegmentIndex
            : DataSegmentIndex;
    const auto &Owner = Segments[SegmentIndex];
    if (Owner.Sections.empty())
      return diagnosticError("Mach-O rebase targets an empty segment");
    uint64_t Address = 0;
    if (!addUnsigned(Graph.sections()[Value->Section].Address, Value->Offset,
                     Address) ||
        Address < Owner.Address)
      return diagnosticError("Mach-O rebase precedes its segment");
    Result.push_back(static_cast<Byte>(
        static_cast<uint8_t>(
            llvm::MachO::REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB) |
        SegmentIndex));
    appendULEB(Result, Address - Owner.Address);
    Result.push_back(static_cast<Byte>(
        static_cast<uint8_t>(llvm::MachO::REBASE_OPCODE_DO_REBASE_IMM_TIMES) |
        uint8_t{1}));
  }
  Result.push_back(llvm::MachO::REBASE_OPCODE_DONE);
  return Result;
}

} // namespace

bool Internal::machOBuildVersionCommandSize(uint64_t ToolCount,
                                            uint32_t &Result) noexcept {
  uint64_t ToolsSize = 0;
  uint64_t Size = 0;
  if (!checkedMul(ToolCount, BuildToolVersionSize, ToolsSize) ||
      !addUnsigned(BuildVersionCommandSize, ToolsSize, Size) ||
      Size > UINT32_MAX)
    return false;
  Result = static_cast<uint32_t>(Size);
  return true;
}

Expect<void> MachOWriter::layout(LinkGraph &Graph) noexcept {
  auto Fail = []() { return Unexpect(ErrCode::Value::IllegalPath); };
  try {
    if (!supported(Graph, false) || Graph.relocationsApplied() ||
        !Graph.validate())
      return Fail();
    uint64_t CommandsSize = 0;
    uint64_t Address = 0;
    if (!loadCommandSize(Graph, CommandsSize) ||
        !addUnsigned(HeaderSize, CommandsSize, Address) ||
        !addUnsigned(Address, CodeSignatureCommandSize, Address) ||
        !checkedAlign(Address, SectionStartAlignment, Address))
      return Fail();
    uint64_t FileOffset = Address;
    const uint64_t PageSize = targetInfo(Graph.target()).PageSize;
    std::vector<Internal::Placement> Placements(Graph.sections().size());
    constexpr std::array<SectionKind, 5> Kinds{
        SectionKind::Text, SectionKind::Unwind, SectionKind::ReadOnly,
        SectionKind::Data, SectionKind::BSS};
    for (const auto Kind : Kinds) {
      if ((Kind == SectionKind::ReadOnly || Kind == SectionKind::Data) &&
          (!checkedAlign(Address, PageSize, Address) ||
           !checkedAlign(FileOffset, PageSize, FileOffset)))
        return Fail();
      for (const auto Id : Internal::sectionsOfKind(Graph, Kind)) {
        const auto &SectionValue = Graph.sections()[Id];
        if (!checkedAlign(Address, SectionValue.Alignment, Address))
          return Fail();
        if (Kind != SectionKind::BSS &&
            !checkedAlign(FileOffset, SectionValue.Alignment, FileOffset))
          return Fail();
        Placements[Id] = {Address,
                          Kind == SectionKind::BSS ? uint64_t{0} : FileOffset};
        if (!addUnsigned(Address, SectionValue.VirtualSize, Address) ||
            (Kind != SectionKind::BSS &&
             !addUnsigned(FileOffset, SectionValue.VirtualSize, FileOffset)))
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

LinkExpect<void> MachOWriter::write(const LinkGraph &Graph,
                                    Writer &Output) noexcept {
  try {
    if (!supported(Graph, true) || !Graph.relocationsApplied())
      return diagnosticError("Mach-O writer requires a relocated, finalized "
                             "Mach-O link graph");
    EXPECTED_TRY(Graph.validate());
    if (Graph.sections().size() > UINT8_MAX ||
        Graph.symbols().size() > UINT32_MAX)
      return diagnosticError("Mach-O link graph has too many entries");
    const auto Info = targetInfo(Graph.target());
    uint64_t CommandsSize = 0;
    uint64_t MinimumSectionOffset = 0;
    if (!loadCommandSize(Graph, CommandsSize) || CommandsSize > UINT32_MAX ||
        !addUnsigned(HeaderSize, CommandsSize, MinimumSectionOffset) ||
        !addUnsigned(MinimumSectionOffset, CodeSignatureCommandSize,
                     MinimumSectionOffset) ||
        !checkedAlign(MinimumSectionOffset, SectionStartAlignment,
                      MinimumSectionOffset) ||
        std::any_of(Graph.sections().begin(), Graph.sections().end(),
                    [&](const auto &SectionValue) {
                      return SectionValue.Kind != SectionKind::BSS &&
                             SectionValue.FileOffset < MinimumSectionOffset;
                    }))
      return diagnosticError("Mach-O sections overlap the load commands");
    for (const auto &Value : Graph.rebases())
      if (Value.Format != ObjectFormat::MachO || Value.Width != 8 ||
          Value.Type != Info.RebaseType)
        return diagnosticError("unsupported Mach-O rebase");

    std::vector<const Symbol *> Symbols;
    for (const auto &Value : Graph.symbols())
      Symbols.push_back(&Value);
    std::stable_sort(Symbols.begin(), Symbols.end(),
                     [](const auto *L, const auto *R) {
                       return std::tuple(L->Global, L->Name) <
                              std::tuple(R->Global, R->Name);
                     });
    std::vector<Byte> Strings(1);
    std::vector<uint32_t> StringOffsets;
    for (const auto *Value : Symbols) {
      if (Value->Name.size() >= UINT32_MAX ||
          Strings.size() > UINT32_MAX - Value->Name.size() - 1)
        return diagnosticError("Mach-O string table overflows");
      StringOffsets.push_back(static_cast<uint32_t>(Strings.size()));
      Strings.insert(Strings.end(), Value->Name.begin(), Value->Name.end());
      Strings.push_back(0);
    }

    uint64_t TextAddress = UINT64_MAX;
    for (const auto &SectionValue : Graph.sections()) {
      if (SectionValue.Kind == SectionKind::BSS)
        continue;
      if (SectionValue.Address < SectionValue.FileOffset)
        return diagnosticError("Mach-O section address precedes its offset");
      const uint64_t Address = SectionValue.Address - SectionValue.FileOffset;
      if (TextAddress == UINT64_MAX)
        TextAddress = Address;
      else if (TextAddress != Address)
        return diagnosticError("Mach-O sections use inconsistent file bias");
    }
    uint64_t VirtualEnd = 0;
    uint64_t FileEnd = 0;
    if (TextAddress == UINT64_MAX ||
        !addUnsigned(HeaderSize, CommandsSize, VirtualEnd) ||
        !addUnsigned(TextAddress, VirtualEnd, VirtualEnd))
      return diagnosticError("Mach-O text segment address overflows");
    EXPECTED_TRY(auto Exports, exportTrie(Graph, TextAddress));
    FileEnd = HeaderSize + CommandsSize;
    for (const auto &SectionValue : Graph.sections()) {
      uint64_t End = 0;
      if (!addUnsigned(SectionValue.Address, SectionValue.VirtualSize, End))
        return diagnosticError("Mach-O section end overflows");
      VirtualEnd = std::max(VirtualEnd, End);
      if (SectionValue.Kind != SectionKind::BSS) {
        if (!addUnsigned(SectionValue.FileOffset, SectionValue.VirtualSize,
                         End))
          return diagnosticError("Mach-O section end overflows");
        FileEnd = std::max(FileEnd, End);
      }
    }
    uint64_t LinkEditAddress = 0;
    uint64_t LinkEditOffset = 0;
    if (!checkedAlign(VirtualEnd, Info.PageSize, LinkEditAddress) ||
        !checkedAlign(FileEnd, Info.PageSize, LinkEditOffset))
      return diagnosticError("Mach-O link-edit segment address overflows");
    auto Segments =
        segments(Graph, TextAddress, LinkEditAddress, LinkEditOffset, 0);
    if (!Segments)
      return diagnosticError("Mach-O segment layout overflows");
    EXPECTED_TRY(auto RebaseStream, rebaseStream(Graph, *Segments));

    const uint64_t RebaseOffset = LinkEditOffset;
    uint64_t RebaseEnd = 0;
    uint64_t ExportOffset = 0;
    uint64_t ExportEnd = 0;
    uint64_t SymbolOffset = 0;
    uint64_t SymbolSize = 0;
    uint64_t StringOffset = 0;
    uint64_t FileSize = 0;
    if (!addUnsigned(RebaseOffset, RebaseStream.size(), RebaseEnd) ||
        !checkedAlign(RebaseEnd, LinkEditAlignment, ExportOffset) ||
        !addUnsigned(ExportOffset, Exports.size(), ExportEnd) ||
        !checkedAlign(ExportEnd, LinkEditAlignment, SymbolOffset) ||
        !checkedMul(Symbols.size(), SymbolEntrySize, SymbolSize) ||
        !addUnsigned(SymbolOffset, SymbolSize, StringOffset) ||
        !addUnsigned(StringOffset, Strings.size(), FileSize) ||
        FileSize > UINT32_MAX || !fitsSizeT(FileSize))
      return diagnosticError("Mach-O link-edit data overflows");
    Segments->back().Size = FileSize - LinkEditOffset;
    Segments->back().FileSize = FileSize - LinkEditOffset;
    if (!validateSegments(Graph, *Segments, FileSize))
      return diagnosticError("invalid Mach-O segment layout");
    for (const auto &SegmentValue : *Segments) {
      if (SegmentValue.Sections.size() > UINT32_MAX ||
          SegmentValue.Sections.size() >
              (UINT32_MAX - SegmentCommandSize) / SectionCommandSize)
        return diagnosticError("Mach-O segment has too many sections");
    }

    std::vector<Byte> Bytes(static_cast<size_t>(FileSize));
    auto OutsideOutput = []() {
      return diagnosticError("Mach-O content is outside output");
    };
    {
      using llvm::MachO::mach_header_64;
      putInteger(Bytes, offsetof(mach_header_64, magic),
                 llvm::MachO::MH_MAGIC_64, 4);
      putInteger(Bytes, offsetof(mach_header_64, cputype), Info.CPUType, 4);
      putInteger(Bytes, offsetof(mach_header_64, cpusubtype), Info.CPUSubtype,
                 4);
      putInteger(Bytes, offsetof(mach_header_64, filetype),
                 llvm::MachO::MH_DYLIB, 4);
      putInteger(Bytes, offsetof(mach_header_64, ncmds),
                 FixedLoadCommandCount + Graph.machOBuildVersions().size(), 4);
      putInteger(Bytes, offsetof(mach_header_64, sizeofcmds), CommandsSize, 4);
      putInteger(Bytes, offsetof(mach_header_64, flags),
                 llvm::MachO::MH_NOUNDEFS | llvm::MachO::MH_DYLDLINK |
                     llvm::MachO::MH_TWOLEVEL,
                 4);
    }

    uint64_t Command = HeaderSize;
    std::vector<uint8_t> SectionOrdinals(Graph.sections().size());
    uint32_t Ordinal = 1;
    for (const auto &SegmentValue : *Segments) {
      using llvm::MachO::section_64;
      using llvm::MachO::segment_command_64;
      const uint64_t CommandSize =
          SegmentCommandSize +
          SectionCommandSize * SegmentValue.Sections.size();
      putInteger(Bytes, Command + offsetof(segment_command_64, cmd),
                 llvm::MachO::LC_SEGMENT_64, 4);
      putInteger(Bytes, Command + offsetof(segment_command_64, cmdsize),
                 CommandSize, 4);
      if (!putName(Bytes, Command + offsetof(segment_command_64, segname),
                   SegmentValue.Name))
        return OutsideOutput();
      putInteger(Bytes, Command + offsetof(segment_command_64, vmaddr),
                 SegmentValue.Address, 8);
      putInteger(Bytes, Command + offsetof(segment_command_64, vmsize),
                 SegmentValue.Size, 8);
      putInteger(Bytes, Command + offsetof(segment_command_64, fileoff),
                 SegmentValue.FileOffset, 8);
      putInteger(Bytes, Command + offsetof(segment_command_64, filesize),
                 SegmentValue.FileSize, 8);
      putInteger(Bytes, Command + offsetof(segment_command_64, maxprot),
                 SegmentValue.MaxProtection, 4);
      putInteger(Bytes, Command + offsetof(segment_command_64, initprot),
                 SegmentValue.InitialProtection, 4);
      putInteger(Bytes, Command + offsetof(segment_command_64, nsects),
                 SegmentValue.Sections.size(), 4);
      if (SegmentValue.Name == "__DATA_CONST")
        putInteger(Bytes, Command + offsetof(segment_command_64, flags),
                   SegmentReadOnlyFlag, 4);
      uint64_t SectionCommand = Command + SegmentCommandSize;
      for (const auto Id : SegmentValue.Sections) {
        const auto &SectionValue = Graph.sections()[Id];
        if (!putName(Bytes, SectionCommand + offsetof(section_64, sectname),
                     SectionValue.Name) ||
            !putName(Bytes, SectionCommand + offsetof(section_64, segname),
                     SegmentValue.Name))
          return OutsideOutput();
        putInteger(Bytes, SectionCommand + offsetof(section_64, addr),
                   SectionValue.Address, 8);
        putInteger(Bytes, SectionCommand + offsetof(section_64, size),
                   SectionValue.VirtualSize, 8);
        putInteger(Bytes, SectionCommand + offsetof(section_64, offset),
                   SectionValue.FileOffset, 4);
        putInteger(Bytes, SectionCommand + offsetof(section_64, align),
                   llvm::Log2_64(SectionValue.Alignment), 4);
        putInteger(Bytes, SectionCommand + offsetof(section_64, flags),
                   sectionFlags(SectionValue), 4);
        SectionOrdinals[Id] = static_cast<uint8_t>(Ordinal++);
        SectionCommand += SectionCommandSize;
      }
      Command += CommandSize;
    }
    {
      using llvm::MachO::dyld_info_command;
      putInteger(Bytes, Command + offsetof(dyld_info_command, cmd),
                 llvm::MachO::LC_DYLD_INFO_ONLY, 4);
      putInteger(Bytes, Command + offsetof(dyld_info_command, cmdsize),
                 DyldInfoCommandSize, 4);
      putInteger(Bytes, Command + offsetof(dyld_info_command, rebase_off),
                 RebaseOffset, 4);
      putInteger(Bytes, Command + offsetof(dyld_info_command, rebase_size),
                 RebaseStream.size(), 4);
      putInteger(Bytes, Command + offsetof(dyld_info_command, export_off),
                 ExportOffset, 4);
      putInteger(Bytes, Command + offsetof(dyld_info_command, export_size),
                 Exports.size(), 4);
      Command += DyldInfoCommandSize;
    }
    {
      using llvm::MachO::symtab_command;
      putInteger(Bytes, Command + offsetof(symtab_command, cmd),
                 llvm::MachO::LC_SYMTAB, 4);
      putInteger(Bytes, Command + offsetof(symtab_command, cmdsize),
                 SymtabCommandSize, 4);
      putInteger(Bytes, Command + offsetof(symtab_command, symoff),
                 SymbolOffset, 4);
      putInteger(Bytes, Command + offsetof(symtab_command, nsyms),
                 Symbols.size(), 4);
      putInteger(Bytes, Command + offsetof(symtab_command, stroff),
                 StringOffset, 4);
      putInteger(Bytes, Command + offsetof(symtab_command, strsize),
                 Strings.size(), 4);
      Command += SymtabCommandSize;
    }
    {
      using llvm::MachO::dysymtab_command;
      const auto FirstExternal = static_cast<uint32_t>(
          std::count_if(Symbols.begin(), Symbols.end(),
                        [](const auto *V) { return !V->Global; }));
      putInteger(Bytes, Command + offsetof(dysymtab_command, cmd),
                 llvm::MachO::LC_DYSYMTAB, 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, cmdsize),
                 DysymtabCommandSize, 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, ilocalsym), 0, 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, nlocalsym),
                 FirstExternal, 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, iextdefsym),
                 FirstExternal, 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, nextdefsym),
                 Symbols.size() - FirstExternal, 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, iundefsym),
                 Symbols.size(), 4);
      putInteger(Bytes, Command + offsetof(dysymtab_command, nundefsym), 0, 4);
      Command += DysymtabCommandSize;
    }
    if (!putDylibCommand(Bytes, Command, llvm::MachO::LC_ID_DYLIB, InstallName))
      return OutsideOutput();
    Command += dylibCommandSize(InstallName);
    if (!putDylibCommand(Bytes, Command, llvm::MachO::LC_LOAD_DYLIB,
                         SystemLibraryName))
      return OutsideOutput();
    Command += dylibCommandSize(SystemLibraryName);
    const uint64_t UUIDOffset = Command;
    putInteger(Bytes, Command + offsetof(llvm::MachO::uuid_command, cmd),
               llvm::MachO::LC_UUID, 4);
    putInteger(Bytes, Command + offsetof(llvm::MachO::uuid_command, cmdsize),
               UUIDCommandSize, 4);
    Command += UUIDCommandSize;
    for (const auto &Version : Graph.machOBuildVersions()) {
      using llvm::MachO::build_tool_version;
      using llvm::MachO::build_version_command;
      uint32_t Size = 0;
      if (!Internal::machOBuildVersionCommandSize(Version.Tools.size(), Size))
        return diagnosticError("Mach-O build version command overflows");
      putInteger(Bytes, Command + offsetof(build_version_command, cmd),
                 llvm::MachO::LC_BUILD_VERSION, 4);
      putInteger(Bytes, Command + offsetof(build_version_command, cmdsize),
                 Size, 4);
      putInteger(Bytes, Command + offsetof(build_version_command, platform),
                 Version.Platform, 4);
      putInteger(Bytes, Command + offsetof(build_version_command, minos),
                 Version.MinimumOS, 4);
      putInteger(Bytes, Command + offsetof(build_version_command, sdk),
                 Version.SDK, 4);
      putInteger(Bytes, Command + offsetof(build_version_command, ntools),
                 Version.Tools.size(), 4);
      uint64_t ToolCommand = Command + BuildVersionCommandSize;
      for (const auto &Tool : Version.Tools) {
        putInteger(Bytes, ToolCommand + offsetof(build_tool_version, tool),
                   Tool.Tool, 4);
        putInteger(Bytes, ToolCommand + offsetof(build_tool_version, version),
                   Tool.Version, 4);
        ToolCommand += BuildToolVersionSize;
      }
      Command += Size;
    }
    if (Command != HeaderSize + CommandsSize)
      return diagnosticError("Mach-O load command size mismatch");

    for (const auto &SectionValue : Graph.sections())
      if (SectionValue.Kind != SectionKind::BSS &&
          !copyAt(Bytes, SectionValue.FileOffset, SectionValue.Content.begin(),
                  SectionValue.Content.end()))
        return OutsideOutput();
    if (!copyAt(Bytes, RebaseOffset, RebaseStream.begin(),
                RebaseStream.end()) ||
        !copyAt(Bytes, ExportOffset, Exports.begin(), Exports.end()))
      return OutsideOutput();
    for (size_t I = 0; I < Symbols.size(); ++I) {
      using llvm::MachO::nlist_64;
      const auto &Value = *Symbols[I];
      uint64_t EntryOffset = 0;
      uint64_t Offset = 0;
      uint64_t Address = 0;
      if (!checkedMul(I, SymbolEntrySize, EntryOffset) ||
          !addUnsigned(SymbolOffset, EntryOffset, Offset) ||
          !addUnsigned(Graph.sections()[Value.Section].Address, Value.Offset,
                       Address))
        return diagnosticError("Mach-O symbol table entry overflows");
      putInteger(Bytes, Offset + offsetof(nlist_64, n_strx), StringOffsets[I],
                 4);
      Bytes[Offset + offsetof(nlist_64, n_type)] =
          llvm::MachO::N_SECT | (Value.Global ? llvm::MachO::N_EXT : 0);
      Bytes[Offset + offsetof(nlist_64, n_sect)] =
          SectionOrdinals[Value.Section];
      putInteger(Bytes, Offset + offsetof(nlist_64, n_value), Address, 8);
    }
    if (!copyAt(Bytes, StringOffset, Strings.begin(), Strings.end()))
      return OutsideOutput();
    std::array<uint8_t, 16> UUID = llvm::MD5::hash(llvm::ArrayRef<uint8_t>(
        reinterpret_cast<const uint8_t *>(Bytes.data()), Bytes.size()));
    UUID[6] = static_cast<uint8_t>((UUID[6] & 0x0F) | 0x30);
    UUID[8] = static_cast<uint8_t>((UUID[8] & 0x3F) | 0x80);
    if (!copyAt(Bytes, UUIDOffset + offsetof(llvm::MachO::uuid_command, uuid),
                UUID.begin(), UUID.end()))
      return OutsideOutput();
    return writeImage(Output, Bytes, "Mach-O");
  } catch (...) {
    return diagnosticError("Mach-O writer failed");
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
