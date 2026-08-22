// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/macho_writer.h"

#include <llvm/BinaryFormat/MachO.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <numeric>
#include <optional>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {
bool machOBuildVersionCommandSize(uint64_t ToolCount,
                                  uint32_t &Result) noexcept;
}

namespace {

constexpr uint32_t SegmentReadOnlyFlag = 0x10;

// Header, load-command, segment, symbol, dyld-info, and version fields follow
// the Mach-O ABI structures represented by llvm/BinaryFormat/MachO.h.
constexpr uint64_t X86PageSize = 4096;
constexpr uint64_t ARMPageSize = 16384;
constexpr uint64_t HeaderSize = 32;
constexpr uint64_t SegmentCommandSize = 72;
constexpr uint64_t SectionCommandSize = 80;
constexpr uint64_t DyldInfoCommandSize = 48;
constexpr uint64_t SymtabCommandSize = 24;
constexpr uint64_t DysymtabCommandSize = 80;
constexpr uint64_t BuildVersionCommandSize = 24;
constexpr uint64_t DylibCommandPrefixSize = 24;
constexpr uint64_t CodeSignatureCommandSize = 16;
constexpr uint32_t FixedLoadCommandCount = 8;
constexpr std::string_view InstallName = "@rpath/libwasmedge-aot.dylib";

Expect<void> fail() noexcept { return Unexpect(ErrCode::Value::IllegalPath); }

bool add(uint64_t A, uint64_t B, uint64_t &Result) noexcept {
  if (A > UINT64_MAX - B)
    return false;
  Result = A + B;
  return true;
}

bool multiply(uint64_t A, uint64_t B, uint64_t &Result) noexcept {
  if (A != 0 && B > UINT64_MAX / A)
    return false;
  Result = A * B;
  return true;
}

bool align(uint64_t Value, uint64_t Alignment, uint64_t &Result) noexcept {
  const uint64_t Mask = Alignment - 1;
  if (!add(Value, Mask, Result))
    return false;
  Result &= ~Mask;
  return true;
}

uint64_t pageSize(Target Architecture) noexcept {
  return Architecture == Target::AArch64 ? ARMPageSize : X86PageSize;
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

uint64_t dylibCommandSize() noexcept {
  uint64_t Result = DylibCommandPrefixSize + InstallName.size() + 1;
  return (Result + 7) & ~UINT64_C(7);
}

bool loadCommandSize(const LinkGraph &Graph, uint64_t &Result) noexcept {
  uint64_t SectionsSize = 0;
  if (!multiply(SectionCommandSize, Graph.sections().size(), SectionsSize))
    return false;
  Result = SegmentCommandSize * 4 + DyldInfoCommandSize + SymtabCommandSize +
           DysymtabCommandSize + dylibCommandSize();
  if (!add(Result, SectionsSize, Result) ||
      Graph.machOBuildVersions().size() > UINT32_MAX - FixedLoadCommandCount)
    return false;
  for (const auto &Version : Graph.machOBuildVersions()) {
    uint32_t Size = 0;
    if (!Internal::machOBuildVersionCommandSize(Version.Tools.size(), Size) ||
        !add(Result, Size, Result))
      return false;
  }
  return Result <= UINT32_MAX;
}

void put(std::vector<Byte> &Bytes, uint64_t Offset, uint64_t Value,
         uint8_t Width) {
  for (uint8_t I = 0; I < Width; ++I)
    Bytes[Offset + I] = static_cast<Byte>(Value >> (I * 8));
}

template <typename Iterator>
Expect<void> copyAt(std::vector<Byte> &Bytes, uint64_t Offset, Iterator Begin,
                    Iterator End) {
  const auto Size = static_cast<uint64_t>(std::distance(Begin, End));
  if (Offset > Bytes.size() || Size > Bytes.size() - Offset)
    return fail();
  std::copy(Begin, End, Bytes.begin() + Offset);
  return {};
}

Expect<void> putName(std::vector<Byte> &Bytes, uint64_t Offset,
                     std::string_view Name) {
  const auto Size = std::min<size_t>(Name.size(), 16);
  return copyAt(Bytes, Offset, Name.begin(), Name.begin() + Size);
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

struct Export {
  std::string Name;
  uint64_t Address;
};

std::optional<std::vector<Byte>> exportTrie(const LinkGraph &Graph,
                                            uint64_t ImageBase) {
  std::vector<Export> Exports;
  for (const auto &SymbolValue : Graph.symbols()) {
    if (!SymbolValue.Exported)
      continue;
    const auto &Name =
        SymbolValue.ExportName ? *SymbolValue.ExportName : SymbolValue.Name;
    uint64_t Address = 0;
    if (!add(Graph.sections()[SymbolValue.Section].Address, SymbolValue.Offset,
             Address) ||
        Address < ImageBase)
      return std::nullopt;
    Exports.push_back({Name, Address - ImageBase});
  }
  std::sort(Exports.begin(), Exports.end(),
            [](const auto &L, const auto &R) { return L.Name < R.Name; });
  for (size_t I = 1; I < Exports.size(); ++I)
    if (Exports[I - 1].Name == Exports[I].Name)
      return std::nullopt;
  struct Node {
    std::optional<uint64_t> Address;
    std::map<Byte, size_t> Children;
  };
  std::vector<Node> Nodes(1);
  for (const auto &Value : Exports) {
    if (Value.Name.empty() || Value.Name.find('\0') != Value.Name.npos)
      return std::nullopt;
    size_t NodeIndex = 0;
    for (const char Character : Value.Name) {
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
    Nodes[NodeIndex].Address = Value.Address;
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
      for (const auto &[Edge, Child] : NodeValue.Children) {
        static_cast<void>(Edge);
        Cursor += 2 + ulebSize(Offsets[Child]);
      }
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
      return std::nullopt;
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
  const uint64_t Page = pageSize(Graph.target());
  uint64_t CommandsSize = 0;
  uint64_t TextEnd = 0;
  if (!loadCommandSize(Graph, CommandsSize) ||
      !add(HeaderSize, CommandsSize, TextEnd) ||
      !add(TextAddress, TextEnd, TextEnd))
    return std::nullopt;
  uint64_t DataStart = LinkEditAddress;
  uint64_t DataEnd = 0;
  uint64_t DataFileStart = LinkEditOffset;
  uint64_t DataFileEnd = 0;
  for (const auto Id : Text) {
    const auto &SectionValue = Graph.sections()[Id];
    uint64_t End = 0;
    if (!add(SectionValue.Address, SectionValue.VirtualSize, End))
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
    if (!add(SectionValue.Address, SectionValue.VirtualSize, End) ||
        !add(SectionValue.FileOffset, SectionValue.VirtualSize, FileEnd))
      return std::nullopt;
    ConstantStart = std::min(ConstantStart, SectionValue.Address);
    ConstantEnd = std::max(ConstantEnd, End);
    ConstantFileStart = std::min(ConstantFileStart, SectionValue.FileOffset);
    ConstantFileEnd = std::max(ConstantFileEnd, FileEnd);
  }
  for (const auto Id : Data) {
    const auto &SectionValue = Graph.sections()[Id];
    uint64_t End = 0;
    if (!add(SectionValue.Address, SectionValue.VirtualSize, End))
      return std::nullopt;
    DataStart = std::min(DataStart, SectionValue.Address);
    DataEnd = std::max(DataEnd, End);
    if (SectionValue.Kind != SectionKind::BSS) {
      uint64_t FileEnd = 0;
      if (!add(SectionValue.FileOffset, SectionValue.VirtualSize, FileEnd))
        return std::nullopt;
      DataFileStart = std::min(DataFileStart, SectionValue.FileOffset);
      DataFileEnd = std::max(DataFileEnd, FileEnd);
    }
  }
  uint64_t TextSize = 0;
  uint64_t AlignedTextEnd = 0;
  if (!align(TextEnd, Page, AlignedTextEnd) || AlignedTextEnd < TextAddress)
    return std::nullopt;
  TextSize = AlignedTextEnd - TextAddress;
  if (Data.empty())
    DataStart = AlignedTextEnd;
  return std::vector<Segment>{
      {"__TEXT", TextAddress, TextSize, 0, TextSize,
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ) |
           static_cast<uint32_t>(llvm::MachO::VM_PROT_EXECUTE),
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ) |
           static_cast<uint32_t>(llvm::MachO::VM_PROT_EXECUTE),
       std::move(Text)},
      {"__DATA_CONST", Constant.empty() ? DataStart : ConstantStart,
       Constant.empty() ? 0 : ConstantEnd - ConstantStart,
       Constant.empty() ? DataFileStart : ConstantFileStart,
       Constant.empty() ? 0 : ConstantFileEnd - ConstantFileStart,
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ) |
           static_cast<uint32_t>(llvm::MachO::VM_PROT_WRITE),
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ) |
           static_cast<uint32_t>(llvm::MachO::VM_PROT_WRITE),
       std::move(Constant)},
      {"__DATA", DataStart, Data.empty() ? 0 : DataEnd - DataStart,
       DataFileStart,
       Data.empty() || DataFileEnd <= DataFileStart
           ? 0
           : DataFileEnd - DataFileStart,
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ) |
           static_cast<uint32_t>(llvm::MachO::VM_PROT_WRITE),
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ) |
           static_cast<uint32_t>(llvm::MachO::VM_PROT_WRITE),
       std::move(Data)},
      {"__LINKEDIT",
       LinkEditAddress,
       LinkEditSize,
       LinkEditOffset,
       LinkEditSize,
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ),
       static_cast<uint32_t>(llvm::MachO::VM_PROT_READ),
       {}}};
}

