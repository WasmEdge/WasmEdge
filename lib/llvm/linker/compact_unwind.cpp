// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/compact_unwind.h"

#include "linker/byte_io.h"
#include "linker/eh_frame.h"
#include "linker/link_graph.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Support/Endian.h>
#include <llvm/Support/LEB128.h>

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using llvm::support::endian::read32le;
using llvm::support::endian::write16le;
using llvm::support::endian::write32le;
using llvm::support::endian::write64le;

// Encoding masks and __unwind_info layout follow Apple's compact unwind ABI,
// as documented by compact_unwind_encoding.h and unwind.h in llvm-project.
constexpr uint32_t UnwindIsNotFunctionStart = 0x80000000;
constexpr uint32_t UnwindHasLSDA = 0x40000000;
constexpr uint32_t UnwindPersonalityMask = 0x30000000;
constexpr uint32_t UnwindArm64ModeMask = 0x0F000000;
constexpr uint32_t UnwindArm64ModeFrameless = 0x02000000;
constexpr uint32_t UnwindArm64FramelessStackSizeMask = 0x00FFF000;
constexpr uint32_t UnwindArm64FrameRegisterMask = 0x00000F1F;
constexpr uint32_t UnwindX8664ModeMask = 0x0F000000;
constexpr uint32_t UnwindX8664ModeRBPFrame = 0x01000000;
constexpr uint32_t UnwindX8664ModeStackImmediate = 0x02000000;
constexpr uint32_t UnwindX8664RBPFrameRegisters = 0x00007FFF;
constexpr uint32_t UnwindX8664RBPFrameOffset = 0x00FF0000;
constexpr uint32_t UnwindX8664ModeStackIndirect = 0x03000000;
constexpr uint32_t UnwindArm64ModeFrame = 0x04000000;

constexpr uint32_t UnwindInfoVersion = 1;
constexpr size_t UnwindHeaderSize = 28;
constexpr size_t UnwindHeaderVersionOffset = 0;
constexpr size_t UnwindHeaderCommonEncodingsOffset = 4;
constexpr size_t UnwindHeaderCommonEncodingsCountOffset = 8;
constexpr size_t UnwindHeaderPersonalitiesOffset = 12;
constexpr size_t UnwindHeaderPersonalitiesCountOffset = 16;
constexpr size_t UnwindHeaderIndexOffset = 20;
constexpr size_t UnwindHeaderIndexCountOffset = 24;
constexpr size_t UnwindEncodingSize = 4;
constexpr size_t UnwindIndexSize = 12;
constexpr size_t UnwindIndexFunctionOffset = 0;
constexpr size_t UnwindIndexPageOffset = 4;
constexpr size_t UnwindIndexLSDAOffset = 8;
constexpr size_t UnwindLSDASize = 8;
constexpr size_t UnwindLSDAFunctionOffset = 0;
constexpr size_t UnwindLSDAOffset = 4;
constexpr size_t UnwindPageSize = 4096;
constexpr size_t UnwindPageKindOffset = 0;
constexpr size_t UnwindPageEntriesOffset = 4;
constexpr size_t UnwindPageEntryCountOffset = 6;
constexpr size_t UnwindCompressedPageEncodingsOffset = 8;
constexpr size_t UnwindCompressedPageEncodingCountOffset = 10;
constexpr size_t RegularPageHeaderSize = 8;
constexpr size_t RegularPageEntrySize = 8;
constexpr size_t RegularEntryEncodingOffset = 4;
constexpr size_t CompressedPageHeaderSize = 12;
constexpr size_t CompressedPageEntrySize = 4;
constexpr uint32_t CompressedFunctionOffsetMask = 0x00FFFFFF;
constexpr unsigned CompressedEncodingIndexShift = 24;
constexpr size_t RegularPageCapacity =
    (UnwindPageSize - RegularPageHeaderSize) / RegularPageEntrySize;
constexpr size_t MaxCommonEncodings = 127;
constexpr size_t MaxEncodingIndexes = 256;

enum class UnwindPageKind : uint32_t {
  Regular = 2,
  Compressed = 3,
};

