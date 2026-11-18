// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/thunks.h"

#include "linker/byte_io.h"
#include "linker/relocation.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

constexpr int64_t AArch64BranchRange = INT64_C(1) << 27;
constexpr int64_t ThunkPlacementMargin = INT64_C(1) << 20;
constexpr uint32_t AArch64BranchOpcodeMask = UINT32_C(0xFC000000);
constexpr uint32_t AArch64BranchImmediateMask = UINT32_C(0x03FFFFFF);
constexpr uint32_t AArch64BranchClassMask = UINT32_C(0x7C000000);
constexpr uint32_t AArch64BranchClass = UINT32_C(0x14000000);
constexpr uint32_t AArch64BranchLinkBit = UINT32_C(0x80000000);
constexpr unsigned AArch64BranchImmediateBits = 26;
constexpr unsigned AArch64BranchImmediateShift = 2;
constexpr uint64_t AArch64InstructionAlignment = 4;
constexpr std::array<uint32_t, 3> AArch64ThunkCode{
    UINT32_C(0x90000010),
    UINT32_C(0x91000210),
    UINT32_C(0xD61F0200),
};
constexpr uint64_t AArch64ThunkSize =
    AArch64ThunkCode.size() * sizeof(uint32_t);
constexpr uint64_t AArch64ThunkLow12Offset = 4;

bool isAArch64Branch26(const Relocation &Rel) noexcept {
  switch (Rel.Format) {
  case ObjectFormat::ELF:
    return Rel.Type == llvm::ELF::R_AARCH64_CALL26 ||
           Rel.Type == llvm::ELF::R_AARCH64_JUMP26;
  case ObjectFormat::MachO:
    return Rel.Type == llvm::MachO::ARM64_RELOC_BRANCH26;
  case ObjectFormat::COFF:
    return Rel.Type == llvm::COFF::IMAGE_REL_ARM64_BRANCH26;
  }
  return false;
}

uint32_t branch26Type(ObjectFormat Format, uint32_t Instruction) noexcept {
  switch (Format) {
  case ObjectFormat::ELF:
    return (Instruction & AArch64BranchLinkBit) != 0
               ? llvm::ELF::R_AARCH64_CALL26
               : llvm::ELF::R_AARCH64_JUMP26;
  case ObjectFormat::MachO:
    return llvm::MachO::ARM64_RELOC_BRANCH26;
  case ObjectFormat::COFF:
    return llvm::COFF::IMAGE_REL_ARM64_BRANCH26;
  }
  return 0;
}

uint32_t page21Type(ObjectFormat Format) noexcept {
  switch (Format) {
  case ObjectFormat::ELF:
    return llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21;
  case ObjectFormat::MachO:
    return llvm::MachO::ARM64_RELOC_PAGE21;
  case ObjectFormat::COFF:
    return llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21;
  }
  return 0;
}

uint32_t low12Type(ObjectFormat Format) noexcept {
  switch (Format) {
  case ObjectFormat::ELF:
    return llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC;
  case ObjectFormat::MachO:
    return llvm::MachO::ARM64_RELOC_PAGEOFF12;
  case ObjectFormat::COFF:
    return llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A;
  }
  return 0;
}

std::string islandName(ObjectFormat Format) {
  switch (Format) {
  case ObjectFormat::ELF:
    return ".text.thunks";
  case ObjectFormat::MachO:
    return "__thunks";
  case ObjectFormat::COFF:
    return ".text$thunks";
  }
  return {};
}

bool reachable(uint64_t From, uint64_t To) noexcept {
  int64_t Delta = 0;
  return Internal::signedDelta(To, From, Delta) &&
         Delta >= -AArch64BranchRange + ThunkPlacementMargin &&
         Delta < AArch64BranchRange - ThunkPlacementMargin;
}

int64_t branch26Displacement(uint32_t Instruction) noexcept {
  return llvm::SignExtend64<AArch64BranchImmediateBits>(
             Instruction & AArch64BranchImmediateMask) *
         (INT64_C(1) << AArch64BranchImmediateShift);
}

