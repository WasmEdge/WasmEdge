// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>

#include <string_view>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {
namespace {

constexpr uint64_t PEImageBase = UINT64_C(0x180000000);

enum class Low12Scale : unsigned {
  Byte,
  Half,
  Word,
  DoubleWord,
  QuadWord,
};

struct Low12Encoding {
  Low12Scale Scale;
  uint32_t OpcodeMask;
  uint32_t Opcode;
};

constexpr unsigned value(Low12Scale Value) noexcept {
  return static_cast<unsigned>(Value);
}

LinkExpect<RelocationResult>
fail(const Relocation &Value, std::string_view Message,
     DiagnosticKind Kind = DiagnosticKind::Malformed) {
  Diagnostic Diag{std::string(Message)};
  Diag.Section = Value.Section;
  Diag.Symbol = Value.Symbol;
  Diag.RelocationType = Value.Type;
  Diag.Offset = Value.Offset;
  Diag.Kind = Kind;
  return Unexpected<Diagnostic>(std::move(Diag));
}

bool addUnsigned(uint64_t Left, uint64_t Right, uint64_t &Result) noexcept {
  if (Right > UINT64_MAX - Left)
    return false;
  Result = Left + Right;
  return true;
}

bool addSigned(uint64_t Base, int64_t Delta, uint64_t &Result) noexcept {
  if (Delta >= 0)
    return addUnsigned(Base, static_cast<uint64_t>(Delta), Result);
  const uint64_t Magnitude = static_cast<uint64_t>(-(Delta + 1)) + 1;
  if (Magnitude > Base)
    return false;
  Result = Base - Magnitude;
  return true;
}

bool addSigned(int64_t Left, int64_t Right, int64_t &Result) noexcept {
  if ((Right > 0 && Left > INT64_MAX - Right) ||
      (Right < 0 && Left < INT64_MIN - Right))
    return false;
  Result = Left + Right;
  return true;
}

bool signedDelta(uint64_t Left, uint64_t Right, int64_t &Result) noexcept {
  if (Left >= Right) {
    const uint64_t Difference = Left - Right;
    if (Difference > static_cast<uint64_t>(INT64_MAX))
      return false;
    Result = static_cast<int64_t>(Difference);
    return true;
  }
  const uint64_t Difference = Right - Left;
  const uint64_t MinimumMagnitude = UINT64_C(1) << 63;
  if (Difference > MinimumMagnitude)
    return false;
  Result = Difference == MinimumMagnitude ? INT64_MIN
                                          : -static_cast<int64_t>(Difference);
  return true;
}

bool signedBits(int64_t Value, unsigned Bits) noexcept {
  if (Bits == 64)
    return true;
  const int64_t Limit = INT64_C(1) << (Bits - 1);
  return Value >= -Limit && Value < Limit;
}

int64_t signExtend(uint64_t Value, unsigned Bits) noexcept {
  const uint64_t Sign = UINT64_C(1) << (Bits - 1);
  return static_cast<int64_t>((Value ^ Sign) - Sign);
}

} // namespace

