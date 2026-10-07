// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/eh_frame.h"

#include "linker/byte_io.h"

#include <algorithm>
#include <charconv>
#include <map>
#include <set>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using Internal::addSigned;
using Internal::addUnsigned;
using Internal::EHFrameLengthFieldSize;
using Internal::EHFramePointerWidth;
using Internal::EHFrameRecordHeaderSize;
using Internal::signedDelta;
using Internal::validField;

auto fail() noexcept { return Unexpect(ErrCode::Value::IllegalPath); }

bool readU32(Span<const Byte> Bytes, size_t Offset, uint32_t &Value) noexcept {
  const auto Read =
      Internal::readUnsigned(Bytes, Offset, sizeof(Value), Endianness::Little);
  if (!Read)
    return false;
  Value = static_cast<uint32_t>(*Read);
  return true;
}

bool readU64(Span<const Byte> Bytes, size_t Offset, uint64_t &Value) noexcept {
  const auto Read =
      Internal::readUnsigned(Bytes, Offset, sizeof(Value), Endianness::Little);
  if (!Read)
    return false;
  Value = *Read;
  return true;
}

bool addAddressDelta(uint64_t First, uint64_t Second, uint64_t Third,
                     int64_t Delta, uint64_t &Result) noexcept {
  if (Delta < 0) {
    uint64_t Magnitude = static_cast<uint64_t>(-(Delta + 1)) + 1;
    const uint64_t FromThird = std::min(Third, Magnitude);
    Third -= FromThird;
    Magnitude -= FromThird;
    const uint64_t FromSecond = std::min(Second, Magnitude);
    Second -= FromSecond;
    Magnitude -= FromSecond;
    const uint64_t FromFirst = std::min(First, Magnitude);
    First -= FromFirst;
    Magnitude -= FromFirst;
    if (Magnitude != 0)
      return false;
    Delta = 0;
  }
  return addUnsigned(First, Second, Result) &&
         addUnsigned(Result, Third, Result) && addSigned(Result, Delta, Result);
}

bool readSLEB(Span<const Byte> Bytes, size_t &Offset, int64_t &Value) noexcept {
  constexpr size_t MaxBytes = 10;
  uint64_t Raw = 0;
  for (size_t I = 0; I < MaxBytes; ++I) {
    if (Offset >= Bytes.size())
      return false;
    const uint8_t Byte = Bytes[Offset++];
    const uint8_t Payload = Byte & 0x7F;
    if (I == MaxBytes - 1) {
      if ((Byte & 0x80) != 0 || (Payload != 0x00 && Payload != 0x7F))
        return false;
      const bool Bit62 = (Raw & (UINT64_C(1) << 62)) != 0;
      if ((Payload == 0x00) != Bit62)
        return false;
      if (Payload == 0x7F)
        Raw |= UINT64_C(1) << 63;
      Value = static_cast<int64_t>(Raw);
      return true;
    }
    const unsigned Shift = static_cast<unsigned>(I * 7);
    Raw |= static_cast<uint64_t>(Payload) << Shift;
    if ((Byte & 0x80) == 0) {
      const unsigned UsedBits = Shift + 7;
      if ((Byte & 0x40) != 0)
        Raw |= UINT64_MAX << UsedBits;
      Value = static_cast<int64_t>(Raw);
      return true;
    }
  }
  return false;
}

bool requiredSemanticFunction(const LinkGraph &Graph,
                              const Symbol &Symbol) noexcept {
  if (!Symbol.Global && !Symbol.Exported)
    return false;
  std::string_view Name = Symbol.exportedName();
  if (Graph.format() == ObjectFormat::MachO && !Name.empty() &&
      Name.front() == '_')
    Name.remove_prefix(1);
  if (Name.size() < 2 || (Name.front() != 't' && Name.front() != 'f'))
    return false;
  uint64_t Index = 0;
  const auto Parsed =
      std::from_chars(Name.data() + 1, Name.data() + Name.size(), Index);
  return Parsed.ec == std::errc{} && Parsed.ptr == Name.data() + Name.size() &&
         std::to_string(Index) == Name.substr(1);
}

bool expectByte(Span<const Byte> Bytes, size_t &Cursor,
                Byte Expected) noexcept {
  return Cursor < Bytes.size() && Bytes[Cursor++] == Expected;
}

