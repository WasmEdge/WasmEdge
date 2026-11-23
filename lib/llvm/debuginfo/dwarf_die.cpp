// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/dwarf_die.h"
#include "debuginfo/dwarf_reader.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/Support/LEB128.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

namespace {

#if LLVM_VERSION_MAJOR < 10
std::optional<std::vector<LocationEntry>>
locationList(DWARFDie D, const llvm::DWARFFormValue &Attr) noexcept {
  auto Offset = Attr.getAsSectionOffset();
  if (!Offset) {
    return std::nullopt;
  }
  auto *U = D.getDwarfUnit();
  const auto Section = U->getContext().getDWARFObj().getLocSection().Data;
  llvm::DataExtractor Data(Section, true, U->getAddressByteSize());
  const uint64_t Max = U->getAddressByteSize() == 4 ? UINT32_MAX : UINT64_MAX;
  uint64_t Base = 0;
  if (auto B = U->getBaseAddress()) {
    Base = B->Address;
  }
  uint32_t Cursor = static_cast<uint32_t>(*Offset);
  const auto Convention = locationConvention(U);
  std::vector<LocationEntry> Entries;
  while (Data.isValidOffsetForDataOfSize(Cursor, 2 * U->getAddressByteSize())) {
    const auto Begin = Data.getAddress(&Cursor);
    const auto End = Data.getAddress(&Cursor);
    if (Begin == 0 && End == 0) {
      return Entries;
    }
    if (Begin == Max) {
      Base = End;
      continue;
    }
    const auto Size = Data.getU16(&Cursor);
    if (!Data.isValidOffsetForDataOfSize(Cursor, Size)) {
      return std::nullopt;
    }
    Entries.push_back({Base + Begin, Base + End,
                       decodeLocation(llvm::ArrayRef<uint8_t>(
                                          Section.bytes_begin() + Cursor, Size),
                                      U, Convention)});
    Cursor += Size;
  }
  return std::nullopt;
}
#endif

} // namespace

std::optional<VariableLocation> locationOf(DWARFDie D,
                                           bool ConstantsUsable) noexcept {
  auto Attr = D.find(dwarf::DW_AT_location);
  if (!Attr) {
    return std::nullopt;
  }
  if (auto Block = Attr->getAsBlock()) {
    return VariableLocation{decodeLocation(*Block, D.getDwarfUnit()), {}};
  }
  VariableLocation Result;
  std::vector<LocationEntry> Bounded;
  std::vector<WasmLocation> Defaults;
#if LLVM_VERSION_MAJOR < 10
  auto Locs = locationList(D, *Attr);
  if (!Locs) {
    return std::nullopt;
  }
  Bounded = std::move(*Locs);
#else
  auto Locs = D.getLocations(dwarf::DW_AT_location);
  if (!Locs) {
    llvm::consumeError(Locs.takeError());
    return std::nullopt;
  }
  auto *U = D.getDwarfUnit();
  const auto Convention = locationConvention(U);
  for (const auto &E : *Locs) {
    const auto L = decodeLocation(E.Expr, U, Convention);
    if (E.Range) {
      Bounded.push_back({E.Range->LowPC, E.Range->HighPC, L});
    } else {
      Defaults.push_back(L);
    }
  }
#endif
  Result.Ranges =
      normalizeLocationEntries(std::move(Bounded), Defaults, ConstantsUsable);
  if (std::none_of(Result.Ranges.begin(), Result.Ranges.end(),
                   [](const LocationEntry &E) {
                     return E.L.K != WasmLocation::Kind::None;
                   })) {
    return std::nullopt;
  }
  return Result;
}

std::optional<uint64_t>
globalAddress(const std::optional<VariableLocation> &L) noexcept {
  if (!L) {
    return std::nullopt;
  }
  if (L->Single) {
    if (L->Single->K == WasmLocation::Kind::Address) {
      return L->Single->Index;
    }
    return std::nullopt;
  }
  if (L->Ranges.size() != 1 || L->Ranges.front().Low != 0 ||
      L->Ranges.front().High != UINT64_MAX ||
      L->Ranges.front().L.K != WasmLocation::Kind::Address) {
    return std::nullopt;
  }
  return L->Ranges.front().L.Index;
}

std::optional<uint64_t> globalAddress(DWARFDie D) noexcept {
  return globalAddress(locationOf(D));
}

bool coversScope(DWARFDie Scope,
                 const std::vector<LocationEntry> &Entries) noexcept {
  auto Ranges = Scope.getAddressRanges();
  if (!Ranges) {
    llvm::consumeError(Ranges.takeError());
    return false;
  }
  return !Ranges->empty() &&
         std::all_of(Ranges->begin(), Ranges->end(), [&](const auto &R) {
           return coversRange(Entries, R.LowPC, R.HighPC);
         });
}

std::string lineTableFile(const llvm::DWARFDebugLine::LineTable *LT,
                          llvm::DWARFUnit *U, uint64_t Index) noexcept {
  std::string Path;
  if (!LT || !LT->getFileNameByIndex(
                 Index, U->getCompilationDir(),
                 llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath,
                 Path)) {
    return {};
  }
  return Path;
}

