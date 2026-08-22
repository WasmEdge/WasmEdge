// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Support/MathExtras.h>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

LinkExpect<RelocationResult> applyARM(const LinkGraph &Graph) {
  constexpr uint8_t InstructionWidth = 4;
  constexpr uint8_t WordWidth = 4;
  constexpr unsigned HalfwordBits = 16;
  constexpr uint64_t InstructionAlignmentMask = InstructionWidth - 1;
  constexpr uint64_t ThumbAlignmentMask = 1;
  RelocationWriter Writer(Graph);
  for (const auto &Rel : Graph.relocations()) {
    if (Rel.Format != ObjectFormat::ELF) {
      return relocationError(Graph, Rel, "unsupported object format",
                             DiagnosticKind::Unsupported);
    }
    if (Rel.Type == llvm::ELF::R_ARM_NONE) {
      continue;
    }
    auto &Bytes = Writer.content(Rel.Section);
    auto Word =
        readUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little);
    const bool ThumbBranch = Rel.Type == llvm::ELF::R_ARM_THM_CALL;
    const bool DataRelocation = Rel.Type == llvm::ELF::R_ARM_ABS32 ||
                                Rel.Type == llvm::ELF::R_ARM_REL32;
    const uint64_t AlignmentMask = DataRelocation ? UINT64_C(0)
                                   : ThumbBranch  ? ThumbAlignmentMask
                                                  : InstructionAlignmentMask;
    if (!Word || (Rel.Offset & AlignmentMask) != 0) {
      return relocationError(Graph, Rel, "invalid relocation field");
    }
    const auto &Symbol = Graph.symbols()[Rel.Symbol];
    EXPECTED_TRY(const uint64_t S, symbolAddress(Graph, Rel));
    EXPECTED_TRY(const uint64_t P, placeAddress(Graph, Rel));
    const uint64_t T = Symbol.Thumb ? UINT64_C(1) : UINT64_C(0);
    int64_t Addend = Rel.Addend;
    if (Rel.AddendIsImplicit && DataRelocation) {
      auto Value = readSigned(Bytes, Rel.Offset, WordWidth, Endianness::Little);
      if (!Value) {
        return relocationError(Graph, Rel, "cannot decode implicit addend");
      }
      Addend = *Value;
    }
    if (Rel.Type == llvm::ELF::R_ARM_ABS32) {
      uint64_t Value = 0;
      if (!addSigned(S, Addend, Value)) {
        return relocationError(Graph, Rel, "absolute relocation overflows");
      }
      EXPECTED_TRY(Writer.writeAbsolute(Rel, WordWidth, Endianness::Little,
                                        Value | T, Addend));
    } else if (Rel.Type == llvm::ELF::R_ARM_REL32) {
      uint64_t Target = 0;
      int64_t Value = 0;
      if (!addSigned(S, Addend, Target)) {
        return relocationError(Graph, Rel, "relative relocation overflows");
      }
      Target |= T;
      if (!signedDelta(Target, P, Value) || !llvm::isInt<32>(Value) ||
          !writeSigned(Bytes, Rel.Offset, WordWidth, Endianness::Little,
                       Value)) {
        return relocationError(Graph, Rel, "relative relocation overflows");
      }
    } else if (ThumbBranch) {
      constexpr unsigned BranchDisplacementBits = 25;
      constexpr unsigned BranchImmediateShift = 1;
      constexpr uint16_t FirstOpcodeMask = UINT16_C(0xF800);
      constexpr uint16_t FirstOpcode = UINT16_C(0xF000);
      constexpr uint16_t SecondBranchOpcodeMask = UINT16_C(0xD000);
      constexpr uint16_t SecondCallOpcode = UINT16_C(0xD000);
      constexpr uint16_t SecondExchangeOpcode = UINT16_C(0xC000);
      constexpr uint16_t SignMask = UINT16_C(0x0400);
      constexpr uint16_t Imm10Mask = UINT16_C(0x03FF);
      constexpr uint16_t J1Mask = UINT16_C(0x2000);
      constexpr uint16_t J2Mask = UINT16_C(0x0800);
      constexpr uint16_t Imm11Mask = UINT16_C(0x07FF);
      constexpr uint16_t ExchangeImm10LMask = UINT16_C(0x07FE);
      constexpr uint64_t ExchangeAlignmentMask = 3;
      constexpr unsigned SignShift = 24;
      constexpr unsigned I1Shift = 23;
      constexpr unsigned I2Shift = 22;
      constexpr unsigned Imm10Shift = 12;
      constexpr unsigned J1Shift = 13;
      constexpr unsigned J2Shift = 11;
      const uint16_t First = static_cast<uint16_t>(*Word);
      const uint16_t Second = static_cast<uint16_t>(*Word >> HalfwordBits);
      if ((First & FirstOpcodeMask) != FirstOpcode ||
          ((Second & SecondBranchOpcodeMask) != SecondCallOpcode &&
           (Second & SecondBranchOpcodeMask) != SecondExchangeOpcode)) {
        return relocationError(Graph, Rel, "invalid Thumb branch instruction");
      }
      if (Rel.AddendIsImplicit) {
        const uint32_t SBit = (First & SignMask) != 0;
        const uint32_t J1 = (Second & J1Mask) != 0;
        const uint32_t J2 = (Second & J2Mask) != 0;
        const uint32_t I1 = !(J1 ^ SBit);
        const uint32_t I2 = !(J2 ^ SBit);
        const uint32_t Encoded =
            (SBit << SignShift) | (I1 << I1Shift) | (I2 << I2Shift) |
            (static_cast<uint32_t>(First & Imm10Mask) << Imm10Shift) |
            (static_cast<uint32_t>(Second & Imm11Mask) << BranchImmediateShift);
        Addend = llvm::SignExtend64<BranchDisplacementBits>(Encoded);
      }
      const bool CrossState = !Symbol.Thumb;
      const uint64_t BranchP = CrossState ? (P & ~ExchangeAlignmentMask) : P;
      int64_t Value = 0;
      if (!signedDelta(S, BranchP, Value) || !addSigned(Value, Addend, Value)) {
        return relocationError(Graph, Rel,
                               "Thumb branch displacement overflows");
      }
      const uint64_t ValueAlignmentMask =
          CrossState ? ExchangeAlignmentMask : ThumbAlignmentMask;
      if ((static_cast<uint64_t>(Value) & ValueAlignmentMask) != 0 ||
          !llvm::isInt<BranchDisplacementBits>(Value)) {
        return relocationError(Graph, Rel,
                               "Thumb branch displacement overflows");
      }
      const uint32_t Encoded = static_cast<uint32_t>(Value);
      const uint32_t SBit = (Encoded >> SignShift) & 1;
      const uint32_t I1 = (Encoded >> I1Shift) & 1;
      const uint32_t I2 = (Encoded >> I2Shift) & 1;
      const uint32_t J1 = !(I1 ^ SBit);
      const uint32_t J2 = !(I2 ^ SBit);
      const uint16_t EncodedFirst = static_cast<uint16_t>(
          (First & FirstOpcodeMask) | (SBit ? SignMask : 0) |
          ((Encoded >> Imm10Shift) & Imm10Mask));
      const uint16_t EncodedSecond =
          CrossState
              ? static_cast<uint16_t>(
                    SecondExchangeOpcode | (J1 << J1Shift) | (J2 << J2Shift) |
                    ((Encoded >> BranchImmediateShift) & ExchangeImm10LMask))
              : static_cast<uint16_t>(
                    SecondCallOpcode | (J1 << J1Shift) | (J2 << J2Shift) |
                    ((Encoded >> BranchImmediateShift) & Imm11Mask));
      if (!writeUnsigned(
              Bytes, Rel.Offset, InstructionWidth, Endianness::Little,
              static_cast<uint32_t>(EncodedFirst) |
                  (static_cast<uint32_t>(EncodedSecond) << HalfwordBits))) {
        return relocationError(Graph, Rel, "cannot encode Thumb branch");
      }
    } else if (Rel.Type == llvm::ELF::R_ARM_CALL ||
               Rel.Type == llvm::ELF::R_ARM_JUMP24) {
      constexpr unsigned BranchImmediateBits = 24;
      constexpr unsigned BranchDisplacementBits = 26;
      constexpr unsigned BranchImmediateShift = 2;
      constexpr uint32_t BranchOpcodeMask = UINT32_C(0x0E000000);
      constexpr uint32_t BranchOpcode = UINT32_C(0x0A000000);
      constexpr uint32_t BranchImmediateMask = UINT32_C(0x00FFFFFF);
      constexpr uint32_t BranchConditionMask = UINT32_C(0xF0000000);
      constexpr uint32_t ConditionAlways = UINT32_C(0xE0000000);
      constexpr uint32_t CallOpcode = UINT32_C(0x0B000000);
      constexpr uint32_t JumpOpcode = UINT32_C(0x0A000000);
      constexpr uint32_t ExchangeOpcodeMask = UINT32_C(0xFE000000);
      constexpr uint32_t ExchangeOpcode = UINT32_C(0xFA000000);
      constexpr uint32_t ExchangeHBit = UINT32_C(0x01000000);
      constexpr uint32_t HalfwordOffsetBit = 2;
      constexpr unsigned HalfwordOffsetToHBitShift = 23;
      constexpr int64_t ExchangeHalfwordOffset = 2;
      const uint32_t Original = static_cast<uint32_t>(*Word);
      if ((Original & BranchOpcodeMask) != BranchOpcode) {
        return relocationError(Graph, Rel, "invalid ARM branch instruction");
      }
      const bool InputExchange =
          (Original & ExchangeOpcodeMask) == ExchangeOpcode;
      if (Rel.AddendIsImplicit) {
        Addend = llvm::SignExtend64<BranchImmediateBits>(Original &
                                                         BranchImmediateMask) *
                     InstructionWidth +
                 (InputExchange && (Original & ExchangeHBit) != 0
                      ? ExchangeHalfwordOffset
                      : 0);
      }
      if (Symbol.Thumb && Rel.Type == llvm::ELF::R_ARM_JUMP24)
        return relocationError(Graph, Rel, "unsupported ARM-to-Thumb jump",
                               DiagnosticKind::Unsupported);
      int64_t Value = 0;
      if (!signedDelta(S, P, Value) || !addSigned(Value, Addend, Value)) {
        return relocationError(Graph, Rel, "ARM branch displacement overflows");
      }
      const bool CrossState = Symbol.Thumb;
      const uint64_t ValueAlignmentMask =
          CrossState ? ThumbAlignmentMask : InstructionAlignmentMask;
      if ((static_cast<uint64_t>(Value) & ValueAlignmentMask) != 0 ||
          !llvm::isInt<BranchDisplacementBits>(Value)) {
        return relocationError(Graph, Rel, "ARM branch displacement overflows");
      }
      const uint32_t Encoded =
          static_cast<uint32_t>(Value >> BranchImmediateShift) &
          BranchImmediateMask;
      const uint32_t Condition =
          InputExchange ? ConditionAlways : Original & BranchConditionMask;
      const uint32_t Instruction =
          CrossState ? ExchangeOpcode |
                           ((static_cast<uint32_t>(Value) & HalfwordOffsetBit)
                            << HalfwordOffsetToHBitShift) |
                           Encoded
                     : Condition |
                           (Rel.Type == llvm::ELF::R_ARM_CALL ? CallOpcode
                                                              : JumpOpcode) |
                           Encoded;
      if (!writeUnsigned(Bytes, Rel.Offset, InstructionWidth,
                         Endianness::Little, Instruction)) {
        return relocationError(Graph, Rel, "cannot encode ARM branch");
      }
    } else if (Rel.Type == llvm::ELF::R_ARM_PREL31) {
      constexpr unsigned Prel31Bits = 31;
      constexpr uint32_t Prel31ValueMask = UINT32_C(0x7FFFFFFF);
      constexpr uint32_t Prel31InstructionBit = UINT32_C(0x80000000);
      int64_t Prel31Addend = Rel.Addend;
      if (Rel.AddendIsImplicit) {
        Prel31Addend = llvm::SignExtend64<Prel31Bits>(
            static_cast<uint32_t>(*Word) & Prel31ValueMask);
      }
      uint64_t Target = 0;
      int64_t Value = 0;
      if (!addSigned(S, Prel31Addend, Target) ||
          !signedDelta(Target, P, Value)) {
        return relocationError(Graph, Rel, "PREL31 relocation overflows");
      }
      if (!llvm::isInt<Prel31Bits>(Value) ||
          !writeUnsigned(
              Bytes, Rel.Offset, WordWidth, Endianness::Little,
              (*Word & Prel31InstructionBit) |
                  (static_cast<uint32_t>(Value) & Prel31ValueMask))) {
        return relocationError(Graph, Rel, "PREL31 relocation overflows");
      }
    } else {
      return relocationError(Graph, Rel, "unsupported ARM relocation type",
                             DiagnosticKind::Unsupported);
    }
  }
  return std::move(Writer).finish();
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