// Returns the end offset of the record at Offset, or std::nullopt when it is
// the zero terminator followed only by zero padding.
Expect<std::optional<size_t>> recordEnd(Span<const Byte> Bytes,
                                        size_t Offset) noexcept {
  constexpr uint32_t Dwarf64Marker = UINT32_MAX;
  uint32_t Length = 0;
  if (!readU32(Bytes, Offset, Length))
    return fail();
  const size_t Body = Offset + EHFrameLengthFieldSize;
  if (Length == 0) {
    if (std::any_of(Bytes.begin() + Body, Bytes.end(),
                    [](Byte Value) { return Value != 0; }))
      return fail();
    return std::optional<size_t>{};
  }
  if (Length == Dwarf64Marker || Length > Bytes.size() - Body)
    return fail();
  return std::optional<size_t>{Body + Length};
}

Expect<void> parseCIE(Span<const Byte> Record, size_t Cursor,
                      Target Architecture) {
  if (!expectByte(Record, Cursor, Internal::EHFrameCIEVersion))
    return fail();
  for (const char Value : Internal::EHFrameCIEAugmentation) {
    if (!expectByte(Record, Cursor, static_cast<Byte>(Value)))
      return fail();
  }
  if (!expectByte(Record, Cursor, 0))
    return fail();
  uint64_t CodeAlignment = 0;
  int64_t DataAlignment = 0;
  uint64_t ReturnAddressRegister = 0;
  uint64_t AugmentationLength = 0;
  if (!Internal::readULEB128(Record, Cursor, CodeAlignment) ||
      !readSLEB(Record, Cursor, DataAlignment) ||
      !Internal::readULEB128(Record, Cursor, ReturnAddressRegister) ||
      !Internal::readULEB128(Record, Cursor, AugmentationLength))
    return fail();
  const bool ValidCodeAlignment =
      CodeAlignment == Internal::EHFrameCIECodeAlignment ||
      (Architecture == Target::AArch64 &&
       CodeAlignment == Internal::EHFrameAArch64CIECodeAlignment);
  if (!ValidCodeAlignment || DataAlignment >= 0)
    return fail();
  if (AugmentationLength != Internal::EHFrameCIEAugmentationLength ||
      !expectByte(Record, Cursor, Internal::EHFrameFDEPointerEncoding))
    return fail();
  return {};
}

Expect<void> parseFDE(Span<const Byte> Record, size_t PCBegin) {
  const size_t PCRange = PCBegin + EHFramePointerWidth;
  if (!validField(Record, PCBegin, EHFramePointerWidth) ||
      !validField(Record, PCRange, EHFramePointerWidth))
    return fail();
  size_t Cursor = PCRange + EHFramePointerWidth;
  uint64_t AugmentationLength = 0;
  if (!Internal::readULEB128(Record, Cursor, AugmentationLength) ||
      AugmentationLength != 0)
    return fail();
  return {};
}

Expect<std::set<size_t>> parse(Span<const Byte> Bytes, Target Architecture) {
  std::set<size_t> FDEFields;
  std::set<size_t> CIEs;
  size_t Offset = 0;
  bool Terminated = false;
  while (Offset < Bytes.size()) {
    EXPECTED_TRY(const auto End, recordEnd(Bytes, Offset));
    if (!End) {
      Terminated = true;
      break;
    }
    const auto Record = Bytes.first(*End);
    const size_t IdOffset = Offset + EHFrameLengthFieldSize;
    const size_t BodyOffset = Offset + EHFrameRecordHeaderSize;
    uint32_t Id = 0;
    if (!readU32(Record, IdOffset, Id))
      return fail();
    if (Id == 0) {
      EXPECTED_TRY(parseCIE(Record, BodyOffset, Architecture));
      CIEs.insert(Offset);
    } else {
      if (Id > IdOffset || CIEs.count(IdOffset - Id) == 0)
        return fail();
      EXPECTED_TRY(parseFDE(Record, BodyOffset));
      FDEFields.insert(BodyOffset);
    }
    Offset = *End;
  }
  if (!Terminated || CIEs.empty() || FDEFields.empty())
    return fail();
  return FDEFields;
}

} // namespace

bool Internal::readULEB128(Span<const Byte> Bytes, size_t &Offset,
                           uint64_t &Value) noexcept {
  Value = 0;
  unsigned Shift = 0;
  while (Offset < Bytes.size() && Shift < 64) {
    const uint8_t Byte = Bytes[Offset++];
    if (Shift == 63 && (Byte & 0xFE) != 0)
      return false;
    Value |= static_cast<uint64_t>(Byte & 0x7F) << Shift;
    if ((Byte & 0x80) == 0)
      return true;
    Shift += 7;
  }
  return false;
}

