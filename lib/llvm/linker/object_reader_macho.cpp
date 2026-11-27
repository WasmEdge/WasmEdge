// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/object_reader_context.h"

#include "linker/byte_io.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/relocation.h"

#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Object/MachO.h>
#include <llvm/Support/Casting.h>

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

using namespace Reader;

namespace {

constexpr uint64_t CompactUnwindRecordSize = 32;
constexpr uint64_t CompactUnwindFunctionOffset = 0;
constexpr uint64_t CompactUnwindLengthOffset = 8;
constexpr uint64_t CompactUnwindEncodingOffset = 12;
constexpr uint64_t CompactUnwindPersonalityOffset = 16;
constexpr uint64_t CompactUnwindLSDAOffset = 24;

} // namespace

bool Internal::supportsMachORelocationMetadata(Target TargetValue,
                                               bool Scattered) noexcept {
  return !Scattered ||
         (TargetValue != Target::X86_64 && TargetValue != Target::AArch64);
}

LinkExpect<void> Reader::associateRawX86_64MachOFDEs(LinkGraph &Graph) {
  if (Graph.format() != ObjectFormat::MachO || Graph.target() != Target::X86_64)
    return {};
  if (std::none_of(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   }))
    return {};

  std::set<std::pair<SectionId, uint64_t>> RelocatedFields;
  for (const auto &Relocation : Graph.relocations())
    RelocatedFields.emplace(Relocation.Section, Relocation.Offset);

  struct AddressSymbols {
    std::optional<SymbolId> Text;
    bool HasText = false;
    bool HasNonText = false;
  };
  std::map<uint64_t, AddressSymbols> SymbolsByAddress;
  for (SymbolId Symbol = 0; Symbol < Graph.symbols().size(); ++Symbol) {
    const auto &Value = Graph.symbols()[Symbol];
    if (Value.Section >= Graph.sections().size())
      continue;
    const auto &TargetSection = Graph.sections()[Value.Section];
    if (Value.Offset > UINT64_MAX - TargetSection.InputAddress)
      return diagnosticError("x86_64 Mach-O FDE symbol address overflows");
    auto &Address = SymbolsByAddress[TargetSection.InputAddress + Value.Offset];
    if (TargetSection.Kind != SectionKind::Text) {
      Address.HasNonText = true;
      continue;
    }
    Address.HasText = true;
    if (!Address.Text && !isSectionSymbol(Value.Name))
      Address.Text = Symbol;
  }

  for (SectionId Section = 0; Section < Graph.sections().size(); ++Section) {
    const auto &EHFrame = Graph.sections()[Section];
    if (EHFrame.Purpose != SectionPurpose::EHFrame)
      continue;
    auto Fields = machOEHFrameFields(EHFrame.Content, Target::X86_64);
    if (!Fields)
      return diagnosticError("malformed x86_64 Mach-O EH frame");
    for (const auto Field : *Fields) {
      if (RelocatedFields.count({Section, Field}) != 0)
        continue;
      auto Delta = Internal::readSigned(EHFrame.Content, Field, DoubleWordPatch,
                                        Endianness::Little);
      if (!Delta)
        return diagnosticError(
            "cannot read x86_64 Mach-O FDE initial location");
      auto Address = Internal::resolveMachOFDEAddress(0, EHFrame.InputAddress,
                                                      Field, *Delta);
      if (!Address)
        return diagnosticError("x86_64 Mach-O FDE target address overflows");

      const auto Symbols = SymbolsByAddress.find(*Address);
      if (Symbols == SymbolsByAddress.end() || !Symbols->second.Text) {
        if (Symbols != SymbolsByAddress.end() && Symbols->second.HasNonText &&
            !Symbols->second.HasText)
          return diagnosticError(
              "x86_64 Mach-O FDE target is not a text symbol");
        return diagnosticError("unresolved x86_64 Mach-O FDE target");
      }
      EXPECTED_TRY(Graph.addEHFrameReference(EHFrameReference{
          Section, static_cast<uint64_t>(Field), *Symbols->second.Text}));
    }
  }
  return {};
}

