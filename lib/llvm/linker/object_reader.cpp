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

#include "linker/byte_io.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/relocation.h"

#include <llvm/ADT/STLExtras.h>
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
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

// Object decoding follows the ELF gABI plus target psABIs, the Mach-O ABI,
// and the PE/COFF specification; LLVM's object readers provide the structures.
constexpr uint64_t CompactUnwindRecordSize = 32;
constexpr uint64_t CompactUnwindFunctionOffset = 0;
constexpr uint64_t CompactUnwindLengthOffset = 8;
constexpr uint64_t CompactUnwindEncodingOffset = 12;
constexpr uint64_t CompactUnwindPersonalityOffset = 16;
constexpr uint64_t CompactUnwindLSDAOffset = 24;
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

Unexpected<Diagnostic> addContext(Diagnostic Value, std::string_view Context) {
  Value.Message = std::string(Context) + ": " + Value.Message;
  return diagnosticError(std::move(Value));
}

template <typename T>
LinkExpect<T> takeOr(llvm::Expected<T> Value, std::string_view Context,
                     DiagnosticKind Kind = DiagnosticKind::Malformed) {
  if (!Value)
    return diagnosticError(
        std::string(Context) + ": " + llvm::toString(Value.takeError()), Kind);
  return std::move(*Value);
}

Unexpected<Diagnostic> undefinedSymbol(std::string Name) {
  Diagnostic Diag{"undefined symbol"};
  Diag.SymbolName = std::move(Name);
  return diagnosticError(std::move(Diag));
}

constexpr llvm::StringLiteral SectionSymbolPrefix = "$section";

bool startsWith(llvm::StringRef Value, llvm::StringRef Prefix) noexcept {
#if LLVM_VERSION_MAJOR >= 16
  return Value.starts_with(Prefix);
#else
  return Value.startswith(Prefix);
#endif
}

bool isSectionSymbol(llvm::StringRef Name) noexcept {
  return startsWith(Name, SectionSymbolPrefix);
}

template <typename T>
bool readInteger(Span<const Byte> Buffer, uint64_t Offset, bool LittleEndian,
                 T &Value) noexcept {
  static_assert(std::is_unsigned_v<T>);
  const auto Raw = Internal::readUnsigned(
      Buffer, Offset, static_cast<uint8_t>(sizeof(T)),
      LittleEndian ? Endianness::Little : Endianness::Big);
  if (!Raw)
    return false;
  Value = static_cast<T>(*Raw);
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
  if (!readInteger(Span<const Byte>(Bytes.data(), Bytes.size()), Offset,
                   LittleEndian, Value))
    return false;
  Offset += sizeof(Value);
  return true;
}

bool skipARMAttributeString(llvm::ArrayRef<uint8_t> Bytes, size_t &Offset) {
  while (Offset < Bytes.size()) {
    if (Bytes[Offset++] == 0)
      return true;
  }
  return false;
}

Unexpected<Diagnostic> malformedARMAttributes() {
  return diagnosticError("malformed ARM attributes");
}

struct AEABISubsection {
  size_t VendorStart;
  size_t Start;
  size_t Body;
  size_t End;
  uint64_t Tag;
  uint32_t Length;
};

template <typename VendorFn, typename SubsectionFn>
LinkExpect<void> forEachAEABISubsection(llvm::ArrayRef<uint8_t> Bytes,
                                        bool LittleEndian, VendorFn OnVendor,
                                        SubsectionFn OnSubsection) {
  constexpr uint32_t MinimumVendorLength = 4;
  constexpr uint32_t SubsectionHeaderSize = 5;
  if (Bytes.empty() || Bytes[0] != 'A')
    return malformedARMAttributes();
  size_t Offset = 1;
  while (Offset < Bytes.size()) {
    const size_t VendorStart = Offset;
    uint32_t VendorLength = 0;
    if (!readARMAttributeWord(Bytes, Offset, LittleEndian, VendorLength) ||
        VendorLength < MinimumVendorLength || VendorStart > Bytes.size() ||
        VendorLength > Bytes.size() - VendorStart)
      return malformedARMAttributes();
    const size_t VendorEnd = VendorStart + VendorLength;
    const auto Vendor = Bytes.slice(0, VendorEnd);
    const size_t VendorName = Offset;
    if (!skipARMAttributeString(Vendor, Offset))
      return malformedARMAttributes();
    const bool AEABI = Offset - VendorName == 6 &&
                       Bytes.slice(VendorName, 5) ==
                           llvm::ArrayRef<uint8_t>{'a', 'e', 'a', 'b', 'i'};
    if (!AEABI) {
      Offset = VendorEnd;
      continue;
    }
    OnVendor(VendorStart, VendorEnd);
    while (Offset < VendorEnd) {
      AEABISubsection Subsection{VendorStart, Offset, 0, 0, 0, 0};
      if (!readARMAttributeULEB(Vendor, Offset, Subsection.Tag) ||
          !readARMAttributeWord(Vendor, Offset, LittleEndian,
                                Subsection.Length) ||
          Offset - Subsection.Start != SubsectionHeaderSize ||
          Subsection.Length < SubsectionHeaderSize ||
          Subsection.Length > VendorEnd - Subsection.Start)
        return malformedARMAttributes();
      Subsection.Body = Offset;
      Subsection.End = Subsection.Start + Subsection.Length;
      EXPECTED_TRY(OnSubsection(Bytes.slice(0, Subsection.End), Subsection));
      Offset = Subsection.End;
    }
  }
  return {};
}

bool isScopedARMAttributeSubsection(uint64_t Tag) noexcept {
  return Tag == llvm::ARMBuildAttrs::Section ||
         Tag == llvm::ARMBuildAttrs::Symbol;
}

bool skipARMAttributeIndices(llvm::ArrayRef<uint8_t> Bytes, size_t &Offset) {
  uint64_t Index = 0;
  do {
    if (!readARMAttributeULEB(Bytes, Offset, Index))
      return false;
  } while (Index != 0);
  return true;
}

LinkExpect<std::vector<uint64_t>> armVFPArgs(llvm::ArrayRef<uint8_t> Bytes,
                                             bool LittleEndian) {
  std::vector<uint64_t> Result;
  EXPECTED_TRY(forEachAEABISubsection(
      Bytes, LittleEndian, [](size_t, size_t) {},
      [&](llvm::ArrayRef<uint8_t> Subsection,
          const AEABISubsection &Header) -> LinkExpect<void> {
        size_t Offset = Header.Body;
        if (isScopedARMAttributeSubsection(Header.Tag)) {
          if (!skipARMAttributeIndices(Subsection, Offset))
            return malformedARMAttributes();
        } else if (Header.Tag != llvm::ARMBuildAttrs::File) {
          return malformedARMAttributes();
        }
        while (Offset < Header.End) {
          uint64_t Tag = 0;
          uint64_t Value = 0;
          if (!readARMAttributeULEB(Subsection, Offset, Tag))
            return malformedARMAttributes();
          const bool StringValue =
              Tag == llvm::ARMBuildAttrs::CPU_raw_name ||
              Tag == llvm::ARMBuildAttrs::CPU_name ||
              Tag == llvm::ARMBuildAttrs::also_compatible_with ||
              Tag == llvm::ARMBuildAttrs::conformance ||
              (Tag >= 32 && (Tag & 1) != 0);
          if (StringValue) {
            if (!skipARMAttributeString(Subsection, Offset))
              return malformedARMAttributes();
          } else {
            if (!readARMAttributeULEB(Subsection, Offset, Value))
              return malformedARMAttributes();
            if (Tag == llvm::ARMBuildAttrs::ABI_VFP_args)
              Result.push_back(Value);
            if (Tag == llvm::ARMBuildAttrs::compatibility &&
                !skipARMAttributeString(Subsection, Offset))
              return malformedARMAttributes();
          }
        }
        return {};
      }));
  return Result;
}

LinkExpect<std::vector<uint8_t>>
armAttributesForLLVM(llvm::ArrayRef<uint8_t> Bytes, bool LittleEndian) {
  std::vector<uint8_t> Result;
  Result.reserve(Bytes.size());
  if (!Bytes.empty())
    Result.push_back(Bytes.front());
  size_t OutputVendorStart = 0;
  EXPECTED_TRY(forEachAEABISubsection(
      Bytes, LittleEndian,
      [&](size_t VendorStart, size_t VendorEnd) {
        OutputVendorStart = Result.size();
        Result.insert(Result.end(), Bytes.begin() + VendorStart,
                      Bytes.begin() + VendorEnd);
      },
      [&](llvm::ArrayRef<uint8_t> Subsection,
          const AEABISubsection &Header) -> LinkExpect<void> {
        if (!isScopedARMAttributeSubsection(Header.Tag))
          return {};
        size_t Offset = Header.Body;
        if (!skipARMAttributeIndices(Subsection, Offset))
          return malformedARMAttributes();
        const uint32_t ParserLength =
            Header.Length - static_cast<uint32_t>(Offset - Header.Body);
        const size_t LengthOffset =
            OutputVendorStart + (Header.Start - Header.VendorStart) + 1;
        for (unsigned I = 0; I < 4; ++I) {
          const unsigned Shift = LittleEndian ? I * 8 : (3 - I) * 8;
          Result[LengthOffset + I] =
              static_cast<uint8_t>(ParserLength >> Shift);
        }
        return {};
      }));
  return Result;
}

