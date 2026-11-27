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

struct OutputSections {
  std::vector<UniversalWasmSection> Sections;
  std::vector<Byte> PData;
};

uint64_t sectionHeaderSize(uint64_t Address, uint64_t Size,
                           uint64_t ContentSize) noexcept {
  return 1 + llvm::getULEB128Size(Address) + llvm::getULEB128Size(Size) +
         llvm::getULEB128Size(ContentSize);
}

UniversalWasmSection outputSection(AOT::SectionKind Kind, uint64_t Address,
                                   uint64_t Size, Span<const Byte> Content) {
  UniversalWasmSection Result{Kind, Address, Size, {}};
  if (Kind != AOT::SectionKind::BSS && !Content.empty())
    Result.Chunks.push_back(UniversalWasmChunk{0, Content});
  return Result;
}

LinkExpect<OutputSections> outputSections(const LinkGraph &Graph,
                                          uint64_t ImageBase) {
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
              const auto LeftKind = sectionKind(Graph.format(), *Left);
              const auto RightKind = sectionKind(Graph.format(), *Right);
              return std::tie(LeftKind, Left->Address, Left->Name) <
                     std::tie(RightKind, Right->Address, Right->Name);
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
  OutputSections Output;
  auto &Result = Output.Sections;
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
    const auto StartsSection = [&]() {
      const auto &Last = Result.back();
      if (Last.Kind != *Kind || CrossesReserved(Last, Address, RelativeEnd) ||
          StartsBetween(Last.Address + Last.Size + ImageBase, Value->Address))
        return true;
      if (Last.Kind == AOT::SectionKind::BSS || Value->Content.empty() ||
          Address < Last.Address)
        return false;
      const uint64_t Filled = Last.Address + Last.contentSize();
      return Address > Filled &&
             Address - Filled > sectionHeaderSize(Address, Value->VirtualSize,
                                                  Value->Content.size());
    };
    if (Result.empty() || StartsSection()) {
      Result.push_back(
          outputSection(*Kind, Address, Value->VirtualSize, Value->Content));
      continue;
    }
    auto &Last = Result.back();
    if (Address < Last.Address || Last.Size > Address - Last.Address)
      return writeError("section overlaps the previous output section");
    if (Last.Kind != AOT::SectionKind::BSS && !Value->Content.empty())
      Last.Chunks.push_back(
          UniversalWasmChunk{Address - Last.Address, Value->Content});
    if (Last.contentSize() > std::numeric_limits<uint32_t>::max())
      return writeError("merged section content is too large");
    Last.Size = RelativeEnd - Last.Address;
  }
  if (!Reserved.Present)
    return Output;
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
  Output.PData = std::move(*PData);
  Result.push_back(outputSection(AOT::SectionKind::Unwind, Reserved.Address,
                                 Output.PData.size(), Output.PData));
  std::sort(Result.begin(), Result.end(),
            [](const auto &Left, const auto &Right) {
              return std::tie(Left.Kind, Left.Address) <
                     std::tie(Right.Kind, Right.Address);
            });
  return Output;
}

std::optional<uint64_t> runtimeAddress(const LinkGraph &Graph,
                                       const Symbol &Value,
                                       uint64_t ImageBase) noexcept {
  const auto Address = Internal::symbolAddress(Graph, Value);
  if (!Address || *Address < ImageBase)
    return std::nullopt;
  return (*Address - ImageBase) | static_cast<uint64_t>(Value.Thumb);
}

LinkExpect<std::vector<SemanticAddress>>
semanticAddresses(const LinkGraph &Graph, uint64_t ImageBase) {
  std::vector<SemanticAddress> Result;
  for (const auto &SymbolValue : Graph.symbols()) {
    const auto Address = runtimeAddress(Graph, SymbolValue, ImageBase);
    if (!Address)
      return writeError("symbol address precedes the image base");
    if (const auto Name = Internal::semanticSymbolName(Graph, SymbolValue))
      Result.push_back({*Name, *Address});
  }
  return Result;
}

