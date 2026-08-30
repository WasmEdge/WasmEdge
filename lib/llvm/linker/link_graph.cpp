// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/link_graph.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

constexpr uint64_t EHFrameReferenceSize = 8;
constexpr std::string_view MachOUnwindInfoSectionName = "__unwind_info";

bool extendsBeyond(uint64_t Offset, uint64_t Size, uint64_t Limit) noexcept {
  return Offset > Limit || Size > Limit - Offset;
}

uint8_t rebaseWidth(const Rebase &Value) noexcept {
  return std::max<uint8_t>(Value.Width, MinimumRebaseWidth);
}

bool isUnwindInfoSection(const Section &Value) noexcept {
  return Value.Purpose == SectionPurpose::UnwindInfo &&
         Value.Name == MachOUnwindInfoSectionName &&
         Value.Kind == SectionKind::Unwind;
}

template <typename ContentSizeFn>
bool isValidEHFrameReference(const EHFrameReference &Value,
                             const std::vector<Section> &Sections,
                             size_t SymbolCount, ContentSizeFn ContentSize) {
  return Value.Section < Sections.size() && Value.Symbol < SymbolCount &&
         Sections[Value.Section].Purpose == SectionPurpose::EHFrame &&
         !extendsBeyond(Value.Offset, EHFrameReferenceSize,
                        ContentSize(Value.Section));
}

Diagnostic patchDiag(std::string Message, SectionId Section, uint32_t Type,
                     uint64_t Offset) {
  Diagnostic Diag{std::move(Message)};
  Diag.Section = Section;
  Diag.RelocationType = Type;
  Diag.Offset = Offset;
  return Diag;
}

Diagnostic relocationDiag(std::string Message, const Relocation &Value) {
  Diagnostic Diag =
      patchDiag(std::move(Message), Value.Section, Value.Type, Value.Offset);
  Diag.Symbol = Value.Symbol;
  return Diag;
}

Diagnostic rebaseDiag(std::string Message, const Rebase &Value) {
  return patchDiag(std::move(Message), Value.Section, Value.Type, Value.Offset);
}

Diagnostic symbolDiag(std::string Message, const Symbol &Value,
                      std::optional<SymbolId> Id) {
  Diagnostic Diag{std::move(Message)};
  Diag.Section = Value.Section;
  Diag.Symbol = Id;
  Diag.SymbolName = Value.Name;
  Diag.Offset = Value.Offset;
  return Diag;
}

Diagnostic sectionDiag(std::string Message, const Section &Value,
                       std::optional<SectionId> Id) {
  Diagnostic Diag{std::move(Message)};
  Diag.Section = Id;
  Diag.SectionName = Value.Name;
  return Diag;
}

Diagnostic sectionIdDiag(std::string Message, SectionId Id) {
  Diagnostic Diag{std::move(Message)};
  Diag.Section = Id;
  return Diag;
}

bool isRISCVSymbolDifferenceAdd(const Relocation &Value) noexcept {
  return (Value.PatchSize == BytePatch &&
          (Value.Type == llvm::ELF::R_RISCV_ADD8 ||
           Value.Type == llvm::ELF::R_RISCV_SET8)) ||
         (Value.PatchSize == WordPatch &&
          Value.Type == llvm::ELF::R_RISCV_ADD32);
}

bool isRISCVSymbolDifferenceSub(const Relocation &Value) noexcept {
  return (Value.PatchSize == BytePatch &&
          Value.Type == llvm::ELF::R_RISCV_SUB8) ||
         (Value.PatchSize == WordPatch &&
          Value.Type == llvm::ELF::R_RISCV_SUB32);
}

bool isRISCVSymbolDifference(const Relocation &Value) noexcept {
  return Value.Format == ObjectFormat::ELF &&
         (isRISCVSymbolDifferenceAdd(Value) ||
          isRISCVSymbolDifferenceSub(Value));
}

bool composeRISCVSymbolDifference(const Relocation &Left,
                                  const Relocation &Right) noexcept {
  return isRISCVSymbolDifference(Left) && isRISCVSymbolDifference(Right) &&
         Left.Section == Right.Section && Left.Offset == Right.Offset &&
         Left.Format == Right.Format && Left.PatchSize == Right.PatchSize &&
         ((isRISCVSymbolDifferenceAdd(Left) &&
           isRISCVSymbolDifferenceSub(Right)) ||
          (isRISCVSymbolDifferenceSub(Left) &&
           isRISCVSymbolDifferenceAdd(Right)));
}

template <typename T, typename Size>
bool overlaps(const T &Left, const T &Right, Size GetSize) noexcept {
  if (Left.Section != Right.Section) {
    return false;
  }
  const uint64_t LeftSize = GetSize(Left);
  const uint64_t RightSize = GetSize(Right);
  if (Left.Offset <= Right.Offset) {
    return LeftSize > Right.Offset - Left.Offset;
  }
  return RightSize > Left.Offset - Right.Offset;
}

template <typename IntervalMap, typename ConflictFn>
bool hasNeighborConflict(const IntervalMap &Intervals, SectionId Section,
                         uint64_t Offset, uint64_t Width,
                         ConflictFn Conflicts) {
  const auto First = Intervals.lower_bound(std::make_pair(Section, Offset));
  if (First != Intervals.begin()) {
    const auto Previous = std::prev(First);
    if (Previous->first.first == Section && Conflicts(*Previous))
      return true;
  }
  for (auto It = First;
       It != Intervals.end() && It->first.first == Section &&
       It->first.second >= Offset && Width > It->first.second - Offset;
       ++It) {
    if (Conflicts(*It))
      return true;
  }
  return false;
}