namespace Internal {

std::optional<size_t> ehFrameTerminatorOffset(Span<const Byte> Bytes) {
  size_t Offset = 0;
  while (Offset < Bytes.size()) {
    const auto End = recordEnd(Bytes, Offset);
    if (!End)
      return std::nullopt;
    if (!*End)
      return Offset;
    Offset = **End;
  }
  return std::nullopt;
}

Expect<int64_t> decodeSLEB128(Span<const Byte> Bytes) {
  size_t Offset = 0;
  int64_t Value = 0;
  if (!readSLEB(Bytes, Offset, Value) || Offset != Bytes.size())
    return fail();
  return Value;
}

Expect<uint64_t> resolveMachOFDEAddress(uint64_t LoadBase,
                                        uint64_t SectionAddress, uint64_t Field,
                                        int64_t Delta) {
  uint64_t Value = 0;
  if (!addAddressDelta(LoadBase, SectionAddress, Field, Delta, Value))
    return fail();
  return Value;
}

} // namespace Internal

Expect<std::set<size_t>> machOEHFrameFields(Span<const Byte> Bytes,
                                            Target Architecture) {
  return parse(Bytes, Architecture);
}

Expect<void> normalizeMachOEHFrame(LinkGraph &Graph) {
  if (Graph.format() != ObjectFormat::MachO)
    return {};
  const auto &Sections = Graph.sections();
  const auto &Relocations = Graph.relocations();
  std::vector<std::pair<SectionId, std::vector<Byte>>> Content;
  std::vector<std::set<size_t>> Fields(Sections.size());
  for (SectionId I = 0; I < Sections.size(); ++I) {
    if (Sections[I].Purpose != SectionPurpose::EHFrame)
      continue;
    Content.emplace_back(I, Sections[I].Content);
    EXPECTED_TRY(Fields[I], parse(Content.back().second, Graph.target()));
  }
  std::set<std::pair<SectionId, uint64_t>> ReferenceFields;
  for (const auto &Reference : Graph.ehFrameReferences()) {
    if (Reference.Section >= Sections.size() ||
        Sections[Reference.Section].Purpose != SectionPurpose::EHFrame ||
        Reference.Symbol >= Graph.symbols().size() ||
        Graph.symbols()[Reference.Symbol].Section >= Sections.size() ||
        Fields[Reference.Section].count(
            static_cast<size_t>(Reference.Offset)) == 0 ||
        !ReferenceFields.emplace(Reference.Section, Reference.Offset).second)
      return fail();
  }
  std::vector<uint8_t> Remove(Relocations.size());
  for (auto &[I, Bytes] : Content) {
    const auto &FDEFields = Fields[I];
    struct TargetReference {
      SymbolId Symbol;
      int64_t Addend;
    };
    std::map<size_t, TargetReference> References;
    for (const auto &Entry : Graph.ehFrameReferences()) {
      if (Entry.Section != I)
        continue;
      References.emplace(Entry.Offset, TargetReference{Entry.Symbol, 0});
    }
    for (size_t J = 0; J < Relocations.size(); ++J) {
      const auto &Relocation = Relocations[J];
      if (Relocation.Section != I)
        continue;
      if (FDEFields.count(Relocation.Offset) == 0 ||
          Relocation.PatchSize != EHFramePointerWidth ||
          Relocation.Symbol >= Graph.symbols().size())
        return fail();
      const auto &Symbol = Graph.symbols()[Relocation.Symbol];
      if (Symbol.Section >= Sections.size())
        return fail();
      int64_t Addend = Relocation.Addend;
      if (Relocation.AddendIsImplicit) {
        const auto Raw = Internal::readSigned(
            Bytes, Relocation.Offset, EHFramePointerWidth, Endianness::Little);
        if (!Raw)
          return fail();
        Addend = *Raw;
      }
      if (!References
               .emplace(Relocation.Offset,
                        TargetReference{Relocation.Symbol, Addend})
               .second)
        return fail();
      Remove[J] = true;
    }
    for (const auto Field : FDEFields) {
      if (References.count(Field) != 0)
        continue;
      uint64_t Raw = 0;
      if (!readU64(Bytes, Field, Raw))
        return fail();
      uint64_t Target = 0;
      if (!addAddressDelta(Sections[I].InputAddress, Field, 0,
                           static_cast<int64_t>(Raw), Target))
        return fail();
      const auto Symbol = std::find_if(
          Graph.symbols().begin(), Graph.symbols().end(),
          [&](const auto &Value) {
            return Value.Section < Sections.size() &&
                   Sections[Value.Section].InputAddress + Value.Offset ==
                       Target;
          });
      if (Symbol == Graph.symbols().end())
        return fail();
      References.emplace(
          Field,
          TargetReference{
              static_cast<SymbolId>(Symbol - Graph.symbols().begin()), 0});
    }
    for (const auto &[Field, Entry] : References) {
      const auto &Symbol = Graph.symbols()[Entry.Symbol];
      uint64_t SymbolAddress = 0;
      uint64_t FieldAddress = 0;
      int64_t Delta = 0;
      if (!addUnsigned(Sections[Symbol.Section].Address, Symbol.Offset,
                       SymbolAddress) ||
          !addAddressDelta(SymbolAddress, 0, 0, Entry.Addend, SymbolAddress) ||
          !addUnsigned(Sections[I].Address, Field, FieldAddress) ||
          !signedDelta(SymbolAddress, FieldAddress, Delta))
        return fail();
      if (!Internal::writeSigned(Bytes, Field, EHFramePointerWidth,
                                 Endianness::Little, Delta))
        return fail();
    }
    EXPECTED_TRY(parse(Bytes, Graph.target()));
  }
  auto Committed = Graph.commitNormalizedEHFrame(std::move(Content), Remove);
  if (!Committed)
    return fail();
  return {};
}

