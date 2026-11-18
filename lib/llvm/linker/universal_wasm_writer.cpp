// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/universal_wasm_writer.h"

#include "aot/version.h"
#include "linker/architecture.h"
#include "linker/byte_io.h"
#include "linker/pe_writer.h"
#include "linker/writer.h"

#include <llvm/Support/LEB128.h>

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using Internal::addUnsigned;
using Internal::fitsSizeT;

constexpr uint8_t CustomSectionId = 0;
constexpr Byte LastCoreSectionId = 13;
constexpr unsigned MaxULEB32Length = 5;

bool overlaps(uint64_t LeftAddress, uint64_t LeftSize, uint64_t RightAddress,
              uint64_t RightSize) noexcept {
  if (LeftSize == 0 || RightSize == 0)
    return false;
  return LeftAddress <= RightAddress ? RightAddress - LeftAddress < LeftSize
                                     : LeftAddress - RightAddress < RightSize;
}

Unexpected<Diagnostic>
writeError(std::string_view Reason,
           DiagnosticKind Kind = DiagnosticKind::Malformed) {
  return diagnosticError("universal writer: " + std::string(Reason), Kind);
}

LinkExpect<void> encoded(Expect<void> Result, std::string_view Action) {
  if (!Result)
    return writeError("failed to " + std::string(Action), DiagnosticKind::IO);
  return {};
}

std::optional<AOT::SectionKind> sectionKind(ObjectFormat Format,
                                            const Section &Value) noexcept {
  if (Value.Purpose == SectionPurpose::PData)
    return AOT::SectionKind::Unwind;
  if (Value.Purpose == SectionPurpose::EHFrame)
    return Format == ObjectFormat::COFF ? AOT::SectionKind::Data
                                        : AOT::SectionKind::Unwind;
  if (Value.Purpose == SectionPurpose::ARMExidx)
    return AOT::SectionKind::Data;
  switch (Value.Kind) {
  case SectionKind::Text:
    return AOT::SectionKind::Text;
  case SectionKind::ReadOnly:
  case SectionKind::Data:
  case SectionKind::Unwind:
    return AOT::SectionKind::Data;
  case SectionKind::BSS:
    return AOT::SectionKind::BSS;
  }
  return std::nullopt;
}

struct PDataReservation {
  std::vector<std::pair<uint64_t, uint64_t>> Ranges;
  uint64_t Address = UINT64_MAX;
  uint64_t End = 0;
  bool Present = false;
};

LinkExpect<PDataReservation> reservePData(const LinkGraph &Graph,
                                          uint64_t ImageBase) {
  PDataReservation Result;
  for (const auto &Value : Graph.sections()) {
    if (Value.Purpose != SectionPurpose::PData)
      continue;
    uint64_t AbsoluteEnd = 0;
    if (Value.Address < ImageBase ||
        !addUnsigned(Value.Address, Value.VirtualSize, AbsoluteEnd))
      return writeError("PE runtime function table address overflow");
    const uint64_t RelativeAddress = Value.Address - ImageBase;
    if (Value.VirtualSize != 0)
      Result.Ranges.emplace_back(RelativeAddress, Value.VirtualSize);
    Result.Address = std::min(Result.Address, RelativeAddress);
    Result.End = std::max(Result.End, AbsoluteEnd - ImageBase);
    Result.Present = true;
  }
  return Result;
}