LinkExpect<uint32_t> armELFFlags(const llvm::object::ObjectFile &Object,
                                 uint32_t InputFlags) {
  constexpr uint32_t Supported = llvm::ELF::EF_ARM_EABIMASK |
                                 llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                 llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  if ((InputFlags & ~Supported) != 0)
    return diagnosticError("unsupported ARM ELF header flags",
                           DiagnosticKind::Unsupported);
  enum class FloatABI { Unspecified, Soft, Hard, ToolChain };
  FloatABI ABI = FloatABI::Unspecified;
  bool HasConstraint = false;
  for (const auto &Section : Object.sections()) {
    const llvm::object::ELFSectionRef ELFSection(Section);
    if (ELFSection.getType() != llvm::ELF::SHT_ARM_ATTRIBUTES)
      continue;
    EXPECTED_TRY(const llvm::StringRef Contents,
                 takeOr(Section.getContents(), "cannot read ARM attributes"));
    const auto Bytes = llvm::ArrayRef<uint8_t>(
        reinterpret_cast<const uint8_t *>(Contents.data()), Contents.size());
    EXPECTED_TRY(const auto ParserBytes,
                 armAttributesForLLVM(Bytes, Object.isLittleEndian()));
    llvm::ARMAttributeParser Parser;
#if LLVM_VERSION_MAJOR >= 18
    const auto Endian = Object.isLittleEndian() ? llvm::endianness::little
                                                : llvm::endianness::big;
#else
    const auto Endian =
        Object.isLittleEndian() ? llvm::support::little : llvm::support::big;
#endif
    if (auto Parsed = Parser.parse(ParserBytes, Endian))
      return diagnosticError("malformed ARM attributes: " +
                             llvm::toString(std::move(Parsed)));
    EXPECTED_TRY(const auto Values, armVFPArgs(Bytes, Object.isLittleEndian()));
    for (const uint64_t Value : Values) {
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
        return diagnosticError("unknown ARM Tag_ABI_VFP_args value");
      }
      if (HasConstraint && ABI != SectionABI)
        return diagnosticError("conflicting ARM Tag_ABI_VFP_args values");
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

LinkExpect<uint32_t> symbolFlags(const llvm::object::SymbolRef &Symbol) {
#if LLVM_VERSION_MAJOR >= 11
  return takeOr(Symbol.getFlags(), "cannot read symbol flags");
#else
  return Symbol.getFlags();
#endif
}

struct ELFHeader {
  bool Is64;
  bool LittleEndian;
  uint64_t SectionTable;
  uint16_t SectionEntrySize;
  uint16_t SectionCount;
};

bool hasELFMagic(Span<const Byte> Buffer) noexcept {
  return Buffer.size() >= value(ELFIdentification::Size) &&
         Buffer[value(ELFIdentification::Magic0)] == llvm::ELF::ElfMagic[0] &&
         Buffer[value(ELFIdentification::Magic1)] == llvm::ELF::ElfMagic[1] &&
         Buffer[value(ELFIdentification::Magic2)] == llvm::ELF::ElfMagic[2] &&
         Buffer[value(ELFIdentification::Magic3)] == llvm::ELF::ElfMagic[3];
}

std::optional<ELFHeader> readELFHeader(Span<const Byte> Buffer) noexcept {
  if (Buffer.size() < value(ELFIdentification::Size))
    return std::nullopt;
  ELFHeader Header{};
  Header.Is64 =
      Buffer[value(ELFIdentification::Class)] == llvm::ELF::ELFCLASS64;
  Header.LittleEndian =
      Buffer[value(ELFIdentification::Data)] == llvm::ELF::ELFDATA2LSB;
  if (Header.Is64) {
    if (!readInteger(Buffer, value(ELF64Offset::SectionTable),
                     Header.LittleEndian, Header.SectionTable))
      return std::nullopt;
  } else {
    uint32_t SectionTable32 = 0;
    if (!readInteger(Buffer, value(ELF32Offset::SectionTable),
                     Header.LittleEndian, SectionTable32))
      return std::nullopt;
    Header.SectionTable = SectionTable32;
  }
  if (!readInteger(Buffer,
                   Header.Is64 ? value(ELF64Offset::SectionEntrySize)
                               : value(ELF32Offset::SectionEntrySize),
                   Header.LittleEndian, Header.SectionEntrySize) ||
      !readInteger(Buffer,
                   Header.Is64 ? value(ELF64Offset::SectionCount)
                               : value(ELF32Offset::SectionCount),
                   Header.LittleEndian, Header.SectionCount))
    return std::nullopt;
  return Header;
}

bool hasTrailingELFObject(Span<const Byte> Buffer,
                          const llvm::object::ObjectFile &Object) noexcept {
  if (!Object.isELF()) {
    return false;
  }
  const auto Header = readELFHeader(Buffer);
  if (!Header || (Header->SectionEntrySize != 0 &&
                  Header->SectionCount > (UINT64_MAX - Header->SectionTable) /
                                             Header->SectionEntrySize)) {
    return false;
  }
  uint64_t End =
      Header->SectionTable + static_cast<uint64_t>(Header->SectionEntrySize) *
                                 static_cast<uint64_t>(Header->SectionCount);
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
  constexpr uint16_t MachineAlpha = 0x0183;
  constexpr uint16_t MachineAlpha64 = 0x0184;
  constexpr uint16_t MachineM68K = 0x0150;
  constexpr uint16_t MachinePARISC = 0x0290;
  constexpr uint16_t MachineM68KWindows = 0x0268;
  constexpr uint16_t MachineARM64EC = 0xA641;
  constexpr uint16_t MachineARM64X = 0xA64E;
  uint16_t Machine = 0;
  if (!readInteger(Buffer, Offset, true, Machine))
    return false;
  switch (Machine) {
  case llvm::COFF::IMAGE_FILE_MACHINE_UNKNOWN:
  case llvm::COFF::IMAGE_FILE_MACHINE_POWERPC:
  case MachineAlpha:
  case MachineAlpha64:
  case llvm::COFF::IMAGE_FILE_MACHINE_R4000:
  case MachineM68K:
  case llvm::COFF::IMAGE_FILE_MACHINE_I386:
  case llvm::COFF::IMAGE_FILE_MACHINE_ARMNT:
  case MachinePARISC:
  case MachineM68KWindows:
  case llvm::COFF::IMAGE_FILE_MACHINE_AMD64:
  case llvm::COFF::IMAGE_FILE_MACHINE_ARM64:
  case MachineARM64EC:
  case MachineARM64X:
    return true;
  default:
    return false;
  }
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
  constexpr uint64_t MachineOffset = 0;
  constexpr uint64_t SectionCountOffset = 2;
  constexpr uint64_t SymbolTableOffset = 8;
  constexpr uint64_t SymbolCountOffset = 12;
  constexpr uint64_t OptionalHeaderSizeOffset = 16;
  constexpr uint64_t BigObjSignatureOffset = 2;
  constexpr uint64_t BigObjVersionOffset = 4;
  constexpr uint64_t BigObjSectionCountOffset = 44;
  constexpr uint64_t BigObjSymbolTableOffset = 48;
  constexpr uint64_t BigObjSymbolCountOffset = 52;
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
    return readInteger(Buffer, Offset, true, Value);
  };
  auto Read32 = [&](uint64_t Offset, uint32_t &Value) noexcept {
    return readInteger(Buffer, Offset, true, Value);
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
  if (!Read16(MachineOffset, Signature1) ||
      !Read16(BigObjSignatureOffset, Signature2) ||
      !Read16(BigObjVersionOffset, BigObjVersion)) {
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
    if (!Read32(BigObjSectionCountOffset, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SectionCount = Value;
    if (!Read32(BigObjSymbolTableOffset, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SymbolTable = Value;
    if (!Read32(BigObjSymbolCountOffset, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SymbolCount = Value;
    HeaderSize = BigObjHeaderSize;
    InputSymbolSize = BigObjSymbolSize;
  } else {
    uint16_t Sections = 0;
    uint16_t OptionalHeaderSize = 0;
    uint32_t Value = 0;
    if (!Read16(SectionCountOffset, Sections) ||
        !Read32(SymbolTableOffset, Value)) {
      return COFFTrailingObject::Malformed;
    }
    SectionCount = Sections;
    SymbolTable = Value;
    if (!Read32(SymbolCountOffset, Value) ||
        !Read16(OptionalHeaderSizeOffset, OptionalHeaderSize)) {
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
  if (!hasELFMagic(Buffer)) {
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
  const auto Header = readELFHeader(Buffer);
  if (!Header) {
    return false;
  }
  const uint64_t SectionOffset = Header->SectionTable;
  const uint16_t SectionEntrySize = Header->SectionEntrySize;
  uint64_t SectionCount = Header->SectionCount;
  const uint64_t RequiredSectionSize =
      Is64 ? ELF64SectionEntrySize : ELF32SectionEntrySize;
  if (SectionEntrySize < RequiredSectionSize || SectionOffset > Buffer.size() ||
      SectionEntrySize > Buffer.size() - SectionOffset) {
    return false;
  }
  if (SectionCount == 0) {
    if (Is64) {
      if (!readInteger(Buffer, SectionOffset + value(ELF64Offset::SectionSize),
                       LittleEndian, SectionCount)) {
        return false;
      }
    } else {
      uint32_t ExtendedCount = 0;
      if (!readInteger(Buffer, SectionOffset + value(ELF32Offset::SectionSize),
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
    if (!readInteger(Buffer,
                     Base + (Is64 ? value(ELF64Offset::SectionType)
                                  : value(ELF32Offset::SectionType)),
                     LittleEndian, Type) ||
        !readInteger(Buffer,
                     Base + (Is64 ? value(ELF64Offset::SectionLink)
                                  : value(ELF32Offset::SectionLink)),
                     LittleEndian, Link)) {
      return false;
    }
    if (Is64) {
      return readInteger(Buffer, Base + value(ELF64Offset::SectionOffset),
                         LittleEndian, Offset) &&
             readInteger(Buffer, Base + value(ELF64Offset::SectionSize),
                         LittleEndian, Size) &&
             readInteger(Buffer, Base + value(ELF64Offset::SectionEntry),
                         LittleEndian, EntrySize);
    }
    uint32_t Offset32 = 0;
    uint32_t Size32 = 0;
    uint32_t EntrySize32 = 0;
    if (!readInteger(Buffer, Base + value(ELF32Offset::SectionOffset),
                     LittleEndian, Offset32) ||
        !readInteger(Buffer, Base + value(ELF32Offset::SectionSize),
                     LittleEndian, Size32) ||
        !readInteger(Buffer, Base + value(ELF32Offset::SectionEntry),
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
    if (Type == llvm::ELF::SHT_SYMTAB || Type == llvm::ELF::SHT_DYNSYM) {
      const uint64_t ExpectedSymbolEntrySize =
          Is64 ? ELF64SymbolEntrySize : ELF32SymbolEntrySize;
      if (EntrySize != ExpectedSymbolEntrySize ||
          Size % ExpectedSymbolEntrySize != 0 || Offset > Buffer.size() ||
          Size > Buffer.size() - Offset) {
        return false;
      }
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
      const auto OnHeader = [&](uint64_t Count, bool) {
        DeclaredCount = Count;
      };
      const auto OnRelocation = [&](const auto &Relocation) {
        ++DecodedCount;
        ValidSymbols &= Relocation.r_symidx < SymbolCount;
      };
      auto Error =
          Is64 ? llvm::object::decodeCrel<true>(Content, OnHeader, OnRelocation)
               : llvm::object::decodeCrel<false>(Content, OnHeader,
                                                 OnRelocation);
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
        if (!readInteger(Buffer, InfoOffset, LittleEndian, Info) ||
            (Info >> ELF64SymbolIndexShift) >= SymbolCount) {
          return false;
        }
      } else {
        uint32_t Info32 = 0;
        if (!readInteger(Buffer, InfoOffset, LittleEndian, Info32) ||
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
  if (llvm::isa<llvm::object::ELFObjectFileBase>(&Object)) {
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
  if (Name == ".ARM.exidx" || startsWith(Name, ".ARM.exidx."))
    return SectionPurpose::ARMExidx;
  if (Name.contains("eh_frame"))
    return SectionPurpose::EHFrame;
  if (Object.isCOFF() && startsWith(Name, ".pdata"))
    return SectionPurpose::PData;
  if (Object.isCOFF() && startsWith(Name, ".xdata"))
    return SectionPurpose::XData;
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
      return diagnosticError("x86_64 Mach-O FDE symbol address overflows");
    auto &Address = SymbolsByAddress[TargetSection.InputAddress + Value.Offset];
    if (TargetSection.Kind != SectionKind::Text) {
      Address.HasNonText = true;
      continue;
    }
    Address.HasText = true;
    if (!Address.Text && !isSectionSymbol(Value.Name))
      Address.Text = Symbol;
  }

  for (SectionId Section = 0; Section < Graph.sections().size(); ++Section) {
    const auto &EHFrame = Graph.sections()[Section];
    if (EHFrame.Purpose != SectionPurpose::EHFrame)
      continue;
    auto Fields = machOEHFrameFields(EHFrame.Content, Target::X86_64);
    if (!Fields)
      return diagnosticError("malformed x86_64 Mach-O EH frame");
    for (const auto Field : *Fields) {
      if (RelocatedFields.count({Section, Field}) != 0)
        continue;
      auto Delta = Internal::readSigned(EHFrame.Content, Field, DoubleWordPatch,
                                        Endianness::Little);
      if (!Delta)
        return diagnosticError(
            "cannot read x86_64 Mach-O FDE initial location");
      auto Address = Internal::resolveMachOFDEAddress(0, EHFrame.InputAddress,
                                                      Field, *Delta);
      if (!Address)
        return diagnosticError("x86_64 Mach-O FDE target address overflows");

      const auto Symbols = SymbolsByAddress.find(*Address);
      if (Symbols == SymbolsByAddress.end() || !Symbols->second.Text) {
        if (Symbols != SymbolsByAddress.end() && Symbols->second.HasNonText &&
            !Symbols->second.HasText)
          return diagnosticError(
              "x86_64 Mach-O FDE target is not a text symbol");
        return diagnosticError("unresolved x86_64 Mach-O FDE target");
      }
      EXPECTED_TRY(Graph.addEHFrameReference(EHFrameReference{
          Section, static_cast<uint64_t>(Field), *Symbols->second.Text}));
    }
  }
  return {};
}

std::optional<RelocationMetadata>
relocationMetadata(const llvm::object::ObjectFile &Object,
                   const llvm::object::RelocationRef &Relocation,
                   Target TargetValue) noexcept {
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
  if (!AddendMetadata || AddendMetadata->PatchSize != WordPatch ||
      AddendMetadata->PCRelative || AddendMetadata->External ||
      AddendMetadata->Scattered)
    return diagnosticError("malformed addend relocation metadata");

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
    return diagnosticError("AArch64 Mach-O addend relocation lacks successor");
  if (Relocation->getType() > UINT32_MAX)
    return diagnosticError("relocation type is out of range");
  const uint32_t Type = static_cast<uint32_t>(Relocation->getType());
  if (Type != llvm::MachO::ARM64_RELOC_BRANCH26 &&
      Type != llvm::MachO::ARM64_RELOC_PAGE21 &&
      Type != llvm::MachO::ARM64_RELOC_PAGEOFF12)
    return diagnosticError(
        "unsupported AArch64 Mach-O addend relocation successor",
        DiagnosticKind::Unsupported);
  const auto Metadata =
      relocationMetadata(Object, *Relocation, Target::AArch64);
  const bool ExpectedPCRelative = Type != llvm::MachO::ARM64_RELOC_PAGEOFF12;
  if (!Metadata || Metadata->PatchSize != WordPatch || Metadata->Scattered ||
      !Metadata->External || Metadata->PCRelative != ExpectedPCRelative)
    return diagnosticError("malformed addend relocation successor");
  if (Relocation->getOffset() != Offset)
    return diagnosticError("AArch64 Mach-O addend relocation addresses differ");
  const auto Embedded =
      Internal::readUnsigned(Content, Offset, WordPatch, Endianness::Little);
  if (!Embedded)
    return diagnosticError("cannot read AArch64 Mach-O addend field");
  const uint32_t Instruction = static_cast<uint32_t>(*Embedded);
  const bool HasEmbeddedAddend =
      Type == llvm::MachO::ARM64_RELOC_BRANCH26
          ? (Instruction & UINT32_C(0x03FFFFFF)) != 0
      : Type == llvm::MachO::ARM64_RELOC_PAGE21
          ? (Instruction & UINT32_C(0x60FFFFE0)) != 0
          : (Instruction & UINT32_C(0x003FFC00)) != 0;
  if (HasEmbeddedAddend)
    return diagnosticError(
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
      !readInteger(Command, CommandOffset, LittleEndian, Type) ||
      !readInteger(Command, CommandSizeOffset, LittleEndian, CommandSize) ||
      !readInteger(Command, PlatformOffset, LittleEndian, Platform) ||
      !readInteger(Command, MinimumOSOffset, LittleEndian, MinimumOS) ||
      !readInteger(Command, SDKOffset, LittleEndian, SDK) ||
      !readInteger(Command, ToolCountOffset, LittleEndian, ToolCount) ||
      Type != llvm::MachO::LC_BUILD_VERSION || CommandSize != Command.size() ||
      ToolCount > (Command.size() - HeaderSize) / ToolSize ||
      HeaderSize + static_cast<uint64_t>(ToolCount) * ToolSize !=
          Command.size())
    return diagnosticError("malformed Mach-O build version command");

  MachOBuildVersion Result{Platform, MinimumOS, SDK, {}};
  Result.Tools.reserve(ToolCount);
  for (uint32_t I = 0; I < ToolCount; ++I) {
    const uint64_t Offset = HeaderSize + static_cast<uint64_t>(I) * ToolSize;
    uint32_t Tool = 0;
    uint32_t Version = 0;
    if (!readInteger(Command, Offset, LittleEndian, Tool) ||
        !readInteger(Command, Offset + 4, LittleEndian, Version))
      return diagnosticError("malformed Mach-O build version command");
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
      return diagnosticError(
          "compact unwind local target precedes its section");
    return RawAddend - SectionAddress;
  }
  if (RawAddend <= static_cast<uint64_t>(INT64_MAX)) {
    if (RawAddend > UINT64_MAX - SymbolOffset)
      return diagnosticError("compact unwind target address overflows");
    return SymbolOffset + RawAddend;
  }
  const uint64_t Magnitude = UINT64_MAX - RawAddend + 1;
  if (Magnitude > SymbolOffset)
    return diagnosticError("compact unwind target address underflows");
  return SymbolOffset - Magnitude;
}

LinkExpect<std::vector<DecodedCompactUnwindRecord>>
parseCompactUnwindSection(Target TargetValue, Span<const Byte> Content,
                          Span<const CompactUnwindRelocation> Relocations) {
  if (Content.size() % CompactUnwindRecordSize != 0)
    return diagnosticError("compact unwind size is not a multiple of 32");
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
      return diagnosticError("unsupported compact unwind relocation field",
                             DiagnosticKind::Unsupported);
    if (!Fields.emplace(Relocation.Offset).second)
      return diagnosticError("duplicate compact unwind relocation field");
    if (Relocation.Type != ExpectedType ||
        Relocation.PatchSize != DoubleWordPatch || Relocation.PCRelative ||
        Relocation.Scattered)
      return diagnosticError("unsupported compact unwind relocation",
                             DiagnosticKind::Unsupported);
  }

  std::vector<DecodedCompactUnwindRecord> Records;
  Records.reserve(Content.size() / CompactUnwindRecordSize);
  for (uint64_t Offset = 0; Offset < Content.size();
       Offset += CompactUnwindRecordSize) {
    if (Fields.count(Offset + CompactUnwindFunctionOffset) == 0)
      return diagnosticError("compact unwind record lacks function relocation");
    DecodedCompactUnwindRecord Record{};
    if (!readInteger(Content, Offset + CompactUnwindFunctionOffset, true,
                     Record.Function) ||
        !readInteger(Content, Offset + CompactUnwindLengthOffset, true,
                     Record.Length) ||
        !readInteger(Content, Offset + CompactUnwindEncodingOffset, true,
                     Record.Encoding) ||
        !readInteger(Content, Offset + CompactUnwindPersonalityOffset, true,
                     Record.Personality) ||
        !readInteger(Content, Offset + CompactUnwindLSDAOffset, true,
                     Record.LSDA))
      return diagnosticError("cannot read compact unwind record");
    if ((Record.Personality != 0 &&
         Fields.count(Offset + CompactUnwindPersonalityOffset) == 0) ||
        (Record.LSDA != 0 &&
         Fields.count(Offset + CompactUnwindLSDAOffset) == 0))
      return diagnosticError("compact unwind target lacks relocation");
    Records.push_back(Record);
  }
  return Records;
}

} // namespace Internal

namespace {

using SymbolPreference = bool (*)(const Symbol &Candidate,
                                  const Symbol &Current) noexcept;

bool preferVisibleSymbol(const Symbol &Candidate,
                         const Symbol &Current) noexcept {
  return std::tie(Candidate.Exported, Candidate.Global, Candidate.Size) >
         std::tie(Current.Exported, Current.Global, Current.Size);
}

enum class SectionSymbols : uint8_t { Include, Exclude };

std::optional<SymbolId> findSymbolAt(const LinkGraph &Graph, SectionId Section,
                                     uint64_t Offset, SectionSymbols Policy,
                                     SymbolPreference Prefer = nullptr) {
  const auto &Symbols = Graph.symbols();
  std::optional<SymbolId> Best;
  for (SymbolId Id = 0; Id < Symbols.size(); ++Id) {
    const auto &Value = Symbols[Id];
    if (Value.Section != Section || Value.Offset != Offset ||
        (Policy == SectionSymbols::Exclude && isSectionSymbol(Value.Name)))
      continue;
    if (!Prefer)
      return Id;
    if (!Best || Prefer(Value, Symbols[*Best]))
      Best = Id;
  }
  return Best;
}

LinkExpect<SymbolId>
findOrAddSymbolAt(LinkGraph &Graph, SectionId Section, uint64_t Offset,
                  SectionSymbols Policy, SymbolPreference Prefer,
                  llvm::function_ref<std::string()> SyntheticName) {
  if (const auto Existing =
          findSymbolAt(Graph, Section, Offset, Policy, Prefer))
    return *Existing;
  return Graph.addSymbol(Symbol{SyntheticName(), Section, Offset, 0, false});
}

LinkExpect<std::unique_ptr<llvm::object::ObjectFile>>
openObject(Span<const Byte> Buffer) {
  if (Buffer.empty()) {
    return diagnosticError("empty object buffer");
  }
  if (!validELFRelocations(Buffer)) {
    return diagnosticError("malformed ELF relocation metadata");
  }
  const auto Data = llvm::StringRef(
      reinterpret_cast<const char *>(Buffer.data()), Buffer.size());
  auto ObjectResult = llvm::object::ObjectFile::createObjectFile(
      llvm::MemoryBufferRef(Data, "object"));
  if (!ObjectResult) {
    return diagnosticError("object file parse error: " +
                           llvm::toString(ObjectResult.takeError()));
  }
  return std::move(*ObjectResult);
}

} // namespace

struct ObjectReader::ReadContext {
  using DataRef = llvm::object::DataRefImpl;

  ReadContext(const llvm::object::ObjectFile &Object, Target ActualTarget,
              ObjectReaderInputPolicy InputPolicy)
      : Object(Object), ActualTarget(ActualTarget), InputPolicy(InputPolicy),
        Graph(ActualTarget,
              Object.isLittleEndian() ? Endianness::Little : Endianness::Big,
              objectFormat(Object)) {}

  static LinkExpect<Target>
  validateInput(Span<const Byte> Buffer, const llvm::object::ObjectFile &Object,
                Target ExpectedTarget);
  LinkExpect<void> beginInput();
  LinkExpect<void> readMachOBuildVersions();
  LinkExpect<void> readELFFlags();
  LinkExpect<void> collectMachOFunctionSections();
  LinkExpect<void> readSections();
  LinkExpect<void> linkARMExidx();
  LinkExpect<void> readSymbols();
  LinkExpect<void> readCompactUnwind();
  LinkExpect<void> checkUndefinedMarkers();
  LinkExpect<void> readRelocations();
  LinkExpect<LinkGraph> finish();

  LinkExpect<SymbolId>
  resolveCompactUnwindTarget(const llvm::object::RelocationRef &Relocation,
                             uint64_t Addend);
  LinkExpect<SymbolId> resolveDwarfFDE(SymbolId Function, uint32_t Encoding);
  LinkExpect<void>
  readAArch64MachOEHFramePair(SectionId Section,
                              const RelocationMetadata &Metadata,
                              llvm::object::relocation_iterator &Relocation,
                              const llvm::object::relocation_iterator &End);
  LinkExpect<std::optional<int64_t>> resolveMachOSectionAddend(
      const Section &Input, const llvm::object::RelocationRef &Relocation,
      llvm::object::section_iterator TargetSection, const Section &GraphTarget,
      uint32_t Type, const RelocationMetadata &Metadata,
      std::optional<uint8_t> PatchSize);

  llvm::object::section_iterator
  machORelocationSection(const llvm::object::RelocationRef &Relocation) const {
    return llvm::cast<llvm::object::MachOObjectFile>(&Object)
        ->getRelocationSection(Relocation.getRawDataRefImpl());
  }

  std::optional<SectionId>
  graphSectionOf(llvm::object::section_iterator Section) const {
    if (Section == Object.section_end())
      return std::nullopt;
    const auto Found = SectionIds.find(Section->getIndex());
    if (Found == SectionIds.end())
      return std::nullopt;
    return Found->second;
  }

  const llvm::object::ObjectFile &Object;
  const Target ActualTarget;
  const ObjectReaderInputPolicy InputPolicy;
  LinkGraph Graph;
  std::map<uint64_t, SectionId> SectionIds;
  std::map<std::string, std::string> COFFExports;
  std::optional<uint64_t> CompactUnwindSection;
  std::set<uint64_t> MachOFunctionSections;
  std::map<DataRef, SymbolId> SymbolIds;
  std::set<DataRef> IgnorableUndefinedSymbols;
  std::map<DataRef, std::string> UnwindMarkerCandidates;
};

LinkExpect<Target>
ObjectReader::ReadContext::validateInput(Span<const Byte> Buffer,
                                         const llvm::object::ObjectFile &Object,
                                         Target ExpectedTarget) {
  const auto TrailingCOFF = findTrailingCOFFObject(Buffer, Object);
  if (TrailingCOFF == COFFTrailingObject::Malformed) {
    return diagnosticError("malformed COFF structural metadata");
  }
  if (hasTrailingELFObject(Buffer, Object) ||
      TrailingCOFF == COFFTrailingObject::Found) {
    return diagnosticError("multiple input objects are not supported");
  }
  if (!Object.isRelocatableObject()) {
    return diagnosticError("input is not a relocatable object");
  }
  if (!isSupportedFormat(Object)) {
    return diagnosticError("unsupported object format",
                           DiagnosticKind::Unsupported);
  }
  const auto Normalized = normalizeTarget(Object.getArch());
  if (!Normalized) {
    return diagnosticError("unsupported object architecture",
                           DiagnosticKind::Unsupported);
  }
  if (*Normalized != ExpectedTarget) {
    return diagnosticError("object target does not match expected host target");
  }
  if (const auto *MachO =
          llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object);
      MachO && MachO->getHeader().cputype == MachOCPUTypeArm64 &&
      (MachO->getHeader().cpusubtype & ~MachOCPUSubtypeMask) !=
          MachOCPUSubtypeArm64All) {
    return diagnosticError(
        "unsupported non-generic AArch64 Mach-O CPU subtype (including arm64e)",
        DiagnosticKind::Unsupported);
  }
  return *Normalized;
}

LinkExpect<void> ObjectReader::ReadContext::beginInput() {
  if (auto Begun = Graph.beginInput("object"); !Begun)
    return addContext(std::move(Begun.error()),
                      "cannot initialize link graph input");
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readMachOBuildVersions() {
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object);
  if (!MachO)
    return {};
  for (const auto &Command : MachO->load_commands()) {
    if (Command.C.cmd != llvm::MachO::LC_BUILD_VERSION)
      continue;
    const Span<const Byte> Bytes(reinterpret_cast<const Byte *>(Command.Ptr),
                                 Command.C.cmdsize);
    EXPECTED_TRY(auto Version,
                 Internal::parseMachOBuildVersion(Bytes, Graph.endianness()));
    if (auto Added = Graph.addMachOBuildVersion(std::move(Version)); !Added)
      return addContext(std::move(Added.error()),
                        "cannot preserve Mach-O build version");
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readELFFlags() {
  const auto *ELF = llvm::dyn_cast<llvm::object::ELFObjectFileBase>(&Object);
  if (!ELF)
    return {};
  uint32_t InputFlags = ELF->getPlatformFlags();
  if (ActualTarget == Target::ARM) {
    EXPECTED_TRY(InputFlags, armELFFlags(Object, InputFlags));
  }
  if (auto Flags = Graph.setELFFlags(InputFlags); !Flags)
    return addContext(std::move(Flags.error()), "cannot preserve ELF flags");
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::collectMachOFunctionSections() {
  if (!Object.isMachO())
    return {};
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
    EXPECTED_TRY(const auto Type,
                 takeOr(InputSymbol.getType(), "cannot read symbol type"));
    EXPECTED_TRY(const uint32_t Flags, symbolFlags(InputSymbol));
    if (Type != llvm::object::SymbolRef::ST_Function &&
        (Flags & llvm::object::SymbolRef::SF_Executable) == 0)
      continue;
    EXPECTED_TRY(const auto InputSection,
                 takeOr(InputSymbol.getSection(),
                        "cannot read function symbol section"));
    if (InputSection != Object.section_end())
      MachOFunctionSections.emplace(InputSection->getIndex());
  }
  for (const auto &InputSection : Object.sections()) {
    EXPECTED_TRY(const llvm::StringRef Name,
                 takeOr(InputSection.getName(), "cannot read section name"));
    if (sectionPurpose(Object, Name) != SectionPurpose::CompactUnwind)
      continue;
    for (const auto &Relocation : InputSection.relocations()) {
      if (Relocation.getOffset() % CompactUnwindRecordSize !=
          CompactUnwindFunctionOffset)
        continue;
      auto TargetSection = Object.section_end();
      const auto InputSymbol = Relocation.getSymbol();
      if (InputSymbol == Object.symbol_end()) {
        TargetSection = machORelocationSection(Relocation);
      } else {
        EXPECTED_TRY(TargetSection,
                     takeOr(InputSymbol->getSection(),
                            "cannot read compact unwind function section"));
      }
      if (TargetSection != Object.section_end())
        MachOFunctionSections.emplace(TargetSection->getIndex());
    }
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readSections() {
  for (const auto &InputSection : Object.sections()) {
    if (ActualTarget == Target::ARM && Object.isELF() &&
        llvm::object::ELFSectionRef(InputSection).getType() ==
            llvm::ELF::SHT_ARM_ATTRIBUTES)
      continue;
    EXPECTED_TRY(const llvm::StringRef Name,
                 takeOr(InputSection.getName(), "cannot read section name"));
    if (Object.isCOFF() && Name == ".drectve") {
      EXPECTED_TRY(const llvm::StringRef Contents,
                   takeOr(InputSection.getContents(), "cannot read .drectve"));
      auto Exports = Internal::parseCOFFExports(Contents.str());
      if (!Exports) {
        return diagnosticError("malformed COFF .drectve export directive");
      }
      COFFExports.insert(Exports->begin(), Exports->end());
    }
    if (!isAllocatable(Object, InputSection)) {
      continue;
    }
    llvm::StringRef Contents;
    if (!InputSection.isVirtual()) {
      EXPECTED_TRY(Contents, takeOr(InputSection.getContents(),
                                    "cannot read section contents"));
    }
    std::vector<Byte> Bytes(Contents.bytes_begin(), Contents.bytes_end());
    const auto Purpose = sectionPurpose(Object, Name);
    if (Purpose == SectionPurpose::CompactUnwind) {
      if (CompactUnwindSection)
        return diagnosticError("multiple compact unwind sections");
      CompactUnwindSection = InputSection.getIndex();
      continue;
    }
    constexpr uint64_t EHFrameTerminatorSize = 4;
    const uint64_t VirtualSize =
        InputSection.getSize() +
        (Purpose == SectionPurpose::EHFrame ? EHFrameTerminatorSize : 0);
    if (VirtualSize < InputSection.getSize())
      return diagnosticError("section size overflows");
    if (VirtualSize != InputSection.getSize())
      Bytes.resize(static_cast<size_t>(VirtualSize));
    const auto Kind = MachOFunctionSections.count(InputSection.getIndex()) != 0
                          ? SectionKind::Text
                          : sectionKind(InputSection, Purpose);
    EXPECTED_TRY(const SectionId Added,
                 Graph.addSection(Section{Name.str(), Kind,
                                          sectionAlignment(InputSection),
                                          VirtualSize, 0, 0, std::move(Bytes),
                                          Purpose, InputSection.getAddress()}));
    SectionIds.emplace(InputSection.getIndex(), Added);
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::linkARMExidx() {
  for (const auto &InputSection : Object.sections()) {
    const auto Section = SectionIds.find(InputSection.getIndex());
    if (Section == SectionIds.end())
      continue;
    const auto &Value = Graph.sections()[Section->second];
    if (Value.Purpose != SectionPurpose::ARMExidx)
      continue;
    const auto LinkedInput = elfLinkedSection(Object, InputSection);
    if (!LinkedInput)
      return diagnosticError("cannot read linked ARM exidx section");
    const auto Linked = SectionIds.find(*LinkedInput);
    if (Linked == SectionIds.end() ||
        Graph.sections()[Linked->second].Kind != SectionKind::Text)
      return diagnosticError("invalid linked ARM exidx section");
    if (auto Associated =
            Graph.setLinkedSection(Section->second, Linked->second);
        !Associated)
      return addContext(std::move(Associated.error()),
                        "invalid linked ARM exidx section");
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readSymbols() {
  std::map<DataRef, uint64_t> SymbolSizes;
  for (const auto &[InputSymbol, Size] :
       llvm::object::computeSymbolSizes(Object)) {
    SymbolSizes.emplace(InputSymbol.getRawDataRefImpl(), Size);
  }
  for (const auto &InputSymbol : Object.symbols()) {
    EXPECTED_TRY(llvm::StringRef Name,
                 takeOr(InputSymbol.getName(), "cannot read symbol name"));
    EXPECTED_TRY(const uint32_t Flags, symbolFlags(InputSymbol));
    EXPECTED_TRY(const auto Type,
                 takeOr(InputSymbol.getType(), "cannot read symbol type"));
    if ((Flags & llvm::object::SymbolRef::SF_Undefined) != 0) {
      if (!Name.empty() && Type != llvm::object::SymbolRef::ST_File &&
          Type != llvm::object::SymbolRef::ST_Debug) {
        if (InputPolicy ==
                ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused &&
            Object.isCOFF() && Name == "_fltused" &&
            (ActualTarget == Target::X86_64 ||
             ActualTarget == Target::AArch64)) {
          IgnorableUndefinedSymbols.insert(InputSymbol.getRawDataRefImpl());
          continue;
        }
        if (Object.isELF() && ActualTarget == Target::ARM) {
          UnwindMarkerCandidates.emplace(InputSymbol.getRawDataRefImpl(),
                                         Name.str());
          continue;
        }
        return undefinedSymbol(Name.str());
      }
      continue;
    }
    auto InputSection = InputSymbol.getSection();
    if (!InputSection) {
      Diagnostic Diag{"cannot read symbol section: " +
                      llvm::toString(InputSection.takeError())};
      Diag.SymbolName = Name.str();
      return diagnosticError(std::move(Diag));
    }
    if (*InputSection == Object.section_end()) {
      continue;
    }
    const auto Section = SectionIds.find((*InputSection)->getIndex());
    if (Section == SectionIds.end()) {
      continue;
    }
    if (Name.empty()) {
      Name = SectionSymbolPrefix;
    }
    EXPECTED_TRY(const uint64_t Address, takeOr(InputSymbol.getAddress(),
                                                "cannot read symbol address"));
    const uint64_t Base = (*InputSection)->getAddress();
    if (Address < Base) {
      return diagnosticError("symbol address precedes its section");
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
    if (SymbolName == SectionSymbolPrefix) {
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
    if (Object.isMachO()) {
      const uint64_t SymbolOffset = Address - Base;
      const uint64_t SectionLimit =
          Graph.sections()[Section->second].VirtualSize;
      if (SymbolOffset <= SectionLimit)
        SymbolSize = std::min(SymbolSize, SectionLimit - SymbolOffset);
    }
    EXPECTED_TRY(const SymbolId Added,
                 Graph.addSymbol(Symbol{std::move(SymbolName), Section->second,
                                        Address - Base, SymbolSize, Exported,
                                        ExportName, Global, Thumb}));
    SymbolIds.emplace(InputSymbol.getRawDataRefImpl(), Added);
  }
  return {};
}

LinkExpect<SymbolId> ObjectReader::ReadContext::resolveCompactUnwindTarget(
    const llvm::object::RelocationRef &Relocation, uint64_t Addend) {
  const auto Metadata = relocationMetadata(Object, Relocation, ActualTarget);
  if (!Metadata)
    return diagnosticError("malformed compact unwind relocation");
  const auto InputSymbol = Relocation.getSymbol();
  auto TargetSection = Object.section_end();
  uint64_t SymbolOffset = 0;
  if (InputSymbol == Object.symbol_end()) {
    if (!Object.isMachO())
      return diagnosticError("compact unwind relocation has no target symbol");
    TargetSection = machORelocationSection(Relocation);
  } else {
    EXPECTED_TRY(TargetSection,
                 takeOr(InputSymbol->getSection(),
                        "cannot read compact unwind target section"));
    EXPECTED_TRY(const uint64_t Address,
                 takeOr(InputSymbol->getAddress(),
                        "cannot read compact unwind target address"));
    if (TargetSection != Object.section_end()) {
      const uint64_t Base = TargetSection->getAddress();
      if (Address < Base)
        return diagnosticError("compact unwind target precedes its section");
      SymbolOffset = Address - Base;
    }
  }
  if (TargetSection == Object.section_end())
    return diagnosticError("compact unwind relocation target is undefined");
  const auto GraphSection = graphSectionOf(TargetSection);
  if (!GraphSection)
    return diagnosticError("compact unwind targets an unsupported section",
                           DiagnosticKind::Unsupported);
  EXPECTED_TRY(const uint64_t Offset,
               Internal::resolveCompactUnwindTargetOffset(
                   Metadata->External, TargetSection->getAddress(),
                   SymbolOffset, Addend));
  if (Offset >= Graph.sections()[*GraphSection].VirtualSize)
    return diagnosticError("compact unwind target is outside its section");
  return findOrAddSymbolAt(Graph, *GraphSection, Offset,
                           SectionSymbols::Exclude, preferVisibleSymbol, [&] {
                             return "$compact_unwind." +
                                    std::to_string(TargetSection->getIndex()) +
                                    "." + std::to_string(Offset);
                           });
}

LinkExpect<SymbolId>
ObjectReader::ReadContext::resolveDwarfFDE(SymbolId Function,
                                           uint32_t Encoding) {
  if (!Internal::isDwarfCompactUnwind(ActualTarget, Encoding))
    return diagnosticError("compact unwind encoding is not DWARF");
  if (Function >= Graph.symbols().size())
    return diagnosticError("invalid DWARF compact unwind function");
  const auto &FunctionSymbol = Graph.symbols()[Function];
  const uint32_t EncodedOffset =
      Encoding & Internal::CompactUnwindDwarfOffsetMask;
  bool CanonicalCandidate = false;
  std::optional<std::pair<SectionId, uint64_t>> Match;
  for (const auto &EHSection : Object.sections()) {
    const auto GraphSection = SectionIds.find(EHSection.getIndex());
    if (GraphSection == SectionIds.end() ||
        Graph.sections()[GraphSection->second].Purpose !=
            SectionPurpose::EHFrame)
      continue;
    EXPECTED_TRY(
        const llvm::StringRef EHContents,
        takeOr(EHSection.getContents(), "cannot read EH frame contents"));
    const Span<const Byte> EHBytes(
        reinterpret_cast<const Byte *>(EHContents.data()), EHContents.size());
    std::optional<std::pair<uint64_t, SymbolId>> Subtractor;
    for (const auto &Relocation : EHSection.relocations()) {
      if (ActualTarget == Target::AArch64 &&
          Relocation.getType() == llvm::MachO::ARM64_RELOC_SUBTRACTOR) {
        const auto InputSymbol = Relocation.getSymbol();
        if (InputSymbol == Object.symbol_end()) {
          Subtractor.reset();
          continue;
        }
        const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
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
      if (ActualTarget == Target::AArch64 &&
          Relocation.getType() == llvm::MachO::ARM64_RELOC_UNSIGNED &&
          Subtractor && Subtractor->first == Relocation.getOffset()) {
        auto InputTargetSection = Object.section_end();
        if (InputSymbol == Object.symbol_end()) {
          InputTargetSection = machORelocationSection(Relocation);
        } else {
          EXPECTED_TRY(InputTargetSection,
                       takeOr(InputSymbol->getSection(),
                              "cannot read DWARF FDE target section"));
          const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
          if (Symbol == SymbolIds.end())
            continue;
          TargetOffset = Graph.symbols()[Symbol->second].Offset;
        }
        const auto GraphTargetSection = graphSectionOf(InputTargetSection);
        if (!GraphTargetSection)
          continue;
        TargetSection = *GraphTargetSection;
        const auto &SubtractorSymbol = Graph.symbols()[Subtractor->second];
        const uint64_t Field = Relocation.getOffset();
        if (SubtractorSymbol.Offset > Field)
          return diagnosticError("invalid DWARF FDE subtractor offset");
        uint64_t Raw = 0;
        if (!readInteger(EHBytes, Field, true, Raw))
          return diagnosticError("cannot read DWARF FDE addend");
        auto Resolved = Internal::resolveMachOFDEAddress(
            TargetOffset, 0, Field - SubtractorSymbol.Offset,
            static_cast<int64_t>(Raw));
        if (!Resolved)
          return diagnosticError("DWARF FDE target address overflows");
        TargetOffset = *Resolved;
        Subtractor.reset();
      } else {
        if (InputSymbol == Object.symbol_end())
          continue;
        const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
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
      if (Field < Internal::EHFrameRecordHeaderSize)
        return diagnosticError("invalid DWARF compact unwind FDE field");
      const uint64_t Record = Field - Internal::EHFrameRecordHeaderSize;
      uint64_t Cursor = 0;
      bool Boundary = false;
      while (Cursor < EHBytes.size()) {
        uint32_t Length = 0;
        if (!readInteger(EHBytes, Cursor, true, Length) || Length == 0 ||
            Length == UINT32_MAX ||
            Length > EHBytes.size() - Cursor - Internal::EHFrameLengthFieldSize)
          return diagnosticError("malformed EH frame record");
        if (Cursor == Record) {
          uint32_t Id = 0;
          if (!readInteger(EHBytes, Cursor + Internal::EHFrameLengthFieldSize,
                           true, Id) ||
              Id == 0)
            return diagnosticError("invalid DWARF compact unwind FDE");
          Boundary = true;
          break;
        }
        Cursor += Internal::EHFrameLengthFieldSize + Length;
      }
      if (!Boundary)
        return diagnosticError("DWARF compact unwind offset is not an FDE");
      if (EncodedOffset != 0 && EncodedOffset != Record)
        continue;
      if (Match && *Match != std::pair{GraphSection->second, Record})
        return diagnosticError("ambiguous DWARF compact unwind FDE");
      Match = std::pair{GraphSection->second, Record};
    }
  }
  if (!Match && EncodedOffset != 0 && CanonicalCandidate)
    return diagnosticError("DWARF compact unwind FDE offset mismatch");
  if (!Match)
    return diagnosticError("missing DWARF compact unwind FDE");
  return findOrAddSymbolAt(
      Graph, Match->first, Match->second, SectionSymbols::Include, nullptr,
      [&] { return "$compact_unwind.fde." + std::to_string(Match->second); });
}

LinkExpect<void> ObjectReader::ReadContext::readCompactUnwind() {
  if (!CompactUnwindSection)
    return {};
  auto InputSection = std::find_if(
      Object.sections().begin(), Object.sections().end(),
      [&](const auto &S) { return S.getIndex() == *CompactUnwindSection; });
  if (InputSection == Object.sections().end())
    return diagnosticError("cannot find compact unwind section");
  EXPECTED_TRY(const llvm::StringRef Contents,
               takeOr(InputSection->getContents(),
                      "cannot read compact unwind contents"));
  const Span<const Byte> Bytes(reinterpret_cast<const Byte *>(Contents.data()),
                               Contents.size());
  if (Bytes.size() % CompactUnwindRecordSize != 0)
    return diagnosticError("compact unwind size is not a multiple of 32");

  std::map<uint64_t, llvm::object::RelocationRef> Relocations;
  std::vector<Internal::CompactUnwindRelocation> DecodedRelocations;
  for (const auto &Relocation : InputSection->relocations()) {
    const auto Metadata = relocationMetadata(Object, Relocation, ActualTarget);
    if (!Metadata || Relocation.getType() > UINT32_MAX)
      return diagnosticError("malformed compact unwind relocation");
    DecodedRelocations.push_back(Internal::CompactUnwindRelocation{
        Relocation.getOffset(), static_cast<uint32_t>(Relocation.getType()),
        Metadata->PatchSize, Metadata->PCRelative, Metadata->External,
        Metadata->Scattered});
    Relocations.emplace(Relocation.getOffset(), Relocation);
  }
  EXPECTED_TRY(const auto Records,
               Internal::parseCompactUnwindSection(ActualTarget, Bytes,
                                                   DecodedRelocations));

  for (size_t I = 0; I < Records.size(); ++I) {
    const uint64_t Offset = I * CompactUnwindRecordSize;
    const auto &Record = Records[I];
    const auto FunctionRelocation =
        Relocations.find(Offset + CompactUnwindFunctionOffset);
    if (FunctionRelocation == Relocations.end())
      return diagnosticError("compact unwind record lacks function relocation");
    EXPECTED_TRY(const SymbolId Function,
                 resolveCompactUnwindTarget(FunctionRelocation->second,
                                            Record.Function));
    const auto ResolveOptional =
        [&](uint64_t Field) -> LinkExpect<std::optional<SymbolId>> {
      const auto Relocation = Relocations.find(Offset + Field);
      if (Relocation == Relocations.end())
        return std::optional<SymbolId>{};
      uint64_t Addend = 0;
      if (!readInteger(Bytes, Offset + Field, true, Addend))
        return diagnosticError("cannot read compact unwind target");
      EXPECTED_TRY(const SymbolId Symbol,
                   resolveCompactUnwindTarget(Relocation->second, Addend));
      return std::optional<SymbolId>{Symbol};
    };
    auto Personality = ResolveOptional(CompactUnwindPersonalityOffset);
    auto LSDA = ResolveOptional(CompactUnwindLSDAOffset);
    if (!Personality)
      return diagnosticError(std::move(Personality.error()));
    if (!LSDA)
      return diagnosticError(std::move(LSDA.error()));
    std::optional<SymbolId> FDE;
    if (Internal::isDwarfCompactUnwind(ActualTarget, Record.Encoding)) {
      EXPECTED_TRY(FDE, resolveDwarfFDE(Function, Record.Encoding));
    }
    EXPECTED_TRY(Graph.addCompactUnwind(CompactUnwindRecord{
        Function, Record.Length, Record.Encoding, *Personality, *LSDA, FDE}));
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::checkUndefinedMarkers() {
  if (!IgnorableUndefinedSymbols.empty()) {
    for (const auto &InputSection : Object.sections()) {
      for (const auto &InputRelocation : InputSection.relocations()) {
        const auto InputSymbol = InputRelocation.getSymbol();
        if (InputSymbol != Object.symbol_end() &&
            IgnorableUndefinedSymbols.count(InputSymbol->getRawDataRefImpl()) !=
                0)
          return diagnosticError("relocation targets MSVC CRT marker");
      }
    }
  }

  if (!UnwindMarkerCandidates.empty()) {
    std::set<DataRef> Referenced;
    for (const auto &InputSection : Object.sections()) {
      for (const auto &InputRelocation : InputSection.relocations()) {
        const auto InputSymbol = InputRelocation.getSymbol();
        if (InputSymbol == Object.symbol_end())
          continue;
        const auto Candidate =
            UnwindMarkerCandidates.find(InputSymbol->getRawDataRefImpl());
        if (Candidate == UnwindMarkerCandidates.end())
          continue;
        if (InputRelocation.getType() != llvm::ELF::R_ARM_NONE)
          return undefinedSymbol(Candidate->second);
        Referenced.insert(Candidate->first);
      }
    }
    for (const auto &[Candidate, CandidateName] : UnwindMarkerCandidates) {
      if (Referenced.count(Candidate) == 0)
        return undefinedSymbol(CandidateName);
    }
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readAArch64MachOEHFramePair(
    SectionId Section, const RelocationMetadata &Metadata,
    llvm::object::relocation_iterator &Relocation,
    const llvm::object::relocation_iterator &End) {
  const auto &Input = Graph.sections()[Section];
  if (Metadata.PatchSize != DoubleWordPatch || Metadata.PCRelative ||
      !Metadata.External || Metadata.Scattered)
    return diagnosticError("malformed AArch64 Mach-O EH frame subtractor");
  const uint64_t Offset = Relocation->getOffset();
  auto ParsedFields = machOEHFrameFields(Input.Content, ActualTarget);
  if (!ParsedFields)
    return diagnosticError("malformed Mach-O EH frame");
  if (ParsedFields->count(static_cast<size_t>(Offset)) == 0)
    return diagnosticError("AArch64 Mach-O EH frame relocation is not an FDE "
                           "initial-location field");
  const auto SubtractorInputSymbol = Relocation->getSymbol();
  if (SubtractorInputSymbol == Object.symbol_end())
    return diagnosticError("AArch64 Mach-O EH frame subtractor has no "
                           "target symbol");
  const auto Subtractor =
      SymbolIds.find(SubtractorInputSymbol->getRawDataRefImpl());
  if (Subtractor == SymbolIds.end() ||
      Graph.symbols()[Subtractor->second].Section != Section)
    return diagnosticError(
        "unsupported AArch64 Mach-O EH frame subtractor symbol",
        DiagnosticKind::Unsupported);
  ++Relocation;
  if (Relocation == End ||
      Relocation->getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
    return diagnosticError("AArch64 Mach-O EH frame subtractor is not followed "
                           "by unsigned relocation");
  const auto UnsignedMetadata =
      relocationMetadata(Object, *Relocation, ActualTarget);
  if (!UnsignedMetadata || UnsignedMetadata->PatchSize != DoubleWordPatch ||
      UnsignedMetadata->PCRelative || UnsignedMetadata->Scattered)
    return diagnosticError(
        "malformed AArch64 Mach-O EH frame unsigned relocation");
  if (Relocation->getOffset() != Offset)
    return diagnosticError(
        "AArch64 Mach-O EH frame relocation pair addresses differ");
  const auto UnsignedInputSymbol = Relocation->getSymbol();
  const auto &SubtractorSymbol = Graph.symbols()[Subtractor->second];
  if (SubtractorSymbol.Offset > Offset)
    return diagnosticError(
        "invalid AArch64 Mach-O EH frame subtractor symbol offset");
  uint64_t Raw = 0;
  if (!readInteger(Input.Content, Offset, true, Raw))
    return diagnosticError(
        "malformed AArch64 Mach-O EH frame relocation addend");
  const uint64_t Delta = Offset - SubtractorSymbol.Offset;
  SymbolId Unsigned = InvalidSymbolId;
  if (UnsignedMetadata->External) {
    if (UnsignedInputSymbol == Object.symbol_end())
      return diagnosticError("AArch64 Mach-O EH frame unsigned "
                             "relocation has no target symbol");
    const auto Symbol =
        SymbolIds.find(UnsignedInputSymbol->getRawDataRefImpl());
    if (Symbol == SymbolIds.end() || Raw != UINT64_C(0) - Delta)
      return diagnosticError(
          "malformed AArch64 Mach-O EH frame relocation addend");
    Unsigned = Symbol->second;
  } else {
    const auto GraphTargetSection =
        graphSectionOf(machORelocationSection(*Relocation));
    if (!GraphTargetSection)
      return diagnosticError(
          "AArch64 Mach-O EH frame relocation targets an unsupported "
          "section",
          DiagnosticKind::Unsupported);
    auto TargetOffset = Internal::resolveMachOFDEAddress(
        0, 0, Delta, static_cast<int64_t>(Raw));
    if (!TargetOffset)
      return diagnosticError("DWARF FDE target address overflows");
    const auto Symbol = findSymbolAt(Graph, *GraphTargetSection, *TargetOffset,
                                     SectionSymbols::Exclude);
    if (!Symbol)
      return diagnosticError(
          "AArch64 Mach-O EH frame relocation targets an unsupported "
          "symbol",
          DiagnosticKind::Unsupported);
    Unsigned = *Symbol;
  }
  const auto &UnsignedSymbol = Graph.symbols()[Unsigned];
  if (UnsignedSymbol.Section >= Graph.sections().size() ||
      Graph.sections()[UnsignedSymbol.Section].Kind != SectionKind::Text)
    return diagnosticError(
        "AArch64 Mach-O EH frame relocation target is not a function "
        "symbol",
        DiagnosticKind::Unsupported);
  if (auto Added = Graph.addEHFrameReference(
          EHFrameReference{Section, Offset, Unsigned});
      !Added)
    return addContext(std::move(Added.error()),
                      "invalid Mach-O EH frame relocation");
  return {};
}

LinkExpect<std::optional<int64_t>>
ObjectReader::ReadContext::resolveMachOSectionAddend(
    const Section &Input, const llvm::object::RelocationRef &Relocation,
    llvm::object::section_iterator TargetSection, const Section &GraphTarget,
    uint32_t Type, const RelocationMetadata &Metadata,
    std::optional<uint8_t> PatchSize) {
  if (ActualTarget == Target::X86_64 && Metadata.PCRelative) {
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
      return diagnosticError(
          "unsupported PC-relative x86_64 Mach-O section relocation "
          "type",
          DiagnosticKind::Unsupported);
    }
    const uint64_t Offset = Relocation.getOffset();
    auto Raw = Internal::readSigned(Input.Content, Offset, WordPatch,
                                    Endianness::Little);
    if (!Raw)
      return diagnosticError(
          "cannot read x86_64 Mach-O section relocation addend");
    uint64_t Positive = Input.InputAddress;
    if (Offset > UINT64_MAX - Positive)
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    Positive += Offset;
    if (WordPatch > UINT64_MAX - Positive)
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    Positive += WordPatch;
    if (Suffix > UINT64_MAX - Positive)
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    Positive += Suffix;
    uint64_t Negative = TargetSection->getAddress();
    if (*Raw >= 0) {
      const uint64_t Value = static_cast<uint64_t>(*Raw);
      if (Value > UINT64_MAX - Positive)
        return diagnosticError(
            "x86_64 Mach-O section relocation offset overflows");
      Positive += Value;
    } else {
      const uint64_t Magnitude = static_cast<uint64_t>(-(*Raw + 1)) + 1;
      if (Magnitude > UINT64_MAX - Negative)
        return diagnosticError(
            "x86_64 Mach-O section relocation offset overflows");
      Negative += Magnitude;
    }
    if (Positive < Negative)
      return diagnosticError(
          "x86_64 Mach-O section relocation target is negative");
    const uint64_t TargetOffset = Positive - Negative;
    if (TargetOffset > static_cast<uint64_t>(INT64_MAX))
      return diagnosticError(
          "x86_64 Mach-O section relocation offset overflows");
    if (TargetOffset >= GraphTarget.VirtualSize)
      return diagnosticError(
          "x86_64 Mach-O section relocation target is outside its "
          "section");
    return static_cast<int64_t>(TargetOffset);
  }
  if (PatchSize && ((ActualTarget == Target::X86_64 &&
                     Type == llvm::MachO::X86_64_RELOC_UNSIGNED) ||
                    (ActualTarget == Target::AArch64 &&
                     Type == llvm::MachO::ARM64_RELOC_UNSIGNED))) {
    const auto Raw = Internal::readUnsigned(
        Input.Content, Relocation.getOffset(), *PatchSize,
        Object.isLittleEndian() ? Endianness::Little : Endianness::Big);
    if (!Raw || *Raw < TargetSection->getAddress() ||
        *Raw - TargetSection->getAddress() > static_cast<uint64_t>(INT64_MAX)) {
      return diagnosticError("invalid Mach-O section relocation target offset");
    }
    const uint64_t TargetOffset = *Raw - TargetSection->getAddress();
    if (TargetOffset >= GraphTarget.VirtualSize) {
      return diagnosticError(
          "Mach-O section relocation target is outside its section");
    }
    return static_cast<int64_t>(TargetOffset);
  }
  if (ActualTarget == Target::AArch64 &&
      Type == llvm::MachO::ARM64_RELOC_PAGEOFF12) {
    const auto Raw = Internal::readUnsigned(
        Input.Content, Relocation.getOffset(), WordPatch, Endianness::Little);
    if (!Raw)
      return diagnosticError("cannot read AArch64 Mach-O PAGEOFF12 addend");
    const uint32_t Instruction = static_cast<uint32_t>(*Raw);
    const unsigned Scale =
        Internal::aarch64LoadStoreScale(Instruction).value_or(0);
    return static_cast<int64_t>(((Instruction & UINT32_C(0x003FFC00)) >> 10)
                                << Scale);
  }
  return std::optional<int64_t>{};
}

LinkExpect<void> ObjectReader::ReadContext::readRelocations() {
  for (const auto &InputSection : Object.sections()) {
    auto RelocatedSection = InputSection.getRelocatedSection();
    if (!RelocatedSection) {
      return diagnosticError("cannot read relocated section: " +
                             llvm::toString(RelocatedSection.takeError()));
    }
    const uint64_t SectionIndex = *RelocatedSection == Object.section_end()
                                      ? InputSection.getIndex()
                                      : (*RelocatedSection)->getIndex();
    const auto Section = SectionIds.find(SectionIndex);
    if (Section == SectionIds.end()) {
      continue;
    }
    const SectionId GraphSection = Section->second;
    const auto &Input = Graph.sections()[GraphSection];
    if (Input.Purpose == SectionPurpose::CompactUnwind)
      continue;
    if (Input.Purpose == SectionPurpose::XData &&
        InputSection.relocation_begin() != InputSection.relocation_end()) {
      return diagnosticError("personality relocation in .xdata is unsupported",
                             DiagnosticKind::Unsupported);
    }
    auto InputRelocation = InputSection.relocation_begin();
    const auto RelocationEnd = InputSection.relocation_end();
    while (InputRelocation != RelocationEnd) {
      if (InputRelocation->getType() > UINT32_MAX) {
        return diagnosticError("relocation type is out of range");
      }
      uint32_t Type = static_cast<uint32_t>(InputRelocation->getType());
      if (Object.isELF() && ActualTarget == Target::ARM &&
          Type == llvm::ELF::R_ARM_NONE) {
        ++InputRelocation;
        continue;
      }
      auto Metadata =
          relocationMetadata(Object, *InputRelocation, ActualTarget);
      if (!Metadata) {
        return diagnosticError("malformed relocation metadata");
      }
      std::optional<int64_t> PairedAddend;
      if (Object.isMachO() && ActualTarget == Target::AArch64 &&
          Type == llvm::MachO::ARM64_RELOC_ADDEND) {
        EXPECTED_TRY(const auto Pair, consumeAArch64MachOAddendPair(
                                          Object, Input.Content,
                                          InputRelocation, RelocationEnd));
        Type = Pair.Type;
        Metadata = Pair.Metadata;
        PairedAddend = Pair.Addend;
      }
      if (Object.isMachO() && ActualTarget == Target::AArch64 &&
          Input.Purpose == SectionPurpose::EHFrame) {
        const auto AArch64Type = InputRelocation->getType();
        if (AArch64Type == llvm::MachO::ARM64_RELOC_UNSIGNED)
          return diagnosticError(
              "AArch64 Mach-O EH frame unsigned relocation lacks subtractor");
        if (AArch64Type == llvm::MachO::ARM64_RELOC_SUBTRACTOR) {
          EXPECTED_TRY(readAArch64MachOEHFramePair(
              GraphSection, *Metadata, InputRelocation, RelocationEnd));
          ++InputRelocation;
          continue;
        }
      }
      const auto InputSymbol = InputRelocation->getSymbol();
      SymbolId TargetSymbol = InvalidSymbolId;
      int64_t Addend = 0;
      bool Implicit = true;
      const auto PatchSize = relocationPatchSize(
          objectFormat(Object), ActualTarget, Type, Metadata->PatchSize);
      if (InputSymbol == Object.symbol_end()) {
        if (Object.isELF() && PatchSize && *PatchSize == NoPatch) {
          ++InputRelocation;
          continue;
        }
        if (!Object.isMachO() || Metadata->External)
          return diagnosticError("relocation has no target symbol");
        const auto TargetSection = machORelocationSection(*InputRelocation);
        const auto GraphTargetSection = graphSectionOf(TargetSection);
        if (!GraphTargetSection)
          return diagnosticError("relocation targets an unsupported section",
                                 DiagnosticKind::Unsupported);
        EXPECTED_TRY(
            const auto ExplicitAddend,
            resolveMachOSectionAddend(Input, *InputRelocation, TargetSection,
                                      Graph.sections()[*GraphTargetSection],
                                      Type, *Metadata, PatchSize));
        if (ExplicitAddend) {
          Addend = *ExplicitAddend;
          Implicit = false;
        }
        EXPECTED_TRY(TargetSymbol,
                     findOrAddSymbolAt(Graph, *GraphTargetSection, 0,
                                       SectionSymbols::Include, nullptr, [&] {
                                         return SectionSymbolPrefix.str() +
                                                std::to_string(
                                                    TargetSection->getIndex());
                                       }));
      } else {
        const auto Symbol = SymbolIds.find(InputSymbol->getRawDataRefImpl());
        if (Symbol == SymbolIds.end())
          return diagnosticError("relocation targets an unsupported symbol",
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
        Diag.Section = GraphSection;
        Diag.Symbol = TargetSymbol;
        Diag.RelocationType = Type;
        Diag.Offset = InputRelocation->getOffset();
        Diag.SectionName = Input.Name;
        Diag.SymbolName = Graph.symbols()[TargetSymbol].Name;
        Diag.Kind = DiagnosticKind::Unsupported;
        return diagnosticError(std::move(Diag));
      }
      EXPECTED_TRY(Graph.addRelocation(Relocation{
          GraphSection, InputRelocation->getOffset(), Type, TargetSymbol,
          Addend, Implicit, objectFormat(Object), *PatchSize,
          Metadata->PCRelative, Metadata->External, Metadata->Scattered}));
      ++InputRelocation;
    }
  }
  return {};
}

LinkExpect<LinkGraph> ObjectReader::ReadContext::finish() {
  EXPECTED_TRY(associateRawX86_64MachOFDEs(Graph));
  EXPECTED_TRY(Graph.validate());
  if (!Graph.compactUnwind().empty() && !validateCompactUnwind(Graph))
    return diagnosticError("unsupported compact unwind encoding",
                           DiagnosticKind::Unsupported);
  return std::move(Graph);
}

LinkExpect<LinkGraph> ObjectReader::read(Span<const Byte> Buffer,
                                         Target ExpectedTarget,
                                         ObjectReaderInputPolicy InputPolicy) {
  EXPECTED_TRY(const auto Object, openObject(Buffer));
  EXPECTED_TRY(const Target ActualTarget,
               ReadContext::validateInput(Buffer, *Object, ExpectedTarget));
  ReadContext Context(*Object, ActualTarget, InputPolicy);
  EXPECTED_TRY(Context.beginInput());
  EXPECTED_TRY(Context.readMachOBuildVersions());
  EXPECTED_TRY(Context.readELFFlags());
  EXPECTED_TRY(Context.collectMachOFunctionSections());
  EXPECTED_TRY(Context.readSections());
  EXPECTED_TRY(Context.linkARMExidx());
  EXPECTED_TRY(Context.readSymbols());
  EXPECTED_TRY(Context.readCompactUnwind());
  EXPECTED_TRY(Context.checkUndefinedMarkers());
  EXPECTED_TRY(Context.readRelocations());
  return Context.finish();
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