LinkExpect<uint64_t> compactUnwindAddress(const CompactUnwindRecord &Value,
                                          const std::vector<Section> &Sections,
                                          const std::vector<Symbol> &Symbols) {
  const auto Fail = [&](std::string Message,
                        std::optional<SectionId> Section = std::nullopt,
                        std::optional<uint64_t> Offset = std::nullopt) {
    Diagnostic Diag{std::move(Message)};
    Diag.Symbol = Value.Function;
    Diag.Section = Section;
    Diag.Offset = Offset;
    return diagnosticError(std::move(Diag));
  };
  if (Value.Function >= Symbols.size())
    return Fail("invalid compact unwind function symbol ID");
  const auto &Function = Symbols[Value.Function];
  if (Function.Section >= Sections.size())
    return Fail("invalid compact unwind function section ID", Function.Section);
  const auto &Section = Sections[Function.Section];
  if (Section.Kind != SectionKind::Text)
    return Fail("compact unwind function must reference text section",
                Function.Section);
  if (Value.Length == 0)
    return Fail("compact unwind function length must be non-zero");
  if (extendsBeyond(Function.Offset, Value.Length, Section.VirtualSize))
    return Fail("compact unwind function range exceeds text section",
                Function.Section, Function.Offset);
  if (Function.Offset > UINT64_MAX - Section.InputAddress)
    return Fail("compact unwind function address overflows");
  const uint64_t Address = Section.InputAddress + Function.Offset;
  if (Value.Length > UINT64_MAX - Address)
    return Fail("compact unwind function address overflows");
  return Address;
}

LinkExpect<void>
validateCompactUnwindSymbol(std::optional<SymbolId> Id, std::string_view Name,
                            const std::vector<Section> &Sections,
                            const std::vector<Symbol> &Symbols,
                            bool RequireEHFrame) {
  if (!Id)
    return {};
  if (*Id >= Symbols.size()) {
    Diagnostic Diag{"invalid compact unwind " + std::string(Name) +
                    " symbol ID"};
    Diag.Symbol = *Id;
    return diagnosticError(std::move(Diag));
  }
  const auto &Symbol = Symbols[*Id];
  if (Symbol.Section >= Sections.size()) {
    return diagnosticError(symbolDiag("invalid compact unwind " +
                                          std::string(Name) + " section ID",
                                      Symbol, *Id));
  }
  const auto &Section = Sections[Symbol.Section];
  const auto FailForSymbol = [&](std::string Message) {
    Diagnostic Diag = symbolDiag(std::move(Message), Symbol, *Id);
    Diag.SectionName = Section.Name;
    return diagnosticError(std::move(Diag));
  };
  if (Symbol.Offset >= Section.VirtualSize) {
    return FailForSymbol("compact unwind symbol is outside section storage");
  }
  if (RequireEHFrame && Section.Purpose != SectionPurpose::EHFrame) {
    return FailForSymbol(
        "compact unwind FDE must reference an EH frame section");
  }
  if (!RequireEHFrame && Section.Kind == SectionKind::BSS) {
    return FailForSymbol("compact unwind " + std::string(Name) +
                         " must reference a non-BSS section");
  }
  return {};
}

LinkExpect<uint64_t>
validateCompactUnwindRecord(const CompactUnwindRecord &Value,
                            const std::vector<Section> &Sections,
                            const std::vector<Symbol> &Symbols) {
  EXPECTED_TRY(const uint64_t Address,
               compactUnwindAddress(Value, Sections, Symbols));
  EXPECTED_TRY(validateCompactUnwindSymbol(Value.Personality, "personality",
                                           Sections, Symbols, false));
  EXPECTED_TRY(validateCompactUnwindSymbol(Value.LSDA, "LSDA", Sections,
                                           Symbols, false));
  EXPECTED_TRY(
      validateCompactUnwindSymbol(Value.FDE, "FDE", Sections, Symbols, true));
  return Address;
}

LinkExpect<void> validateCompactUnwindOrder(const CompactUnwindRecord &Previous,
                                            uint64_t PreviousAddress,
                                            uint64_t Address) {
  if (Address < PreviousAddress)
    return diagnosticError("compact unwind records are unordered");
  if (Address == PreviousAddress) {
    return diagnosticError("duplicate compact unwind function address");
  }
  if (Address - PreviousAddress < Previous.Length) {
    return diagnosticError("overlapping compact unwind function ranges");
  }
  return {};
}

} // namespace