LinkExpect<std::vector<UniversalWasmSection>>
outputSections(const LinkGraph &Graph, uint64_t ImageBase) {
  std::vector<const Section *> Ordered;
  for (const auto &Value : Graph.sections()) {
    if (Value.VirtualSize != 0 &&
        Value.Purpose != SectionPurpose::CompactUnwind &&
        Value.Purpose != SectionPurpose::PData) {
      Ordered.push_back(&Value);
    }
  }
  std::sort(Ordered.begin(), Ordered.end(),
            [&Graph](const auto *Left, const auto *Right) {
              return std::tuple(sectionKind(Graph.format(), *Left),
                                Left->Address, Left->Name) <
                     std::tuple(sectionKind(Graph.format(), *Right),
                                Right->Address, Right->Name);
            });
  std::vector<uint64_t> Starts;
  Starts.reserve(Ordered.size());
  for (const auto *Value : Ordered) {
    Starts.push_back(Value->Address);
  }
  std::sort(Starts.begin(), Starts.end());
  const auto StartsBetween = [&Starts](uint64_t Begin, uint64_t End) {
    const auto First = std::lower_bound(Starts.begin(), Starts.end(), Begin);
    return First != Starts.end() && *First < End;
  };
  EXPECTED_TRY(auto Reserved, reservePData(Graph, ImageBase));
  const auto CrossesReserved = [&Reserved](const UniversalWasmSection &Output,
                                           uint64_t Address,
                                           uint64_t RelativeEnd) {
    if (Address < Output.Address || RelativeEnd < Output.Address)
      return false;
    const uint64_t Extent = RelativeEnd - Output.Address;
    return std::any_of(
        Reserved.Ranges.begin(), Reserved.Ranges.end(), [&](const auto &Value) {
          return overlaps(Value.first, Value.second, Output.Address, Extent);
        });
  };
  std::vector<UniversalWasmSection> Result;
  for (const auto *Value : Ordered) {
    const auto Kind = sectionKind(Graph.format(), *Value);
    if (!Kind)
      return writeError("unsupported section kind");
    if (Value->Content.size() > Value->VirtualSize)
      return writeError("section content exceeds its virtual size");
    uint64_t End = 0;
    if (Value->Address < ImageBase ||
        !addUnsigned(Value->Address, Value->VirtualSize, End))
      return writeError("section address overflow");
    const uint64_t Address = Value->Address - ImageBase;
    const uint64_t RelativeEnd = End - ImageBase;
    if (Result.empty() || Result.back().Kind != *Kind ||
        CrossesReserved(Result.back(), Address, RelativeEnd) ||
        StartsBetween(Result.back().Address + Result.back().Size + ImageBase,
                      Value->Address)) {
      Result.push_back(UniversalWasmSection{*Kind, Address, Value->VirtualSize,
                                            Value->Content});
      continue;
    }
    auto &Output = Result.back();
    if (Address < Output.Address || Output.Size > Address - Output.Address)
      return writeError("section overlaps the previous output section");
    const uint64_t Gap = Address - Output.Address;
    const uint64_t Size = RelativeEnd - Output.Address;
    if (!fitsSizeT(Gap) || !fitsSizeT(Size))
      return writeError("section gap or size is too large");
    if (Output.Kind != AOT::SectionKind::BSS) {
      if (Gap > Output.Content.max_size() ||
          Value->Content.size() >
              Output.Content.max_size() - static_cast<size_t>(Gap))
        return writeError("merged section content is too large");
      Output.Content.resize(static_cast<size_t>(Gap));
      Output.Content.insert(Output.Content.end(), Value->Content.begin(),
                            Value->Content.end());
    }
    Output.Size = Size;
  }
  if (!Reserved.Present)
    return Result;
  if (Graph.format() != ObjectFormat::COFF)
    return writeError("PE runtime function table outside a COFF object");
  auto PData = normalizePERuntimeFunctions(Graph, ImageBase);
  if (!PData)
    return writeError("PE runtime function table: " + PData.error().Message);
  uint64_t End = 0;
  if (!addUnsigned(Reserved.Address, PData->size(), End) || End > Reserved.End)
    return writeError("PE runtime function table exceeds its reservation");
  if (std::any_of(Result.begin(), Result.end(), [&](const auto &Value) {
        return overlaps(Reserved.Address, PData->size(), Value.Address,
                        Value.Size);
      }))
    return writeError("PE runtime function table overlaps an output section");
  Result.push_back(UniversalWasmSection{AOT::SectionKind::Unwind,
                                        Reserved.Address, PData->size(),
                                        std::move(*PData)});
  std::sort(Result.begin(), Result.end(),
            [](const auto &Left, const auto &Right) {
              return std::tie(Left.Kind, Left.Address) <
                     std::tie(Right.Kind, Right.Address);
            });
  return Result;
}

std::optional<uint64_t> runtimeAddress(const LinkGraph &Graph,
                                       const Symbol &Value,
                                       uint64_t ImageBase) noexcept {
  const auto Address = Internal::symbolAddress(Graph, Value);
  if (!Address || *Address < ImageBase)
    return std::nullopt;
  return (*Address - ImageBase) | static_cast<uint64_t>(Value.Thumb);
}

struct IndexedTable {
  std::vector<uint64_t> Addresses;
  std::vector<bool> Present;
};

struct SemanticTables {
  uint64_t Version = 0;
  uint64_t Intrinsics = 0;
  IndexedTable Types;
  IndexedTable Codes;
};

