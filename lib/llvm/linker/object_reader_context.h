// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/byte_io.h"
#include "linker/object_reader.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include <cstdint>
#include <map>
#include <optional>
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
namespace Reader {

struct RelocationMetadata {
  uint8_t PatchSize = BytePatch;
  bool PCRelative = false;
  bool External = false;
  bool Scattered = false;
};

struct MachOAddendPair {
  uint32_t Type;
  RelocationMetadata Metadata;
  int64_t Addend;
};

Unexpected<Diagnostic> addContext(Diagnostic Value, std::string_view Context);

llvm::Expected<llvm::StringRef>
sectionName(const llvm::object::SectionRef &Section);

bool isSectionSymbol(llvm::StringRef Name) noexcept;

LinkExpect<uint32_t> symbolFlags(const llvm::object::SymbolRef &Symbol);

std::optional<uint64_t>
sectionEntryCount(const llvm::object::SectionRef &Section, uint64_t EntrySize);

SectionPurpose sectionPurpose(const llvm::object::ObjectFile &Object,
                              llvm::StringRef Name) noexcept;

ObjectFormat objectFormat(const llvm::object::ObjectFile &Object) noexcept;

std::optional<RelocationMetadata>
relocationMetadata(const llvm::object::ObjectFile &Object,
                   const llvm::object::RelocationRef &Relocation,
                   Target TargetValue) noexcept;

template <typename T>
LinkExpect<T> takeOr(llvm::Expected<T> Value, std::string_view Context,
                     DiagnosticKind Kind = DiagnosticKind::Malformed) {
  if (!Value)
    return diagnosticError(
        std::string(Context) + ": " + llvm::toString(Value.takeError()), Kind);
  return std::move(*Value);
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

bool validELFTables(const llvm::object::ObjectFile &Object);

LinkExpect<void> associateRawX86_64MachOFDEs(LinkGraph &Graph);

LinkExpect<MachOAddendPair>
consumeAArch64MachOAddendPair(const llvm::object::ObjectFile &Object,
                              Span<const Byte> Content,
                              llvm::object::relocation_iterator &Relocation,
                              const llvm::object::relocation_iterator &End);

using SymbolPreference = bool (*)(const Symbol &Candidate,
                                  const Symbol &Current) noexcept;

inline bool preferVisibleSymbol(const Symbol &Candidate,
                                const Symbol &Current) noexcept {
  return std::tie(Candidate.Exported, Candidate.Global, Candidate.Size) >
         std::tie(Current.Exported, Current.Global, Current.Size);
}

enum class SectionSymbols : uint8_t { Include, Exclude };

class SymbolLocations {
public:
  std::optional<SymbolId> find(const LinkGraph &Graph, SectionId Section,
                               uint64_t Offset, SectionSymbols Policy,
                               SymbolPreference Prefer = nullptr) {
    const auto &Symbols = Graph.symbols();
    if (Symbols.size() < Indexed) {
      Index.clear();
      Indexed = 0;
    }
    for (; Indexed < Symbols.size(); ++Indexed)
      Index[{Symbols[Indexed].Section, Symbols[Indexed].Offset}].push_back(
          static_cast<SymbolId>(Indexed));
    const auto Found = Index.find({Section, Offset});
    if (Found == Index.end())
      return std::nullopt;
    std::optional<SymbolId> Best;
    for (const SymbolId Id : Found->second) {
      const auto &Value = Symbols[Id];
      if (Policy == SectionSymbols::Exclude && isSectionSymbol(Value.Name))
        continue;
      if (!Prefer)
        return Id;
      if (!Best || Prefer(Value, Symbols[*Best]))
        Best = Id;
    }
    return Best;
  }

  LinkExpect<SymbolId>
  findOrAdd(LinkGraph &Graph, SectionId Section, uint64_t Offset,
            SectionSymbols Policy, SymbolPreference Prefer,
            llvm::function_ref<std::string()> SyntheticName) {
    if (const auto Existing = find(Graph, Section, Offset, Policy, Prefer))
      return *Existing;
    return Graph.addSymbol(Symbol{SyntheticName(), Section, Offset, 0, false});
  }

private:
  std::map<std::pair<SectionId, uint64_t>, std::vector<SymbolId>> Index;
  size_t Indexed = 0;
};

} // namespace Reader

struct ObjectReader::ReadContext {
  using DataRef = llvm::object::DataRefImpl;

  ReadContext(const llvm::object::ObjectFile &Object, Target ActualTarget,
              ObjectReaderInputPolicy InputPolicy)
      : Object(Object), ActualTarget(ActualTarget), InputPolicy(InputPolicy),
        Graph(ActualTarget,
              Object.isLittleEndian() ? Endianness::Little : Endianness::Big,
              Reader::objectFormat(Object)) {}

  static LinkExpect<Target>
  validateInput(const llvm::object::ObjectFile &Object, Target ExpectedTarget);
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
  struct DwarfFDESection {
    SectionId Section = InvalidSectionId;
    std::map<uint64_t, bool> FDEs;
    bool Malformed = false;
  };
  struct DwarfFDECandidate {
    size_t Section;
    uint64_t Field;
  };
  struct DwarfFDEIndex {
    std::vector<DwarfFDESection> Sections;
    std::vector<DwarfFDECandidate> Candidates;
    std::map<std::pair<SectionId, uint64_t>, std::vector<size_t>> ByTarget;
    std::optional<Diagnostic> Error;
  };
  LinkExpect<void> indexDwarfFDEs(DwarfFDEIndex &Index);
  LinkExpect<SymbolId> resolveDwarfFDE(SymbolId Function, uint32_t Encoding);
  LinkExpect<void>
  readAArch64MachOEHFramePair(SectionId Section,
                              const Reader::RelocationMetadata &Metadata,
                              llvm::object::relocation_iterator &Relocation,
                              const llvm::object::relocation_iterator &End);
  LinkExpect<std::optional<int64_t>> resolveMachOSectionAddend(
      const Section &Input, const llvm::object::RelocationRef &Relocation,
      llvm::object::section_iterator TargetSection, const Section &GraphTarget,
      uint32_t Type, const Reader::RelocationMetadata &Metadata,
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
  Reader::SymbolLocations Locations;
  std::map<uint64_t, SectionId> SectionIds;
  std::map<std::string, std::string> COFFExports;
  std::optional<uint64_t> CompactUnwindSection;
  std::set<uint64_t> MachOFunctionSections;
  std::map<DataRef, SymbolId> SymbolIds;
  std::set<DataRef> IgnorableUndefinedSymbols;
  std::map<DataRef, std::string> UnwindMarkerCandidates;
  std::optional<DwarfFDEIndex> DwarfFDEs;
  std::map<SectionId, std::set<size_t>> MachOEHFrameFieldCache;
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
