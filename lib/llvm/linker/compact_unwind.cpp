// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/compact_unwind.h"

#include "linker/link_graph.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

// Encoding masks and __unwind_info layout follow Apple's compact unwind ABI,
// as documented by compact_unwind_encoding.h and unwind.h in llvm-project.
constexpr uint32_t UnwindIsNotFunctionStart = 0x80000000;
constexpr uint32_t UnwindHasLSDA = 0x40000000;
constexpr uint32_t UnwindPersonalityMask = 0x30000000;
constexpr uint32_t UnwindArm64ModeMask = 0x0F000000;
constexpr uint32_t UnwindArm64ModeFrameless = 0x02000000;
constexpr uint32_t UnwindArm64ModeDwarf = 0x03000000;
constexpr uint32_t UnwindArm64FramelessStackSizeMask = 0x00FFF000;
constexpr uint32_t UnwindArm64FrameRegisterMask = 0x00000F1F;
constexpr uint32_t UnwindX8664ModeMask = 0x0F000000;
constexpr uint32_t UnwindX8664ModeRBPFrame = 0x01000000;
constexpr uint32_t UnwindX8664ModeStackImmediate = 0x02000000;
constexpr uint32_t UnwindX8664ModeDwarf = 0x04000000;
constexpr uint32_t UnwindX8664RBPFrameRegisters = 0x00007FFF;
constexpr uint32_t UnwindX8664RBPFrameOffset = 0x00FF0000;
constexpr uint32_t UnwindDwarfOffsetMask = 0x00FFFFFF;
constexpr uint32_t UnwindModeMask = 0x0F000000;
constexpr uint32_t UnwindX8664ModeStackIndirect = 0x03000000;
constexpr uint32_t UnwindArm64ModeFrame = 0x04000000;
constexpr size_t UnwindHeaderSize = 28;
constexpr size_t UnwindIndexSize = 12;
constexpr size_t UnwindLSDASize = 8;
constexpr size_t UnwindPageSize = 4096;
constexpr size_t RegularPageCapacity = (UnwindPageSize - 8) / 8;
constexpr size_t MaxCommonEncodings = 127;
constexpr size_t MaxEncodingIndexes = 256;

constexpr Byte DWCfaOffset = 0x80;
constexpr Byte DWCfaSameValue = 0x08;
constexpr Byte DWCfaDefCFA = 0x0C;
constexpr Byte DWCfaOffsetExtended = 0x05;

Expect<void> fail() noexcept { return Unexpect(ErrCode::Value::IllegalPath); }

void append32(std::vector<Byte> &Bytes, uint32_t Value) {
  for (uint8_t I = 0; I < 4; ++I)
    Bytes.push_back(static_cast<Byte>(Value >> (I * 8)));
}

void append64(std::vector<Byte> &Bytes, uint64_t Value) {
  for (uint8_t I = 0; I < 8; ++I)
    Bytes.push_back(static_cast<Byte>(Value >> (I * 8)));
}

uint32_t get32(Span<const Byte> Bytes, size_t Offset) {
  uint32_t Result = 0;
  for (uint8_t I = 0; I < 4; ++I)
    Result |= static_cast<uint32_t>(Bytes[Offset + I]) << (I * 8);
  return Result;
}

void appendULEB(std::vector<Byte> &Bytes, uint64_t Value) {
  do {
    Byte Part = static_cast<Byte>(Value & 0x7F);
    Value >>= 7;
    if (Value != 0)
      Part |= 0x80;
    Bytes.push_back(Part);
  } while (Value != 0);
}

void appendOffset(std::vector<Byte> &Bytes, uint32_t Register,
                  uint64_t Factor) {
  if (Register < 64) {
    Bytes.push_back(DWCfaOffset | static_cast<Byte>(Register));
  } else {
    Bytes.push_back(DWCfaOffsetExtended);
    appendULEB(Bytes, Register);
  }
  appendULEB(Bytes, Factor);
}

void finishRecord(std::vector<Byte> &Bytes, size_t Start) {
  while ((Bytes.size() - Start) % 4 != 0)
    Bytes.push_back(0);
  const uint32_t Length = static_cast<uint32_t>(Bytes.size() - Start - 4);
  for (uint8_t I = 0; I < 4; ++I)
    Bytes[Start + I] = static_cast<Byte>(Length >> (I * 8));
}

std::optional<size_t> terminatorOffset(Span<const Byte> Bytes) {
  size_t Offset = 0;
  while (Offset <= Bytes.size() && Bytes.size() - Offset >= 4) {
    const uint32_t Length = get32(Bytes, Offset);
    if (Length == 0) {
      if (std::any_of(Bytes.begin() + Offset + 4, Bytes.end(),
                      [](Byte Value) { return Value != 0; }))
        return std::nullopt;
      return Offset;
    }
    if (Length == UINT32_MAX || Length > Bytes.size() - Offset - 4)
      return std::nullopt;
    Offset += 4 + Length;
  }
  return std::nullopt;
}