constexpr uint32_t AArch64DwarfX19 = 19;
constexpr uint32_t AArch64DwarfFP = 29;
constexpr uint32_t AArch64DwarfLR = 30;
constexpr uint32_t AArch64DwarfSP = 31;
constexpr uint32_t AArch64DwarfD8 = 72;
constexpr uint32_t X8664DwarfRBP = 6;
constexpr uint32_t X8664DwarfRSP = 7;
constexpr uint32_t X8664DwarfReturnAddress = 16;
constexpr uint32_t X8664CompactRegisterRBP = 6;
// Compact unwind register numbers 1-6 name RBX, R12, R13, R14, R15 and RBP.
constexpr std::array<uint8_t, 7> X8664CompactToDwarf{
    {0, 3, 12, 13, 14, 15, X8664DwarfRBP}};

constexpr uint32_t DwarfRegisterInlineLimit = 64;

auto fail() noexcept { return Unexpect(ErrCode::Value::IllegalPath); }

void append32(std::vector<Byte> &Bytes, uint32_t Value) {
  const size_t Offset = Bytes.size();
  Bytes.resize(Offset + sizeof(Value));
  write32le(Bytes.data() + Offset, Value);
}

void append64(std::vector<Byte> &Bytes, uint64_t Value) {
  const size_t Offset = Bytes.size();
  Bytes.resize(Offset + sizeof(Value));
  write64le(Bytes.data() + Offset, Value);
}

void appendULEB(std::vector<Byte> &Bytes, uint64_t Value) {
  uint8_t Encoded[16];
  const unsigned Size = llvm::encodeULEB128(Value, Encoded);
  Bytes.insert(Bytes.end(), Encoded, Encoded + Size);
}

void appendSLEB(std::vector<Byte> &Bytes, int64_t Value) {
  uint8_t Encoded[16];
  const unsigned Size = llvm::encodeSLEB128(Value, Encoded);
  Bytes.insert(Bytes.end(), Encoded, Encoded + Size);
}

void appendOffset(std::vector<Byte> &Bytes, uint32_t Register,
                  uint64_t Factor) {
  if (Register < DwarfRegisterInlineLimit) {
    Bytes.push_back(static_cast<Byte>(llvm::dwarf::DW_CFA_offset | Register));
  } else {
    Bytes.push_back(llvm::dwarf::DW_CFA_offset_extended);
    appendULEB(Bytes, Register);
  }
  appendULEB(Bytes, Factor);
}

void appendDefCFA(std::vector<Byte> &Bytes, uint32_t Register,
                  uint64_t Offset) {
  Bytes.push_back(llvm::dwarf::DW_CFA_def_cfa);
  appendULEB(Bytes, Register);
  appendULEB(Bytes, Offset);
}

void finishRecord(std::vector<Byte> &Bytes, size_t Start) {
  constexpr size_t RecordAlignment = 4;
  while ((Bytes.size() - Start) % RecordAlignment != 0)
    Bytes.push_back(0);
  write32le(Bytes.data() + Start,
            static_cast<uint32_t>(Bytes.size() - Start -
                                  Internal::EHFrameLengthFieldSize));
}

void appendAArch64SavedPairs(uint32_t Encoding, uint64_t Factor,
                             std::vector<Byte> &Bytes) {
  constexpr uint32_t GPRPairBit = UINT32_C(0x1);
  constexpr uint32_t FPRPairBit = UINT32_C(0x100);
  constexpr unsigned GPRPairCount = 5;
  constexpr unsigned FPRPairCount = 4;
  const auto AppendPairs = [&](uint32_t FirstBit, unsigned Count,
                               uint32_t FirstRegister) {
    for (unsigned Pair = 0; Pair < Count; ++Pair) {
      if ((Encoding & (FirstBit << Pair)) == 0)
        continue;
      appendOffset(Bytes, FirstRegister + Pair * 2, Factor++);
      appendOffset(Bytes, FirstRegister + Pair * 2 + 1, Factor++);
    }
  };
  AppendPairs(GPRPairBit, GPRPairCount, AArch64DwarfX19);
  AppendPairs(FPRPairBit, FPRPairCount, AArch64DwarfD8);
}