namespace {

template <typename OptionalIndex>
std::string unitFile(DWARFDie D, const OptionalIndex &Index) noexcept {
  if (!Index) {
    return {};
  }
  auto *U = D.getDwarfUnit();
  return lineTableFile(U->getContext().getLineTableForUnit(U), U, *Index);
}

} // namespace

std::string declFile(DWARFDie D) noexcept {
#if LLVM_VERSION_MAJOR < 12
  const auto Attr = D.findRecursively(dwarf::DW_AT_decl_file);
  const auto Index = dwarf::toUnsigned(Attr);
  auto *U = Attr ? const_cast<llvm::DWARFUnit *>(Attr->getUnit()) : nullptr;
  if (!Index || !U) {
    return {};
  }
  return lineTableFile(U->getContext().getLineTableForUnit(U), U, *Index);
#else
  return D.getDeclFile(
      llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath);
#endif
}

std::string callFile(DWARFDie D) noexcept {
  return unitFile(D, dwarf::toUnsigned(D.find(dwarf::DW_AT_call_file)));
}

unsigned line(DWARFDie D) noexcept {
  return static_cast<unsigned>(D.getDeclLine());
}

llvm::StringRef name(DWARFDie D) noexcept {
  const char *N = D.getName(llvm::DINameKind::ShortName);
  return N ? llvm::StringRef(N) : llvm::StringRef();
}

uint64_t dieKey(DWARFDie D) noexcept {
  const auto *U = D.getDwarfUnit();
  const bool InTypes = U && U->isTypeUnit() && U->getVersion() < 5;
  return D.getOffset() | (InTypes ? UINT64_C(1) << 63 : 0);
}

DWARFDie ref(DWARFDie D) noexcept {
  if (!D) {
    return {};
  }
  const auto Attr = D.findRecursively({dwarf::DW_AT_type});
  if (!Attr || !Attr->getUnit()) {
    return {};
  }
  auto *Owner = const_cast<llvm::DWARFUnit *>(Attr->getUnit());
  auto T = Owner->getUnitDIE().getAttributeValueAsReferencedDie(*Attr);
  if (T && T.find(dwarf::DW_AT_signature)) {
    if (auto Def = T.getAttributeValueAsReferencedDie(dwarf::DW_AT_signature)) {
      return Def;
    }
  }
  return T;
}

void inlinedChain(llvm::DWARFUnit &U, uint64_t Address,
                  llvm::SmallVectorImpl<DWARFDie> &Chain) noexcept {
  for (auto D = U.getSubroutineForAddress(Address); D; D = D.getParent()) {
    if (D.isSubprogramDIE()) {
      Chain.push_back(D);
      return;
    }
    if (D.getTag() == dwarf::DW_TAG_inlined_subroutine) {
      Chain.push_back(D);
    }
  }
}

DWARFDie declarationOf(DWARFDie D) noexcept {
  constexpr uint32_t MaxChain = 8;
  for (uint32_t Depth = 0; D && Depth < MaxChain; ++Depth) {
    auto Next = D.getAttributeValueAsReferencedDie(dwarf::DW_AT_specification);
    if (!Next) {
      Next = D.getAttributeValueAsReferencedDie(dwarf::DW_AT_abstract_origin);
    }
    if (!Next) {
      return D;
    }
    D = Next;
  }
  return D;
}

bool isObjectPointer(DWARFDie Fn, DWARFDie Param) noexcept {
  if (!Fn || Fn.getTag() == dwarf::DW_TAG_lexical_block) {
    return false;
  }
  auto Object =
      Fn.getAttributeValueAsReferencedDie(dwarf::DW_AT_object_pointer);
  auto Origin = Param;
  if (!Object) {
    const auto Abstract =
        Fn.getAttributeValueAsReferencedDie(dwarf::DW_AT_abstract_origin);
    Object = Abstract ? Abstract.getAttributeValueAsReferencedDie(
                            dwarf::DW_AT_object_pointer)
                      : DWARFDie();
    Origin =
        Param.getAttributeValueAsReferencedDie(dwarf::DW_AT_abstract_origin);
  }
  return Object && Origin == Object;
}

DWARFDie stripAliases(DWARFDie T) noexcept {
  for (uint32_t Depth = 0; T && Depth < 16; ++Depth) {
    switch (T.getTag()) {
    case dwarf::DW_TAG_typedef:
    case dwarf::DW_TAG_const_type:
    case dwarf::DW_TAG_volatile_type:
    case dwarf::DW_TAG_restrict_type:
      T = ref(T);
      break;
    default:
      return T;
    }
  }
  return T;
}

bool isSignedType(DWARFDie T) noexcept {
  T = stripAliases(T);
  if (!T || T.getTag() != dwarf::DW_TAG_base_type) {
    return false;
  }
  const auto Encoding = dwarf::toUnsigned(T.find(dwarf::DW_AT_encoding), 0);
  return Encoding == dwarf::DW_ATE_signed ||
         Encoding == dwarf::DW_ATE_signed_char;
}

