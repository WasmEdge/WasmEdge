// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/pe_writer.h"
#include "linker/relocation.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Support/MathExtras.h>

#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

namespace {

constexpr uint8_t WordWidth = 4;
constexpr uint8_t DoubleWordWidth = 8;
constexpr int64_t PCRelativeAddendBias = -4;

enum class Opcode : uint8_t {
  Group5 = 0xFF,
  Load = 0x8B,
  LoadEffectiveAddress = 0x8D,
  CallRelative = 0xE8,
  JumpRelative = 0xE9,
  Nop = 0x90,
};

enum class ModRM : uint8_t {
  CallPCRelative = 0x15,
  JumpPCRelative = 0x25,
  LoadPCRelative = 0x05,
};

enum class Prefix : uint8_t {
  AddressSizeOverride = 0x67,
  Rex = 0x40,
};

constexpr uint8_t value(Opcode Value) noexcept {
  return static_cast<uint8_t>(Value);
}

constexpr uint8_t value(ModRM Value) noexcept {
  return static_cast<uint8_t>(Value);
}

constexpr uint8_t value(Prefix Value) noexcept {
  return static_cast<uint8_t>(Value);
}

bool delta32(uint64_t Symbol, int64_t Addend, uint64_t Place,
             int64_t &Result) noexcept {
  int64_t Delta = 0;
  uint64_t Target = 0;
  const bool Exact =
      (signedDelta(Symbol, Place, Delta) && addSigned(Delta, Addend, Delta)) ||
      (addSigned(Symbol, Addend, Target) && signedDelta(Target, Place, Delta));
  if (!Exact || !llvm::isInt<32>(Delta)) {
    return false;
  }
  Result = Delta;
  return true;
}

LinkExpect<bool> relaxGOTPCRELX(const LinkGraph &Graph, const Relocation &Rel,
                                std::vector<Byte> &Bytes, uint64_t S,
                                uint64_t P, int64_t Addend) {
  constexpr uint8_t RexPrefixMask = 0xF0;
  constexpr uint8_t LoadModRMMask = 0xC7;
  constexpr uint64_t OpcodeAndModRMSize = 2;
  constexpr uint64_t RexOpcodeAndModRMSize = 3;
  constexpr uint64_t ModRMOffsetFromOpcode = 1;
  constexpr int64_t NextInstructionBias = 1;
  constexpr uint64_t DisplacementFieldBacktrack = 1;
  constexpr uint64_t TrailingNopOffsetFromPatch = 3;
  if (Rel.AddendIsImplicit || Addend != PCRelativeAddendBias ||
      Rel.Offset < OpcodeAndModRMSize) {
    return relocationError(Graph, Rel, "unsupported GOTPCRELX instruction",
                           DiagnosticKind::Unsupported);
  }
  const size_t OpcodeOffset =
      static_cast<size_t>(Rel.Offset - OpcodeAndModRMSize);
  const uint8_t Op = Bytes[OpcodeOffset];
  const uint8_t ModRMByte = Bytes[OpcodeOffset + ModRMOffsetFromOpcode];
  const bool HasRex = Rel.Offset >= RexOpcodeAndModRMSize &&
                      (Bytes[Rel.Offset - RexOpcodeAndModRMSize] &
                       RexPrefixMask) == value(Prefix::Rex);
  if (Rel.Type == llvm::ELF::R_X86_64_REX_GOTPCRELX && !HasRex) {
    return relocationError(Graph, Rel, "unsupported GOTPCRELX instruction",
                           DiagnosticKind::Unsupported);
  }
  if (Op == value(Opcode::Load) &&
      (ModRMByte & LoadModRMMask) == value(ModRM::LoadPCRelative)) {
    Bytes[OpcodeOffset] = value(Opcode::LoadEffectiveAddress);
    return false;
  }
  if (Op == value(Opcode::Group5) &&
      ModRMByte == value(ModRM::CallPCRelative)) {
    Bytes[OpcodeOffset] = value(Prefix::AddressSizeOverride);
    Bytes[OpcodeOffset + ModRMOffsetFromOpcode] = value(Opcode::CallRelative);
    return false;
  }
  if (Op == value(Opcode::Group5) &&
      ModRMByte == value(ModRM::JumpPCRelative)) {
    int64_t Value = 0;
    if (!delta32(S, Addend + NextInstructionBias, P, Value) ||
        !writeSigned(Bytes, Rel.Offset - DisplacementFieldBacktrack, WordWidth,
                     Endianness::Little, Value)) {
      return relocationError(Graph, Rel, "PC-relative relocation overflows");
    }
    Bytes[OpcodeOffset] = value(Opcode::JumpRelative);
    Bytes[Rel.Offset + TrailingNopOffsetFromPatch] = value(Opcode::Nop);
    return true;
  }
  return relocationError(Graph, Rel, "unsupported GOTPCRELX instruction",
                         DiagnosticKind::Unsupported);
}

} // namespace

