// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/pe_writer.h"
#include "linker/relocation.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Support/MathExtras.h>

#include <optional>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {
namespace {

constexpr uint8_t InstructionWidth = 4;
constexpr uint8_t WordWidth = 4;
constexpr uint8_t DoubleWordWidth = 8;
constexpr uint64_t InstructionAlignmentMask = InstructionWidth - 1;
constexpr uint64_t PageOffsetMask = UINT64_C(0xFFF);

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

constexpr uint32_t LoadStoreUnsignedImmediateMask = UINT32_C(0x3B000000);
constexpr uint32_t LoadStoreUnsignedImmediate = UINT32_C(0x39000000);
constexpr Low12Encoding AddImmediateEncoding{
    Low12Scale::Byte, UINT32_C(0x7FC00000), UINT32_C(0x11000000)};

std::optional<Low12Scale> decodeLoadStoreScale(uint32_t Instruction) noexcept {
  if (const auto Scale = aarch64LoadStoreScale(Instruction))
    return static_cast<Low12Scale>(*Scale);
  return std::nullopt;
}

bool isFormatType(const Relocation &Rel, uint32_t ELFType, uint32_t MachOType,
                  uint32_t COFFType) noexcept {
  switch (Rel.Format) {
  case ObjectFormat::ELF:
    return Rel.Type == ELFType;
  case ObjectFormat::MachO:
    return Rel.Type == MachOType;
  case ObjectFormat::COFF:
    return Rel.Type == COFFType;
  }
  return false;
}

LinkExpect<uint32_t> encodeBranch26(const LinkGraph &Graph,
                                    const Relocation &Rel, uint32_t Instruction,
                                    uint64_t S, uint64_t P) {
  constexpr unsigned BranchImmediateBits = 26;
  constexpr unsigned BranchDisplacementBits = 28;
  constexpr unsigned BranchImmediateShift = 2;
  constexpr uint32_t BranchOpcodeMask = UINT32_C(0xFC000000);
  constexpr uint32_t BranchImmediateMask = UINT32_C(0x03FFFFFF);
  constexpr uint32_t JumpOpcode = UINT32_C(0x14000000);
  constexpr uint32_t CallOpcode = UINT32_C(0x94000000);
  const uint32_t Opcode = Instruction & BranchOpcodeMask;
  if (Opcode != CallOpcode && Opcode != JumpOpcode) {
    return relocationError(Graph, Rel, "invalid branch instruction");
  }
  int64_t Value = 0;
  if (!signedDelta(S, P, Value))
    return relocationError(Graph, Rel, "branch displacement overflows");
  if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF &&
      !addSigned(Value,
                 llvm::SignExtend64<BranchImmediateBits>(Instruction &
                                                         BranchImmediateMask)
                     << BranchImmediateShift,
                 Value))
    return relocationError(Graph, Rel, "branch displacement overflows");
  if ((Value & static_cast<int64_t>(InstructionAlignmentMask)) != 0 ||
      !llvm::isInt<BranchDisplacementBits>(Value)) {
    return relocationError(Graph, Rel, "branch displacement overflows");
  }
  return Opcode | (static_cast<uint32_t>(Value >> BranchImmediateShift) &
                   BranchImmediateMask);
}

LinkExpect<uint32_t> encodePage21(const LinkGraph &Graph, const Relocation &Rel,
                                  uint32_t Instruction, uint64_t S,
                                  uint64_t P) {
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
    return relocationError(Graph, Rel, "invalid ADRP instruction");
  }
  uint64_t Target = S;
  if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF) {
    const uint32_t Raw =
        ((Instruction >> AdrpImmediateLowShift) & AdrpImmediateLowMask) |
        ((Instruction >> AdrpImmediateHighShift) & AdrpImmediateHighMask);
    if (!addSigned(Target, llvm::SignExtend64<PageDisplacementBits>(Raw),
                   Target))
      return relocationError(Graph, Rel, "ADRP page displacement overflows");
  }
  int64_t Pages = 0;
  if (!signedDelta(Target >> PageShift, P >> PageShift, Pages) ||
      !llvm::isInt<PageDisplacementBits>(Pages)) {
    return relocationError(Graph, Rel, "ADRP page displacement overflows");
  }
  const uint32_t Value = static_cast<uint32_t>(Pages) & AdrpImmediateMask;
  return (Instruction & AdrpPreservedMask) |
         ((Value & AdrpImmediateLowMask) << AdrpImmediateLowShift) |
         ((Value & AdrpImmediateHighMask) << AdrpImmediateHighShift);
}

