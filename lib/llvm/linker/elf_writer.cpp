// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/elf_writer.h"

#include "linker/byte_io.h"
#include "linker/eh_frame.h"
#include "linker/layout.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using Internal::addUnsigned;
using Internal::checkedAlign;
using Internal::checkedMul;
using Internal::fitsSizeT;
using Internal::getInteger;
using Internal::Placement;
using Internal::putInteger;

// File, program, section, dynamic, and symbol table layouts follow the System
// V ELF gABI; relocation values and e_flags follow each architecture psABI.
constexpr uint8_t EncodingFormatMask = 0x0F;
constexpr uint8_t EncodingApplicationMask = 0x70;
constexpr uint32_t ExtendedRecordLength = UINT32_MAX;
constexpr uint64_t FixedProgramHeaderCount = 6;
constexpr uint64_t GeneratedSectionCount = 8;
constexpr uint8_t EHFrameHeaderVersion = 1;
constexpr uint64_t EHFrameHeaderPrefixSize = 12;
constexpr uint64_t EHFrameHeaderEntrySize = 8;
constexpr uint64_t EHFrameHeaderAlignment = 4;
constexpr uint64_t HashWordSize = 4;
constexpr uint64_t HashHeaderWords = 2;
constexpr size_t DynamicTagCount = 10;

struct ELFClass {
  bool Wide;
  uint8_t AddressSize;
  uint64_t Maximum;
  uint64_t HeaderSize;
  uint64_t ProgramHeaderSize;
  uint64_t SectionHeaderSize;
  uint64_t SymbolSize;
  uint64_t RelocationSize;
  uint64_t DynamicSize;
};

bool is64(Target Value) noexcept { return Value != Target::ARM; }

ELFClass elfClass(Target Value) noexcept {
  if (is64(Value))
    return ELFClass{true,
                    sizeof(llvm::ELF::Elf64_Addr),
                    UINT64_MAX,
                    sizeof(llvm::ELF::Elf64_Ehdr),
                    sizeof(llvm::ELF::Elf64_Phdr),
                    sizeof(llvm::ELF::Elf64_Shdr),
                    sizeof(llvm::ELF::Elf64_Sym),
                    sizeof(llvm::ELF::Elf64_Rela),
                    sizeof(llvm::ELF::Elf64_Dyn)};
  return ELFClass{false,
                  sizeof(llvm::ELF::Elf32_Addr),
                  UINT32_MAX,
                  sizeof(llvm::ELF::Elf32_Ehdr),
                  sizeof(llvm::ELF::Elf32_Phdr),
                  sizeof(llvm::ELF::Elf32_Shdr),
                  sizeof(llvm::ELF::Elf32_Sym),
                  sizeof(llvm::ELF::Elf32_Rel),
                  sizeof(llvm::ELF::Elf32_Dyn)};
}

bool boundedAdd(uint64_t Value, uint64_t Increment, uint64_t Maximum,
                uint64_t &Result) noexcept {
  return addUnsigned(Value, Increment, Result) && Result <= Maximum;
}

bool boundedAlign(uint64_t Value, uint64_t Alignment, uint64_t Maximum,
                  uint64_t &Result) noexcept {
  return checkedAlign(Value, Alignment, Result) && Result <= Maximum;
}

bool signedDelta32(uint64_t Target, uint64_t Base, int32_t &Result) noexcept {
  int64_t Delta = 0;
  if (!Internal::signedDelta(Target, Base, Delta) || !llvm::isInt<32>(Delta))
    return false;
  Result = static_cast<int32_t>(Delta);
  return true;
}

Expect<void> copyAt(Span<Byte> Bytes, uint64_t Offset,
                    Span<const Byte> Content) {
  if (Offset > Bytes.size() || Content.size() > Bytes.size() - Offset)
    return Unexpect(ErrCode::Value::IllegalPath);
  Bytes = Bytes.subspan(static_cast<size_t>(Offset), Content.size());
  std::copy(Content.begin(), Content.end(), Bytes.begin());
  return {};
}

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
  const uint8_t Width = elfClass(Graph.target()).AddressSize;
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
  uint64_t Result = 0;
  if (!addUnsigned(Graph.sections().size(), FixedProgramHeaderCount, Result) ||
      Result > UINT16_MAX)
    return std::nullopt;
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

