// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include <llvm/Support/JSON.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "linker/object_reader.h"

#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/relocation.h"

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/BinaryFormat/Magic.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/SymbolSize.h>
#include <llvm/Support/ARMAttributeParser.h>
#include <llvm/Support/ARMBuildAttributes.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBufferRef.h>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

// Object decoding follows the ELF gABI plus target psABIs, the Mach-O ABI,
// and the PE/COFF specification; LLVM's object readers provide the structures.
constexpr uint8_t BytePatch = 1;
constexpr uint64_t CompactUnwindRecordSize = 32;
constexpr uint64_t CompactUnwindFunctionOffset = 0;
constexpr uint64_t CompactUnwindLengthOffset = 8;
constexpr uint64_t CompactUnwindEncodingOffset = 12;
constexpr uint64_t CompactUnwindPersonalityOffset = 16;
constexpr uint64_t CompactUnwindLSDAOffset = 24;
constexpr uint32_t CompactUnwindModeMask = 0x0F000000;
constexpr uint32_t CompactUnwindAArch64Dwarf = 0x03000000;
constexpr uint32_t CompactUnwindX8664Dwarf = 0x04000000;
constexpr uint32_t CompactUnwindDwarfOffsetMask = 0x00FFFFFF;
constexpr uint32_t MachOCPUTypeArm64 = 0x0100000C;
constexpr uint32_t MachOCPUSubtypeMask = 0xFF000000;
constexpr uint32_t MachOCPUSubtypeArm64All = 0;

enum class ELFIdentification : uint64_t {
  Size = llvm::ELF::EI_NIDENT,
  Magic0 = llvm::ELF::EI_MAG0,
  Magic1 = llvm::ELF::EI_MAG1,
  Magic2 = llvm::ELF::EI_MAG2,
  Magic3 = llvm::ELF::EI_MAG3,
  Class = llvm::ELF::EI_CLASS,
  Data = llvm::ELF::EI_DATA,
};

enum class ELF32Offset : uint64_t {
  SectionTable = 32,
  SectionEntrySize = 46,
  SectionCount = 48,
  SectionType = 4,
  SectionOffset = 16,
  SectionSize = 20,
  SectionLink = 24,
  SectionEntry = 36,
  RelocationInfo = 4,
};

enum class ELF64Offset : uint64_t {
  SectionTable = 40,
  SectionEntrySize = 58,
  SectionCount = 60,
  SectionType = 4,
  SectionOffset = 24,
  SectionSize = 32,
  SectionLink = 40,
  SectionEntry = 56,
  RelocationInfo = 8,
};

constexpr uint64_t value(ELFIdentification Value) noexcept {
  return static_cast<uint64_t>(Value);
}

constexpr uint64_t value(ELF32Offset Value) noexcept {
  return static_cast<uint64_t>(Value);
}

constexpr uint64_t value(ELF64Offset Value) noexcept {
  return static_cast<uint64_t>(Value);
}

static_assert(value(ELF32Offset::SectionTable) == 32);
static_assert(value(ELF32Offset::SectionEntrySize) == 46);
static_assert(value(ELF64Offset::SectionTable) == 40);
static_assert(value(ELF64Offset::SectionEntrySize) == 58);

template <typename T>
LinkExpect<T> fail(std::string_view Message,
                   DiagnosticKind Kind = DiagnosticKind::Malformed) {
  Diagnostic Diag{std::string(Message)};
  Diag.Kind = Kind;
  return Unexpected<Diagnostic>(std::move(Diag));
}

Unexpected<Diagnostic> addContext(Diagnostic Value, std::string_view Context) {
  Value.Message = std::string(Context) + ": " + Value.Message;
  return Unexpected<Diagnostic>(std::move(Value));
}

template <typename T>
bool take(llvm::Expected<T> Value, T &Output, std::string_view Context,
          Diagnostic *Error = nullptr,
          DiagnosticKind Kind = DiagnosticKind::Malformed) {
  if (!Value) {
    if (Error) {
      Error->Message =
          std::string(Context) + ": " + llvm::toString(Value.takeError());
      Error->Kind = Kind;
    } else
      llvm::consumeError(Value.takeError());
    return false;
  }
  Output = std::move(*Value);
  return true;
}

std::optional<Target> normalizeTarget(llvm::Triple::ArchType Arch) noexcept {
  switch (Arch) {
  case llvm::Triple::x86_64:
    return Target::X86_64;
  case llvm::Triple::arm:
  case llvm::Triple::armeb:
  case llvm::Triple::thumb:
  case llvm::Triple::thumbeb:
    return Target::ARM;
  case llvm::Triple::aarch64:
  case llvm::Triple::aarch64_be:
    return Target::AArch64;
  case llvm::Triple::riscv64:
    return Target::RISCV64;
  case llvm::Triple::systemz:
    return Target::S390X;
  default:
    return std::nullopt;
  }
}

bool isSupportedFormat(const llvm::object::ObjectFile &Object) noexcept {
  return Object.isELF() || Object.isCOFF() || Object.isMachO();
}

bool readARMAttributeULEB(llvm::ArrayRef<uint8_t> Bytes, size_t &Offset,
                          uint64_t &Value) {
  Value = 0;
  for (unsigned Shift = 0; Shift < 64; Shift += 7) {
    if (Offset >= Bytes.size())
      return false;
    const uint8_t Byte = Bytes[Offset++];
    if (Shift == 63 && (Byte & UINT8_C(0xFE)) != 0)
      return false;
    Value |= static_cast<uint64_t>(Byte & UINT8_C(0x7F)) << Shift;
    if ((Byte & UINT8_C(0x80)) == 0)
      return true;
  }
  return false;
}

bool readARMAttributeWord(llvm::ArrayRef<uint8_t> Bytes, size_t &Offset,
                          bool LittleEndian, uint32_t &Value) {
  if (Offset > Bytes.size() || Bytes.size() - Offset < 4)
    return false;
  Value = 0;
  for (unsigned I = 0; I < 4; ++I) {
    const unsigned Shift = LittleEndian ? I * 8 : (3 - I) * 8;
    Value |= static_cast<uint32_t>(Bytes[Offset + I]) << Shift;
  }
  Offset += 4;
  return true;
}

bool skipARMAttributeString(llvm::ArrayRef<uint8_t> Bytes, size_t &Offset) {
  while (Offset < Bytes.size()) {
    if (Bytes[Offset++] == 0)
      return true;
  }
  return false;
}

LinkExpect<std::vector<uint64_t>> armVFPArgs(llvm::ArrayRef<uint8_t> Bytes,
                                             bool LittleEndian) {
  std::vector<uint64_t> Result;
  if (Bytes.empty() || Bytes[0] != 'A')
    return fail<std::vector<uint64_t>>("malformed ARM attributes");
  size_t Offset = 1;
  while (Offset < Bytes.size()) {
    const size_t VendorStart = Offset;
    uint32_t VendorLength = 0;
    if (!readARMAttributeWord(Bytes, Offset, LittleEndian, VendorLength) ||
        VendorLength < 4 || VendorStart > Bytes.size() ||
        VendorLength > Bytes.size() - VendorStart)
      return fail<std::vector<uint64_t>>("malformed ARM attributes");
    const size_t VendorEnd = VendorStart + VendorLength;
    const size_t VendorName = Offset;
    if (!skipARMAttributeString(Bytes.slice(0, VendorEnd), Offset))
      return fail<std::vector<uint64_t>>("malformed ARM attributes");
    const bool AEABI = Offset - VendorName == 6 &&
                       Bytes.slice(VendorName, 5) ==
                           llvm::ArrayRef<uint8_t>{'a', 'e', 'a', 'b', 'i'};
    if (!AEABI) {
      Offset = VendorEnd;
      continue;
    }
    while (Offset < VendorEnd) {
      const size_t SubsectionStart = Offset;
      uint64_t SubsectionTag = 0;
      uint32_t SubsectionLength = 0;
      if (!readARMAttributeULEB(Bytes.slice(0, VendorEnd), Offset,
                                SubsectionTag) ||
          !readARMAttributeWord(Bytes.slice(0, VendorEnd), Offset, LittleEndian,
                                SubsectionLength) ||
          Offset - SubsectionStart != 5 || SubsectionLength < 5 ||
          SubsectionLength > VendorEnd - SubsectionStart)
        return fail<std::vector<uint64_t>>("malformed ARM attributes");
      const size_t SubsectionEnd = SubsectionStart + SubsectionLength;
      if (SubsectionTag == llvm::ARMBuildAttrs::Section ||
          SubsectionTag == llvm::ARMBuildAttrs::Symbol) {
        uint64_t Index = 0;
        do {
          if (!readARMAttributeULEB(Bytes.slice(0, SubsectionEnd), Offset,
                                    Index))
            return fail<std::vector<uint64_t>>("malformed ARM attributes");
        } while (Index != 0);
      } else if (SubsectionTag != llvm::ARMBuildAttrs::File) {
        return fail<std::vector<uint64_t>>("malformed ARM attributes");
      }
      while (Offset < SubsectionEnd) {
        uint64_t Tag = 0;
        uint64_t Value = 0;
        if (!readARMAttributeULEB(Bytes.slice(0, SubsectionEnd), Offset, Tag))
          return fail<std::vector<uint64_t>>("malformed ARM attributes");
        const bool StringValue =
            Tag == llvm::ARMBuildAttrs::CPU_raw_name ||
            Tag == llvm::ARMBuildAttrs::CPU_name ||
            Tag == llvm::ARMBuildAttrs::also_compatible_with ||
            Tag == llvm::ARMBuildAttrs::conformance ||
            (Tag >= 32 && (Tag & 1) != 0);
        if (StringValue) {
          if (!skipARMAttributeString(Bytes.slice(0, SubsectionEnd), Offset))
            return fail<std::vector<uint64_t>>("malformed ARM attributes");
        } else {
          if (!readARMAttributeULEB(Bytes.slice(0, SubsectionEnd), Offset,
                                    Value))
            return fail<std::vector<uint64_t>>("malformed ARM attributes");
          if (Tag == llvm::ARMBuildAttrs::ABI_VFP_args)
            Result.push_back(Value);
          if (Tag == llvm::ARMBuildAttrs::compatibility &&
              !skipARMAttributeString(Bytes.slice(0, SubsectionEnd), Offset))
            return fail<std::vector<uint64_t>>("malformed ARM attributes");
        }
      }
    }
  }
  return Result;
}

LinkExpect<std::vector<uint8_t>>
armAttributesForLLVM(llvm::ArrayRef<uint8_t> Bytes, bool LittleEndian) {
  if (Bytes.empty() || Bytes[0] != 'A')
    return fail<std::vector<uint8_t>>("malformed ARM attributes");
  std::vector<uint8_t> Result;
  Result.reserve(Bytes.size());
  Result.push_back(Bytes[0]);
  size_t Offset = 1;
  while (Offset < Bytes.size()) {
    const size_t VendorStart = Offset;
    uint32_t VendorLength = 0;
    if (!readARMAttributeWord(Bytes, Offset, LittleEndian, VendorLength) ||
        VendorLength < 4 || VendorLength > Bytes.size() - VendorStart)
      return fail<std::vector<uint8_t>>("malformed ARM attributes");
    const size_t VendorEnd = VendorStart + VendorLength;
    const size_t VendorName = Offset;
    if (!skipARMAttributeString(Bytes.slice(0, VendorEnd), Offset))
      return fail<std::vector<uint8_t>>("malformed ARM attributes");
    const bool AEABI = Offset - VendorName == 6 &&
                       Bytes.slice(VendorName, 5) ==
                           llvm::ArrayRef<uint8_t>{'a', 'e', 'a', 'b', 'i'};
    if (!AEABI) {
      Offset = VendorEnd;
      continue;
    }
    const size_t OutputVendorStart = Result.size();
    Result.insert(Result.end(), Bytes.begin() + VendorStart,
                  Bytes.begin() + VendorEnd);
    while (Offset < VendorEnd) {
      const size_t SubsectionStart = Offset;
      uint64_t SubsectionTag = 0;
      uint32_t SubsectionLength = 0;
      if (!readARMAttributeULEB(Bytes.slice(0, VendorEnd), Offset,
                                SubsectionTag) ||
          !readARMAttributeWord(Bytes.slice(0, VendorEnd), Offset, LittleEndian,
                                SubsectionLength) ||
          Offset - SubsectionStart != 5 || SubsectionLength < 5 ||
          SubsectionLength > VendorEnd - SubsectionStart)
        return fail<std::vector<uint8_t>>("malformed ARM attributes");
      const size_t SubsectionEnd = SubsectionStart + SubsectionLength;
      if (SubsectionTag == llvm::ARMBuildAttrs::Section ||
          SubsectionTag == llvm::ARMBuildAttrs::Symbol) {
        const size_t IndexStart = Offset;
        uint64_t Index = 0;
        do {
          if (!readARMAttributeULEB(Bytes.slice(0, SubsectionEnd), Offset,
                                    Index))
            return fail<std::vector<uint8_t>>("malformed ARM attributes");
        } while (Index != 0);
        const uint32_t ParserLength =
            SubsectionLength - static_cast<uint32_t>(Offset - IndexStart);
        const size_t LengthOffset =
            OutputVendorStart + (SubsectionStart - VendorStart) + 1;
        for (unsigned I = 0; I < 4; ++I) {
          const unsigned Shift = LittleEndian ? I * 8 : (3 - I) * 8;
          Result[LengthOffset + I] =
              static_cast<uint8_t>(ParserLength >> Shift);
        }
      }
      Offset = SubsectionEnd;
    }
  }
  return Result;
}

