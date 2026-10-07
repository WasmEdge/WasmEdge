// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/layout.h"

#include "linker/byte_io.h"

#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

enum class SectionOrder : uint8_t {
  Text,
  ReadOnly,
  Unwind,
  Data,
  BSS,
  Invalid = UINT8_MAX,
};

SectionOrder kindOrder(SectionKind Kind) noexcept {
  switch (Kind) {
  case SectionKind::Text:
    return SectionOrder::Text;
  case SectionKind::ReadOnly:
    return SectionOrder::ReadOnly;
  case SectionKind::Unwind:
    return SectionOrder::Unwind;
  case SectionKind::Data:
    return SectionOrder::Data;
  case SectionKind::BSS:
    return SectionOrder::BSS;
  }
  return SectionOrder::Invalid;
}

LinkExpect<void> overflow(const Section &SectionValue, SectionId Id,
                          const char *Message) {
  Diagnostic Diag{Message};
  Diag.Section = Id;
  Diag.SectionName = SectionValue.Name;
  return diagnosticError(std::move(Diag));
}

} // namespace

std::vector<SectionId> Internal::sectionsOfKind(const LinkGraph &Graph,
                                                SectionKind Kind) {
  const auto &Sections = Graph.sections();
  std::vector<SectionId> Result;
  for (SectionId Id = 0; Id < Sections.size(); ++Id)
    if (Sections[Id].Kind == Kind)
      Result.push_back(Id);
  std::sort(Result.begin(), Result.end(), [&](SectionId Left, SectionId Right) {
    return std::tie(Sections[Left].Name, Left) <
           std::tie(Sections[Right].Name, Right);
  });
  return Result;
}

LinkExpect<void> Internal::applyPlacements(LinkGraph &Graph,
                                           Span<const Placement> Placements) {
  for (SectionId Id = 0; Id < Placements.size(); ++Id) {
    EXPECTED_TRY(Graph.setSectionAddress(Id, Placements[Id].Address));
    EXPECTED_TRY(Graph.setSectionFileOffset(Id, Placements[Id].FileOffset));
  }
  return {};
}

LinkExpect<void> layout(LinkGraph &Graph, uint64_t ImageBase,
                        uint64_t SegmentAlignment) {
  if (!llvm::isPowerOf2_64(SegmentAlignment)) {
    return diagnosticError("invalid segment alignment");
  }
  if (Graph.relocationsApplied()) {
    return diagnosticError("cannot layout relocated link graph");
  }
  EXPECTED_TRY(Graph.validate());

  const auto &Sections = Graph.sections();
  std::vector<SectionId> Order(Sections.size());
  std::iota(Order.begin(), Order.end(), SectionId{0});
  std::sort(Order.begin(), Order.end(), [&](SectionId Left, SectionId Right) {
    const auto &L = Sections[Left];
    const auto &R = Sections[Right];
    return std::tuple(kindOrder(L.Kind), L.Name, Left) <
           std::tuple(kindOrder(R.Kind), R.Name, Right);
  });

  std::vector<Internal::Placement> Placements(Sections.size());
  uint64_t Address = ImageBase;
  uint64_t FileOffset = 0;
  SectionOrder Previous = SectionOrder::Invalid;
  for (const SectionId Id : Order) {
    const auto &SectionValue = Sections[Id];
    const auto Current = kindOrder(SectionValue.Kind);
    if (Previous != SectionOrder::Invalid && Current != Previous &&
        !Internal::checkedAlign(Address, SegmentAlignment, Address)) {
      return overflow(SectionValue, Id, "section segment alignment overflows");
    }
    if (!Internal::checkedAlign(Address, SectionValue.Alignment, Address)) {
      return overflow(SectionValue, Id, "section address alignment overflows");
    }
    if (SectionValue.Kind != SectionKind::BSS &&
        !Internal::checkedAlign(FileOffset, SectionValue.Alignment,
                                FileOffset)) {
      return overflow(SectionValue, Id,
                      "section file offset alignment overflows");
    }
    Placements[Id] = Internal::Placement{
        Address, SectionValue.Kind == SectionKind::BSS ? 0 : FileOffset};
    if (!Internal::addUnsigned(Address, SectionValue.VirtualSize, Address)) {
      return overflow(SectionValue, Id, "section virtual size overflows");
    }
    if (SectionValue.Kind != SectionKind::BSS &&
        !Internal::addUnsigned(FileOffset, SectionValue.Content.size(),
                               FileOffset)) {
      return overflow(SectionValue, Id, "section file size overflows");
    }
    Previous = Current;
  }

  return Internal::applyPlacements(Graph, Placements);
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
