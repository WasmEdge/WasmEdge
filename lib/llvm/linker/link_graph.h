// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/expected.h"
#include "common/span.h"
#include "common/types.h"

namespace WasmEdge {
namespace LLVM {
namespace Linker {

enum class Target : uint8_t { X86_64, ARM, AArch64, RISCV64, S390X };
enum class Endianness : uint8_t { Little, Big };
enum class SectionKind : uint8_t { Text, ReadOnly, Data, BSS, Unwind };
enum class SectionPurpose : uint8_t {
  Default,
  EHFrame,
  PData,
  XData,
  CompactUnwind,
  UnwindInfo,
  ARMExidx,
};
enum class ObjectFormat : uint8_t { ELF, MachO, COFF };
enum class MachOUnwindInfoState : uint8_t { None, Reserved, Populated };

using SectionId = uint32_t;
using SymbolId = uint32_t;
inline constexpr SectionId InvalidSectionId = UINT32_MAX;
inline constexpr SymbolId InvalidSymbolId = UINT32_MAX;
inline constexpr uint8_t NoPatch = 0;
inline constexpr uint8_t MinimumRebaseWidth = 1;

struct Section {
  std::string Name;
  SectionKind Kind;
  uint64_t Alignment;
  uint64_t VirtualSize = 0;
  uint64_t Address = 0;
  uint64_t FileOffset = 0;
  std::vector<Byte> Content{};
  SectionPurpose Purpose = SectionPurpose::Default;
  uint64_t InputAddress = 0;
  std::optional<SectionId> LinkedSection = std::nullopt;
};

struct EHFrameReference {
  SectionId Section;
  uint64_t Offset;
  SymbolId Symbol;
};

struct CompactUnwindRecord {
  SymbolId Function;
  uint32_t Length;
  uint32_t Encoding;
  std::optional<SymbolId> Personality;
  std::optional<SymbolId> LSDA;
  std::optional<SymbolId> FDE;
};

struct MachOBuildToolVersion {
  uint32_t Tool;
  uint32_t Version;
};

struct MachOBuildVersion {
  uint32_t Platform;
  uint32_t MinimumOS;
  uint32_t SDK;
  std::vector<MachOBuildToolVersion> Tools;
};

struct Symbol {
  std::string Name;
  SectionId Section;
  uint64_t Offset;
  uint64_t Size;
  bool Exported;
  std::optional<std::string> ExportName = std::nullopt;
  bool Global = false;
  bool Thumb = false;
};

struct Relocation {
  SectionId Section;
  uint64_t Offset;
  uint32_t Type;
  SymbolId Symbol;
  int64_t Addend;
  bool AddendIsImplicit = false;
  ObjectFormat Format = ObjectFormat::ELF;
  uint8_t PatchSize = NoPatch;
  bool PCRelative = false;
  bool External = false;
  bool Scattered = false;
};

struct Rebase {
  SectionId Section;
  uint64_t Offset;
  uint32_t Type;
  int64_t Addend;
  uint8_t Width = NoPatch;
  ObjectFormat Format = ObjectFormat::ELF;
};

enum class DiagnosticKind : uint8_t { Malformed, Unsupported, IO };

struct Diagnostic {
  explicit Diagnostic(std::string Value) : Message(std::move(Value)) {}

  std::string Message;
  std::optional<SectionId> Section;
  std::optional<SymbolId> Symbol;
  std::optional<uint32_t> RelocationType;
  std::optional<uint64_t> Offset;
  std::string SectionName;
  std::string SymbolName;
  DiagnosticKind Kind = DiagnosticKind::Malformed;
};

template <typename T> using LinkExpect = Expected<T, Diagnostic>;

std::optional<uint8_t> relocationPatchSize(ObjectFormat Format,
                                           Target TargetValue, uint32_t Type,
                                           uint8_t MetadataSize) noexcept;
bool relocationIsPCRelative(ObjectFormat Format, Target TargetValue,
                            uint32_t Type) noexcept;

class ObjectReader;
namespace Internal {
struct EHFrameCommitTestAccess;
}

class LinkGraph {
public:
  LinkGraph(Target TargetValue, Endianness EndianValue,
            ObjectFormat FormatValue = ObjectFormat::ELF) noexcept
      : TargetValue(TargetValue), EndianValue(EndianValue),
        FormatValue(FormatValue) {}
  LinkGraph(const LinkGraph &) = default;
  LinkGraph &operator=(const LinkGraph &Other);
  LinkGraph(LinkGraph &&Other) noexcept;
  LinkGraph &operator=(LinkGraph &&Other) noexcept;
  void swap(LinkGraph &Other) noexcept;

