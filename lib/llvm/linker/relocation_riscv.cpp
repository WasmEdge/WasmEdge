// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <llvm/BinaryFormat/ELF.h>

#include <map>
#include <string_view>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {
namespace {

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

bool signedBits(int128_t Value, unsigned Bits) {
  const int128_t Limit = int128_t(1) << (Bits - 1);
  return Value >= -Limit && Value < Limit;
}

} // namespace

LinkExpect<RelocationResult> applyRISCV(const LinkGraph &Graph) {
  // Relocation formulas follow the RISC-V ELF psABI, Relocations chapter.
  constexpr uint8_t InstructionWidth = 4;
  constexpr uint64_t InstructionAlignmentMask = InstructionWidth - 1;
  constexpr unsigned WordBits = 32;
  constexpr unsigned ImmediateShift = 12;
  constexpr int128_t ImmediateRoundingBias = int128_t(1) << 11;
  constexpr int128_t ImmediateScale = int128_t(1) << ImmediateShift;
  constexpr uint32_t OpcodeMask = UINT32_C(0x7F);
  constexpr uint32_t AuipcOpcode = UINT32_C(0x17);
  constexpr uint32_t UpperImmediatePreservedMask = UINT32_C(0xFFF);
  RelocationResult Result;
  Result.Content.reserve(Graph.sections().size());
  for (const auto &Section : Graph.sections()) {
    Result.Content.push_back(Section.Content);
  }
  Result.Rebases = Graph.rebases();
  RebaseIntervalIndex RebaseIntervals(Result.Rebases);
  std::map<std::pair<SectionId, uint64_t>, int128_t> HighValues;
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
      return fail(Rel, "invalid relocation field");
    }
    const auto &Bytes = Graph.sections()[Rel.Section].Content;
    const auto Word =
        readUnsigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little);
    if (!Word || (*Word & OpcodeMask) != AuipcOpcode) {
      return fail(Rel, "invalid PCREL_HI20 instruction");
    }
    const auto &Symbol = Graph.symbols()[Rel.Symbol];
    const int128_t S = int128_t(Graph.sections()[Symbol.Section].Address) +
                       Symbol.Offset + Rel.Addend;
    const int128_t P =
        int128_t(Graph.sections()[Rel.Section].Address) + Rel.Offset;
    const int128_t Delta = S - P;
    // HI20 is (value + 0x800) >> 12 so LO12 is the signed residual.
    const int128_t High = (Delta + ImmediateRoundingBias) >> ImmediateShift;
    if (!signedBits(High, 20) ||
        !HighValues.emplace(std::make_pair(Rel.Section, Rel.Offset), Delta)
             .second) {
      return fail(Rel, "invalid PCREL_HI20 relocation");
    }
  }
  for (const auto &Rel : Graph.relocations()) {
    auto &Bytes = Result.Content[Rel.Section];
    if (Rel.Format != ObjectFormat::ELF) {
      return fail(Rel, "invalid relocation field");
    }
    if (Rel.Type == llvm::ELF::R_RISCV_RELAX) {
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_SUB8 ||
        Rel.Type == llvm::ELF::R_RISCV_SUB32) {
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_ADD8 ||
        Rel.Type == llvm::ELF::R_RISCV_SET8 ||
        Rel.Type == llvm::ELF::R_RISCV_ADD32) {
      const auto Pair = SymbolDifferences.find({Rel.Section, Rel.Offset});
      if (Pair == SymbolDifferences.end() || Pair->second.first == nullptr ||
          Pair->second.second == nullptr) {
        return fail(Rel, "unpaired symbol difference relocation");
      }
      const auto Address = [&](const Relocation &Value,
                               uint64_t &Output) noexcept {
        const auto &Target = Graph.symbols()[Value.Symbol];
        const uint64_t Base = Graph.sections()[Target.Section].Address;
        if (Target.Offset > UINT64_MAX - Base)
          return false;
        Output = Base + Target.Offset;
        return true;
      };
      uint64_t AddAddress = 0;
      uint64_t SubAddress = 0;
      const uint8_t Width = Rel.PatchSize;
      const auto Original =
          readUnsigned(Bytes, Rel.Offset, Width, Endianness::Little);
      if (!Original || !Address(*Pair->second.first, AddAddress) ||
          !Address(*Pair->second.second, SubAddress)) {
        return fail(Rel, "invalid symbol difference relocation");
      }
      uint64_t Value = Rel.Type == llvm::ELF::R_RISCV_SET8 ? 0 : *Original;
      Value += AddAddress + static_cast<uint64_t>(Pair->second.first->Addend);
      Value -= SubAddress + static_cast<uint64_t>(Pair->second.second->Addend);
      if (Width < sizeof(Value))
        Value &= (UINT64_C(1) << (Width * 8)) - 1;
      if (!writeUnsigned(Bytes, Rel.Offset, Width, Endianness::Little, Value)) {
        return fail(Rel, "cannot encode symbol difference");
      }
      continue;
    }
    const auto &Symbol = Graph.symbols()[Rel.Symbol];
    const int128_t S = int128_t(Graph.sections()[Symbol.Section].Address) +
                       Symbol.Offset + Rel.Addend;
    const int128_t P =
        int128_t(Graph.sections()[Rel.Section].Address) + Rel.Offset;
    if (Rel.Type == llvm::ELF::R_RISCV_64) {
      constexpr uint8_t AbsolutePointerWidth = 8;
      if (S < 0 || S > UINT64_MAX ||
          !writeUnsigned(Bytes, Rel.Offset, AbsolutePointerWidth,
                         Endianness::Little, static_cast<uint64_t>(S))) {
        return fail(Rel, "absolute relocation overflows");
      }
      if (!RebaseIntervals.insert(Rel.Section, Rel.Offset,
                                  AbsolutePointerWidth)) {
        return fail(Rel, "overlapping generated rebase");
      }
      Result.Rebases.push_back(Rebase{Rel.Section, Rel.Offset, Rel.Type,
                                      Rel.Addend, AbsolutePointerWidth});
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_32_PCREL) {
      const int128_t Value = S - P;
      if (!signedBits(Value, WordBits) ||
          !writeSigned(Bytes, Rel.Offset, InstructionWidth, Endianness::Little,
                       static_cast<int64_t>(Value))) {
        return fail(Rel, "32_PCREL relocation overflows");
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
    if (Rel.Type == llvm::ELF::R_RISCV_CALL ||
        Rel.Type == llvm::ELF::R_RISCV_CALL_PLT) {
      constexpr unsigned ImmediateBits = 12;
      constexpr unsigned IImmediateShift = 20;
      constexpr uint32_t JalrOpcode = UINT32_C(0x67);
      constexpr uint32_t JalrOpcodeMask = UINT32_C(0x707F);
      constexpr uint32_t IImmediatePreservedMask = UINT32_C(0x000FFFFF);
      constexpr uint32_t ImmediateMask = UINT32_C(0xFFF);
      auto Second = readUnsigned(Bytes, Rel.Offset + InstructionWidth,
                                 InstructionWidth, Endianness::Little);
      if ((Instruction & OpcodeMask) != AuipcOpcode || !Second ||
          (*Second & JalrOpcodeMask) != JalrOpcode) {
        return fail(Rel, "invalid call instruction pair");
      }
      const int128_t Delta = S - P;
      const int128_t High = (Delta + ImmediateRoundingBias) >> ImmediateShift;
      if (!signedBits(High, 20)) {
        return fail(Rel, "call displacement overflows");
      }
      const int128_t Low = Delta - High * ImmediateScale;
      if (!signedBits(Low, ImmediateBits)) {
        return fail(Rel, "call displacement overflows");
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
        return fail(Rel, "cannot encode call pair");
      }
      continue;
    }
    if (Rel.Type == llvm::ELF::R_RISCV_PCREL_HI20) {
      const auto HighValue = HighValues.find({Rel.Section, Rel.Offset});
      if (HighValue == HighValues.end()) {
        return fail(Rel, "missing precomputed PCREL_HI20 relocation");
      }
      const int128_t Delta = HighValue->second;
      const int128_t High = (Delta + ImmediateRoundingBias) >> ImmediateShift;
      Instruction = (Instruction & UpperImmediatePreservedMask) |
                    (static_cast<uint32_t>(High) << ImmediateShift);
    } else if (Rel.Type == llvm::ELF::R_RISCV_PCREL_LO12_I ||
               Rel.Type == llvm::ELF::R_RISCV_PCREL_LO12_S) {
      constexpr unsigned IImmediateShift = 20;
      constexpr unsigned SImmediateLowShift = 7;
      constexpr unsigned SImmediateHighShift = 20;
      constexpr uint32_t OpImmOpcode = UINT32_C(0x13);
      constexpr uint32_t LoadOpcode = UINT32_C(0x03);
      constexpr uint32_t StoreOpcode = UINT32_C(0x23);
      constexpr uint32_t JalrOpcode = UINT32_C(0x67);
      constexpr uint32_t IImmediatePreservedMask = UINT32_C(0x000FFFFF);
      constexpr uint32_t SImmediatePreservedMask = UINT32_C(0x01FFF07F);
      constexpr uint32_t ImmediateMask = UINT32_C(0xFFF);
      constexpr uint32_t SImmediateLowMask = UINT32_C(0x1F);
      constexpr uint32_t SImmediateHighMask = UINT32_C(0xFE0);
      const auto HighSection = Graph.symbols()[Rel.Symbol].Section;
      const auto HighOffset = Graph.symbols()[Rel.Symbol].Offset;
      const auto High = HighValues.find({HighSection, HighOffset});
      if (High == HighValues.end()) {
        return fail(Rel, "missing PCREL_HI20 pair");
      }
      const int128_t Rounded =
          (High->second + ImmediateRoundingBias) >> ImmediateShift;
      const uint32_t Low =
          static_cast<uint32_t>(High->second - Rounded * ImmediateScale) &
          ImmediateMask;
      if (Rel.Type == llvm::ELF::R_RISCV_PCREL_LO12_I) {
        const uint32_t Opcode = Instruction & OpcodeMask;
        if (Opcode != OpImmOpcode && Opcode != LoadOpcode &&
            Opcode != JalrOpcode) {
          return fail(Rel, "invalid PCREL_LO12_I instruction");
        }
        Instruction =
            (Instruction & IImmediatePreservedMask) | (Low << IImmediateShift);
      } else {
        if ((Instruction & OpcodeMask) != StoreOpcode) {
          return fail(Rel, "invalid PCREL_LO12_S instruction");
        }
        Instruction = (Instruction & SImmediatePreservedMask) |
                      ((Low & SImmediateLowMask) << SImmediateLowShift) |
                      ((Low & SImmediateHighMask) << SImmediateHighShift);
      }
    } else {
      return fail(Rel, "unsupported RISC-V relocation type",
                  DiagnosticKind::Unsupported);
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