// Displacement of a PC-relative instruction whose target the assembler
// already encoded. ADRP is absent because its page offset depends on the
// final address, so the assembler always leaves a relocation for it.
std::optional<int64_t> encodedDisplacement(uint32_t Instruction) noexcept {
  if ((Instruction & AArch64BranchClassMask) == AArch64BranchClass)
    return branch26Displacement(Instruction);
  if ((Instruction & UINT32_C(0xFF000000)) == UINT32_C(0x54000000) ||
      (Instruction & UINT32_C(0x7E000000)) == UINT32_C(0x34000000) ||
      (Instruction & UINT32_C(0x3B000000)) == UINT32_C(0x18000000))
    return llvm::SignExtend64<19>((Instruction >> 5) & UINT32_C(0x7FFFF)) * 4;
  if ((Instruction & UINT32_C(0x7E000000)) == UINT32_C(0x36000000))
    return llvm::SignExtend64<14>((Instruction >> 5) & UINT32_C(0x3FFF)) * 4;
  if ((Instruction & UINT32_C(0x9F000000)) == UINT32_C(0x10000000))
    return llvm::SignExtend64<21>(
        ((Instruction >> 29) & UINT32_C(3)) |
        (((Instruction >> 5) & UINT32_C(0x7FFFF)) << 2));
  return std::nullopt;
}

// Byte addend of a relocation relative to its symbol, including an addend
// that the object stores in the patched field.
std::optional<int64_t> relocationAddend(const LinkGraph &Graph,
                                        const Relocation &Rel) {
  if (!Rel.AddendIsImplicit)
    return Rel.Addend;
  const auto &Content = Graph.sections()[Rel.Section].Content;
  if (Rel.Format == ObjectFormat::MachO) {
    if (Rel.Type != llvm::MachO::ARM64_RELOC_UNSIGNED)
      return Rel.Addend;
    if (const auto Value = Internal::readSigned(
            Content, Rel.Offset, DoubleWordPatch, Endianness::Little))
      return *Value;
    return std::nullopt;
  }
  if (Rel.Format != ObjectFormat::COFF)
    return std::nullopt;
  if (Rel.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR64) {
    if (const auto Value = Internal::readSigned(
            Content, Rel.Offset, DoubleWordPatch, Endianness::Little))
      return *Value;
    return std::nullopt;
  }
  const auto Word = Internal::readUnsigned(Content, Rel.Offset, WordPatch,
                                           Endianness::Little);
  if (!Word)
    return std::nullopt;
  const auto Instruction = static_cast<uint32_t>(*Word);
  int64_t Implicit = 0;
  switch (Rel.Type) {
  case llvm::COFF::IMAGE_REL_ARM64_ADDR32NB:
    Implicit = static_cast<int64_t>(*Word);
    break;
  case llvm::COFF::IMAGE_REL_ARM64_BRANCH26:
    Implicit = branch26Displacement(Instruction);
    break;
  case llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21:
    Implicit =
        llvm::SignExtend64<21>(((Instruction >> 29) & UINT32_C(3)) |
                               ((Instruction >> 3) & UINT32_C(0x1FFFFC)));
    break;
  case llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A:
    Implicit = (Instruction >> 10) & UINT32_C(0xFFF);
    break;
  case llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L: {
    const auto Scale = Internal::aarch64LoadStoreScale(Instruction);
    if (!Scale)
      return std::nullopt;
    Implicit =
        static_cast<int64_t>(((Instruction >> 10) & UINT32_C(0xFFF)) << *Scale);
    break;
  }
  default:
    return std::nullopt;
  }
  int64_t Addend = 0;
  if (!Internal::addSigned(Rel.Addend, Implicit, Addend))
    return std::nullopt;
  return Addend;
}

bool symbolIn(const LinkGraph &Graph, SymbolId Id, SectionId Section) {
  return Id < Graph.symbols().size() && Graph.symbols()[Id].Section == Section;
}

struct Retarget {
  size_t Relocation;
  size_t Piece;
  int64_t Addend;
};

struct SplitPlan {
  std::vector<uint64_t> Cuts;
  std::vector<std::pair<uint64_t, uint64_t>> Branches;
  std::vector<Retarget> Retargets;