bool appendAArch64CFI(uint32_t Encoding, std::vector<Byte> &Bytes) {
  const uint32_t Mode = Encoding & UnwindArm64ModeMask;
  if (Mode == UnwindArm64ModeFrame) {
    if ((Encoding & ~(UnwindIsNotFunctionStart | UnwindHasLSDA |
                      UnwindArm64ModeMask | UnwindArm64FrameRegisterMask)) != 0)
      return false;
    Bytes.push_back(DWCfaDefCFA);
    appendULEB(Bytes, 29);
    appendULEB(Bytes, 16);
    appendOffset(Bytes, 30, 1);
    appendOffset(Bytes, 29, 2);
    uint64_t Factor = 3;
    for (unsigned Pair = 0; Pair < 5; ++Pair) {
      if ((Encoding & (UINT32_C(1) << Pair)) == 0)
        continue;
      appendOffset(Bytes, 19 + Pair * 2, Factor++);
      appendOffset(Bytes, 20 + Pair * 2, Factor++);
    }
    for (unsigned Pair = 0; Pair < 4; ++Pair) {
      if ((Encoding & (UINT32_C(0x100) << Pair)) == 0)
        continue;
      appendOffset(Bytes, 72 + Pair * 2, Factor++);
      appendOffset(Bytes, 73 + Pair * 2, Factor++);
    }
    return true;
  }
  if (Mode != UnwindArm64ModeFrameless ||
      (Encoding & ~(UnwindIsNotFunctionStart | UnwindHasLSDA |
                    UnwindArm64ModeMask | UnwindArm64FramelessStackSizeMask |
                    UnwindArm64FrameRegisterMask)) != 0)
    return false;
  const uint64_t StackSize =
      static_cast<uint64_t>((Encoding & UnwindArm64FramelessStackSizeMask) >>
                            12) *
      16;
  Bytes.push_back(DWCfaDefCFA);
  appendULEB(Bytes, 31);
  appendULEB(Bytes, StackSize);
  Bytes.push_back(DWCfaSameValue);
  appendULEB(Bytes, 30);
  uint64_t Factor = 0;
  for (unsigned Pair = 0; Pair < 5; ++Pair) {
    if ((Encoding & (UINT32_C(1) << Pair)) == 0)
      continue;
    appendOffset(Bytes, 19 + Pair * 2, Factor++);
    appendOffset(Bytes, 20 + Pair * 2, Factor++);
  }
  for (unsigned Pair = 0; Pair < 4; ++Pair) {
    if ((Encoding & (UINT32_C(0x100) << Pair)) == 0)
      continue;
    appendOffset(Bytes, 72 + Pair * 2, Factor++);
    appendOffset(Bytes, 73 + Pair * 2, Factor++);
  }
  return true;
}

bool decodePermutation(uint32_t Count, uint32_t Permutation,
                       std::vector<uint32_t> &Registers) {
  if (Count > 6)
    return false;
  static constexpr std::array<std::array<uint32_t, 6>, 7> Divisors{
      {{},
       {{1}},
       {{5, 1}},
       {{20, 4, 1}},
       {{60, 12, 3, 1}},
       {{120, 24, 6, 2, 1}},
       {{120, 24, 6, 2, 1, 1}}}};
  std::array<bool, 7> Used{};
  for (uint32_t I = 0; I < Count; ++I) {
    const uint32_t Divisor = Divisors[Count][I];
    const uint32_t Digit = Permutation / Divisor;
    Permutation %= Divisor;
    uint32_t Available = 0;
    uint32_t Register = 0;
    for (uint32_t Candidate = 1; Candidate <= 6; ++Candidate) {
      if (Used[Candidate])
        continue;
      if (Available++ == Digit) {
        Register = Candidate;
        break;
      }
    }
    if (Register == 0)
      return false;
    Used[Register] = true;
    Registers.push_back(Register);
  }
  return Permutation == 0;
}