LinkExpect<uint32_t> armELFFlags(const llvm::object::ObjectFile &Object,
                                 uint32_t InputFlags) {
  constexpr uint32_t Supported = llvm::ELF::EF_ARM_EABIMASK |
                                 llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                 llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  if ((InputFlags & ~Supported) != 0)
    return fail<uint32_t>("unsupported ARM ELF header flags",
                          DiagnosticKind::Unsupported);
  enum class FloatABI { Unspecified, Soft, Hard, ToolChain };
  FloatABI ABI = FloatABI::Unspecified;
  bool HasConstraint = false;
  for (const auto &Section : Object.sections()) {
    const llvm::object::ELFSectionRef ELFSection(Section);
    if (ELFSection.getType() != llvm::ELF::SHT_ARM_ATTRIBUTES)
      continue;
    llvm::StringRef Contents;
    Diagnostic Error{""};
    if (!take(Section.getContents(), Contents, "cannot read ARM attributes",
              &Error))
      return Unexpected<Diagnostic>(std::move(Error));
    const auto Bytes = llvm::ArrayRef<uint8_t>(
        reinterpret_cast<const uint8_t *>(Contents.data()), Contents.size());
    auto ParserBytes = armAttributesForLLVM(Bytes, Object.isLittleEndian());
    if (!ParserBytes)
      return Unexpected<Diagnostic>(std::move(ParserBytes.error()));
    llvm::ARMAttributeParser Parser;
#if LLVM_VERSION_MAJOR >= 18
    const auto Endian = Object.isLittleEndian() ? llvm::endianness::little
                                                : llvm::endianness::big;
#else
    const auto Endian =
        Object.isLittleEndian() ? llvm::support::little : llvm::support::big;
#endif
    if (auto Parsed = Parser.parse(*ParserBytes, Endian))
      return fail<uint32_t>("malformed ARM attributes: " +
                            llvm::toString(std::move(Parsed)));
    auto Values = armVFPArgs(Bytes, Object.isLittleEndian());
    if (!Values)
      return Unexpected<Diagnostic>(std::move(Values.error()));
    for (const uint64_t Value : *Values) {
      FloatABI SectionABI = FloatABI::Unspecified;
      switch (Value) {
      case llvm::ARMBuildAttrs::BaseAAPCS:
        SectionABI = FloatABI::Soft;
        break;
      case llvm::ARMBuildAttrs::HardFPAAPCS:
        SectionABI = FloatABI::Hard;
        break;
      case llvm::ARMBuildAttrs::ToolChainFPPCS:
        SectionABI = FloatABI::ToolChain;
        break;
      case llvm::ARMBuildAttrs::CompatibleFPAAPCS:
        continue;
      default:
        return fail<uint32_t>("unknown ARM Tag_ABI_VFP_args value");
      }
      if (HasConstraint && ABI != SectionABI)
        return fail<uint32_t>("conflicting ARM Tag_ABI_VFP_args values");
      ABI = SectionABI;
      HasConstraint = true;
    }
  }
  uint32_t Flags = llvm::ELF::EF_ARM_EABI_VER5;
  if (!HasConstraint || ABI == FloatABI::Soft)
    Flags |= llvm::ELF::EF_ARM_ABI_FLOAT_SOFT;
  else if (ABI == FloatABI::Hard)
    Flags |= llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  return Flags;
}

uint64_t sectionAlignment(const llvm::object::SectionRef &Section) noexcept {
#if LLVM_VERSION_MAJOR >= 16
  return Internal::normalizeSectionAlignment(Section.getAlignment().value());
#else
  return Internal::normalizeSectionAlignment(Section.getAlignment());
#endif
}

bool symbolFlags(const llvm::object::SymbolRef &Symbol, uint32_t &Flags,
                 Diagnostic *Error = nullptr) {
#if LLVM_VERSION_MAJOR >= 11
  return take(Symbol.getFlags(), Flags, "cannot read symbol flags", Error);
#else
  Flags = Symbol.getFlags();
  return true;
#endif
}

template <typename T>
bool readELFInteger(Span<const Byte> Buffer, uint64_t Offset, bool LittleEndian,
                    T &Value) noexcept {
  constexpr uint8_t BitsPerByte = 8;
  if (Offset > Buffer.size() || sizeof(T) > Buffer.size() - Offset) {
    return false;
  }
  Value = 0;
  for (size_t I = 0; I < sizeof(T); ++I) {
    const size_t Shift =
        LittleEndian ? I * BitsPerByte : (sizeof(T) - I - 1) * BitsPerByte;
    Value |= static_cast<T>(Buffer[Offset + I]) << Shift;
  }
  return true;
}

bool hasTrailingELFObject(Span<const Byte> Buffer,
                          const llvm::object::ObjectFile &Object) noexcept {
  if (!Object.isELF() || Buffer.size() < value(ELFIdentification::Size)) {
    return false;
  }
  const bool Is64 =
      Buffer[value(ELFIdentification::Class)] == llvm::ELF::ELFCLASS64;
  const bool LittleEndian =
      Buffer[value(ELFIdentification::Data)] == llvm::ELF::ELFDATA2LSB;
  uint64_t SectionTable = 0;
  uint16_t SectionEntrySize = 0;
  uint16_t SectionCount = 0;
  if (Is64) {
    if (!readELFInteger(Buffer, value(ELF64Offset::SectionTable), LittleEndian,
                        SectionTable)) {
      return false;
    }
  } else {
    uint32_t SectionTable32 = 0;
    if (!readELFInteger(Buffer, value(ELF32Offset::SectionTable), LittleEndian,
                        SectionTable32)) {
      return false;
    }
    SectionTable = SectionTable32;
  }
  if (!readELFInteger(Buffer,
                      Is64 ? value(ELF64Offset::SectionEntrySize)
                           : value(ELF32Offset::SectionEntrySize),
                      LittleEndian, SectionEntrySize) ||
      !readELFInteger(Buffer,
                      Is64 ? value(ELF64Offset::SectionCount)
                           : value(ELF32Offset::SectionCount),
                      LittleEndian, SectionCount) ||
      (SectionEntrySize != 0 &&
       SectionCount > (UINT64_MAX - SectionTable) / SectionEntrySize)) {
    return false;
  }
  uint64_t End = SectionTable + static_cast<uint64_t>(SectionEntrySize) *
                                    static_cast<uint64_t>(SectionCount);
  for (const auto &Section : Object.sections()) {
    const llvm::object::ELFSectionRef ELFSection(Section);
    if (ELFSection.getType() != llvm::ELF::SHT_NOBITS) {
      End = std::max(End, ELFSection.getOffset() + ELFSection.getSize());
    }
  }
  return End <= Buffer.size() - std::min<size_t>(Buffer.size(), 4) &&
         Buffer[End] == llvm::ELF::ElfMagic[0] &&
         Buffer[End + 1] == llvm::ELF::ElfMagic[1] &&
         Buffer[End + 2] == llvm::ELF::ElfMagic[2] &&
         Buffer[End + 3] == llvm::ELF::ElfMagic[3];
}

enum class COFFTrailingObject { None, Found, Malformed };

bool isCOFFObjectCandidate(Span<const Byte> Buffer, uint64_t Offset) noexcept {
  const Byte First = Buffer[Offset];
  const Byte Second = Buffer[Offset + 1];
  if (First == 0 && Second == 0)
    return true;
  if (Second == 0x01)
    return First == 0xF0 || First == 0x83 || First == 0x84 || First == 0x66 ||
           First == 0x50 || First == 0x4C || First == 0xC4;
  if (Second == 0x02)
    return First == 0x90 || First == 0x68;
  if (First == 0x64)
    return Second == 0x86 || Second == 0xAA;
  return (First == 0x41 || First == 0x4E) && Second == 0xA6;
}