  size_t piece(uint64_t Offset) const noexcept {
    return static_cast<size_t>(
        std::upper_bound(Cuts.begin(), Cuts.end(), Offset) - Cuts.begin());
  }
  uint64_t start(size_t Piece) const noexcept {
    return Piece == 0 ? 0 : Cuts[Piece - 1];
  }
};

std::vector<uint64_t> functionStarts(const LinkGraph &Graph, SectionId Id) {
  const auto &Text = Graph.sections()[Id];
  std::vector<uint64_t> Starts;
  auto Add = [&](uint64_t Offset) {
    if (Offset > 0 && Offset < Text.Content.size() &&
        Offset % Text.Alignment == 0)
      Starts.push_back(Offset);
  };
  for (const auto &Value : Graph.symbols())
    if (Value.Section == Id && Value.Size > 0)
      Add(Value.Offset);
  for (const auto &Record : Graph.compactUnwind())
    if (symbolIn(Graph, Record.Function, Id))
      Add(Graph.symbols()[Record.Function].Offset);
  for (const auto &Rel : Graph.relocations()) {
    if (Graph.sections()[Rel.Section].Kind != SectionKind::Unwind ||
        !symbolIn(Graph, Rel.Symbol, Id))
      continue;
    const auto Addend = relocationAddend(Graph, Rel);
    uint64_t Target = 0;
    if (Addend && Internal::addSigned(Graph.symbols()[Rel.Symbol].Offset,
                                      *Addend, Target))
      Add(Target);
  }
  std::sort(Starts.begin(), Starts.end());
  Starts.erase(std::unique(Starts.begin(), Starts.end()), Starts.end());
  return Starts;
}

// Cuts at the returned starts break no symbol, relocation, rebase or
// assembler-resolved reference other than a branch to a function start.
std::vector<uint64_t>
allowedStarts(const LinkGraph &Graph, SectionId Id,
              const std::vector<uint64_t> &Starts,
              std::vector<std::pair<uint64_t, uint64_t>> &Branches) {
  const auto &Content = Graph.sections()[Id].Content;
  std::vector<int64_t> Blocked(Starts.size() + 1);
  auto Forbid = [&](uint64_t Low, uint64_t High) {
    const auto First = std::upper_bound(Starts.begin(), Starts.end(), Low);
    const auto Last = std::upper_bound(Starts.begin(), Starts.end(), High);
    if (First >= Last)
      return;
    ++Blocked[static_cast<size_t>(First - Starts.begin())];
    --Blocked[static_cast<size_t>(Last - Starts.begin())];
  };
  std::vector<bool> Relocated(Content.size() / AArch64InstructionAlignment);
  for (const auto &Rel : Graph.relocations()) {
    if (Rel.Section != Id)
      continue;
    Forbid(Rel.Offset, Rel.Offset + std::max<uint64_t>(Rel.PatchSize, 1) - 1);
    if (Rel.Offset % AArch64InstructionAlignment == 0 &&
        Rel.Offset / AArch64InstructionAlignment < Relocated.size())
      Relocated[Rel.Offset / AArch64InstructionAlignment] = true;
  }
  for (const auto &Value : Graph.rebases())
    if (Value.Section == Id)
      Forbid(Value.Offset,
             Value.Offset + std::max<uint64_t>(Value.Width, 1) - 1);
  for (const auto &Value : Graph.symbols())
    if (Value.Section == Id && Value.Size > 0)
      Forbid(Value.Offset, Value.Offset + Value.Size - 1);
  for (uint64_t Index = 0; Index < Relocated.size(); ++Index) {
    if (Relocated[Index])
      continue;
    const uint64_t Offset = Index * AArch64InstructionAlignment;
    const auto Instruction = static_cast<uint32_t>(
        Internal::getInteger(Content, Offset, WordPatch, Endianness::Little));
    const auto Displacement = encodedDisplacement(Instruction);
    uint64_t Target = 0;
    if (!Displacement || *Displacement == 0 ||
        !Internal::addSigned(Offset, *Displacement, Target) ||
        Target > Content.size())
      continue;
    if ((Instruction & AArch64BranchClassMask) == AArch64BranchClass &&
        std::binary_search(Starts.begin(), Starts.end(), Target)) {
      Branches.emplace_back(Offset, Target);
      continue;
    }
    Forbid(std::min(Offset, Target), std::max(Offset, Target));
  }
  std::vector<uint64_t> Result;
  int64_t Depth = 0;
  for (size_t Index = 0; Index < Starts.size(); ++Index) {
    Depth += Blocked[Index];
    if (Depth == 0)
      Result.push_back(Starts[Index]);
  }
  return Result;
}