std::optional<uint8_t> relocationPatchSize(ObjectFormat Format,
                                           Target TargetValue, uint32_t Type,
                                           uint8_t MetadataSize) noexcept {
  if (TargetValue == Target::X86_64) {
    if (Format == ObjectFormat::ELF) {
      switch (Type) {
      case llvm::ELF::R_X86_64_64:
        return DoubleWordPatch;
      case llvm::ELF::R_X86_64_PC32:
      case llvm::ELF::R_X86_64_PLT32:
      case llvm::ELF::R_X86_64_GOTPCRELX:
      case llvm::ELF::R_X86_64_REX_GOTPCRELX:
        return WordPatch;
      default:
        return std::nullopt;
      }
    }
    if (Format == ObjectFormat::MachO) {
      switch (Type) {
      case llvm::MachO::X86_64_RELOC_UNSIGNED:
        return MetadataSize == DoubleWordPatch
                   ? std::optional<uint8_t>{DoubleWordPatch}
                   : std::nullopt;
      case llvm::MachO::X86_64_RELOC_SIGNED:
      case llvm::MachO::X86_64_RELOC_SIGNED_1:
      case llvm::MachO::X86_64_RELOC_SIGNED_2:
      case llvm::MachO::X86_64_RELOC_SIGNED_4:
      case llvm::MachO::X86_64_RELOC_BRANCH:
        return WordPatch;
      default:
        return std::nullopt;
      }
    }
    if (Format == ObjectFormat::COFF) {
      if (Type == llvm::COFF::IMAGE_REL_AMD64_ADDR64)
        return DoubleWordPatch;
      if ((Type >= llvm::COFF::IMAGE_REL_AMD64_REL32 &&
           Type <= llvm::COFF::IMAGE_REL_AMD64_REL32_5) ||
          Type == llvm::COFF::IMAGE_REL_AMD64_ADDR32NB) {
        return WordPatch;
      }
    }
    return std::nullopt;
  }
  if (TargetValue == Target::AArch64 && Format == ObjectFormat::MachO) {
    switch (Type) {
    case llvm::MachO::ARM64_RELOC_UNSIGNED:
      return MetadataSize == DoubleWordPatch
                 ? std::optional<uint8_t>{DoubleWordPatch}
                 : std::nullopt;
    case llvm::MachO::ARM64_RELOC_BRANCH26:
    case llvm::MachO::ARM64_RELOC_PAGE21:
    case llvm::MachO::ARM64_RELOC_PAGEOFF12:
      return WordPatch;
    default:
      return std::nullopt;
    }
  }
  if (TargetValue == Target::AArch64 && Format == ObjectFormat::COFF) {
    switch (Type) {
    case llvm::COFF::IMAGE_REL_ARM64_ADDR64:
      return DoubleWordPatch;
    case llvm::COFF::IMAGE_REL_ARM64_BRANCH26:
    case llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21:
    case llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A:
    case llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L:
    case llvm::COFF::IMAGE_REL_ARM64_ADDR32NB:
      return WordPatch;
    default:
      return std::nullopt;
    }
  }
  if (Format != ObjectFormat::ELF)
    return std::nullopt;
  if (TargetValue == Target::ARM) {
    switch (Type) {
    case llvm::ELF::R_ARM_NONE:
      return NoPatch;
    case llvm::ELF::R_ARM_ABS32:
    case llvm::ELF::R_ARM_REL32:
    case llvm::ELF::R_ARM_THM_CALL:
    case llvm::ELF::R_ARM_THM_JUMP24:
    case llvm::ELF::R_ARM_CALL:
    case llvm::ELF::R_ARM_JUMP24:
    case llvm::ELF::R_ARM_PREL31:
      return WordPatch;
    default:
      return std::nullopt;
    }
  }
  if (TargetValue == Target::AArch64) {
    switch (Type) {
    case llvm::ELF::R_AARCH64_ABS64:
    case llvm::ELF::R_AARCH64_PREL64:
      return DoubleWordPatch;
    case llvm::ELF::R_AARCH64_PREL32:
    case llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21:
    case llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_JUMP26:
    case llvm::ELF::R_AARCH64_CALL26:
    case llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC:
      return WordPatch;
    default:
      return std::nullopt;
    }
  }
  if (TargetValue == Target::RISCV64) {
    switch (Type) {
    case llvm::ELF::R_RISCV_ADD8:
    case llvm::ELF::R_RISCV_SUB8:
    case llvm::ELF::R_RISCV_SET8:
      return BytePatch;
    case llvm::ELF::R_RISCV_64:
    case llvm::ELF::R_RISCV_CALL:
    case llvm::ELF::R_RISCV_CALL_PLT:
      return DoubleWordPatch;
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
    case llvm::ELF::R_RISCV_PCREL_LO12_S:
    case llvm::ELF::R_RISCV_32_PCREL:
    case llvm::ELF::R_RISCV_ADD32:
    case llvm::ELF::R_RISCV_SUB32:
      return WordPatch;
    case llvm::ELF::R_RISCV_RELAX:
      return NoPatch;
    default:
      return std::nullopt;
    }
  }
  if (TargetValue == Target::S390X) {
    switch (Type) {
    case llvm::ELF::R_390_64:
      return DoubleWordPatch;
    case llvm::ELF::R_390_PC32:
    case llvm::ELF::R_390_PC32DBL:
    case llvm::ELF::R_390_PLT32DBL:
      return WordPatch;
    default:
      return std::nullopt;
    }
  }
  if (MetadataSize == BytePatch || MetadataSize == HalfPatch ||
      MetadataSize == WordPatch || MetadataSize == DoubleWordPatch) {
    return MetadataSize;
  }
  return std::nullopt;
}

bool relocationIsPCRelative(ObjectFormat Format, Target TargetValue,
                            uint32_t Type) noexcept {
  if (Format == ObjectFormat::MachO) {
    if (TargetValue == Target::X86_64) {
      return Type == llvm::MachO::X86_64_RELOC_SIGNED ||
             Type == llvm::MachO::X86_64_RELOC_SIGNED_1 ||
             Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ||
             Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ||
             Type == llvm::MachO::X86_64_RELOC_BRANCH;
    }
    return TargetValue == Target::AArch64 &&
           (Type == llvm::MachO::ARM64_RELOC_BRANCH26 ||
            Type == llvm::MachO::ARM64_RELOC_PAGE21);
  }
  if (Format == ObjectFormat::COFF) {
    if (TargetValue == Target::X86_64) {
      return Type >= llvm::COFF::IMAGE_REL_AMD64_REL32 &&
             Type <= llvm::COFF::IMAGE_REL_AMD64_REL32_5;
    }
    return TargetValue == Target::AArch64 &&
           (Type == llvm::COFF::IMAGE_REL_ARM64_BRANCH26 ||
            Type == llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21);
  }
  switch (TargetValue) {
  case Target::X86_64:
    return Type == llvm::ELF::R_X86_64_PC32 ||
           Type == llvm::ELF::R_X86_64_PLT32 ||
           Type == llvm::ELF::R_X86_64_GOTPCRELX ||
           Type == llvm::ELF::R_X86_64_REX_GOTPCRELX;
  case Target::ARM:
    return Type == llvm::ELF::R_ARM_REL32 ||
           Type == llvm::ELF::R_ARM_THM_CALL ||
           Type == llvm::ELF::R_ARM_THM_JUMP24 ||
           Type == llvm::ELF::R_ARM_CALL || Type == llvm::ELF::R_ARM_JUMP24 ||
           Type == llvm::ELF::R_ARM_PREL31;
  case Target::AArch64:
    return Type == llvm::ELF::R_AARCH64_PREL64 ||
           Type == llvm::ELF::R_AARCH64_PREL32 ||
           Type == llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21 ||
           Type == llvm::ELF::R_AARCH64_JUMP26 ||
           Type == llvm::ELF::R_AARCH64_CALL26;
  case Target::RISCV64:
    return Type == llvm::ELF::R_RISCV_CALL ||
           Type == llvm::ELF::R_RISCV_CALL_PLT ||
           Type == llvm::ELF::R_RISCV_PCREL_HI20 ||
           Type == llvm::ELF::R_RISCV_PCREL_LO12_I ||
           Type == llvm::ELF::R_RISCV_PCREL_LO12_S ||
           Type == llvm::ELF::R_RISCV_32_PCREL;
  case Target::S390X:
    return Type == llvm::ELF::R_390_PC32 || Type == llvm::ELF::R_390_PC32DBL ||
           Type == llvm::ELF::R_390_PLT32DBL;
  }
  return false;
}

LinkGraph::LinkGraph(LinkGraph &&Other) noexcept
    : LinkGraph(Other.TargetValue, Other.EndianValue, Other.FormatValue) {
  swap(Other);
}

LinkGraph &LinkGraph::operator=(const LinkGraph &Other) {
  LinkGraph Copy(Other);
  swap(Copy);
  return *this;
}