COFFTrailingObject
findTrailingCOFFObject(Span<const Byte> Buffer,
                       const llvm::object::ObjectFile &Object) noexcept {
  constexpr uint64_t COFFHeaderSize = 20;
  constexpr uint64_t BigObjHeaderSize = 56;
  constexpr uint64_t SectionSize = 40;
  constexpr uint64_t RelocationSize = 10;
  constexpr uint64_t LineNumberSize = 6;
  constexpr uint64_t SymbolSize = 18;
  constexpr uint64_t BigObjSymbolSize = 20;
  constexpr uint64_t BigObjMagicOffset = 12;
  constexpr uint64_t SectionRawSizeOffset = 16;
  constexpr uint64_t SectionRawDataOffset = 20;
  constexpr uint64_t SectionRelocationOffset = 24;
  constexpr uint64_t SectionLineNumberOffset = 28;
  constexpr uint64_t SectionRelocationCountOffset = 32;
  constexpr uint64_t SectionLineNumberCountOffset = 34;
  constexpr uint64_t SectionCharacteristicsOffset = 36;
  constexpr uint64_t RelocationCountOffset = 0;
  constexpr uint16_t BigObjSignature = UINT16_MAX;
  constexpr uint16_t MinimumBigObjVersion = 2;
  constexpr uint16_t OverflowRelocationCount = UINT16_MAX;

  if (!Object.isCOFF()) {
    return COFFTrailingObject::None;
  }
  if (Buffer.size() < COFFHeaderSize) {
    return COFFTrailingObject::Malformed;
  }
  auto Read16 = [&](uint64_t Offset, uint16_t &Value) noexcept {
    return readELFInteger(Buffer, Offset, true, Value);
  };
  auto Read32 = [&](uint64_t Offset, uint32_t &Value) noexcept {
    return readELFInteger(Buffer, Offset, true, Value);
  };
  auto Extend = [&](uint64_t Offset, uint64_t Count, uint64_t Width,
                    uint64_t &End) noexcept {
    if (Count == 0) {
      return true;
    }
    if (Offset > Buffer.size() || Count > (Buffer.size() - Offset) / Width) {
      return false;
    }
    End = std::max(End, Offset + Count * Width);
    return true;
  };

  uint16_t Signature1 = 0;
  uint16_t Signature2 = 0;
  uint16_t BigObjVersion = 0;
  if (!Read16(0, Signature1) || !Read16(2, Signature2) ||
      !Read16(4, BigObjVersion)) {
    return COFFTrailingObject::Malformed;
  }
  bool IsBigObj = Signature1 == llvm::COFF::IMAGE_FILE_MACHINE_UNKNOWN &&
                  Signature2 == BigObjSignature &&
                  BigObjVersion >= MinimumBigObjVersion &&
                  Buffer.size() >= BigObjHeaderSize;
  if (IsBigObj) {
    for (size_t I = 0; I < sizeof(llvm::COFF::BigObjMagic); ++I) {
      if (Buffer[BigObjMagicOffset + I] !=
          static_cast<Byte>(llvm::COFF::BigObjMagic[I])) {
        IsBigObj = false;
        break;
      }
    }
  }

  uint64_t HeaderSize = COFFHeaderSize;
  uint64_t SectionCount = 0;
  uint64_t SymbolTable = 0;
  uint64_t SymbolCount = 0;
  uint64_t InputSymbolSize = SymbolSize;
  if (IsBigObj) {
    uint32_t Value = 0;
    if (!Read32(44, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SectionCount = Value;
    if (!Read32(48, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SymbolTable = Value;
    if (!Read32(52, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SymbolCount = Value;
    HeaderSize = BigObjHeaderSize;
    InputSymbolSize = BigObjSymbolSize;
  } else {
    uint16_t Sections = 0;
    uint16_t OptionalHeaderSize = 0;
    uint32_t Value = 0;
    if (!Read16(2, Sections) || !Read32(8, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SectionCount = Sections;
    SymbolTable = Value;
    if (!Read32(12, Value) || !Read16(16, OptionalHeaderSize)) {
      return COFFTrailingObject::Malformed;
    }
    SymbolCount = Value;
    HeaderSize += OptionalHeaderSize;
  }

  uint64_t End = HeaderSize;
  if (!Extend(HeaderSize, SectionCount, SectionSize, End)) {
    return COFFTrailingObject::Malformed;
  }
  for (uint64_t I = 0; I < SectionCount; ++I) {
    const uint64_t Section = HeaderSize + I * SectionSize;
    uint32_t RawSize = 0;
    uint32_t RawData = 0;
    uint32_t Relocations = 0;
    uint32_t LineNumbers = 0;
    uint32_t Characteristics = 0;
    uint16_t RelocationCount = 0;
    uint16_t LineNumberCount = 0;
    if (!Read32(Section + SectionRawSizeOffset, RawSize) ||
        !Read32(Section + SectionRawDataOffset, RawData) ||
        !Read32(Section + SectionRelocationOffset, Relocations) ||
        !Read32(Section + SectionLineNumberOffset, LineNumbers) ||
        !Read16(Section + SectionRelocationCountOffset, RelocationCount) ||
        !Read16(Section + SectionLineNumberCountOffset, LineNumberCount) ||
        !Read32(Section + SectionCharacteristicsOffset, Characteristics) ||
        (RawData != 0 && !Extend(RawData, RawSize, 1, End))) {
      return COFFTrailingObject::Malformed;
    }
    uint64_t ActualRelocationCount = RelocationCount;
    const bool HasOverflow =
        (Characteristics & llvm::COFF::IMAGE_SCN_LNK_NRELOC_OVFL) != 0;
    if (HasOverflow && RelocationCount != OverflowRelocationCount) {
      return COFFTrailingObject::Malformed;
    }
    if (HasOverflow) {
      uint32_t ExtendedCount = 0;
      if (!Read32(Relocations + RelocationCountOffset, ExtendedCount) ||
          ExtendedCount < UINT32_C(0x10000)) {
        return COFFTrailingObject::Malformed;
      }
      ActualRelocationCount = ExtendedCount;
    }
    if (!Extend(Relocations, ActualRelocationCount, RelocationSize, End) ||
        !Extend(LineNumbers, LineNumberCount, LineNumberSize, End)) {
      return COFFTrailingObject::Malformed;
    }
  }
  if (SymbolCount != 0) {
    if (SymbolTable > Buffer.size() ||
        SymbolCount > (Buffer.size() - SymbolTable) / InputSymbolSize) {
      return COFFTrailingObject::Malformed;
    }
    const uint64_t SymbolEnd = SymbolTable + SymbolCount * InputSymbolSize;
    End = std::max(End, SymbolEnd);
    uint32_t StringTableSize = 0;
    if (!Read32(SymbolEnd, StringTableSize) ||
        !Extend(SymbolEnd,
                std::max<uint32_t>(StringTableSize, sizeof(uint32_t)), 1,
                End)) {
      return COFFTrailingObject::Malformed;
    }
  } else if (SymbolTable != 0) {
    uint32_t StringTableSize = 0;
    uint64_t StringTableEnd = End;
    if (Read32(SymbolTable, StringTableSize) &&
        Extend(SymbolTable,
               std::max<uint32_t>(StringTableSize, sizeof(uint32_t)), 1,
               StringTableEnd)) {
      End = StringTableEnd;
    }
  }
  if (End >= Buffer.size()) {
    return COFFTrailingObject::None;
  }
  for (uint64_t Offset = End; Offset <= Buffer.size() - COFFHeaderSize;
       ++Offset) {
    if (!isCOFFObjectCandidate(Buffer, Offset))
      continue;
    const auto TrailingData =
        llvm::StringRef(reinterpret_cast<const char *>(Buffer.data() + Offset),
                        Buffer.size() - Offset);
    if (llvm::identify_magic(TrailingData) != llvm::file_magic::coff_object) {
      continue;
    }
    auto TrailingObject = llvm::object::ObjectFile::createObjectFile(
        llvm::MemoryBufferRef(TrailingData, "trailing object"));
    if (!TrailingObject) {
      llvm::consumeError(TrailingObject.takeError());
      continue;
    }
    if ((*TrailingObject)->isCOFF()) {
      return COFFTrailingObject::Found;
    }
  }
  return COFFTrailingObject::None;
}

bool validELFRelocations(Span<const Byte> Buffer) noexcept {
  constexpr uint64_t MinimumCrelEntrySize = 1;
  constexpr uint64_t ELF32SectionEntrySize = 40;
  constexpr uint64_t ELF64SectionEntrySize = 64;
  constexpr uint64_t ELF32RelEntrySize = 8;
  constexpr uint64_t ELF32RelaEntrySize = 12;
  constexpr uint64_t ELF64RelEntrySize = 16;
  constexpr uint64_t ELF64RelaEntrySize = 24;
  constexpr uint64_t ELF32SymbolEntrySize = 16;
  constexpr uint64_t ELF64SymbolEntrySize = 24;
  constexpr unsigned ELF32SymbolIndexShift = 8;
  constexpr unsigned ELF64SymbolIndexShift = 32;
  if (Buffer.size() < value(ELFIdentification::Size) ||
      Buffer[value(ELFIdentification::Magic0)] != llvm::ELF::ElfMagic[0] ||
      Buffer[value(ELFIdentification::Magic1)] != llvm::ELF::ElfMagic[1] ||
      Buffer[value(ELFIdentification::Magic2)] != llvm::ELF::ElfMagic[2] ||
      Buffer[value(ELFIdentification::Magic3)] != llvm::ELF::ElfMagic[3]) {
    return true;
  }
  const bool Is64 =
      Buffer[value(ELFIdentification::Class)] == llvm::ELF::ELFCLASS64;
  const bool Is32 =
      Buffer[value(ELFIdentification::Class)] == llvm::ELF::ELFCLASS32;
  const bool LittleEndian =
      Buffer[value(ELFIdentification::Data)] == llvm::ELF::ELFDATA2LSB;
  if ((!Is32 && !Is64) ||
      (!LittleEndian &&
       Buffer[value(ELFIdentification::Data)] != llvm::ELF::ELFDATA2MSB)) {
    return false;
  }
  uint64_t SectionOffset = 0;
  uint16_t SectionEntrySize = 0;
  uint16_t SectionCount16 = 0;
  if (Is64) {
    if (!readELFInteger(Buffer, value(ELF64Offset::SectionTable), LittleEndian,
                        SectionOffset) ||
        !readELFInteger(Buffer, value(ELF64Offset::SectionEntrySize),
                        LittleEndian, SectionEntrySize) ||
        !readELFInteger(Buffer, value(ELF64Offset::SectionCount), LittleEndian,
                        SectionCount16)) {
      return false;
    }
  } else {
    uint32_t Offset32 = 0;
    if (!readELFInteger(Buffer, value(ELF32Offset::SectionTable), LittleEndian,
                        Offset32) ||
        !readELFInteger(Buffer, value(ELF32Offset::SectionEntrySize),
                        LittleEndian, SectionEntrySize) ||
        !readELFInteger(Buffer, value(ELF32Offset::SectionCount), LittleEndian,
                        SectionCount16)) {
      return false;
    }
    SectionOffset = Offset32;
  }
  uint64_t SectionCount = SectionCount16;
  const uint64_t RequiredSectionSize =
      Is64 ? ELF64SectionEntrySize : ELF32SectionEntrySize;
  if (SectionEntrySize < RequiredSectionSize || SectionOffset > Buffer.size() ||
      SectionEntrySize > Buffer.size() - SectionOffset) {
    return false;
  }
  if (SectionCount == 0) {
    if (Is64) {
      if (!readELFInteger(Buffer,
                          SectionOffset + value(ELF64Offset::SectionSize),
                          LittleEndian, SectionCount)) {
        return false;
      }
    } else {
      uint32_t ExtendedCount = 0;
      if (!readELFInteger(Buffer,
                          SectionOffset + value(ELF32Offset::SectionSize),
                          LittleEndian, ExtendedCount)) {
        return false;
      }
      SectionCount = ExtendedCount;
    }
  }
  if (SectionCount == 0 ||
      static_cast<uint64_t>(SectionCount) >
          (Buffer.size() - SectionOffset) / SectionEntrySize) {
    return false;
  }
  auto ReadSection = [&](uint64_t Index, uint32_t &Type, uint64_t &Offset,
                         uint64_t &Size, uint32_t &Link,
                         uint64_t &EntrySize) noexcept {
    if (Index >= SectionCount) {
      return false;
    }
    const uint64_t Base = SectionOffset + Index * SectionEntrySize;
    if (!readELFInteger(Buffer,
                        Base + (Is64 ? value(ELF64Offset::SectionType)
                                     : value(ELF32Offset::SectionType)),
                        LittleEndian, Type) ||
        !readELFInteger(Buffer,
                        Base + (Is64 ? value(ELF64Offset::SectionLink)
                                     : value(ELF32Offset::SectionLink)),
                        LittleEndian, Link)) {
      return false;
    }
    if (Is64) {
      return readELFInteger(Buffer, Base + value(ELF64Offset::SectionOffset),
                            LittleEndian, Offset) &&
             readELFInteger(Buffer, Base + value(ELF64Offset::SectionSize),
                            LittleEndian, Size) &&
             readELFInteger(Buffer, Base + value(ELF64Offset::SectionEntry),
                            LittleEndian, EntrySize);
    }
    uint32_t Offset32 = 0;
    uint32_t Size32 = 0;
    uint32_t EntrySize32 = 0;
    if (!readELFInteger(Buffer, Base + value(ELF32Offset::SectionOffset),
                        LittleEndian, Offset32) ||
        !readELFInteger(Buffer, Base + value(ELF32Offset::SectionSize),
                        LittleEndian, Size32) ||
        !readELFInteger(Buffer, Base + value(ELF32Offset::SectionEntry),
                        LittleEndian, EntrySize32)) {
      return false;
    }
    Offset = Offset32;
    Size = Size32;
    EntrySize = EntrySize32;
    return true;
  };
  for (uint64_t I = 0; I < SectionCount; ++I) {
    uint32_t Type = 0;
    uint32_t Link = 0;
    uint64_t Offset = 0;
    uint64_t Size = 0;
    uint64_t EntrySize = 0;
    if (!ReadSection(I, Type, Offset, Size, Link, EntrySize)) {
      return false;
    }
    const bool IsCrel =
#if LLVM_VERSION_MAJOR >= 19
        Type == llvm::ELF::SHT_CREL;
#else
        false;
#endif
    if (Type != llvm::ELF::SHT_REL && Type != llvm::ELF::SHT_RELA && !IsCrel) {
      continue;
    }
    const uint64_t ExpectedEntrySize =
        IsCrel ? MinimumCrelEntrySize
        : Is64 ? (Type == llvm::ELF::SHT_RELA ? ELF64RelaEntrySize
                                              : ELF64RelEntrySize)
               : (Type == llvm::ELF::SHT_RELA ? ELF32RelaEntrySize
                                              : ELF32RelEntrySize);
    uint32_t SymbolType = 0;
    uint32_t SymbolLink = 0;
    uint64_t SymbolOffset = 0;
    uint64_t SymbolSize = 0;
    uint64_t SymbolEntrySize = 0;
    if (EntrySize != ExpectedEntrySize || Size % EntrySize != 0 ||
        Offset > Buffer.size() || Size > Buffer.size() - Offset ||
        !ReadSection(Link, SymbolType, SymbolOffset, SymbolSize, SymbolLink,
                     SymbolEntrySize) ||
        (SymbolType != llvm::ELF::SHT_SYMTAB &&
         SymbolType != llvm::ELF::SHT_DYNSYM) ||
        SymbolEntrySize !=
            (Is64 ? ELF64SymbolEntrySize : ELF32SymbolEntrySize) ||
        SymbolSize % SymbolEntrySize != 0 || SymbolOffset > Buffer.size() ||
        SymbolSize > Buffer.size() - SymbolOffset) {
      return false;
    }
    const uint64_t SymbolCount = SymbolSize / SymbolEntrySize;
    if (IsCrel) {
#if LLVM_VERSION_MAJOR >= 19
      uint64_t DeclaredCount = 0;
      uint64_t DecodedCount = 0;
      bool ValidSymbols = true;
      const auto Content =
          llvm::ArrayRef<uint8_t>(Buffer.data() + Offset, Size);
      auto Error =
          Is64 ? llvm::object::decodeCrel<true>(
                     Content,
                     [&](uint64_t Count, bool) { DeclaredCount = Count; },
                     [&](const auto &Relocation) {
                       ++DecodedCount;
                       ValidSymbols &= Relocation.r_symidx < SymbolCount;
                     })
               : llvm::object::decodeCrel<false>(
                     Content,
                     [&](uint64_t Count, bool) { DeclaredCount = Count; },
                     [&](const auto &Relocation) {
                       ++DecodedCount;
                       ValidSymbols &= Relocation.r_symidx < SymbolCount;
                     });
      if (Error) {
        llvm::consumeError(std::move(Error));
        return false;
      }
      if (!ValidSymbols || DecodedCount != DeclaredCount) {
        return false;
      }
#endif
      continue;
    }
    for (uint64_t J = 0; J < Size / EntrySize; ++J) {
      const uint64_t InfoOffset = Offset + J * EntrySize +
                                  (Is64 ? value(ELF64Offset::RelocationInfo)
                                        : value(ELF32Offset::RelocationInfo));
      uint64_t Info = 0;
      if (Is64) {
        if (!readELFInteger(Buffer, InfoOffset, LittleEndian, Info) ||
            (Info >> ELF64SymbolIndexShift) >= SymbolCount) {
          return false;
        }
      } else {
        uint32_t Info32 = 0;
        if (!readELFInteger(Buffer, InfoOffset, LittleEndian, Info32) ||
            (Info32 >> ELF32SymbolIndexShift) >= SymbolCount) {
          return false;
        }
      }
    }
  }
  return true;
}

bool isAllocatable(const llvm::object::ObjectFile &Object,
                   const llvm::object::SectionRef &Section) noexcept {
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELFObjectFileBase>(&Object)) {
    (void)ELF;
    return (llvm::object::ELFSectionRef(Section).getFlags() &
            llvm::ELF::SHF_ALLOC) != 0;
  }
  if (const auto *COFF =
          llvm::dyn_cast<llvm::object::COFFObjectFile>(&Object)) {
    const auto Flags = COFF->getCOFFSection(Section)->Characteristics;
    return (Flags & (llvm::COFF::IMAGE_SCN_MEM_EXECUTE |
                     llvm::COFF::IMAGE_SCN_MEM_READ |
                     llvm::COFF::IMAGE_SCN_MEM_WRITE)) != 0 &&
           (Flags & llvm::COFF::IMAGE_SCN_MEM_DISCARDABLE) == 0;
  }
  return Section.isBerkeleyText() || Section.isBerkeleyData() ||
         Section.isBSS();
}

SectionPurpose sectionPurpose(const llvm::object::ObjectFile &Object,
                              llvm::StringRef Name) noexcept {
#if LLVM_VERSION_MAJOR >= 16
  if (Name == ".ARM.exidx" || Name.starts_with(".ARM.exidx."))
#else
  if (Name == ".ARM.exidx" || Name.startswith(".ARM.exidx."))
#endif
    return SectionPurpose::ARMExidx;
  if (Name.contains("eh_frame"))
    return SectionPurpose::EHFrame;
#if LLVM_VERSION_MAJOR >= 16
  if (Object.isCOFF() && Name.starts_with(".pdata"))
    return SectionPurpose::PData;
  if (Object.isCOFF() && Name.starts_with(".xdata"))
    return SectionPurpose::XData;
#else
  if (Object.isCOFF() && Name.startswith(".pdata"))
    return SectionPurpose::PData;
  if (Object.isCOFF() && Name.startswith(".xdata"))
    return SectionPurpose::XData;
#endif
  if (Object.isMachO() && Name.contains("compact_unwind"))
    return SectionPurpose::CompactUnwind;
  return SectionPurpose::Default;
}

SectionKind sectionKind(const llvm::object::SectionRef &Section,
                        SectionPurpose Purpose) noexcept {
  if (Section.isText()) {
    return SectionKind::Text;
  }
  if (Section.isBSS() || Section.isVirtual()) {
    return SectionKind::BSS;
  }
  if (Purpose == SectionPurpose::EHFrame ||
      Purpose == SectionPurpose::ARMExidx || Purpose == SectionPurpose::PData) {
    return SectionKind::Unwind;
  }
  if (Purpose == SectionPurpose::XData ||
      Purpose == SectionPurpose::CompactUnwind)
    return SectionKind::ReadOnly;
  if (Section.isBerkeleyText()) {
    return SectionKind::ReadOnly;
  }
  if (Section.isData() || Section.isBerkeleyData()) {
    return SectionKind::Data;
  }
  return SectionKind::ReadOnly;
}

std::pair<int64_t, bool>
relocationAddend(const llvm::object::ObjectFile &Object,
                 const llvm::object::RelocationRef &Relocation) noexcept {
  if (!llvm::isa<llvm::object::ELFObjectFileBase>(Object)) {
    return {0, true};
  }
  auto Addend = llvm::object::ELFRelocationRef(Relocation).getAddend();
  if (!Addend) {
    llvm::consumeError(Addend.takeError());
    return {0, true};
  }
  return {*Addend, false};
}

ObjectFormat objectFormat(const llvm::object::ObjectFile &Object) noexcept {
  if (Object.isMachO()) {
    return ObjectFormat::MachO;
  }
  if (Object.isCOFF()) {
    return ObjectFormat::COFF;
  }
  return ObjectFormat::ELF;
}

std::optional<uint32_t>
elfLinkedSection(const llvm::object::ObjectFile &Object,
                 const llvm::object::SectionRef &Section) noexcept {
  const auto Reference = Section.getRawDataRefImpl();
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF32LEObjectFile>(&Object))
    return ELF->getSection(Reference)->sh_link;
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF32BEObjectFile>(&Object))
    return ELF->getSection(Reference)->sh_link;
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(&Object))
    return ELF->getSection(Reference)->sh_link;
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF64BEObjectFile>(&Object))
    return ELF->getSection(Reference)->sh_link;
  return std::nullopt;
}

struct RelocationMetadata {
  uint8_t PatchSize = BytePatch;
  bool PCRelative = false;
  bool External = false;
  bool Scattered = false;
};

} // namespace

bool Internal::supportsMachORelocationMetadata(Target TargetValue,
                                               bool Scattered) noexcept {
  return !Scattered ||
         (TargetValue != Target::X86_64 && TargetValue != Target::AArch64);
}

namespace {

LinkExpect<void> associateRawX86_64MachOFDEs(LinkGraph &Graph) {
  if (Graph.format() != ObjectFormat::MachO || Graph.target() != Target::X86_64)
    return {};
  if (std::none_of(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   }))
    return {};

  std::set<std::pair<SectionId, uint64_t>> RelocatedFields;
  for (const auto &Relocation : Graph.relocations())
    RelocatedFields.emplace(Relocation.Section, Relocation.Offset);

  struct AddressSymbols {
    std::optional<SymbolId> Text;
    bool HasText = false;
    bool HasNonText = false;
  };
  std::map<uint64_t, AddressSymbols> SymbolsByAddress;
  for (SymbolId Symbol = 0; Symbol < Graph.symbols().size(); ++Symbol) {
    const auto &Value = Graph.symbols()[Symbol];
    if (Value.Section >= Graph.sections().size())
      continue;
    const auto &TargetSection = Graph.sections()[Value.Section];
    if (Value.Offset > UINT64_MAX - TargetSection.InputAddress)
      return fail<void>("x86_64 Mach-O FDE symbol address overflows");
    auto &Address = SymbolsByAddress[TargetSection.InputAddress + Value.Offset];
    if (TargetSection.Kind != SectionKind::Text) {
      Address.HasNonText = true;
      continue;
    }
    Address.HasText = true;
    if (!Address.Text && Value.Name.rfind("$section", 0) != 0)
      Address.Text = Symbol;
  }

  for (SectionId Section = 0; Section < Graph.sections().size(); ++Section) {
    if (Graph.sections()[Section].Purpose != SectionPurpose::EHFrame)
      continue;
    auto Fields =
        machOEHFrameFields(Graph.sections()[Section].Content, Target::X86_64);
    if (!Fields)
      return fail<void>("malformed x86_64 Mach-O EH frame");
    for (const auto Field : *Fields) {
      if (RelocatedFields.count({Section, Field}) != 0)
        continue;
      auto Delta = Internal::readSigned(Graph.sections()[Section].Content,
                                        Field, 8, Endianness::Little);
      if (!Delta)
        return fail<void>("cannot read x86_64 Mach-O FDE initial location");
      auto Address = Internal::resolveMachOFDEAddress(
          0, Graph.sections()[Section].InputAddress, Field, *Delta);
      if (!Address)
        return fail<void>("x86_64 Mach-O FDE target address overflows");

      const auto Symbols = SymbolsByAddress.find(*Address);
      if (Symbols == SymbolsByAddress.end() || !Symbols->second.Text) {
        if (Symbols != SymbolsByAddress.end() && Symbols->second.HasNonText &&
            !Symbols->second.HasText)
          return fail<void>("x86_64 Mach-O FDE target is not a text symbol");
        return fail<void>("unresolved x86_64 Mach-O FDE target");
      }
      auto Added = Graph.addEHFrameReference(EHFrameReference{
          Section, static_cast<uint64_t>(Field), *Symbols->second.Text});
      if (!Added)
        return Unexpected<Diagnostic>(std::move(Added.error()));
    }
  }
  return {};
}

std::optional<RelocationMetadata>
relocationMetadata(const llvm::object::ObjectFile &Object,
                   const llvm::object::RelocationRef &Relocation,
                   Target TargetValue) noexcept {
  constexpr uint8_t WordPatch = 4;
  constexpr uint8_t DoubleWordPatch = 8;
  constexpr unsigned MachORelocationLengthMax = 3;
  RelocationMetadata Metadata;
  if (Relocation.getType() > UINT32_MAX) {
    return std::nullopt;
  }
  const uint32_t Type = static_cast<uint32_t>(Relocation.getType());
  if (const auto *MachO =
          llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object)) {
    const auto Raw = MachO->getRelocation(Relocation.getRawDataRefImpl());
    Metadata.Scattered = MachO->isRelocationScattered(Raw);
    Metadata.PCRelative = MachO->getAnyRelocationPCRel(Raw) != 0;
    const unsigned Length = MachO->getAnyRelocationLength(Raw);
    if (Length > MachORelocationLengthMax) {
      return std::nullopt;
    }
    Metadata.PatchSize = static_cast<uint8_t>(BytePatch << Length);
    Metadata.External =
        !Metadata.Scattered && MachO->getPlainRelocationExternal(Raw);
    if (!Internal::supportsMachORelocationMetadata(TargetValue,
                                                   Metadata.Scattered)) {
      return std::nullopt;
    }
    if (TargetValue == Target::X86_64 &&
        Type == llvm::MachO::X86_64_RELOC_SIGNED &&
        (!Metadata.PCRelative || Metadata.PatchSize != WordPatch)) {
      return std::nullopt;
    }
    return Metadata;
  }
  if (TargetValue == Target::X86_64 && Object.isELF()) {
    if (Type == llvm::ELF::R_X86_64_64) {
      Metadata.PatchSize = DoubleWordPatch;
    } else if (Type == llvm::ELF::R_X86_64_PC32 ||
               Type == llvm::ELF::R_X86_64_PLT32 ||
               Type == llvm::ELF::R_X86_64_GOTPCRELX ||
               Type == llvm::ELF::R_X86_64_REX_GOTPCRELX) {
      Metadata.PatchSize = WordPatch;
    }
  } else if (TargetValue == Target::X86_64 && Object.isCOFF() &&
             Type >= llvm::COFF::IMAGE_REL_AMD64_REL32 &&
             Type <= llvm::COFF::IMAGE_REL_AMD64_REL32_5) {
    Metadata.PatchSize = WordPatch;
  }
  Metadata.PCRelative =
      relocationIsPCRelative(objectFormat(Object), TargetValue, Type);
  return Metadata;
}

struct MachOAddendPair {
  uint32_t Type;
  RelocationMetadata Metadata;
  int64_t Addend;
};

LinkExpect<MachOAddendPair>
consumeAArch64MachOAddendPair(const llvm::object::ObjectFile &Object,
                              Span<const Byte> Content,
                              llvm::object::relocation_iterator &Relocation,
                              const llvm::object::relocation_iterator &End) {
  const auto AddendMetadata =
      relocationMetadata(Object, *Relocation, Target::AArch64);
  if (!AddendMetadata || AddendMetadata->PatchSize != 4 ||
      AddendMetadata->PCRelative || AddendMetadata->External ||
      AddendMetadata->Scattered)
    return fail<MachOAddendPair>("malformed addend relocation metadata");

  // LLVM MC and lld pair this signed 24-bit addend with BRANCH26, PAGE21, or
  // PAGEOFF12 at the same address.
  const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&Object);
  const auto Raw = MachO->getRelocation(Relocation->getRawDataRefImpl());
  const uint32_t Payload =
      MachO->getPlainRelocationSymbolNum(Raw) & UINT32_C(0x00FFFFFF);
  int64_t Addend = static_cast<int64_t>(Payload);
  if ((Payload & UINT32_C(0x00800000)) != 0)
    Addend -= INT64_C(1) << 24;
  const uint64_t Offset = Relocation->getOffset();
  ++Relocation;
  if (Relocation == End)
    return fail<MachOAddendPair>(
        "AArch64 Mach-O addend relocation lacks successor");
  if (Relocation->getType() > UINT32_MAX)
    return fail<MachOAddendPair>("relocation type is out of range");
  const uint32_t Type = static_cast<uint32_t>(Relocation->getType());
  if (Type != llvm::MachO::ARM64_RELOC_BRANCH26 &&
      Type != llvm::MachO::ARM64_RELOC_PAGE21 &&
      Type != llvm::MachO::ARM64_RELOC_PAGEOFF12)
    return fail<MachOAddendPair>(
        "unsupported AArch64 Mach-O addend relocation successor",
        DiagnosticKind::Unsupported);
  const auto Metadata =
      relocationMetadata(Object, *Relocation, Target::AArch64);
  const bool ExpectedPCRelative = Type != llvm::MachO::ARM64_RELOC_PAGEOFF12;
  if (!Metadata || Metadata->PatchSize != 4 || Metadata->Scattered ||
      !Metadata->External || Metadata->PCRelative != ExpectedPCRelative)
    return fail<MachOAddendPair>("malformed addend relocation successor");
  if (Relocation->getOffset() != Offset)
    return fail<MachOAddendPair>(
        "AArch64 Mach-O addend relocation addresses differ");
  const auto Embedded =
      Internal::readUnsigned(Content, Offset, 4, Endianness::Little);
  if (!Embedded)
    return fail<MachOAddendPair>("cannot read AArch64 Mach-O addend field");
  const uint32_t Instruction = static_cast<uint32_t>(*Embedded);
  const bool HasEmbeddedAddend =
      Type == llvm::MachO::ARM64_RELOC_BRANCH26
          ? (Instruction & UINT32_C(0x03FFFFFF)) != 0
      : Type == llvm::MachO::ARM64_RELOC_PAGE21
          ? (Instruction & UINT32_C(0x60FFFFE0)) != 0
          : (Instruction & UINT32_C(0x003FFC00)) != 0;
  if (HasEmbeddedAddend)
    return fail<MachOAddendPair>(
        "AArch64 Mach-O addend relocation conflicts with embedded addend");
  return MachOAddendPair{Type, *Metadata, Addend};
}

} // namespace

