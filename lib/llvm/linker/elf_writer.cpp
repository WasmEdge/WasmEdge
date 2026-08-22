// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/elf_writer.h"

#include "linker/relocation.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/BinaryFormat/ELF.h>

#include <algorithm>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

// File, program, section, dynamic, and symbol table layouts follow the System
// V ELF gABI; relocation values and e_flags follow each architecture psABI.
constexpr uint8_t ELFMagic0 = 0x7F;
constexpr uint8_t ELFMagic1 = 'E';
constexpr uint8_t ELFMagic2 = 'L';
constexpr uint8_t ELFMagic3 = 'F';
constexpr uint8_t EncodingFormatMask = 0x0F;
constexpr uint8_t EncodingApplicationMask = 0x70;
constexpr uint32_t ExtendedRecordLength = UINT32_MAX;

struct Failure {
  operator Expect<void>() const noexcept {
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  operator LinkExpect<void>() const {
    return Unexpected<Diagnostic>(Diagnostic("ELF writer failed"));
  }
};

Failure fail() noexcept { return {}; }

LinkExpect<void> linkFail(std::string Message,
                          const Section *SectionValue = nullptr,
                          std::optional<SectionId> Id = std::nullopt,
                          std::optional<uint64_t> Offset = std::nullopt,
                          DiagnosticKind Kind = DiagnosticKind::Malformed) {
  Diagnostic Diag(std::move(Message));
  Diag.Section = Id;
  Diag.Offset = Offset;
  Diag.Kind = Kind;
  if (SectionValue)
    Diag.SectionName = SectionValue->Name;
  return Unexpected<Diagnostic>(std::move(Diag));
}

bool add(uint64_t Left, uint64_t Right, uint64_t &Result) noexcept {
  if (Left > std::numeric_limits<uint64_t>::max() - Right)
    return false;
  Result = Left + Right;
  return true;
}

bool multiply(uint64_t Left, uint64_t Right, uint64_t &Result) noexcept {
  if (Left != 0 && Right > std::numeric_limits<uint64_t>::max() / Left)
    return false;
  Result = Left * Right;
  return true;
}

bool vectorSize(uint64_t Value) noexcept {
#if SIZE_MAX < UINT64_MAX
  return Value <= SIZE_MAX;
#else
  static_cast<void>(Value);
  return true;
#endif
}

Expect<void> copyAt(std::vector<Byte> &Bytes, uint64_t Offset,
                    Span<const Byte> Content) {
  if (Offset > Bytes.size() || Content.size() > Bytes.size() - Offset)
    return fail();
  std::copy(Content.begin(), Content.end(), Bytes.begin() + Offset);
  return {};
}

bool align(uint64_t Value, uint64_t Alignment, uint64_t &Result) noexcept {
  const uint64_t Mask = Alignment - 1;
  if (!add(Value, Mask, Result))
    return false;
  Result &= ~Mask;
  return true;
}

bool boundedAdd(uint64_t Value, uint64_t Increment, uint64_t Maximum,
                uint64_t &Result) noexcept {
  return add(Value, Increment, Result) && Result <= Maximum;
}

bool boundedAlign(uint64_t Value, uint64_t Alignment, uint64_t Maximum,
                  uint64_t &Result) noexcept {
  return align(Value, Alignment, Result) && Result <= Maximum;
}

bool is64(Target Value) noexcept { return Value != Target::ARM; }

uint64_t maximumPageSize(Target Value) noexcept {
  switch (Value) {
  case Target::ARM:
  case Target::AArch64:
    return 65536;
  case Target::X86_64:
  case Target::RISCV64:
  case Target::S390X:
    return 4096;
  }
  return 0;
}

uint16_t machine(Target Value) noexcept {
  switch (Value) {
  case Target::ARM:
    return llvm::ELF::EM_ARM;
  case Target::X86_64:
    return llvm::ELF::EM_X86_64;
  case Target::AArch64:
    return llvm::ELF::EM_AARCH64;
  case Target::RISCV64:
    return llvm::ELF::EM_RISCV;
  case Target::S390X:
    return llvm::ELF::EM_S390;
  }
  return llvm::ELF::EM_NONE;
}

uint32_t relativeType(Target Value) noexcept {
  switch (Value) {
  case Target::ARM:
    return llvm::ELF::R_ARM_RELATIVE;
  case Target::X86_64:
    return llvm::ELF::R_X86_64_RELATIVE;
  case Target::AArch64:
    return llvm::ELF::R_AARCH64_RELATIVE;
  case Target::RISCV64:
    return llvm::ELF::R_RISCV_RELATIVE;
  case Target::S390X:
    return llvm::ELF::R_390_RELATIVE;
  }
  return 0;
}

bool validRebase(const LinkGraph &Graph, const Rebase &Value) noexcept {
  const uint8_t Width = is64(Graph.target()) ? 8 : 4;
  if (Value.Format != ObjectFormat::ELF || Value.Width != Width)
    return false;
  switch (Graph.target()) {
  case Target::ARM:
    return Value.Type == llvm::ELF::R_ARM_ABS32 ||
           Value.Type == llvm::ELF::R_ARM_RELATIVE;
  case Target::X86_64:
    return Value.Type == llvm::ELF::R_X86_64_64 ||
           Value.Type == llvm::ELF::R_X86_64_RELATIVE;
  case Target::AArch64:
    return Value.Type == llvm::ELF::R_AARCH64_ABS64 ||
           Value.Type == llvm::ELF::R_AARCH64_RELATIVE;
  case Target::RISCV64:
    return Value.Type == llvm::ELF::R_RISCV_64 ||
           Value.Type == llvm::ELF::R_RISCV_RELATIVE;
  case Target::S390X:
    return Value.Type == llvm::ELF::R_390_64 ||
           Value.Type == llvm::ELF::R_390_RELATIVE;
  }
  return false;
}

bool validELFFlags(const LinkGraph &Graph) noexcept {
  if (Graph.target() == Target::RISCV64) {
    constexpr uint32_t Supported = llvm::ELF::EF_RISCV_RVC |
                                   llvm::ELF::EF_RISCV_FLOAT_ABI |
#if LLVM_VERSION_MAJOR >= 18
                                   llvm::ELF::EF_RISCV_TSO |
#endif
                                   llvm::ELF::EF_RISCV_RVE;
    return (Graph.elfFlags() & ~Supported) == 0 &&
           (Graph.elfFlags() & llvm::ELF::EF_RISCV_RVE) == 0;
  }
  if (Graph.target() != Target::ARM)
    return Graph.elfFlags() == 0;
  constexpr uint32_t Float =
      llvm::ELF::EF_ARM_ABI_FLOAT_SOFT | llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  constexpr uint32_t Supported = llvm::ELF::EF_ARM_EABIMASK | Float;
  return (Graph.elfFlags() & llvm::ELF::EF_ARM_EABIMASK) ==
             llvm::ELF::EF_ARM_EABI_VER5 &&
         (Graph.elfFlags() & Float) != Float &&
         (Graph.elfFlags() & ~Supported) == 0;
}

std::optional<uint64_t>
reservedProgramHeaders(const LinkGraph &Graph) noexcept {
  constexpr uint64_t FixedHeaders = 6;
  uint64_t Result = 0;
  if (!add(Graph.sections().size(), FixedHeaders, Result) ||
      Result > UINT16_MAX)
    return std::nullopt;
  return Result;
}

void put(std::vector<Byte> &Bytes, uint64_t Offset, uint64_t Value,
         uint8_t Width, Endianness Endian) {
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift = Endian == Endianness::Little ? I : Width - I - 1;
    Bytes[Offset + I] = static_cast<Byte>(Value >> (Shift * 8));
  }
}

uint64_t get(Span<const Byte> Bytes, uint64_t Offset, uint8_t Width,
             Endianness Endian) {
  uint64_t Result = 0;
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift = Endian == Endianness::Little ? I : Width - I - 1;
    Result |= static_cast<uint64_t>(Bytes[Offset + I]) << (Shift * 8);
  }
  return Result;
}