bool appendAArch64CFI(uint32_t Encoding, std::vector<Byte> &Bytes) {
  constexpr uint64_t FrameRecordSize = 16;
  constexpr unsigned FramelessStackSizeShift = 12;
  constexpr uint64_t StackSizeScale = 16;
  const uint32_t Mode = Encoding & UnwindArm64ModeMask;
  if (Mode == UnwindArm64ModeFrame) {
    if ((Encoding & ~(UnwindIsNotFunctionStart | UnwindHasLSDA |
                      UnwindArm64ModeMask | UnwindArm64FrameRegisterMask)) != 0)
      return false;
    appendDefCFA(Bytes, AArch64DwarfFP, FrameRecordSize);
    appendOffset(Bytes, AArch64DwarfLR, 1);
    appendOffset(Bytes, AArch64DwarfFP, 2);
    appendAArch64SavedPairs(Encoding, 3, Bytes);
    return true;
  }
  if (Mode != UnwindArm64ModeFrameless ||
      (Encoding & ~(UnwindIsNotFunctionStart | UnwindHasLSDA |
                    UnwindArm64ModeMask | UnwindArm64FramelessStackSizeMask |
                    UnwindArm64FrameRegisterMask)) != 0)
    return false;
  const uint64_t StackSize =
      static_cast<uint64_t>((Encoding & UnwindArm64FramelessStackSizeMask) >>
                            FramelessStackSizeShift) *
      StackSizeScale;
  appendDefCFA(Bytes, AArch64DwarfSP, StackSize);
  Bytes.push_back(llvm::dwarf::DW_CFA_same_value);
  appendULEB(Bytes, AArch64DwarfLR);
  appendAArch64SavedPairs(Encoding, 0, Bytes);
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
  constexpr unsigned StackShift = 16;
  constexpr unsigned AdjustShift = 13;
  constexpr unsigned CountShift = 10;
  constexpr uint64_t PointerSize = 8;
  constexpr uint64_t Imm32Size = 4;
  constexpr std::array<Byte, 3> SubRSPImm32Prefix{{0x48, 0x81, 0xEC}};
  const uint32_t Mode = Record.Encoding & UnwindX8664ModeMask;
  if ((Mode != UnwindX8664ModeStackImmediate &&
       Mode != UnwindX8664ModeStackIndirect) ||
      (Record.Encoding &
       ~(UnwindIsNotFunctionStart | UnwindHasLSDA | UnwindX8664ModeMask |
         StackMask | AdjustMask | CountMask | PermutationMask)) != 0)
    return false;
  const uint32_t Count = (Record.Encoding & CountMask) >> CountShift;
  std::vector<uint32_t> Registers;
  if (!decodePermutation(Count, Record.Encoding & PermutationMask, Registers))
    return false;
  uint64_t StackSize = 0;
  if (Mode == UnwindX8664ModeStackImmediate) {
    if ((Record.Encoding & AdjustMask) != 0)
      return false;
    StackSize =
        static_cast<uint64_t>((Record.Encoding & StackMask) >> StackShift) *
        PointerSize;
  } else {
    const auto &Function = Graph.symbols()[Record.Function];
    const auto &Content = Graph.sections()[Function.Section].Content;
    const uint64_t Immediate =
        Function.Offset + ((Record.Encoding & StackMask) >> StackShift);
    const uint64_t FunctionEnd = Function.Offset + Record.Length;
    if (FunctionEnd < Function.Offset ||
        Immediate < Function.Offset + SubRSPImm32Prefix.size() ||
        Immediate > FunctionEnd || FunctionEnd - Immediate < Imm32Size ||
        Immediate > Content.size() || Content.size() - Immediate < Imm32Size ||
        !std::equal(SubRSPImm32Prefix.begin(), SubRSPImm32Prefix.end(),
                    Content.begin() +
                        static_cast<std::ptrdiff_t>(Immediate -
                                                    SubRSPImm32Prefix.size())))
      return false;
    StackSize = read32le(Content.data() + Immediate);
    const uint64_t Adjust =
        static_cast<uint64_t>((Record.Encoding & AdjustMask) >> AdjustShift) *
        PointerSize;
    if (StackSize > UINT64_MAX - Adjust)
      return false;
    StackSize += Adjust;
  }
  if (StackSize < PointerSize * (Count + 1))
    return false;
  appendDefCFA(Bytes, X8664DwarfRSP, StackSize);
  appendOffset(Bytes, X8664DwarfReturnAddress, 1);
  uint64_t Factor = 2;
  for (auto It = Registers.rbegin(); It != Registers.rend(); ++It)
    appendOffset(Bytes, X8664CompactToDwarf[*It], Factor++);
  return true;
}

