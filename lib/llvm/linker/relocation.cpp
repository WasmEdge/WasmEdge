// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

constexpr uint8_t BitsPerByte = 8;
constexpr uint8_t ByteField = 1;
constexpr uint8_t HalfField = 2;
constexpr uint8_t WordField = 4;
constexpr uint8_t DoubleWordField = 8;

bool validField(Span<const Byte> Bytes, uint64_t Offset,
                uint8_t Width) noexcept {
  return (Width == ByteField || Width == HalfField || Width == WordField ||
          Width == DoubleWordField) &&
         Offset <= Bytes.size() && Width <= Bytes.size() - Offset;
}

template <typename T> Expect<T> fieldError() noexcept {
  return Unexpect(ErrCode::Value::IllegalPath);
}

} // namespace

namespace Internal {

RebaseIntervalIndex::RebaseIntervalIndex(Span<const Rebase> Rebases) {
  for (const auto &Value : Rebases) {
    Intervals.emplace(std::make_pair(Value.Section, Value.Offset),
                      std::max<uint8_t>(Value.Width, MinimumRebaseWidth));
  }
}

bool RebaseIntervalIndex::insert(SectionId Section, uint64_t Offset,
                                 uint8_t Width) {
  Width = std::max<uint8_t>(Width, MinimumRebaseWidth);
  const auto Key = std::make_pair(Section, Offset);
  const auto Next = Intervals.lower_bound(Key);
  if (Next != Intervals.end() && Next->first.first == Section &&
      Width > Next->first.second - Offset) {
    return false;
  }
  if (Next != Intervals.begin()) {
    const auto Previous = std::prev(Next);
    if (Previous->first.first == Section &&
        Previous->second > Offset - Previous->first.second) {
      return false;
    }
  }
  Intervals.emplace_hint(Next, Key, Width);
  return true;
}

Expect<uint64_t> readUnsigned(Span<const Byte> Bytes, uint64_t Offset,
                              uint8_t Width, Endianness Endian) noexcept {
  if (!validField(Bytes, Offset, Width)) {
    return fieldError<uint64_t>();
  }
  uint64_t Value = 0;
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift =
        Endian == Endianness::Little
            ? static_cast<uint8_t>(I * BitsPerByte)
            : static_cast<uint8_t>((Width - I - 1) * BitsPerByte);
    Value |= static_cast<uint64_t>(Bytes[Offset + I]) << Shift;
  }
  return Value;
}

Expect<int64_t> readSigned(Span<const Byte> Bytes, uint64_t Offset,
                           uint8_t Width, Endianness Endian) noexcept {
  auto Value = readUnsigned(Bytes, Offset, Width, Endian);
  if (!Value) {
    return fieldError<int64_t>();
  }
  if (Width == DoubleWordField) {
    int64_t Result = 0;
    std::memcpy(&Result, &*Value, sizeof(Result));
    return Result;
  }
  const uint8_t Bits = static_cast<uint8_t>(Width * BitsPerByte);
  if ((*Value & (UINT64_C(1) << (Bits - 1))) == 0) {
    return static_cast<int64_t>(*Value);
  }
  return static_cast<int64_t>(*Value | (~UINT64_C(0) << Bits));
}

Expect<void> writeUnsigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                           Endianness Endian, uint64_t Value) noexcept {
  if (!validField(Span<const Byte>(Bytes.data(), Bytes.size()), Offset,
                  Width) ||
      (Width < DoubleWordField &&
       Value >= (UINT64_C(1) << (Width * BitsPerByte)))) {
    return fieldError<void>();
  }
  for (uint8_t I = 0; I < Width; ++I) {
    const uint8_t Shift =
        Endian == Endianness::Little
            ? static_cast<uint8_t>(I * BitsPerByte)
            : static_cast<uint8_t>((Width - I - 1) * BitsPerByte);
    Bytes[Offset + I] = static_cast<Byte>(Value >> Shift);
  }
  return {};
}

Expect<void> writeSigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                         Endianness Endian, int64_t Value) noexcept {
  if (Width != ByteField && Width != HalfField && Width != WordField &&
      Width != DoubleWordField) {
    return fieldError<void>();
  }
  if (Width < DoubleWordField) {
    const uint8_t Bits = static_cast<uint8_t>(Width * BitsPerByte);
    const int64_t Minimum = -(INT64_C(1) << (Bits - 1));
    const int64_t Maximum = (INT64_C(1) << (Bits - 1)) - 1;
    if (Value < Minimum || Value > Maximum) {
      return fieldError<void>();
    }
  }
  uint64_t Bits = 0;
  std::memcpy(&Bits, &Value, sizeof(Bits));
  if (Width < DoubleWordField) {
    Bits &= (UINT64_C(1) << (Width * BitsPerByte)) - 1;
  }
  return writeUnsigned(Bytes, Offset, Width, Endian, Bits);
}

} // namespace Internal

LinkExpect<void> applyRelocations(LinkGraph &Graph) {
  if (Graph.RelocationsApplied ||
      Graph.UnwindInfoState == MachOUnwindInfoState::Populated) {
    return Unexpected<Diagnostic>(
        Diagnostic{"link graph relocations already applied"});
  }
  if (auto Valid = Graph.validate(); !Valid)
    return Unexpected<Diagnostic>(std::move(Valid.error()));
  const bool CorrectEndian = Graph.target() == Target::S390X
                                 ? Graph.endianness() == Endianness::Big
                                 : Graph.endianness() == Endianness::Little;
  if (!CorrectEndian) {
    return Unexpected<Diagnostic>(
        Diagnostic{"target input has wrong endianness"});
  }
  Diagnostic Unsupported{"unsupported relocation target"};
  Unsupported.Kind = DiagnosticKind::Unsupported;
  LinkExpect<Internal::RelocationResult> Result =
      Unexpected<Diagnostic>(std::move(Unsupported));
  switch (Graph.target()) {
  case Target::X86_64:
#if WASMEDGE_LINKER_HAS_X86_64
    Result = Internal::applyX86_64(Graph);
#endif
    break;
  case Target::ARM:
#if WASMEDGE_LINKER_HAS_ARM
    Result = Internal::applyARM(Graph);
#endif
    break;
  case Target::AArch64:
#if WASMEDGE_LINKER_HAS_AARCH64
    Result = Internal::applyAArch64(Graph);
#endif
    break;
  case Target::RISCV64:
#if WASMEDGE_LINKER_HAS_RISCV64
    Result = Internal::applyRISCV(Graph);
#endif
    break;
  case Target::S390X:
#if WASMEDGE_LINKER_HAS_S390X
    Result = Internal::applyS390X(Graph);
#endif
    break;
  }
  if (!Result)
    return Unexpected<Diagnostic>(std::move(Result.error()));
  if (Result->Content.size() != Graph.Sections.size())
    return Unexpected<Diagnostic>(
        Diagnostic{"relocation backend returned wrong section count"});
  LinkGraph::RelocationIntervalMap NewRelocationIntervals;
  LinkGraph::RebaseIntervalMap NewRebaseIntervals;
  EXPECTED_TRY(LinkGraph::buildPatchIntervals(
      Graph.Relocations, Result->Rebases, NewRelocationIntervals,
      NewRebaseIntervals));
  for (size_t I = 0; I < Result->Content.size(); ++I) {
    Graph.Sections[I].Content.swap(Result->Content[I]);
  }
  Graph.Rebases.swap(Result->Rebases);
  Graph.RelocationIntervals.swap(NewRelocationIntervals);
  Graph.RebaseIntervals.swap(NewRebaseIntervals);
  Graph.RelocationsApplied = true;
  return {};
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