bool appendX8664FramelessCFI(const LinkGraph &Graph,
                             const CompactUnwindRecord &Record,
                             std::vector<Byte> &Bytes) {
  constexpr uint32_t StackMask = 0x00FF0000;
  constexpr uint32_t AdjustMask = 0x0000E000;
  constexpr uint32_t CountMask = 0x00001C00;
  constexpr uint32_t PermutationMask = 0x000003FF;
  const uint32_t Mode = Record.Encoding & UnwindX8664ModeMask;
  if ((Mode != UnwindX8664ModeStackImmediate &&
       Mode != UnwindX8664ModeStackIndirect) ||
      (Record.Encoding &
       ~(UnwindIsNotFunctionStart | UnwindHasLSDA | UnwindX8664ModeMask |
         StackMask | AdjustMask | CountMask | PermutationMask)) != 0)
    return false;
  const uint32_t Count = (Record.Encoding & CountMask) >> 10;
  std::vector<uint32_t> Registers;
  if (!decodePermutation(Count, Record.Encoding & PermutationMask, Registers))
    return false;
  uint64_t StackSize = 0;
  if (Mode == UnwindX8664ModeStackImmediate) {
    if ((Record.Encoding & AdjustMask) != 0)
      return false;
    StackSize = static_cast<uint64_t>((Record.Encoding & StackMask) >> 16) * 8;
  } else {
    const auto &Function = Graph.symbols()[Record.Function];
    const auto &Content = Graph.sections()[Function.Section].Content;
    const uint64_t Immediate =
        Function.Offset + ((Record.Encoding & StackMask) >> 16);
    const uint64_t FunctionEnd = Function.Offset + Record.Length;
    if (FunctionEnd < Function.Offset || Immediate < Function.Offset + 3 ||
        Immediate > FunctionEnd || FunctionEnd - Immediate < 4 ||
        Immediate > Content.size() || Content.size() - Immediate < 4 ||
        Content[Immediate - 3] != 0x48 || Content[Immediate - 2] != 0x81 ||
        Content[Immediate - 1] != 0xEC)
      return false;
    StackSize = get32(Content, Immediate);
    const uint64_t Adjust =
        static_cast<uint64_t>((Record.Encoding & AdjustMask) >> 13) * 8;
    if (StackSize > UINT64_MAX - Adjust)
      return false;
    StackSize += Adjust;
  }
  if (StackSize < 8 * (Count + 1))
    return false;
  static constexpr std::array<uint8_t, 7> Dwarf{{0, 3, 12, 13, 14, 15, 6}};
  Bytes.push_back(DWCfaDefCFA);
  appendULEB(Bytes, 7);
  appendULEB(Bytes, StackSize);
  appendOffset(Bytes, 16, 1);
  uint64_t Factor = 2;
  for (auto It = Registers.rbegin(); It != Registers.rend(); ++It)
    appendOffset(Bytes, Dwarf[*It], Factor++);
  return true;
}

bool decodeX8664RBP(uint32_t Encoding,
                    std::vector<std::pair<uint32_t, unsigned>> &Saved) {
  if ((Encoding & UnwindX8664ModeMask) != UnwindX8664ModeRBPFrame ||
      (Encoding &
       ~(UnwindIsNotFunctionStart | UnwindHasLSDA | UnwindX8664ModeMask |
         UnwindX8664RBPFrameOffset | UnwindX8664RBPFrameRegisters)) != 0)
    return false;
  const uint32_t StackOffset = (Encoding & UnwindX8664RBPFrameOffset) >> 16;
  uint32_t Registers = Encoding & UnwindX8664RBPFrameRegisters;
  const std::array<uint8_t, 6> DwarfRegisters{{0, 3, 12, 13, 14, 15}};
  std::set<uint32_t> Seen;
  for (unsigned Slot = 0; Slot < 5; ++Slot) {
    const uint32_t Register = Registers & 7;
    Registers >>= 3;
    if (Register == 0)
      continue;
    if (Register >= DwarfRegisters.size() || !Seen.emplace(Register).second)
      return false;
    Saved.emplace_back(DwarfRegisters[Register], Slot);
  }
  if (Registers != 0 ||
      std::any_of(Saved.begin(), Saved.end(), [&](const auto &Value) {
        return StackOffset + 2 < Value.second;
      }))
    return false;
  return true;
}

bool appendX8664CFI(uint32_t Encoding, std::vector<Byte> &Bytes) {
  std::vector<std::pair<uint32_t, unsigned>> Saved;
  if (!decodeX8664RBP(Encoding, Saved))
    return false;
  const uint32_t StackOffset = (Encoding & UnwindX8664RBPFrameOffset) >> 16;
  Bytes.push_back(DWCfaDefCFA);
  appendULEB(Bytes, 6);
  appendULEB(Bytes, 16);
  Bytes.push_back(DWCfaOffset | 16);
  appendULEB(Bytes, 1);
  Bytes.push_back(DWCfaOffset | 6);
  appendULEB(Bytes, 2);
  for (const auto &[Register, Slot] : Saved) {
    Bytes.push_back(static_cast<Byte>(DWCfaOffset | Register));
    appendULEB(Bytes, static_cast<uint64_t>(StackOffset) + 2 - Slot);
  }
  return true;
}

bool appendCFI(const LinkGraph &Graph, const CompactUnwindRecord &Record,
               std::vector<Byte> &Bytes) {
  if (Graph.target() == Target::AArch64)
    return appendAArch64CFI(Record.Encoding, Bytes);
  const uint32_t Mode = Record.Encoding & UnwindX8664ModeMask;
  if (Mode == UnwindX8664ModeRBPFrame)
    return appendX8664CFI(Record.Encoding, Bytes);
  return appendX8664FramelessCFI(Graph, Record, Bytes);
}