LinkExpect<Low12Encoding> low12Encoding(const LinkGraph &Graph,
                                        const Relocation &Rel,
                                        uint32_t Instruction) {
  if (Rel.Format == ObjectFormat::MachO &&
      Rel.Type == llvm::MachO::ARM64_RELOC_PAGEOFF12) {
    if (const auto Scale = decodeLoadStoreScale(Instruction))
      return Low12Encoding{*Scale, 0, 0};
    return AddImmediateEncoding;
  }
  if (Rel.Format == ObjectFormat::COFF) {
    if (Rel.Type == llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A)
      return AddImmediateEncoding;
    if (Rel.Type == llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L) {
      const auto Scale = decodeLoadStoreScale(Instruction);
      if (!Scale)
        return relocationError(Graph, Rel, "invalid low12 instruction");
      return Low12Encoding{*Scale, 0, 0};
    }
  }
  switch (Rel.Type) {
  case llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC:
    return AddImmediateEncoding;
  case llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC:
    return Low12Encoding{Low12Scale::Byte, LoadStoreUnsignedImmediateMask,
                         LoadStoreUnsignedImmediate};
  case llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC:
    return Low12Encoding{Low12Scale::Half, UINT32_C(0xFB000000),
                         UINT32_C(0x79000000)};
  case llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC:
    return Low12Encoding{Low12Scale::Word, UINT32_C(0xFB000000),
                         UINT32_C(0xB9000000)};
  case llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC:
    return Low12Encoding{Low12Scale::DoubleWord, UINT32_C(0xFB000000),
                         UINT32_C(0xF9000000)};
  case llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC:
    return Low12Encoding{Low12Scale::QuadWord, UINT32_C(0x3F800000),
                         UINT32_C(0x3D800000)};
  default:
    return relocationError(Graph, Rel, "unsupported AArch64 relocation type",
                           DiagnosticKind::Unsupported);
  }
}

LinkExpect<uint32_t> encodeLow12(const LinkGraph &Graph, const Relocation &Rel,
                                 uint32_t Instruction, uint64_t S) {
  constexpr uint32_t Low12ImmediateMask = UINT32_C(0x003FFC00);
  constexpr unsigned Low12ImmediateShift = 10;
  EXPECTED_TRY(const auto Encoding, low12Encoding(Graph, Rel, Instruction));
  if (Encoding.OpcodeMask != 0 &&
      (Instruction & Encoding.OpcodeMask) != Encoding.Opcode) {
    return relocationError(Graph, Rel, "invalid low12 instruction");
  }
  const uint64_t Low = S & PageOffsetMask;
  if ((Low & ((UINT64_C(1) << value(Encoding.Scale)) - 1)) != 0) {
    return relocationError(Graph, Rel, "unaligned low12 relocation");
  }
  uint64_t Immediate = Low >> value(Encoding.Scale);
  if (Rel.AddendIsImplicit && Rel.Format == ObjectFormat::COFF)
    Immediate += (Instruction & Low12ImmediateMask) >> Low12ImmediateShift;
  const uint64_t Maximum = PageOffsetMask >> value(Encoding.Scale);
  return (Instruction & ~Low12ImmediateMask) |
         (static_cast<uint32_t>(Immediate & Maximum) << Low12ImmediateShift);
}

} // namespace