uint32_t hash(std::string_view Name) noexcept {
  uint32_t Result = 0;
  for (const char CharacterValue : Name) {
    const auto Character = static_cast<unsigned char>(CharacterValue);
    Result = (Result << 4) + Character;
    const uint32_t High = Result & UINT32_C(0xF0000000);
    if (High != 0)
      Result ^= High >> 24;
    Result &= ~High;
  }
  return Result;
}

bool readULEB(Span<const Byte> Bytes, size_t &Offset,
              uint64_t &Result) noexcept {
  Result = 0;
  for (uint8_t Shift = 0; Shift < 64 && Offset < Bytes.size(); Shift += 7) {
    const uint8_t Value = Bytes[Offset++];
    if (Shift == 63 && (Value & 0x7E) != 0)
      return false;
    Result |= static_cast<uint64_t>(Value & 0x7F) << Shift;
    if ((Value & 0x80) == 0)
      return true;
  }
  return false;
}

bool skipSLEB(Span<const Byte> Bytes, size_t &Offset) noexcept {
  for (uint8_t Count = 0; Count < 10 && Offset < Bytes.size(); ++Count)
    if ((Bytes[Offset++] & 0x80) == 0)
      return true;
  return false;
}

struct CIEInfo {
  uint8_t Encoding;
};

struct FDEInfo {
  uint64_t Function;
  uint64_t Address;
};

bool signedDelta(uint64_t Target, uint64_t Base, int32_t &Result) noexcept {
  const uint64_t Magnitude = Target >= Base ? Target - Base : Base - Target;
  if ((Target >= Base && Magnitude > static_cast<uint64_t>(INT32_MAX)) ||
      (Target < Base && Magnitude > UINT64_C(1) << 31))
    return false;
  Result = Target >= Base
               ? static_cast<int32_t>(Magnitude)
               : static_cast<int32_t>(-static_cast<int64_t>(Magnitude));
  return true;
}

bool decodeFDEAddress(Span<const Byte> Bytes, size_t Offset, uint8_t Encoding,
                      uint64_t FieldAddress, Endianness Endian,
                      uint64_t &Result) noexcept {
  const uint8_t Format = Encoding & EncodingFormatMask;
  uint8_t Width = 0;
  bool Signed = false;
  switch (Format) {
  case llvm::dwarf::DW_EH_PE_sdata2:
    Width = 2;
    Signed = true;
    break;
  case llvm::dwarf::DW_EH_PE_sdata4:
    Width = 4;
    Signed = true;
    break;
  case llvm::dwarf::DW_EH_PE_sdata8:
    Width = 8;
    Signed = true;
    break;
  case llvm::dwarf::DW_EH_PE_udata2:
    Width = 2;
    break;
  case llvm::dwarf::DW_EH_PE_udata4:
    Width = 4;
    break;
  case llvm::dwarf::DW_EH_PE_udata8:
    Width = 8;
    break;
  default:
    return false;
  }
  if (Offset > Bytes.size() || Width > Bytes.size() - Offset)
    return false;
  const uint64_t Raw = get(Bytes, Offset, Width, Endian);
  int64_t Value = static_cast<int64_t>(Raw);
  if (Signed && Width < 8 && (Raw & (UINT64_C(1) << (Width * 8 - 1))) != 0)
    Value = static_cast<int64_t>(Raw | (~UINT64_C(0) << (Width * 8)));
  if ((Encoding & EncodingApplicationMask) == llvm::dwarf::DW_EH_PE_pcrel) {
    if (Value < 0) {
      const uint64_t Magnitude = static_cast<uint64_t>(-(Value + 1)) + 1;
      if (Magnitude > FieldAddress)
        return false;
      Result = FieldAddress - Magnitude;
    } else {
      if (!add(FieldAddress, static_cast<uint64_t>(Value), Result))
        return false;
    }
    return true;
  }
  if ((Encoding & EncodingApplicationMask) != llvm::dwarf::DW_EH_PE_absptr ||
      Value < 0)
    return false;
  Result = static_cast<uint64_t>(Value);
  return true;
}

uint8_t encodingWidth(uint8_t Encoding) noexcept {
  switch (Encoding & EncodingFormatMask) {
  case llvm::dwarf::DW_EH_PE_sdata2:
  case llvm::dwarf::DW_EH_PE_udata2:
    return 2;
  case llvm::dwarf::DW_EH_PE_sdata4:
  case llvm::dwarf::DW_EH_PE_udata4:
    return 4;
  case llvm::dwarf::DW_EH_PE_sdata8:
  case llvm::dwarf::DW_EH_PE_udata8:
    return 8;
  default:
    return 0;
  }
}

bool graphContainsAddress(const LinkGraph &Graph, uint64_t Address) noexcept {
  return std::any_of(Graph.sections().begin(), Graph.sections().end(),
                     [&](const auto &SectionValue) {
                       return Address >= SectionValue.Address &&
                              Address - SectionValue.Address <
                                  SectionValue.VirtualSize;
                     });
}

bool graphHasRebasedSlot(const LinkGraph &Graph, uint64_t Address) noexcept {
  const uint8_t PointerWidth = is64(Graph.target()) ? 8 : 4;
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    const auto &SectionValue = Graph.sections()[I];
    if (Address < SectionValue.Address ||
        Address - SectionValue.Address >= SectionValue.Content.size())
      continue;
    const uint64_t Offset = Address - SectionValue.Address;
    return std::any_of(Graph.rebases().begin(), Graph.rebases().end(),
                       [&](const auto &RebaseValue) {
                         return RebaseValue.Section == I &&
                                RebaseValue.Offset == Offset &&
                                RebaseValue.Width == PointerWidth;
                       });
  }
  return false;
}