bool hasExistingFDE(const LinkGraph &Graph, SymbolId Function) {
  if (std::any_of(
          Graph.ehFrameReferences().begin(), Graph.ehFrameReferences().end(),
          [&](const auto &Reference) { return Reference.Symbol == Function; }))
    return true;
  return std::any_of(Graph.relocations().begin(), Graph.relocations().end(),
                     [&](const auto &Relocation) {
                       return Relocation.Symbol == Function &&
                              Relocation.Section < Graph.sections().size() &&
                              Graph.sections()[Relocation.Section].Purpose ==
                                  SectionPurpose::EHFrame;
                     });
}

bool hasAssociatedFDE(const LinkGraph &Graph,
                      const CompactUnwindRecord &Record) {
  if (!Record.FDE || *Record.FDE >= Graph.symbols().size() ||
      Record.Function >= Graph.symbols().size())
    return false;
  const auto &FDE = Graph.symbols()[*Record.FDE];
  const auto &Function = Graph.symbols()[Record.Function];
  const auto IsFunction = [&](SymbolId Id) {
    return Id < Graph.symbols().size() &&
           Graph.symbols()[Id].Section == Function.Section &&
           Graph.symbols()[Id].Offset == Function.Offset;
  };
  const auto Matches = [&](const auto &Reference) {
    return Reference.Section == FDE.Section &&
           Reference.Offset == FDE.Offset + 8 && IsFunction(Reference.Symbol);
  };
  return std::any_of(Graph.ehFrameReferences().begin(),
                     Graph.ehFrameReferences().end(), Matches) ||
         std::any_of(Graph.relocations().begin(), Graph.relocations().end(),
                     Matches);
}

bool isDwarf(const LinkGraph &Graph, uint32_t Encoding) noexcept {
  const uint32_t Mode = Encoding & UnwindModeMask;
  return (Graph.target() == Target::AArch64 && Mode == UnwindArm64ModeDwarf) ||
         (Graph.target() == Target::X86_64 && Mode == UnwindX8664ModeDwarf);
}

bool canonicalEHFrameSection(const LinkGraph &Graph, SectionId &Result) {
  bool Found = false;
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    const auto &Section = Graph.sections()[I];
    if (Section.Purpose != SectionPurpose::EHFrame)
      continue;
    if (Found || Section.Name != "__eh_frame")
      return false;
    Found = true;
    Result = I;
  }
  return Found;
}

std::optional<uint64_t> symbolAddress(const LinkGraph &Graph, SymbolId Id) {
  const auto &Symbol = Graph.symbols()[Id];
  const uint64_t Address = Graph.sections()[Symbol.Section].Address;
  if (Symbol.Offset > UINT64_MAX - Address)
    return std::nullopt;
  return Address + Symbol.Offset;
}

struct NativeRecord {
  uint64_t Address;
  uint32_t Length;
  uint32_t Encoding;
  std::optional<uint64_t> LSDA;
};

struct NativePage {
  uint32_t Kind;
  size_t Begin;
  size_t Count;
  std::vector<uint32_t> LocalEncodings;
  std::map<uint32_t, uint8_t> EncodingIndexes;
};

struct NativeLayout {
  size_t ReservedCommonCount = 0;
  size_t ReservedPageCount = 0;
  size_t LSDACount = 0;
  std::vector<NativeRecord> Records;
  std::vector<uint32_t> CommonEncodings;
  std::map<uint32_t, uint8_t> CommonIndexes;
  std::vector<NativePage> Pages;
};