Expect<void> validateMachOEHFrameCoverage(const LinkGraph &Graph) {
  if (Graph.format() != ObjectFormat::MachO)
    return {};
  std::set<uint64_t> CoveredAddresses;
  const bool HasEHFrame =
      std::any_of(Graph.sections().begin(), Graph.sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::EHFrame;
                  });
  if (HasEHFrame) {
    EXPECTED_TRY(auto Starts, machOEHFrameStarts(Graph, 0));
    CoveredAddresses.insert(Starts.begin(), Starts.end());
  }
  const bool HasUnwindInfo =
      std::any_of(Graph.sections().begin(), Graph.sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::UnwindInfo;
                  });
  if (HasUnwindInfo) {
    for (const auto &Record : Graph.compactUnwind()) {
      if (Record.Function >= Graph.symbols().size())
        return fail();
      const auto &Symbol = Graph.symbols()[Record.Function];
      if (Symbol.Section >= Graph.sections().size())
        return fail();
      CoveredAddresses.insert(Graph.sections()[Symbol.Section].Address +
                              Symbol.Offset);
    }
  }
  std::set<uint64_t> RequiredAddresses;
  for (const auto &Symbol : Graph.symbols()) {
    if (!requiredSemanticFunction(Graph, Symbol))
      continue;
    if (Symbol.Section >= Graph.sections().size())
      return fail();
    uint64_t Address = 0;
    if (!addUnsigned(Graph.sections()[Symbol.Section].Address, Symbol.Offset,
                     Address))
      return fail();
    RequiredAddresses.insert(Address);
  }
  if (!std::includes(CoveredAddresses.begin(), CoveredAddresses.end(),
                     RequiredAddresses.begin(), RequiredAddresses.end()))
    return fail();
  return {};
}

Expect<std::vector<uint64_t>> machOEHFrameStarts(const LinkGraph &Graph,
                                                 uint64_t LoadBase) {
  std::vector<uint64_t> Result;
  for (const auto &Section : Graph.sections()) {
    if (Section.Purpose != SectionPurpose::EHFrame)
      continue;
    EXPECTED_TRY(const auto FDEFields, parse(Section.Content, Graph.target()));
    for (const auto Field : FDEFields) {
      uint64_t Raw = 0;
      if (!readU64(Section.Content, Field, Raw))
        return fail();
      const int64_t Delta = static_cast<int64_t>(Raw);
      EXPECTED_TRY(auto Address, Internal::resolveMachOFDEAddress(
                                     LoadBase, Section.Address, Field, Delta));
      Result.push_back(Address);
    }
  }
  if (Result.empty())
    return fail();
  return Result;
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
