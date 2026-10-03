// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Support/MathExtras.h>

#include <map>
#include <utility>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {
namespace {

constexpr uint8_t InstructionWidth = 4;
constexpr uint8_t WordWidth = 4;
constexpr uint8_t DoubleWordWidth = 8;
constexpr unsigned BitsPerByte = 8;
constexpr uint64_t InstructionAlignmentMask = 1;
constexpr unsigned ImmediateBits = 12;
constexpr unsigned ImmediateShift = 12;
constexpr unsigned UpperImmediateBits = 20;
constexpr int64_t ImmediateRoundingBias = INT64_C(1) << 11;
constexpr int64_t ImmediateScale = INT64_C(1) << ImmediateShift;
constexpr uint32_t OpcodeMask = UINT32_C(0x7F);
constexpr uint32_t AuipcOpcode = UINT32_C(0x17);
constexpr uint32_t JalrOpcode = UINT32_C(0x67);
constexpr uint32_t UpperImmediatePreservedMask = UINT32_C(0xFFF);
constexpr unsigned IImmediateShift = 20;
constexpr uint32_t IImmediatePreservedMask = UINT32_C(0x000FFFFF);
constexpr uint32_t ImmediateMask = UINT32_C(0xFFF);

bool roundedHigh(int64_t Delta, int64_t &Result) noexcept {
  if (!addSigned(Delta, ImmediateRoundingBias, Result))
    return false;
  Result >>= ImmediateShift;
  return true;
}

} // namespace