LinkExpect<std::vector<uint64_t>>
denseTable(std::vector<std::pair<uint64_t, uint64_t>> Entries,
           std::string_view Kind, bool StartsAtZero) {
  std::sort(Entries.begin(), Entries.end());
  std::vector<uint64_t> Addresses;
  Addresses.reserve(Entries.size());
  for (size_t I = 0; I < Entries.size(); ++I) {
    const uint64_t Expected =
        (StartsAtZero ? 0 : Entries.front().first) + static_cast<uint64_t>(I);
    if (I != 0 && Entries[I].first == Entries[I - 1].first)
      return writeError("duplicate " + std::string(Kind) + " index");
    if (Entries[I].first != Expected)
      return writeError("missing " + std::string(Kind) + " table entry");
    Addresses.push_back(Entries[I].second);
  }
  return Addresses;
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
    const uint64_t ContentSize = Value.contentSize();
    if (ContentSize > std::numeric_limits<uint32_t>::max())
      return Unexpect(ErrCode::Value::IllegalPath);
    EXPECTED_TRY(Output.writeByte(static_cast<uint8_t>(Value.Kind)));
    EXPECTED_TRY(Output.writeU64(Value.Address));
    EXPECTED_TRY(Output.writeU64(Value.Size));
    EXPECTED_TRY(Output.writeU32(static_cast<uint32_t>(ContentSize)));
    uint64_t Written = 0;
    for (const auto &Chunk : Value.Chunks) {
      if (Chunk.Offset < Written)
        return Unexpect(ErrCode::Value::IllegalPath);
      EXPECTED_TRY(Output.writeZeros(Chunk.Offset - Written));
      EXPECTED_TRY(Output.write(Chunk.Bytes));
      Written = Chunk.Offset + Chunk.Bytes.size();
    }
  }
  return {};
}

Expect<void> writePayload(Writer &Output, const UniversalWasmTables &Tables,
                          Span<const UniversalWasmSection> Sections) {
  EXPECTED_TRY(writeHeader(Output));
  EXPECTED_TRY(writeAddresses(Output, Tables));
  return writeSections(Output, Sections);
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
  const auto Arch = Internal::architecture(Graph.target(), Graph.elfFlags());
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
  if (Graph.format() != ObjectFormat::COFF &&
      std::count_if(Graph.sections().begin(), Graph.sections().end(),
                    [](const Section &Value) {
                      return Value.Purpose == SectionPurpose::EHFrame &&
                             Value.VirtualSize != 0;
                    }) > 1)
    return writeError("multiple eh_frame sections are not supported");
  const uint64_t ImageBase = Graph.imageBase();

  EXPECTED_TRY(const auto Symbols, semanticAddresses(Graph, ImageBase));
  EXPECTED_TRY(const auto Tables, tables(Symbols));
  EXPECTED_TRY(auto Output, outputSections(Graph, ImageBase));
  return encoded(encode(Result, Wasm, Tables, Output.Sections), "write output");
}

LinkExpect<UniversalWasmTables>
UniversalWasmWriter::tables(Span<const SemanticAddress> Symbols) {
  UniversalWasmTables Result;
  bool HasVersion = false;
  bool HasIntrinsics = false;
  std::vector<std::pair<uint64_t, uint64_t>> Types;
  std::vector<std::pair<uint64_t, uint64_t>> Codes;
  for (const auto &[Name, Address] : Symbols) {
    const auto Semantic = AOT::semanticSymbol(Name);
    if (!Semantic)
      continue;
    if (Semantic->Index > std::numeric_limits<uint32_t>::max())
      return writeError(Semantic->Kind == AOT::SemanticSymbolKind::Type
                            ? "type index out of range"
                            : "code index out of range");
    switch (Semantic->Kind) {
    case AOT::SemanticSymbolKind::Version:
      if (HasVersion)
        return writeError("duplicate version symbol");
      HasVersion = true;
      Result.Version = Address;
      break;
    case AOT::SemanticSymbolKind::Intrinsics:
      if (HasIntrinsics)
        return writeError("duplicate intrinsics symbol");
      HasIntrinsics = true;
      Result.Intrinsics = Address;
      break;
    case AOT::SemanticSymbolKind::Type:
      Types.emplace_back(Semantic->Index, Address);
      break;
    case AOT::SemanticSymbolKind::Code:
      Codes.emplace_back(Semantic->Index, Address);
      break;
    }
  }
  if (!HasVersion || !HasIntrinsics)
    return writeError("invalid semantic symbol tables version="s +
                      (HasVersion ? "true" : "false") +
                      " intrinsics=" + (HasIntrinsics ? "true" : "false"));
  EXPECTED_TRY(Result.Types, denseTable(std::move(Types), "type", true));
  EXPECTED_TRY(Result.Codes, denseTable(std::move(Codes), "code", false));
  return Result;
}

Expect<void>
UniversalWasmWriter::encode(Writer &Output, Span<const Byte> Wasm,
                            const UniversalWasmTables &Tables,
                            Span<const UniversalWasmSection> Sections) {
  uint64_t PayloadSize = 0;
  {
    Writer Counter(PayloadSize);
    EXPECTED_TRY(writePayload(Counter, Tables, Sections));
  }
  if (PayloadSize > std::numeric_limits<uint32_t>::max())
    return Unexpect(ErrCode::Value::IllegalPath);
  EXPECTED_TRY(Output.write(Wasm));
  EXPECTED_TRY(Output.writeByte(CustomSectionId));
  EXPECTED_TRY(Output.writeU32(static_cast<uint32_t>(PayloadSize)));
  return writePayload(Output, Tables, Sections);
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
