// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Support/MathExtras.h>

namespace WasmEdge {
namespace LLVM {
namespace Linker {
namespace Internal {

LinkExpect<RelocationResult> applyS390X(const LinkGraph &Graph) {
  constexpr uint8_t HalfWordScale = 2;
  constexpr uint8_t WordWidth = 4;
  constexpr uint8_t DoubleWordWidth = 8;
  RelocationWriter Writer(Graph);
  for (const auto &Rel : Graph.relocations()) {
    if (Rel.Format != ObjectFormat::ELF) {
      return relocationError(Graph, Rel, "unsupported object format",
                             DiagnosticKind::Unsupported);
    }
    auto &Bytes = Writer.content(Rel.Section);
    EXPECTED_TRY(uint64_t S, symbolAddress(Graph, Rel));
    if (!addSigned(S, Rel.Addend, S)) {
      return relocationError(Graph, Rel, "relocation address overflows");
    }
    EXPECTED_TRY(const uint64_t P, placeAddress(Graph, Rel));
    if (Rel.Type == llvm::ELF::R_390_64) {
      EXPECTED_TRY(Writer.writeAbsolute(Rel, DoubleWordWidth, Endianness::Big,
                                        S, Rel.Addend));
      continue;
    }
    int64_t Value = 0;
    if (!signedDelta(S, P, Value)) {
      return relocationError(Graph, Rel, "PC-relative relocation overflows");
    }
    if (Rel.Type == llvm::ELF::R_390_PC32DBL ||
        Rel.Type == llvm::ELF::R_390_PLT32DBL) {
      if ((Value & (HalfWordScale - 1)) != 0) {
        return relocationError(Graph, Rel, "doubled displacement is not even");
      }
      Value /= HalfWordScale;
    } else if (Rel.Type != llvm::ELF::R_390_PC32) {
      return relocationError(Graph, Rel, "unsupported s390x relocation type",
                             DiagnosticKind::Unsupported);
    }
    if (!llvm::isInt<32>(Value) ||
        !writeSigned(Bytes, Rel.Offset, WordWidth, Endianness::Big, Value)) {
      return relocationError(Graph, Rel, "PC-relative relocation overflows");
    }
  }
  return std::move(Writer).finish();
}

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