Expect<NativeLayout> buildNativeLayout(const LinkGraph &Graph, bool Final) {
  if (Graph.format() != ObjectFormat::MachO ||
      Graph.endianness() != Endianness::Little ||
      (Graph.target() != Target::X86_64 && Graph.target() != Target::AArch64) ||
      Graph.compactUnwind().empty())
    return Unexpect(ErrCode::Value::IllegalPath);
  NativeLayout Result;
  std::set<uint32_t> ReservedEncodings;
  std::set<SymbolId> Personalities;
  for (const auto &Record : Graph.compactUnwind()) {
    if (Record.Personality)
      Personalities.insert(*Record.Personality);
    if (Personalities.size() > 3 || Record.Personality ||
        (Record.Encoding & UnwindPersonalityMask) != 0 ||
        (((Record.Encoding & UnwindHasLSDA) != 0) !=
         static_cast<bool>(Record.LSDA)))
      return Unexpect(ErrCode::Value::IllegalPath);
    std::vector<Byte> Ignored;
    if (!isDwarf(Graph, Record.Encoding) && !appendCFI(Graph, Record, Ignored))
      return Unexpect(ErrCode::Value::IllegalPath);
    if (isDwarf(Graph, Record.Encoding)) {
      SectionId EHFrame = InvalidSectionId;
      if (!hasAssociatedFDE(Graph, Record) ||
          !canonicalEHFrameSection(Graph, EHFrame) ||
          Graph.symbols()[*Record.FDE].Section != EHFrame)
        return Unexpect(ErrCode::Value::IllegalPath);
    }
    if (!isDwarf(Graph, Record.Encoding) && Record.FDE)
      return Unexpect(ErrCode::Value::IllegalPath);
    if (Record.LSDA)
      ++Result.LSDACount;
    uint32_t ReservedEncoding = Record.Encoding;
    if (isDwarf(Graph, ReservedEncoding)) {
      const auto &FDE = Graph.symbols()[*Record.FDE];
      if (FDE.Offset > UnwindDwarfOffsetMask)
        return Unexpect(ErrCode::Value::IllegalPath);
      ReservedEncoding = (ReservedEncoding & ~UnwindDwarfOffsetMask) |
                         static_cast<uint32_t>(FDE.Offset);
    }
    ReservedEncodings.insert(ReservedEncoding);
    if (!Final)
      continue;
    uint32_t Encoding = Record.Encoding;
    if (isDwarf(Graph, Encoding)) {
      if (!Record.FDE)
        return Unexpect(ErrCode::Value::IllegalPath);
      const auto &FDE = Graph.symbols()[*Record.FDE];
      const auto &EH = Graph.sections()[FDE.Section];
      if (FDE.Offset > UnwindDwarfOffsetMask)
        return Unexpect(ErrCode::Value::IllegalPath);
      Encoding = (Encoding & ~UnwindDwarfOffsetMask) |
                 static_cast<uint32_t>(FDE.Offset);
      if (EH.Purpose != SectionPurpose::EHFrame)
        return Unexpect(ErrCode::Value::IllegalPath);
    }
    const auto FunctionAddress = symbolAddress(Graph, Record.Function);
    std::optional<uint64_t> LSDAAddress;
    if (Record.LSDA)
      LSDAAddress = symbolAddress(Graph, *Record.LSDA);
    if (!FunctionAddress || (Record.LSDA && !LSDAAddress))
      return Unexpect(ErrCode::Value::IllegalPath);
    Result.Records.push_back(
        {*FunctionAddress, Record.Length, Encoding, LSDAAddress});
  }
  Result.ReservedCommonCount =
      std::min(ReservedEncodings.size(), MaxCommonEncodings);
  Result.ReservedPageCount =
      (Graph.compactUnwind().size() + RegularPageCapacity - 1) /
      RegularPageCapacity;
  if (!Final)
    return Result;

  std::sort(Result.Records.begin(), Result.Records.end(),
            [](const auto &L, const auto &R) { return L.Address < R.Address; });
  std::vector<NativeRecord> Merged;
  for (const auto &Record : Result.Records) {
    if (!Merged.empty()) {
      const auto &Previous = Merged.back();
      const bool StackIndirect =
          Graph.target() == Target::X86_64 &&
          (Record.Encoding & UnwindModeMask) == UnwindX8664ModeStackIndirect;
      if (Previous.Address <= UINT64_MAX - Previous.Length &&
          Previous.Address + Previous.Length == Record.Address &&
          Previous.Encoding == Record.Encoding && !Previous.LSDA &&
          !Record.LSDA && !isDwarf(Graph, Record.Encoding) && !StackIndirect) {
        if (Record.Length > UINT32_MAX - Merged.back().Length)
          return Unexpect(ErrCode::Value::IllegalPath);
        Merged.back().Length += Record.Length;
        continue;
      }
    }
    Merged.push_back(Record);
  }
  Result.Records.swap(Merged);

  std::map<uint32_t, size_t> Frequencies;
  for (const auto &Record : Result.Records)
    ++Frequencies[Record.Encoding];
  std::vector<std::pair<uint32_t, size_t>> Ordered(Frequencies.begin(),
                                                   Frequencies.end());
  std::sort(Ordered.begin(), Ordered.end(), [](const auto &L, const auto &R) {
    return std::tie(L.second, L.first) > std::tie(R.second, R.first);
  });
  const size_t CommonCount =
      std::min(Result.ReservedCommonCount, Ordered.size());
  for (size_t I = 0; I < CommonCount; ++I) {
    Result.CommonIndexes.emplace(Ordered[I].first, static_cast<uint8_t>(I));
    Result.CommonEncodings.push_back(Ordered[I].first);
  }

  for (size_t Begin = 0; Begin < Result.Records.size();) {
    const size_t RegularCount =
        std::min(RegularPageCapacity, Result.Records.size() - Begin);
    // A kind-3 compressed second-level page packs a 24-bit function delta and
    // an 8-bit common/local encoding index into each entry.
    NativePage Compressed{3, Begin, 0, {}, Result.CommonIndexes};
    size_t Used = 12;
    for (size_t I = Begin; I < Result.Records.size(); ++I) {
      const auto &Record = Result.Records[I];
      if (Record.Address - Result.Records[Begin].Address >
          UnwindDwarfOffsetMask)
        break;
      auto Encoding = Compressed.EncodingIndexes.find(Record.Encoding);
      if (Encoding == Compressed.EncodingIndexes.end()) {
        if (Compressed.EncodingIndexes.size() == MaxEncodingIndexes ||
            Used + 8 > UnwindPageSize)
          break;
        const auto Index =
            static_cast<uint8_t>(Compressed.EncodingIndexes.size());
        Compressed.EncodingIndexes.emplace(Record.Encoding, Index);
        Compressed.LocalEncodings.push_back(Record.Encoding);
        Used += 4;
      }
      if (Used + 4 > UnwindPageSize)
        break;
      Used += 4;
      ++Compressed.Count;
    }
    const size_t RegularUsed = 8 + RegularCount * 8;
    if (Compressed.Count < RegularCount ||
        (Compressed.Count == RegularCount && Used >= RegularUsed)) {
      // A kind-2 regular page stores full function offsets and encodings.
      Result.Pages.push_back({2, Begin, RegularCount, {}, {}});
      Begin += RegularCount;
    } else {
      Begin += Compressed.Count;
      Result.Pages.push_back(std::move(Compressed));
    }
  }
  if (Result.Pages.size() > Result.ReservedPageCount)
    return Unexpect(ErrCode::Value::IllegalPath);
  return Result;
}