bool decodeX8664RBP(uint32_t Encoding,
                    std::vector<std::pair<uint32_t, unsigned>> &Saved) {
  constexpr unsigned StackOffsetShift = 16;
  constexpr unsigned RegisterSlots = 5;
  constexpr unsigned RegisterBits = 3;
  constexpr uint32_t RegisterMask = 7;
  if ((Encoding & UnwindX8664ModeMask) != UnwindX8664ModeRBPFrame ||
      (Encoding &
       ~(UnwindIsNotFunctionStart | UnwindHasLSDA | UnwindX8664ModeMask |
         UnwindX8664RBPFrameOffset | UnwindX8664RBPFrameRegisters)) != 0)
    return false;
  const uint32_t StackOffset =
      (Encoding & UnwindX8664RBPFrameOffset) >> StackOffsetShift;
  uint32_t Registers = Encoding & UnwindX8664RBPFrameRegisters;
  std::set<uint32_t> Seen;
  for (unsigned Slot = 0; Slot < RegisterSlots; ++Slot) {
    const uint32_t Register = Registers & RegisterMask;
    Registers >>= RegisterBits;
    if (Register == 0)
      continue;
    if (Register >= X8664CompactToDwarf.size() ||
        Register == X8664CompactRegisterRBP || !Seen.emplace(Register).second)
      return false;
    Saved.emplace_back(X8664CompactToDwarf[Register], Slot);
  }
  if (Registers != 0 ||
      std::any_of(Saved.begin(), Saved.end(), [&](const auto &Value) {
        return StackOffset + 2 < Value.second;
      }))
    return false;
  return true;
}

bool appendX8664CFI(uint32_t Encoding, std::vector<Byte> &Bytes) {
  constexpr unsigned StackOffsetShift = 16;
  constexpr uint64_t FrameRecordSize = 16;
  std::vector<std::pair<uint32_t, unsigned>> Saved;
  if (!decodeX8664RBP(Encoding, Saved))
    return false;
  const uint32_t StackOffset =
      (Encoding & UnwindX8664RBPFrameOffset) >> StackOffsetShift;
  appendDefCFA(Bytes, X8664DwarfRBP, FrameRecordSize);
  appendOffset(Bytes, X8664DwarfReturnAddress, 1);
  appendOffset(Bytes, X8664DwarfRBP, 2);
  for (const auto &[Register, Slot] : Saved)
    appendOffset(Bytes, Register,
                 static_cast<uint64_t>(StackOffset) + 2 - Slot);
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
           Reference.Offset == FDE.Offset + Internal::EHFrameRecordHeaderSize &&
           IsFunction(Reference.Symbol);
  };
  return std::any_of(Graph.ehFrameReferences().begin(),
                     Graph.ehFrameReferences().end(), Matches) ||
         std::any_of(Graph.relocations().begin(), Graph.relocations().end(),
                     Matches);
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
  UnwindPageKind Kind;
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

Expect<std::vector<NativeRecord>>
mergeAdjacentRecords(const LinkGraph &Graph,
                     std::vector<NativeRecord> Records) {
  std::sort(Records.begin(), Records.end(),
            [](const auto &L, const auto &R) { return L.Address < R.Address; });
  std::vector<NativeRecord> Merged;
  for (const auto &Record : Records) {
    if (!Merged.empty()) {
      const auto &Previous = Merged.back();
      const bool StackIndirect =
          Graph.target() == Target::X86_64 &&
          (Record.Encoding & Internal::CompactUnwindModeMask) ==
              UnwindX8664ModeStackIndirect;
      if (Previous.Address <= UINT64_MAX - Previous.Length &&
          Previous.Address + Previous.Length == Record.Address &&
          Previous.Encoding == Record.Encoding && !Previous.LSDA &&
          !Record.LSDA &&
          !Internal::isDwarfCompactUnwind(Graph.target(), Record.Encoding) &&
          !StackIndirect) {
        if (Record.Length > UINT32_MAX - Merged.back().Length)
          return fail();
        Merged.back().Length += Record.Length;
        continue;
      }
    }
    Merged.push_back(Record);
  }
  return Merged;
}

