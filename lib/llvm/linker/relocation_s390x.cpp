// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <llvm/BinaryFormat/ELF.h>

#include <string_view>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {
namespace {

LinkExpect<RelocationResult>
fail(const Relocation &Value, std::string_view Message,
     DiagnosticKind Kind = DiagnosticKind::Malformed) {
  Diagnostic Diag{std::string(Message)};
  Diag.Section = Value.Section;
  Diag.Symbol = Value.Symbol;
  Diag.RelocationType = Value.Type;
  Diag.Offset = Value.Offset;
  Diag.Kind = Kind;
  return Unexpected<Diagnostic>(std::move(Diag));
}

bool addUnsigned(uint64_t Left, uint64_t Right, uint64_t &Result) noexcept {
  if (Right > UINT64_MAX - Left)
    return false;
  Result = Left + Right;
  return true;
}

bool addSigned(uint64_t Base, int64_t Delta, uint64_t &Result) noexcept {
  if (Delta >= 0)
    return addUnsigned(Base, static_cast<uint64_t>(Delta), Result);
  const uint64_t Magnitude = static_cast<uint64_t>(-(Delta + 1)) + 1;
  if (Magnitude > Base)
    return false;
  Result = Base - Magnitude;
  return true;
}

bool signedDelta(uint64_t Left, uint64_t Right, int64_t &Result) noexcept {
  if (Left >= Right) {
    const uint64_t Difference = Left - Right;
    if (Difference > static_cast<uint64_t>(INT64_MAX))
      return false;
    Result = static_cast<int64_t>(Difference);
    return true;
  }
  const uint64_t Difference = Right - Left;
  const uint64_t MinimumMagnitude = UINT64_C(1) << 63;
  if (Difference > MinimumMagnitude)
    return false;
  Result = Difference == MinimumMagnitude ? INT64_MIN
                                          : -static_cast<int64_t>(Difference);
  return true;
}

bool signedBits(int64_t Value, unsigned Bits) noexcept {
  if (Bits == 64)
    return true;
  const int64_t Limit = INT64_C(1) << (Bits - 1);
  return Value >= -Limit && Value < Limit;
}

} // namespace

LinkExpect<RelocationResult> applyS390X(const LinkGraph &Graph) {
  constexpr uint8_t HalfWordScale = 2;
  constexpr uint8_t WordWidth = 4;
  constexpr unsigned WordBits = 32;
  RelocationResult Result;
  Result.Content.reserve(Graph.sections().size());
  for (const auto &Section : Graph.sections()) {
    Result.Content.push_back(Section.Content);
  }
  Result.Rebases = Graph.rebases();
  RebaseIntervalIndex RebaseIntervals(Result.Rebases);
  for (const auto &Rel : Graph.relocations()) {
    if (Rel.Format != ObjectFormat::ELF) {
      return fail(Rel, "invalid relocation field");
    }
    auto &Bytes = Result.Content[Rel.Section];
    const auto &Symbol = Graph.symbols()[Rel.Symbol];
    uint64_t S = 0;
    uint64_t P = 0;
    if (!addUnsigned(Graph.sections()[Symbol.Section].Address, Symbol.Offset,
                     S) ||
        !addSigned(S, Rel.Addend, S) ||
        !addUnsigned(Graph.sections()[Rel.Section].Address, Rel.Offset, P)) {
      return fail(Rel, "absolute relocation overflows");
    }
    if (Rel.Type == llvm::ELF::R_390_64) {
      constexpr uint8_t DoubleWordWidth = 8;
      if (!writeUnsigned(Bytes, Rel.Offset, DoubleWordWidth, Endianness::Big,
                         S)) {
        return fail(Rel, "absolute relocation overflows");
      }
      if (!RebaseIntervals.insert(Rel.Section, Rel.Offset, DoubleWordWidth)) {
        return fail(Rel, "overlapping generated rebase");
      }
      Result.Rebases.push_back(Rebase{Rel.Section, Rel.Offset, Rel.Type,
                                      Rel.Addend, DoubleWordWidth});
      continue;
    }
    int64_t Value = 0;
    if (!signedDelta(S, P, Value)) {
      return fail(Rel, "PC-relative relocation overflows");
    }
    if (Rel.Type == llvm::ELF::R_390_PC32DBL ||
        Rel.Type == llvm::ELF::R_390_PLT32DBL) {
      if ((Value & (HalfWordScale - 1)) != 0) {
        return fail(Rel, "doubled displacement is not even");
      }
      Value /= HalfWordScale;
    } else if (Rel.Type != llvm::ELF::R_390_PC32) {
      return fail(Rel, "unsupported s390x relocation type",
                  DiagnosticKind::Unsupported);
    }
    if (!signedBits(Value, WordBits) ||
        !writeSigned(Bytes, Rel.Offset, WordWidth, Endianness::Big, Value)) {
      return fail(Rel, "PC-relative relocation overflows");
    }
  }
  return Result;
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