bool validateSegments(const LinkGraph &Graph,
                      const std::vector<Segment> &Segments,
                      uint64_t FileSize) noexcept {
  const uint64_t Page = pageSize(Graph.target());
  uint64_t PreviousAddressEnd = 0;
  uint64_t PreviousFileEnd = 0;
  for (const auto &SegmentValue : Segments) {
    uint64_t AddressEnd = 0;
    uint64_t FileEnd = 0;
    if (!add(SegmentValue.Address, SegmentValue.Size, AddressEnd) ||
        !add(SegmentValue.FileOffset, SegmentValue.FileSize, FileEnd) ||
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
      if (!add(SectionValue.Address, SectionValue.VirtualSize,
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
      if (!add(SectionValue.FileOffset, SectionValue.VirtualSize,
               SectionFileEnd) ||
          !add(SectionValue.FileOffset, SectionValue.Content.size(),
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

uint32_t alignmentPower(uint64_t Alignment) noexcept {
  uint32_t Result = 0;
  while (Alignment > 1) {
    Alignment >>= 1;
    ++Result;
  }
  return Result;
}

} // namespace

bool Internal::machOBuildVersionCommandSize(uint64_t ToolCount,
                                            uint32_t &Result) noexcept {
  uint64_t ToolsSize = 0;
  uint64_t Size = 0;
  if (!multiply(ToolCount, 8, ToolsSize) ||
      !add(BuildVersionCommandSize, ToolsSize, Size) || Size > UINT32_MAX)
    return false;
  Result = static_cast<uint32_t>(Size);
  return true;
}

Expect<void> MachOWriter::layout(LinkGraph &Graph) noexcept {
  try {
    if (!supported(Graph, false) || Graph.relocationsApplied() ||
        !Graph.validate())
      return fail();
    uint64_t CommandsSize = 0;
    uint64_t Address = 0;
    if (!loadCommandSize(Graph, CommandsSize) ||
        !add(HeaderSize, CommandsSize, Address) ||
        !add(Address, CodeSignatureCommandSize, Address) ||
        !align(Address, 16, Address))
      return fail();
    uint64_t FileOffset = Address;
    std::vector<std::pair<uint64_t, uint64_t>> Placements(
        Graph.sections().size());
    const std::array<SectionKind, 5> Kinds{
        SectionKind::Text, SectionKind::Unwind, SectionKind::ReadOnly,
        SectionKind::Data, SectionKind::BSS};
    for (const auto Kind : Kinds) {
      if ((Kind == SectionKind::ReadOnly || Kind == SectionKind::Data) &&
          (!align(Address, pageSize(Graph.target()), Address) ||
           !align(FileOffset, pageSize(Graph.target()), FileOffset)))
        return fail();
      std::vector<SectionId> Order;
      for (SectionId I = 0; I < Graph.sections().size(); ++I)
        if (Graph.sections()[I].Kind == Kind)
          Order.push_back(I);
      std::sort(Order.begin(), Order.end(), [&](auto L, auto R) {
        return std::tie(Graph.sections()[L].Name, L) <
               std::tie(Graph.sections()[R].Name, R);
      });
      for (const auto Id : Order) {
        const auto &SectionValue = Graph.sections()[Id];
        if (!align(Address, SectionValue.Alignment, Address))
          return fail();
        if (Kind != SectionKind::BSS &&
            !align(FileOffset, SectionValue.Alignment, FileOffset))
          return fail();
        Placements[Id] = {Address,
                          Kind == SectionKind::BSS ? uint64_t{0} : FileOffset};
        if (!add(Address, SectionValue.VirtualSize, Address) ||
            (Kind != SectionKind::BSS &&
             !add(FileOffset, SectionValue.VirtualSize, FileOffset)))
          return fail();
      }
    }
    for (SectionId I = 0; I < Placements.size(); ++I)
      if (!Graph.setSectionAddress(I, Placements[I].first) ||
          !Graph.setSectionFileOffset(I, Placements[I].second))
        return fail();
    return {};
  } catch (...) {
    return fail();
  }
}

Expect<void> MachOWriter::write(const LinkGraph &Graph,
                                Writer &Output) noexcept {
  try {
    if (!supported(Graph, true) || !Graph.relocationsApplied() ||
        !Graph.validate())
      return fail();
    if (Graph.sections().size() > UINT8_MAX ||
        Graph.symbols().size() > UINT32_MAX)
      return fail();
    uint64_t CommandsSize = 0;
    uint64_t MinimumSectionOffset = 0;
    if (!loadCommandSize(Graph, CommandsSize) || CommandsSize > UINT32_MAX ||
        !add(HeaderSize, CommandsSize, MinimumSectionOffset) ||
        !add(MinimumSectionOffset, CodeSignatureCommandSize,
             MinimumSectionOffset) ||
        !align(MinimumSectionOffset, 16, MinimumSectionOffset) ||
        std::any_of(Graph.sections().begin(), Graph.sections().end(),
                    [&](const auto &SectionValue) {
                      return SectionValue.Kind != SectionKind::BSS &&
                             SectionValue.FileOffset < MinimumSectionOffset;
                    }))
      return fail();
    for (const auto &Value : Graph.rebases())
      if (Value.Format != ObjectFormat::MachO || Value.Width != 8 ||
          Value.Type !=
              (Graph.target() == Target::X86_64
                   ? static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED)
                   : static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED)))
        return fail();

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
        return fail();
      StringOffsets.push_back(static_cast<uint32_t>(Strings.size()));
      Strings.insert(Strings.end(), Value->Name.begin(), Value->Name.end());
      Strings.push_back(0);
    }
    std::vector<Byte> RebaseStream;
    if (!Graph.rebases().empty()) {
      RebaseStream.push_back(static_cast<Byte>(
          static_cast<uint8_t>(llvm::MachO::REBASE_OPCODE_SET_TYPE_IMM) |
          static_cast<uint8_t>(llvm::MachO::REBASE_TYPE_POINTER)));
      std::vector<const Linker::Rebase *> Ordered;
      for (const auto &Value : Graph.rebases())
        Ordered.push_back(&Value);
      std::sort(
          Ordered.begin(), Ordered.end(), [&](const auto *L, const auto *R) {
            return std::tie(Graph.sections()[L->Section].Address, L->Offset) <
                   std::tie(Graph.sections()[R->Section].Address, R->Offset);
          });
      for (const auto *Value : Ordered) {
        const auto Kind = Graph.sections()[Value->Section].Kind;
        const uint8_t SegmentIndex = Kind == SectionKind::ReadOnly ? 1 : 2;
        uint64_t SegmentAddress = UINT64_MAX;
        for (const auto &SectionValue : Graph.sections())
          if ((SegmentIndex == 1 &&
               SectionValue.Kind == SectionKind::ReadOnly) ||
              (SegmentIndex == 2 && (SectionValue.Kind == SectionKind::Data ||
                                     SectionValue.Kind == SectionKind::BSS)))
            SegmentAddress = std::min(SegmentAddress, SectionValue.Address);
        if (SegmentAddress == UINT64_MAX)
          return fail();
        RebaseStream.push_back(static_cast<Byte>(
            static_cast<uint8_t>(
                llvm::MachO::REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB) |
            SegmentIndex));
        uint64_t Address = 0;
        if (!add(Graph.sections()[Value->Section].Address, Value->Offset,
                 Address))
          return fail();
        if (Address < SegmentAddress)
          return fail();
        appendULEB(RebaseStream, Address - SegmentAddress);
        RebaseStream.push_back(static_cast<Byte>(
            static_cast<uint8_t>(
                llvm::MachO::REBASE_OPCODE_DO_REBASE_IMM_TIMES) |
            uint8_t{1}));
      }
      RebaseStream.push_back(llvm::MachO::REBASE_OPCODE_DONE);
    }

    uint64_t VirtualEnd = 0;
    uint64_t FileEnd = 0;
    uint64_t TextAddress = UINT64_MAX;
    for (const auto &SectionValue : Graph.sections()) {
      if (SectionValue.Kind != SectionKind::BSS) {
        if (SectionValue.Address < SectionValue.FileOffset)
          return fail();
        const uint64_t Address = SectionValue.Address - SectionValue.FileOffset;
        if (TextAddress == UINT64_MAX)
          TextAddress = Address;
        else if (TextAddress != Address)
          return fail();
      }
    }
    if (TextAddress == UINT64_MAX ||
        !add(HeaderSize, CommandsSize, VirtualEnd) ||
        !add(TextAddress, VirtualEnd, VirtualEnd))
      return fail();
    auto Exports = exportTrie(Graph, TextAddress);
    if (!Exports)
      return fail();
    if (!add(HeaderSize, CommandsSize, FileEnd))
      return fail();
    for (const auto &SectionValue : Graph.sections()) {
      uint64_t End = 0;
      if (!add(SectionValue.Address, SectionValue.VirtualSize, End))
        return fail();
      VirtualEnd = std::max(VirtualEnd, End);
      if (SectionValue.Kind != SectionKind::BSS) {
        if (!add(SectionValue.FileOffset, SectionValue.VirtualSize, End))
          return fail();
        FileEnd = std::max(FileEnd, End);
      }
    }
    uint64_t LinkEditAddress = 0;
    uint64_t LinkEditOffset = 0;
    if (!align(VirtualEnd, pageSize(Graph.target()), LinkEditAddress) ||
        !align(FileEnd, pageSize(Graph.target()), LinkEditOffset))
      return fail();
    uint64_t RebaseOffset = LinkEditOffset;
    uint64_t ExportOffset = 0;
    uint64_t ExportEnd = 0;
    uint64_t SymbolOffset = 0;
    uint64_t SymbolSize = 0;
    uint64_t StringOffset = 0;
    uint64_t FileSize = 0;
    if (!add(RebaseOffset, RebaseStream.size(), ExportOffset) ||
        !add(ExportOffset, Exports->size(), ExportEnd) ||
        !align(ExportEnd, 8, SymbolOffset) ||
        !multiply(Symbols.size(), 16, SymbolSize) ||
        !add(SymbolOffset, SymbolSize, StringOffset) ||
        !add(StringOffset, Strings.size(), FileSize))
      return fail();
    if (FileSize > UINT32_MAX)
      return fail();
#if SIZE_MAX < UINT64_MAX
    if (FileSize > SIZE_MAX)
      return fail();
#endif
    auto Segments = segments(Graph, TextAddress, LinkEditAddress,
                             LinkEditOffset, FileSize - LinkEditOffset);
    if (!Segments || !validateSegments(Graph, *Segments, FileSize))
      return fail();
    for (const auto &SegmentValue : *Segments) {
      if (SegmentValue.Sections.size() > UINT32_MAX ||
          SegmentValue.Sections.size() >
              (UINT32_MAX - SegmentCommandSize) / SectionCommandSize)
        return fail();
    }
    std::vector<Byte> Bytes(static_cast<size_t>(FileSize));
    put(Bytes, 0, llvm::MachO::MH_MAGIC_64, 4);
    put(Bytes, 4,
        Graph.target() == Target::X86_64 ? llvm::MachO::CPU_TYPE_X86_64
                                         : llvm::MachO::CPU_TYPE_ARM64,
        4);
    put(Bytes, 8,
        Graph.target() == Target::X86_64
            ? static_cast<uint32_t>(llvm::MachO::CPU_SUBTYPE_X86_64_ALL)
            : static_cast<uint32_t>(llvm::MachO::CPU_SUBTYPE_ARM64_ALL),
        4);
    put(Bytes, 12, llvm::MachO::MH_DYLIB, 4);
    const uint32_t CommandCount = static_cast<uint32_t>(
        FixedLoadCommandCount + Graph.machOBuildVersions().size());
    put(Bytes, 16, CommandCount, 4);
    put(Bytes, 20, CommandsSize, 4);
    put(Bytes, 24,
        llvm::MachO::MH_NOUNDEFS | llvm::MachO::MH_DYLDLINK |
            llvm::MachO::MH_TWOLEVEL,
        4);

    uint64_t Command = HeaderSize;
    std::vector<uint8_t> SectionOrdinals(Graph.sections().size());
    uint32_t Ordinal = 1;
    for (const auto &SegmentValue : *Segments) {
      put(Bytes, Command, llvm::MachO::LC_SEGMENT_64, 4);
      put(Bytes, Command + 4,
          SegmentCommandSize +
              SectionCommandSize * SegmentValue.Sections.size(),
          4);
      EXPECTED_TRY(putName(Bytes, Command + 8, SegmentValue.Name));
      put(Bytes, Command + 24, SegmentValue.Address, 8);
      put(Bytes, Command + 32, SegmentValue.Size, 8);
      put(Bytes, Command + 40, SegmentValue.FileOffset, 8);
      put(Bytes, Command + 48, SegmentValue.FileSize, 8);
      put(Bytes, Command + 56, SegmentValue.MaxProtection, 4);
      put(Bytes, Command + 60, SegmentValue.InitialProtection, 4);
      put(Bytes, Command + 64, SegmentValue.Sections.size(), 4);
      if (SegmentValue.Name == "__DATA_CONST")
        put(Bytes, Command + 68, SegmentReadOnlyFlag, 4);
      uint64_t SectionCommand = Command + SegmentCommandSize;
      for (const auto Id : SegmentValue.Sections) {
        const auto &SectionValue = Graph.sections()[Id];
        EXPECTED_TRY(putName(Bytes, SectionCommand, SectionValue.Name));
        EXPECTED_TRY(putName(Bytes, SectionCommand + 16, SegmentValue.Name));
        put(Bytes, SectionCommand + 32, SectionValue.Address, 8);
        put(Bytes, SectionCommand + 40, SectionValue.VirtualSize, 8);
        put(Bytes, SectionCommand + 48, SectionValue.FileOffset, 4);
        put(Bytes, SectionCommand + 52, alignmentPower(SectionValue.Alignment),
            4);
        put(Bytes, SectionCommand + 64, sectionFlags(SectionValue), 4);
        SectionOrdinals[Id] = static_cast<uint8_t>(Ordinal++);
        SectionCommand += SectionCommandSize;
      }
      Command += SegmentCommandSize +
                 SectionCommandSize * SegmentValue.Sections.size();
    }
    put(Bytes, Command, llvm::MachO::LC_DYLD_INFO_ONLY, 4);
    put(Bytes, Command + 4, DyldInfoCommandSize, 4);
    put(Bytes, Command + 8, RebaseOffset, 4);
    put(Bytes, Command + 12, RebaseStream.size(), 4);
    put(Bytes, Command + 40, ExportOffset, 4);
    put(Bytes, Command + 44, Exports->size(), 4);
    Command += DyldInfoCommandSize;
    put(Bytes, Command, llvm::MachO::LC_SYMTAB, 4);
    put(Bytes, Command + 4, SymtabCommandSize, 4);
    put(Bytes, Command + 8, SymbolOffset, 4);
    put(Bytes, Command + 12, Symbols.size(), 4);
    put(Bytes, Command + 16, StringOffset, 4);
    put(Bytes, Command + 20, Strings.size(), 4);
    Command += SymtabCommandSize;
    put(Bytes, Command, llvm::MachO::LC_DYSYMTAB, 4);
    put(Bytes, Command + 4, DysymtabCommandSize, 4);
    const auto FirstExternal = static_cast<uint32_t>(
        std::count_if(Symbols.begin(), Symbols.end(),
                      [](const auto *V) { return !V->Global; }));
    put(Bytes, Command + 8, 0, 4);
    put(Bytes, Command + 12, FirstExternal, 4);
    put(Bytes, Command + 16, FirstExternal, 4);
    put(Bytes, Command + 20, Symbols.size() - FirstExternal, 4);
    put(Bytes, Command + 24, Symbols.size(), 4);
    put(Bytes, Command + 28, 0, 4);
    Command += DysymtabCommandSize;
    put(Bytes, Command, llvm::MachO::LC_ID_DYLIB, 4);
    put(Bytes, Command + 4, dylibCommandSize(), 4);
    put(Bytes, Command + 8, DylibCommandPrefixSize, 4);
    // dylib_command encodes current and compatibility versions as X.Y.Z; this
    // generated library declares version 1.0.0 for both fields.
    put(Bytes, Command + 16, UINT32_C(0x10000), 4);
    put(Bytes, Command + 20, UINT32_C(0x10000), 4);
    EXPECTED_TRY(copyAt(Bytes, Command + DylibCommandPrefixSize,
                        InstallName.begin(), InstallName.end()));
    Command += dylibCommandSize();
    for (const auto &Version : Graph.machOBuildVersions()) {
      uint32_t Size = 0;
      if (!Internal::machOBuildVersionCommandSize(Version.Tools.size(), Size))
        return fail();
      put(Bytes, Command, llvm::MachO::LC_BUILD_VERSION, 4);
      put(Bytes, Command + 4, Size, 4);
      put(Bytes, Command + 8, Version.Platform, 4);
      put(Bytes, Command + 12, Version.MinimumOS, 4);
      put(Bytes, Command + 16, Version.SDK, 4);
      put(Bytes, Command + 20, Version.Tools.size(), 4);
      uint64_t ToolCommand = Command + BuildVersionCommandSize;
      for (const auto &Tool : Version.Tools) {
        put(Bytes, ToolCommand, Tool.Tool, 4);
        put(Bytes, ToolCommand + 4, Tool.Version, 4);
        ToolCommand += 8;
      }
      Command += Size;
    }
    if (Command != HeaderSize + CommandsSize)
      return fail();

    for (const auto &SectionValue : Graph.sections()) {
      if (SectionValue.Kind != SectionKind::BSS) {
        EXPECTED_TRY(copyAt(Bytes, SectionValue.FileOffset,
                            SectionValue.Content.begin(),
                            SectionValue.Content.end()));
      }
    }
    EXPECTED_TRY(
        copyAt(Bytes, RebaseOffset, RebaseStream.begin(), RebaseStream.end()));
    EXPECTED_TRY(copyAt(Bytes, ExportOffset, Exports->begin(), Exports->end()));
    for (size_t I = 0; I < Symbols.size(); ++I) {
      const auto &Value = *Symbols[I];
      uint64_t EntryOffset = 0;
      uint64_t Offset = 0;
      uint64_t Address = 0;
      if (!multiply(I, 16, EntryOffset) ||
          !add(SymbolOffset, EntryOffset, Offset) ||
          !add(Graph.sections()[Value.Section].Address, Value.Offset, Address))
        return fail();
      put(Bytes, Offset, StringOffsets[I], 4);
      Bytes[Offset + 4] =
          llvm::MachO::N_SECT | (Value.Global ? llvm::MachO::N_EXT : 0);
      Bytes[Offset + 5] = SectionOrdinals[Value.Section];
      put(Bytes, Offset + 8, Address, 8);
    }
    EXPECTED_TRY(copyAt(Bytes, StringOffset, Strings.begin(), Strings.end()));
    EXPECTED_TRY(Output.write(Bytes));
    return Output.close();
  } catch (...) {
    return fail();
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