std::vector<uint64_t> chooseCuts(const std::vector<uint64_t> &Allowed,
                                 uint64_t Size, uint64_t PieceSize) {
  std::vector<uint64_t> Cuts;
  uint64_t Start = 0;
  while (Size - Start > PieceSize) {
    const auto Limit =
        std::upper_bound(Allowed.begin(), Allowed.end(), Start + PieceSize);
    auto Cut = Limit;
    if (Cut != Allowed.begin() && *std::prev(Cut) > Start)
      Cut = std::prev(Cut);
    else if (Cut == Allowed.end())
      break;
    Cuts.push_back(*Cut);
    Start = *Cut;
  }
  return Cuts;
}

std::optional<SplitPlan> planSplit(const LinkGraph &Graph, SectionId Id,
                                   uint64_t PieceSize) {
  const auto &Text = Graph.sections()[Id];
  if (Text.Kind != SectionKind::Text || Text.Anchor ||
      Text.Content.size() != Text.VirtualSize ||
      Text.Content.size() <= PieceSize)
    return std::nullopt;
  for (const auto &Value : Graph.sections())
    if (Value.LinkedSection == Id)
      return std::nullopt;
  const auto Starts = functionStarts(Graph, Id);
  SplitPlan Plan;
  std::vector<std::pair<uint64_t, uint64_t>> Branches;
  Plan.Cuts = chooseCuts(allowedStarts(Graph, Id, Starts, Branches),
                         Text.Content.size(), PieceSize);
  if (Plan.Cuts.empty())
    return std::nullopt;
  for (const auto &Branch : Branches)
    if (Plan.piece(Branch.first) != Plan.piece(Branch.second))
      Plan.Branches.push_back(Branch);
  const auto &Relocations = Graph.relocations();
  for (size_t Index = 0; Index < Relocations.size(); ++Index) {
    const auto &Rel = Relocations[Index];
    if (!symbolIn(Graph, Rel.Symbol, Id) ||
        (Rel.Format == ObjectFormat::MachO &&
         Rel.Type == llvm::MachO::ARM64_RELOC_SUBTRACTOR))
      continue;
    const auto &Target = Graph.symbols()[Rel.Symbol];
    const auto Addend = relocationAddend(Graph, Rel);
    if (!Addend) {
      if (Target.Size == 0)
        return std::nullopt;
      continue;
    }
    uint64_t Offset = 0;
    if (!Internal::addSigned(Target.Offset, *Addend, Offset))
      return std::nullopt;
    const size_t Piece = Plan.piece(Offset);
    if (Piece == Plan.piece(Target.Offset))
      continue;
    Plan.Retargets.push_back(Retarget{
        Index, Piece, static_cast<int64_t>(Offset - Plan.start(Piece))});
  }
  return Plan;
}

} // namespace