void assignCommonEncodings(NativeLayout &Layout) {
  std::map<uint32_t, size_t> Frequencies;
  for (const auto &Record : Layout.Records)
    ++Frequencies[Record.Encoding];
  std::vector<std::pair<uint32_t, size_t>> Ordered(Frequencies.begin(),
                                                   Frequencies.end());
  std::sort(Ordered.begin(), Ordered.end(), [](const auto &L, const auto &R) {
    return std::tie(L.second, L.first) > std::tie(R.second, R.first);
  });
  const size_t CommonCount =
      std::min(Layout.ReservedCommonCount, Ordered.size());
  for (size_t I = 0; I < CommonCount; ++I) {
    Layout.CommonIndexes.emplace(Ordered[I].first, static_cast<uint8_t>(I));
    Layout.CommonEncodings.push_back(Ordered[I].first);
  }
}

void buildPages(NativeLayout &Layout) {
  for (size_t Begin = 0; Begin < Layout.Records.size();) {
    const size_t RegularCount =
        std::min(RegularPageCapacity, Layout.Records.size() - Begin);
    // A compressed second-level page packs a 24-bit function delta and an
    // 8-bit common/local encoding index into each entry.
    NativePage Compressed{
        UnwindPageKind::Compressed, Begin, 0, {}, Layout.CommonIndexes};
    size_t Used = CompressedPageHeaderSize;
    for (size_t I = Begin; I < Layout.Records.size(); ++I) {
      const auto &Record = Layout.Records[I];
      if (Record.Address - Layout.Records[Begin].Address >
          CompressedFunctionOffsetMask)
        break;
      auto Encoding = Compressed.EncodingIndexes.find(Record.Encoding);
      if (Encoding == Compressed.EncodingIndexes.end()) {
        if (Compressed.EncodingIndexes.size() == MaxEncodingIndexes ||
            Used + UnwindEncodingSize + CompressedPageEntrySize >
                UnwindPageSize)
          break;
        const auto Index =
            static_cast<uint8_t>(Compressed.EncodingIndexes.size());
        Compressed.EncodingIndexes.emplace(Record.Encoding, Index);
        Compressed.LocalEncodings.push_back(Record.Encoding);
        Used += UnwindEncodingSize;
      }
      if (Used + CompressedPageEntrySize > UnwindPageSize)
        break;
      Used += CompressedPageEntrySize;
      ++Compressed.Count;
    }
    const size_t RegularUsed =
        RegularPageHeaderSize + RegularCount * RegularPageEntrySize;
    if (Compressed.Count < RegularCount ||
        (Compressed.Count == RegularCount && Used >= RegularUsed)) {
      // A regular page stores full function offsets and encodings.
      Layout.Pages.push_back(
          {UnwindPageKind::Regular, Begin, RegularCount, {}, {}});
      Begin += RegularCount;
    } else {
      Begin += Compressed.Count;
      Layout.Pages.push_back(std::move(Compressed));
    }
  }
}