LinkGraph &LinkGraph::operator=(LinkGraph &&Other) noexcept {
  LinkGraph Moved(std::move(Other));
  swap(Moved);
  return *this;
}

void LinkGraph::swap(LinkGraph &Other) noexcept {
  using std::swap;
  swap(TargetValue, Other.TargetValue);
  swap(EndianValue, Other.EndianValue);
  swap(FormatValue, Other.FormatValue);
  swap(ELFFlags, Other.ELFFlags);
  InputName.swap(Other.InputName);
  Sections.swap(Other.Sections);
  Symbols.swap(Other.Symbols);
  SymbolNames.swap(Other.SymbolNames);
  Relocations.swap(Other.Relocations);
  Rebases.swap(Other.Rebases);
  RelocationIntervals.swap(Other.RelocationIntervals);
  RebaseIntervals.swap(Other.RebaseIntervals);
  EHFrameReferences.swap(Other.EHFrameReferences);
  CompactUnwind.swap(Other.CompactUnwind);
  MachOBuildVersions.swap(Other.MachOBuildVersions);
  swap(UnwindInfoState, Other.UnwindInfoState);
  swap(RelocationsApplied, Other.RelocationsApplied);
}

LinkExpect<void> LinkGraph::checkNotRelocated() const {
  if (RelocationsApplied)
    return diagnosticError("link graph relocations already applied");
  return {};
}

LinkExpect<void> LinkGraph::checkMutable() const {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return diagnosticError("Mach-O unwind info is populated");
  return checkNotRelocated();
}

LinkExpect<void> LinkGraph::checkSection(const Section &Value,
                                         std::optional<SectionId> Id) const {
  if (!llvm::isPowerOf2_64(Value.Alignment))
    return diagnosticError(sectionDiag(
        "section alignment must be a non-zero power of two", Value, Id));
  if (Value.Content.size() > Value.VirtualSize)
    return diagnosticError(
        sectionDiag("section content exceeds section virtual size", Value, Id));
  if (Value.LinkedSection && *Value.LinkedSection >= Sections.size())
    return diagnosticError(sectionDiag("invalid linked section ID", Value, Id));
  return {};
}

LinkExpect<void> LinkGraph::checkSymbol(const Symbol &Value,
                                        std::optional<SymbolId> Id) const {
  if (Value.Section >= Sections.size())
    return diagnosticError(symbolDiag("invalid section ID", Value, Id));
  if (extendsBeyond(Value.Offset, Value.Size,
                    Sections[Value.Section].VirtualSize))
    return diagnosticError(
        symbolDiag("symbol extends beyond section virtual size", Value, Id));
  return {};
}

LinkExpect<void> LinkGraph::checkRelocation(const Relocation &Value) const {
  if (Value.Symbol >= Symbols.size())
    return diagnosticError(relocationDiag("invalid symbol ID", Value));
  const auto Canonical = relocationPatchSize(Value.Format, TargetValue,
                                             Value.Type, Value.PatchSize);
  if (!Canonical)
    return diagnosticError(
        relocationDiag("unsupported relocation patch size", Value));
  if (*Canonical != Value.PatchSize)
    return diagnosticError(
        relocationDiag("invalid relocation patch size", Value));
  if (extendsBeyond(Value.Offset, Value.PatchSize,
                    Sections[Value.Section].Content.size()))
    return diagnosticError(
        relocationDiag("relocation offset is outside section content", Value));
  return {};
}

LinkExpect<void> LinkGraph::checkRebase(const Rebase &Value) const {
  if (Value.Section >= Sections.size())
    return diagnosticError(rebaseDiag("invalid section ID", Value));
  if (extendsBeyond(Value.Offset, rebaseWidth(Value),
                    Sections[Value.Section].Content.size()))
    return diagnosticError(
        rebaseDiag("rebase offset is outside section content", Value));
  return {};
}

LinkExpect<void> LinkGraph::beginInput(std::string_view Name) {
  EXPECTED_TRY(checkNotRelocated());
  if (InputName) {
    return diagnosticError("link graph accepts exactly one input object");
  }
  InputName = std::string(Name);
  return {};
}

LinkExpect<SectionId> LinkGraph::addSection(Section Value) {
  EXPECTED_TRY(checkMutable());
  EXPECTED_TRY(checkSection(Value, std::nullopt));
  if (Sections.size() >= InvalidSectionId) {
    return diagnosticError("too many sections");
  }
  const SectionId Id{static_cast<uint32_t>(Sections.size())};
  Sections.push_back(std::move(Value));
  return Id;
}

LinkExpect<SymbolId> LinkGraph::addSymbol(Symbol Value) {
  EXPECTED_TRY(checkMutable());
  if (Value.Section == InvalidSectionId) {
    Diagnostic Diag{"undefined symbol"};
    Diag.SymbolName = Value.Name;
    Diag.Offset = Value.Offset;
    return diagnosticError(std::move(Diag));
  }
  EXPECTED_TRY(checkSymbol(Value, std::nullopt));
  if (findSymbol(Value.Name)) {
    return diagnosticError(
        symbolDiag("duplicate symbol definition", Value, std::nullopt));
  }
  if (Symbols.size() >= InvalidSymbolId) {
    return diagnosticError("too many symbols");
  }
  const SymbolId Id{static_cast<uint32_t>(Symbols.size())};
  auto Inserted = SymbolNames.emplace(Value.Name, Id);
  if (!Inserted.second)
    return diagnosticError("duplicate symbol definition");
  try {
    Symbols.push_back(std::move(Value));
  } catch (...) {
    SymbolNames.erase(Inserted.first);
    throw;
  }
  return Id;
}

std::optional<SymbolId> LinkGraph::findSymbol(const std::string &Name) const {
  const auto Found = SymbolNames.find(Name);
  if (Found == SymbolNames.end())
    return std::nullopt;
  return Found->second;
}