bool delta32(uint64_t Address, uint64_t Base, uint32_t &Result) noexcept {
  if (Address < Base || Address - Base > UINT32_MAX)
    return false;
  Result = static_cast<uint32_t>(Address - Base);
  return true;
}

bool narrow32(uint64_t Value, uint32_t &Result) noexcept {
  if (Value > UINT32_MAX)
    return false;
  Result = static_cast<uint32_t>(Value);
  return true;
}

void put16(std::vector<Byte> &Bytes, size_t Offset, uint16_t Value) {
  for (uint8_t I = 0; I < 2; ++I)
    Bytes[Offset + I] = static_cast<Byte>(Value >> (I * 8));
}

void put32(std::vector<Byte> &Bytes, size_t Offset, uint32_t Value) {
  for (uint8_t I = 0; I < 4; ++I)
    Bytes[Offset + I] = static_cast<Byte>(Value >> (I * 8));
}

} // namespace

Expect<uint64_t> machOUnwindInfoSize(const LinkGraph &Graph) {
  EXPECTED_TRY(auto Layout, buildNativeLayout(Graph, false));
  uint64_t Result = UnwindHeaderSize;
  const auto AddProduct = [&](uint64_t Count, uint64_t Width) {
    if (Count > (UINT64_MAX - Result) / Width)
      return false;
    Result += Count * Width;
    return true;
  };
  if (!AddProduct(Layout.ReservedCommonCount, 4) ||
      Layout.ReservedPageCount == UINT64_MAX ||
      !AddProduct(Layout.ReservedPageCount + 1, UnwindIndexSize) ||
      !AddProduct(Layout.LSDACount, UnwindLSDASize) ||
      !AddProduct(Layout.ReservedPageCount, UnwindPageSize) ||
      Result > UINT32_MAX)
    return Unexpect(ErrCode::Value::IllegalPath);
  return Result;
}

Expect<void> reserveMachOUnwindInfo(LinkGraph &Graph) {
  if (Graph.relocationsApplied() ||
      std::any_of(Graph.sections().begin(), Graph.sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::UnwindInfo ||
                           Section.Purpose == SectionPurpose::CompactUnwind ||
                           Section.Name == "__unwind_info" ||
                           Section.Name == "__compact_unwind";
                  }))
    return fail();
  EXPECTED_TRY(auto Size, machOUnwindInfoSize(Graph));
#if SIZE_MAX < UINT64_MAX
  if (Size > SIZE_MAX)
    return fail();
#endif
  auto Reserved = Graph.reserveMachOUnwindInfoSection(
      Section{"__unwind_info", SectionKind::Unwind, 4, Size, 0, 0,
              std::vector<Byte>(static_cast<size_t>(Size)),
              SectionPurpose::UnwindInfo});
  if (!Reserved)
    return fail();
  return {};
}