LinkExpect<MachOAddendPair> Reader::consumeAArch64MachOAddendPair(
    const llvm::object::ObjectFile &Object, Span<const Byte> Content,
    llvm::object::relocation_iterator &Relocation,
    const llvm::object::relocation_iterator &End) {
  const auto AddendMetadata =
      relocationMetadata(Object, *Relocation, Target::AArch64);
  if (!AddendMetadata || AddendMetadata->PatchSize != WordPatch ||
      AddendMetadata->PCRelative || AddendMetadata->External ||
      AddendMetadata->Scattered)
    return diagnosticError("malformed addend relocation metadata");

  // LLVM MC and lld pair this signed 24-bit addend with BRANCH26, PAGE21, or
  // PAGEOFF12 at the same address.
  const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&Object);
  const auto Raw = MachO->getRelocation(Relocation->getRawDataRefImpl());
  const uint32_t Payload =
      MachO->getPlainRelocationSymbolNum(Raw) & UINT32_C(0x00FFFFFF);
  int64_t Addend = static_cast<int64_t>(Payload);
  if ((Payload & UINT32_C(0x00800000)) != 0)
    Addend -= INT64_C(1) << 24;
  const uint64_t Offset = Relocation->getOffset();
  ++Relocation;
  if (Relocation == End)
    return diagnosticError("AArch64 Mach-O addend relocation lacks successor");
  if (Relocation->getType() > UINT32_MAX)
    return diagnosticError("relocation type is out of range");
  const uint32_t Type = static_cast<uint32_t>(Relocation->getType());
  if (Type != llvm::MachO::ARM64_RELOC_BRANCH26 &&
      Type != llvm::MachO::ARM64_RELOC_PAGE21 &&
      Type != llvm::MachO::ARM64_RELOC_PAGEOFF12)
    return diagnosticError(
        "unsupported AArch64 Mach-O addend relocation successor",
        DiagnosticKind::Unsupported);
  const auto Metadata =
      relocationMetadata(Object, *Relocation, Target::AArch64);
  const bool ExpectedPCRelative = Type != llvm::MachO::ARM64_RELOC_PAGEOFF12;
  if (!Metadata || Metadata->PatchSize != WordPatch || Metadata->Scattered ||
      !Metadata->External || Metadata->PCRelative != ExpectedPCRelative)
    return diagnosticError("malformed addend relocation successor");
  if (Relocation->getOffset() != Offset)
    return diagnosticError("AArch64 Mach-O addend relocation addresses differ");
  const auto Embedded =
      Internal::readUnsigned(Content, Offset, WordPatch, Endianness::Little);
  if (!Embedded)
    return diagnosticError("cannot read AArch64 Mach-O addend field");
  const uint32_t Instruction = static_cast<uint32_t>(*Embedded);
  const bool HasEmbeddedAddend =
      Type == llvm::MachO::ARM64_RELOC_BRANCH26
          ? (Instruction & UINT32_C(0x03FFFFFF)) != 0
      : Type == llvm::MachO::ARM64_RELOC_PAGE21
          ? (Instruction & UINT32_C(0x60FFFFE0)) != 0
          : (Instruction & UINT32_C(0x003FFC00)) != 0;
  if (HasEmbeddedAddend)
    return diagnosticError(
        "AArch64 Mach-O addend relocation conflicts with embedded addend");
  return MachOAddendPair{Type, *Metadata, Addend};
}