namespace Internal {

ObjectReaderInputPolicy nativeObjectInputPolicy(ObjectFormat Format) noexcept {
  return Format == ObjectFormat::COFF
             ? ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused
             : ObjectReaderInputPolicy::Strict;
}

uint64_t normalizeSectionAlignment(uint64_t Alignment) noexcept {
  return std::max<uint64_t>(Alignment, 1);
}

std::optional<std::map<std::string, std::string>>
parseCOFFExports(std::string_view Input) {
  constexpr llvm::StringLiteral COFFExportPrefix = "/export:";
  constexpr size_t QuotedTokenDelimiterCount = 2;
  llvm::StringRef Directives(Input.data(), Input.size());
  std::map<std::string, std::string> Exports;
  while (!Directives.trim().empty()) {
    Directives = Directives.ltrim();
    llvm::StringRef Token;
    if (Directives.front() == '"') {
      const auto End = Directives.drop_front().find('"');
      if (End == llvm::StringRef::npos) {
        return std::nullopt;
      }
      Token = Directives.substr(1, End);
      Directives = Directives.drop_front(End + QuotedTokenDelimiterCount);
    } else {
      std::tie(Token, Directives) = Directives.split(' ');
    }
    if (Token.size() < COFFExportPrefix.size() ||
#if LLVM_VERSION_MAJOR >= 13
        !Token.take_front(COFFExportPrefix.size())
             .equals_insensitive(COFFExportPrefix)) {
#else
        !Token.take_front(COFFExportPrefix.size())
             .equals_lower(COFFExportPrefix)) {
#endif
      continue;
    }
    Token = Token.drop_front(COFFExportPrefix.size());
    Token = Token.split(',').first;
    auto [ExportName, SymbolName] = Token.split('=');
    auto Unquote = [](llvm::StringRef &Name) {
      const bool StartsQuoted = !Name.empty() && Name.front() == '"';
      const bool EndsQuoted = !Name.empty() && Name.back() == '"';
      if (StartsQuoted != EndsQuoted)
        return false;
      if (StartsQuoted)
        Name = Name.drop_front().drop_back();
      return true;
    };
    if (!Unquote(ExportName) || !Unquote(SymbolName) || ExportName.empty()) {
      return std::nullopt;
    }
    Exports.emplace(ExportName.str(),
                    SymbolName.empty() ? ExportName.str() : SymbolName.str());
  }
  return Exports;
}

LinkExpect<MachOBuildVersion> parseMachOBuildVersion(Span<const Byte> Command,
                                                     Endianness Endian) {
  constexpr uint64_t HeaderSize = 24;
  constexpr uint64_t ToolSize = 8;
  constexpr uint64_t CommandOffset = 0;
  constexpr uint64_t CommandSizeOffset = 4;
  constexpr uint64_t PlatformOffset = 8;
  constexpr uint64_t MinimumOSOffset = 12;
  constexpr uint64_t SDKOffset = 16;
  constexpr uint64_t ToolCountOffset = 20;
  const bool LittleEndian = Endian == Endianness::Little;
  uint32_t Type = 0;
  uint32_t CommandSize = 0;
  uint32_t Platform = 0;
  uint32_t MinimumOS = 0;
  uint32_t SDK = 0;
  uint32_t ToolCount = 0;
  if (Command.size() < HeaderSize ||
      !readELFInteger(Command, CommandOffset, LittleEndian, Type) ||
      !readELFInteger(Command, CommandSizeOffset, LittleEndian, CommandSize) ||
      !readELFInteger(Command, PlatformOffset, LittleEndian, Platform) ||
      !readELFInteger(Command, MinimumOSOffset, LittleEndian, MinimumOS) ||
      !readELFInteger(Command, SDKOffset, LittleEndian, SDK) ||
      !readELFInteger(Command, ToolCountOffset, LittleEndian, ToolCount) ||
      Type != llvm::MachO::LC_BUILD_VERSION || CommandSize != Command.size() ||
      ToolCount > (Command.size() - HeaderSize) / ToolSize ||
      HeaderSize + static_cast<uint64_t>(ToolCount) * ToolSize !=
          Command.size())
    return fail<MachOBuildVersion>("malformed Mach-O build version command");

  MachOBuildVersion Result{Platform, MinimumOS, SDK, {}};
  Result.Tools.reserve(ToolCount);
  for (uint32_t I = 0; I < ToolCount; ++I) {
    const uint64_t Offset = HeaderSize + static_cast<uint64_t>(I) * ToolSize;
    uint32_t Tool = 0;
    uint32_t Version = 0;
    if (!readELFInteger(Command, Offset, LittleEndian, Tool) ||
        !readELFInteger(Command, Offset + 4, LittleEndian, Version))
      return fail<MachOBuildVersion>("malformed Mach-O build version command");
    Result.Tools.push_back(MachOBuildToolVersion{Tool, Version});
  }
  return Result;
}

LinkExpect<uint64_t> resolveCompactUnwindTargetOffset(bool External,
                                                      uint64_t SectionAddress,
                                                      uint64_t SymbolOffset,
                                                      uint64_t RawAddend) {
  if (!External) {
    if (RawAddend < SectionAddress)
      return Unexpected<Diagnostic>(
          Diagnostic{"compact unwind local target precedes its section"});
    return RawAddend - SectionAddress;
  }
  if (RawAddend <= static_cast<uint64_t>(INT64_MAX)) {
    if (RawAddend > UINT64_MAX - SymbolOffset)
      return Unexpected<Diagnostic>(
          Diagnostic{"compact unwind target address overflows"});
    return SymbolOffset + RawAddend;
  }
  const uint64_t Magnitude = UINT64_MAX - RawAddend + 1;
  if (Magnitude > SymbolOffset)
    return Unexpected<Diagnostic>(
        Diagnostic{"compact unwind target address underflows"});
  return SymbolOffset - Magnitude;
}

LinkExpect<std::vector<DecodedCompactUnwindRecord>>
parseCompactUnwindSection(Target TargetValue, Span<const Byte> Content,
                          Span<const CompactUnwindRelocation> Relocations) {
  if (Content.size() % CompactUnwindRecordSize != 0)
    return Unexpected<Diagnostic>(
        Diagnostic{"compact unwind size is not a multiple of 32"});
  const uint32_t ExpectedType =
      TargetValue == Target::AArch64
          ? static_cast<uint32_t>(llvm::MachO::ARM64_RELOC_UNSIGNED)
      : TargetValue == Target::X86_64
          ? static_cast<uint32_t>(llvm::MachO::X86_64_RELOC_UNSIGNED)
          : UINT32_MAX;
  std::set<uint64_t> Fields;
  for (const auto &Relocation : Relocations) {
    const uint64_t Field = Relocation.Offset % CompactUnwindRecordSize;
    if (Relocation.Offset >= Content.size() ||
        (Field != CompactUnwindFunctionOffset &&
         Field != CompactUnwindPersonalityOffset &&
         Field != CompactUnwindLSDAOffset))
      return fail<std::vector<DecodedCompactUnwindRecord>>(
          "unsupported compact unwind relocation field",
          DiagnosticKind::Unsupported);
    if (!Fields.emplace(Relocation.Offset).second)
      return Unexpected<Diagnostic>(
          Diagnostic{"duplicate compact unwind relocation field"});
    if (Relocation.Type != ExpectedType || Relocation.PatchSize != 8 ||
        Relocation.PCRelative || Relocation.Scattered)
      return fail<std::vector<DecodedCompactUnwindRecord>>(
          "unsupported compact unwind relocation", DiagnosticKind::Unsupported);
  }

  std::vector<DecodedCompactUnwindRecord> Records;
  Records.reserve(Content.size() / CompactUnwindRecordSize);
  for (uint64_t Offset = 0; Offset < Content.size();
       Offset += CompactUnwindRecordSize) {
    if (Fields.count(Offset + CompactUnwindFunctionOffset) == 0)
      return Unexpected<Diagnostic>(
          Diagnostic{"compact unwind record lacks function relocation"});
    DecodedCompactUnwindRecord Record{};
    if (!readELFInteger(Content, Offset + CompactUnwindFunctionOffset, true,
                        Record.Function) ||
        !readELFInteger(Content, Offset + CompactUnwindLengthOffset, true,
                        Record.Length) ||
        !readELFInteger(Content, Offset + CompactUnwindEncodingOffset, true,
                        Record.Encoding) ||
        !readELFInteger(Content, Offset + CompactUnwindPersonalityOffset, true,
                        Record.Personality) ||
        !readELFInteger(Content, Offset + CompactUnwindLSDAOffset, true,
                        Record.LSDA))
      return Unexpected<Diagnostic>(
          Diagnostic{"cannot read compact unwind record"});
    if ((Record.Personality != 0 &&
         Fields.count(Offset + CompactUnwindPersonalityOffset) == 0) ||
        (Record.LSDA != 0 &&
         Fields.count(Offset + CompactUnwindLSDAOffset) == 0))
      return Unexpected<Diagnostic>(
          Diagnostic{"compact unwind target lacks relocation"});
    Records.push_back(Record);
  }
  return Records;
}

} // namespace Internal