bool decodeFDEAddress(Span<const Byte> Bytes, size_t Offset, uint8_t Encoding,
                      uint64_t FieldAddress, Endianness Endian,
                      uint64_t &Result) noexcept {
  const uint8_t Width = encodingWidth(Encoding);
  if (Width == 0)
    return false;
  const bool Signed = (Encoding & llvm::dwarf::DW_EH_PE_signed) != 0;
  if (Offset > Bytes.size() || Width > Bytes.size() - Offset)
    return false;
  const uint64_t Raw = getInteger(Bytes, Offset, Width, Endian);
  int64_t Value = static_cast<int64_t>(Raw);
  if (Signed && Width < 8 && (Raw & (UINT64_C(1) << (Width * 8 - 1))) != 0)
    Value = static_cast<int64_t>(Raw | (~UINT64_C(0) << (Width * 8)));
  if ((Encoding & EncodingApplicationMask) == llvm::dwarf::DW_EH_PE_pcrel)
    return Internal::addSigned(FieldAddress, Value, Result);
  if ((Encoding & EncodingApplicationMask) != llvm::dwarf::DW_EH_PE_absptr ||
      Value < 0)
    return false;
  Result = static_cast<uint64_t>(Value);
  return true;
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
  const uint8_t PointerWidth = elfClass(Graph.target()).AddressSize;
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

LinkExpect<void> parseEHFrame(const LinkGraph &Graph, SectionId EHId,
                              std::vector<FDEInfo> &FDEs) noexcept {
  const auto &EH = Graph.sections()[EHId];
  const auto Endian = Graph.endianness();
  const Span<const Byte> Bytes(EH.Content.data(), EH.Content.size());
  auto Malformed = [&](std::string Message, size_t ErrorOffset) {
    return sectionError(std::move(Message), EH, EHId, ErrorOffset);
  };
  auto Unsupported = [&](std::string Message, size_t ErrorOffset) {
    return sectionError(std::move(Message), EH, EHId, ErrorOffset,
                        DiagnosticKind::Unsupported);
  };
  std::map<size_t, CIEInfo> CIEs;
  size_t Offset = 0;
  while (Offset < Bytes.size()) {
    if (Bytes.size() - Offset < Internal::EHFrameLengthFieldSize)
      return Malformed("truncated EH frame record length", Offset);
    const uint64_t Length =
        getInteger(Bytes, Offset, Internal::EHFrameLengthFieldSize, Endian);
    const size_t Body = Offset + Internal::EHFrameLengthFieldSize;
    if (Length == 0)
      return Body == Bytes.size()
                 ? LinkExpect<void>{}
                 : Malformed("EH frame terminator is not final", Offset);
    if (Length == ExtendedRecordLength || Length > Bytes.size() - Body ||
        Length < Internal::EHFrameLengthFieldSize)
      return Length == ExtendedRecordLength
                 ? Unsupported("unsupported extended EH frame record", Offset)
                 : Malformed("invalid EH frame record length", Offset);
    const size_t End = Body + static_cast<size_t>(Length);
    const auto Prefix = Bytes.subspan(0, End);
    const uint64_t Id =
        getInteger(Bytes, Body, Internal::EHFrameLengthFieldSize, Endian);
    const size_t Fields = Offset + Internal::EHFrameRecordHeaderSize;
    if (Id == 0) {
      size_t Cursor = Fields;
      if (Cursor >= End || Bytes[Cursor++] != Internal::EHFrameCIEVersion)
        return Unsupported("unsupported EH frame CIE version", Fields);
      std::string Augmentation;
      while (Cursor < End && Bytes[Cursor] != 0)
        Augmentation.push_back(static_cast<char>(Bytes[Cursor++]));
      if (Cursor >= End || Augmentation.empty() || Augmentation.front() != 'z')
        return Unsupported("unsupported EH frame augmentation", Fields + 1);
      ++Cursor;
      uint64_t Ignored = 0;
      if (!Internal::readULEB128(Prefix, Cursor, Ignored) ||
          !skipSLEB(Prefix, Cursor) ||
          !Internal::readULEB128(Prefix, Cursor, Ignored))
        return Malformed("invalid EH frame CIE alignment fields", Cursor);
      uint64_t AugmentationSize = 0;
      if (!Internal::readULEB128(Prefix, Cursor, AugmentationSize) ||
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
          if (!decodeFDEAddress(Prefix, Cursor, PersonalityEncoding,
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
      const size_t PointerField = Body;
      if (Id > PointerField)
        return Malformed("invalid EH frame CIE pointer", PointerField);
      const auto CIE = CIEs.find(PointerField - static_cast<size_t>(Id));
      if (CIE == CIEs.end())
        return Malformed("EH frame FDE references an unknown CIE",
                         PointerField);
      const size_t InitialLocation = Fields;
      uint64_t Function = 0;
      if (!decodeFDEAddress(Prefix, InitialLocation, CIE->second.Encoding,
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

OutputSection generatedSection(std::string Name, uint32_t Type, uint64_t Flags,
                               uint64_t Alignment, uint64_t EntrySize,
                               std::vector<Byte> Content) {
  OutputSection Result;
  Result.Name = std::move(Name);
  Result.Type = Type;
  Result.Flags = Flags;
  Result.Size = Content.size();
  Result.Alignment = Alignment;
  Result.EntrySize = EntrySize;
  Result.Content = std::move(Content);
  return Result;
}

struct OutputSegment {
  uint32_t Type;
  uint32_t Flags;
  uint64_t Offset;
  uint64_t Address;
  uint64_t FileSize;
  uint64_t MemorySize;
  uint64_t Alignment;
};

struct Cursor {
  uint64_t Address;
  uint64_t File;
  uint64_t Maximum;

  bool align(uint64_t Alignment) noexcept {
    return boundedAlign(Address, Alignment, Maximum, Address) &&
           boundedAlign(File, Alignment, Maximum, File);
  }
  bool advance(uint64_t Size) noexcept {
    return boundedAdd(Address, Size, Maximum, Address) &&
           boundedAdd(File, Size, Maximum, File);
  }
};

LinkExpect<uint32_t> appendSection(std::vector<OutputSection> &Sections,
                                   Cursor &Position, OutputSection Value) {
  const auto Index = static_cast<uint32_t>(Sections.size());
  Value.Address = Position.Address;
  Value.Offset = Position.File;
  const uint64_t Size = Value.Size;
  const std::string Name = Value.Name;
  Sections.push_back(std::move(Value));
  if (!Position.advance(Size))
    return diagnosticError("ELF " + Name + " section exceeds the ELF class");
  return Index;
}

struct ELFEmissionContext {
  const LinkGraph &Graph;
  const ELFClass &Class;
  const std::vector<OutputSection> &Sections;
  Span<const uint32_t> SectionNameOffsets;
  Span<const OutputSegment> Segments;
  uint64_t FileSize;
  uint64_t SectionHeaderOffset;
  uint32_t SectionNameIndex;
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

std::vector<SectionId> order(const LinkGraph &Graph, SectionKind Kind,
                             Span<const Placement> Placements) {
  auto Result = Internal::sectionsOfKind(Graph, Kind);
  if (Kind != SectionKind::Unwind)
    return Result;
  auto LinkedAddress = [&](const Section &Value) {
    return Value.LinkedSection && *Value.LinkedSection < Placements.size()
               ? Placements[*Value.LinkedSection].Address
               : UINT64_MAX;
  };
  std::sort(Result.begin(), Result.end(), [&](SectionId Left, SectionId Right) {
    const auto &L = Graph.sections()[Left];
    const auto &R = Graph.sections()[Right];
    const bool LExidx = L.Purpose == SectionPurpose::ARMExidx;
    const bool RExidx = R.Purpose == SectionPurpose::ARMExidx;
    if (LExidx != RExidx)
      return LExidx;
    if (LExidx)
      return std::tuple(LinkedAddress(L), Left) <
             std::tuple(LinkedAddress(R), Right);
    return std::tie(L.Name, Left) < std::tie(R.Name, Right);
  });
  return Result;
}

class FieldWriter {
public:
  FieldWriter(Span<Byte> Bytes, uint64_t Base, Endianness Endian) noexcept
      : Bytes(Bytes), Base(Base), Endian(Endian) {}

  void operator()(size_t Offset, size_t Width, uint64_t Value) const noexcept {
    putInteger(Bytes, Base + Offset, Value, static_cast<uint8_t>(Width),
               Endian);
  }

private:
  Span<Byte> Bytes;
  uint64_t Base;
  Endianness Endian;
};

template <typename Ehdr>
void putFileHeader(Span<Byte> Bytes, const ELFEmissionContext &Context) {
  const FieldWriter Put(Bytes, 0, Context.Graph.endianness());
  Put(offsetof(Ehdr, e_type), sizeof(Ehdr::e_type), llvm::ELF::ET_DYN);
  Put(offsetof(Ehdr, e_machine), sizeof(Ehdr::e_machine),
      machine(Context.Graph.target()));
  Put(offsetof(Ehdr, e_version), sizeof(Ehdr::e_version),
      llvm::ELF::EV_CURRENT);
  Put(offsetof(Ehdr, e_phoff), sizeof(Ehdr::e_phoff), Context.Class.HeaderSize);
  Put(offsetof(Ehdr, e_shoff), sizeof(Ehdr::e_shoff),
      Context.SectionHeaderOffset);
  Put(offsetof(Ehdr, e_flags), sizeof(Ehdr::e_flags), Context.Graph.elfFlags());
  Put(offsetof(Ehdr, e_ehsize), sizeof(Ehdr::e_ehsize),
      Context.Class.HeaderSize);
  Put(offsetof(Ehdr, e_phentsize), sizeof(Ehdr::e_phentsize),
      Context.Class.ProgramHeaderSize);
  Put(offsetof(Ehdr, e_phnum), sizeof(Ehdr::e_phnum), Context.Segments.size());
  Put(offsetof(Ehdr, e_shentsize), sizeof(Ehdr::e_shentsize),
      Context.Class.SectionHeaderSize);
  Put(offsetof(Ehdr, e_shnum), sizeof(Ehdr::e_shnum), Context.Sections.size());
  Put(offsetof(Ehdr, e_shstrndx), sizeof(Ehdr::e_shstrndx),
      Context.SectionNameIndex);
}

template <typename Phdr>
void putProgramHeader(Span<Byte> Bytes, uint64_t Offset, Endianness Endian,
                      const OutputSegment &Segment) {
  const FieldWriter Put(Bytes, Offset, Endian);
  Put(offsetof(Phdr, p_type), sizeof(Phdr::p_type), Segment.Type);
  Put(offsetof(Phdr, p_flags), sizeof(Phdr::p_flags), Segment.Flags);
  Put(offsetof(Phdr, p_offset), sizeof(Phdr::p_offset), Segment.Offset);
  Put(offsetof(Phdr, p_vaddr), sizeof(Phdr::p_vaddr), Segment.Address);
  Put(offsetof(Phdr, p_paddr), sizeof(Phdr::p_paddr), Segment.Address);
  Put(offsetof(Phdr, p_filesz), sizeof(Phdr::p_filesz), Segment.FileSize);
  Put(offsetof(Phdr, p_memsz), sizeof(Phdr::p_memsz), Segment.MemorySize);
  Put(offsetof(Phdr, p_align), sizeof(Phdr::p_align), Segment.Alignment);
}

template <typename Shdr>
void putSectionHeader(Span<Byte> Bytes, uint64_t Offset, Endianness Endian,
                      const OutputSection &Section, uint32_t NameOffset) {
  const FieldWriter Put(Bytes, Offset, Endian);
  Put(offsetof(Shdr, sh_name), sizeof(Shdr::sh_name), NameOffset);
  Put(offsetof(Shdr, sh_type), sizeof(Shdr::sh_type), Section.Type);
  Put(offsetof(Shdr, sh_flags), sizeof(Shdr::sh_flags), Section.Flags);
  Put(offsetof(Shdr, sh_addr), sizeof(Shdr::sh_addr), Section.Address);
  Put(offsetof(Shdr, sh_offset), sizeof(Shdr::sh_offset), Section.Offset);
  Put(offsetof(Shdr, sh_size), sizeof(Shdr::sh_size), Section.Size);
  Put(offsetof(Shdr, sh_link), sizeof(Shdr::sh_link), Section.Link);
  Put(offsetof(Shdr, sh_info), sizeof(Shdr::sh_info), Section.Info);
  Put(offsetof(Shdr, sh_addralign), sizeof(Shdr::sh_addralign),
      Section.Alignment);
  Put(offsetof(Shdr, sh_entsize), sizeof(Shdr::sh_entsize), Section.EntrySize);
}

template <typename Ehdr, typename Phdr, typename Shdr>
LinkExpect<void> fillImage(Span<Byte> Bytes,
                           const ELFEmissionContext &Context) {
  const auto Endian = Context.Graph.endianness();
  constexpr size_t MagicSize = sizeof(llvm::ELF::ElfMagic) - 1;
  std::copy_n(llvm::ELF::ElfMagic, MagicSize, Bytes.begin());
  Bytes[llvm::ELF::EI_CLASS] = static_cast<uint8_t>(
      Context.Class.Wide ? llvm::ELF::ELFCLASS64 : llvm::ELF::ELFCLASS32);
  Bytes[llvm::ELF::EI_DATA] = static_cast<uint8_t>(
      Endian == Endianness::Little ? llvm::ELF::ELFDATA2LSB
                                   : llvm::ELF::ELFDATA2MSB);
  Bytes[llvm::ELF::EI_VERSION] = static_cast<uint8_t>(llvm::ELF::EV_CURRENT);
  Bytes[llvm::ELF::EI_OSABI] = static_cast<uint8_t>(llvm::ELF::ELFOSABI_NONE);
  putFileHeader<Ehdr>(Bytes, Context);
  for (size_t I = 0; I < Context.Segments.size(); ++I)
    putProgramHeader<Phdr>(
        Bytes, Context.Class.HeaderSize + I * Context.Class.ProgramHeaderSize,
        Endian, Context.Segments[I]);
  for (size_t I = 1; I < Context.Sections.size(); ++I) {
    const auto &Section = Context.Sections[I];
    if (Section.Type != llvm::ELF::SHT_NOBITS && !Section.Content.empty() &&
        !copyAt(Bytes, Section.Offset, Section.Content))
      return diagnosticError("ELF section content is outside output");
    putSectionHeader<Shdr>(Bytes,
                           Context.SectionHeaderOffset +
                               I * Context.Class.SectionHeaderSize,
                           Endian, Section, Context.SectionNameOffsets[I]);
  }
  return {};
}

LinkExpect<void> emitELF(const ELFEmissionContext &Context, Writer &Output) {
  std::vector<Byte> Bytes(static_cast<size_t>(Context.FileSize));
  if (Context.Class.Wide) {
    EXPECTED_TRY((fillImage<llvm::ELF::Elf64_Ehdr, llvm::ELF::Elf64_Phdr,
                            llvm::ELF::Elf64_Shdr>(Bytes, Context)));
  } else {
    EXPECTED_TRY((fillImage<llvm::ELF::Elf32_Ehdr, llvm::ELF::Elf32_Phdr,
                            llvm::ELF::Elf32_Shdr>(Bytes, Context)));
  }
  return writeImage(Output, Bytes, "ELF");
}

LinkExpect<std::vector<Byte>> buildEHFrameHeader(const LinkGraph &Graph,
                                                 uint64_t HeaderAddress) {
  const auto Endian = Graph.endianness();
  std::vector<FDEInfo> FDEs;
  uint64_t EHAddress = UINT64_MAX;
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    if (Graph.sections()[I].Purpose != SectionPurpose::EHFrame)
      continue;
    EXPECTED_TRY(parseEHFrame(Graph, I, FDEs));
    EHAddress = std::min(EHAddress, Graph.sections()[I].Address);
  }
  if (FDEs.empty())
    return diagnosticError("ELF EH frame has no FDEs");
  std::sort(FDEs.begin(), FDEs.end(), [](const auto &Left, const auto &Right) {
    return std::tie(Left.Function, Left.Address) <
           std::tie(Right.Function, Right.Address);
  });
  uint64_t HeaderSize = 0;
  uint64_t TableSize = 0;
  if (!checkedMul(FDEs.size(), EHFrameHeaderEntrySize, TableSize) ||
      !addUnsigned(EHFrameHeaderPrefixSize, TableSize, HeaderSize) ||
      FDEs.size() > UINT32_MAX)
    return diagnosticError("ELF EH frame header size overflows");
  std::vector<Byte> Content(static_cast<size_t>(HeaderSize));
  Content[0] = EHFrameHeaderVersion;
  Content[1] = llvm::dwarf::DW_EH_PE_pcrel | llvm::dwarf::DW_EH_PE_sdata4;
  Content[2] = llvm::dwarf::DW_EH_PE_udata4;
  Content[3] = llvm::dwarf::DW_EH_PE_datarel | llvm::dwarf::DW_EH_PE_sdata4;
  int32_t Delta = 0;
  if (!signedDelta32(EHAddress, HeaderAddress + 4, Delta))
    return diagnosticError("ELF EH frame pointer is out of range");
  putInteger(Content, 4, static_cast<uint32_t>(Delta), 4, Endian);
  putInteger(Content, 8, FDEs.size(), 4, Endian);
  for (size_t I = 0; I < FDEs.size(); ++I) {
    const uint64_t Entry = EHFrameHeaderPrefixSize + I * EHFrameHeaderEntrySize;
    if (!signedDelta32(FDEs[I].Function, HeaderAddress, Delta))
      return diagnosticError("ELF EH frame function is out of range");
    putInteger(Content, Entry, static_cast<uint32_t>(Delta), 4, Endian);
    if (!signedDelta32(FDEs[I].Address, HeaderAddress, Delta))
      return diagnosticError("ELF EH frame FDE is out of range");
    putInteger(Content, Entry + 4, static_cast<uint32_t>(Delta), 4, Endian);
  }
  return Content;
}

struct StringTable {
  std::vector<Byte> Bytes;
  std::vector<uint32_t> Offsets;
};

bool appendString(StringTable &Table, std::string_view Value) {
  uint64_t NewSize = 0;
  if (!addUnsigned(Table.Bytes.size(), Value.size(), NewSize) ||
      !addUnsigned(NewSize, 1, NewSize) || NewSize > UINT32_MAX ||
      !fitsSizeT(NewSize))
    return false;
  Table.Offsets.push_back(static_cast<uint32_t>(Table.Bytes.size()));
  Table.Bytes.insert(Table.Bytes.end(), Value.begin(), Value.end());
  Table.Bytes.push_back(0);
  return true;
}

LinkExpect<StringTable> buildDynamicStrings(Span<const Symbol *const> Exports) {
  StringTable Result{std::vector<Byte>(1), {}};
  for (const auto *SymbolValue : Exports)
    if (!appendString(Result, SymbolValue->exportedName()))
      return diagnosticError("ELF dynamic string table overflows");
  return Result;
}

template <typename Sym>
void putSymbol(Span<Byte> Bytes, uint64_t Offset, Endianness Endian,
               uint32_t NameOffset, uint8_t Info, uint16_t SectionIndex,
               uint64_t Address, uint64_t Size) {
  const FieldWriter Put(Bytes, Offset, Endian);
  Put(offsetof(Sym, st_name), sizeof(Sym::st_name), NameOffset);
  Put(offsetof(Sym, st_info), sizeof(Sym::st_info), Info);
  Put(offsetof(Sym, st_other), sizeof(Sym::st_other), llvm::ELF::STV_DEFAULT);
  Put(offsetof(Sym, st_shndx), sizeof(Sym::st_shndx), SectionIndex);
  Put(offsetof(Sym, st_value), sizeof(Sym::st_value), Address);
  Put(offsetof(Sym, st_size), sizeof(Sym::st_size), Size);
}

LinkExpect<std::vector<Byte>>
buildDynamicSymbols(const LinkGraph &Graph, const ELFClass &Class,
                    Span<const Symbol *const> Exports,
                    Span<const uint32_t> NameOffsets,
                    Span<const uint32_t> GraphSectionIndices) {
  uint64_t SymbolCount = 0;
  uint64_t Size = 0;
  if (!addUnsigned(Exports.size(), 1, SymbolCount) ||
      SymbolCount > UINT32_MAX ||
      !checkedMul(SymbolCount, Class.SymbolSize, Size) || !fitsSizeT(Size))
    return diagnosticError("ELF dynamic symbol table overflows");
  std::vector<Byte> Result(static_cast<size_t>(Size));
  for (size_t I = 0; I < Exports.size(); ++I) {
    const auto &SymbolValue = *Exports[I];
    const auto &Owner = Graph.sections()[SymbolValue.Section];
    const uint64_t Address = (Owner.Address + SymbolValue.Offset) |
                             static_cast<uint64_t>(SymbolValue.Thumb);
    const uint8_t Type = static_cast<uint8_t>(Owner.Kind == SectionKind::Text
                                                  ? llvm::ELF::STT_FUNC
                                                  : llvm::ELF::STT_OBJECT);
    const uint8_t Info = llvm::ELF::STB_GLOBAL << 4 | Type;
    const uint64_t Offset = (I + 1) * Class.SymbolSize;
    const auto SectionIndex =
        static_cast<uint16_t>(GraphSectionIndices[SymbolValue.Section]);
    if (Class.Wide)
      putSymbol<llvm::ELF::Elf64_Sym>(Result, Offset, Graph.endianness(),
                                      NameOffsets[I], Info, SectionIndex,
                                      Address, SymbolValue.Size);
    else
      putSymbol<llvm::ELF::Elf32_Sym>(Result, Offset, Graph.endianness(),
                                      NameOffsets[I], Info, SectionIndex,
                                      Address, SymbolValue.Size);
  }
  return Result;
}

LinkExpect<std::vector<Byte>> buildHash(Span<const Symbol *const> Exports,
                                        Endianness Endian) {
  const uint64_t SymbolCount = Exports.size() + 1;
  const uint32_t BucketCount =
      std::max<uint32_t>(1, static_cast<uint32_t>(Exports.size()));
  uint64_t Words = 0;
  uint64_t Size = 0;
  if (!addUnsigned(HashHeaderWords, BucketCount, Words) ||
      !addUnsigned(Words, SymbolCount, Words) ||
      !checkedMul(Words, HashWordSize, Size) || !fitsSizeT(Size))
    return diagnosticError("ELF hash table overflows");
  std::vector<Byte> Result(static_cast<size_t>(Size));
  putInteger(Result, 0, BucketCount, HashWordSize, Endian);
  putInteger(Result, HashWordSize, SymbolCount, HashWordSize, Endian);
  for (size_t I = 0; I < Exports.size(); ++I) {
    const uint32_t Index = static_cast<uint32_t>(I + 1);
    const uint32_t Bucket = hash(Exports[I]->exportedName()) % BucketCount;
    const uint64_t BucketOffset = (HashHeaderWords + Bucket) * HashWordSize;
    const auto Previous = static_cast<uint32_t>(
        getInteger(Result, BucketOffset, HashWordSize, Endian));
    putInteger(Result, BucketOffset, Index, HashWordSize, Endian);
    if (Previous != 0)
      putInteger(Result, (HashHeaderWords + BucketCount + Index) * HashWordSize,
                 Previous, HashWordSize, Endian);
  }
  return Result;
}

LinkExpect<std::vector<Byte>> buildRelocations(const LinkGraph &Graph,
                                               const ELFClass &Class) {
  const auto Endian = Graph.endianness();
  uint64_t Size = 0;
  if (!checkedMul(Graph.rebases().size(), Class.RelocationSize, Size) ||
      !fitsSizeT(Size))
    return diagnosticError("ELF dynamic relocation table overflows");
  std::vector<Byte> Result(static_cast<size_t>(Size));
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
      return diagnosticError(std::move(Diag));
    }
    const uint64_t Address = TargetSection.Address + RebaseValue.Offset;
    const FieldWriter Put(Result, I * Class.RelocationSize, Endian);
    if (Class.Wide) {
      using Rela = llvm::ELF::Elf64_Rela;
      Put(offsetof(Rela, r_offset), sizeof(Rela::r_offset), Address);
      Put(offsetof(Rela, r_info), sizeof(Rela::r_info),
          relativeType(Graph.target()));
      Put(offsetof(Rela, r_addend), sizeof(Rela::r_addend),
          getInteger(TargetSection.Content, RebaseValue.Offset,
                     Class.AddressSize, Endian));
    } else {
      using Rel = llvm::ELF::Elf32_Rel;
      Put(offsetof(Rel, r_offset), sizeof(Rel::r_offset), Address);
      Put(offsetof(Rel, r_info), sizeof(Rel::r_info),
          relativeType(Graph.target()));
    }
  }
  return Result;
}

struct DynamicTargets {
  const OutputSection &Hash;
  const OutputSection &Strings;
  const OutputSection &Symbols;
  const OutputSection &Relocations;
};

LinkExpect<std::vector<Byte>> buildDynamic(const ELFClass &Class,
                                           Endianness Endian,
                                           const DynamicTargets &Targets) {
  uint64_t Size = 0;
  if (!checkedMul(DynamicTagCount, Class.DynamicSize, Size) || !fitsSizeT(Size))
    return diagnosticError("ELF dynamic section overflows");
  std::vector<Byte> Result(static_cast<size_t>(Size));
  size_t Index = 0;
  auto Add = [&](int64_t Tag, uint64_t Value) {
    const uint64_t Offset = Index++ * Class.DynamicSize;
    putInteger(Result, Offset, static_cast<uint64_t>(Tag), Class.AddressSize,
               Endian);
    putInteger(Result, Offset + Class.AddressSize, Value, Class.AddressSize,
               Endian);
  };
  Add(llvm::ELF::DT_HASH, Targets.Hash.Address);
  Add(llvm::ELF::DT_STRTAB, Targets.Strings.Address);
  Add(llvm::ELF::DT_STRSZ, Targets.Strings.Size);
  Add(llvm::ELF::DT_SYMTAB, Targets.Symbols.Address);
  Add(llvm::ELF::DT_SYMENT, Class.SymbolSize);
  Add(Class.Wide ? llvm::ELF::DT_RELA : llvm::ELF::DT_REL,
      Targets.Relocations.Address);
  Add(Class.Wide ? llvm::ELF::DT_RELASZ : llvm::ELF::DT_RELSZ,
      Targets.Relocations.Size);
  Add(Class.Wide ? llvm::ELF::DT_RELAENT : llvm::ELF::DT_RELENT,
      Class.RelocationSize);
  Add(llvm::ELF::DT_NULL, 0);
  return Result;
}

LinkExpect<StringTable>
buildSectionNames(const std::vector<OutputSection> &Sections,
                  std::string_view OwnName) {
  StringTable Result{std::vector<Byte>(1), std::vector<uint32_t>(1)};
  for (size_t I = 1; I < Sections.size(); ++I)
    if (!appendString(Result, Sections[I].Name))
      return diagnosticError("ELF section name table overflows");
  if (!appendString(Result, OwnName))
    return diagnosticError("ELF section name table overflows");
  return Result;
}

uint32_t segmentFlags(const OutputSection &Value) noexcept {
  return llvm::ELF::PF_R |
         ((Value.Flags & llvm::ELF::SHF_EXECINSTR) != 0 ? llvm::ELF::PF_X
                                                        : uint32_t{0}) |
         ((Value.Flags & llvm::ELF::SHF_WRITE) != 0 ? llvm::ELF::PF_W
                                                    : uint32_t{0});
}

LinkExpect<void> appendLoadSegments(std::vector<OutputSegment> &Segments,
                                    const std::vector<OutputSection> &Sections,
                                    uint64_t PageSize) {
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
  uint64_t PreviousRoundedEnd = PageSize;
  for (size_t Begin = 0; Begin < Allocated.size();) {
    const uint32_t Flags = segmentFlags(*Allocated[Begin]);
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
          (segmentFlags(*SectionValue) != Flags ||
           CandidateAddress < PreviousRoundedEnd ||
           (HasNOBITS && SectionValue->Type != llvm::ELF::SHT_NOBITS)))
        break;
      if (SectionValue->Type != llvm::ELF::SHT_NOBITS) {
        if (SectionValue->Address < SectionValue->Offset)
          return diagnosticError("ELF section address precedes its offset");
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
      return diagnosticError("ELF load segment starts before the file");
    const uint64_t Offset = First->Offset - Prefix;
    uint64_t FileEnd = Offset;
    uint64_t MemoryEnd = Address;
    for (size_t I = Begin; I < End; ++I) {
      const auto *SectionValue = Allocated[I];
      MemoryEnd =
          std::max(MemoryEnd, SectionValue->Address + SectionValue->Size);
      if (SectionValue->Type != llvm::ELF::SHT_NOBITS)
        FileEnd = std::max(FileEnd, SectionValue->Offset + SectionValue->Size);
    }
    if (Offset % Alignment != Address % Alignment || FileEnd < Offset ||
        MemoryEnd < Address)
      return diagnosticError("ELF load segment is misaligned");
    Segments.push_back({llvm::ELF::PT_LOAD, Flags, Offset, Address,
                        FileEnd - Offset, MemoryEnd - Address, Alignment});
    if (!checkedAlign(MemoryEnd, PageSize, PreviousRoundedEnd))
      return diagnosticError("ELF load segment end overflows");
    Begin = End;
  }
  return {};
}

LinkExpect<void> appendExidxSegment(std::vector<OutputSegment> &Segments,
                                    const std::vector<OutputSection> &Sections,
                                    uint64_t Maximum) {
  std::vector<const OutputSection *> ExidxSections;
  for (const auto &SectionValue : Sections)
    if (SectionValue.Type == llvm::ELF::SHT_ARM_EXIDX && SectionValue.Size != 0)
      ExidxSections.push_back(&SectionValue);
  if (ExidxSections.empty())
    return {};
  std::sort(ExidxSections.begin(), ExidxSections.end(),
            [](const auto *Left, const auto *Right) {
              return std::tie(Left->Address, Left->Offset) <
                     std::tie(Right->Address, Right->Offset);
            });
  const auto &First = *ExidxSections.front();
  uint64_t AddressEnd = 0;
  uint64_t FileEnd = 0;
  uint64_t Alignment = 1;
  for (size_t I = 0; I < ExidxSections.size(); ++I) {
    const auto &SectionValue = *ExidxSections[I];
    uint64_t NextAddressEnd = 0;
    uint64_t NextFileEnd = 0;
    if (!boundedAdd(SectionValue.Address, SectionValue.Size, Maximum,
                    NextAddressEnd) ||
        !boundedAdd(SectionValue.Offset, SectionValue.Size, Maximum,
                    NextFileEnd))
      return diagnosticError("ELF ARM exception index range overflows");
    if (I != 0 &&
        ((First.Address >= First.Offset) !=
             (SectionValue.Address >= SectionValue.Offset) ||
         (First.Address >= First.Offset
              ? First.Address - First.Offset !=
                    SectionValue.Address - SectionValue.Offset
              : First.Offset - First.Address !=
                    SectionValue.Offset - SectionValue.Address) ||
         SectionValue.Address != AddressEnd || SectionValue.Offset != FileEnd))
      return diagnosticError("ELF ARM exception index sections are not "
                             "contiguous");
    AddressEnd = NextAddressEnd;
    FileEnd = NextFileEnd;
    Alignment = std::max(Alignment, SectionValue.Alignment);
  }
  if (First.Offset % Alignment != First.Address % Alignment)
    return diagnosticError("ELF ARM exception index segment is misaligned");
  Segments.push_back({llvm::ELF::PT_ARM_EXIDX, llvm::ELF::PF_R, First.Offset,
                      First.Address, FileEnd - First.Offset,
                      AddressEnd - First.Address, Alignment});
  return {};
}

struct SegmentInputs {
  uint64_t ReservedHeaders;
  uint32_t DynamicIndex;
  uint32_t EHHeaderIndex;
};

LinkExpect<std::vector<OutputSegment>>
buildSegments(const LinkGraph &Graph, const ELFClass &Class,
              const std::vector<OutputSection> &Sections,
              const SegmentInputs &Inputs) {
  const uint64_t PageSize = maximumPageSize(Graph.target());
  const uint64_t HeadersSize =
      Class.HeaderSize + Class.ProgramHeaderSize * Inputs.ReservedHeaders;
  std::vector<OutputSegment> Segments;
  Segments.push_back({llvm::ELF::PT_LOAD, llvm::ELF::PF_R, 0, 0, HeadersSize,
                      HeadersSize, PageSize});
  EXPECTED_TRY(appendLoadSegments(Segments, Sections, PageSize));
  const auto &Dynamic = Sections[Inputs.DynamicIndex];
  Segments.push_back({llvm::ELF::PT_DYNAMIC, llvm::ELF::PF_R | llvm::ELF::PF_W,
                      Dynamic.Offset, Dynamic.Address, Dynamic.Size,
                      Dynamic.Size, Class.AddressSize});
  if (Inputs.EHHeaderIndex != 0) {
    const auto &Header = Sections[Inputs.EHHeaderIndex];
    Segments.push_back({llvm::ELF::PT_GNU_EH_FRAME, llvm::ELF::PF_R,
                        Header.Offset, Header.Address, Header.Size, Header.Size,
                        EHFrameHeaderAlignment});
  }
  EXPECTED_TRY(appendExidxSegment(Segments, Sections, Class.Maximum));
  Segments.push_back({llvm::ELF::PT_GNU_STACK,
                      llvm::ELF::PF_R | llvm::ELF::PF_W, 0, 0, 0, 0,
                      Class.AddressSize});
  if (Segments.size() > Inputs.ReservedHeaders || Segments.size() > UINT16_MAX)
    return diagnosticError("ELF program headers exceed the reservation");
  return Segments;
}

LinkExpect<void> validateWriteInput(const LinkGraph &Graph) {
  if (!Graph.relocationsApplied() || Graph.format() != ObjectFormat::ELF)
    return diagnosticError("ELF writer requires a relocated ELF link graph");
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    const auto &SectionValue = Graph.sections()[I];
    if (SectionValue.Purpose == SectionPurpose::ARMExidx &&
        (SectionValue.Content.size() != SectionValue.VirtualSize ||
         (SectionValue.VirtualSize != 0 && SectionValue.VirtualSize % 8 != 0)))
      return sectionError("invalid ARM exception index section", SectionValue,
                          I);
  }
  EXPECTED_TRY(Graph.validate());
  if (!validELFFlags(Graph))
    return diagnosticError("unsupported ELF flags");
  for (const auto &RebaseValue : Graph.rebases())
    if (!validRebase(Graph, RebaseValue))
      return diagnosticError("unsupported ELF rebase");
  const auto Class = elfClass(Graph.target());
  uint64_t MaximumSectionCount = 0;
  if (!addUnsigned(Graph.sections().size(), GeneratedSectionCount,
                   MaximumSectionCount) ||
      MaximumSectionCount > UINT16_MAX || Graph.symbols().size() > UINT32_MAX ||
      Graph.rebases().size() > UINT32_MAX)
    return diagnosticError("ELF link graph has too many entries");
  if (Class.Wide)
    return {};
  for (const auto &SectionValue : Graph.sections()) {
    uint64_t End = 0;
    if (SectionValue.Address > UINT32_MAX ||
        SectionValue.FileOffset > UINT32_MAX ||
        SectionValue.VirtualSize > UINT32_MAX ||
        SectionValue.Content.size() > UINT32_MAX ||
        !addUnsigned(SectionValue.Address, SectionValue.VirtualSize, End) ||
        End > UINT32_MAX)
      return diagnosticError("ELF32 section exceeds the address space");
  }
  for (const auto &SymbolValue : Graph.symbols()) {
    uint64_t Address = 0;
    if (SymbolValue.Size > UINT32_MAX ||
        !addUnsigned(Graph.sections()[SymbolValue.Section].Address,
                     SymbolValue.Offset, Address) ||
        Address > UINT32_MAX)
      return diagnosticError("ELF32 symbol exceeds the address space");
  }
  return {};
}

} // namespace

Expect<void> ELFWriter::layout(LinkGraph &Graph) noexcept {
  auto Fail = []() { return Unexpect(ErrCode::Value::IllegalPath); };
  try {
    if (Graph.format() != ObjectFormat::ELF || Graph.relocationsApplied() ||
        !Graph.validate() || machine(Graph.target()) == llvm::ELF::EM_NONE ||
        (Graph.target() == Target::S390X
             ? Graph.endianness() != Endianness::Big
             : Graph.endianness() != Endianness::Little))
      return Fail();
    const auto Class = elfClass(Graph.target());
    const uint64_t PageSize = maximumPageSize(Graph.target());
    const auto MaximumProgramHeaders = reservedProgramHeaders(Graph);
    if (!MaximumProgramHeaders)
      return Fail();
    uint64_t AddressCursor = 0;
    uint64_t ProgramHeaderBytes = 0;
    if (!checkedMul(Class.ProgramHeaderSize, *MaximumProgramHeaders,
                    ProgramHeaderBytes) ||
        !addUnsigned(Class.HeaderSize, ProgramHeaderBytes, AddressCursor) ||
        !checkedAlign(AddressCursor, PageSize, AddressCursor))
      return Fail();
    uint64_t FileCursor = AddressCursor;
    constexpr std::array<SectionKind, 5> Kinds{
        SectionKind::Text, SectionKind::ReadOnly, SectionKind::Unwind,
        SectionKind::Data, SectionKind::BSS};
    std::vector<Placement> Placements(Graph.sections().size());
    for (const auto Kind : Kinds) {
      if (Kind != SectionKind::Text &&
          (!checkedAlign(AddressCursor, PageSize, AddressCursor) ||
           (Kind != SectionKind::BSS &&
            !checkedAlign(FileCursor, PageSize, FileCursor))))
        return Fail();
      for (const SectionId Id : order(Graph, Kind, Placements)) {
        const auto &SectionValue = Graph.sections()[Id];
        if (!checkedAlign(AddressCursor, SectionValue.Alignment,
                          AddressCursor) ||
            !checkedAlign(FileCursor, SectionValue.Alignment, FileCursor))
          return Fail();
        const uint64_t Address = AddressCursor;
        const uint64_t FileOffset = FileCursor;
        if (!addUnsigned(AddressCursor, SectionValue.VirtualSize,
                         AddressCursor) ||
            (Kind != SectionKind::BSS &&
             !addUnsigned(FileCursor, SectionValue.VirtualSize, FileCursor)) ||
            (!Class.Wide &&
             (Address > UINT32_MAX || FileOffset > UINT32_MAX ||
              SectionValue.VirtualSize > UINT32_MAX ||
              AddressCursor > UINT32_MAX || FileCursor > UINT32_MAX)))
          return Fail();
        Placements[Id] = {Address, FileOffset};
      }
    }
    if (!Internal::applyPlacements(Graph, Placements))
      return Fail();
    return {};
  } catch (...) {
    return Fail();
  }
}

LinkExpect<void> ELFWriter::write(const LinkGraph &Graph,
                                  Writer &Output) noexcept {
  try {
    EXPECTED_TRY(validateWriteInput(Graph));
    const auto Class = elfClass(Graph.target());
    const uint64_t PageSize = maximumPageSize(Graph.target());
    const auto Endian = Graph.endianness();

    std::vector<OutputSection> Sections(1);
    std::vector<uint32_t> GraphSectionIndices(Graph.sections().size());
    for (SectionId I = 0; I < Graph.sections().size(); ++I) {
      const auto &Input = Graph.sections()[I];
      GraphSectionIndices[I] = static_cast<uint32_t>(Sections.size());
      const bool ARMExidx = Input.Purpose == SectionPurpose::ARMExidx;
      OutputSection Converted;
      Converted.Name = Input.Name;
      Converted.Type = ARMExidx ? llvm::ELF::SHT_ARM_EXIDX
                       : Input.Kind == SectionKind::BSS
                           ? llvm::ELF::SHT_NOBITS
                           : llvm::ELF::SHT_PROGBITS;
      Converted.Flags = ARMExidx
                            ? llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_LINK_ORDER
                            : sectionFlags(Input.Kind);
      Converted.Address = Input.Address;
      Converted.Offset = Input.FileOffset;
      Converted.Size = Input.VirtualSize;
      Converted.Alignment = Input.Alignment;
      Converted.Content = Input.Content;
      Sections.push_back(std::move(Converted));
    }
    for (SectionId I = 0; I < Graph.sections().size(); ++I) {
      const auto &Input = Graph.sections()[I];
      if (Input.Purpose != SectionPurpose::ARMExidx)
        continue;
      if (!Input.LinkedSection)
        return sectionError("ARM exception index section has no linked "
                            "section",
                            Input, I);
      Sections[GraphSectionIndices[I]].Link =
          GraphSectionIndices[*Input.LinkedSection];
    }

    Cursor Position{0, 0, Class.Maximum};
    for (const auto &SectionValue : Graph.sections()) {
      if (!addUnsigned(std::max(Position.Address, SectionValue.Address),
                       SectionValue.VirtualSize, Position.Address) ||
          (SectionValue.Kind != SectionKind::BSS &&
           !addUnsigned(std::max(Position.File, SectionValue.FileOffset),
                        SectionValue.VirtualSize, Position.File)))
        return diagnosticError("ELF input section end overflows");
    }
    if (!Position.align(PageSize))
      return diagnosticError("ELF generated sections exceed the ELF class");

    uint32_t EHHeaderIndex = 0;
    if (std::any_of(Graph.sections().begin(), Graph.sections().end(),
                    [](const Section &Value) {
                      return Value.Purpose == SectionPurpose::EHFrame;
                    })) {
      EXPECTED_TRY(auto Header, buildEHFrameHeader(Graph, Position.Address));
      EXPECTED_TRY(EHHeaderIndex,
                   appendSection(Sections, Position,
                                 generatedSection(".eh_frame_hdr",
                                                  llvm::ELF::SHT_PROGBITS,
                                                  llvm::ELF::SHF_ALLOC,
                                                  EHFrameHeaderAlignment, 0,
                                                  std::move(Header))));
    }

    EXPECTED_TRY(auto Exports, sortedUniqueExports(Graph));
    EXPECTED_TRY(auto DynStr, buildDynamicStrings(Exports));
    if (!Position.align(Class.AddressSize))
      return diagnosticError("ELF .dynstr section exceeds the ELF class");
    EXPECTED_TRY(
        const uint32_t DynStrIndex,
        appendSection(Sections, Position,
                      generatedSection(".dynstr", llvm::ELF::SHT_STRTAB,
                                       llvm::ELF::SHF_ALLOC, 1, 0,
                                       std::move(DynStr.Bytes))));

    if (!Position.align(Class.AddressSize))
      return diagnosticError("ELF .dynsym section exceeds the ELF class");
    EXPECTED_TRY(auto DynSym,
                 buildDynamicSymbols(Graph, Class, Exports, DynStr.Offsets,
                                     GraphSectionIndices));
    auto DynSymSection = generatedSection(
        ".dynsym", llvm::ELF::SHT_DYNSYM, llvm::ELF::SHF_ALLOC,
        Class.AddressSize, Class.SymbolSize, std::move(DynSym));
    DynSymSection.Link = DynStrIndex;
    DynSymSection.Info = 1;
    EXPECTED_TRY(const uint32_t DynSymIndex,
                 appendSection(Sections, Position, std::move(DynSymSection)));

    if (!Position.align(HashWordSize))
      return diagnosticError("ELF .hash section exceeds the ELF class");
    EXPECTED_TRY(auto Hash, buildHash(Exports, Endian));
    auto HashSection =
        generatedSection(".hash", llvm::ELF::SHT_HASH, llvm::ELF::SHF_ALLOC,
                         HashWordSize, HashWordSize, std::move(Hash));
    HashSection.Link = DynSymIndex;
    EXPECTED_TRY(const uint32_t HashIndex,
                 appendSection(Sections, Position, std::move(HashSection)));

    if (!Position.align(Class.AddressSize))
      return diagnosticError("ELF relocation section exceeds the ELF class");
    EXPECTED_TRY(auto Relocations, buildRelocations(Graph, Class));
    auto RelocationSection =
        generatedSection(Class.Wide ? ".rela.dyn" : ".rel.dyn",
                         Class.Wide ? llvm::ELF::SHT_RELA : llvm::ELF::SHT_REL,
                         llvm::ELF::SHF_ALLOC, Class.AddressSize,
                         Class.RelocationSize, std::move(Relocations));
    RelocationSection.Link = DynSymIndex;
    EXPECTED_TRY(
        const uint32_t RelocationIndex,
        appendSection(Sections, Position, std::move(RelocationSection)));

    if (!Position.align(PageSize))
      return diagnosticError("ELF .dynamic section exceeds the ELF class");
    const DynamicTargets Targets{Sections[HashIndex], Sections[DynStrIndex],
                                 Sections[DynSymIndex],
                                 Sections[RelocationIndex]};
    EXPECTED_TRY(auto Dynamic, buildDynamic(Class, Endian, Targets));
    auto DynamicSection = generatedSection(
        ".dynamic", llvm::ELF::SHT_DYNAMIC,
        llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_WRITE, Class.AddressSize,
        Class.DynamicSize, std::move(Dynamic));
    DynamicSection.Link = DynStrIndex;
    EXPECTED_TRY(const uint32_t DynamicIndex,
                 appendSection(Sections, Position, std::move(DynamicSection)));

    constexpr std::string_view SectionNamesName = ".shstrtab";
    EXPECTED_TRY(auto SectionNames,
                 buildSectionNames(Sections, SectionNamesName));
    const auto SectionNameIndex = static_cast<uint32_t>(Sections.size());
    if (!boundedAlign(Position.File, 1, Class.Maximum, Position.File))
      return diagnosticError("ELF .shstrtab section exceeds the ELF class");
    auto NamesSection =
        generatedSection(std::string(SectionNamesName), llvm::ELF::SHT_STRTAB,
                         0, 1, 0, std::move(SectionNames.Bytes));
    NamesSection.Offset = Position.File;
    Sections.push_back(std::move(NamesSection));
    uint64_t SectionHeaderOffset = 0;
    uint64_t SectionHeadersSize = 0;
    uint64_t FileSize = 0;
    if (!boundedAdd(Position.File, Sections.back().Size, Class.Maximum,
                    Position.File) ||
        !boundedAlign(Position.File, Class.AddressSize, Class.Maximum,
                      SectionHeaderOffset) ||
        !checkedMul(Sections.size(), Class.SectionHeaderSize,
                    SectionHeadersSize) ||
        !boundedAdd(SectionHeaderOffset, SectionHeadersSize, Class.Maximum,
                    FileSize) ||
        !fitsSizeT(FileSize))
      return diagnosticError("ELF section headers exceed the ELF class");

    const auto ReservedHeaders = reservedProgramHeaders(Graph);
    if (!ReservedHeaders)
      return diagnosticError("ELF program header count overflows");
    const SegmentInputs Inputs{*ReservedHeaders, DynamicIndex, EHHeaderIndex};
    EXPECTED_TRY(auto Segments, buildSegments(Graph, Class, Sections, Inputs));

    return emitELF(ELFEmissionContext{Graph, Class, Sections,
                                      SectionNames.Offsets, Segments, FileSize,
                                      SectionHeaderOffset, SectionNameIndex},
                   Output);
  } catch (...) {
    return diagnosticError("ELF writer failed");
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