namespace Internal {

LinkExpect<MachOBuildVersion> parseMachOBuildVersion(Span<const Byte> Command,
                                                     Endianness Endian) {
  constexpr uint64_t HeaderSize = 24;
  constexpr uint64_t ToolSize = 8;
  constexpr uint64_t CommandOffset = 0;
  constexpr uint64_t CommandSizeOffset = 4;
  constexpr uint64_t PlatformOffset = 8;
  constexpr uint64_t MinimumOSOffset = 12;
  constexpr uint64_t SDKOffset = 16;
  constexpr uint64_t ToolCountOffset = 20;
  const bool LittleEndian = Endian == Endianness::Little;
  uint32_t Type = 0;
  uint32_t CommandSize = 0;
  uint32_t Platform = 0;
  uint32_t MinimumOS = 0;
  uint32_t SDK = 0;
  uint32_t ToolCount = 0;
  if (Command.size() < HeaderSize ||
      !readInteger(Command, CommandOffset, LittleEndian, Type) ||
      !readInteger(Command, CommandSizeOffset, LittleEndian, CommandSize) ||
      !readInteger(Command, PlatformOffset, LittleEndian, Platform) ||
      !readInteger(Command, MinimumOSOffset, LittleEndian, MinimumOS) ||
      !readInteger(Command, SDKOffset, LittleEndian, SDK) ||
      !readInteger(Command, ToolCountOffset, LittleEndian, ToolCount) ||
      Type != llvm::MachO::LC_BUILD_VERSION || CommandSize != Command.size() ||
      ToolCount > (Command.size() - HeaderSize) / ToolSize ||
      HeaderSize + static_cast<uint64_t>(ToolCount) * ToolSize !=
          Command.size())
    return diagnosticError("malformed Mach-O build version command");

  MachOBuildVersion Result{Platform, MinimumOS, SDK, {}};
  Result.Tools.reserve(ToolCount);
  for (uint32_t I = 0; I < ToolCount; ++I) {
    const uint64_t Offset = HeaderSize + static_cast<uint64_t>(I) * ToolSize;
    uint32_t Tool = 0;
    uint32_t Version = 0;
    if (!readInteger(Command, Offset, LittleEndian, Tool) ||
        !readInteger(Command, Offset + 4, LittleEndian, Version))
      return diagnosticError("malformed Mach-O build version command");
    Result.Tools.push_back(MachOBuildToolVersion{Tool, Version});
  }
  return Result;
}

LinkExpect<uint64_t> resolveCompactUnwindTargetOffset(bool External,
                                                      uint64_t SectionAddress,
                                                      uint64_t SymbolOffset,
                                                      uint64_t RawAddend) {
  if (!External) {
    if (RawAddend < SectionAddress)
      return diagnosticError(
          "compact unwind local target precedes its section");
    return RawAddend - SectionAddress;
  }
  if (RawAddend <= static_cast<uint64_t>(INT64_MAX)) {
    if (RawAddend > UINT64_MAX - SymbolOffset)
      return diagnosticError("compact unwind target address overflows");
    return SymbolOffset + RawAddend;
  }
  const uint64_t Magnitude = UINT64_MAX - RawAddend + 1;
  if (Magnitude > SymbolOffset)
    return diagnosticError("compact unwind target address underflows");
  return SymbolOffset - Magnitude;
}

LinkExpect<std::vector<DecodedCompactUnwindRecord>>
parseCompactUnwindSection(Target TargetValue, Span<const Byte> Content,
                          Span<const CompactUnwindRelocation> Relocations) {
  if (Content.size() % CompactUnwindRecordSize != 0)
    return diagnosticError("compact unwind size is not a multiple of 32");
  const uint32_t ExpectedType =
      TargetValue == Target::AArch64
          ? static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED)
      : TargetValue == Target::X86_64
          ? static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED)
          : UINT32_MAX;
  std::set<uint64_t> Fields;
  for (const auto &Relocation : Relocations) {
    const uint64_t Field = Relocation.Offset % CompactUnwindRecordSize;
    if (Relocation.Offset >= Content.size() ||
        (Field != CompactUnwindFunctionOffset &&
         Field != CompactUnwindPersonalityOffset &&
         Field != CompactUnwindLSDAOffset))
      return diagnosticError("unsupported compact unwind relocation field",
                             DiagnosticKind::Unsupported);
    if (!Fields.emplace(Relocation.Offset).second)
      return diagnosticError("duplicate compact unwind relocation field");
    if (Relocation.Type != ExpectedType ||
        Relocation.PatchSize != DoubleWordPatch || Relocation.PCRelative ||
        Relocation.Scattered)
      return diagnosticError("unsupported compact unwind relocation",
                             DiagnosticKind::Unsupported);
  }

  std::vector<DecodedCompactUnwindRecord> Records;
  Records.reserve(Content.size() / CompactUnwindRecordSize);
  for (uint64_t Offset = 0; Offset < Content.size();
       Offset += CompactUnwindRecordSize) {
    if (Fields.count(Offset + CompactUnwindFunctionOffset) == 0)
      return diagnosticError("compact unwind record lacks function relocation");
    DecodedCompactUnwindRecord Record{};
    if (!readInteger(Content, Offset + CompactUnwindFunctionOffset, true,
                     Record.Function) ||
        !readInteger(Content, Offset + CompactUnwindLengthOffset, true,
                     Record.Length) ||
        !readInteger(Content, Offset + CompactUnwindEncodingOffset, true,
                     Record.Encoding) ||
        !readInteger(Content, Offset + CompactUnwindPersonalityOffset, true,
                     Record.Personality) ||
        !readInteger(Content, Offset + CompactUnwindLSDAOffset, true,
                     Record.LSDA))
      return diagnosticError("cannot read compact unwind record");
    if ((Record.Personality != 0 &&
         Fields.count(Offset + CompactUnwindPersonalityOffset) == 0) ||
        (Record.LSDA != 0 &&
         Fields.count(Offset + CompactUnwindLSDAOffset) == 0))
      return diagnosticError("compact unwind target lacks relocation");
    Records.push_back(Record);
  }
  return Records;
}

} // namespace Internal