LinkExpect<void> LinkGraph::addRelocation(Relocation Value) {
  EXPECTED_TRY(checkMutable());
  if (Value.Section >= Sections.size()) {
    return diagnosticError(patchDiag("invalid section ID", Value.Section,
                                     Value.Type, Value.Offset));
  }
  EXPECTED_TRY(checkRelocation(Value));
  if (Value.PatchSize != NoPatch) {
    const auto Conflicts = [&](const auto &Entry) {
      for (size_t I = 0; I < Entry.second.Count; ++I) {
        const auto &Old = Relocations[Entry.second.Indices[I]];
        if (overlaps(Value, Old,
                     [](const auto &Record) { return Record.PatchSize; }) &&
            !composeRISCVSymbolDifference(Value, Old))
          return true;
      }
      return false;
    };
    const auto Existing =
        RelocationIntervals.find(std::make_pair(Value.Section, Value.Offset));
    if (hasNeighborConflict(RelocationIntervals, Value.Section, Value.Offset,
                            Value.PatchSize, Conflicts) ||
        (Existing != RelocationIntervals.end() &&
         Existing->second.Count == Existing->second.Indices.size())) {
      return diagnosticError(
          relocationDiag("overlapping relocation patches", Value));
    }
  }
  Relocations.push_back(Value);
  if (Value.PatchSize != NoPatch) {
    try {
      auto It = RelocationIntervals
                    .try_emplace(std::make_pair(Value.Section, Value.Offset))
                    .first;
      It->second.Indices[It->second.Count++] = Relocations.size() - 1;
    } catch (...) {
      Relocations.pop_back();
      throw;
    }
  }
  return {};
}

LinkExpect<void> LinkGraph::addRebase(Rebase Value) {
  EXPECTED_TRY(checkMutable());
  EXPECTED_TRY(checkRebase(Value));
  const auto Conflicts = [&](const auto &Entry) {
    return overlaps(Value, Rebases[Entry.second],
                    [](const auto &Record) { return rebaseWidth(Record); });
  };
  if (hasNeighborConflict(RebaseIntervals, Value.Section, Value.Offset,
                          rebaseWidth(Value), Conflicts)) {
    return diagnosticError(rebaseDiag("overlapping rebase patches", Value));
  }
  Rebases.push_back(Value);
  try {
    RebaseIntervals.emplace(std::make_pair(Value.Section, Value.Offset),
                            Rebases.size() - 1);
  } catch (...) {
    Rebases.pop_back();
    throw;
  }
  return {};
}

LinkExpect<void>
LinkGraph::buildPatchIntervals(Span<const Relocation> RelocationValues,
                               Span<const Rebase> RebaseValues,
                               RelocationIntervalMap &NewRelocationIntervals,
                               RebaseIntervalMap &NewRebaseIntervals) {
  for (size_t I = 0; I < RelocationValues.size(); ++I) {
    const auto &Value = RelocationValues[I];
    if (Value.PatchSize != NoPatch) {
      auto &Group = NewRelocationIntervals[{Value.Section, Value.Offset}];
      if (Group.Count == Group.Indices.size())
        return diagnosticError("overlapping relocation patches");
      Group.Indices[Group.Count++] = I;
    }
  }
  for (size_t I = 0; I < RebaseValues.size(); ++I) {
    const auto &Value = RebaseValues[I];
    if (!NewRebaseIntervals
             .emplace(std::make_pair(Value.Section, Value.Offset), I)
             .second)
      return diagnosticError("overlapping rebase patches");
  }
  return {};
}

LinkExpect<void> LinkGraph::rebuildPatchIntervals() {
  RelocationIntervalMap NewRelocationIntervals;
  RebaseIntervalMap NewRebaseIntervals;
  EXPECTED_TRY(buildPatchIntervals(Relocations, Rebases, NewRelocationIntervals,
                                   NewRebaseIntervals));
  RelocationIntervals.swap(NewRelocationIntervals);
  RebaseIntervals.swap(NewRebaseIntervals);
  return {};
}

LinkExpect<void> LinkGraph::addEHFrameReference(EHFrameReference Value) {
  EXPECTED_TRY(checkMutable());
  if (!isValidEHFrameReference(
          Value, Sections, Symbols.size(),
          [&](SectionId Id) { return Sections[Id].Content.size(); }))
    return diagnosticError("invalid EH frame reference");
  EHFrameReferences.push_back(Value);
  return {};
}

