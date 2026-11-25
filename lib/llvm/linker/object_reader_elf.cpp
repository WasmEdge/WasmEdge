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

#include "linker/object_reader_context.h"

#include "linker/eh_frame.h"

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/ARMAttributeParser.h>
#include <llvm/Support/ARMBuildAttributes.h>

#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

using namespace Reader;

namespace {

bool readARMAttributeULEB(llvm::ArrayRef<uint8_t> Bytes, size_t &Offset,
                          uint64_t &Value) {
  return Internal::readULEB128(Span<const Byte>(Bytes.data(), Bytes.size()),
                               Offset, Value);
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

#if LLVM_VERSION_MAJOR < 11
#else
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
#endif

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
#if LLVM_VERSION_MAJOR < 11
#else
    EXPECTED_TRY(const auto ParserBytes,
                 armAttributesForLLVM(Bytes, Object.isLittleEndian()));
    llvm::ARMAttributeParser Parser;
#if LLVM_VERSION_MAJOR < 18
    const auto Endian =
        Object.isLittleEndian() ? llvm::support::little : llvm::support::big;
#else
    const auto Endian = Object.isLittleEndian() ? llvm::endianness::little
                                                : llvm::endianness::big;
#endif
    if (auto Parsed = Parser.parse(ParserBytes, Endian))
      return diagnosticError("malformed ARM attributes: " +
                             llvm::toString(std::move(Parsed)));
#endif
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

template <typename ELFT>
bool validTypedELFTables(const llvm::object::ELFObjectFile<ELFT> &Object) {
  const std::vector<llvm::object::SectionRef> Sections(Object.section_begin(),
                                                       Object.section_end());
  const auto SymbolCount =
      [&](const llvm::object::SectionRef &Section) -> std::optional<uint64_t> {
    const auto *Header = Object.getSection(Section.getRawDataRefImpl());
    if ((Header->sh_type != llvm::ELF::SHT_SYMTAB &&
         Header->sh_type != llvm::ELF::SHT_DYNSYM) ||
        Header->sh_entsize != sizeof(typename ELFT::Sym))
      return std::nullopt;
    return sectionEntryCount(Section, sizeof(typename ELFT::Sym));
  };
  for (const auto &Section : Sections) {
    const auto *Header = Object.getSection(Section.getRawDataRefImpl());
    const uint32_t Type = Header->sh_type;
    if (Type == llvm::ELF::SHT_SYMTAB) {
      if (!SymbolCount(Section))
        return false;
      continue;
    }
#if LLVM_VERSION_MAJOR < 19
    const bool IsCrel = false;
#else
    const bool IsCrel = Type == llvm::ELF::SHT_CREL;
#endif
    if (Type != llvm::ELF::SHT_REL && Type != llvm::ELF::SHT_RELA && !IsCrel)
      continue;
    const uint64_t EntrySize = IsCrel ? 1
                               : Type == llvm::ELF::SHT_RELA
                                   ? sizeof(typename ELFT::Rela)
                                   : sizeof(typename ELFT::Rel);
    if ((!IsCrel && Header->sh_entsize != EntrySize) ||
        !sectionEntryCount(Section, EntrySize) ||
        Header->sh_link >= Sections.size())
      return false;
    const auto Symbols = SymbolCount(Sections[Header->sh_link]);
    if (!Symbols)
      return false;
    const auto RelocationSymbol =
        [&](llvm::object::DataRefImpl Reference) -> uint64_t {
#if LLVM_VERSION_MAJOR < 19
#else
      if (IsCrel)
        return Object.getCrel(Reference).r_symidx;
#endif
      if (Type == llvm::ELF::SHT_RELA)
        return Object.getRela(Reference)->getSymbol(false);
      return Object.getRel(Reference)->getSymbol(false);
    };
    for (const auto &Relocation : Section.relocations()) {
      const uint64_t Symbol = RelocationSymbol(Relocation.getRawDataRefImpl());
      if (Symbol != 0 && Symbol >= *Symbols)
        return false;
    }
#if LLVM_VERSION_MAJOR < 19
#else
    if (IsCrel &&
        !Object.getCrelDecodeProblem(Section.getRawDataRefImpl()).empty())
      return false;
#endif
  }
  return true;
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

} // namespace

bool Reader::validELFTables(const llvm::object::ObjectFile &Object) {
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF32LEObjectFile>(&Object))
    return validTypedELFTables(*ELF);
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF32BEObjectFile>(&Object))
    return validTypedELFTables(*ELF);
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(&Object))
    return validTypedELFTables(*ELF);
  if (const auto *ELF =
          llvm::dyn_cast<llvm::object::ELF64BEObjectFile>(&Object))
    return validTypedELFTables(*ELF);
  return true;
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

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