LinkExpect<RelocationResult> applyAArch64(const LinkGraph &Graph) {
  constexpr uint8_t InstructionWidth = 4;
  constexpr uint64_t InstructionAlignmentMask = InstructionWidth - 1;
  RelocationResult Result;
  Result.Content.reserve(Graph.sections().size());
  for (const auto &Section : Graph.sections()) {
    Result.Content.push_back(Section.Content);
  }
  Result.Rebases = Graph.rebases();
  RebaseIntervalIndex RebaseIntervals(Result.Rebases);
  for (const auto &Rel : Graph.relocations()) {
    auto &Bytes = Result.Content[Rel.Section];
    const auto &Symbol = Graph.symbols()[Rel.Symbol];
    int64_t Addend = Rel.Addend;
    if (Rel.AddendIsImplicit &&
        ((Rel.Format == ObjectFormat::MachO &&
          Rel.Type == llvm::MachO::ARM64_RELOC_UNSIGNED) ||
         (Rel.Format == ObjectFormat::COFF &&
          Rel.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR64))) {
      auto Value = readSigned(Bytes, Rel.Offset, 8, Endianness::Little);
      if (!Value)
        return fail(Rel, "cannot decode implicit addend");
      Addend = *Value;
    }
    if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF &&
        Rel.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR32NB) {
      auto Value = readUnsigned(Bytes, Rel.Offset, 4, Endianness::Little);
      if (!Value || *Value > INT64_MAX ||
          Addend > INT64_MAX - static_cast<int64_t>(*Value))
        return fail(Rel, "cannot decode implicit addend");
      Addend += static_cast<int64_t>(*Value);
    }
    uint64_t S = 0;
    uint64_t P = 0;
    if (!addUnsigned(Graph.sections()[Symbol.Section].Address, Symbol.Offset,
                     S) ||
        !addSigned(S, Addend, S) ||
        !addUnsigned(Graph.sections()[Rel.Section].Address, Rel.Offset, P))
      return fail(Rel, "relocation address overflows");
    if (Rel.Format == ObjectFormat::COFF &&
        Rel.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR32NB) {
      if (S < PEImageBase || S - PEImageBase > UINT32_MAX ||
          !writeUnsigned(Bytes, Rel.Offset, InstructionWidth,
                         Endianness::Little, S - PEImageBase)) {
        return fail(Rel, "ADDR32NB relocation overflows");
      }
      continue;
    }
    if ((Rel.Format == ObjectFormat::ELF &&
         Rel.Type == llvm::ELF::R_AARCH64_ABS64) ||
        (Rel.Format == ObjectFormat::MachO &&
         Rel.Type == llvm::MachO::ARM64_RELOC_UNSIGNED) ||
        (Rel.Format == ObjectFormat::COFF &&
         Rel.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR64)) {
      constexpr uint8_t DoubleWordWidth = 8;
      if (!writeUnsigned(Bytes, Rel.Offset, DoubleWordWidth, Endianness::Little,
                         S)) {
        return fail(Rel, "absolute relocation overflows");
      }
      if (!RebaseIntervals.insert(Rel.Section, Rel.Offset, DoubleWordWidth)) {
        return fail(Rel, "overlapping generated rebase");
      }
      Result.Rebases.push_back(Rebase{Rel.Section, Rel.Offset, Rel.Type, Addend,
                                      DoubleWordWidth, Rel.Format});
      continue;
    }
    if (Rel.Format == ObjectFormat::ELF &&
        Rel.Type == llvm::ELF::R_AARCH64_PREL64) {
      constexpr uint8_t DoubleWordWidth = 8;
      constexpr unsigned DoubleWordBits = 64;
      int64_t Value = 0;
      if (!signedDelta(S, P, Value) || !signedBits(Value, DoubleWordBits) ||
          !writeSigned(Bytes, Rel.Offset, DoubleWordWidth, Endianness::Little,
                       Value)) {
        return fail(Rel, "PREL64 relocation overflows");
      }
      continue;
    }
    if (Rel.Format == ObjectFormat::ELF &&
        Rel.Type == llvm::ELF::R_AARCH64_PREL32) {
      constexpr unsigned WordBits = 32;
      int64_t Value = 0;
      if (!signedDelta(S, P, Value) || !signedBits(Value, WordBits) ||
          !writeSigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little,
                       Value)) {
        return fail(Rel, "PREL32 relocation overflows");
      }
      continue;
    }
    if ((Rel.Offset & InstructionAlignmentMask) != 0) {
      return fail(Rel, "invalid relocation field");
    }
    auto Word =
        readUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little);
    if (!Word) {
      return fail(Rel, "relocation field exceeds section content");
    }
    uint32_t Instruction = static_cast<uint32_t>(*Word);
    const bool Branch26 = (Rel.Format == ObjectFormat::ELF &&
                           (Rel.Type == llvm::ELF::R_AARCH64_JUMP26 ||
                            Rel.Type == llvm::ELF::R_AARCH64_CALL26)) ||
                          (Rel.Format == ObjectFormat::MachO &&
                           Rel.Type == llvm::MachO::ARM64_RELOC_BRANCH26) ||
                          (Rel.Format == ObjectFormat::COFF &&
                           Rel.Type == llvm::COFF::IMAGE_REL_ARM64_BRANCH26);
    if (Branch26) {
      constexpr unsigned BranchDisplacementBits = 28;
      constexpr unsigned BranchImmediateShift = 2;
      constexpr uint32_t BranchOpcodeMask = UINT32_C(0xFC000000);
      constexpr uint32_t BranchImmediateMask = UINT32_C(0x03FFFFFF);
      constexpr uint32_t JumpOpcode = UINT32_C(0x14000000);
      constexpr uint32_t CallOpcode = UINT32_C(0x94000000);
      const uint32_t Opcode = (Instruction & BranchOpcodeMask) == CallOpcode
                                  ? CallOpcode
                                  : JumpOpcode;
      if ((Instruction & BranchOpcodeMask) != Opcode) {
        return fail(Rel, "invalid branch instruction");
      }
      int64_t Value = 0;
      if (!signedDelta(S, P, Value))
        return fail(Rel, "branch displacement overflows");
      if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF &&
          !addSigned(Value,
                     signExtend(Instruction & BranchImmediateMask, 26)
                         << BranchImmediateShift,
                     Value))
        return fail(Rel, "branch displacement overflows");
      if ((Value & static_cast<int64_t>(InstructionAlignmentMask)) != 0 ||
          !signedBits(Value, BranchDisplacementBits)) {
        return fail(Rel, "branch displacement overflows");
      }
      Instruction =
          Opcode | (static_cast<uint32_t>(Value >> BranchImmediateShift) &
                    BranchImmediateMask);
    } else if ((Rel.Format == ObjectFormat::ELF &&
                Rel.Type == llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21) ||
               (Rel.Format == ObjectFormat::MachO &&
                Rel.Type == llvm::MachO::ARM64_RELOC_PAGE21) ||
               (Rel.Format == ObjectFormat::COFF &&
                Rel.Type == llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21)) {
      constexpr unsigned PageShift = 12;
      constexpr unsigned PageDisplacementBits = 21;
      constexpr uint32_t AdrpOpcodeMask = UINT32_C(0x9F000000);
      constexpr uint32_t AdrpOpcode = UINT32_C(0x90000000);
      constexpr uint32_t AdrpPreservedMask = UINT32_C(0x9F00001F);
      constexpr uint32_t AdrpImmediateMask = UINT32_C(0x001FFFFF);
      constexpr uint32_t AdrpImmediateLowMask = UINT32_C(0x00000003);
      constexpr uint32_t AdrpImmediateHighMask = UINT32_C(0x001FFFFC);
      constexpr unsigned AdrpImmediateLowShift = 29;
      constexpr unsigned AdrpImmediateHighShift = 3;
      if ((Instruction & AdrpOpcodeMask) != AdrpOpcode) {
        return fail(Rel, "invalid ADRP instruction");
      }
      uint64_t Target = S;
      if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF) {
        const uint32_t Raw =
            ((Instruction >> AdrpImmediateLowShift) & 3) |
            ((Instruction >> AdrpImmediateHighShift) & AdrpImmediateHighMask);
        if (!addSigned(Target, signExtend(Raw, PageDisplacementBits), Target))
          return fail(Rel, "ADRP page displacement overflows");
      }
      int64_t Pages = 0;
      if (!signedDelta(Target >> PageShift, P >> PageShift, Pages))
        return fail(Rel, "ADRP page displacement overflows");
      if (!signedBits(Pages, PageDisplacementBits)) {
        return fail(Rel, "ADRP page displacement overflows");
      }
      const uint32_t Value = static_cast<uint32_t>(Pages) & AdrpImmediateMask;
      Instruction = (Instruction & AdrpPreservedMask) |
                    ((Value & AdrpImmediateLowMask) << AdrpImmediateLowShift) |
                    ((Value & AdrpImmediateHighMask) << AdrpImmediateHighShift);
    } else {
      constexpr uint64_t PageOffsetMask = UINT64_C(0xFFF);
      constexpr uint32_t Low12ImmediateMask = UINT32_C(0x003FFC00);
      constexpr unsigned Low12ImmediateShift = 10;
      Low12Encoding Encoding{};
      if (Rel.Format == ObjectFormat::MachO &&
          Rel.Type == llvm::MachO::ARM64_RELOC_PAGEOFF12) {
        if ((Instruction & UINT32_C(0x3B000000)) == UINT32_C(0x39000000)) {
          unsigned Scale = (Instruction >> 30) & 3;
          if (Scale == 0 &&
              (Instruction & UINT32_C(0x04800000)) == UINT32_C(0x04800000))
            Scale = 4;
          Encoding = {static_cast<Low12Scale>(Scale), 0, 0};
        } else {
          Encoding = {Low12Scale::Byte, UINT32_C(0x7FC00000),
                      UINT32_C(0x11000000)};
        }
      } else if (Rel.Format == ObjectFormat::COFF &&
                 (Rel.Type == llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A ||
                  Rel.Type == llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L)) {
        if (Rel.Type == llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A) {
          Encoding = {Low12Scale::Byte, UINT32_C(0x7FC00000),
                      UINT32_C(0x11000000)};
        } else {
          if ((Instruction & UINT32_C(0x3B000000)) != UINT32_C(0x39000000))
            return fail(Rel, "invalid low12 instruction");
          unsigned Scale = (Instruction >> 30) & 3;
          if (Scale == 0 &&
              (Instruction & UINT32_C(0x04800000)) == UINT32_C(0x04800000))
            Scale = 4;
          Encoding = {static_cast<Low12Scale>(Scale), 0, 0};
        }
      } else
        switch (Rel.Type) {
        case llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC:
          Encoding = {Low12Scale::Byte, UINT32_C(0x7FC00000),
                      UINT32_C(0x11000000)};
          break;
        case llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC:
          Encoding = {Low12Scale::Byte, UINT32_C(0x3B000000),
                      UINT32_C(0x39000000)};
          break;
        case llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC:
          Encoding = {Low12Scale::Half, UINT32_C(0x3F000000),
                      UINT32_C(0x39000000)};
          break;
        case llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC:
          Encoding = {Low12Scale::Word, UINT32_C(0x3F000000),
                      UINT32_C(0x39000000)};
          break;
        case llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC:
          Encoding = {Low12Scale::DoubleWord, UINT32_C(0xFB000000),
                      UINT32_C(0xF9000000)};
          break;
        case llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC:
          Encoding = {Low12Scale::QuadWord, UINT32_C(0x3F800000),
                      UINT32_C(0x3D800000)};
          break;
        default:
          return fail(Rel, "unsupported AArch64 relocation type",
                      DiagnosticKind::Unsupported);
        }
      if (Encoding.OpcodeMask != 0 &&
          (Instruction & Encoding.OpcodeMask) != Encoding.Opcode) {
        return fail(Rel, "invalid low12 instruction");
      }
      const uint64_t Low = S & PageOffsetMask;
      if ((Low & ((UINT64_C(1) << value(Encoding.Scale)) - 1)) != 0) {
        return fail(Rel, "unaligned low12 relocation");
      }
      uint64_t Immediate = Low >> value(Encoding.Scale);
      if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF)
        Immediate += (Instruction & Low12ImmediateMask) >> Low12ImmediateShift;
      const uint64_t Maximum = UINT64_C(0xFFF) >> value(Encoding.Scale);
      Instruction =
          (Instruction & ~Low12ImmediateMask) |
          (static_cast<uint32_t>(Immediate & Maximum) << Low12ImmediateShift);
    }
    if (!writeUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little,
                       Instruction)) {
      return fail(Rel, "cannot encode instruction");
    }
  }
  return Result;
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