LinkExpect<SemanticTables> semanticTables(const LinkGraph &Graph,
                                          uint64_t ImageBase) {
  SemanticTables Result;
  uint64_t FirstCode = std::numeric_limits<uint64_t>::max();
  bool HasVersion = false;
  bool HasIntrinsics = false;
  auto Insert = [](IndexedTable &Table, uint64_t Index, uint64_t Address,
                   std::string_view Kind) -> LinkExpect<void> {
    if (Index == std::numeric_limits<uint64_t>::max())
      return writeError(std::string(Kind) + " index out of range");
    const uint64_t NextSize = Index + 1;
    if (!fitsSizeT(NextSize) || NextSize > Table.Addresses.max_size() ||
        NextSize > Table.Present.max_size())
      return writeError(std::string(Kind) + " table is too large");
    if (Table.Present.size() > Index && Table.Present[Index])
      return writeError("duplicate " + std::string(Kind) + " index");
    Table.Addresses.resize(
        std::max(Table.Addresses.size(), static_cast<size_t>(NextSize)));
    Table.Present.resize(Table.Addresses.size());
    Table.Addresses[Index] = Address;
    Table.Present[Index] = true;
    return {};
  };
  for (const auto &SymbolValue : Graph.symbols()) {
    const auto Address = runtimeAddress(Graph, SymbolValue, ImageBase);
    if (!Address)
      return writeError("symbol address precedes the image base");
    const auto Name = Internal::semanticSymbolName(Graph, SymbolValue);
    if (!Name)
      continue;
    const auto Semantic = AOT::semanticSymbol(*Name);
    if (!Semantic)
      continue;
    switch (Semantic->Kind) {
    case AOT::SemanticSymbolKind::Version:
      if (HasVersion)
        return writeError("duplicate version symbol");
      HasVersion = true;
      Result.Version = *Address;
      break;
    case AOT::SemanticSymbolKind::Intrinsics:
      if (HasIntrinsics)
        return writeError("duplicate intrinsics symbol");
      HasIntrinsics = true;
      Result.Intrinsics = *Address;
      break;
    case AOT::SemanticSymbolKind::Type:
      EXPECTED_TRY(Insert(Result.Types, Semantic->Index, *Address, "type"));
      break;
    case AOT::SemanticSymbolKind::Code:
      EXPECTED_TRY(Insert(Result.Codes, Semantic->Index, *Address, "code"));
      FirstCode = std::min(FirstCode, Semantic->Index);
      break;
    }
  }
  auto &Types = Result.Types;
  auto &Codes = Result.Codes;
  if (!HasVersion || !HasIntrinsics ||
      std::find(Types.Present.begin(), Types.Present.end(), false) !=
          Types.Present.end())
    return writeError("invalid semantic symbol tables version="s +
                      (HasVersion ? "true" : "false") +
                      " intrinsics=" + (HasIntrinsics ? "true" : "false") +
                      " types=" + std::to_string(Types.Addresses.size()) +
                      " codes=" + std::to_string(Codes.Addresses.size()));
  if (FirstCode != std::numeric_limits<uint64_t>::max()) {
    Codes.Addresses.erase(
        Codes.Addresses.begin(),
        Codes.Addresses.begin() +
            static_cast<std::vector<uint64_t>::difference_type>(FirstCode));
    Codes.Present.erase(
        Codes.Present.begin(),
        Codes.Present.begin() +
            static_cast<std::vector<bool>::difference_type>(FirstCode));
  }
  if (std::find(Codes.Present.begin(), Codes.Present.end(), false) !=
      Codes.Present.end())
    return writeError("missing code table entry");
  return Result;
}

Expect<void> writeAddresses(Writer &Output, const UniversalWasmTables &Tables) {
  EXPECTED_TRY(Output.writeU64(Tables.Version));
  EXPECTED_TRY(Output.writeU64(Tables.Intrinsics));
  for (const auto *Table : {&Tables.Types, &Tables.Codes}) {
    EXPECTED_TRY(Output.writeU64(Table->size()));
    for (const auto Address : *Table)
      EXPECTED_TRY(Output.writeU64(Address));
  }
  return {};
}

Expect<void> writeHeader(Writer &Output) {
  EXPECTED_TRY(Output.writeName("wasmedge"sv));
  EXPECTED_TRY(Output.writeU32(AOT::kBinaryVersion));
  EXPECTED_TRY(Output.writeByte(static_cast<uint8_t>(AOT::kHostOSType)));
  return Output.writeByte(static_cast<uint8_t>(AOT::kHostArchitecture));
}

Expect<void> writeSections(Writer &Output,
                           Span<const UniversalWasmSection> Sections) {
  if (Sections.size() > std::numeric_limits<uint32_t>::max())
    return Unexpect(ErrCode::Value::IllegalPath);
  EXPECTED_TRY(Output.writeU32(static_cast<uint32_t>(Sections.size())));
  for (const auto &Value : Sections) {
    EXPECTED_TRY(Output.writeByte(static_cast<uint8_t>(Value.Kind)));
    EXPECTED_TRY(Output.writeU64(Value.Address));
    EXPECTED_TRY(Output.writeU64(Value.Size));
    EXPECTED_TRY(Output.writeLengthPrefixed(Value.Content));
  }
  return {};
}