Expect<NativeLayout> buildNativeLayout(const LinkGraph &Graph, bool Final) {
  if (Graph.format() != ObjectFormat::MachO ||
      Graph.endianness() != Endianness::Little ||
      (Graph.target() != Target::X86_64 && Graph.target() != Target::AArch64) ||
      Graph.compactUnwind().empty())
    return fail();
  NativeLayout Result;
  std::set<uint32_t> ReservedEncodings;
  for (const auto &Record : Graph.compactUnwind()) {
    if (Record.Personality || (Record.Encoding & UnwindPersonalityMask) != 0 ||
        (((Record.Encoding & UnwindHasLSDA) != 0) !=
         static_cast<bool>(Record.LSDA)))
      return fail();
    const bool Dwarf =
        Internal::isDwarfCompactUnwind(Graph.target(), Record.Encoding);
    uint32_t Encoding = Record.Encoding;
    if (Dwarf) {
      SectionId EHFrame = InvalidSectionId;
      if (!hasAssociatedFDE(Graph, Record) ||
          !canonicalEHFrameSection(Graph, EHFrame) ||
          Graph.symbols()[*Record.FDE].Section != EHFrame)
        return fail();
      const auto &FDE = Graph.symbols()[*Record.FDE];
      if (FDE.Offset > Internal::CompactUnwindDwarfOffsetMask)
        return fail();
      Encoding = (Encoding & ~Internal::CompactUnwindDwarfOffsetMask) |
                 static_cast<uint32_t>(FDE.Offset);
    } else {
      std::vector<Byte> Ignored;
      if (Record.FDE || !appendCFI(Graph, Record, Ignored))
        return fail();
    }
    if (Record.LSDA)
      ++Result.LSDACount;
    ReservedEncodings.insert(Encoding);
    if (!Final)
      continue;
    const auto FunctionAddress = symbolAddress(Graph, Record.Function);
    std::optional<uint64_t> LSDAAddress;
    if (Record.LSDA)
      LSDAAddress = symbolAddress(Graph, *Record.LSDA);
    if (!FunctionAddress || (Record.LSDA && !LSDAAddress))
      return fail();
    Result.Records.push_back(
        {*FunctionAddress, Record.Length, Encoding, LSDAAddress});
  }
  // Second-level entries cover the address range up to the next entry, so a
  // function described only by an independent FDE needs an explicit DWARF-mode
  // entry rather than inheriting the preceding encoding.
  std::set<std::pair<SectionId, uint64_t>> CoveredFunctions;
  for (const auto &Record : Graph.compactUnwind()) {
    if (Record.Function >= Graph.symbols().size())
      return fail();
    const auto &Function = Graph.symbols()[Record.Function];
    CoveredFunctions.emplace(Function.Section, Function.Offset);
  }
  size_t SynthesizedCount = 0;
  SectionId EHFrameSection = InvalidSectionId;
  if (canonicalEHFrameSection(Graph, EHFrameSection)) {
    const uint32_t DwarfMode = Internal::compactUnwindDwarfMode(Graph.target());
    const auto &Content = Graph.sections()[EHFrameSection].Content;
    std::set<std::pair<SectionId, uint64_t>> Synthesized;
    for (const auto &Reference : Graph.ehFrameReferences()) {
      if (Reference.Section != EHFrameSection ||
          Reference.Symbol >= Graph.symbols().size() ||
          Reference.Offset < Internal::EHFrameRecordHeaderSize)
        continue;
      const auto &Function = Graph.symbols()[Reference.Symbol];
      const auto Identity = std::make_pair(Function.Section, Function.Offset);
      if (CoveredFunctions.count(Identity) != 0 ||
          !Synthesized.emplace(Identity).second)
        continue;
      const uint64_t FDEOffset =
          Reference.Offset - Internal::EHFrameRecordHeaderSize;
      if (FDEOffset > Internal::CompactUnwindDwarfOffsetMask)
        return fail();
      const auto Length = Internal::readUnsigned(
          Content, Reference.Offset + Internal::EHFramePointerWidth,
          Internal::EHFramePointerWidth, Endianness::Little);
      if (!Length || *Length > UINT32_MAX)
        return fail();
      const uint32_t Encoding = DwarfMode | static_cast<uint32_t>(FDEOffset);
      ReservedEncodings.insert(Encoding);
      ++SynthesizedCount;
      if (!Final)
        continue;
      const auto FunctionAddress = symbolAddress(Graph, Reference.Symbol);
      if (!FunctionAddress)
        return fail();
      Result.Records.push_back({*FunctionAddress,
                                static_cast<uint32_t>(*Length), Encoding,
                                std::nullopt});
    }
  }
  Result.ReservedCommonCount =
      std::min(ReservedEncodings.size(), MaxCommonEncodings);
  Result.ReservedPageCount = (Graph.compactUnwind().size() + SynthesizedCount +
                              RegularPageCapacity - 1) /
                             RegularPageCapacity;
  if (!Final)
    return Result;

  EXPECTED_TRY(Result.Records,
               mergeAdjacentRecords(Graph, std::move(Result.Records)));
  assignCommonEncodings(Result);
  buildPages(Result);
  if (Result.Pages.size() > Result.ReservedPageCount)
    return fail();
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
  if (!AddProduct(Layout.ReservedCommonCount, UnwindEncodingSize) ||
      Layout.ReservedPageCount == UINT64_MAX ||
      !AddProduct(Layout.ReservedPageCount + 1, UnwindIndexSize) ||
      !AddProduct(Layout.LSDACount, UnwindLSDASize) ||
      !AddProduct(Layout.ReservedPageCount, UnwindPageSize) ||
      Result > UINT32_MAX)
    return fail();
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
  if (!Internal::fitsSizeT(Size))
    return fail();
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
      CommonOffset + Layout.ReservedCommonCount * UnwindEncodingSize;
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
  const auto Put32 = [&Content](size_t Offset, uint32_t Value) {
    write32le(Content.data() + Offset, Value);
  };
  const auto Put16 = [&Content](size_t Offset, uint16_t Value) {
    write16le(Content.data() + Offset, Value);
  };
  Put32(UnwindHeaderVersionOffset, UnwindInfoVersion);
  Put32(UnwindHeaderCommonEncodingsOffset, CommonOffset32);
  Put32(UnwindHeaderCommonEncodingsCountOffset, CommonCount32);
  Put32(UnwindHeaderPersonalitiesOffset, PersonalityOffset32);
  Put32(UnwindHeaderPersonalitiesCountOffset, 0);
  Put32(UnwindHeaderIndexOffset, IndexOffset32);
  Put32(UnwindHeaderIndexCountOffset, IndexCount32);
  for (size_t I = 0; I < Layout.CommonEncodings.size(); ++I)
    Put32(CommonOffset + I * UnwindEncodingSize, Layout.CommonEncodings[I]);

  size_t LSDAIndex = 0;
  for (size_t I = 0; I < Layout.Pages.size(); ++I) {
    const auto &Page = Layout.Pages[I];
    uint32_t FunctionDelta = 0;
    if (!delta32(Layout.Records[Page.Begin].Address, ImageBase, FunctionDelta))
      return fail();
    Put32(IndexOffset + I * UnwindIndexSize + UnwindIndexFunctionOffset,
          FunctionDelta);
    uint32_t CurrentPageOffset = 0;
    uint32_t CurrentLSDAOffset = 0;
    if (!narrow32(PageOffset + I * UnwindPageSize, CurrentPageOffset) ||
        !narrow32(LSDAOffset + LSDAIndex * UnwindLSDASize, CurrentLSDAOffset))
      return fail();
    Put32(IndexOffset + I * UnwindIndexSize + UnwindIndexPageOffset,
          CurrentPageOffset);
    Put32(IndexOffset + I * UnwindIndexSize + UnwindIndexLSDAOffset,
          CurrentLSDAOffset);
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
  Put32(Sentinel + UnwindIndexFunctionOffset, EndDelta);
  uint32_t LSDAEndOffset = 0;
  if (!narrow32(LSDAOffset + Layout.LSDACount * UnwindLSDASize, LSDAEndOffset))
    return fail();
  Put32(Sentinel + UnwindIndexLSDAOffset, LSDAEndOffset);

  size_t LSDAWrite = LSDAOffset;
  for (const auto &Record : Layout.Records) {
    if (!Record.LSDA)
      continue;
    uint32_t FunctionDelta = 0;
    uint32_t LSDADelta = 0;
    if (!delta32(Record.Address, ImageBase, FunctionDelta) ||
        !delta32(*Record.LSDA, ImageBase, LSDADelta))
      return fail();
    Put32(LSDAWrite + UnwindLSDAFunctionOffset, FunctionDelta);
    Put32(LSDAWrite + UnwindLSDAOffset, LSDADelta);
    LSDAWrite += UnwindLSDASize;
  }

  for (size_t I = 0; I < Layout.Pages.size(); ++I) {
    const auto &Page = Layout.Pages[I];
    const size_t Offset = PageOffset + I * UnwindPageSize;
    Put32(Offset + UnwindPageKindOffset, static_cast<uint32_t>(Page.Kind));
    Put16(Offset + UnwindPageEntryCountOffset,
          static_cast<uint16_t>(Page.Count));
    if (Page.Kind == UnwindPageKind::Regular) {
      Put16(Offset + UnwindPageEntriesOffset, RegularPageHeaderSize);
      for (size_t J = 0; J < Page.Count; ++J) {
        const auto &Record = Layout.Records[Page.Begin + J];
        uint32_t FunctionDelta = 0;
        if (!delta32(Record.Address, ImageBase, FunctionDelta))
          return fail();
        const size_t Entry =
            Offset + RegularPageHeaderSize + J * RegularPageEntrySize;
        Put32(Entry, FunctionDelta);
        Put32(Entry + RegularEntryEncodingOffset, Record.Encoding);
      }
    } else {
      const uint16_t EncodingOffset = static_cast<uint16_t>(
          CompressedPageHeaderSize + Page.Count * CompressedPageEntrySize);
      Put16(Offset + UnwindPageEntriesOffset, CompressedPageHeaderSize);
      Put16(Offset + UnwindCompressedPageEncodingsOffset, EncodingOffset);
      Put16(Offset + UnwindCompressedPageEncodingCountOffset,
            static_cast<uint16_t>(Page.LocalEncodings.size()));
      for (size_t J = 0; J < Page.Count; ++J) {
        const auto &Record = Layout.Records[Page.Begin + J];
        const uint64_t FunctionOffset =
            Record.Address - Layout.Records[Page.Begin].Address;
        const auto Encoding = Page.EncodingIndexes.find(Record.Encoding);
        if (Encoding == Page.EncodingIndexes.end() ||
            FunctionOffset > CompressedFunctionOffsetMask)
          return fail();
        Put32(Offset + CompressedPageHeaderSize + J * CompressedPageEntrySize,
              (static_cast<uint32_t>(Encoding->second)
               << CompressedEncodingIndexShift) |
                  static_cast<uint32_t>(FunctionOffset));
      }
      for (size_t J = 0; J < Page.LocalEncodings.size(); ++J)
        Put32(Offset + EncodingOffset + J * UnwindEncodingSize,
              Page.LocalEncodings[J]);
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
    if (Internal::isDwarfCompactUnwind(Graph.target(), Record.Encoding)) {
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
    const auto Terminator =
        Internal::ehFrameTerminatorOffset(Graph.sections()[I].Content);
    if (!Terminator)
      return fail();
    ExistingSection = I;
    Synthetic = Graph.sections()[I];
    Content.assign(Synthetic.Content.data(),
                   Synthetic.Content.data() + *Terminator);
    break;
  }
  const bool AArch64 = Graph.target() == Target::AArch64;
  const size_t CIE = Content.size();
  append32(Content, 0);
  append32(Content, 0);
  Content.push_back(Internal::EHFrameCIEVersion);
  Content.insert(Content.end(), Internal::EHFrameCIEAugmentation.begin(),
                 Internal::EHFrameCIEAugmentation.end());
  Content.push_back(0);
  appendULEB(Content, AArch64 ? Internal::EHFrameAArch64CIECodeAlignment
                              : Internal::EHFrameCIECodeAlignment);
  appendSLEB(Content, Internal::EHFrameCIEDataAlignment);
  appendULEB(Content, AArch64 ? AArch64DwarfLR : X8664DwarfReturnAddress);
  appendULEB(Content, Internal::EHFrameCIEAugmentationLength);
  Content.push_back(Internal::EHFrameFDEPointerEncoding);
  finishRecord(Content, CIE);

  std::vector<EHFrameReference> References;
  References.reserve(FDEs.size());
  for (const auto &[Function, Length, CFI] : FDEs) {
    const size_t FDE = Content.size();
    append32(Content, 0);
    append32(Content, static_cast<uint32_t>(
                          FDE + Internal::EHFrameLengthFieldSize - CIE));
    const uint64_t FunctionField = Content.size();
    append64(Content, 0);
    append64(Content, Length);
    appendULEB(Content, 0);
    Content.insert(Content.end(), CFI.begin(), CFI.end());
    finishRecord(Content, FDE);
    References.push_back(
        EHFrameReference{InvalidSectionId, FunctionField, Function});
  }
  Content.resize(Content.size() + Internal::EHFrameLengthFieldSize);
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
    if (Internal::isDwarfCompactUnwind(Graph.target(), Record.Encoding)) {
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