LinkExpect<RelocationResult> applyRISCV(const LinkGraph &Graph) {
  // Relocation formulas follow the RISC-V ELF psABI, Relocations chapter.
  RelocationWriter Writer(Graph);
  std::map<std::pair<SectionId, uint64_t>, int64_t> PcrelHi20Deltas;
  std::map<std::pair<SectionId, uint64_t>,
           std::pair<const Relocation *, const Relocation *>>
      SymbolDifferences;
  for (const auto &Rel : Graph.relocations()) {
    if (Rel.Type == llvm::ELF::R_RISCV_ADD8 ||
        Rel.Type == llvm::ELF::R_RISCV_SET8 ||
        Rel.Type == llvm::ELF::R_RISCV_SUB8 ||
        Rel.Type == llvm::ELF::R_RISCV_ADD32 ||
        Rel.Type == llvm::ELF::R_RISCV_SUB32) {
      auto &Pair = SymbolDifferences[{Rel.Section, Rel.Offset}];
      (Rel.Type == llvm::ELF::R_RISCV_SUB8 ||
               Rel.Type == llvm::ELF::R_RISCV_SUB32
           ? Pair.second
           : Pair.first) = &Rel;
    }
    if (Rel.Format != ObjectFormat::ELF ||
        Rel.Type != llvm::ELF::R_RISCV_PCREL_HI20) {
      continue;
    }
    if ((Rel.Offset & InstructionAlignmentMask) != 0) {
      return relocationError(Graph, Rel, "invalid relocation field");
    }
    const auto &Bytes = Graph.sections()[Rel.Section].Content;
    const auto Word =
        readUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little);
    if (!Word || (*Word & OpcodeMask) != AuipcOpcode) {
      return relocationError(Graph, Rel, "invalid PCREL_HI20 instruction");
    }
    EXPECTED_TRY(uint64_t S, symbolAddress(Graph, Rel));
    EXPECTED_TRY(const uint64_t P, placeAddress(Graph, Rel));
    int64_t Delta = 0;
    // HI20 is (value + 0x800) >> 12 so LO12 is the signed residual.
    int64_t High = 0;
    if (!addSigned(S, Rel.Addend, S) || !signedDelta(S, P, Delta) ||
        !roundedHigh(Delta, High) || !llvm::isIntN(UpperImmediateBits, High) ||
        !PcrelHi20Deltas.emplace(std::make_pair(Rel.Section, Rel.Offset), Delta)
             .second) {
      return relocationError(Graph, Rel, "invalid PCREL_HI20 relocation");
    }
  }
  for (const auto &Rel : Graph.relocations()) {
    auto &Bytes = Writer.content(Rel.Section);
    if (Rel.Format != ObjectFormat::ELF) {
      return relocationError(Graph, Rel, "unsupported object format",
                             DiagnosticKind::Unsupported);
    }
    if (Rel.Type == llvm::ELF::R_RISCV_RELAX ||
        Rel.Type == llvm::ELF::R_RISCV_SUB8 ||
        Rel.Type == llvm::ELF::R_RISCV_SUB32) {
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_ADD8 ||
        Rel.Type == llvm::ELF::R_RISCV_SET8 ||
        Rel.Type == llvm::ELF::R_RISCV_ADD32) {
      const auto Pair = SymbolDifferences.find({Rel.Section, Rel.Offset});
      if (Pair == SymbolDifferences.end() || Pair->second.first == nullptr ||
          Pair->second.second == nullptr) {
        return relocationError(Graph, Rel,
                               "unpaired symbol difference relocation");
      }
      const auto &AddRel = *Pair->second.first;
      const auto &SubRel = *Pair->second.second;
      const uint8_t Width = Rel.PatchSize;
      const auto Original =
          readUnsigned(Bytes, Rel.Offset, Width, Endianness::Little);
      const auto AddAddress = symbolAddress(Graph, AddRel);
      const auto SubAddress = symbolAddress(Graph, SubRel);
      if (!Original || !AddAddress || !SubAddress) {
        return relocationError(Graph, Rel,
                               "invalid symbol difference relocation");
      }
      uint64_t Value = Rel.Type == llvm::ELF::R_RISCV_SET8 ? 0 : *Original;
      Value += *AddAddress + static_cast<uint64_t>(AddRel.Addend);
      Value -= *SubAddress + static_cast<uint64_t>(SubRel.Addend);
      if (Width < sizeof(Value))
        Value &= (UINT64_C(1) << (Width * BitsPerByte)) - 1;
      if (!writeUnsigned(Bytes, Rel.Offset, Width, Endianness::Little, Value)) {
        return relocationError(Graph, Rel, "cannot encode symbol difference");
      }
      continue;
    }
    EXPECTED_TRY(uint64_t S, symbolAddress(Graph, Rel));
    if (!addSigned(S, Rel.Addend, S)) {
      return relocationError(Graph, Rel, "relocation address overflows");
    }
    EXPECTED_TRY(const uint64_t P, placeAddress(Graph, Rel));
    if (Rel.Type == llvm::ELF::R_RISCV_64) {
      EXPECTED_TRY(Writer.writeAbsolute(Rel, DoubleWordWidth,
                                        Endianness::Little, S, Rel.Addend));
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_32_PCREL) {
      int64_t Value = 0;
      if (!signedDelta(S, P, Value) || !llvm::isInt<32>(Value) ||
          !writeSigned(Bytes, Rel.Offset, WordWidth, Endianness::Little,
                       Value)) {
        return relocationError(Graph, Rel, "32_PCREL relocation overflows");
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
    uint32_t Instruction = static_cast<uint32_t>(*Word);
    if (Rel.Type == llvm::ELF::R_RISCV_CALL ||
        Rel.Type == llvm::ELF::R_RISCV_CALL_PLT) {
      constexpr uint32_t JalrOpcodeMask = UINT32_C(0x707F);
      auto Second = readUnsigned(Bytes, Rel.Offset + InstructionWidth,
                                 InstructionWidth, Endianness::Little);
      if ((Instruction & OpcodeMask) != AuipcOpcode || !Second ||
          (*Second & JalrOpcodeMask) != JalrOpcode) {
        return relocationError(Graph, Rel, "invalid call instruction pair");
      }
      int64_t Delta = 0;
      int64_t High = 0;
      if (!signedDelta(S, P, Delta) || !roundedHigh(Delta, High) ||
          !llvm::isIntN(UpperImmediateBits, High)) {
        return relocationError(Graph, Rel, "call displacement overflows");
      }
      const int64_t Low = Delta - High * ImmediateScale;
      if (!llvm::isIntN(ImmediateBits, Low)) {
        return relocationError(Graph, Rel, "call displacement overflows");
      }
      Instruction = (Instruction & UpperImmediatePreservedMask) |
                    (static_cast<uint32_t>(High) << ImmediateShift);
      const uint32_t Jalr =
          (static_cast<uint32_t>(*Second) & IImmediatePreservedMask) |
          ((static_cast<uint32_t>(Low) & ImmediateMask) << IImmediateShift);
      if (!writeUnsigned(Bytes, Rel.Offset, InstructionWidth,
                         Endianness::Little, Instruction) ||
          !writeUnsigned(Bytes, Rel.Offset + InstructionWidth, InstructionWidth,
                         Endianness::Little, Jalr)) {
        return relocationError(Graph, Rel, "cannot encode call pair");
      }
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_PCREL_HI20) {
      const auto PcrelHi20Delta =
          PcrelHi20Deltas.find({Rel.Section, Rel.Offset});
      if (PcrelHi20Delta == PcrelHi20Deltas.end()) {
        return relocationError(Graph, Rel,
                               "missing precomputed PCREL_HI20 relocation");
      }
      int64_t High = 0;
      if (!roundedHigh(PcrelHi20Delta->second, High)) {
        return relocationError(Graph, Rel, "invalid PCREL_HI20 relocation");
      }
      Instruction = (Instruction & UpperImmediatePreservedMask) |
                    (static_cast<uint32_t>(High) << ImmediateShift);
    } else if (Rel.Type == llvm::ELF::R_RISCV_PCREL_LO12_I ||
               Rel.Type == llvm::ELF::R_RISCV_PCREL_LO12_S) {
      constexpr unsigned SImmediateLowShift = 7;
      constexpr unsigned SImmediateHighShift = 20;
      constexpr uint32_t OpImmOpcode = UINT32_C(0x13);
      constexpr uint32_t LoadOpcode = UINT32_C(0x03);
      constexpr uint32_t LoadFpOpcode = UINT32_C(0x07);
      constexpr uint32_t StoreOpcode = UINT32_C(0x23);
      constexpr uint32_t StoreFpOpcode = UINT32_C(0x27);
      constexpr uint32_t SImmediatePreservedMask = UINT32_C(0x01FFF07F);
      constexpr uint32_t SImmediateLowMask = UINT32_C(0x1F);
      constexpr uint32_t SImmediateHighMask = UINT32_C(0xFE0);
      const auto &HighSymbol = Graph.symbols()[Rel.Symbol];
      const auto PcrelHi20Delta =
          PcrelHi20Deltas.find({HighSymbol.Section, HighSymbol.Offset});
      int64_t Rounded = 0;
      if (PcrelHi20Delta == PcrelHi20Deltas.end() ||
          !roundedHigh(PcrelHi20Delta->second, Rounded)) {
        return relocationError(Graph, Rel, "missing PCREL_HI20 pair");
      }
      const uint32_t Low = static_cast<uint32_t>(PcrelHi20Delta->second -
                                                 Rounded * ImmediateScale) &
                           ImmediateMask;
      const uint32_t Opcode = Instruction & OpcodeMask;
      if (Rel.Type == llvm::ELF::R_RISCV_PCREL_LO12_I) {
        if (Opcode != OpImmOpcode && Opcode != LoadOpcode &&
            Opcode != LoadFpOpcode && Opcode != JalrOpcode) {
          return relocationError(Graph, Rel,
                                 "invalid PCREL_LO12_I instruction");
        }
        Instruction =
            (Instruction & IImmediatePreservedMask) | (Low << IImmediateShift);
      } else {
        if (Opcode != StoreOpcode && Opcode != StoreFpOpcode) {
          return relocationError(Graph, Rel,
                                 "invalid PCREL_LO12_S instruction");
        }
        Instruction = (Instruction & SImmediatePreservedMask) |
                      ((Low & SImmediateLowMask) << SImmediateLowShift) |
                      ((Low & SImmediateHighMask) << SImmediateHighShift);
      }
    } else {
      return relocationError(Graph, Rel, "unsupported RISC-V relocation type",
                             DiagnosticKind::Unsupported);
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