LinkExpect<void> parseEHFrame(const LinkGraph &Graph, const Section &EH,
                              Endianness Endian,
                              std::vector<FDEInfo> &FDEs) noexcept {
  const Span<const Byte> Bytes(EH.Content.data(), EH.Content.size());
  const auto SectionIt =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [&](const Section &Value) { return &Value == &EH; });
  const std::optional<SectionId> SectionIndex =
      SectionIt == Graph.sections().end()
          ? std::nullopt
          : std::optional<SectionId>(static_cast<SectionId>(
                std::distance(Graph.sections().begin(), SectionIt)));
  auto Malformed = [&](std::string Message, size_t ErrorOffset) {
    return linkFail(std::move(Message), &EH, SectionIndex, ErrorOffset);
  };
  auto Unsupported = [&](std::string Message, size_t ErrorOffset) {
    return linkFail(std::move(Message), &EH, SectionIndex, ErrorOffset,
                    DiagnosticKind::Unsupported);
  };
  std::map<size_t, CIEInfo> CIEs;
  size_t Offset = 0;
  while (Offset < Bytes.size()) {
    if (Bytes.size() - Offset < 4)
      return Malformed("truncated EH frame record length", Offset);
    const uint64_t Length = get(Bytes, Offset, 4, Endian);
    if (Length == 0)
      return Offset + 4 == Bytes.size()
                 ? LinkExpect<void>{}
                 : Malformed("EH frame terminator is not final", Offset);
    if (Length == ExtendedRecordLength || Length > Bytes.size() - Offset - 4 ||
        Length < 4)
      return Length == ExtendedRecordLength
                 ? Unsupported("unsupported extended EH frame record", Offset)
                 : Malformed("invalid EH frame record length", Offset);
    const size_t End = Offset + 4 + static_cast<size_t>(Length);
    const auto Record = Bytes.subspan(0, End);
    const uint64_t Id = get(Bytes, Offset + 4, 4, Endian);
    if (Id == 0) {
      size_t Cursor = Offset + 8;
      if (Cursor >= End || Bytes[Cursor++] != 1)
        return Unsupported("unsupported EH frame CIE version", Offset + 8);
      std::string Augmentation;
      while (Cursor < End && Bytes[Cursor] != 0)
        Augmentation.push_back(static_cast<char>(Bytes[Cursor++]));
      if (Cursor >= End || Augmentation.empty() || Augmentation.front() != 'z')
        return Unsupported("unsupported EH frame augmentation", Offset + 9);
      ++Cursor;
      uint64_t Ignored = 0;
      if (!readULEB(Record, Cursor, Ignored) || !skipSLEB(Record, Cursor) ||
          !readULEB(Record, Cursor, Ignored))
        return Malformed("invalid EH frame CIE alignment fields", Cursor);
      uint64_t AugmentationSize = 0;
      if (!readULEB(Record, Cursor, AugmentationSize) ||
          AugmentationSize > End - Cursor)
        return Malformed("invalid EH frame CIE augmentation size", Cursor);
      const size_t AugmentationEnd = Cursor + AugmentationSize;
      uint8_t Encoding = llvm::dwarf::DW_EH_PE_omit;
      for (const char Character : Augmentation.substr(1)) {
        if (Character == 'R') {
          if (Cursor >= AugmentationEnd)
            return Malformed("missing EH frame FDE encoding", Cursor);
          Encoding = Bytes[Cursor++];
        } else if (Character == 'P') {
          if (Cursor >= AugmentationEnd)
            return Malformed("missing EH frame personality encoding", Cursor);
          const uint8_t PersonalityEncoding = Bytes[Cursor++];
          const uint8_t Width = encodingWidth(PersonalityEncoding);
          if ((PersonalityEncoding & EncodingApplicationMask) !=
                  llvm::dwarf::DW_EH_PE_pcrel ||
              (PersonalityEncoding & EncodingFormatMask) <
                  llvm::dwarf::DW_EH_PE_sdata2 ||
              Width == 0 || Width > AugmentationEnd - Cursor)
            return Unsupported("unsupported EH frame personality encoding",
                               Cursor - 1);
          uint64_t Personality = 0;
          if (!decodeFDEAddress(Record, Cursor, PersonalityEncoding,
                                EH.Address + Cursor, Endian, Personality))
            return Malformed("invalid EH frame personality address", Cursor);
          const bool Indirect =
              (PersonalityEncoding & llvm::dwarf::DW_EH_PE_indirect) != 0;
          if (Indirect ? !graphHasRebasedSlot(Graph, Personality)
                       : !graphContainsAddress(Graph, Personality))
            return Malformed("EH frame personality target is invalid", Cursor);
          Cursor += Width;
        } else if (Character == 'L') {
          if (Cursor >= AugmentationEnd)
            return Malformed("missing EH frame LSDA encoding", Cursor);
          ++Cursor;
        } else {
          return Unsupported("unsupported EH frame augmentation character",
                             Cursor);
        }
      }
      if (Cursor != AugmentationEnd || Encoding == llvm::dwarf::DW_EH_PE_omit)
        return Encoding == llvm::dwarf::DW_EH_PE_omit
                   ? Unsupported("missing EH frame FDE address encoding",
                                 Cursor)
                   : Malformed("invalid EH frame augmentation payload", Cursor);
      CIEs.emplace(Offset, CIEInfo{Encoding});
    } else {
      const size_t PointerField = Offset + 4;
      if (Id > PointerField)
        return Malformed("invalid EH frame CIE pointer", PointerField);
      const auto CIE = CIEs.find(PointerField - static_cast<size_t>(Id));
      if (CIE == CIEs.end())
        return Malformed("EH frame FDE references an unknown CIE",
                         PointerField);
      const size_t InitialLocation = Offset + 8;
      uint64_t Function = 0;
      if (!decodeFDEAddress(Record, InitialLocation, CIE->second.Encoding,
                            EH.Address + InitialLocation, Endian, Function))
        return Malformed("invalid EH frame FDE initial location",
                         InitialLocation);
      FDEs.push_back(FDEInfo{Function, EH.Address + Offset});
    }
    Offset = End;
  }
  return {};
}

struct OutputSection {
  std::string Name;
  uint32_t Type = llvm::ELF::SHT_PROGBITS;
  uint64_t Flags = 0;
  uint64_t Address = 0;
  uint64_t Offset = 0;
  uint64_t Size = 0;
  uint32_t Link = 0;
  uint32_t Info = 0;
  uint64_t Alignment = 1;
  uint64_t EntrySize = 0;
  std::vector<Byte> Content;
};

struct OutputSegment {
  uint32_t Type;
  uint32_t Flags;
  uint64_t Offset;
  uint64_t Address;
  uint64_t FileSize;
  uint64_t MemorySize;
  uint64_t Alignment;
};

struct ELFEmissionContext {
  const LinkGraph &Graph;
  const std::vector<OutputSection> &Sections;
  Span<const uint32_t> SectionNameOffsets;
  Span<const OutputSegment> Segments;
  uint64_t FileSize;
  uint64_t ELFHeaderSize;
  uint64_t ProgramHeaderSize;
  uint64_t SectionHeaderSize;
  uint64_t SectionHeaderOffset;
  uint32_t SectionNameIndex;
  bool Wide;
  Endianness Endian;
};

uint64_t sectionFlags(SectionKind Kind) noexcept {
  switch (Kind) {
  case SectionKind::Text:
    return llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_EXECINSTR;
  case SectionKind::ReadOnly:
  case SectionKind::Unwind:
    return llvm::ELF::SHF_ALLOC;
  case SectionKind::Data:
  case SectionKind::BSS:
    return llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_WRITE;
  }
  return 0;
}

std::vector<SectionId>
order(const LinkGraph &Graph, SectionKind Kind,
      Span<const std::pair<uint64_t, uint64_t>> Placements = {}) {
  std::vector<SectionId> Result;
  for (SectionId I = 0; I < Graph.sections().size(); ++I)
    if (Graph.sections()[I].Kind == Kind)
      Result.push_back(I);
  std::sort(Result.begin(), Result.end(), [&](SectionId Left, SectionId Right) {
    const auto &L = Graph.sections()[Left];
    const auto &R = Graph.sections()[Right];
    if (Kind == SectionKind::Unwind) {
      const bool LExidx = L.Purpose == SectionPurpose::ARMExidx;
      const bool RExidx = R.Purpose == SectionPurpose::ARMExidx;
      if (LExidx != RExidx)
        return LExidx;
      if (LExidx) {
        const uint64_t LAddress =
            L.LinkedSection && *L.LinkedSection < Placements.size()
                ? Placements[*L.LinkedSection].first
                : UINT64_MAX;
        const uint64_t RAddress =
            R.LinkedSection && *R.LinkedSection < Placements.size()
                ? Placements[*R.LinkedSection].first
                : UINT64_MAX;
        return std::tie(LAddress, Left) < std::tie(RAddress, Right);
      }
    }
    return std::tie(L.Name, Left) < std::tie(R.Name, Right);
  });
  return Result;
}