LinkExpect<void> ObjectReader::ReadContext::readMachOBuildVersions() {
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object);
  if (!MachO)
    return {};
  for (const auto &Command : MachO->load_commands()) {
    if (Command.C.cmd == llvm::MachO::LC_VERSION_MIN_MACOSX) {
      const auto VersionMin = MachO->getVersionMinLoadCommand(Command);
      if (auto Added = Graph.addMachOBuildVersion(
              MachOBuildVersion{llvm::MachO::PLATFORM_MACOS,
                                VersionMin.version,
                                VersionMin.sdk,
                                {},
                                true});
          !Added)
        return addContext(std::move(Added.error()),
                          "cannot preserve Mach-O deployment target");
      continue;
    }
    if (Command.C.cmd != llvm::MachO::LC_BUILD_VERSION)
      continue;
    const Span<const Byte> Bytes(reinterpret_cast<const Byte *>(Command.Ptr),
                                 Command.C.cmdsize);
    EXPECTED_TRY(auto Version,
                 Internal::parseMachOBuildVersion(Bytes, Graph.endianness()));
    if (auto Added = Graph.addMachOBuildVersion(std::move(Version)); !Added)
      return addContext(std::move(Added.error()),
                        "cannot preserve Mach-O build version");
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::collectMachOFunctionSections() {
  if (!Object.isMachO())
    return {};
  const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&Object);
  for (const auto &InputSection : Object.sections()) {
    const uint32_t Flags =
        MachO->is64Bit()
            ? MachO->getSection64(InputSection.getRawDataRefImpl()).flags
            : MachO->getSection(InputSection.getRawDataRefImpl()).flags;
    if ((Flags & (llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                  llvm::MachO::S_ATTR_SOME_INSTRUCTIONS)) != 0)
      MachOFunctionSections.emplace(InputSection.getIndex());
  }
  for (const auto &InputSymbol : Object.symbols()) {
    EXPECTED_TRY(const auto Type,
                 takeOr(InputSymbol.getType(), "cannot read symbol type"));
    EXPECTED_TRY(const uint32_t Flags, symbolFlags(InputSymbol));
    if (Type != llvm::object::SymbolRef::ST_Function &&
        (Flags & llvm::object::SymbolRef::SF_Executable) == 0)
      continue;
    EXPECTED_TRY(const auto InputSection,
                 takeOr(InputSymbol.getSection(),
                        "cannot read function symbol section"));
    if (InputSection != Object.section_end())
      MachOFunctionSections.emplace(InputSection->getIndex());
  }
  for (const auto &InputSection : Object.sections()) {
    EXPECTED_TRY(const llvm::StringRef Name,
                 takeOr(sectionName(InputSection), "cannot read section name"));
    if (sectionPurpose(Object, Name) != SectionPurpose::CompactUnwind)
      continue;
    for (const auto &Relocation : InputSection.relocations()) {
      if (Relocation.getOffset() % CompactUnwindRecordSize !=
          CompactUnwindFunctionOffset)
        continue;
      auto TargetSection = Object.section_end();
      const auto InputSymbol = Relocation.getSymbol();
      if (InputSymbol == Object.symbol_end()) {
        TargetSection = machORelocationSection(Relocation);
      } else {
        EXPECTED_TRY(TargetSection,
                     takeOr(InputSymbol->getSection(),
                            "cannot read compact unwind function section"));
      }
      if (TargetSection != Object.section_end())
        MachOFunctionSections.emplace(TargetSection->getIndex());
    }
  }
  return {};
}

LinkExpect<SymbolId> ObjectReader::ReadContext::resolveCompactUnwindTarget(
    const llvm::object::RelocationRef &Relocation, uint64_t Addend) {
  const auto Metadata = relocationMetadata(Object, Relocation, ActualTarget);
  if (!Metadata)
    return diagnosticError("malformed compact unwind relocation");
  const auto InputSymbol = Relocation.getSymbol();
  auto TargetSection = Object.section_end();
  uint64_t SymbolOffset = 0;
  if (InputSymbol == Object.symbol_end()) {
    if (!Object.isMachO())
      return diagnosticError("compact unwind relocation has no target symbol");
    TargetSection = machORelocationSection(Relocation);
  } else {
    EXPECTED_TRY(TargetSection,
                 takeOr(InputSymbol->getSection(),
                        "cannot read compact unwind target section"));
    EXPECTED_TRY(const uint64_t Address,
                 takeOr(InputSymbol->getAddress(),
                        "cannot read compact unwind target address"));
    if (TargetSection != Object.section_end()) {
      const uint64_t Base = TargetSection->getAddress();
      if (Address < Base)
        return diagnosticError("compact unwind target precedes its section");
      SymbolOffset = Address - Base;
    }
  }
  if (TargetSection == Object.section_end())
    return diagnosticError("compact unwind relocation target is undefined");
  const auto GraphSection = graphSectionOf(TargetSection);
  if (!GraphSection)
    return diagnosticError("compact unwind targets an unsupported section",
                           DiagnosticKind::Unsupported);
  EXPECTED_TRY(const uint64_t Offset,
               Internal::resolveCompactUnwindTargetOffset(
                   Metadata->External, TargetSection->getAddress(),
                   SymbolOffset, Addend));
  if (Offset >= Graph.sections()[*GraphSection].VirtualSize)
    return diagnosticError("compact unwind target is outside its section");
  return Locations.findOrAdd(
      Graph, *GraphSection, Offset, SectionSymbols::Exclude,
      preferVisibleSymbol, [&] {
        return "$compact_unwind." + std::to_string(TargetSection->getIndex()) +
               "." + std::to_string(Offset);
      });
}