LinkExpect<LinkGraph> ObjectReader::read(Span<const Byte> Buffer,
                                         Target ExpectedTarget,
                                         ObjectReaderInputPolicy InputPolicy) {
  if (Buffer.empty()) {
    return fail<LinkGraph>("empty object buffer");
  }
  if (!validELFRelocations(Buffer)) {
    return fail<LinkGraph>("malformed ELF relocation metadata");
  }
  const auto Data = llvm::StringRef(
      reinterpret_cast<const char *>(Buffer.data()), Buffer.size());
  auto ObjectResult = llvm::object::ObjectFile::createObjectFile(
      llvm::MemoryBufferRef(Data, "object"));
  if (!ObjectResult) {
    return fail<LinkGraph>("object file parse error: " +
                           llvm::toString(ObjectResult.takeError()));
  }
  const auto &Object = **ObjectResult;
  const auto TrailingCOFF = findTrailingCOFFObject(Buffer, Object);
  if (TrailingCOFF == COFFTrailingObject::Malformed) {
    return fail<LinkGraph>("malformed COFF structural metadata");
  }
  if (hasTrailingELFObject(Buffer, Object) ||
      TrailingCOFF == COFFTrailingObject::Found) {
    return fail<LinkGraph>("multiple input objects are not supported");
  }
  if (!Object.isRelocatableObject()) {
    return fail<LinkGraph>("input is not a relocatable object");
  }
  if (!isSupportedFormat(Object)) {
    return fail<LinkGraph>("unsupported object format",
                           DiagnosticKind::Unsupported);
  }
  const auto ActualTarget = normalizeTarget(Object.getArch());
  if (!ActualTarget) {
    return fail<LinkGraph>("unsupported object architecture",
                           DiagnosticKind::Unsupported);
  }
  if (*ActualTarget != ExpectedTarget) {
    return fail<LinkGraph>("object target does not match expected host target");
  }
  if (const auto *MachO =
          llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object);
      MachO && MachO->getHeader().cputype == MachOCPUTypeArm64 &&
      (MachO->getHeader().cpusubtype & ~MachOCPUSubtypeMask) !=
          MachOCPUSubtypeArm64All) {
    return fail<LinkGraph>(
        "unsupported non-generic AArch64 Mach-O CPU subtype (including arm64e)",
        DiagnosticKind::Unsupported);
  }

  LinkGraph Graph(*ActualTarget,
                  Object.isLittleEndian() ? Endianness::Little
                                          : Endianness::Big,
                  objectFormat(Object));
  if (auto Begun = Graph.beginInput("object"); !Begun)
    return addContext(std::move(Begun.error()),
                      "cannot initialize link graph input");
  if (const auto *MachO =
          llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object)) {
    for (const auto &Command : MachO->load_commands()) {
      if (Command.C.cmd != llvm::MachO::LC_BUILD_VERSION)
        continue;
      const Span<const Byte> Bytes(reinterpret_cast<const Byte *>(Command.Ptr),
                                   Command.C.cmdsize);
      auto Version =
          Internal::parseMachOBuildVersion(Bytes, Graph.endianness());
      if (!Version)
        return Unexpected<Diagnostic>(std::move(Version.error()));
      if (auto Added = Graph.addMachOBuildVersion(std::move(*Version)); !Added)
        return addContext(std::move(Added.error()),
                          "cannot preserve Mach-O build version");
    }
  }
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELFObjectFileBase>(&Object)) {
    uint32_t InputFlags = ELF->getPlatformFlags();
    if (*ActualTarget == Target::ARM) {
      auto Flags = armELFFlags(Object, InputFlags);
      if (!Flags)
        return Unexpected<Diagnostic>(std::move(Flags.error()));
      InputFlags = *Flags;
    }
    if (auto Flags = Graph.setELFFlags(InputFlags); !Flags)
      return addContext(std::move(Flags.error()), "cannot preserve ELF flags");
  }
  std::map<uint64_t, SectionId> SectionIds;
  std::map<std::string, std::string> COFFExports;
  std::optional<uint64_t> CompactUnwindSection;
  std::set<uint64_t> MachOFunctionSections;
  if (Object.isMachO()) {
    const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&Object);
    for (const auto &InputSection : Object.sections()) {
      const uint32_t Flags =
          MachO->is64Bit()
              ? MachO->getSection64(InputSection.getRawDataRefImpl()).flags
              : MachO->getSection(InputSection.getRawDataRefImpl()).flags;
      if ((Flags & (llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                    llvm::MachO::S_ATTR_SOME_INSTRUCTIONS)) != 0)
        MachOFunctionSections.emplace(InputSection.getIndex());
    }
    for (const auto &InputSymbol : Object.symbols()) {
      llvm::object::SymbolRef::Type Type{};
      uint32_t Flags = 0;
      Diagnostic Error{""};
      if (!take(InputSymbol.getType(), Type, "cannot read symbol type",
                &Error) ||
          !symbolFlags(InputSymbol, Flags, &Error))
        return Unexpected<Diagnostic>(std::move(Error));
      if (Type != llvm::object::SymbolRef::ST_Function &&
          (Flags & llvm::object::SymbolRef::SF_Executable) == 0)
        continue;
      auto InputSection = Object.section_end();
      if (!take(InputSymbol.getSection(), InputSection,
                "cannot read function symbol section", &Error))
        return Unexpected<Diagnostic>(std::move(Error));
      if (InputSection != Object.section_end())
        MachOFunctionSections.emplace(InputSection->getIndex());
    }
    for (const auto &InputSection : Object.sections()) {
      llvm::StringRef Name;
      Diagnostic Error{""};
      if (!take(InputSection.getName(), Name, "cannot read section name",
                &Error))
        return Unexpected<Diagnostic>(std::move(Error));
      if (sectionPurpose(Object, Name) != SectionPurpose::CompactUnwind)
        continue;
      for (const auto &Relocation : InputSection.relocations()) {
        if (Relocation.getOffset() % CompactUnwindRecordSize !=
            CompactUnwindFunctionOffset)
          continue;
        auto TargetSection = Object.section_end();
        const auto InputSymbol = Relocation.getSymbol();
        if (InputSymbol == Object.symbol_end()) {
          TargetSection =
              MachO->getRelocationSection(Relocation.getRawDataRefImpl());
        } else {
          if (!take(InputSymbol->getSection(), TargetSection,
                    "cannot read compact unwind function section", &Error))
            return Unexpected<Diagnostic>(std::move(Error));
        }
        if (TargetSection != Object.section_end())
          MachOFunctionSections.emplace(TargetSection->getIndex());
      }
    }
  }
  for (const auto &InputSection : Object.sections()) {
    if (*ActualTarget == Target::ARM && Object.isELF() &&
        llvm::object::ELFSectionRef(InputSection).getType() ==
            llvm::ELF::SHT_ARM_ATTRIBUTES)
      continue;
    llvm::StringRef Name;
    Diagnostic Error{""};
    if (!take(InputSection.getName(), Name, "cannot read section name", &Error))
      return Unexpected<Diagnostic>(std::move(Error));
    if (Object.isCOFF() && Name == ".drectve") {
      llvm::StringRef Contents;
      if (!take(InputSection.getContents(), Contents, "cannot read .drectve",
                &Error))
        return Unexpected<Diagnostic>(std::move(Error));
      auto Exports = Internal::parseCOFFExports(Contents.str());
      if (!Exports) {
        return fail<LinkGraph>("malformed COFF .drectve export directive");
      }
      COFFExports.insert(Exports->begin(), Exports->end());
    }
    if (!isAllocatable(Object, InputSection)) {
      continue;
    }
    llvm::StringRef Contents;
    if (!InputSection.isVirtual() &&
        !take(InputSection.getContents(), Contents,
              "cannot read section contents", &Error))
      return Unexpected<Diagnostic>(std::move(Error));
    std::vector<Byte> Bytes(Contents.bytes_begin(), Contents.bytes_end());
    const auto Purpose = sectionPurpose(Object, Name);
    if (Purpose == SectionPurpose::CompactUnwind) {
      if (CompactUnwindSection)
        return fail<LinkGraph>("multiple compact unwind sections");
      CompactUnwindSection = InputSection.getIndex();
      continue;
    }
    constexpr uint64_t EHFrameTerminatorSize = 4;
    const uint64_t VirtualSize =
        InputSection.getSize() +
        (Purpose == SectionPurpose::EHFrame ? EHFrameTerminatorSize : 0);
    if (VirtualSize < InputSection.getSize())
      return fail<LinkGraph>("section size overflows");
    if (VirtualSize != InputSection.getSize())
      Bytes.resize(static_cast<size_t>(VirtualSize));
    const auto Kind = MachOFunctionSections.count(InputSection.getIndex()) != 0
                          ? SectionKind::Text
                          : sectionKind(InputSection, Purpose);
    auto Added = Graph.addSection(
        Section{Name.str(), Kind, sectionAlignment(InputSection), VirtualSize,
                0, 0, std::move(Bytes), Purpose, InputSection.getAddress()});
    if (!Added) {
      return Unexpected<Diagnostic>(std::move(Added.error()));
    }
    SectionIds.emplace(InputSection.getIndex(), *Added);
  }
  for (const auto &InputSection : Object.sections()) {
    const auto Section = SectionIds.find(InputSection.getIndex());
    if (Section == SectionIds.end())
      continue;
    const auto &Value = Graph.sections()[Section->second];
    if (Value.Purpose != SectionPurpose::ARMExidx)
      continue;
    const auto LinkedInput = elfLinkedSection(Object, InputSection);
    if (!LinkedInput)
      return fail<LinkGraph>("cannot read linked ARM exidx section");
    const auto Linked = SectionIds.find(*LinkedInput);
    if (Linked == SectionIds.end() ||
        Graph.sections()[Linked->second].Kind != SectionKind::Text)
      return fail<LinkGraph>("invalid linked ARM exidx section");
    if (auto Associated =
            Graph.setLinkedSection(Section->second, Linked->second);
        !Associated)
      return addContext(std::move(Associated.error()),
                        "invalid linked ARM exidx section");
  }

  std::map<llvm::object::DataRefImpl, SymbolId> SymbolIds;
  std::set<llvm::object::DataRefImpl> IgnorableUndefinedSymbols;
  std::map<llvm::object::DataRefImpl, uint64_t> SymbolSizes;
  for (const auto &[InputSymbol, Size] :
       llvm::object::computeSymbolSizes(Object)) {
    SymbolSizes.emplace(InputSymbol.getRawDataRefImpl(), Size);
  }
  for (const auto &InputSymbol : Object.symbols()) {
    llvm::StringRef Name;
    uint32_t Flags = 0;
    llvm::object::SymbolRef::Type Type{};
    Diagnostic Error{""};
    if (!take(InputSymbol.getName(), Name, "cannot read symbol name", &Error) ||
        !symbolFlags(InputSymbol, Flags, &Error) ||
        !take(InputSymbol.getType(), Type, "cannot read symbol type", &Error))
      return Unexpected<Diagnostic>(std::move(Error));
    if ((Flags & llvm::object::SymbolRef::SF_Undefined) != 0) {
      if (!Name.empty() && Type != llvm::object::SymbolRef::ST_File &&
          Type != llvm::object::SymbolRef::ST_Debug) {
        if (InputPolicy ==
                ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused &&
            Object.isCOFF() && Name == "_fltused" &&
            (*ActualTarget == Target::X86_64 ||
             *ActualTarget == Target::AArch64)) {
          IgnorableUndefinedSymbols.insert(InputSymbol.getRawDataRefImpl());
          continue;
        }
        Diagnostic Diag{"undefined symbol"};
        Diag.SymbolName = Name.str();
        return Unexpected<Diagnostic>(std::move(Diag));
      }
      continue;
    }
    auto InputSection = InputSymbol.getSection();
    if (!InputSection) {
      Diagnostic Diag{"cannot read symbol section: " +
                      llvm::toString(InputSection.takeError())};
      Diag.SymbolName = Name.str();
      return Unexpected<Diagnostic>(std::move(Diag));
    }
    if (*InputSection == Object.section_end()) {
      continue;
    }
    const auto Section = SectionIds.find((*InputSection)->getIndex());
    if (Section == SectionIds.end()) {
      continue;
    }
    if (Name.empty()) {
      Name = "$section";
    }
    uint64_t Address = 0;
    if (!take(InputSymbol.getAddress(), Address, "cannot read symbol address",
              &Error))
      return Unexpected<Diagnostic>(std::move(Error));
    const uint64_t Base = (*InputSection)->getAddress();
    if (Address < Base) {
      return fail<LinkGraph>("symbol address precedes its section");
    }
    bool Exported = (Flags & llvm::object::SymbolRef::SF_Exported) != 0;
    const bool Global = (Flags & llvm::object::SymbolRef::SF_Global) != 0;
    const bool Thumb = (Flags & llvm::object::SymbolRef::SF_Thumb) != 0;
    std::optional<std::string> ExportName;
    if (Object.isMachO()) {
      Exported = (Flags & llvm::object::SymbolRef::SF_Global) != 0 &&
                 (Flags & llvm::object::SymbolRef::SF_Hidden) == 0;
    } else if (Object.isCOFF()) {
      const auto Export =
          std::find_if(COFFExports.begin(), COFFExports.end(),
                       [&](const auto &Entry) { return Entry.second == Name; });
      Exported = Export != COFFExports.end();
      if (Exported && Export->first != Name) {
        ExportName = Export->first;
      }
    }
    std::string SymbolName = Name.str();
    if (SymbolName == "$section") {
      SymbolName += std::to_string((*InputSection)->getIndex());
    }
    if (!Exported && Graph.findSymbol(SymbolName)) {
      SymbolName += "." + std::to_string((*InputSection)->getIndex()) + "." +
                    std::to_string(SymbolIds.size());
    }
    uint64_t SymbolSize = SymbolSizes[InputSymbol.getRawDataRefImpl()];
    if (const auto *COFF =
            llvm::dyn_cast<llvm::object::COFFObjectFile>(&Object);
        COFF && COFF->getCOFFSymbol(InputSymbol).isSectionDefinition())
      SymbolSize = 0;
    auto Added = Graph.addSymbol(Symbol{std::move(SymbolName), Section->second,
                                        Address - Base, SymbolSize, Exported,
                                        ExportName, Global, Thumb});
    if (!Added) {
      return Unexpected<Diagnostic>(std::move(Added.error()));
    }
    SymbolIds.emplace(InputSymbol.getRawDataRefImpl(), *Added);
  }

  if (CompactUnwindSection) {
    auto InputSection = std::find_if(
        Object.sections().begin(), Object.sections().end(),
        [&](const auto &S) { return S.getIndex() == *CompactUnwindSection; });
    if (InputSection == Object.sections().end())
      return fail<LinkGraph>("cannot find compact unwind section");
    llvm::StringRef Contents;
    Diagnostic Error{""};
    if (!take(InputSection->getContents(), Contents,
              "cannot read compact unwind contents", &Error))
      return Unexpected<Diagnostic>(std::move(Error));
    const Span<const Byte> Bytes(
        reinterpret_cast<const Byte *>(Contents.data()), Contents.size());
    if (Bytes.size() % CompactUnwindRecordSize != 0)
      return fail<LinkGraph>("compact unwind size is not a multiple of 32");

    std::map<uint64_t, llvm::object::RelocationRef> Relocations;
    std::vector<Internal::CompactUnwindRelocation> DecodedRelocations;
    for (const auto &Relocation : InputSection->relocations()) {
      const auto Metadata =
          relocationMetadata(Object, Relocation, *ActualTarget);
      if (!Metadata || Relocation.getType() > UINT32_MAX)
        return fail<LinkGraph>("malformed compact unwind relocation");
      DecodedRelocations.push_back(Internal::CompactUnwindRelocation{
          Relocation.getOffset(), static_cast<uint32_t>(Relocation.getType()),
          Metadata->PatchSize, Metadata->PCRelative, Metadata->External,
          Metadata->Scattered});
      Relocations.emplace(Relocation.getOffset(), Relocation);
    }
    auto Records = Internal::parseCompactUnwindSection(*ActualTarget, Bytes,
                                                       DecodedRelocations);
    if (!Records)
      return Unexpected<Diagnostic>(std::move(Records.error()));

    const auto Resolve = [&](const llvm::object::RelocationRef &Relocation,
                             uint64_t Addend) -> LinkExpect<SymbolId> {
      const auto Metadata =
          relocationMetadata(Object, Relocation, *ActualTarget);
      if (!Metadata)
        return fail<SymbolId>("malformed compact unwind relocation");
      const auto InputSymbol = Relocation.getSymbol();
      auto TargetSection = Object.section_end();
      uint64_t SymbolOffset = 0;
      if (InputSymbol == Object.symbol_end()) {
        const auto *MachO =
            llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object);
        if (MachO == nullptr)
          return fail<SymbolId>(
              "compact unwind relocation has no target symbol");
        TargetSection =
            MachO->getRelocationSection(Relocation.getRawDataRefImpl());
      } else {
        Diagnostic SectionDetail{""};
        if (!take(InputSymbol->getSection(), TargetSection,
                  "cannot read compact unwind target section", &SectionDetail))
          return Unexpected<Diagnostic>(std::move(SectionDetail));
        uint64_t Address = 0;
        Diagnostic AddressDetail{""};
        if (!take(InputSymbol->getAddress(), Address,
                  "cannot read compact unwind target address", &AddressDetail))
          return Unexpected<Diagnostic>(std::move(AddressDetail));
        if (TargetSection != Object.section_end()) {
          const uint64_t Base = TargetSection->getAddress();
          if (Address < Base)
            return fail<SymbolId>("compact unwind target precedes its section");
          SymbolOffset = Address - Base;
        }
      }
      if (TargetSection == Object.section_end())
        return fail<SymbolId>("compact unwind relocation target is undefined");
      const auto GraphSection = SectionIds.find(TargetSection->getIndex());
      if (GraphSection == SectionIds.end())
        return fail<SymbolId>("compact unwind targets an unsupported section",
                              DiagnosticKind::Unsupported);
      auto Resolved = Internal::resolveCompactUnwindTargetOffset(
          Metadata->External, TargetSection->getAddress(), SymbolOffset,
          Addend);
      if (!Resolved)
        return Unexpected<Diagnostic>(std::move(Resolved.error()));
      const uint64_t Offset = *Resolved;
      const auto &Section = Graph.sections()[GraphSection->second];
      if (Offset >= Section.VirtualSize)
        return fail<SymbolId>("compact unwind target is outside its section");
      auto Existing = Graph.symbols().end();
      for (auto Symbol = Graph.symbols().begin();
           Symbol != Graph.symbols().end(); ++Symbol) {
        if (Symbol->Section != GraphSection->second ||
            Symbol->Offset != Offset || Symbol->Name.rfind("$section", 0) == 0)
          continue;
        if (Existing == Graph.symbols().end() ||
            std::tie(Symbol->Exported, Symbol->Global, Symbol->Size) >
                std::tie(Existing->Exported, Existing->Global, Existing->Size))
          Existing = Symbol;
      }
      if (Existing != Graph.symbols().end())
        return static_cast<SymbolId>(Existing - Graph.symbols().begin());
      auto Added = Graph.addSymbol(Symbol{
          "$compact_unwind." + std::to_string(TargetSection->getIndex()) + "." +
              std::to_string(Offset),
          GraphSection->second, Offset, 0, false});
      if (!Added)
        return Unexpected<Diagnostic>(std::move(Added.error()));
      return *Added;
    };

    const auto ResolveDwarfFDE =
        [&](SymbolId Function, uint32_t Encoding) -> LinkExpect<SymbolId> {
      const uint32_t Mode = Encoding & CompactUnwindModeMask;
      const bool IsDwarf =
          (*ActualTarget == Target::AArch64 &&
           Mode == CompactUnwindAArch64Dwarf) ||
          (*ActualTarget == Target::X86_64 && Mode == CompactUnwindX8664Dwarf);
      if (!IsDwarf)
        return fail<SymbolId>("compact unwind encoding is not DWARF");
      if (Function >= Graph.symbols().size())
        return fail<SymbolId>("invalid DWARF compact unwind function");
      const auto &FunctionSymbol = Graph.symbols()[Function];
      const uint32_t EncodedOffset = Encoding & CompactUnwindDwarfOffsetMask;
      bool CanonicalCandidate = false;
      std::optional<std::pair<SectionId, uint64_t>> Match;
      for (const auto &EHSection : Object.sections()) {
        const auto GraphSection = SectionIds.find(EHSection.getIndex());
        if (GraphSection == SectionIds.end() ||
            Graph.sections()[GraphSection->second].Purpose !=
                SectionPurpose::EHFrame)
          continue;
        llvm::StringRef EHContents;
        Diagnostic Detail{""};
        if (!take(EHSection.getContents(), EHContents,
                  "cannot read EH frame contents", &Detail))
          return Unexpected<Diagnostic>(std::move(Detail));
        const Span<const Byte> EHBytes(
            reinterpret_cast<const Byte *>(EHContents.data()),
            EHContents.size());
        std::optional<std::pair<uint64_t, SymbolId>> Subtractor;
        for (const auto &Relocation : EHSection.relocations()) {
          if (*ActualTarget == Target::AArch64 &&
              Relocation.getType() == llvm::MachO::ARM64_RELOC_SUBTRACTOR) {
            const auto InputSymbol = Relocation.getSymbol();
            if (InputSymbol == Object.symbol_end()) {
              Subtractor.reset();
              continue;
            }
            const auto Symbol =
                SymbolIds.find(InputSymbol->getRawDataRefImpl());
            if (Symbol == SymbolIds.end()) {
              Subtractor.reset();
              continue;
            }
            Subtractor = std::pair{Relocation.getOffset(), Symbol->second};
            continue;
          }
          const auto InputSymbol = Relocation.getSymbol();
          SectionId TargetSection = InvalidSectionId;
          uint64_t TargetOffset = 0;
          if (*ActualTarget == Target::AArch64 &&
              Relocation.getType() == llvm::MachO::ARM64_RELOC_UNSIGNED &&
              Subtractor && Subtractor->first == Relocation.getOffset()) {
            auto InputTargetSection = Object.section_end();
            if (InputSymbol == Object.symbol_end()) {
              const auto *MachO =
                  llvm::cast<llvm::object::MachOObjectFile>(&Object);
              InputTargetSection =
                  MachO->getRelocationSection(Relocation.getRawDataRefImpl());
            } else {
              Diagnostic SectionDetail{""};
              if (!take(InputSymbol->getSection(), InputTargetSection,
                        "cannot read DWARF FDE target section", &SectionDetail))
                return Unexpected<Diagnostic>(std::move(SectionDetail));
              const auto Symbol =
                  SymbolIds.find(InputSymbol->getRawDataRefImpl());
              if (Symbol == SymbolIds.end())
                continue;
              TargetOffset = Graph.symbols()[Symbol->second].Offset;
            }
            if (InputTargetSection == Object.section_end())
              continue;
            const auto GraphTargetSection =
                SectionIds.find(InputTargetSection->getIndex());
            if (GraphTargetSection == SectionIds.end())
              continue;
            TargetSection = GraphTargetSection->second;
            const auto &SubtractorSymbol = Graph.symbols()[Subtractor->second];
            const uint64_t Field = Relocation.getOffset();
            if (SubtractorSymbol.Offset > Field)
              return fail<SymbolId>("invalid DWARF FDE subtractor offset");
            uint64_t Raw = 0;
            if (!readELFInteger(EHBytes, Field, true, Raw))
              return fail<SymbolId>("cannot read DWARF FDE addend");
            auto Resolved = Internal::resolveMachOFDEAddress(
                TargetOffset, 0, Field - SubtractorSymbol.Offset,
                static_cast<int64_t>(Raw));
            if (!Resolved)
              return fail<SymbolId>("DWARF FDE target address overflows");
            TargetOffset = *Resolved;
            Subtractor.reset();
          } else {
            if (InputSymbol == Object.symbol_end())
              continue;
            const auto Symbol =
                SymbolIds.find(InputSymbol->getRawDataRefImpl());
            if (Symbol == SymbolIds.end())
              continue;
            const auto &Target = Graph.symbols()[Symbol->second];
            TargetSection = Target.Section;
            TargetOffset = Target.Offset;
          }
          if (TargetSection != FunctionSymbol.Section ||
              TargetOffset != FunctionSymbol.Offset)
            continue;
          CanonicalCandidate = true;
          const uint64_t Field = Relocation.getOffset();
          if (Field < 8)
            return fail<SymbolId>("invalid DWARF compact unwind FDE field");
          const uint64_t Record = Field - 8;
          uint64_t Cursor = 0;
          bool Boundary = false;
          while (Cursor < EHBytes.size()) {
            uint32_t Length = 0;
            if (!readELFInteger(EHBytes, Cursor, true, Length) || Length == 0 ||
                Length == UINT32_MAX || Length > EHBytes.size() - Cursor - 4)
              return fail<SymbolId>("malformed EH frame record");
            if (Cursor == Record) {
              uint32_t Id = 0;
              if (!readELFInteger(EHBytes, Cursor + 4, true, Id) || Id == 0)
                return fail<SymbolId>("invalid DWARF compact unwind FDE");
              Boundary = true;
              break;
            }
            Cursor += 4 + Length;
          }
          if (!Boundary)
            return fail<SymbolId>("DWARF compact unwind offset is not an FDE");
          if (EncodedOffset != 0 && EncodedOffset != Record)
            continue;
          if (Match && *Match != std::pair{GraphSection->second, Record})
            return fail<SymbolId>("ambiguous DWARF compact unwind FDE");
          Match = std::pair{GraphSection->second, Record};
        }
      }
      if (!Match && EncodedOffset != 0 && CanonicalCandidate)
        return fail<SymbolId>("DWARF compact unwind FDE offset mismatch");
      if (!Match)
        return fail<SymbolId>("missing DWARF compact unwind FDE");
      const auto Existing =
          std::find_if(Graph.symbols().begin(), Graph.symbols().end(),
                       [&](const auto &Value) {
                         return Value.Section == Match->first &&
                                Value.Offset == Match->second;
                       });
      if (Existing != Graph.symbols().end())
        return static_cast<SymbolId>(Existing - Graph.symbols().begin());
      auto Added = Graph.addSymbol(
          Symbol{"$compact_unwind.fde." + std::to_string(Match->second),
                 Match->first, Match->second, 0, false});
      if (!Added)
        return Unexpected<Diagnostic>(std::move(Added.error()));
      return *Added;
    };

    for (size_t I = 0; I < Records->size(); ++I) {
      const uint64_t Offset = I * CompactUnwindRecordSize;
      const auto &Record = (*Records)[I];
      const auto FunctionRelocation =
          Relocations.find(Offset + CompactUnwindFunctionOffset);
      if (FunctionRelocation == Relocations.end())
        return fail<LinkGraph>(
            "compact unwind record lacks function relocation");
      auto Function = Resolve(FunctionRelocation->second, Record.Function);
      if (!Function)
        return Unexpected<Diagnostic>(std::move(Function.error()));
      const auto ResolveOptional =
          [&](uint64_t Field) -> LinkExpect<std::optional<SymbolId>> {
        const auto Relocation = Relocations.find(Offset + Field);
        if (Relocation == Relocations.end())
          return std::optional<SymbolId>{};
        uint64_t Addend = 0;
        if (!readELFInteger(Bytes, Offset + Field, true, Addend))
          return fail<std::optional<SymbolId>>(
              "cannot read compact unwind target");
        auto Symbol = Resolve(Relocation->second, Addend);
        if (!Symbol)
          return Unexpected<Diagnostic>(std::move(Symbol.error()));
        return std::optional<SymbolId>{*Symbol};
      };
      auto Personality = ResolveOptional(CompactUnwindPersonalityOffset);
      auto LSDA = ResolveOptional(CompactUnwindLSDAOffset);
      if (!Personality)
        return Unexpected<Diagnostic>(std::move(Personality.error()));
      if (!LSDA)
        return Unexpected<Diagnostic>(std::move(LSDA.error()));
      std::optional<SymbolId> FDE;
      const uint32_t Mode = Record.Encoding & CompactUnwindModeMask;
      if ((*ActualTarget == Target::AArch64 &&
           Mode == CompactUnwindAArch64Dwarf) ||
          (*ActualTarget == Target::X86_64 &&
           Mode == CompactUnwindX8664Dwarf)) {
        auto ResolvedFDE = ResolveDwarfFDE(*Function, Record.Encoding);
        if (!ResolvedFDE)
          return Unexpected<Diagnostic>(std::move(ResolvedFDE.error()));
        FDE = *ResolvedFDE;
      }
      auto Added = Graph.addCompactUnwind(CompactUnwindRecord{
          *Function, Record.Length, Record.Encoding, *Personality, *LSDA, FDE});
      if (!Added)
        return Unexpected<Diagnostic>(std::move(Added.error()));
    }
  }

  if (!IgnorableUndefinedSymbols.empty()) {
    for (const auto &InputSection : Object.sections()) {
      for (const auto &InputRelocation : InputSection.relocations()) {
        const auto InputSymbol = InputRelocation.getSymbol();
        if (InputSymbol != Object.symbol_end() &&
            IgnorableUndefinedSymbols.count(InputSymbol->getRawDataRefImpl()) !=
                0)
          return fail<LinkGraph>("relocation targets MSVC CRT marker");
      }
    }
  }

  for (const auto &InputSection : Object.sections()) {
    auto RelocatedSection = InputSection.getRelocatedSection();
    if (!RelocatedSection) {
      return fail<LinkGraph>("cannot read relocated section: " +
                             llvm::toString(RelocatedSection.takeError()));
    }
    const uint64_t SectionIndex = *RelocatedSection == Object.section_end()
                                      ? InputSection.getIndex()
                                      : (*RelocatedSection)->getIndex();
    const auto Section = SectionIds.find(SectionIndex);
    if (Section == SectionIds.end()) {
      continue;
    }
    if (Graph.sections()[Section->second].Purpose ==
        SectionPurpose::CompactUnwind)
      continue;
    if (Graph.sections()[Section->second].Purpose == SectionPurpose::XData &&
        InputSection.relocation_begin() != InputSection.relocation_end()) {
      return fail<LinkGraph>("personality relocation in .xdata is unsupported",
                             DiagnosticKind::Unsupported);
    }
    auto InputRelocation = InputSection.relocation_begin();
    const auto RelocationEnd = InputSection.relocation_end();
    while (InputRelocation != RelocationEnd) {
      if (InputRelocation->getType() > UINT32_MAX) {
        return fail<LinkGraph>("relocation type is out of range");
      }
      uint32_t Type = static_cast<uint32_t>(InputRelocation->getType());
      auto Metadata =
          relocationMetadata(Object, *InputRelocation, *ActualTarget);
      if (!Metadata) {
        return fail<LinkGraph>("malformed relocation metadata");
      }
      std::optional<int64_t> PairedAddend;
      if (Object.isMachO() && *ActualTarget == Target::AArch64 &&
          Type == llvm::MachO::ARM64_RELOC_ADDEND) {
        auto Pair = consumeAArch64MachOAddendPair(
            Object, Graph.sections()[Section->second].Content, InputRelocation,
            RelocationEnd);
        if (!Pair)
          return Unexpected<Diagnostic>(std::move(Pair.error()));
        Type = Pair->Type;
        Metadata = Pair->Metadata;
        PairedAddend = Pair->Addend;
      }
      if (Object.isMachO() && *ActualTarget == Target::AArch64 &&
          Graph.sections()[Section->second].Purpose ==
              SectionPurpose::EHFrame) {
        const auto AArch64Type = InputRelocation->getType();
        if (AArch64Type == llvm::MachO::ARM64_RELOC_UNSIGNED)
          return fail<LinkGraph>(
              "AArch64 Mach-O EH frame unsigned relocation lacks subtractor");
        if (AArch64Type == llvm::MachO::ARM64_RELOC_SUBTRACTOR) {
          if (Metadata->PatchSize != 8 || Metadata->PCRelative ||
              !Metadata->External || Metadata->Scattered)
            return fail<LinkGraph>(
                "malformed AArch64 Mach-O EH frame subtractor");
          const uint64_t Offset = InputRelocation->getOffset();
          auto ParsedFields = machOEHFrameFields(
              Graph.sections()[Section->second].Content, *ActualTarget);
          if (!ParsedFields)
            return fail<LinkGraph>("malformed Mach-O EH frame");
          const auto &Fields = *ParsedFields;
          if (Fields.count(static_cast<size_t>(Offset)) == 0)
            return fail<LinkGraph>(
                "AArch64 Mach-O EH frame relocation is not an FDE "
                "initial-location field");
          const auto SubtractorInputSymbol = InputRelocation->getSymbol();
          if (SubtractorInputSymbol == Object.symbol_end())
            return fail<LinkGraph>("AArch64 Mach-O EH frame subtractor has no "
                                   "target symbol");
          const auto Subtractor =
              SymbolIds.find(SubtractorInputSymbol->getRawDataRefImpl());
          if (Subtractor == SymbolIds.end() ||
              Graph.symbols()[Subtractor->second].Section != Section->second)
            return fail<LinkGraph>(
                "unsupported AArch64 Mach-O EH frame subtractor symbol",
                DiagnosticKind::Unsupported);
          ++InputRelocation;
          if (InputRelocation == RelocationEnd ||
              InputRelocation->getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
            return fail<LinkGraph>(
                "AArch64 Mach-O EH frame subtractor is not followed by "
                "unsigned relocation");
          const auto UnsignedMetadata =
              relocationMetadata(Object, *InputRelocation, *ActualTarget);
          if (!UnsignedMetadata || UnsignedMetadata->PatchSize != 8 ||
              UnsignedMetadata->PCRelative || UnsignedMetadata->Scattered)
            return fail<LinkGraph>(
                "malformed AArch64 Mach-O EH frame unsigned relocation");
          if (InputRelocation->getOffset() != Offset)
            return fail<LinkGraph>(
                "AArch64 Mach-O EH frame relocation pair addresses differ");
          const auto UnsignedInputSymbol = InputRelocation->getSymbol();
          const auto &SubtractorSymbol = Graph.symbols()[Subtractor->second];
          if (SubtractorSymbol.Offset > Offset)
            return fail<LinkGraph>(
                "invalid AArch64 Mach-O EH frame subtractor symbol offset");
          uint64_t Raw = 0;
          if (!readELFInteger(Graph.sections()[Section->second].Content, Offset,
                              true, Raw))
            return fail<LinkGraph>(
                "malformed AArch64 Mach-O EH frame relocation addend");
          const uint64_t Delta = Offset - SubtractorSymbol.Offset;
          SymbolId Unsigned = InvalidSymbolId;
          if (UnsignedMetadata->External) {
            if (UnsignedInputSymbol == Object.symbol_end())
              return fail<LinkGraph>("AArch64 Mach-O EH frame unsigned "
                                     "relocation has no target symbol");
            const auto Symbol =
                SymbolIds.find(UnsignedInputSymbol->getRawDataRefImpl());
            if (Symbol == SymbolIds.end() || Raw != UINT64_C(0) - Delta)
              return fail<LinkGraph>(
                  "malformed AArch64 Mach-O EH frame relocation addend");
            Unsigned = Symbol->second;
          } else {
            const auto *MachO =
                llvm::cast<llvm::object::MachOObjectFile>(&Object);
            const auto TargetSection = MachO->getRelocationSection(
                InputRelocation->getRawDataRefImpl());
            const auto GraphTargetSection =
                TargetSection == Object.section_end()
                    ? SectionIds.end()
                    : SectionIds.find(TargetSection->getIndex());
            if (GraphTargetSection == SectionIds.end())
              return fail<LinkGraph>(
                  "AArch64 Mach-O EH frame relocation targets an unsupported "
                  "section",
                  DiagnosticKind::Unsupported);
            auto TargetOffset = Internal::resolveMachOFDEAddress(
                0, 0, Delta, static_cast<int64_t>(Raw));
            if (!TargetOffset)
              return fail<LinkGraph>("DWARF FDE target address overflows");
            const auto Symbol = std::find_if(
                Graph.symbols().begin(), Graph.symbols().end(),
                [&](const auto &Value) {
                  return Value.Section == GraphTargetSection->second &&
                         Value.Offset == *TargetOffset &&
                         Value.Name.rfind("$section", 0) != 0;
                });
            if (Symbol == Graph.symbols().end())
              return fail<LinkGraph>(
                  "AArch64 Mach-O EH frame relocation targets an unsupported "
                  "symbol",
                  DiagnosticKind::Unsupported);
            Unsigned = static_cast<SymbolId>(Symbol - Graph.symbols().begin());
          }
          const auto &UnsignedSymbol = Graph.symbols()[Unsigned];
          if (UnsignedSymbol.Section >= Graph.sections().size() ||
              Graph.sections()[UnsignedSymbol.Section].Kind !=
                  SectionKind::Text)
            return fail<LinkGraph>(
                "AArch64 Mach-O EH frame relocation target is not a function "
                "symbol",
                DiagnosticKind::Unsupported);
          if (auto Added = Graph.addEHFrameReference(
                  EHFrameReference{Section->second, Offset, Unsigned});
              !Added)
            return addContext(std::move(Added.error()),
                              "invalid Mach-O EH frame relocation");
          ++InputRelocation;
          continue;
        }
      }
      const auto InputSymbol = InputRelocation->getSymbol();
      SymbolId TargetSymbol = InvalidSymbolId;
      int64_t Addend = 0;
      bool Implicit = true;
      const auto PatchSize = relocationPatchSize(
          objectFormat(Object), *ActualTarget, Type, Metadata->PatchSize);
      if (InputSymbol == Object.symbol_end()) {
        if (Object.isELF() && PatchSize && *PatchSize == NoPatch) {
          ++InputRelocation;
          continue;
        }
        const auto *MachO =
            llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object);
        if (MachO == nullptr || Metadata->External)
          return fail<LinkGraph>("relocation has no target symbol");
        const auto TargetSection =
            MachO->getRelocationSection(InputRelocation->getRawDataRefImpl());
        const auto GraphTargetSection =
            TargetSection == Object.section_end()
                ? SectionIds.end()
                : SectionIds.find(TargetSection->getIndex());
        if (GraphTargetSection == SectionIds.end())
          return fail<LinkGraph>("relocation targets an unsupported section",
                                 DiagnosticKind::Unsupported);
        if (Object.isMachO() && *ActualTarget == Target::X86_64 &&
            Metadata->PCRelative) {
          uint64_t Suffix = 0;
          switch (Type) {
          case llvm::MachO::X86_64_RELOC_SIGNED:
          case llvm::MachO::X86_64_RELOC_BRANCH:
            break;
          case llvm::MachO::X86_64_RELOC_SIGNED_1:
            Suffix = 1;
            break;
          case llvm::MachO::X86_64_RELOC_SIGNED_2:
            Suffix = 2;
            break;
          case llvm::MachO::X86_64_RELOC_SIGNED_4:
            Suffix = 4;
            break;
          default:
            return fail<LinkGraph>(
                "unsupported PC-relative x86_64 Mach-O section relocation "
                "type",
                DiagnosticKind::Unsupported);
          }
          const uint64_t Offset = InputRelocation->getOffset();
          auto Raw =
              Internal::readSigned(Graph.sections()[Section->second].Content,
                                   Offset, 4, Endianness::Little);
          if (!Raw)
            return fail<LinkGraph>(
                "cannot read x86_64 Mach-O section relocation addend");
          uint64_t Positive = Graph.sections()[Section->second].InputAddress;
          if (Offset > UINT64_MAX - Positive)
            return fail<LinkGraph>(
                "x86_64 Mach-O section relocation offset overflows");
          Positive += Offset;
          if (4 > UINT64_MAX - Positive)
            return fail<LinkGraph>(
                "x86_64 Mach-O section relocation offset overflows");
          Positive += 4;
          if (Suffix > UINT64_MAX - Positive)
            return fail<LinkGraph>(
                "x86_64 Mach-O section relocation offset overflows");
          Positive += Suffix;
          uint64_t Negative = TargetSection->getAddress();
          if (*Raw >= 0) {
            const uint64_t Value = static_cast<uint64_t>(*Raw);
            if (Value > UINT64_MAX - Positive)
              return fail<LinkGraph>(
                  "x86_64 Mach-O section relocation offset overflows");
            Positive += Value;
          } else {
            const uint64_t Magnitude = static_cast<uint64_t>(-(*Raw + 1)) + 1;
            if (Magnitude > UINT64_MAX - Negative)
              return fail<LinkGraph>(
                  "x86_64 Mach-O section relocation offset overflows");
            Negative += Magnitude;
          }
          if (Positive < Negative)
            return fail<LinkGraph>(
                "x86_64 Mach-O section relocation target is negative");
          const uint64_t TargetOffset = Positive - Negative;
          if (TargetOffset > static_cast<uint64_t>(INT64_MAX))
            return fail<LinkGraph>(
                "x86_64 Mach-O section relocation offset overflows");
          if (TargetOffset >=
              Graph.sections()[GraphTargetSection->second].VirtualSize)
            return fail<LinkGraph>(
                "x86_64 Mach-O section relocation target is outside its "
                "section");
          Addend = static_cast<int64_t>(TargetOffset);
          Implicit = false;
        } else if (PatchSize && ((*ActualTarget == Target::X86_64 &&
                                  Type == llvm::MachO::X86_64_RELOC_UNSIGNED) ||
                                 (*ActualTarget == Target::AArch64 &&
                                  Type == llvm::MachO::ARM64_RELOC_UNSIGNED))) {
          const auto Raw = Internal::readUnsigned(
              Graph.sections()[Section->second].Content,
              InputRelocation->getOffset(), *PatchSize,
              Object.isLittleEndian() ? Endianness::Little : Endianness::Big);
          if (!Raw || *Raw < TargetSection->getAddress() ||
              *Raw - TargetSection->getAddress() >
                  static_cast<uint64_t>(INT64_MAX)) {
            return fail<LinkGraph>(
                "invalid Mach-O section relocation target offset");
          }
          const uint64_t TargetOffset = *Raw - TargetSection->getAddress();
          if (TargetOffset >=
              Graph.sections()[GraphTargetSection->second].VirtualSize) {
            return fail<LinkGraph>(
                "Mach-O section relocation target is outside its section");
          }
          Addend = static_cast<int64_t>(TargetOffset);
          Implicit = false;
        } else if (*ActualTarget == Target::AArch64 &&
                   Type == llvm::MachO::ARM64_RELOC_PAGEOFF12) {
          const auto Raw = Internal::readUnsigned(
              Graph.sections()[Section->second].Content,
              InputRelocation->getOffset(), 4, Endianness::Little);
          if (!Raw)
            return fail<LinkGraph>(
                "cannot read AArch64 Mach-O PAGEOFF12 addend");
          const uint32_t Instruction = static_cast<uint32_t>(*Raw);
          unsigned Scale = 0;
          if ((Instruction & UINT32_C(0x3B000000)) == UINT32_C(0x39000000)) {
            Scale = (Instruction >> 30) & 3;
            if (Scale == 0 &&
                (Instruction & UINT32_C(0x04800000)) == UINT32_C(0x04800000))
              Scale = 4;
          }
          Addend = static_cast<int64_t>(
              ((Instruction & UINT32_C(0x003FFC00)) >> 10) << Scale);
          Implicit = false;
        }
        const auto Existing =
            std::find_if(Graph.symbols().begin(), Graph.symbols().end(),
                         [&](const auto &Value) {
                           return Value.Section == GraphTargetSection->second &&
                                  Value.Offset == 0;
                         });
        if (Existing != Graph.symbols().end()) {
          TargetSymbol =
              static_cast<SymbolId>(Existing - Graph.symbols().begin());
        } else {
          auto Added = Graph.addSymbol(
              Symbol{"$section" + std::to_string(TargetSection->getIndex()),
                     GraphTargetSection->second, 0, 0, false});
          if (!Added)
            return Unexpected<Diagnostic>(std::move(Added.error()));
          TargetSymbol = *Added;
        }
      } else {
        const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
        if (Symbol == SymbolIds.end())
          return fail<LinkGraph>("relocation targets an unsupported symbol",
                                 DiagnosticKind::Unsupported);
        TargetSymbol = Symbol->second;
      }
      if (PairedAddend) {
        Addend = *PairedAddend;
        Implicit = false;
      } else if (Implicit) {
        std::tie(Addend, Implicit) = relocationAddend(Object, *InputRelocation);
      }
      if (!PatchSize) {
        Diagnostic Diag{"unsupported relocation patch size"};
        Diag.Section = Section->second;
        Diag.Symbol = TargetSymbol;
        Diag.RelocationType = Type;
        Diag.Offset = InputRelocation->getOffset();
        Diag.SectionName = Graph.sections()[Section->second].Name;
        Diag.SymbolName = Graph.symbols()[TargetSymbol].Name;
        Diag.Kind = DiagnosticKind::Unsupported;
        return Unexpected<Diagnostic>(std::move(Diag));
      }
      auto Added = Graph.addRelocation(Relocation{
          Section->second, InputRelocation->getOffset(), Type, TargetSymbol,
          Addend, Implicit, objectFormat(Object), *PatchSize,
          Metadata->PCRelative, Metadata->External, Metadata->Scattered});
      if (!Added) {
        return Unexpected<Diagnostic>(std::move(Added.error()));
      }
      ++InputRelocation;
    }
  }
  if (auto Added = associateRawX86_64MachOFDEs(Graph); !Added)
    return Unexpected<Diagnostic>(std::move(Added.error()));
  if (auto Valid = Graph.validate(); !Valid) {
    return Unexpected<Diagnostic>(std::move(Valid.error()));
  }
  if (!Graph.compactUnwind().empty() && !validateCompactUnwind(Graph))
    return fail<LinkGraph>("unsupported compact unwind encoding",
                           DiagnosticKind::Unsupported);
  return Graph;
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