Expect<void> populateMachOUnwindInfo(LinkGraph &Graph, uint64_t ImageBase) {
  if (!Graph.relocationsApplied())
    return fail();
  EXPECTED_TRY(auto Layout, buildNativeLayout(Graph, true));
  EXPECTED_TRY(auto ExpectedSize, machOUnwindInfoSize(Graph));
  const auto It =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::UnwindInfo;
                   });
  if (It == Graph.sections().end() || It->Content.size() != ExpectedSize ||
      It->VirtualSize != ExpectedSize)
    return fail();
  const SectionId Id = static_cast<SectionId>(It - Graph.sections().begin());
  std::vector<Byte> Content(It->Content.size());
  const size_t CommonOffset = UnwindHeaderSize;
  const size_t PersonalityOffset =
      CommonOffset + Layout.ReservedCommonCount * 4;
  const size_t IndexOffset = PersonalityOffset;
  const size_t ReservedIndexCount = Layout.ReservedPageCount + 1;
  const size_t LSDAOffset = IndexOffset + ReservedIndexCount * UnwindIndexSize;
  const size_t PageOffset = LSDAOffset + Layout.LSDACount * UnwindLSDASize;
  uint32_t CommonOffset32 = 0;
  uint32_t PersonalityOffset32 = 0;
  uint32_t IndexOffset32 = 0;
  uint32_t CommonCount32 = 0;
  uint32_t IndexCount32 = 0;
  if (!narrow32(CommonOffset, CommonOffset32) ||
      !narrow32(PersonalityOffset, PersonalityOffset32) ||
      !narrow32(IndexOffset, IndexOffset32) ||
      !narrow32(Layout.CommonEncodings.size(), CommonCount32) ||
      !narrow32(Layout.Pages.size() + 1, IndexCount32))
    return fail();
  put32(Content, 0, 1);
  put32(Content, 4, CommonOffset32);
  put32(Content, 8, CommonCount32);
  put32(Content, 12, PersonalityOffset32);
  put32(Content, 16, 0);
  put32(Content, 20, IndexOffset32);
  put32(Content, 24, IndexCount32);
  for (size_t I = 0; I < Layout.CommonEncodings.size(); ++I)
    put32(Content, CommonOffset + I * 4, Layout.CommonEncodings[I]);

  size_t LSDAIndex = 0;
  for (size_t I = 0; I < Layout.Pages.size(); ++I) {
    const auto &Page = Layout.Pages[I];
    uint32_t FunctionDelta = 0;
    if (!delta32(Layout.Records[Page.Begin].Address, ImageBase, FunctionDelta))
      return fail();
    put32(Content, IndexOffset + I * UnwindIndexSize, FunctionDelta);
    uint32_t CurrentPageOffset = 0;
    uint32_t CurrentLSDAOffset = 0;
    if (!narrow32(PageOffset + I * UnwindPageSize, CurrentPageOffset) ||
        !narrow32(LSDAOffset + LSDAIndex * UnwindLSDASize, CurrentLSDAOffset))
      return fail();
    put32(Content, IndexOffset + I * UnwindIndexSize + 4, CurrentPageOffset);
    put32(Content, IndexOffset + I * UnwindIndexSize + 8, CurrentLSDAOffset);
    for (size_t J = 0; J < Page.Count; ++J)
      if (Layout.Records[Page.Begin + J].LSDA)
        ++LSDAIndex;
  }
  const auto &Last = Layout.Records.back();
  if (Last.Address > UINT64_MAX - Last.Length)
    return fail();
  uint32_t EndDelta = 0;
  if (!delta32(Last.Address + Last.Length, ImageBase, EndDelta))
    return fail();
  const size_t Sentinel = IndexOffset + Layout.Pages.size() * UnwindIndexSize;
  put32(Content, Sentinel, EndDelta);
  uint32_t LSDAEndOffset = 0;
  if (!narrow32(LSDAOffset + Layout.LSDACount * UnwindLSDASize, LSDAEndOffset))
    return fail();
  put32(Content, Sentinel + 8, LSDAEndOffset);

  size_t LSDAWrite = LSDAOffset;
  for (const auto &Record : Layout.Records) {
    if (!Record.LSDA)
      continue;
    uint32_t FunctionDelta = 0;
    uint32_t LSDADelta = 0;
    if (!delta32(Record.Address, ImageBase, FunctionDelta) ||
        !delta32(*Record.LSDA, ImageBase, LSDADelta))
      return fail();
    put32(Content, LSDAWrite, FunctionDelta);
    put32(Content, LSDAWrite + 4, LSDADelta);
    LSDAWrite += UnwindLSDASize;
  }

  for (size_t I = 0; I < Layout.Pages.size(); ++I) {
    const auto &Page = Layout.Pages[I];
    const size_t Offset = PageOffset + I * UnwindPageSize;
    put32(Content, Offset, Page.Kind);
    if (Page.Kind == 2) {
      put16(Content, Offset + 4, 8);
      put16(Content, Offset + 6, static_cast<uint16_t>(Page.Count));
      for (size_t J = 0; J < Page.Count; ++J) {
        const auto &Record = Layout.Records[Page.Begin + J];
        uint32_t FunctionDelta = 0;
        if (!delta32(Record.Address, ImageBase, FunctionDelta))
          return fail();
        put32(Content, Offset + 8 + J * 8, FunctionDelta);
        put32(Content, Offset + 12 + J * 8, Record.Encoding);
      }
    } else {
      const uint16_t EncodingOffset =
          static_cast<uint16_t>(12 + Page.Count * 4);
      put16(Content, Offset + 4, 12);
      put16(Content, Offset + 6, static_cast<uint16_t>(Page.Count));
      put16(Content, Offset + 8, EncodingOffset);
      put16(Content, Offset + 10,
            static_cast<uint16_t>(Page.LocalEncodings.size()));
      for (size_t J = 0; J < Page.Count; ++J) {
        const auto &Record = Layout.Records[Page.Begin + J];
        const uint64_t FunctionOffset =
            Record.Address - Layout.Records[Page.Begin].Address;
        const auto Encoding = Page.EncodingIndexes.find(Record.Encoding);
        if (Encoding == Page.EncodingIndexes.end() ||
            FunctionOffset > UnwindDwarfOffsetMask)
          return fail();
        put32(Content, Offset + 12 + J * 4,
              (static_cast<uint32_t>(Encoding->second) << 24) |
                  static_cast<uint32_t>(FunctionOffset));
      }
      for (size_t J = 0; J < Page.LocalEncodings.size(); ++J)
        put32(Content, Offset + EncodingOffset + J * 4, Page.LocalEncodings[J]);
    }
  }
  auto Populated = Graph.populateMachOUnwindInfoSection(Id, std::move(Content));
  if (!Populated)
    return fail();
  return {};
}

