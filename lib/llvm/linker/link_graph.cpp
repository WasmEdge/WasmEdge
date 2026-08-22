// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/link_graph.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>

#include <algorithm>
#include <tuple>
#include <utility>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

template <typename T> LinkExpect<T> fail(Diagnostic Value) {
  return Unexpected<Diagnostic>(std::move(Value));
}

bool extendsBeyond(uint64_t Offset, uint64_t Size, uint64_t Limit) noexcept {
  return Offset > Limit || Size > Limit - Offset;
}

bool isRISCVSymbolDifferenceAdd(const Relocation &Value) noexcept {
  return (Value.PatchSize == 1 && (Value.Type == llvm::ELF::R_RISCV_ADD8 ||
                                   Value.Type == llvm::ELF::R_RISCV_SET8)) ||
         (Value.PatchSize == 4 && Value.Type == llvm::ELF::R_RISCV_ADD32);
}

bool isRISCVSymbolDifferenceSub(const Relocation &Value) noexcept {
  return (Value.PatchSize == 1 && Value.Type == llvm::ELF::R_RISCV_SUB8) ||
         (Value.PatchSize == 4 && Value.Type == llvm::ELF::R_RISCV_SUB32);
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

LinkExpect<void> relocated() {
  return fail<void>(Diagnostic{"link graph relocations already applied"});
}

LinkExpect<uint64_t> compactUnwindAddress(const CompactUnwindRecord &Value,
                                          const std::vector<Section> &Sections,
                                          const std::vector<Symbol> &Symbols) {
  if (Value.Function >= Symbols.size()) {
    Diagnostic Diag{"invalid compact unwind function symbol ID"};
    Diag.Symbol = Value.Function;
    return fail<uint64_t>(std::move(Diag));
  }
  const auto &Function = Symbols[Value.Function];
  if (Function.Section >= Sections.size()) {
    Diagnostic Diag{"invalid compact unwind function section ID"};
    Diag.Symbol = Value.Function;
    Diag.Section = Function.Section;
    return fail<uint64_t>(std::move(Diag));
  }
  const auto &Section = Sections[Function.Section];
  if (Section.Kind != SectionKind::Text) {
    Diagnostic Diag{"compact unwind function must reference text section"};
    Diag.Symbol = Value.Function;
    Diag.Section = Function.Section;
    return fail<uint64_t>(std::move(Diag));
  }
  if (Value.Length == 0) {
    Diagnostic Diag{"compact unwind function length must be non-zero"};
    Diag.Symbol = Value.Function;
    return fail<uint64_t>(std::move(Diag));
  }
  if (extendsBeyond(Function.Offset, Value.Length, Section.VirtualSize)) {
    Diagnostic Diag{"compact unwind function range exceeds text section"};
    Diag.Symbol = Value.Function;
    Diag.Section = Function.Section;
    Diag.Offset = Function.Offset;
    return fail<uint64_t>(std::move(Diag));
  }
  if (Function.Offset > UINT64_MAX - Section.InputAddress) {
    Diagnostic Diag{"compact unwind function address overflows"};
    Diag.Symbol = Value.Function;
    return fail<uint64_t>(std::move(Diag));
  }
  const uint64_t Address = Section.InputAddress + Function.Offset;
  if (Value.Length > UINT64_MAX - Address) {
    Diagnostic Diag{"compact unwind function address overflows"};
    Diag.Symbol = Value.Function;
    return fail<uint64_t>(std::move(Diag));
  }
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
    return fail<void>(std::move(Diag));
  }
  const auto &Symbol = Symbols[*Id];
  if (Symbol.Section >= Sections.size()) {
    Diagnostic Diag{"invalid compact unwind " + std::string(Name) +
                    " section ID"};
    Diag.Symbol = *Id;
    Diag.Section = Symbol.Section;
    Diag.Offset = Symbol.Offset;
    Diag.SymbolName = Symbol.Name;
    return fail<void>(std::move(Diag));
  }
  const auto &Section = Sections[Symbol.Section];
  const auto FailForSymbol = [&](std::string Message) {
    Diagnostic Diag{std::move(Message)};
    Diag.Symbol = *Id;
    Diag.Section = Symbol.Section;
    Diag.Offset = Symbol.Offset;
    Diag.SymbolName = Symbol.Name;
    Diag.SectionName = Section.Name;
    return fail<void>(std::move(Diag));
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
  auto Address = compactUnwindAddress(Value, Sections, Symbols);
  if (!Address)
    return Unexpected<Diagnostic>(std::move(Address.error()));
  auto Personality = validateCompactUnwindSymbol(
      Value.Personality, "personality", Sections, Symbols, false);
  if (!Personality)
    return fail<uint64_t>(std::move(Personality.error()));
  auto LSDA =
      validateCompactUnwindSymbol(Value.LSDA, "LSDA", Sections, Symbols, false);
  if (!LSDA)
    return fail<uint64_t>(std::move(LSDA.error()));
  auto FDE =
      validateCompactUnwindSymbol(Value.FDE, "FDE", Sections, Symbols, true);
  if (!FDE)
    return fail<uint64_t>(std::move(FDE.error()));
  return *Address;
}

LinkExpect<void> validateCompactUnwindOrder(const CompactUnwindRecord &Previous,
                                            uint64_t PreviousAddress,
                                            uint64_t Address) {
  if (Address < PreviousAddress)
    return fail<void>(Diagnostic{"compact unwind records are unordered"});
  if (Address == PreviousAddress) {
    return fail<void>(Diagnostic{"duplicate compact unwind function address"});
  }
  if (Address - PreviousAddress < Previous.Length) {
    return fail<void>(Diagnostic{"overlapping compact unwind function ranges"});
  }
  return {};
}

} // namespace