Expect<void> writeContainer(Writer &Output, Span<const Byte> Wasm,
                            Span<const Byte> Payload) {
  EXPECTED_TRY(Output.write(Wasm));
  EXPECTED_TRY(Output.writeByte(CustomSectionId));
  EXPECTED_TRY(Output.writeU32(static_cast<uint32_t>(Payload.size())));
  EXPECTED_TRY(Output.write(Payload));
  return Output.close();
}

std::string_view objectFormatName(ObjectFormat Format) noexcept {
  switch (Format) {
  case ObjectFormat::ELF:
    return "ELF"sv;
  case ObjectFormat::MachO:
    return "MachO"sv;
  case ObjectFormat::COFF:
    return "COFF"sv;
  }
  return "unknown"sv;
}

Unexpected<Diagnostic> unsupportedRebase(const LinkGraph &Graph,
                                         const Rebase &Value) {
  Diagnostic Diag("universal writer: rebase unsupported width=" +
                  std::to_string(Value.Width) +
                  " format=" + std::string(objectFormatName(Value.Format)));
  Diag.Section = Value.Section;
  Diag.Offset = Value.Offset;
  Diag.RelocationType = Value.Type;
  Diag.SectionName = Value.Section < Graph.sections().size()
                         ? Graph.sections()[Value.Section].Name
                         : "<invalid>";
  return diagnosticError(std::move(Diag));
}

LinkExpect<void> validateWasmSections(Span<const Byte> Wasm) {
  constexpr std::array<Byte, 8> WasmHeader{0x00, 0x61, 0x73, 0x6D,
                                           0x01, 0x00, 0x00, 0x00};
  if (Wasm.size() < WasmHeader.size() ||
      !std::equal(WasmHeader.begin(), WasmHeader.end(), Wasm.begin()))
    return writeError("invalid wasm header");
  size_t Offset = WasmHeader.size();
  while (Offset < Wasm.size()) {
    const Byte SectionId = Wasm[Offset++];
    if (SectionId > LastCoreSectionId)
      return writeError("invalid wasm section id");
    unsigned Length = 0;
    const char *Error = nullptr;
    const uint64_t Size = llvm::decodeULEB128(
        Wasm.data() + Offset, &Length, Wasm.data() + Wasm.size(), &Error);
    if (Error || Length > MaxULEB32Length ||
        Size > std::numeric_limits<uint32_t>::max())
      return writeError("malformed wasm section size");
    Offset += Length;
    if (Offset > Wasm.size() || Size > Wasm.size() - Offset)
      return writeError("wasm section extends beyond the input");
    Offset += Size;
  }
  return {};
}

} // namespace

LinkExpect<void> UniversalWasmWriter::write(const LinkGraph &Graph,
                                            Span<const Byte> Wasm,
                                            Writer &Result) {
  EXPECTED_TRY(validateWasmSections(Wasm));
  const auto Arch = Internal::architecture(Graph.target());
  if (AOT::kHostOSType == AOT::OSType::Unsupported)
    return writeError("unsupported host operating system");
  if (!Arch)
    return writeError("unsupported object architecture");
  if (*Arch != AOT::kHostArchitecture)
    return writeError("object architecture does not match the host");
  if (!Graph.relocationsApplied())
    return writeError("relocations have not been applied");
  if (!Graph.rebases().empty())
    return unsupportedRebase(Graph, Graph.rebases().front());
  const uint64_t ImageBase = Graph.imageBase();

  EXPECTED_TRY(auto Tables, semanticTables(Graph, ImageBase));
  EXPECTED_TRY(auto Sections, outputSections(Graph, ImageBase));
  return encoded(encode(Result, Wasm,
                        UniversalWasmTables{Tables.Version, Tables.Intrinsics,
                                            std::move(Tables.Types.Addresses),
                                            std::move(Tables.Codes.Addresses)},
                        Sections),
                 "write output");
}

Expect<void>
UniversalWasmWriter::encode(Writer &Output, Span<const Byte> Wasm,
                            const UniversalWasmTables &Tables,
                            Span<const UniversalWasmSection> Sections) {
  std::vector<Byte> Payload;
  Writer Section(Payload);
  EXPECTED_TRY(writeHeader(Section));
  EXPECTED_TRY(writeAddresses(Section, Tables));
  EXPECTED_TRY(writeSections(Section, Sections));
  if (Payload.size() > std::numeric_limits<uint32_t>::max())
    return Unexpect(ErrCode::Value::IllegalPath);
  return writeContainer(Output, Wasm, Payload);
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