  LinkExpect<void> beginInput(std::string_view Name);
  LinkExpect<SectionId> addSection(Section Value);
  LinkExpect<SymbolId> addSymbol(Symbol Value);
  LinkExpect<void> addRelocation(Relocation Value);
  LinkExpect<void> addRebase(Rebase Value);
  LinkExpect<void> addEHFrameReference(EHFrameReference Value);
  LinkExpect<void> addCompactUnwind(CompactUnwindRecord Value);
  LinkExpect<void> addMachOBuildVersion(MachOBuildVersion Value);
  LinkExpect<void> pruneUnreferencedMachOEHFrame();
  LinkExpect<void> validate() const;
  LinkExpect<void> setSectionAddress(SectionId Id, uint64_t Address);
  LinkExpect<void> setSectionFileOffset(SectionId Id, uint64_t FileOffset);
  LinkExpect<void> setLinkedSection(SectionId Id, SectionId Linked);
  LinkExpect<void> setELFFlags(uint32_t Flags);
  Target target() const noexcept { return TargetValue; }
  Endianness endianness() const noexcept { return EndianValue; }
  ObjectFormat format() const noexcept { return FormatValue; }
  uint32_t elfFlags() const noexcept { return ELFFlags; }
  const std::vector<Section> &sections() const noexcept { return Sections; }
  const std::vector<Symbol> &symbols() const noexcept { return Symbols; }
  const std::vector<Relocation> &relocations() const noexcept {
    return Relocations;
  }
  const std::vector<Rebase> &rebases() const noexcept { return Rebases; }
  const std::vector<EHFrameReference> &ehFrameReferences() const noexcept {
    return EHFrameReferences;
  }
  const std::vector<CompactUnwindRecord> &compactUnwind() const noexcept {
    return CompactUnwind;
  }
  const std::vector<MachOBuildVersion> &machOBuildVersions() const noexcept {
    return MachOBuildVersions;
  }
  bool relocationsApplied() const noexcept { return RelocationsApplied; }
  MachOUnwindInfoState machOUnwindInfoState() const noexcept {
    return UnwindInfoState;
  }

private:
  struct RelocationIntervalGroup {
    std::array<size_t, 2> Indices{};
    size_t Count = 0;
  };
  using RelocationIntervalMap =
      std::map<std::pair<SectionId, uint64_t>, RelocationIntervalGroup>;
  using RebaseIntervalMap = std::map<std::pair<SectionId, uint64_t>, size_t>;

  Target TargetValue;
  Endianness EndianValue;
  ObjectFormat FormatValue;
  uint32_t ELFFlags = 0;
  std::optional<std::string> InputName;
  std::vector<Section> Sections;
  std::vector<Symbol> Symbols;
  std::unordered_map<std::string, SymbolId> SymbolNames;
  std::vector<Relocation> Relocations;
  std::vector<Rebase> Rebases;
  RelocationIntervalMap RelocationIntervals;
  RebaseIntervalMap RebaseIntervals;
  std::vector<EHFrameReference> EHFrameReferences;
  std::vector<CompactUnwindRecord> CompactUnwind;
  std::vector<MachOBuildVersion> MachOBuildVersions;
  MachOUnwindInfoState UnwindInfoState = MachOUnwindInfoState::None;
  bool RelocationsApplied = false;

  friend LinkExpect<void> applyRelocations(LinkGraph &);
  friend class ObjectReader;
  friend Expect<void> compactUnwindToEHFrame(LinkGraph &);
  friend Expect<void> reserveMachOUnwindInfo(LinkGraph &);
  friend Expect<void> populateMachOUnwindInfo(LinkGraph &, uint64_t);
  friend Expect<void> normalizeMachOEHFrame(LinkGraph &);
  friend struct Internal::EHFrameCommitTestAccess;
  std::optional<SymbolId> findSymbol(const std::string &Name) const;
  static LinkExpect<void>
  buildPatchIntervals(Span<const Relocation> RelocationValues,
                      Span<const Rebase> RebaseValues,
                      RelocationIntervalMap &NewRelocationIntervals,
                      RebaseIntervalMap &NewRebaseIntervals);
  LinkExpect<void> rebuildPatchIntervals();
  LinkExpect<void> commitNormalizedEHFrame(
      std::vector<std::pair<SectionId, std::vector<Byte>>> Content,
      Span<const uint8_t> RemoveRelocations);
  LinkExpect<void>
  addSynthesizedEHFrame(std::optional<SectionId> Existing, Section Value,
                        std::vector<EHFrameReference> References);
  LinkExpect<void> reserveMachOUnwindInfoSection(Section Value);
  LinkExpect<void> populateMachOUnwindInfoSection(SectionId Id,
                                                  std::vector<Byte> Content);
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