LinkExpect<void>
ObjectReader::ReadContext::indexDwarfFDEs(DwarfFDEIndex &Index) {
  for (const auto &EHSection : Object.sections()) {
    const auto GraphSection = SectionIds.find(EHSection.getIndex());
    if (GraphSection == SectionIds.end() ||
        Graph.sections()[GraphSection->second].Purpose !=
            SectionPurpose::EHFrame)
      continue;
    EXPECTED_TRY(
        const llvm::StringRef EHContents,
        takeOr(EHSection.getContents(), "cannot read EH frame contents"));
    const Span<const Byte> EHBytes(
        reinterpret_cast<const Byte *>(EHContents.data()), EHContents.size());
    const size_t SectionIndex = Index.Sections.size();
    auto &Records = Index.Sections.emplace_back();
    Records.Section = GraphSection->second;
    uint64_t Cursor = 0;
    while (Cursor < EHBytes.size()) {
      uint32_t Length = 0;
      if (!readInteger(EHBytes, Cursor, true, Length) || Length == 0 ||
          Length == UINT32_MAX ||
          Length > EHBytes.size() - Cursor - Internal::EHFrameLengthFieldSize) {
        Records.Malformed = true;
        break;
      }
      uint32_t Id = 0;
      Records.FDEs.emplace(
          Cursor,
          readInteger(EHBytes, Cursor + Internal::EHFrameLengthFieldSize, true,
                      Id) &&
              Id != 0);
      Cursor += Internal::EHFrameLengthFieldSize + Length;
    }
    std::optional<std::pair<uint64_t, SymbolId>> Subtractor;
    for (const auto &Relocation : EHSection.relocations()) {
      if (ActualTarget == Target::AArch64 &&
          Relocation.getType() == llvm::MachO::ARM64_RELOC_SUBTRACTOR) {
        const auto InputSymbol = Relocation.getSymbol();
        if (InputSymbol == Object.symbol_end()) {
          Subtractor.reset();
          continue;
        }
        const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
        if (Symbol == SymbolIds.end()) {
          Subtractor.reset();
          continue;
        }
        Subtractor = std::pair{Relocation.getOffset(), Symbol->second};
        continue;
      }
      const auto InputSymbol = Relocation.getSymbol();
      SectionId TargetSection = InvalidSectionId;
      uint64_t TargetOffset = 0;
      if (ActualTarget == Target::AArch64 &&
          Relocation.getType() == llvm::MachO::ARM64_RELOC_UNSIGNED &&
          Subtractor && Subtractor->first == Relocation.getOffset()) {
        auto InputTargetSection = Object.section_end();
        if (InputSymbol == Object.symbol_end()) {
          InputTargetSection = machORelocationSection(Relocation);
        } else {
          EXPECTED_TRY(InputTargetSection,
                       takeOr(InputSymbol->getSection(),
                              "cannot read DWARF FDE target section"));
          const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
          if (Symbol == SymbolIds.end())
            continue;
          TargetOffset = Graph.symbols()[Symbol->second].Offset;
        }
        const auto GraphTargetSection = graphSectionOf(InputTargetSection);
        if (!GraphTargetSection)
          continue;
        TargetSection = *GraphTargetSection;
        const auto &SubtractorSymbol = Graph.symbols()[Subtractor->second];
        const uint64_t Field = Relocation.getOffset();
        if (SubtractorSymbol.Offset > Field)
          return diagnosticError("invalid DWARF FDE subtractor offset");
        uint64_t Raw = 0;
        if (!readInteger(EHBytes, Field, true, Raw))
          return diagnosticError("cannot read DWARF FDE addend");
        auto Resolved = Internal::resolveMachOFDEAddress(
            TargetOffset, 0, Field - SubtractorSymbol.Offset,
            Internal::signExtend(Raw, 64));
        if (!Resolved)
          return diagnosticError("DWARF FDE target address overflows");
        TargetOffset = *Resolved;
        Subtractor.reset();
      } else {
        if (InputSymbol == Object.symbol_end())
          continue;
        const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
        if (Symbol == SymbolIds.end())
          continue;
        const auto &Target = Graph.symbols()[Symbol->second];
        TargetSection = Target.Section;
        TargetOffset = Target.Offset;
      }
      Index.ByTarget[{TargetSection, TargetOffset}].push_back(
          Index.Candidates.size());
      Index.Candidates.push_back({SectionIndex, Relocation.getOffset()});
    }
  }
  return {};
}