LinkExpect<bool> splitTextSections(LinkGraph &Graph, uint64_t PieceSize) {
  if (Graph.target() != Target::AArch64 || PieceSize == 0)
    return false;
  EXPECTED_TRY(Graph.checkMutable());
  bool Split = false;
  const SectionId Count = static_cast<SectionId>(Graph.Sections.size());
  for (SectionId Id = 0; Id < Count; ++Id) {
    const auto Plan = planSplit(Graph, Id, PieceSize);
    if (!Plan)
      continue;
    std::vector<SectionId> Pieces{Id};
    std::vector<SymbolId> Anchors;
    for (size_t Piece = 1; Piece <= Plan->Cuts.size(); ++Piece) {
      const auto &Text = Graph.Sections[Id];
      const uint64_t Begin = Plan->start(Piece);
      const uint64_t End =
          Piece < Plan->Cuts.size() ? Plan->Cuts[Piece] : Text.Content.size();
      Section Value{Text.Name, Text.Kind, Text.Alignment, End - Begin};
      Value.Content.assign(
          Text.Content.begin() + static_cast<std::ptrdiff_t>(Begin),
          Text.Content.begin() + static_cast<std::ptrdiff_t>(End));
      Value.Purpose = Text.Purpose;
      Value.InputAddress = Text.InputAddress + Begin;
      EXPECTED_TRY(const SectionId PieceId, Graph.addSection(std::move(Value)));
      Pieces.push_back(PieceId);
    }
    auto &Text = Graph.Sections[Id];
    Text.Content.resize(Plan->Cuts.front());
    Text.Content.shrink_to_fit();
    Text.VirtualSize = Text.Content.size();
    for (const SectionId Piece : Pieces) {
      EXPECTED_TRY(
          const SymbolId Anchor,
          Graph.addSymbol(Symbol{"$section.split." + std::to_string(Piece),
                                 Piece, 0, 0, false}));
      Anchors.push_back(Anchor);
    }
    for (auto &Value : Graph.Symbols) {
      if (Value.Section != Id)
        continue;
      const size_t Piece = Plan->piece(Value.Offset);
      Value.Offset -= Plan->start(Piece);
      Value.Section = Pieces[Piece];
    }
    for (const auto &Change : Plan->Retargets) {
      auto &Rel = Graph.Relocations[Change.Relocation];
      Rel.Symbol = Anchors[Change.Piece];
      Rel.Addend = Change.Addend;
      Rel.AddendIsImplicit = false;
    }
    auto Move = [&](SectionId &Section, uint64_t &Offset) {
      if (Section != Id)
        return;
      const size_t Piece = Plan->piece(Offset);
      Offset -= Plan->start(Piece);
      Section = Pieces[Piece];
    };
    for (auto &Rel : Graph.Relocations)
      Move(Rel.Section, Rel.Offset);
    for (auto &Value : Graph.Rebases)
      Move(Value.Section, Value.Offset);
    for (auto &Value : Graph.EHFrameReferences)
      Move(Value.Section, Value.Offset);
    for (const auto &[Offset, Target] : Plan->Branches) {
      const size_t From = Plan->piece(Offset);
      const size_t To = Plan->piece(Target);
      const auto Instruction = static_cast<uint32_t>(Internal::getInteger(
          Graph.Sections[Pieces[From]].Content, Offset - Plan->start(From),
          WordPatch, Endianness::Little));
      EXPECTED_TRY(Graph.addRelocation(
          Relocation{Pieces[From], Offset - Plan->start(From),
                     branch26Type(Graph.format(), Instruction), Anchors[To],
                     static_cast<int64_t>(Target - Plan->start(To)), false,
                     Graph.format(), WordPatch}));
    }
    Split = true;
  }
  if (Split) {
    EXPECTED_TRY(Graph.validate());
  }
  return Split;
}

