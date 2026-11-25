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

#include "linker/byte_io.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/relocation.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/SymbolSize.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>
#if LLVM_VERSION_MAJOR < 12
#include <llvm/Support/MemoryBuffer.h>
#else
#include <llvm/Support/MemoryBufferRef.h>
#endif

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
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
constexpr uint32_t MachOCPUTypeArm64 = 0x0100000C;
constexpr uint32_t MachOCPUSubtypeMask = 0xFF000000;
constexpr uint32_t MachOCPUSubtypeArm64All = 0;

LinkExpect<std::unique_ptr<llvm::object::ObjectFile>>
openObject(Span<const Byte> Buffer) {
  if (Buffer.empty()) {
    return diagnosticError("empty object buffer");
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

namespace Reader {

namespace {

constexpr llvm::StringLiteral SectionSymbolPrefix = "$section";

Unexpected<Diagnostic> undefinedSymbol(std::string Name) {
  Diagnostic Diag{"undefined symbol"};
  Diag.SymbolName = std::move(Name);
  return diagnosticError(std::move(Diag));
}

bool startsWith(llvm::StringRef Value, llvm::StringRef Prefix) noexcept {
#if LLVM_VERSION_MAJOR < 16
  return Value.startswith(Prefix);
#else
  return Value.starts_with(Prefix);
#endif
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

uint64_t sectionAlignment(const llvm::object::SectionRef &Section) noexcept {
#if LLVM_VERSION_MAJOR < 16
  return Internal::normalizeSectionAlignment(Section.getAlignment());
#else
  return Internal::normalizeSectionAlignment(Section.getAlignment().value());
#endif
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

bool isWritable(const llvm::object::ObjectFile &Object,
                const llvm::object::SectionRef &Section,
                llvm::StringRef Name) noexcept {
  if (llvm::isa<llvm::object::ELFObjectFileBase>(&Object))
    return (llvm::object::ELFSectionRef(Section).getFlags() &
            llvm::ELF::SHF_WRITE) != 0;
  if (const auto *COFF = llvm::dyn_cast<llvm::object::COFFObjectFile>(&Object))
    return (COFF->getCOFFSection(Section)->Characteristics &
            llvm::COFF::IMAGE_SCN_MEM_WRITE) != 0;
  if (const auto *MachO =
          llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object)) {
    const auto Segment =
        MachO->getSectionFinalSegmentName(Section.getRawDataRefImpl());
    return Segment != "__TEXT" && Segment != "__DATA_CONST" &&
           (Segment != "__DATA" || Name != "__const");
  }
  return true;
}

SectionKind sectionKind(const llvm::object::ObjectFile &Object,
                        const llvm::object::SectionRef &Section,
                        llvm::StringRef Name, SectionPurpose Purpose) noexcept {
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
  return isWritable(Object, Section, Name) ? SectionKind::Data
                                           : SectionKind::ReadOnly;
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

} // namespace

Unexpected<Diagnostic> addContext(Diagnostic Value, std::string_view Context) {
  Value.Message = std::string(Context) + ": " + Value.Message;
  return diagnosticError(std::move(Value));
}

llvm::Expected<llvm::StringRef>
sectionName(const llvm::object::SectionRef &Section) {
#if LLVM_VERSION_MAJOR < 10
  llvm::StringRef Name;
  if (const auto Error = Section.getName(Name))
    return llvm::errorCodeToError(Error);
  return Name;
#else
  return Section.getName();
#endif
}

bool isSectionSymbol(llvm::StringRef Name) noexcept {
  return startsWith(Name, SectionSymbolPrefix);
}

LinkExpect<uint32_t> symbolFlags(const llvm::object::SymbolRef &Symbol) {
#if LLVM_VERSION_MAJOR < 11
  return Symbol.getFlags();
#else
  return takeOr(Symbol.getFlags(), "cannot read symbol flags");
#endif
}

std::optional<uint64_t>
sectionEntryCount(const llvm::object::SectionRef &Section, uint64_t EntrySize) {
  auto Contents = Section.getContents();
  if (!Contents) {
    llvm::consumeError(Contents.takeError());
    return std::nullopt;
  }
  if (EntrySize == 0 || Contents->size() % EntrySize != 0)
    return std::nullopt;
  return Contents->size() / EntrySize;
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

ObjectFormat objectFormat(const llvm::object::ObjectFile &Object) noexcept {
  if (Object.isMachO()) {
    return ObjectFormat::MachO;
  }
  if (Object.isCOFF()) {
    return ObjectFormat::COFF;
  }
  return ObjectFormat::ELF;
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
  return Metadata;
}

} // namespace Reader

namespace Internal {

ObjectReaderInputPolicy nativeObjectInputPolicy(ObjectFormat Format) noexcept {
  return Format == ObjectFormat::COFF
             ? ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused
             : ObjectReaderInputPolicy::Strict;
}

uint64_t normalizeSectionAlignment(uint64_t Alignment) noexcept {
  return std::max<uint64_t>(Alignment, 1);
}

} // namespace Internal

using namespace Reader;

LinkExpect<Target>
ObjectReader::ReadContext::validateInput(const llvm::object::ObjectFile &Object,
                                         Target ExpectedTarget) {
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
  if (!validELFTables(Object)) {
    return diagnosticError("malformed ELF relocation metadata");
  }
  return *Normalized;
}

LinkExpect<void> ObjectReader::ReadContext::beginInput() {
  if (auto Begun = Graph.beginInput("object"); !Begun)
    return addContext(std::move(Begun.error()),
                      "cannot initialize link graph input");
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readSections() {
  for (const auto &InputSection : Object.sections()) {
    if (ActualTarget == Target::ARM && Object.isELF() &&
        llvm::object::ELFSectionRef(InputSection).getType() ==
            llvm::ELF::SHT_ARM_ATTRIBUTES)
      continue;
    EXPECTED_TRY(const llvm::StringRef Name,
                 takeOr(sectionName(InputSection), "cannot read section name"));
    if (Object.isCOFF() && Name == ".drectve") {
      EXPECTED_TRY(const llvm::StringRef Contents,
                   takeOr(InputSection.getContents(), "cannot read .drectve"));
      auto Exports = Internal::parseCOFFExports(Contents.str());
      if (!Exports) {
        return diagnosticError("malformed COFF .drectve export directive");
      }
      for (const auto &[ExportName, SymbolName] : *Exports) {
        if (const auto [Found, Inserted] =
                COFFExports.emplace(ExportName, SymbolName);
            !Inserted && Found->second != SymbolName)
          return diagnosticError("COFF export name refers to two symbols");
      }
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
                          : sectionKind(Object, InputSection, Name, Purpose);
    EXPECTED_TRY(const SectionId Added,
                 Graph.addSection(Section{Name.str(), Kind,
                                          sectionAlignment(InputSection),
                                          VirtualSize, 0, 0, std::move(Bytes),
                                          Purpose, InputSection.getAddress()}));
    SectionIds.emplace(InputSection.getIndex(), Added);
  }
  return {};
}

LinkExpect<void> ObjectReader::ReadContext::readSymbols() {
  std::map<DataRef, uint64_t> SymbolSizes;
  for (const auto &[InputSymbol, Size] :
       llvm::object::computeSymbolSizes(Object)) {
    SymbolSizes.emplace(InputSymbol.getRawDataRefImpl(), Size);
  }
  std::map<std::string_view, std::string_view> COFFExportsBySymbol;
  for (const auto &[ExportName, SymbolName] : COFFExports)
    if (!COFFExportsBySymbol.emplace(SymbolName, ExportName).second)
      return diagnosticError("COFF symbol has more than one export name");
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
        if (Object.isELF() && ActualTarget == Target::ARM &&
            (Name == "__aeabi_unwind_cpp_pr0" ||
             Name == "__aeabi_unwind_cpp_pr1" ||
             Name == "__aeabi_unwind_cpp_pr2")) {
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
    if (const auto *MachO =
            llvm::dyn_cast<llvm::object::MachOObjectFile>(&Object)) {
      const auto Ref = InputSymbol.getRawDataRefImpl();
      const uint8_t MachOType = MachO->is64Bit()
                                    ? MachO->getSymbol64TableEntry(Ref).n_type
                                    : MachO->getSymbolTableEntry(Ref).n_type;
      Exported = Global && (MachOType & llvm::MachO::N_PEXT) == 0;
    } else if (Object.isCOFF()) {
      const std::string_view NameView(Name.data(), Name.size());
      const auto Export = COFFExportsBySymbol.find(NameView);
      Exported = Export != COFFExportsBySymbol.end();
      if (Exported && Export->second != NameView) {
        ExportName = std::string(Export->second);
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
        llvm::Expected<llvm::object::section_iterator> Target =
            InputSection.getRelocatedSection();
        if (!Target)
          return diagnosticError("cannot read relocated section: " +
                                 llvm::toString(Target.takeError()));
        llvm::StringRef TargetName;
        if (*Target != Object.section_end()) {
          EXPECTED_TRY(TargetName, takeOr(sectionName(**Target),
                                          "cannot read section name"));
        }
        if (InputRelocation.getType() != llvm::ELF::R_ARM_NONE ||
            !startsWith(TargetName, ".ARM.exidx"))
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

LinkExpect<void> ObjectReader::ReadContext::readRelocations() {
  for (const auto &InputSection : Object.sections()) {
    llvm::Expected<llvm::object::section_iterator> RelocatedSection =
        InputSection.getRelocatedSection();
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
                     Locations.findOrAdd(
                         Graph, *GraphTargetSection, 0, SectionSymbols::Include,
                         nullptr, [&] {
                           return SectionSymbolPrefix.str() +
                                  std::to_string(TargetSection->getIndex());
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
          Addend, Implicit, objectFormat(Object), *PatchSize}));
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
               ReadContext::validateInput(*Object, ExpectedTarget));
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