LinkExpect<void> emitELF(const ELFEmissionContext &Context, Writer &Output) {
  std::vector<Byte> Bytes(static_cast<size_t>(Context.FileSize));
  Bytes[0] = ELFMagic0;
  Bytes[1] = ELFMagic1;
  Bytes[2] = ELFMagic2;
  Bytes[3] = ELFMagic3;
  Bytes[llvm::ELF::EI_CLASS] = static_cast<uint8_t>(
      Context.Wide ? llvm::ELF::ELFCLASS64 : llvm::ELF::ELFCLASS32);
  Bytes[llvm::ELF::EI_DATA] = static_cast<uint8_t>(
      Context.Endian == Endianness::Little ? llvm::ELF::ELFDATA2LSB
                                           : llvm::ELF::ELFDATA2MSB);
  Bytes[llvm::ELF::EI_VERSION] = static_cast<uint8_t>(llvm::ELF::EV_CURRENT);
  Bytes[llvm::ELF::EI_OSABI] = static_cast<uint8_t>(llvm::ELF::ELFOSABI_NONE);
  put(Bytes, 16, llvm::ELF::ET_DYN, 2, Context.Endian);
  put(Bytes, 18, machine(Context.Graph.target()), 2, Context.Endian);
  put(Bytes, 20, llvm::ELF::EV_CURRENT, 4, Context.Endian);
  if (Context.Wide) {
    put(Bytes, 48, Context.Graph.elfFlags(), 4, Context.Endian);
    put(Bytes, 32, Context.ELFHeaderSize, 8, Context.Endian);
    put(Bytes, 40, Context.SectionHeaderOffset, 8, Context.Endian);
    put(Bytes, 52, Context.ELFHeaderSize, 2, Context.Endian);
    put(Bytes, 54, Context.ProgramHeaderSize, 2, Context.Endian);
    put(Bytes, 56, Context.Segments.size(), 2, Context.Endian);
    put(Bytes, 58, Context.SectionHeaderSize, 2, Context.Endian);
    put(Bytes, 60, Context.Sections.size(), 2, Context.Endian);
    put(Bytes, 62, Context.SectionNameIndex, 2, Context.Endian);
  } else {
    put(Bytes, 28, Context.ELFHeaderSize, 4, Context.Endian);
    put(Bytes, 32, Context.SectionHeaderOffset, 4, Context.Endian);
    put(Bytes, 36, Context.Graph.elfFlags(), 4, Context.Endian);
    put(Bytes, 40, Context.ELFHeaderSize, 2, Context.Endian);
    put(Bytes, 42, Context.ProgramHeaderSize, 2, Context.Endian);
    put(Bytes, 44, Context.Segments.size(), 2, Context.Endian);
    put(Bytes, 46, Context.SectionHeaderSize, 2, Context.Endian);
    put(Bytes, 48, Context.Sections.size(), 2, Context.Endian);
    put(Bytes, 50, Context.SectionNameIndex, 2, Context.Endian);
  }
  for (size_t I = 0; I < Context.Segments.size(); ++I) {
    const auto &Segment = Context.Segments[I];
    const uint64_t Offset =
        Context.ELFHeaderSize + I * Context.ProgramHeaderSize;
    put(Bytes, Offset, Segment.Type, 4, Context.Endian);
    if (Context.Wide) {
      put(Bytes, Offset + 4, Segment.Flags, 4, Context.Endian);
      put(Bytes, Offset + 8, Segment.Offset, 8, Context.Endian);
      put(Bytes, Offset + 16, Segment.Address, 8, Context.Endian);
      put(Bytes, Offset + 24, Segment.Address, 8, Context.Endian);
      put(Bytes, Offset + 32, Segment.FileSize, 8, Context.Endian);
      put(Bytes, Offset + 40, Segment.MemorySize, 8, Context.Endian);
      put(Bytes, Offset + 48, Segment.Alignment, 8, Context.Endian);
    } else {
      put(Bytes, Offset + 4, Segment.Offset, 4, Context.Endian);
      put(Bytes, Offset + 8, Segment.Address, 4, Context.Endian);
      put(Bytes, Offset + 12, Segment.Address, 4, Context.Endian);
      put(Bytes, Offset + 16, Segment.FileSize, 4, Context.Endian);
      put(Bytes, Offset + 20, Segment.MemorySize, 4, Context.Endian);
      put(Bytes, Offset + 24, Segment.Flags, 4, Context.Endian);
      put(Bytes, Offset + 28, Segment.Alignment, 4, Context.Endian);
    }
  }
  for (size_t I = 1; I < Context.Sections.size(); ++I) {
    const auto &Section = Context.Sections[I];
    if (Section.Type != llvm::ELF::SHT_NOBITS && !Section.Content.empty() &&
        !copyAt(Bytes, Section.Offset, Section.Content))
      return linkFail("ELF section content is outside output");
    const uint64_t Offset =
        Context.SectionHeaderOffset + I * Context.SectionHeaderSize;
    put(Bytes, Offset, Context.SectionNameOffsets[I], 4, Context.Endian);
    put(Bytes, Offset + 4, Section.Type, 4, Context.Endian);
    if (Context.Wide) {
      put(Bytes, Offset + 8, Section.Flags, 8, Context.Endian);
      put(Bytes, Offset + 16, Section.Address, 8, Context.Endian);
      put(Bytes, Offset + 24, Section.Offset, 8, Context.Endian);
      put(Bytes, Offset + 32, Section.Size, 8, Context.Endian);
      put(Bytes, Offset + 40, Section.Link, 4, Context.Endian);
      put(Bytes, Offset + 44, Section.Info, 4, Context.Endian);
      put(Bytes, Offset + 48, Section.Alignment, 8, Context.Endian);
      put(Bytes, Offset + 56, Section.EntrySize, 8, Context.Endian);
    } else {
      put(Bytes, Offset + 8, Section.Flags, 4, Context.Endian);
      put(Bytes, Offset + 12, Section.Address, 4, Context.Endian);
      put(Bytes, Offset + 16, Section.Offset, 4, Context.Endian);
      put(Bytes, Offset + 20, Section.Size, 4, Context.Endian);
      put(Bytes, Offset + 24, Section.Link, 4, Context.Endian);
      put(Bytes, Offset + 28, Section.Info, 4, Context.Endian);
      put(Bytes, Offset + 32, Section.Alignment, 4, Context.Endian);
      put(Bytes, Offset + 36, Section.EntrySize, 4, Context.Endian);
    }
  }
  if (!Output.write(Bytes))
    return linkFail("failed to write ELF output", nullptr, std::nullopt,
                    std::nullopt, DiagnosticKind::IO);
  if (!Output.close())
    return linkFail("failed to close ELF output", nullptr, std::nullopt,
                    std::nullopt, DiagnosticKind::IO);
  return {};
}

} // namespace

Expect<void> ELFWriter::layout(LinkGraph &Graph) noexcept {
  try {
    if (Graph.format() != ObjectFormat::ELF || Graph.relocationsApplied() ||
        !Graph.validate() || machine(Graph.target()) == llvm::ELF::EM_NONE ||
        (Graph.target() == Target::S390X
             ? Graph.endianness() != Endianness::Big
             : Graph.endianness() != Endianness::Little))
      return fail();
    const uint64_t HeaderSize = is64(Graph.target()) ? 64 : 52;
    const uint64_t ProgramHeaderSize = is64(Graph.target()) ? 56 : 32;
    const uint64_t PageSize = maximumPageSize(Graph.target());
    const auto MaximumProgramHeaders = reservedProgramHeaders(Graph);
    if (!MaximumProgramHeaders)
      return fail();
    uint64_t AddressCursor = 0;
    uint64_t FileCursor = 0;
    uint64_t ProgramHeaderBytes = 0;
    if (!multiply(ProgramHeaderSize, *MaximumProgramHeaders,
                  ProgramHeaderBytes) ||
        !add(HeaderSize, ProgramHeaderBytes, AddressCursor) ||
        !align(AddressCursor, PageSize, AddressCursor))
      return fail();
    FileCursor = AddressCursor;
    const std::array<SectionKind, 5> Kinds{
        SectionKind::Text, SectionKind::ReadOnly, SectionKind::Unwind,
        SectionKind::Data, SectionKind::BSS};
    std::vector<std::pair<uint64_t, uint64_t>> Placements(
        Graph.sections().size());
    for (const auto Kind : Kinds) {
      if (Kind != SectionKind::Text &&
          (!align(AddressCursor, PageSize, AddressCursor) ||
           (Kind != SectionKind::BSS &&
            !align(FileCursor, PageSize, FileCursor))))
        return fail();
      for (const SectionId Id : order(Graph, Kind, Placements)) {
        const auto &SectionValue = Graph.sections()[Id];
        if (!align(AddressCursor, SectionValue.Alignment, AddressCursor) ||
            !align(FileCursor, SectionValue.Alignment, FileCursor))
          return fail();
        const uint64_t Address = AddressCursor;
        const uint64_t FileOffset = FileCursor;
        if (!add(AddressCursor, SectionValue.VirtualSize, AddressCursor) ||
            (Kind != SectionKind::BSS &&
             !add(FileCursor, SectionValue.VirtualSize, FileCursor)) ||
            (!is64(Graph.target()) &&
             (Address > UINT32_MAX || FileOffset > UINT32_MAX ||
              SectionValue.VirtualSize > UINT32_MAX ||
              AddressCursor > UINT32_MAX || FileCursor > UINT32_MAX)))
          return fail();
        Placements[Id] = {Address, FileOffset};
      }
    }
    for (SectionId I = 0; I < Placements.size(); ++I) {
      if (!Graph.setSectionAddress(I, Placements[I].first) ||
          !Graph.setSectionFileOffset(I, Placements[I].second))
        return fail();
    }
    return {};
  } catch (...) {
    return fail();
  }
}