LinkExpect<bool> insertBranchThunks(LinkGraph &Graph) {
  if (Graph.target() != Target::AArch64)
    return false;
  EXPECTED_TRY(Graph.checkMutable());

  std::map<std::pair<SectionId, bool>, SectionId> Islands;
  for (SectionId Id = 0; Id < Graph.Sections.size(); ++Id)
    if (const auto &Anchor = Graph.Sections[Id].Anchor)
      Islands.emplace(std::pair(Anchor->Section, Anchor->After), Id);

  using ThunkKey = std::tuple<SectionId, SymbolId, int64_t>;
  std::map<ThunkKey, SymbolId> Thunks;
  if (!Islands.empty()) {
    std::map<std::pair<SectionId, uint64_t>, SymbolId> IslandSymbols;
    for (SymbolId Id = 0; Id < Graph.Symbols.size(); ++Id) {
      const auto &Value = Graph.Symbols[Id];
      if (Graph.Sections[Value.Section].Anchor)
        IslandSymbols.emplace(std::pair(Value.Section, Value.Offset), Id);
    }
    for (const auto &Rel : Graph.Relocations) {
      if (!Graph.Sections[Rel.Section].Anchor ||
          Rel.Type != page21Type(Graph.format()))
        continue;
      if (const auto Found =
              IslandSymbols.find(std::pair(Rel.Section, Rel.Offset));
          Found != IslandSymbols.end())
        Thunks.emplace(ThunkKey{Rel.Section, Rel.Symbol, Rel.Addend},
                       Found->second);
    }
  }

  bool Inserted = false;
  const size_t Count = Graph.Relocations.size();
  for (size_t Index = 0; Index < Count; ++Index) {
    const Relocation Rel = Graph.Relocations[Index];
    if (!isAArch64Branch26(Rel) ||
        Graph.Sections[Graph.Symbols[Rel.Symbol].Section].Anchor)
      continue;
    EXPECTED_TRY(const uint64_t S, Internal::symbolAddress(Graph, Rel));
    EXPECTED_TRY(const uint64_t P, Internal::placeAddress(Graph, Rel));
    int64_t Addend = Rel.Addend;
    if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF) {
      const auto Word =
          Internal::readUnsigned(Graph.Sections[Rel.Section].Content,
                                 Rel.Offset, WordPatch, Endianness::Little);
      if (!Word)
        return Internal::relocationError(
            Graph, Rel, "relocation field exceeds section content");
      if (!Internal::addSigned(
              Addend, branch26Displacement(static_cast<uint32_t>(*Word)),
              Addend))
        return Internal::relocationError(Graph, Rel,
                                         "branch displacement overflows");
    }
    uint64_t Destination = 0;
    if (!Internal::addSigned(S, Addend, Destination))
      return Internal::relocationError(Graph, Rel,
                                       "branch displacement overflows");
    if (reachable(P, Destination))
      continue;

    const auto &Caller = Graph.Sections[Rel.Section];
    std::optional<bool> After;
    if (reachable(P, Caller.Address))
      After = false;
    else if (reachable(P, Caller.Address + Caller.VirtualSize))
      After = true;
    if (!After)
      continue;

    SectionId Island = InvalidSectionId;
    if (const auto Found = Islands.find(std::pair(Rel.Section, *After));
        Found != Islands.end()) {
      Island = Found->second;
    } else {
      Section Value{islandName(Graph.format()), SectionKind::Text,
                    AArch64InstructionAlignment};
      Value.Anchor = SectionAnchor{Rel.Section, *After};
      EXPECTED_TRY(Island, Graph.addSection(std::move(Value)));
      Islands.emplace(std::pair(Rel.Section, *After), Island);
    }

    const ThunkKey Key{Island, Rel.Symbol, Addend};
    auto Found = Thunks.find(Key);
    if (Found == Thunks.end()) {
      auto &IslandSection = Graph.Sections[Island];
      const uint64_t Offset = IslandSection.Content.size();
      IslandSection.Content.resize(Offset + AArch64ThunkSize);
      IslandSection.VirtualSize = IslandSection.Content.size();
      for (size_t I = 0; I < AArch64ThunkCode.size(); ++I)
        Internal::putInteger(IslandSection.Content,
                             Offset + I * sizeof(uint32_t), AArch64ThunkCode[I],
                             WordPatch);
      EXPECTED_TRY(
          const SymbolId Thunk,
          Graph.addSymbol(Symbol{"__wasmedge_branch_thunk_" +
                                     std::to_string(Graph.Symbols.size()),
                                 Island, Offset, AArch64ThunkSize, false}));
      EXPECTED_TRY(Graph.addRelocation(
          Relocation{Island, Offset, page21Type(Graph.format()), Rel.Symbol,
                     Addend, false, Graph.format(), WordPatch}));
      EXPECTED_TRY(Graph.addRelocation(Relocation{
          Island, Offset + AArch64ThunkLow12Offset, low12Type(Graph.format()),
          Rel.Symbol, Addend, false, Graph.format(), WordPatch}));
      Found = Thunks.emplace(Key, Thunk).first;
    }

    auto &Redirected = Graph.Relocations[Index];
    Redirected.Symbol = Found->second;
    Redirected.Addend = 0;
    if (Redirected.AddendIsImplicit &&
        Redirected.Format == ObjectFormat::COFF) {
      auto &Content = Graph.Sections[Redirected.Section].Content;
      const uint64_t Word =
          Internal::getInteger(Content, Redirected.Offset, WordPatch) &
          AArch64BranchOpcodeMask;
      Internal::putInteger(Content, Redirected.Offset, Word, WordPatch);
    }
    Inserted = true;
  }
  return Inserted;
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