uint64_t byteSize(DWARFDie D) noexcept {
  return dwarf::toUnsigned(D.find(dwarf::DW_AT_byte_size), 0);
}

std::optional<std::pair<unsigned, bool>> integerType(DWARFDie T) noexcept {
  T = stripAliases(T);
  if (!T) {
    return std::nullopt;
  }
  const auto Bits = byteSize(T) * 8;
  if (Bits == 0 || Bits > 64) {
    return std::nullopt;
  }
  if (T.getTag() == dwarf::DW_TAG_enumeration_type) {
    return std::pair{static_cast<unsigned>(Bits), isSignedType(ref(T))};
  }
  if (T.getTag() != dwarf::DW_TAG_base_type) {
    return std::nullopt;
  }
  switch (dwarf::toUnsigned(T.find(dwarf::DW_AT_encoding), 0)) {
  case dwarf::DW_ATE_signed:
  case dwarf::DW_ATE_signed_char:
    return std::pair{static_cast<unsigned>(Bits), true};
  case dwarf::DW_ATE_unsigned:
  case dwarf::DW_ATE_unsigned_char:
  case dwarf::DW_ATE_boolean:
  case dwarf::DW_ATE_UTF:
    return std::pair{static_cast<unsigned>(Bits), false};
  default:
    return std::nullopt;
  }
}

bool isAggregate(dwarf::Tag Tag) noexcept {
  return Tag == dwarf::DW_TAG_structure_type ||
         Tag == dwarf::DW_TAG_class_type || Tag == dwarf::DW_TAG_union_type;
}

std::optional<uint64_t> memberOffset(DWARFDie D) noexcept {
  auto V = D.find(dwarf::DW_AT_data_member_location);
  if (!V) {
    return 0;
  }
  return memberOffset(*V, D.getDwarfUnit()->getVersion());
}

std::optional<uint64_t> memberBitOffset(DWARFDie D) noexcept {
  if (!D.find(dwarf::DW_AT_data_member_location)) {
    if (auto Bits = dwarf::toUnsigned(D.find(dwarf::DW_AT_data_bit_offset))) {
      return *Bits;
    }
    return 0;
  }
  const auto Bytes = memberOffset(D);
  if (!Bytes || *Bytes > UINT64_MAX / 8) {
    return std::nullopt;
  }
  return *Bytes * 8;
}

std::optional<uint64_t> memberOffset(const llvm::DWARFFormValue &V,
                                     uint16_t Version) noexcept {
  switch (V.getForm()) {
  case dwarf::DW_FORM_data4:
  case dwarf::DW_FORM_data8:
    if (Version < 4) {
      return std::nullopt;
    }
    [[fallthrough]];
  case dwarf::DW_FORM_data1:
  case dwarf::DW_FORM_data2:
  case dwarf::DW_FORM_udata:
  case dwarf::DW_FORM_implicit_const:
    if (const auto Offset = V.getAsUnsignedConstant()) {
      return *Offset;
    }
    return std::nullopt;
  case dwarf::DW_FORM_sdata:
    if (const auto Offset = V.getAsSignedConstant(); Offset && *Offset >= 0) {
      return static_cast<uint64_t>(*Offset);
    }
    return std::nullopt;
  default:
    break;
  }
  if (auto Block = V.getAsBlock();
      Block && !Block->empty() && Block->front() == dwarf::DW_OP_plus_uconst) {
    unsigned Length = 0;
    const char *Error = nullptr;
    const auto Offset = llvm::decodeULEB128(
        Block->data() + 1, &Length, Block->data() + Block->size(), &Error);
    if (!Error && Length + 1 == Block->size()) {
      return Offset;
    }
  }
  return std::nullopt;
}

void ScopeIndex::add(DWARFDie Frame, DWARFDie Block, uint32_t Depth,
                     uint64_t Low, uint64_t High) {
  if (Low < High) {
    Ranges[Frame.getOffset()].push_back({Low, High, Depth, Block});
  }
}

void ScopeIndex::build() noexcept {
  Spans.clear();
  for (const auto &Entry : Ranges) {
    const auto &List = Entry.second;
    std::set<std::pair<uint32_t, size_t>> Active;
    Spans[Entry.first] = sweepIntervals<DWARFDie>(
        llvm::ArrayRef<Range>(List),
        [&](size_t I) { Active.emplace(List[I].Depth, I); },
        [&](size_t I) { Active.erase({List[I].Depth, I}); },
        [&]() -> std::optional<DWARFDie> {
          if (Active.empty()) {
            return std::nullopt;
          }
          return List[std::prev(Active.end())->second].Block;
        });
  }
  Ranges = decltype(Ranges)();
}

DWARFDie ScopeIndex::innermost(DWARFDie Frame, uint64_t Addr) const noexcept {
  const auto Found = Spans.find(Frame.getOffset());
  if (Found == Spans.end()) {
    return Frame;
  }
  const auto I =
      findInterval(llvm::ArrayRef<AddressSpan<DWARFDie>>(Found->second), Addr);
  return I ? Found->second[*I].Value : Frame;
}

} // namespace WasmEdge::LLVM::DebugInfo