LinkExpect<RelocationResult> applyAArch64(const LinkGraph &Graph) {
  RelocationWriter Writer(Graph);
  for (const auto &Rel : Graph.relocations()) {
    auto &Bytes = Writer.content(Rel.Section);
    int64_t Addend = Rel.Addend;
    const bool Absolute64 = isFormatType(Rel, llvm::ELF::R_AARCH64_ABS64,
                                         llvm::MachO::ARM64_RELOC_UNSIGNED,
                                         llvm::COFF::IMAGE_REL_ARM64_ADDR64);
    const bool ImageRelative32 =
        Rel.Format == ObjectFormat::COFF &&
        Rel.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR32NB;
    if (Rel.AddendIsImplicit && Absolute64 && Rel.Format != ObjectFormat::ELF) {
      auto Value =
          readSigned(Bytes, Rel.Offset, DoubleWordWidth, Endianness::Little);
      if (!Value)
        return relocationError(Graph, Rel, "cannot decode implicit addend");
      Addend = *Value;
    }
    if (Rel.AddendIsImplicit && ImageRelative32) {
      auto Value =
          readUnsigned(Bytes, Rel.Offset, WordWidth, Endianness::Little);
      if (!Value || *Value > INT64_MAX ||
          Addend > INT64_MAX - static_cast<int64_t>(*Value))
        return relocationError(Graph, Rel, "cannot decode implicit addend");
      Addend += static_cast<int64_t>(*Value);
    }
    EXPECTED_TRY(uint64_t S, symbolAddress(Graph, Rel));
    if (!addSigned(S, Addend, S))
      return relocationError(Graph, Rel, "relocation address overflows");
    EXPECTED_TRY(const uint64_t P, placeAddress(Graph, Rel));
    if (ImageRelative32) {
      if (S < PEImageBase || S - PEImageBase > UINT32_MAX ||
          !writeUnsigned(Bytes, Rel.Offset, WordWidth, Endianness::Little,
                         S - PEImageBase)) {
        return relocationError(Graph, Rel, "ADDR32NB relocation overflows");
      }
      continue;
    }
    if (Absolute64) {
      EXPECTED_TRY(Writer.writeAbsolute(Rel, DoubleWordWidth,
                                        Endianness::Little, S, Addend));
      continue;
    }
    if (Rel.Format == ObjectFormat::ELF &&
        Rel.Type == llvm::ELF::R_AARCH64_PREL64) {
      int64_t Value = 0;
      if (!signedDelta(S, P, Value) ||
          !writeSigned(Bytes, Rel.Offset, DoubleWordWidth, Endianness::Little,
                       Value)) {
        return relocationError(Graph, Rel, "PREL64 relocation overflows");
      }
      continue;
    }
    if (Rel.Format == ObjectFormat::ELF &&
        Rel.Type == llvm::ELF::R_AARCH64_PREL32) {
      int64_t Value = 0;
      if (!signedDelta(S, P, Value) || !llvm::isInt<32>(Value) ||
          !writeSigned(Bytes, Rel.Offset, WordWidth, Endianness::Little,
                       Value)) {
        return relocationError(Graph, Rel, "PREL32 relocation overflows");
      }
      continue;
    }
    if ((Rel.Offset & InstructionAlignmentMask) != 0) {
      return relocationError(Graph, Rel, "invalid relocation field");
    }
    auto Word =
        readUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little);
    if (!Word) {
      return relocationError(Graph, Rel,
                             "relocation field exceeds section content");
    }
    const uint32_t Original = static_cast<uint32_t>(*Word);
    const bool Branch26 = (Rel.Format == ObjectFormat::ELF &&
                           Rel.Type == llvm::ELF::R_AARCH64_JUMP26) ||
                          isFormatType(Rel, llvm::ELF::R_AARCH64_CALL26,
                                       llvm::MachO::ARM64_RELOC_BRANCH26,
                                       llvm::COFF::IMAGE_REL_ARM64_BRANCH26);
    const bool Page21 =
        isFormatType(Rel, llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
                     llvm::MachO::ARM64_RELOC_PAGE21,
                     llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21);
    uint32_t Instruction = 0;
    if (Branch26) {
      EXPECTED_TRY(Instruction, encodeBranch26(Graph, Rel, Original, S, P));
    } else if (Page21) {
      EXPECTED_TRY(Instruction, encodePage21(Graph, Rel, Original, S, P));
    } else {
      EXPECTED_TRY(Instruction, encodeLow12(Graph, Rel, Original, S));
    }
    if (!writeUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little,
                       Instruction)) {
      return relocationError(Graph, Rel, "cannot encode instruction");
    }
  }
  return std::move(Writer).finish();
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