std::optional<uint8_t> relocationPatchSize(ObjectFormat Format,
                                           Target TargetValue, uint32_t Type,
                                           uint8_t MetadataSize) noexcept {
  constexpr uint8_t BytePatch = 1;
  constexpr uint8_t HalfPatch = 2;
  constexpr uint8_t WordPatch = 4;
  constexpr uint8_t DoubleWordPatch = 8;
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
           Type == llvm::ELF::R_ARM_THM_CALL || Type == llvm::ELF::R_ARM_CALL ||
           Type == llvm::ELF::R_ARM_JUMP24 || Type == llvm::ELF::R_ARM_PREL31;
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
    : TargetValue(Other.TargetValue), EndianValue(Other.EndianValue),
      FormatValue(Other.FormatValue), ELFFlags(Other.ELFFlags),
      InputName(std::move(Other.InputName)),
      Sections(std::move(Other.Sections)), Symbols(std::move(Other.Symbols)),
      SymbolNames(std::move(Other.SymbolNames)),
      Relocations(std::move(Other.Relocations)),
      Rebases(std::move(Other.Rebases)),
      RelocationIntervals(std::move(Other.RelocationIntervals)),
      RebaseIntervals(std::move(Other.RebaseIntervals)),
      EHFrameReferences(std::move(Other.EHFrameReferences)),
      CompactUnwind(std::move(Other.CompactUnwind)),
      MachOBuildVersions(std::move(Other.MachOBuildVersions)),
      UnwindInfoState(Other.UnwindInfoState),
      RelocationsApplied(Other.RelocationsApplied) {
  Other.InputName.reset();
  Other.Sections.clear();
  Other.Symbols.clear();
  Other.SymbolNames.clear();
  Other.Relocations.clear();
  Other.Rebases.clear();
  Other.RelocationIntervals.clear();
  Other.RebaseIntervals.clear();
  Other.EHFrameReferences.clear();
  Other.CompactUnwind.clear();
  Other.MachOBuildVersions.clear();
  Other.ELFFlags = 0;
  Other.UnwindInfoState = MachOUnwindInfoState::None;
  Other.RelocationsApplied = false;
}

LinkGraph &LinkGraph::operator=(const LinkGraph &Other) {
  LinkGraph Copy(Other);
  swap(Copy);
  return *this;
}

LinkGraph &LinkGraph::operator=(LinkGraph &&Other) noexcept {
  if (this == &Other)
    return *this;
  TargetValue = Other.TargetValue;
  EndianValue = Other.EndianValue;
  FormatValue = Other.FormatValue;
  ELFFlags = Other.ELFFlags;
  InputName = std::move(Other.InputName);
  Sections = std::move(Other.Sections);
  Symbols = std::move(Other.Symbols);
  SymbolNames = std::move(Other.SymbolNames);
  Relocations = std::move(Other.Relocations);
  Rebases = std::move(Other.Rebases);
  RelocationIntervals = std::move(Other.RelocationIntervals);
  RebaseIntervals = std::move(Other.RebaseIntervals);
  EHFrameReferences = std::move(Other.EHFrameReferences);
  CompactUnwind = std::move(Other.CompactUnwind);
  MachOBuildVersions = std::move(Other.MachOBuildVersions);
  UnwindInfoState = Other.UnwindInfoState;
  RelocationsApplied = Other.RelocationsApplied;
  Other.InputName.reset();
  Other.Sections.clear();
  Other.Symbols.clear();
  Other.SymbolNames.clear();
  Other.Relocations.clear();
  Other.Rebases.clear();
  Other.RelocationIntervals.clear();
  Other.RebaseIntervals.clear();
  Other.EHFrameReferences.clear();
  Other.CompactUnwind.clear();
  Other.MachOBuildVersions.clear();
  Other.ELFFlags = 0;
  Other.UnwindInfoState = MachOUnwindInfoState::None;
  Other.RelocationsApplied = false;
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

LinkExpect<void> LinkGraph::beginInput(std::string_view Name) {
  if (RelocationsApplied) {
    return relocated();
  }
  if (InputName) {
    return fail<void>(
        Diagnostic{"link graph accepts exactly one input object"});
  }
  InputName = std::string(Name);
  return {};
}

LinkExpect<SectionId> LinkGraph::addSection(Section Value) {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<SectionId>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return fail<SectionId>(
        Diagnostic{"link graph relocations already applied"});
  }
  if (Value.Alignment == 0 || (Value.Alignment & (Value.Alignment - 1)) != 0) {
    Diagnostic Diag{"section alignment must be a non-zero power of two"};
    Diag.SectionName = Value.Name;
    return fail<SectionId>(std::move(Diag));
  }
  if (Value.Content.size() > Value.VirtualSize) {
    Diagnostic Diag{"section content exceeds section virtual size"};
    Diag.SectionName = Value.Name;
    return fail<SectionId>(std::move(Diag));
  }
  if (Value.LinkedSection && *Value.LinkedSection >= Sections.size()) {
    Diagnostic Diag{"invalid linked section ID"};
    Diag.SectionName = Value.Name;
    return fail<SectionId>(std::move(Diag));
  }
  if (Sections.size() >= InvalidSectionId) {
    return fail<SectionId>(Diagnostic{"too many sections"});
  }
  const SectionId Id{static_cast<uint32_t>(Sections.size())};
  Sections.push_back(std::move(Value));
  return Id;
}

LinkExpect<SymbolId> LinkGraph::addSymbol(Symbol Value) {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<SymbolId>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return fail<SymbolId>(Diagnostic{"link graph relocations already applied"});
  }
  if (Value.Section == InvalidSectionId) {
    Diagnostic Diag{"undefined symbol"};
    Diag.SymbolName = Value.Name;
    Diag.Offset = Value.Offset;
    return fail<SymbolId>(std::move(Diag));
  }
  if (Value.Section >= Sections.size()) {
    Diagnostic Diag{"invalid section ID"};
    Diag.Section = Value.Section;
    Diag.SymbolName = Value.Name;
    Diag.Offset = Value.Offset;
    return fail<SymbolId>(std::move(Diag));
  }
  if (extendsBeyond(Value.Offset, Value.Size,
                    Sections[Value.Section].VirtualSize)) {
    Diagnostic Diag{"symbol extends beyond section virtual size"};
    Diag.Section = Value.Section;
    Diag.SymbolName = Value.Name;
    Diag.Offset = Value.Offset;
    return fail<SymbolId>(std::move(Diag));
  }
  if (findSymbol(Value.Name)) {
    Diagnostic Diag{"duplicate symbol definition"};
    Diag.SymbolName = Value.Name;
    Diag.Section = Value.Section;
    Diag.Offset = Value.Offset;
    return fail<SymbolId>(std::move(Diag));
  }
  if (Symbols.size() >= InvalidSymbolId) {
    return fail<SymbolId>(Diagnostic{"too many symbols"});
  }
  const SymbolId Id{static_cast<uint32_t>(Symbols.size())};
  auto Inserted = SymbolNames.emplace(Value.Name, Id);
  if (!Inserted.second)
    return fail<SymbolId>(Diagnostic{"duplicate symbol definition"});
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
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return relocated();
  }
  if (Value.Section >= Sections.size()) {
    Diagnostic Diag{"invalid section ID"};
    Diag.Section = Value.Section;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  if (Value.Symbol >= Symbols.size()) {
    Diagnostic Diag{"invalid symbol ID"};
    Diag.Section = Value.Section;
    Diag.Symbol = Value.Symbol;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  const auto Canonical = relocationPatchSize(Value.Format, TargetValue,
                                             Value.Type, Value.PatchSize);
  if (!Canonical) {
    Diagnostic Diag{"unsupported relocation patch size"};
    Diag.Section = Value.Section;
    Diag.Symbol = Value.Symbol;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  if (*Canonical != Value.PatchSize) {
    Diagnostic Diag{"invalid relocation patch size"};
    Diag.Section = Value.Section;
    Diag.Symbol = Value.Symbol;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  if (extendsBeyond(Value.Offset, Value.PatchSize,
                    Sections[Value.Section].Content.size())) {
    Diagnostic Diag{"relocation offset is outside section content"};
    Diag.Section = Value.Section;
    Diag.Symbol = Value.Symbol;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  if (Value.PatchSize != NoPatch) {
    const auto Key = std::make_pair(Value.Section, Value.Offset);
    auto First = RelocationIntervals.lower_bound(Key);
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
    bool Conflict = false;
    if (First != RelocationIntervals.begin()) {
      const auto Previous = std::prev(First);
      Conflict = Previous->first.first == Value.Section && Conflicts(*Previous);
    }
    for (auto It = First;
         !Conflict && It != RelocationIntervals.end() &&
         It->first.first == Value.Section && It->first.second >= Value.Offset &&
         Value.PatchSize > It->first.second - Value.Offset;
         ++It)
      Conflict = Conflicts(*It);
    if (Conflict) {
      Diagnostic Diag{"overlapping relocation patches"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    const auto Existing = RelocationIntervals.find(Key);
    if (Existing != RelocationIntervals.end() &&
        Existing->second.Count == Existing->second.Indices.size()) {
      Diagnostic Diag{"overlapping relocation patches"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
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
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return relocated();
  }
  if (Value.Section >= Sections.size()) {
    Diagnostic Diag{"invalid section ID"};
    Diag.Section = Value.Section;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  const uint8_t Width = std::max<uint8_t>(Value.Width, MinimumRebaseWidth);
  if (extendsBeyond(Value.Offset, Width,
                    Sections[Value.Section].Content.size())) {
    Diagnostic Diag{"rebase offset is outside section content"};
    Diag.Section = Value.Section;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  const auto Key = std::make_pair(Value.Section, Value.Offset);
  auto First = RebaseIntervals.lower_bound(Key);
  const auto Conflicts = [&](const auto &Entry) {
    return overlaps(Value, Rebases[Entry.second], [](const auto &Record) {
      return std::max<uint8_t>(Record.Width, MinimumRebaseWidth);
    });
  };
  bool Conflict = false;
  if (First != RebaseIntervals.begin()) {
    const auto Previous = std::prev(First);
    Conflict = Previous->first.first == Value.Section && Conflicts(*Previous);
  }
  for (auto It = First;
       !Conflict && It != RebaseIntervals.end() &&
       It->first.first == Value.Section && It->first.second >= Value.Offset &&
       Width > It->first.second - Value.Offset;
       ++It)
    Conflict = Conflicts(*It);
  if (Conflict) {
    Diagnostic Diag{"overlapping rebase patches"};
    Diag.Section = Value.Section;
    Diag.RelocationType = Value.Type;
    Diag.Offset = Value.Offset;
    return fail<void>(std::move(Diag));
  }
  Rebases.push_back(Value);
  try {
    RebaseIntervals.emplace(Key, Rebases.size() - 1);
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
        return fail<void>(Diagnostic{"overlapping relocation patches"});
      Group.Indices[Group.Count++] = I;
    }
  }
  for (size_t I = 0; I < RebaseValues.size(); ++I) {
    const auto &Value = RebaseValues[I];
    if (!NewRebaseIntervals
             .emplace(std::make_pair(Value.Section, Value.Offset), I)
             .second)
      return fail<void>(Diagnostic{"overlapping rebase patches"});
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
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied)
    return relocated();
  if (Value.Section >= Sections.size() || Value.Symbol >= Symbols.size() ||
      Sections[Value.Section].Purpose != SectionPurpose::EHFrame ||
      extendsBeyond(Value.Offset, 8, Sections[Value.Section].Content.size()))
    return fail<void>(Diagnostic{"invalid EH frame reference"});
  EHFrameReferences.push_back(Value);
  return {};
}

LinkExpect<void> LinkGraph::commitNormalizedEHFrame(
    std::vector<std::pair<SectionId, std::vector<Byte>>> Content,
    Span<const uint8_t> RemoveRelocations) {
  if (RelocationsApplied)
    return relocated();
  if (RemoveRelocations.size() != Relocations.size())
    return fail<void>(
        Diagnostic{"EH frame relocation mask size does not match relocations"});

  std::vector<uint8_t> Seen(Sections.size());
  for (const auto &[Id, Bytes] : Content) {
    if (Id >= Sections.size()) {
      Diagnostic Diag{"invalid normalized EH frame section ID"};
      Diag.Section = Id;
      return fail<void>(std::move(Diag));
    }
    if (Sections[Id].Purpose != SectionPurpose::EHFrame) {
      Diagnostic Diag{"normalized content targets a non-EH-frame section"};
      Diag.Section = Id;
      Diag.SectionName = Sections[Id].Name;
      return fail<void>(std::move(Diag));
    }
    if (Seen[Id]) {
      Diagnostic Diag{"duplicate normalized EH frame section"};
      Diag.Section = Id;
      Diag.SectionName = Sections[Id].Name;
      return fail<void>(std::move(Diag));
    }
    Seen[Id] = true;
    if (Bytes.size() > Sections[Id].VirtualSize) {
      Diagnostic Diag{"normalized EH frame content exceeds virtual size"};
      Diag.Section = Id;
      Diag.SectionName = Sections[Id].Name;
      Diag.Offset = Bytes.size();
      return fail<void>(std::move(Diag));
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
        Diagnostic Diag{"cannot remove a non-EH-frame relocation"};
        Diag.Section = Section;
        Diag.Symbol = Relocations[I].Symbol;
        Diag.RelocationType = Relocations[I].Type;
        Diag.Offset = Relocations[I].Offset;
        return fail<void>(std::move(Diag));
      }
      continue;
    }
    NewRelocations.push_back(Relocations[I]);
  }
  std::map<std::pair<SectionId, uint64_t>, RelocationIntervalGroup>
      NewIntervals;
  for (size_t I = 0; I < NewRelocations.size(); ++I) {
    const auto &Value = NewRelocations[I];
    const auto Canonical = relocationPatchSize(Value.Format, TargetValue,
                                               Value.Type, Value.PatchSize);
    if (Value.Section >= Sections.size() || Value.Symbol >= Symbols.size() ||
        !Canonical || *Canonical != Value.PatchSize ||
        extendsBeyond(Value.Offset, Value.PatchSize,
                      ContentSize(Value.Section))) {
      Diagnostic Diag{"normalized EH frame invalidates a relocation"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    if (Value.PatchSize == NoPatch)
      continue;
    auto &Group = NewIntervals[{Value.Section, Value.Offset}];
    if (Group.Count == Group.Indices.size() ||
        (Group.Count == 1 && !composeRISCVSymbolDifference(
                                 Value, NewRelocations[Group.Indices[0]])))
      return fail<void>(Diagnostic{"overlapping relocation patches"});
    Group.Indices[Group.Count++] = I;
  }
  std::optional<SectionId> PreviousSection;
  uint64_t PreviousEnd = 0;
  for (const auto &[Key, Group] : NewIntervals) {
    const auto &Value = NewRelocations[Group.Indices[0]];
    if (PreviousSection && *PreviousSection == Key.first &&
        Key.second < PreviousEnd)
      return fail<void>(Diagnostic{"overlapping relocation patches"});
    PreviousSection = Key.first;
    PreviousEnd = Key.second + Value.PatchSize;
  }
  for (const auto &Value : Rebases) {
    if (Value.Section >= Sections.size() ||
        extendsBeyond(Value.Offset,
                      std::max<uint8_t>(Value.Width, MinimumRebaseWidth),
                      ContentSize(Value.Section))) {
      Diagnostic Diag{"normalized EH frame invalidates a rebase"};
      Diag.Section = Value.Section;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
  }
  for (const auto &Value : EHFrameReferences) {
    if (Value.Section >= Sections.size() || Value.Symbol >= Symbols.size() ||
        Sections[Value.Section].Purpose != SectionPurpose::EHFrame ||
        extendsBeyond(Value.Offset, 8, ContentSize(Value.Section))) {
      Diagnostic Diag{"normalized EH frame invalidates an EH frame reference"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
  }

  for (auto &[Id, Bytes] : Content)
    Sections[Id].Content.swap(Bytes);
  Relocations.swap(NewRelocations);
  RelocationIntervals.swap(NewIntervals);
  return {};
}

LinkExpect<void> LinkGraph::addCompactUnwind(CompactUnwindRecord Value) {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied)
    return relocated();
  if (FormatValue != ObjectFormat::MachO) {
    return fail<void>(
        Diagnostic{"compact unwind requires a Mach-O link graph"});
  }
  auto Address = validateCompactUnwindRecord(Value, Sections, Symbols);
  if (!Address)
    return fail<void>(std::move(Address.error()));
  if (!CompactUnwind.empty()) {
    auto PreviousAddress =
        compactUnwindAddress(CompactUnwind.back(), Sections, Symbols);
    if (!PreviousAddress)
      return fail<void>(std::move(PreviousAddress.error()));
    auto Ordered = validateCompactUnwindOrder(CompactUnwind.back(),
                                              *PreviousAddress, *Address);
    if (!Ordered)
      return Ordered;
  }
  CompactUnwind.push_back(std::move(Value));
  return {};
}

LinkExpect<void> LinkGraph::addMachOBuildVersion(MachOBuildVersion Value) {
  if (RelocationsApplied)
    return relocated();
  if (FormatValue != ObjectFormat::MachO) {
    return fail<void>(
        Diagnostic{"Mach-O build version requires a Mach-O link graph"});
  }
  MachOBuildVersions.push_back(std::move(Value));
  return {};
}

LinkExpect<void> LinkGraph::pruneUnreferencedMachOEHFrame() {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied)
    return relocated();
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
      return fail<void>(Diagnostic{"section links to pruned EH frame"});
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
      return fail<void>(Diagnostic{"relocation targets pruned EH frame"});
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
      return fail<void>(Diagnostic{"compact unwind targets pruned EH frame"});
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
    return fail<void>(Diagnostic{"invalid graph after pruning EH frame"});
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
      Value.Purpose != SectionPurpose::UnwindInfo ||
      Value.Name != "__unwind_info" || Value.Kind != SectionKind::Unwind ||
      Value.Content.size() != Value.VirtualSize)
    return fail<void>(Diagnostic{"invalid Mach-O unwind info reservation"});
  if (Sections.size() >= InvalidSectionId)
    return fail<void>(Diagnostic{"too many sections"});
  auto NewSections = Sections;
  NewSections.push_back(std::move(Value));
  Sections.swap(NewSections);
  UnwindInfoState = MachOUnwindInfoState::Reserved;
  return {};
}

LinkExpect<void>
LinkGraph::populateMachOUnwindInfoSection(SectionId Id,
                                          std::vector<Byte> Content) {
  if (!RelocationsApplied ||
      UnwindInfoState != MachOUnwindInfoState::Reserved ||
      Id >= Sections.size() ||
      Sections[Id].Purpose != SectionPurpose::UnwindInfo ||
      Sections[Id].Name != "__unwind_info" ||
      Sections[Id].Kind != SectionKind::Unwind ||
      Content.size() != Sections[Id].VirtualSize)
    return fail<void>(Diagnostic{"invalid Mach-O unwind info population"});
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
    return fail<void>(Diagnostic{"invalid synthesized EH frame"});
  if (Existing && (*Existing >= Sections.size() ||
                   Sections[*Existing].Purpose != SectionPurpose::EHFrame))
    return fail<void>(Diagnostic{"invalid synthesized EH frame section"});
  if (!Existing && Sections.size() >= InvalidSectionId)
    return fail<void>(Diagnostic{"too many sections"});
  const SectionId Id =
      Existing ? *Existing : static_cast<SectionId>(Sections.size());
  for (auto &Reference : References) {
    if (Reference.Section != InvalidSectionId ||
        Reference.Symbol >= Symbols.size() ||
        extendsBeyond(Reference.Offset, 8, Value.Content.size()))
      return fail<void>(Diagnostic{"invalid synthesized EH frame reference"});
    Reference.Section = Id;
  }
  auto NewSections = Sections;
  auto NewReferences = EHFrameReferences;
  if (Existing)
    NewSections[*Existing] = std::move(Value);
  else
    NewSections.push_back(std::move(Value));
  NewReferences.insert(NewReferences.end(), References.begin(),
                       References.end());
  Sections.swap(NewSections);
  EHFrameReferences.swap(NewReferences);
  return {};
}

LinkExpect<void> LinkGraph::validate() const {
  if (!InputName) {
    return fail<void>(Diagnostic{"link graph requires one input object"});
  }
  for (SectionId I = 0; I < Sections.size(); ++I) {
    const auto &Value = Sections[I];
    if (Value.Alignment == 0 ||
        (Value.Alignment & (Value.Alignment - 1)) != 0) {
      Diagnostic Diag{"section alignment must be a non-zero power of two"};
      Diag.Section = I;
      Diag.SectionName = Value.Name;
      return fail<void>(std::move(Diag));
    }
    if (Value.Content.size() > Value.VirtualSize) {
      Diagnostic Diag{"section content exceeds section virtual size"};
      Diag.Section = I;
      Diag.SectionName = Value.Name;
      return fail<void>(std::move(Diag));
    }
    if (Value.LinkedSection && *Value.LinkedSection >= Sections.size()) {
      Diagnostic Diag{"invalid linked section ID"};
      Diag.Section = I;
      Diag.SectionName = Value.Name;
      return fail<void>(std::move(Diag));
    }
  }
  const auto UnwindInfoCount =
      std::count_if(Sections.begin(), Sections.end(), [](const auto &Section) {
        return Section.Purpose == SectionPurpose::UnwindInfo;
      });
  if ((UnwindInfoState == MachOUnwindInfoState::None && UnwindInfoCount != 0) ||
      (UnwindInfoState != MachOUnwindInfoState::None &&
       (FormatValue != ObjectFormat::MachO || CompactUnwind.empty() ||
        UnwindInfoCount != 1)))
    return fail<void>(Diagnostic{"incoherent Mach-O unwind info state"});
  if (UnwindInfoState != MachOUnwindInfoState::None) {
    const auto &UnwindInfo = *std::find_if(
        Sections.begin(), Sections.end(), [](const auto &Section) {
          return Section.Purpose == SectionPurpose::UnwindInfo;
        });
    if (UnwindInfo.Name != "__unwind_info" ||
        UnwindInfo.Kind != SectionKind::Unwind ||
        UnwindInfo.Content.size() != UnwindInfo.VirtualSize)
      return fail<void>(Diagnostic{"incoherent Mach-O unwind info section"});
  }
  for (size_t I = 0; I < Symbols.size(); ++I) {
    const auto &Value = Symbols[I];
    if (Value.Section >= Sections.size()) {
      Diagnostic Diag{"invalid section ID"};
      Diag.Section = Value.Section;
      Diag.Symbol = static_cast<SymbolId>(I);
      Diag.SymbolName = Value.Name;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    if (extendsBeyond(Value.Offset, Value.Size,
                      Sections[Value.Section].VirtualSize)) {
      Diagnostic Diag{"symbol extends beyond section virtual size"};
      Diag.Section = Value.Section;
      Diag.Symbol = static_cast<SymbolId>(I);
      Diag.SymbolName = Value.Name;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
  }
  for (const auto &Value : Relocations) {
    if (Value.Section >= Sections.size()) {
      Diagnostic Diag{"invalid section ID"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    if (Value.Symbol >= Symbols.size()) {
      Diagnostic Diag{"invalid symbol ID"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    const auto Canonical = relocationPatchSize(Value.Format, TargetValue,
                                               Value.Type, Value.PatchSize);
    if (!Canonical || *Canonical != Value.PatchSize) {
      Diagnostic Diag{Canonical ? "invalid relocation patch size"
                                : "unsupported relocation patch size"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    if (extendsBeyond(Value.Offset, Value.PatchSize,
                      Sections[Value.Section].Content.size())) {
      Diagnostic Diag{"relocation offset is outside section content"};
      Diag.Section = Value.Section;
      Diag.Symbol = Value.Symbol;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
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
      return fail<void>(Diagnostic{"overlapping relocation patches"});
    if (Count == 1 && isRISCVSymbolDifference(First))
      HasUnpairedRISCVDifference = true;
    if (RelocationSection && *RelocationSection == First.Section &&
        First.Offset < RelocationEnd)
      return fail<void>(Diagnostic{"overlapping relocation patches"});
    if (First.PatchSize > UINT64_MAX - First.Offset)
      return fail<void>(
          Diagnostic{"relocation offset is outside section content"});
    RelocationSection = First.Section;
    RelocationEnd = First.Offset + First.PatchSize;
    Begin = End;
  }
  if (HasUnpairedRISCVDifference)
    return fail<void>(Diagnostic{"unpaired RISC-V symbol difference"});
  for (const auto &Value : Rebases) {
    if (Value.Section >= Sections.size()) {
      Diagnostic Diag{"invalid section ID"};
      Diag.Section = Value.Section;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
    if (extendsBeyond(Value.Offset,
                      std::max<uint8_t>(Value.Width, MinimumRebaseWidth),
                      Sections[Value.Section].Content.size())) {
      Diagnostic Diag{"rebase offset is outside section content"};
      Diag.Section = Value.Section;
      Diag.RelocationType = Value.Type;
      Diag.Offset = Value.Offset;
      return fail<void>(std::move(Diag));
    }
  }
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
      return fail<void>(Diagnostic{"overlapping rebase patches"});
    const uint8_t Width = std::max<uint8_t>(Value.Width, MinimumRebaseWidth);
    if (Width > UINT64_MAX - Value.Offset)
      return fail<void>(Diagnostic{"rebase offset is outside section content"});
    RebaseSection = Value.Section;
    RebaseEnd = Value.Offset + Width;
  }
  if (!CompactUnwind.empty() && FormatValue != ObjectFormat::MachO) {
    return fail<void>(
        Diagnostic{"compact unwind requires a Mach-O link graph"});
  }
  std::optional<uint64_t> PreviousAddress;
  for (size_t I = 0; I < CompactUnwind.size(); ++I) {
    auto Address =
        validateCompactUnwindRecord(CompactUnwind[I], Sections, Symbols);
    if (!Address)
      return fail<void>(std::move(Address.error()));
    if (PreviousAddress) {
      auto Ordered = validateCompactUnwindOrder(CompactUnwind[I - 1],
                                                *PreviousAddress, *Address);
      if (!Ordered)
        return Ordered;
    }
    PreviousAddress = *Address;
  }
  return {};
}

LinkExpect<void> LinkGraph::setSectionAddress(SectionId Id, uint64_t Address) {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return relocated();
  }
  if (Id >= Sections.size()) {
    Diagnostic Diag{"invalid section ID"};
    Diag.Section = Id;
    return fail<void>(std::move(Diag));
  }
  Sections[Id].Address = Address;
  return {};
}

LinkExpect<void> LinkGraph::setSectionFileOffset(SectionId Id,
                                                 uint64_t FileOffset) {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return relocated();
  }
  if (Id >= Sections.size()) {
    Diagnostic Diag{"invalid section ID"};
    Diag.Section = Id;
    return fail<void>(std::move(Diag));
  }
  Sections[Id].FileOffset = FileOffset;
  return {};
}

LinkExpect<void> LinkGraph::setELFFlags(uint32_t Flags) {
  if (RelocationsApplied) {
    return relocated();
  }
  if (FormatValue != ObjectFormat::ELF) {
    return fail<void>(Diagnostic{"ELF flags require an ELF link graph"});
  }
  ELFFlags = Flags;
  return {};
}

LinkExpect<void> LinkGraph::setLinkedSection(SectionId Id, SectionId Linked) {
  if (UnwindInfoState == MachOUnwindInfoState::Populated)
    return fail<void>(Diagnostic{"Mach-O unwind info is populated"});
  if (RelocationsApplied) {
    return relocated();
  }
  if (Id >= Sections.size() || Linked >= Sections.size()) {
    return fail<void>(Diagnostic{"invalid linked section ID"});
  }
  Sections[Id].LinkedSection = Linked;
  return {};
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