LinkExpect<SymbolId>
ObjectReader::ReadContext::resolveDwarfFDE(SymbolId Function,
                                           uint32_t Encoding) {
  if (!Internal::isDwarfCompactUnwind(ActualTarget, Encoding))
    return diagnosticError("compact unwind encoding is not DWARF");
  if (Function >= Graph.symbols().size())
    return diagnosticError("invalid DWARF compact unwind function");
  const auto &FunctionSymbol = Graph.symbols()[Function];
  const uint32_t EncodedOffset =
      Encoding & Internal::CompactUnwindDwarfOffsetMask;
  if (!DwarfFDEs) {
    DwarfFDEs.emplace();
    if (auto Indexed = indexDwarfFDEs(*DwarfFDEs); !Indexed)
      DwarfFDEs->Error = std::move(Indexed.error());
  }
  const auto &Index = *DwarfFDEs;
  bool CanonicalCandidate = false;
  std::optional<std::pair<SectionId, uint64_t>> Match;
  const auto Candidates =
      Index.ByTarget.find({FunctionSymbol.Section, FunctionSymbol.Offset});
  if (Candidates != Index.ByTarget.end()) {
    for (const size_t Ordinal : Candidates->second) {
      const auto &Candidate = Index.Candidates[Ordinal];
      const auto &Records = Index.Sections[Candidate.Section];
      CanonicalCandidate = true;
      if (Candidate.Field < Internal::EHFrameRecordHeaderSize)
        return diagnosticError("invalid DWARF compact unwind FDE field");
      const uint64_t Record =
          Candidate.Field - Internal::EHFrameRecordHeaderSize;
      const auto Boundary = Records.FDEs.find(Record);
      if (Boundary == Records.FDEs.end()) {
        if (Records.Malformed)
          return diagnosticError("malformed EH frame record");
        return diagnosticError("DWARF compact unwind offset is not an FDE");
      }
      if (!Boundary->second)
        return diagnosticError("invalid DWARF compact unwind FDE");
      if (EncodedOffset != 0 && EncodedOffset != Record)
        continue;
      if (Match && *Match != std::pair{Records.Section, Record})
        return diagnosticError("ambiguous DWARF compact unwind FDE");
      Match = std::pair{Records.Section, Record};
    }
  }
  if (Index.Error)
    return diagnosticError(*Index.Error);
  if (!Match && EncodedOffset != 0 && CanonicalCandidate)
    return diagnosticError("DWARF compact unwind FDE offset mismatch");
  if (!Match)
    return diagnosticError("missing DWARF compact unwind FDE");
  return Locations.findOrAdd(
      Graph, Match->first, Match->second, SectionSymbols::Include, nullptr,
      [&] { return "$compact_unwind.fde." + std::to_string(Match->second); });
}

