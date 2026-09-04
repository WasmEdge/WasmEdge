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

bool signedBits(int128_t Value, unsigned Bits) {
  const int128_t Limit = int128_t(1) << (Bits - 1);
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
    const int128_t S = int128_t(Graph.sections()[Symbol.Section].Address) +
                       Symbol.Offset + Rel.Addend;
    const int128_t P =
        int128_t(Graph.sections()[Rel.Section].Address) + Rel.Offset;
    if (Rel.Type == llvm::ELF::R_390_64) {
      constexpr uint8_t DoubleWordWidth = 8;
      if (S < 0 || S > UINT64_MAX ||
          !writeUnsigned(Bytes, Rel.Offset, DoubleWordWidth, Endianness::Big,
                         static_cast<uint64_t>(S))) {
        return fail(Rel, "absolute relocation overflows");
      }
      if (!RebaseIntervals.insert(Rel.Section, Rel.Offset, DoubleWordWidth)) {
        return fail(Rel, "overlapping generated rebase");
      }
      Result.Rebases.push_back(Rebase{Rel.Section, Rel.Offset, Rel.Type,
                                      Rel.Addend, DoubleWordWidth});
      continue;
    }
    int128_t Value = S - P;
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
        !writeSigned(Bytes, Rel.Offset, WordWidth, Endianness::Big,
                     static_cast<int64_t>(Value))) {
      return fail(Rel, "PC-relative relocation overflows");
    }
  }
  return Result;
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