Expect<void> compactUnwindToEHFrame(LinkGraph &Graph) {
  if (Graph.format() != ObjectFormat::MachO ||
      Graph.endianness() != Endianness::Little || Graph.relocationsApplied() ||
      Graph.machOUnwindInfoState() == MachOUnwindInfoState::Populated)
    return fail();
  struct PendingFDE {
    SymbolId Function;
    uint32_t Length;
    std::vector<Byte> CFI;
  };
  std::vector<PendingFDE> FDEs;
  for (const auto &Record : Graph.compactUnwind()) {
    if ((Record.Encoding & (UnwindHasLSDA | UnwindPersonalityMask)) != 0 ||
        Record.Personality || Record.LSDA)
      return fail();
    const uint32_t Mode = Record.Encoding & 0x0F000000;
    const bool Dwarf =
        (Graph.target() == Target::AArch64 && Mode == UnwindArm64ModeDwarf) ||
        (Graph.target() == Target::X86_64 && Mode == UnwindX8664ModeDwarf);
    if (Dwarf) {
      if (!hasAssociatedFDE(Graph, Record))
        return fail();
      continue;
    }
    if (hasExistingFDE(Graph, Record.Function))
      continue;
    std::vector<Byte> CFI;
    const bool Decoded = appendCFI(Graph, Record, CFI);
    if (!Decoded)
      return fail();
    FDEs.push_back(PendingFDE{Record.Function, Record.Length, std::move(CFI)});
  }
  if (FDEs.empty())
    return {};

  std::optional<SectionId> ExistingSection;
  Section Synthetic{"__eh_frame", SectionKind::Unwind,    8, 0, 0, 0,
                    {},           SectionPurpose::EHFrame};
  std::vector<Byte> Content;
  for (SectionId I = 0; I < Graph.sections().size(); ++I) {
    if (Graph.sections()[I].Purpose != SectionPurpose::EHFrame)
      continue;
    const auto Terminator = terminatorOffset(Graph.sections()[I].Content);
    if (!Terminator)
      return fail();
    ExistingSection = I;
    Synthetic = Graph.sections()[I];
    Content.assign(Synthetic.Content.data(),
                   Synthetic.Content.data() + *Terminator);
    break;
  }
  const size_t CIE = Content.size();
  append32(Content, 0);
  append32(Content, 0);
  Content.insert(Content.end(), {1, 'z', 'R', 0});
  appendULEB(Content, Graph.target() == Target::AArch64 ? 4 : 1);
  Content.push_back(0x78);
  appendULEB(Content, Graph.target() == Target::AArch64 ? 30 : 16);
  appendULEB(Content, 1);
  Content.push_back(0x10);
  finishRecord(Content, CIE);

  std::vector<EHFrameReference> References;
  References.reserve(FDEs.size());
  for (const auto &[Function, Length, CFI] : FDEs) {
    const size_t FDE = Content.size();
    append32(Content, 0);
    append32(Content, static_cast<uint32_t>(FDE + 4 - CIE));
    const uint64_t FunctionField = Content.size();
    append64(Content, 0);
    append64(Content, Length);
    appendULEB(Content, 0);
    Content.insert(Content.end(), CFI.begin(), CFI.end());
    finishRecord(Content, FDE);
    References.push_back(
        EHFrameReference{InvalidSectionId, FunctionField, Function});
  }
  Content.resize(Content.size() + 4);
  Synthetic.Content = std::move(Content);
  Synthetic.VirtualSize = Synthetic.Content.size();
  auto Committed = Graph.addSynthesizedEHFrame(
      ExistingSection, std::move(Synthetic), std::move(References));
  if (!Committed)
    return fail();
  return {};
}

Expect<void> validateCompactUnwind(const LinkGraph &Graph) {
  if (Graph.format() != ObjectFormat::MachO)
    return {};
  for (const auto &Record : Graph.compactUnwind()) {
    if (Record.Function >= Graph.symbols().size() || Record.Personality ||
        ((Record.Encoding & UnwindPersonalityMask) != 0) ||
        (((Record.Encoding & UnwindHasLSDA) != 0) !=
         static_cast<bool>(Record.LSDA)))
      return fail();
    if (isDwarf(Graph, Record.Encoding)) {
      if (!hasAssociatedFDE(Graph, Record))
        return fail();
      continue;
    }
    std::vector<Byte> Ignored;
    if (Record.FDE || !appendCFI(Graph, Record, Ignored))
      return fail();
  }
  return {};
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