LinkExpect<void> ObjectReader::ReadContext::readCompactUnwind() {
  if (!CompactUnwindSection)
    return {};
  auto InputSection = std::find_if(
      Object.sections().begin(), Object.sections().end(),
      [&](const auto &S) { return S.getIndex() == *CompactUnwindSection; });
  if (InputSection == Object.sections().end())
    return diagnosticError("cannot find compact unwind section");
  EXPECTED_TRY(const llvm::StringRef Contents,
               takeOr(InputSection->getContents(),
                      "cannot read compact unwind contents"));
  const Span<const Byte> Bytes(reinterpret_cast<const Byte *>(Contents.data()),
                               Contents.size());
  if (Bytes.size() % CompactUnwindRecordSize != 0)
    return diagnosticError("compact unwind size is not a multiple of 32");

  std::map<uint64_t, llvm::object::RelocationRef> Relocations;
  std::vector<Internal::CompactUnwindRelocation> DecodedRelocations;
  for (const auto &Relocation : InputSection->relocations()) {
    const auto Metadata = relocationMetadata(Object, Relocation, ActualTarget);
    if (!Metadata || Relocation.getType() > UINT32_MAX)
      return diagnosticError("malformed compact unwind relocation");
    DecodedRelocations.push_back(Internal::CompactUnwindRelocation{
        Relocation.getOffset(), static_cast<uint32_t>(Relocation.getType()),
        Metadata->PatchSize, Metadata->PCRelative, Metadata->External,
        Metadata->Scattered});
    Relocations.emplace(Relocation.getOffset(), Relocation);
  }
  EXPECTED_TRY(const auto Records,
               Internal::parseCompactUnwindSection(ActualTarget, Bytes,
                                                   DecodedRelocations));

  for (size_t I = 0; I < Records.size(); ++I) {
    const uint64_t Offset = I * CompactUnwindRecordSize;
    const auto &Record = Records[I];
    const auto FunctionRelocation =
        Relocations.find(Offset + CompactUnwindFunctionOffset);
    if (FunctionRelocation == Relocations.end())
      return diagnosticError("compact unwind record lacks function relocation");
    EXPECTED_TRY(const SymbolId Function,
                 resolveCompactUnwindTarget(FunctionRelocation->second,
                                            Record.Function));
    const auto ResolveOptional =
        [&](uint64_t Field) -> LinkExpect<std::optional<SymbolId>> {
      const auto Relocation = Relocations.find(Offset + Field);
      if (Relocation == Relocations.end())
        return std::optional<SymbolId>{};
      uint64_t Addend = 0;
      if (!readInteger(Bytes, Offset + Field, true, Addend))
        return diagnosticError("cannot read compact unwind target");
      EXPECTED_TRY(const SymbolId Symbol,
                   resolveCompactUnwindTarget(Relocation->second, Addend));
      return std::optional<SymbolId>{Symbol};
    };
    auto Personality = ResolveOptional(CompactUnwindPersonalityOffset);
    auto LSDA = ResolveOptional(CompactUnwindLSDAOffset);
    if (!Personality)
      return diagnosticError(std::move(Personality.error()));
    if (!LSDA)
      return diagnosticError(std::move(LSDA.error()));
    std::optional<SymbolId> FDE;
    if (Internal::isDwarfCompactUnwind(ActualTarget, Record.Encoding)) {
      EXPECTED_TRY(FDE, resolveDwarfFDE(Function, Record.Encoding));
    }
    EXPECTED_TRY(Graph.addCompactUnwind(CompactUnwindRecord{
        Function, Record.Length, Record.Encoding, *Personality, *LSDA, FDE}));
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readAArch64MachOEHFramePair(
    SectionId Section, const RelocationMetadata &Metadata,
    llvm::object::relocation_iterator &Relocation,
    const llvm::object::relocation_iterator &End) {
  const auto &Input = Graph.sections()[Section];
  if (Metadata.PatchSize != DoubleWordPatch || Metadata.PCRelative ||
      !Metadata.External || Metadata.Scattered)
    return diagnosticError("malformed AArch64 Mach-O EH frame subtractor");
  const uint64_t Offset = Relocation->getOffset();
  auto CachedFields = MachOEHFrameFieldCache.find(Section);
  if (CachedFields == MachOEHFrameFieldCache.end()) {
    auto ParsedFields = machOEHFrameFields(Input.Content, ActualTarget);
    if (!ParsedFields)
      return diagnosticError("malformed Mach-O EH frame");
    CachedFields =
        MachOEHFrameFieldCache.emplace(Section, std::move(*ParsedFields)).first;
  }
  if (CachedFields->second.count(static_cast<size_t>(Offset)) == 0)
    return diagnosticError("AArch64 Mach-O EH frame relocation is not an FDE "
                           "initial-location field");
  const auto SubtractorInputSymbol = Relocation->getSymbol();
  if (SubtractorInputSymbol == Object.symbol_end())
    return diagnosticError("AArch64 Mach-O EH frame subtractor has no "
                           "target symbol");
  const auto Subtractor =
      SymbolIds.find(SubtractorInputSymbol->getRawDataRefImpl());
  if (Subtractor == SymbolIds.end() ||
      Graph.symbols()[Subtractor->second].Section != Section)
    return diagnosticError(
        "unsupported AArch64 Mach-O EH frame subtractor symbol",
        DiagnosticKind::Unsupported);
  ++Relocation;
  if (Relocation == End ||
      Relocation->getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
    return diagnosticError("AArch64 Mach-O EH frame subtractor is not followed "
                           "by unsigned relocation");
  const auto UnsignedMetadata =
      relocationMetadata(Object, *Relocation, ActualTarget);
  if (!UnsignedMetadata || UnsignedMetadata->PatchSize != DoubleWordPatch ||
      UnsignedMetadata->PCRelative || UnsignedMetadata->Scattered)
    return diagnosticError(
        "malformed AArch64 Mach-O EH frame unsigned relocation");
  if (Relocation->getOffset() != Offset)
    return diagnosticError(
        "AArch64 Mach-O EH frame relocation pair addresses differ");
  const auto UnsignedInputSymbol = Relocation->getSymbol();
  const auto &SubtractorSymbol = Graph.symbols()[Subtractor->second];
  if (SubtractorSymbol.Offset > Offset)
    return diagnosticError(
        "invalid AArch64 Mach-O EH frame subtractor symbol offset");
  uint64_t Raw = 0;
  if (!readInteger(Input.Content, Offset, true, Raw))
    return diagnosticError(
        "malformed AArch64 Mach-O EH frame relocation addend");
  const uint64_t Delta = Offset - SubtractorSymbol.Offset;
  SymbolId Unsigned = InvalidSymbolId;
  if (UnsignedMetadata->External) {
    if (UnsignedInputSymbol == Object.symbol_end())
      return diagnosticError("AArch64 Mach-O EH frame unsigned "
                             "relocation has no target symbol");
    const auto Symbol =
        SymbolIds.find(UnsignedInputSymbol->getRawDataRefImpl());
    if (Symbol == SymbolIds.end() || Raw != UINT64_C(0) - Delta)
      return diagnosticError(
          "malformed AArch64 Mach-O EH frame relocation addend");
    Unsigned = Symbol->second;
  } else {
    const auto GraphTargetSection =
        graphSectionOf(machORelocationSection(*Relocation));
    if (!GraphTargetSection)
      return diagnosticError(
          "AArch64 Mach-O EH frame relocation targets an unsupported "
          "section",
          DiagnosticKind::Unsupported);
    auto TargetOffset = Internal::resolveMachOFDEAddress(
        0, 0, Delta, Internal::signExtend(Raw, 64));
    if (!TargetOffset)
      return diagnosticError("DWARF FDE target address overflows");
    const auto Symbol = Locations.find(Graph, *GraphTargetSection,
                                       *TargetOffset, SectionSymbols::Exclude);
    if (!Symbol)
      return diagnosticError(
          "AArch64 Mach-O EH frame relocation targets an unsupported "
          "symbol",
          DiagnosticKind::Unsupported);
    Unsigned = *Symbol;
  }
  const auto &UnsignedSymbol = Graph.symbols()[Unsigned];
  if (UnsignedSymbol.Section >= Graph.sections().size() ||
      Graph.sections()[UnsignedSymbol.Section].Kind != SectionKind::Text)
    return diagnosticError(
        "AArch64 Mach-O EH frame relocation target is not a function "
        "symbol",
        DiagnosticKind::Unsupported);
  if (auto Added = Graph.addEHFrameReference(
          EHFrameReference{Section, Offset, Unsigned});
      !Added)
    return addContext(std::move(Added.error()),
                      "invalid Mach-O EH frame relocation");
  return {};
}

LinkExpect<std::optional<int64_t>>
ObjectReader::ReadContext::resolveMachOSectionAddend(
    const Section &Input, const llvm::object::RelocationRef &Relocation,
    llvm::object::section_iterator TargetSection, const Section &GraphTarget,
    uint32_t Type, const RelocationMetadata &Metadata,
    std::optional<uint8_t> PatchSize) {
  if (ActualTarget == Target::AArch64 &&
      (Type == llvm::MachO::ARM64_RELOC_BRANCH26 ||
       Type == llvm::MachO::ARM64_RELOC_PAGE21 ||
       Type == llvm::MachO::ARM64_RELOC_PAGEOFF12))
    return diagnosticError(
        "unsupported non-external AArch64 Mach-O section relocation",
        DiagnosticKind::Unsupported);
  if (ActualTarget == Target::X86_64 && Metadata.PCRelative) {
    uint64_t Suffix = 0;
    switch (Type) {
    case llvm::MachO::X86_64_RELOC_SIGNED:
    case llvm::MachO::X86_64_RELOC_BRANCH:
      break;
    case llvm::MachO::X86_64_RELOC_SIGNED_1:
      Suffix = 1;
      break;
    case llvm::MachO::X86_64_RELOC_SIGNED_2:
      Suffix = 2;
      break;
    case llvm::MachO::X86_64_RELOC_SIGNED_4:
      Suffix = 4;
      break;
    default:
      return diagnosticError(
          "unsupported PC-relative x86_64 Mach-O section relocation "
          "type",
          DiagnosticKind::Unsupported);
    }
    const uint64_t Offset = Relocation.getOffset();
    auto Raw = Internal::readSigned(Input.Content, Offset, WordPatch,
                                    Endianness::Little);
    if (!Raw)
      return diagnosticError(
          "cannot read x86_64 Mach-O section relocation addend");
    uint64_t Positive = Input.InputAddress;
    if (Offset > UINT64_MAX - Positive)
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    Positive += Offset;
    if (WordPatch > UINT64_MAX - Positive)
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    Positive += WordPatch;
    if (Suffix > UINT64_MAX - Positive)
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    Positive += Suffix;
    uint64_t Negative = TargetSection->getAddress();
    if (*Raw >= 0) {
      const uint64_t Value = static_cast<uint64_t>(*Raw);
      if (Value > UINT64_MAX - Positive)
        return diagnosticError(
            "x86_64 Mach-O section relocation offset overflows");
      Positive += Value;
    } else {
      const uint64_t Magnitude = static_cast<uint64_t>(-(*Raw + 1)) + 1;
      if (Magnitude > UINT64_MAX - Negative)
        return diagnosticError(
            "x86_64 Mach-O section relocation offset overflows");
      Negative += Magnitude;
    }
    if (Positive < Negative)
      return diagnosticError(
          "x86_64 Mach-O section relocation target is negative");
    const uint64_t TargetOffset = Positive - Negative;
    if (TargetOffset > static_cast<uint64_t>(INT64_MAX))
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    if (TargetOffset >= GraphTarget.VirtualSize)
      return diagnosticError(
          "x86_64 Mach-O section relocation target is outside its "
          "section");
    return static_cast<int64_t>(TargetOffset);
  }
  if (PatchSize && ((ActualTarget == Target::X86_64 &&
                     Type == llvm::MachO::X86_64_RELOC_UNSIGNED) ||
                    (ActualTarget == Target::AArch64 &&
                     Type == llvm::MachO::ARM64_RELOC_UNSIGNED))) {
    const auto Raw = Internal::readUnsigned(
        Input.Content, Relocation.getOffset(), *PatchSize,
        Object.isLittleEndian() ? Endianness::Little : Endianness::Big);
    if (!Raw || *Raw < TargetSection->getAddress() ||
        *Raw - TargetSection->getAddress() > static_cast<uint64_t>(INT64_MAX)) {
      return diagnosticError("invalid Mach-O section relocation target offset");
    }
    const uint64_t TargetOffset = *Raw - TargetSection->getAddress();
    if (TargetOffset >= GraphTarget.VirtualSize) {
      return diagnosticError(
          "Mach-O section relocation target is outside its section");
    }
    return static_cast<int64_t>(TargetOffset);
  }
  return std::optional<int64_t>{};
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