LinkExpect<void> LinkGraph::commitNormalizedEHFrame(
    std::vector<std::pair<SectionId, std::vector<Byte>>> Content,
    Span<const uint8_t> RemoveRelocations) {
  EXPECTED_TRY(checkNotRelocated());
  if (RemoveRelocations.size() != Relocations.size())
    return diagnosticError(
        "EH frame relocation mask size does not match relocations");

  std::vector<uint8_t> Seen(Sections.size());
  for (const auto &[Id, Bytes] : Content) {
    if (Id >= Sections.size()) {
      return diagnosticError(
          sectionIdDiag("invalid normalized EH frame section ID", Id));
    }
    const auto &TargetSection = Sections[Id];
    if (TargetSection.Purpose != SectionPurpose::EHFrame) {
      return diagnosticError(
          sectionDiag("normalized content targets a non-EH-frame section",
                      TargetSection, Id));
    }
    if (Seen[Id]) {
      return diagnosticError(sectionDiag(
          "duplicate normalized EH frame section", TargetSection, Id));
    }
    Seen[Id] = true;
    if (Bytes.size() > TargetSection.VirtualSize) {
      Diagnostic Diag =
          sectionDiag("normalized EH frame content exceeds virtual size",
                      TargetSection, Id);
      Diag.Offset = Bytes.size();
      return diagnosticError(std::move(Diag));
    }
  }

  const auto ContentSize = [&](SectionId Id) {
    const auto Found = std::find_if(Content.begin(), Content.end(),
                                    [&](auto &V) { return V.first == Id; });
    return Found == Content.end() ? Sections[Id].Content.size()
                                  : Found->second.size();
  };
  std::vector<Relocation> NewRelocations;
  NewRelocations.reserve(Relocations.size());
  for (size_t I = 0; I < Relocations.size(); ++I) {
    if (RemoveRelocations[I]) {
      const auto Section = Relocations[I].Section;
      if (Section >= Sections.size() ||
          Sections[Section].Purpose != SectionPurpose::EHFrame) {
        return diagnosticError(relocationDiag(
            "cannot remove a non-EH-frame relocation", Relocations[I]));
      }
      continue;
    }
    NewRelocations.push_back(Relocations[I]);
  }
  RelocationIntervalMap NewIntervals;
  for (size_t I = 0; I < NewRelocations.size(); ++I) {
    const auto &Value = NewRelocations[I];
    const auto Canonical = relocationPatchSize(Value.Format, TargetValue,
                                               Value.Type, Value.PatchSize);
    if (Value.Section >= Sections.size() || Value.Symbol >= Symbols.size() ||
        !Canonical || *Canonical != Value.PatchSize ||
        extendsBeyond(Value.Offset, Value.PatchSize,
                      ContentSize(Value.Section))) {
      return diagnosticError(relocationDiag(
          "normalized EH frame invalidates a relocation", Value));
    }
    if (Value.PatchSize == NoPatch)
      continue;
    auto &Group = NewIntervals[{Value.Section, Value.Offset}];
    if (Group.Count == Group.Indices.size() ||
        (Group.Count == 1 && !composeRISCVSymbolDifference(
                                 Value, NewRelocations[Group.Indices[0]])))
      return diagnosticError("overlapping relocation patches");
    Group.Indices[Group.Count++] = I;
  }
  std::optional<SectionId> PreviousSection;
  uint64_t PreviousEnd = 0;
  for (const auto &[Key, Group] : NewIntervals) {
    const auto &Value = NewRelocations[Group.Indices[0]];
    if (PreviousSection && *PreviousSection == Key.first &&
        Key.second < PreviousEnd)
      return diagnosticError("overlapping relocation patches");
    PreviousSection = Key.first;
    PreviousEnd = Key.second + Value.PatchSize;
  }
  for (const auto &Value : Rebases) {
    if (Value.Section >= Sections.size() ||
        extendsBeyond(Value.Offset, rebaseWidth(Value),
                      ContentSize(Value.Section))) {
      return diagnosticError(
          rebaseDiag("normalized EH frame invalidates a rebase", Value));
    }
  }
  for (const auto &Value : EHFrameReferences) {
    if (!isValidEHFrameReference(Value, Sections, Symbols.size(),
                                 ContentSize)) {
      Diagnostic Diag{"normalized EH frame invalidates an EH frame reference"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.Offset = Value.Offset;
      return diagnosticError(std::move(Diag));
    }
  }

  for (auto &[Id, Bytes] : Content)
    Sections[Id].Content.swap(Bytes);
  Relocations.swap(NewRelocations);
  RelocationIntervals.swap(NewIntervals);
  return {};
}

LinkExpect<void> LinkGraph::addCompactUnwind(CompactUnwindRecord Value) {
  EXPECTED_TRY(checkMutable());
  if (FormatValue != ObjectFormat::MachO) {
    return diagnosticError("compact unwind requires a Mach-O link graph");
  }
  EXPECTED_TRY(const uint64_t Address,
               validateCompactUnwindRecord(Value, Sections, Symbols));
  if (!CompactUnwind.empty()) {
    EXPECTED_TRY(const uint64_t PreviousAddress,
                 compactUnwindAddress(CompactUnwind.back(), Sections, Symbols));
    EXPECTED_TRY(validateCompactUnwindOrder(CompactUnwind.back(),
                                            PreviousAddress, Address));
  }
  CompactUnwind.push_back(std::move(Value));
  return {};
}

LinkExpect<void> LinkGraph::addMachOBuildVersion(MachOBuildVersion Value) {
  EXPECTED_TRY(checkNotRelocated());
  if (FormatValue != ObjectFormat::MachO) {
    return diagnosticError("Mach-O build version requires a Mach-O link graph");
  }
  MachOBuildVersions.push_back(std::move(Value));
  return {};
}

LinkExpect<void> LinkGraph::pruneUnreferencedMachOEHFrame() {
  EXPECTED_TRY(checkMutable());
  if (FormatValue != ObjectFormat::MachO || CompactUnwind.empty() ||
      !EHFrameReferences.empty() ||
      std::any_of(Relocations.begin(), Relocations.end(),
                  [&](const auto &Value) {
                    return Value.Section < Sections.size() &&
                           Sections[Value.Section].Purpose ==
                               SectionPurpose::EHFrame;
                  }) ||
      std::any_of(CompactUnwind.begin(), CompactUnwind.end(),
                  [](const auto &Record) { return Record.FDE.has_value(); }))
    return {};

  std::vector<SectionId> SectionMap(Sections.size(), InvalidSectionId);
  std::vector<Section> NewSections;
  NewSections.reserve(Sections.size());
  for (SectionId I = 0; I < Sections.size(); ++I) {
    if (Sections[I].Purpose == SectionPurpose::EHFrame)
      continue;
    SectionMap[I] = static_cast<SectionId>(NewSections.size());
    NewSections.push_back(Sections[I]);
  }
  if (NewSections.size() == Sections.size())
    return {};
  for (auto &Section : NewSections) {
    if (!Section.LinkedSection)
      continue;
    if (SectionMap[*Section.LinkedSection] == InvalidSectionId)
      return diagnosticError("section links to pruned EH frame");
    Section.LinkedSection = SectionMap[*Section.LinkedSection];
  }

  std::vector<SymbolId> SymbolMap(Symbols.size(), InvalidSymbolId);
  std::vector<Symbol> NewSymbols;
  NewSymbols.reserve(Symbols.size());
  for (SymbolId I = 0; I < Symbols.size(); ++I) {
    if (SectionMap[Symbols[I].Section] == InvalidSectionId)
      continue;
    SymbolMap[I] = static_cast<SymbolId>(NewSymbols.size());
    NewSymbols.push_back(Symbols[I]);
    NewSymbols.back().Section = SectionMap[Symbols[I].Section];
  }

  std::vector<Relocation> NewRelocations;
  NewRelocations.reserve(Relocations.size());
  for (const auto &RelocationValue : Relocations) {
    if (SectionMap[RelocationValue.Section] == InvalidSectionId)
      continue;
    if (SymbolMap[RelocationValue.Symbol] == InvalidSymbolId)
      return diagnosticError("relocation targets pruned EH frame");
    NewRelocations.push_back(RelocationValue);
    NewRelocations.back().Section = SectionMap[RelocationValue.Section];
    NewRelocations.back().Symbol = SymbolMap[RelocationValue.Symbol];
  }

  std::vector<Rebase> NewRebases;
  NewRebases.reserve(Rebases.size());
  for (const auto &RebaseValue : Rebases) {
    if (SectionMap[RebaseValue.Section] == InvalidSectionId)
      continue;
    NewRebases.push_back(RebaseValue);
    NewRebases.back().Section = SectionMap[RebaseValue.Section];
  }

  std::vector<CompactUnwindRecord> NewCompactUnwind = CompactUnwind;
  for (auto &Record : NewCompactUnwind) {
    if (SymbolMap[Record.Function] == InvalidSymbolId ||
        (Record.Personality &&
         SymbolMap[*Record.Personality] == InvalidSymbolId) ||
        (Record.LSDA && SymbolMap[*Record.LSDA] == InvalidSymbolId))
      return diagnosticError("compact unwind targets pruned EH frame");
    Record.Function = SymbolMap[Record.Function];
    if (Record.Personality)
      Record.Personality = SymbolMap[*Record.Personality];
    if (Record.LSDA)
      Record.LSDA = SymbolMap[*Record.LSDA];
  }

  LinkGraph Candidate(TargetValue, EndianValue, FormatValue);
  Candidate.ELFFlags = ELFFlags;
  Candidate.InputName = InputName;
  Candidate.Sections = std::move(NewSections);
  Candidate.Symbols = std::move(NewSymbols);
  for (SymbolId I = 0; I < Candidate.Symbols.size(); ++I)
    Candidate.SymbolNames.emplace(Candidate.Symbols[I].Name, I);
  Candidate.Relocations = std::move(NewRelocations);
  Candidate.Rebases = std::move(NewRebases);
  EXPECTED_TRY(Candidate.rebuildPatchIntervals());
  Candidate.CompactUnwind = std::move(NewCompactUnwind);
  Candidate.UnwindInfoState = UnwindInfoState;
  if (!Candidate.validate())
    return diagnosticError("invalid graph after pruning EH frame");
  Sections.swap(Candidate.Sections);
  Symbols.swap(Candidate.Symbols);
  SymbolNames.swap(Candidate.SymbolNames);
  Relocations.swap(Candidate.Relocations);
  Rebases.swap(Candidate.Rebases);
  RelocationIntervals.swap(Candidate.RelocationIntervals);
  RebaseIntervals.swap(Candidate.RebaseIntervals);
  EHFrameReferences.clear();
  CompactUnwind.swap(Candidate.CompactUnwind);
  return {};
}

LinkExpect<void> LinkGraph::reserveMachOUnwindInfoSection(Section Value) {
  if (RelocationsApplied || UnwindInfoState != MachOUnwindInfoState::None ||
      FormatValue != ObjectFormat::MachO || CompactUnwind.empty() ||
      !isUnwindInfoSection(Value) || Value.Content.size() != Value.VirtualSize)
    return diagnosticError("invalid Mach-O unwind info reservation");
  if (Sections.size() >= InvalidSectionId)
    return diagnosticError("too many sections");
  static_assert(std::is_nothrow_move_constructible_v<Section>);
  Sections.push_back(std::move(Value));
  UnwindInfoState = MachOUnwindInfoState::Reserved;
  return {};
}

LinkExpect<void>
LinkGraph::populateMachOUnwindInfoSection(SectionId Id,
                                          std::vector<Byte> Content) {
  if (!RelocationsApplied ||
      UnwindInfoState != MachOUnwindInfoState::Reserved ||
      Id >= Sections.size() || !isUnwindInfoSection(Sections[Id]) ||
      Content.size() != Sections[Id].VirtualSize)
    return diagnosticError("invalid Mach-O unwind info population");
  Sections[Id].Content.swap(Content);
  UnwindInfoState = MachOUnwindInfoState::Populated;
  return {};
}

LinkExpect<void>
LinkGraph::addSynthesizedEHFrame(std::optional<SectionId> Existing,
                                 Section Value,
                                 std::vector<EHFrameReference> References) {
  if (RelocationsApplied ||
      UnwindInfoState == MachOUnwindInfoState::Populated ||
      Value.Purpose != SectionPurpose::EHFrame ||
      Value.Kind != SectionKind::Unwind ||
      Value.Content.size() != Value.VirtualSize)
    return diagnosticError("invalid synthesized EH frame");
  if (Existing && (*Existing >= Sections.size() ||
                   Sections[*Existing].Purpose != SectionPurpose::EHFrame))
    return diagnosticError("invalid synthesized EH frame section");
  if (!Existing && Sections.size() >= InvalidSectionId)
    return diagnosticError("too many sections");
  const SectionId Id =
      Existing ? *Existing : static_cast<SectionId>(Sections.size());
  for (auto &Reference : References) {
    if (Reference.Section != InvalidSectionId ||
        Reference.Symbol >= Symbols.size() ||
        extendsBeyond(Reference.Offset, EHFrameReferenceSize,
                      Value.Content.size()))
      return diagnosticError("invalid synthesized EH frame reference");
    Reference.Section = Id;
  }
  static_assert(std::is_nothrow_move_assignable_v<Section>);
  static_assert(std::is_nothrow_move_constructible_v<Section>);
  EHFrameReferences.reserve(EHFrameReferences.size() + References.size());
  if (Existing)
    Sections[*Existing] = std::move(Value);
  else
    Sections.push_back(std::move(Value));
  EHFrameReferences.insert(EHFrameReferences.end(), References.begin(),
                           References.end());
  return {};
}

LinkExpect<void> LinkGraph::validateSections() const {
  for (SectionId I = 0; I < Sections.size(); ++I)
    EXPECTED_TRY(checkSection(Sections[I], I));
  return {};
}

LinkExpect<void> LinkGraph::validateUnwindInfoState() const {
  const auto IsUnwindInfo = [](const auto &Section) {
    return Section.Purpose == SectionPurpose::UnwindInfo;
  };
  const auto UnwindInfoCount =
      std::count_if(Sections.begin(), Sections.end(), IsUnwindInfo);
  if ((UnwindInfoState == MachOUnwindInfoState::None && UnwindInfoCount != 0) ||
      (UnwindInfoState != MachOUnwindInfoState::None &&
       (FormatValue != ObjectFormat::MachO || CompactUnwind.empty() ||
        UnwindInfoCount != 1)))
    return diagnosticError("incoherent Mach-O unwind info state");
  if (UnwindInfoState != MachOUnwindInfoState::None) {
    const auto &UnwindInfo =
        *std::find_if(Sections.begin(), Sections.end(), IsUnwindInfo);
    if (!isUnwindInfoSection(UnwindInfo) ||
        UnwindInfo.Content.size() != UnwindInfo.VirtualSize)
      return diagnosticError("incoherent Mach-O unwind info section");
  }
  return {};
}

LinkExpect<void> LinkGraph::validateSymbols() const {
  for (size_t I = 0; I < Symbols.size(); ++I)
    EXPECTED_TRY(checkSymbol(Symbols[I], static_cast<SymbolId>(I)));
  return {};
}

LinkExpect<void> LinkGraph::validateRelocations() const {
  for (const auto &Value : Relocations) {
    if (Value.Section >= Sections.size())
      return diagnosticError(relocationDiag("invalid section ID", Value));
    EXPECTED_TRY(checkRelocation(Value));
  }
  std::vector<size_t> RelocationOrder;
  RelocationOrder.reserve(Relocations.size());
  for (size_t I = 0; I < Relocations.size(); ++I) {
    if (Relocations[I].PatchSize != NoPatch)
      RelocationOrder.push_back(I);
  }
  std::sort(RelocationOrder.begin(), RelocationOrder.end(),
            [&](size_t Left, size_t Right) {
              const auto &L = Relocations[Left];
              const auto &R = Relocations[Right];
              return std::tie(L.Section, L.Offset, Left) <
                     std::tie(R.Section, R.Offset, Right);
            });
  std::optional<SectionId> RelocationSection;
  uint64_t RelocationEnd = 0;
  bool HasUnpairedRISCVDifference = false;
  for (size_t Begin = 0; Begin < RelocationOrder.size();) {
    const auto &First = Relocations[RelocationOrder[Begin]];
    size_t End = Begin + 1;
    while (End < RelocationOrder.size()) {
      const auto &Next = Relocations[RelocationOrder[End]];
      if (Next.Section != First.Section || Next.Offset != First.Offset)
        break;
      ++End;
    }
    const size_t Count = End - Begin;
    if (Count > 2 ||
        (Count == 2 && !composeRISCVSymbolDifference(
                           First, Relocations[RelocationOrder[Begin + 1]])))
      return diagnosticError("overlapping relocation patches");
    if (Count == 1 && isRISCVSymbolDifference(First))
      HasUnpairedRISCVDifference = true;
    if (RelocationSection && *RelocationSection == First.Section &&
        First.Offset < RelocationEnd)
      return diagnosticError("overlapping relocation patches");
    if (First.PatchSize > UINT64_MAX - First.Offset)
      return diagnosticError("relocation offset is outside section content");
    RelocationSection = First.Section;
    RelocationEnd = First.Offset + First.PatchSize;
    Begin = End;
  }
  if (HasUnpairedRISCVDifference)
    return diagnosticError("unpaired RISC-V symbol difference");
  return {};
}

LinkExpect<void> LinkGraph::validateRebases() const {
  for (const auto &Value : Rebases)
    EXPECTED_TRY(checkRebase(Value));
  std::vector<size_t> RebaseOrder(Rebases.size());
  for (size_t I = 0; I < Rebases.size(); ++I)
    RebaseOrder[I] = I;
  std::sort(RebaseOrder.begin(), RebaseOrder.end(),
            [&](size_t Left, size_t Right) {
              const auto &L = Rebases[Left];
              const auto &R = Rebases[Right];
              return std::tie(L.Section, L.Offset, Left) <
                     std::tie(R.Section, R.Offset, Right);
            });
  std::optional<SectionId> RebaseSection;
  uint64_t RebaseEnd = 0;
  for (const size_t I : RebaseOrder) {
    const auto &Value = Rebases[I];
    if (RebaseSection && *RebaseSection == Value.Section &&
        Value.Offset < RebaseEnd)
      return diagnosticError("overlapping rebase patches");
    const uint8_t Width = rebaseWidth(Value);
    if (Width > UINT64_MAX - Value.Offset)
      return diagnosticError("rebase offset is outside section content");
    RebaseSection = Value.Section;
    RebaseEnd = Value.Offset + Width;
  }
  return {};
}

LinkExpect<void> LinkGraph::validateCompactUnwind() const {
  if (!CompactUnwind.empty() && FormatValue != ObjectFormat::MachO) {
    return diagnosticError("compact unwind requires a Mach-O link graph");
  }
  std::optional<uint64_t> PreviousAddress;
  for (size_t I = 0; I < CompactUnwind.size(); ++I) {
    EXPECTED_TRY(
        const uint64_t Address,
        validateCompactUnwindRecord(CompactUnwind[I], Sections, Symbols));
    if (PreviousAddress) {
      EXPECTED_TRY(validateCompactUnwindOrder(CompactUnwind[I - 1],
                                              *PreviousAddress, Address));
    }
    PreviousAddress = Address;
  }
  return {};
}

LinkExpect<void> LinkGraph::validate() const {
  if (!InputName) {
    return diagnosticError("link graph requires one input object");
  }
  EXPECTED_TRY(validateSections());
  EXPECTED_TRY(validateUnwindInfoState());
  EXPECTED_TRY(validateSymbols());
  EXPECTED_TRY(validateRelocations());
  EXPECTED_TRY(validateRebases());
  EXPECTED_TRY(validateCompactUnwind());
  return {};
}

LinkExpect<void> LinkGraph::setSectionAddress(SectionId Id, uint64_t Address) {
  EXPECTED_TRY(checkMutable());
  if (Id >= Sections.size()) {
    return diagnosticError(sectionIdDiag("invalid section ID", Id));
  }
  Sections[Id].Address = Address;
  return {};
}

LinkExpect<void> LinkGraph::setSectionFileOffset(SectionId Id,
                                                 uint64_t FileOffset) {
  EXPECTED_TRY(checkMutable());
  if (Id >= Sections.size()) {
    return diagnosticError(sectionIdDiag("invalid section ID", Id));
  }
  Sections[Id].FileOffset = FileOffset;
  return {};
}

LinkExpect<void> LinkGraph::setELFFlags(uint32_t Flags) {
  EXPECTED_TRY(checkNotRelocated());
  if (FormatValue != ObjectFormat::ELF) {
    return diagnosticError("ELF flags require an ELF link graph");
  }
  ELFFlags = Flags;
  return {};
}

LinkExpect<void> LinkGraph::setLinkedSection(SectionId Id, SectionId Linked) {
  EXPECTED_TRY(checkMutable());
  if (Id >= Sections.size() || Linked >= Sections.size()) {
    return diagnosticError("invalid linked section ID");
  }
  Sections[Id].LinkedSection = Linked;
  return {};
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