LinkExpect<RelocationResult> applyX86_64(const LinkGraph &Graph) {
  RelocationWriter Writer(Graph);
  for (const auto &Rel : Graph.relocations()) {
    uint8_t Width = 0;
    bool Absolute = false;
    bool ImageRelative = false;
    bool Relax = false;
    int64_t FormatAdjustment = 0;
    if (Rel.Format == ObjectFormat::ELF) {
      switch (Rel.Type) {
      case llvm::ELF::R_X86_64_64:
        Width = DoubleWordWidth;
        Absolute = true;
        break;
      case llvm::ELF::R_X86_64_PC32:
      case llvm::ELF::R_X86_64_PLT32:
        Width = WordWidth;
        break;
      case llvm::ELF::R_X86_64_GOTPCRELX:
      case llvm::ELF::R_X86_64_REX_GOTPCRELX:
        Width = WordWidth;
        Relax = true;
        break;
      default:
        return relocationError(Graph, Rel, "unsupported x86_64 relocation type",
                               DiagnosticKind::Unsupported);
      }
    } else if (Rel.Format == ObjectFormat::MachO &&
               Rel.Type == llvm::MachO::X86_64_RELOC_UNSIGNED) {
      Width = DoubleWordWidth;
      Absolute = true;
    } else if (Rel.Format == ObjectFormat::MachO &&
               (Rel.Type == llvm::MachO::X86_64_RELOC_SIGNED ||
                Rel.Type == llvm::MachO::X86_64_RELOC_SIGNED_1 ||
                Rel.Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ||
                Rel.Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ||
                Rel.Type == llvm::MachO::X86_64_RELOC_BRANCH)) {
      Width = WordWidth;
      FormatAdjustment = -static_cast<int64_t>(WordWidth);
      switch (Rel.Type) {
      case llvm::MachO::X86_64_RELOC_SIGNED_1:
        if (!Rel.AddendIsImplicit)
          FormatAdjustment -= 1;
        break;
      case llvm::MachO::X86_64_RELOC_SIGNED_2:
        if (!Rel.AddendIsImplicit)
          FormatAdjustment -= 2;
        break;
      case llvm::MachO::X86_64_RELOC_SIGNED_4:
        if (!Rel.AddendIsImplicit)
          FormatAdjustment -= 4;
        break;
      default:
        break;
      }
    } else if (Rel.Format == ObjectFormat::COFF &&
               Rel.Type == llvm::COFF::IMAGE_REL_AMD64_ADDR64) {
      Width = DoubleWordWidth;
      Absolute = true;
    } else if (Rel.Format == ObjectFormat::COFF &&
               Rel.Type >= llvm::COFF::IMAGE_REL_AMD64_REL32 &&
               Rel.Type <= llvm::COFF::IMAGE_REL_AMD64_REL32_5) {
      Width = WordWidth;
      FormatAdjustment =
          PCRelativeAddendBias -
          static_cast<int64_t>(Rel.Type - llvm::COFF::IMAGE_REL_AMD64_REL32);
    } else if (Rel.Format == ObjectFormat::COFF &&
               Rel.Type == llvm::COFF::IMAGE_REL_AMD64_ADDR32NB) {
      Width = WordWidth;
      ImageRelative = true;
    } else {
      return relocationError(Graph, Rel, "unsupported x86_64 relocation type",
                             DiagnosticKind::Unsupported);
    }
    auto &Bytes = Writer.content(Rel.Section);
    if (!validField(Bytes, Rel.Offset, Width)) {
      return relocationError(Graph, Rel,
                             "relocation field exceeds section content");
    }
    EXPECTED_TRY(const uint64_t S, symbolAddress(Graph, Rel));
    EXPECTED_TRY(const uint64_t P, placeAddress(Graph, Rel));
    int64_t Addend = Rel.Addend;
    if (Rel.AddendIsImplicit) {
      if (ImageRelative) {
        auto Decoded =
            readUnsigned(Bytes, Rel.Offset, Width, Endianness::Little);
        if (!Decoded || *Decoded > INT64_MAX) {
          return relocationError(Graph, Rel, "cannot decode implicit addend");
        }
        Addend = static_cast<int64_t>(*Decoded);
      } else {
        auto Decoded = readSigned(Bytes, Rel.Offset, Width, Endianness::Little);
        if (!Decoded) {
          return relocationError(Graph, Rel, "cannot decode implicit addend");
        }
        Addend = *Decoded;
      }
    }
    if (!addSigned(Addend, FormatAdjustment, Addend)) {
      return relocationError(Graph, Rel, "relocation addend overflows");
    }
    if (ImageRelative) {
      const uint64_t ImageBase = S >= PEImageBase ? PEImageBase : 0;
      if (S - ImageBase > UINT32_MAX) {
        return relocationError(Graph, Rel, "absolute relocation overflows");
      }
      const uint32_t Value =
          static_cast<uint32_t>(S - ImageBase) + static_cast<uint32_t>(Addend);
      if (!writeUnsigned(Bytes, Rel.Offset, Width, Endianness::Little, Value)) {
        return relocationError(Graph, Rel, "absolute relocation overflows");
      }
      continue;
    }
    if (Absolute) {
      uint64_t Value = 0;
      if (!addSigned(S, Addend, Value)) {
        return relocationError(Graph, Rel, "absolute relocation overflows");
      }
      EXPECTED_TRY(
          Writer.writeAbsolute(Rel, Width, Endianness::Little, Value, Addend));
      continue;
    }
    if (Relax) {
      EXPECTED_TRY(const bool Patched,
                   relaxGOTPCRELX(Graph, Rel, Bytes, S, P, Addend));
      if (Patched) {
        continue;
      }
    }
    int64_t Value = 0;
    if (!delta32(S, Addend, P, Value) ||
        !writeSigned(Bytes, Rel.Offset, Width, Endianness::Little, Value)) {
      return relocationError(Graph, Rel, "PC-relative relocation overflows");
    }
  }
  return std::move(Writer).finish();
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