LinkExpect<void> ELFWriter::write(const LinkGraph &Graph,
                                  Writer &Output) noexcept {
  try {
    if (!Graph.relocationsApplied() || Graph.format() != ObjectFormat::ELF)
      return fail();
    for (SectionId I = 0; I < Graph.sections().size(); ++I) {
      const auto &SectionValue = Graph.sections()[I];
      if (SectionValue.Purpose == SectionPurpose::ARMExidx &&
          (SectionValue.Content.size() != SectionValue.VirtualSize ||
           (SectionValue.VirtualSize != 0 &&
            SectionValue.VirtualSize % 8 != 0)))
        return linkFail("invalid ARM exception index section", &SectionValue,
                        I);
    }
    if (!Graph.validate() || !validELFFlags(Graph))
      return fail();
    for (const auto &RebaseValue : Graph.rebases())
      if (!validRebase(Graph, RebaseValue))
        return fail();
    const bool Wide = is64(Graph.target());
    const uint64_t ClassMaximum = Wide ? UINT64_MAX : UINT32_MAX;
    const uint8_t AddressSize = Wide ? 8 : 4;
    const uint64_t ELFHeaderSize = Wide ? 64 : 52;
    const uint64_t ProgramHeaderSize = Wide ? 56 : 32;
    const uint64_t SectionHeaderSize = Wide ? 64 : 40;
    const uint64_t SymbolSize = Wide ? 24 : 16;
    const uint64_t RelocationSize = Wide ? 24 : 8;
    const uint64_t DynamicSize = Wide ? 16 : 8;
    const uint64_t PageSize = maximumPageSize(Graph.target());
    const auto Endian = Graph.endianness();
    constexpr uint64_t GeneratedSectionCount = 8;
    uint64_t MaximumSectionCount = 0;
    if (!add(Graph.sections().size(), GeneratedSectionCount,
             MaximumSectionCount) ||
        MaximumSectionCount > UINT16_MAX ||
        Graph.symbols().size() > UINT32_MAX ||
        Graph.rebases().size() > UINT32_MAX)
      return fail();
    if (!Wide) {
      for (const auto &SectionValue : Graph.sections()) {
        uint64_t End = 0;
        if (SectionValue.Address > UINT32_MAX ||
            SectionValue.FileOffset > UINT32_MAX ||
            SectionValue.VirtualSize > UINT32_MAX ||
            SectionValue.Content.size() > UINT32_MAX ||
            !add(SectionValue.Address, SectionValue.VirtualSize, End) ||
            End > UINT32_MAX)
          return fail();
      }
      for (const auto &SymbolValue : Graph.symbols()) {
        uint64_t Address = 0;
        if (SymbolValue.Size > UINT32_MAX ||
            !add(Graph.sections()[SymbolValue.Section].Address,
                 SymbolValue.Offset, Address) ||
            Address > UINT32_MAX)
          return fail();
      }
    }

    std::vector<OutputSection> Sections(1);
    std::vector<uint32_t> GraphSectionIndices(Graph.sections().size());
    for (SectionId I = 0; I < Graph.sections().size(); ++I) {
      const auto &Input = Graph.sections()[I];
      GraphSectionIndices[I] = static_cast<uint32_t>(Sections.size());
      const bool ARMExidx = Input.Purpose == SectionPurpose::ARMExidx;
      Sections.push_back(OutputSection{
          Input.Name,
          ARMExidx                         ? llvm::ELF::SHT_ARM_EXIDX
          : Input.Kind == SectionKind::BSS ? llvm::ELF::SHT_NOBITS
                                           : llvm::ELF::SHT_PROGBITS,
          ARMExidx ? llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_LINK_ORDER
                   : sectionFlags(Input.Kind),
          Input.Address, Input.FileOffset, Input.VirtualSize, 0, 0,
          Input.Alignment, 0, Input.Content});
    }
    for (size_t I = 0; I < Graph.sections().size(); ++I) {
      if (Graph.sections()[I].Purpose != SectionPurpose::ARMExidx)
        continue;
      if (!Graph.sections()[I].LinkedSection)
        return fail();
      Sections[GraphSectionIndices[I]].Link =
          GraphSectionIndices[*Graph.sections()[I].LinkedSection];
    }

    uint64_t AddressCursor = 0;
    uint64_t FileCursor = 0;
    for (const auto &SectionValue : Graph.sections()) {
      if (!add(std::max(AddressCursor, SectionValue.Address),
               SectionValue.VirtualSize, AddressCursor))
        return fail();
      if (SectionValue.Kind != SectionKind::BSS &&
          !add(std::max(FileCursor, SectionValue.FileOffset),
               SectionValue.VirtualSize, FileCursor))
        return fail();
    }
    if (!boundedAlign(AddressCursor, PageSize, ClassMaximum, AddressCursor) ||
        !boundedAlign(FileCursor, PageSize, ClassMaximum, FileCursor))
      return fail();

    std::vector<const Section *> EHSections;
    for (const auto &SectionValue : Graph.sections())
      if (SectionValue.Purpose == SectionPurpose::EHFrame)
        EHSections.push_back(&SectionValue);
    uint32_t EHHeaderIndex = 0;
    if (!EHSections.empty()) {
      std::vector<FDEInfo> FDEs;
      for (const auto *EH : EHSections) {
        auto Parsed = parseEHFrame(Graph, *EH, Endian, FDEs);
        if (!Parsed)
          return Unexpected<Diagnostic>(std::move(Parsed.error()));
      }
      if (FDEs.empty())
        return fail();
      std::sort(FDEs.begin(), FDEs.end(),
                [](const auto &Left, const auto &Right) {
                  return std::tie(Left.Function, Left.Address) <
                         std::tie(Right.Function, Right.Address);
                });
      constexpr uint64_t EHHeaderPrefixSize = 12;
      constexpr uint64_t EHHeaderEntrySize = 8;
      uint64_t EHHeaderSize = 0;
      uint64_t EHTableSize = 0;
      if (!multiply(FDEs.size(), EHHeaderEntrySize, EHTableSize) ||
          !add(EHHeaderPrefixSize, EHTableSize, EHHeaderSize) ||
          FDEs.size() > UINT32_MAX)
        return fail();
      OutputSection Header{".eh_frame_hdr",
                           llvm::ELF::SHT_PROGBITS,
                           llvm::ELF::SHF_ALLOC,
                           AddressCursor,
                           FileCursor,
                           EHHeaderSize,
                           0,
                           0,
                           4,
                           0,
                           std::vector<Byte>(EHHeaderSize)};
      Header.Content[0] = 1;
      Header.Content[1] =
          llvm::dwarf::DW_EH_PE_pcrel | llvm::dwarf::DW_EH_PE_sdata4;
      Header.Content[2] = llvm::dwarf::DW_EH_PE_udata4;
      Header.Content[3] =
          llvm::dwarf::DW_EH_PE_datarel | llvm::dwarf::DW_EH_PE_sdata4;
      int32_t Delta = 0;
      const uint64_t EHAddress =
          std::min_element(EHSections.begin(), EHSections.end(),
                           [](const auto *Left, const auto *Right) {
                             return Left->Address < Right->Address;
                           })[0]
              ->Address;
      if (!signedDelta(EHAddress, Header.Address + 4, Delta))
        return fail();
      put(Header.Content, 4, static_cast<uint32_t>(Delta), 4, Endian);
      put(Header.Content, 8, FDEs.size(), 4, Endian);
      for (size_t I = 0; I < FDEs.size(); ++I) {
        if (!signedDelta(FDEs[I].Function, Header.Address, Delta))
          return fail();
        put(Header.Content, EHHeaderPrefixSize + I * EHHeaderEntrySize,
            static_cast<uint32_t>(Delta), 4, Endian);
        if (!signedDelta(FDEs[I].Address, Header.Address, Delta))
          return fail();
        put(Header.Content, EHHeaderPrefixSize + I * EHHeaderEntrySize + 4,
            static_cast<uint32_t>(Delta), 4, Endian);
      }
      EHHeaderIndex = static_cast<uint32_t>(Sections.size());
      Sections.push_back(std::move(Header));
      if (!boundedAdd(AddressCursor, EHHeaderSize, ClassMaximum,
                      AddressCursor) ||
          !boundedAdd(FileCursor, EHHeaderSize, ClassMaximum, FileCursor))
        return fail();
    }

    std::vector<const Symbol *> Exports;
    for (const auto &SymbolValue : Graph.symbols())
      if (SymbolValue.Exported)
        Exports.push_back(&SymbolValue);
    std::sort(Exports.begin(), Exports.end(),
              [](const auto *Left, const auto *Right) {
                const auto &L =
                    Left->ExportName ? *Left->ExportName : Left->Name;
                const auto &R =
                    Right->ExportName ? *Right->ExportName : Right->Name;
                return L < R;
              });
    for (size_t I = 1; I < Exports.size(); ++I) {
      const auto &Left = Exports[I - 1]->ExportName
                             ? *Exports[I - 1]->ExportName
                             : Exports[I - 1]->Name;
      const auto &Right =
          Exports[I]->ExportName ? *Exports[I]->ExportName : Exports[I]->Name;
      if (Left == Right)
        return fail();
    }

    std::vector<Byte> DynStr(1);
    std::vector<uint32_t> NameOffsets;
    for (const auto *SymbolValue : Exports) {
      const auto &Name = SymbolValue->ExportName ? *SymbolValue->ExportName
                                                 : SymbolValue->Name;
      uint64_t NewSize = 0;
      if (!add(DynStr.size(), Name.size(), NewSize) ||
          !add(NewSize, 1, NewSize) || NewSize > UINT32_MAX ||
          !vectorSize(NewSize))
        return fail();
      NameOffsets.push_back(static_cast<uint32_t>(DynStr.size()));
      DynStr.insert(DynStr.end(), Name.begin(), Name.end());
      DynStr.push_back(0);
    }
    if (!boundedAlign(AddressCursor, AddressSize, ClassMaximum,
                      AddressCursor) ||
        !boundedAlign(FileCursor, AddressSize, ClassMaximum, FileCursor))
      return fail();
    const uint32_t DynStrIndex = static_cast<uint32_t>(Sections.size());
    Sections.push_back(OutputSection{
        ".dynstr", llvm::ELF::SHT_STRTAB, llvm::ELF::SHF_ALLOC, AddressCursor,
        FileCursor, DynStr.size(), 0, 0, 1, 0, std::move(DynStr)});
    if (!boundedAdd(AddressCursor, Sections.back().Size, ClassMaximum,
                    AddressCursor) ||
        !boundedAdd(FileCursor, Sections.back().Size, ClassMaximum, FileCursor))
      return fail();

    if (!boundedAlign(AddressCursor, AddressSize, ClassMaximum,
                      AddressCursor) ||
        !boundedAlign(FileCursor, AddressSize, ClassMaximum, FileCursor))
      return fail();
    uint64_t SymbolCount = 0;
    uint64_t DynSymSize = 0;
    if (!add(Exports.size(), 1, SymbolCount) || SymbolCount > UINT32_MAX ||
        !multiply(SymbolCount, SymbolSize, DynSymSize) ||
        !vectorSize(DynSymSize))
      return fail();
    std::vector<Byte> DynSym(static_cast<size_t>(DynSymSize));
    for (size_t I = 0; I < Exports.size(); ++I) {
      const auto &SymbolValue = *Exports[I];
      const uint64_t SymbolAddress =
          (Graph.sections()[SymbolValue.Section].Address + SymbolValue.Offset) |
          static_cast<uint64_t>(SymbolValue.Thumb);
      const uint64_t Offset = (I + 1) * SymbolSize;
      const uint8_t Type = static_cast<uint8_t>(
          Graph.sections()[SymbolValue.Section].Kind == SectionKind::Text
              ? llvm::ELF::STT_FUNC
              : llvm::ELF::STT_OBJECT);
      if (Wide) {
        put(DynSym, Offset, NameOffsets[I], 4, Endian);
        DynSym[Offset + 4] = llvm::ELF::STB_GLOBAL << 4 | Type;
        DynSym[Offset + 5] = llvm::ELF::STV_DEFAULT;
        put(DynSym, Offset + 6, GraphSectionIndices[SymbolValue.Section], 2,
            Endian);
        put(DynSym, Offset + 8, SymbolAddress, 8, Endian);
        put(DynSym, Offset + 16, SymbolValue.Size, 8, Endian);
      } else {
        put(DynSym, Offset, NameOffsets[I], 4, Endian);
        put(DynSym, Offset + 4, SymbolAddress, 4, Endian);
        put(DynSym, Offset + 8, SymbolValue.Size, 4, Endian);
        DynSym[Offset + 12] = llvm::ELF::STB_GLOBAL << 4 | Type;
        DynSym[Offset + 13] = llvm::ELF::STV_DEFAULT;
        put(DynSym, Offset + 14, GraphSectionIndices[SymbolValue.Section], 2,
            Endian);
      }
    }
    const uint32_t DynSymIndex = static_cast<uint32_t>(Sections.size());
    Sections.push_back(
        OutputSection{".dynsym", llvm::ELF::SHT_DYNSYM, llvm::ELF::SHF_ALLOC,
                      AddressCursor, FileCursor, DynSym.size(), DynStrIndex, 1,
                      AddressSize, SymbolSize, std::move(DynSym)});
    if (!boundedAdd(AddressCursor, Sections.back().Size, ClassMaximum,
                    AddressCursor) ||
        !boundedAdd(FileCursor, Sections.back().Size, ClassMaximum, FileCursor))
      return fail();

    if (!boundedAlign(AddressCursor, 4, ClassMaximum, AddressCursor) ||
        !boundedAlign(FileCursor, 4, ClassMaximum, FileCursor))
      return fail();
    const uint32_t BucketCount =
        std::max<uint32_t>(1, static_cast<uint32_t>(Exports.size()));
    uint64_t HashWords = 0;
    uint64_t HashSize = 0;
    if (!add(2, BucketCount, HashWords) ||
        !add(HashWords, SymbolCount, HashWords) ||
        !multiply(HashWords, 4, HashSize) || !vectorSize(HashSize))
      return fail();
    std::vector<Byte> Hash(static_cast<size_t>(HashSize));
    put(Hash, 0, BucketCount, 4, Endian);
    put(Hash, 4, Exports.size() + 1, 4, Endian);
    for (size_t I = 0; I < Exports.size(); ++I) {
      const auto &Name =
          Exports[I]->ExportName ? *Exports[I]->ExportName : Exports[I]->Name;
      const uint32_t Index = static_cast<uint32_t>(I + 1);
      const uint32_t Bucket = hash(Name) % BucketCount;
      const uint64_t BucketOffset = (UINT64_C(2) + Bucket) * UINT64_C(4);
      const uint32_t Previous =
          static_cast<uint32_t>(get(Hash, BucketOffset, 4, Endian));
      put(Hash, BucketOffset, Index, 4, Endian);
      if (Previous != 0)
        put(Hash, (UINT64_C(2) + BucketCount + Index) * UINT64_C(4), Previous,
            4, Endian);
    }
    const uint32_t HashIndex = static_cast<uint32_t>(Sections.size());
    Sections.push_back(OutputSection{
        ".hash", llvm::ELF::SHT_HASH, llvm::ELF::SHF_ALLOC, AddressCursor,
        FileCursor, Hash.size(), DynSymIndex, 0, 4, 4, std::move(Hash)});
    if (!boundedAdd(AddressCursor, Sections.back().Size, ClassMaximum,
                    AddressCursor) ||
        !boundedAdd(FileCursor, Sections.back().Size, ClassMaximum, FileCursor))
      return fail();

    if (!boundedAlign(AddressCursor, AddressSize, ClassMaximum,
                      AddressCursor) ||
        !boundedAlign(FileCursor, AddressSize, ClassMaximum, FileCursor))
      return fail();
    uint64_t RelocationsSize = 0;
    if (!multiply(Graph.rebases().size(), RelocationSize, RelocationsSize) ||
        !vectorSize(RelocationsSize))
      return fail();
    std::vector<Byte> Relocations(static_cast<size_t>(RelocationsSize));
    for (size_t I = 0; I < Graph.rebases().size(); ++I) {
      const auto &RebaseValue = Graph.rebases()[I];
      const auto &TargetSection = Graph.sections()[RebaseValue.Section];
      if ((sectionFlags(TargetSection.Kind) & llvm::ELF::SHF_WRITE) == 0) {
        Diagnostic Diag("ELF rebase targets non-writable section");
        Diag.Section = RebaseValue.Section;
        Diag.SectionName = TargetSection.Name;
        Diag.RelocationType = RebaseValue.Type;
        Diag.Offset = RebaseValue.Offset;
        Diag.Kind = DiagnosticKind::Unsupported;
        return Unexpected<Diagnostic>(std::move(Diag));
      }
      const uint64_t Address = TargetSection.Address + RebaseValue.Offset;
      const uint64_t EntryOffset = I * RelocationSize;
      put(Relocations, EntryOffset, Address, AddressSize, Endian);
      if (Wide) {
        put(Relocations, EntryOffset + 8, relativeType(Graph.target()), 8,
            Endian);
        put(Relocations, EntryOffset + 16,
            get(TargetSection.Content, RebaseValue.Offset, AddressSize, Endian),
            8, Endian);
      } else {
        put(Relocations, EntryOffset + 4, relativeType(Graph.target()), 4,
            Endian);
      }
    }
    const uint32_t RelocationIndex = static_cast<uint32_t>(Sections.size());
    Sections.push_back(OutputSection{
        Wide ? ".rela.dyn" : ".rel.dyn",
        Wide ? llvm::ELF::SHT_RELA : llvm::ELF::SHT_REL, llvm::ELF::SHF_ALLOC,
        AddressCursor, FileCursor, Relocations.size(), DynSymIndex, 0,
        AddressSize, RelocationSize, std::move(Relocations)});
    if (!boundedAdd(AddressCursor, Sections.back().Size, ClassMaximum,
                    AddressCursor) ||
        !boundedAdd(FileCursor, Sections.back().Size, ClassMaximum, FileCursor))
      return fail();

    if (!boundedAlign(AddressCursor, PageSize, ClassMaximum, AddressCursor) ||
        !boundedAlign(FileCursor, PageSize, ClassMaximum, FileCursor))
      return fail();
    constexpr size_t DynamicTagCount = 10;
    uint64_t DynamicBytes = 0;
    if (!multiply(DynamicTagCount, DynamicSize, DynamicBytes) ||
        !vectorSize(DynamicBytes))
      return fail();
    std::vector<Byte> Dynamic(static_cast<size_t>(DynamicBytes));
    size_t DynamicIndex = 0;
    auto AddDynamic = [&](int64_t Tag, uint64_t Value) {
      const uint64_t Offset = DynamicIndex++ * DynamicSize;
      put(Dynamic, Offset, static_cast<uint64_t>(Tag), AddressSize, Endian);
      put(Dynamic, Offset + AddressSize, Value, AddressSize, Endian);
    };
    AddDynamic(llvm::ELF::DT_HASH, Sections[HashIndex].Address);
    AddDynamic(llvm::ELF::DT_STRTAB, Sections[DynStrIndex].Address);
    AddDynamic(llvm::ELF::DT_STRSZ, Sections[DynStrIndex].Size);
    AddDynamic(llvm::ELF::DT_SYMTAB, Sections[DynSymIndex].Address);
    AddDynamic(llvm::ELF::DT_SYMENT, SymbolSize);
    AddDynamic(Wide ? llvm::ELF::DT_RELA : llvm::ELF::DT_REL,
               Sections[RelocationIndex].Address);
    AddDynamic(Wide ? llvm::ELF::DT_RELASZ : llvm::ELF::DT_RELSZ,
               Sections[RelocationIndex].Size);
    AddDynamic(Wide ? llvm::ELF::DT_RELAENT : llvm::ELF::DT_RELENT,
               RelocationSize);
    AddDynamic(llvm::ELF::DT_NULL, 0);
    const uint32_t DynamicSectionIndex = static_cast<uint32_t>(Sections.size());
    Sections.push_back(
        OutputSection{".dynamic", llvm::ELF::SHT_DYNAMIC,
                      llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_WRITE,
                      AddressCursor, FileCursor, Dynamic.size(), DynStrIndex, 0,
                      AddressSize, DynamicSize, std::move(Dynamic)});
    if (!boundedAdd(AddressCursor, Sections.back().Size, ClassMaximum,
                    AddressCursor) ||
        !boundedAdd(FileCursor, Sections.back().Size, ClassMaximum, FileCursor))
      return fail();

    std::vector<Byte> SectionNames(1);
    std::vector<uint32_t> SectionNameOffsets(Sections.size());
    for (size_t I = 1; I < Sections.size(); ++I) {
      uint64_t NewSize = 0;
      if (!add(SectionNames.size(), Sections[I].Name.size(), NewSize) ||
          !add(NewSize, 1, NewSize) || NewSize > UINT32_MAX ||
          !vectorSize(NewSize))
        return fail();
      SectionNameOffsets[I] = static_cast<uint32_t>(SectionNames.size());
      SectionNames.insert(SectionNames.end(), Sections[I].Name.begin(),
                          Sections[I].Name.end());
      SectionNames.push_back(0);
    }
    const uint32_t SectionNameIndex = static_cast<uint32_t>(Sections.size());
    SectionNameOffsets.push_back(static_cast<uint32_t>(SectionNames.size()));
    constexpr std::string_view SectionName = ".shstrtab";
    uint64_t FinalNameSize = 0;
    if (!add(SectionNames.size(), SectionName.size(), FinalNameSize) ||
        !add(FinalNameSize, 1, FinalNameSize) || FinalNameSize > UINT32_MAX ||
        !vectorSize(FinalNameSize))
      return fail();
    SectionNames.insert(SectionNames.end(), SectionName.begin(),
                        SectionName.end());
    SectionNames.push_back(0);
    if (!boundedAlign(FileCursor, 1, ClassMaximum, FileCursor))
      return fail();
    Sections.push_back(OutputSection{".shstrtab", llvm::ELF::SHT_STRTAB, 0, 0,
                                     FileCursor, SectionNames.size(), 0, 0, 1,
                                     0, std::move(SectionNames)});
    if (!boundedAdd(FileCursor, Sections.back().Size, ClassMaximum, FileCursor))
      return fail();
    uint64_t SectionHeaderOffset = 0;
    if (!boundedAlign(FileCursor, AddressSize, ClassMaximum,
                      SectionHeaderOffset))
      return fail();
    uint64_t FileSize = 0;
    uint64_t SectionHeadersSize = 0;
    if (!multiply(Sections.size(), SectionHeaderSize, SectionHeadersSize) ||
        !boundedAdd(SectionHeaderOffset, SectionHeadersSize, ClassMaximum,
                    FileSize))
      return fail();
#if SIZE_MAX < UINT64_MAX
    if (FileSize > SIZE_MAX)
      return fail();
#endif

    std::vector<OutputSegment> Segments;
    const auto ReservedHeaders = reservedProgramHeaders(Graph);
    if (!ReservedHeaders)
      return fail();
    Segments.push_back({llvm::ELF::PT_LOAD, llvm::ELF::PF_R, 0, 0,
                        ELFHeaderSize + ProgramHeaderSize * *ReservedHeaders,
                        ELFHeaderSize + ProgramHeaderSize * *ReservedHeaders,
                        PageSize});
    std::vector<const OutputSection *> Allocated;
    for (const auto &SectionValue : Sections)
      if ((SectionValue.Flags & llvm::ELF::SHF_ALLOC) != 0 &&
          SectionValue.Size != 0)
        Allocated.push_back(&SectionValue);
    std::sort(Allocated.begin(), Allocated.end(),
              [](const auto *Left, const auto *Right) {
                return std::tie(Left->Address, Left->Name) <
                       std::tie(Right->Address, Right->Name);
              });
    auto SegmentFlags = [](const OutputSection &Value) {
      return llvm::ELF::PF_R |
             ((Value.Flags & llvm::ELF::SHF_EXECINSTR) != 0 ? llvm::ELF::PF_X
                                                            : uint32_t{0}) |
             ((Value.Flags & llvm::ELF::SHF_WRITE) != 0 ? llvm::ELF::PF_W
                                                        : uint32_t{0});
    };
    uint64_t PreviousRoundedEnd = PageSize;
    for (size_t Begin = 0; Begin < Allocated.size();) {
      const uint32_t Flags = SegmentFlags(*Allocated[Begin]);
      size_t End = Begin + 1;
      uint64_t Alignment = PageSize;
      const auto *First = Allocated[Begin];
      std::optional<uint64_t> FileBias;
      bool HasNOBITS = false;
      for (; End <= Allocated.size(); ++End) {
        const auto *SectionValue = Allocated[End - 1];
        const uint64_t CandidateAlignment =
            std::max(Alignment, SectionValue->Alignment);
        const uint64_t CandidateAddress =
            First->Address & ~(CandidateAlignment - 1);
        if (End - 1 != Begin &&
            (SegmentFlags(*SectionValue) != Flags ||
             CandidateAddress < PreviousRoundedEnd ||
             (HasNOBITS && SectionValue->Type != llvm::ELF::SHT_NOBITS)))
          break;
        if (SectionValue->Type != llvm::ELF::SHT_NOBITS) {
          if (SectionValue->Address < SectionValue->Offset)
            return fail();
          const uint64_t Bias = SectionValue->Address - SectionValue->Offset;
          if (FileBias && *FileBias != Bias)
            break;
          FileBias = Bias;
        } else {
          HasNOBITS = true;
        }
        Alignment = CandidateAlignment;
        if (End == Allocated.size()) {
          ++End;
          break;
        }
      }
      --End;
      const uint64_t Address = First->Address & ~(Alignment - 1);
      const uint64_t Prefix = First->Address - Address;
      if (First->Offset < Prefix)
        return fail();
      const uint64_t Offset = First->Offset - Prefix;
      uint64_t FileEnd = Offset;
      uint64_t MemoryEnd = Address;
      for (size_t I = Begin; I < End; ++I) {
        const auto *SectionValue = Allocated[I];
        MemoryEnd =
            std::max(MemoryEnd, SectionValue->Address + SectionValue->Size);
        if (SectionValue->Type != llvm::ELF::SHT_NOBITS)
          FileEnd =
              std::max(FileEnd, SectionValue->Offset + SectionValue->Size);
      }
      if (Offset % Alignment != Address % Alignment || FileEnd < Offset ||
          MemoryEnd < Address)
        return fail();
      Segments.push_back({llvm::ELF::PT_LOAD, Flags, Offset, Address,
                          FileEnd - Offset, MemoryEnd - Address, Alignment});
      if (!align(MemoryEnd, PageSize, PreviousRoundedEnd))
        return fail();
      Begin = End;
    }
    Segments.push_back({llvm::ELF::PT_DYNAMIC,
                        llvm::ELF::PF_R | llvm::ELF::PF_W,
                        Sections[DynamicSectionIndex].Offset,
                        Sections[DynamicSectionIndex].Address,
                        Sections[DynamicSectionIndex].Size,
                        Sections[DynamicSectionIndex].Size, AddressSize});
    if (EHHeaderIndex != 0)
      Segments.push_back(
          {llvm::ELF::PT_GNU_EH_FRAME, llvm::ELF::PF_R,
           Sections[EHHeaderIndex].Offset, Sections[EHHeaderIndex].Address,
           Sections[EHHeaderIndex].Size, Sections[EHHeaderIndex].Size, 4});
    std::vector<const OutputSection *> ExidxSections;
    for (const auto &SectionValue : Sections)
      if (SectionValue.Type == llvm::ELF::SHT_ARM_EXIDX &&
          SectionValue.Size != 0)
        ExidxSections.push_back(&SectionValue);
    std::sort(ExidxSections.begin(), ExidxSections.end(),
              [](const auto *Left, const auto *Right) {
                return std::tie(Left->Address, Left->Offset) <
                       std::tie(Right->Address, Right->Offset);
              });
    uint64_t ExidxAddressEnd = 0;
    uint64_t ExidxFileEnd = 0;
    uint64_t ExidxAlignment = 1;
    for (size_t I = 0; I < ExidxSections.size(); ++I) {
      const auto &SectionValue = *ExidxSections[I];
      uint64_t AddressEnd = 0;
      uint64_t FileEnd = 0;
      if (!boundedAdd(SectionValue.Address, SectionValue.Size, ClassMaximum,
                      AddressEnd) ||
          !boundedAdd(SectionValue.Offset, SectionValue.Size, ClassMaximum,
                      FileEnd))
        return fail();
      if (I != 0) {
        const auto &First = *ExidxSections.front();
        if ((First.Address >= First.Offset) !=
                (SectionValue.Address >= SectionValue.Offset) ||
            (First.Address >= First.Offset
                 ? First.Address - First.Offset !=
                       SectionValue.Address - SectionValue.Offset
                 : First.Offset - First.Address !=
                       SectionValue.Offset - SectionValue.Address) ||
            SectionValue.Address != ExidxAddressEnd ||
            SectionValue.Offset != ExidxFileEnd)
          return fail();
      }
      ExidxAddressEnd = AddressEnd;
      ExidxFileEnd = FileEnd;
      ExidxAlignment = std::max(ExidxAlignment, SectionValue.Alignment);
    }
    if (!ExidxSections.empty()) {
      const auto &First = *ExidxSections.front();
      if (First.Offset % ExidxAlignment != First.Address % ExidxAlignment)
        return fail();
      Segments.push_back({llvm::ELF::PT_ARM_EXIDX, llvm::ELF::PF_R,
                          First.Offset, First.Address,
                          ExidxFileEnd - First.Offset,
                          ExidxAddressEnd - First.Address, ExidxAlignment});
    }
    Segments.push_back({llvm::ELF::PT_GNU_STACK,
                        llvm::ELF::PF_R | llvm::ELF::PF_W, 0, 0, 0, 0,
                        AddressSize});
    if (Segments.size() > *ReservedHeaders || Segments.size() > UINT16_MAX)
      return fail();

    return emitELF(ELFEmissionContext{Graph, Sections, SectionNameOffsets,
                                      Segments, FileSize, ELFHeaderSize,
                                      ProgramHeaderSize, SectionHeaderSize,
                                      SectionHeaderOffset, SectionNameIndex,
                                      Wide, Endian},
                   Output);
  } catch (...) {
    return fail();
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
