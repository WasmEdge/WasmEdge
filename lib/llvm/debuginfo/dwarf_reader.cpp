// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/dwarf_reader.h"
#include "debuginfo/dwarf_die.h"

#include "common/spdlog.h"
#include "loader/name_section.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/LEB128.h>

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <iterator>
#include <optional>
#include <set>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;

namespace {

constexpr uint8_t WasmLocationOp = 0xED;

class ExpressionReader {
public:
  explicit ExpressionReader(llvm::ArrayRef<uint8_t> Bytes) noexcept
      : Cur(Bytes.begin()), End(Bytes.end()) {}

  bool atEnd() const noexcept { return Cur == End; }

  std::optional<uint8_t> readU8() noexcept {
    if (Cur == End) {
      return std::nullopt;
    }
    return *Cur++;
  }

  std::optional<uint64_t> readFixed(unsigned Size) noexcept {
    if (static_cast<size_t>(End - Cur) < Size) {
      return std::nullopt;
    }
    uint64_t Value = 0;
    for (unsigned I = 0; I < Size; ++I) {
      Value |= static_cast<uint64_t>(Cur[I]) << (I * 8);
    }
    Cur += Size;
    return Value;
  }

  std::optional<uint64_t> readU32() noexcept { return readFixed(4); }

  std::optional<uint64_t> readULEB() noexcept {
    unsigned Size = 0;
    const char *Error = nullptr;
    const auto Value = llvm::decodeULEB128(Cur, &Size, End, &Error);
    if (Error) {
      return std::nullopt;
    }
    Cur += Size;
    return Value;
  }

  std::optional<int64_t> readSLEB() noexcept {
    unsigned Size = 0;
    const char *Error = nullptr;
    const auto Value = llvm::decodeSLEB128(Cur, &Size, End, &Error);
    if (Error) {
      return std::nullopt;
    }
    Cur += Size;
    return Value;
  }

private:
  const uint8_t *Cur;
  const uint8_t *End;
};

std::optional<WasmLocation>
readLocation(ExpressionReader &R, llvm::DWARFUnit *U,
             WasmLocationConvention Convention) noexcept {
  namespace dwarf = llvm::dwarf;
  using Kind = WasmLocation::Kind;
  const auto Code = R.readU8();
  if (!Code) {
    return std::nullopt;
  }
  switch (*Code) {
  case WasmLocationOp: {
    const auto Space = R.readU8();
    if (!Space) {
      return std::nullopt;
    }
    const auto Index = *Space == 3 ? R.readU32() : R.readULEB();
    if (!Index || *Index > UINT32_MAX) {
      return std::nullopt;
    }
    const bool Indirect = Convention == WasmLocationConvention::LLVM;
    if (*Space == 0) {
      return WasmLocation{Kind::Local, *Index, 0, Indirect};
    }
    if (*Space == 1 || *Space == 3) {
      return WasmLocation{Kind::Global, *Index, 0, Indirect};
    }
    return std::nullopt;
  }
  case dwarf::DW_OP_fbreg:
    if (const auto Offset = R.readSLEB()) {
      return WasmLocation{Kind::FrameBaseOffset, 0, *Offset, false};
    }
    return std::nullopt;
  case dwarf::DW_OP_addr:
    if (const auto Address = R.readU32()) {
      return WasmLocation{Kind::Address, *Address, 0, false};
    }
    return std::nullopt;
  case dwarf::DW_OP_addrx: {
    const auto Index = R.readULEB();
    if (!Index || *Index > UINT32_MAX || !U) {
      return std::nullopt;
    }
    if (auto A = U->getAddrOffsetSectionItem(static_cast<uint32_t>(*Index))) {
      return WasmLocation{Kind::Address, A->Address, 0, false};
    }
    return std::nullopt;
  }
  default:
    return std::nullopt;
  }
}

/// The bits of a constant that the next operation pushes, sign-extended for
/// a signed operation.
std::optional<uint64_t> readConstant(ExpressionReader &R) noexcept {
  namespace dwarf = llvm::dwarf;
  const auto Code = R.readU8();
  if (!Code) {
    return std::nullopt;
  }
  if (*Code >= dwarf::DW_OP_lit0 && *Code <= dwarf::DW_OP_lit31) {
    return *Code - dwarf::DW_OP_lit0;
  }
  auto signExtend = [](std::optional<uint64_t> V,
                       unsigned Bits) -> std::optional<uint64_t> {
    if (!V || Bits == 64) {
      return V;
    }
    const auto Sign = UINT64_C(1) << (Bits - 1);
    return (*V ^ Sign) - Sign;
  };
  switch (*Code) {
  case dwarf::DW_OP_const1u:
    return R.readFixed(1);
  case dwarf::DW_OP_const1s:
    return signExtend(R.readFixed(1), 8);
  case dwarf::DW_OP_const2u:
    return R.readFixed(2);
  case dwarf::DW_OP_const2s:
    return signExtend(R.readFixed(2), 16);
  case dwarf::DW_OP_const4u:
    return R.readFixed(4);
  case dwarf::DW_OP_const4s:
    return signExtend(R.readFixed(4), 32);
  case dwarf::DW_OP_const8u:
  case dwarf::DW_OP_const8s:
    return R.readFixed(8);
  case dwarf::DW_OP_constu:
    return R.readULEB();
  case dwarf::DW_OP_consts:
    if (const auto V = R.readSLEB()) {
      return static_cast<uint64_t>(*V);
    }
    return std::nullopt;
  default:
    return std::nullopt;
  }
}

} // namespace

WasmLocationConvention locationConvention(llvm::DWARFUnit *U) noexcept {
  if (!U) {
    return WasmLocationConvention::Unknown;
  }
  const char *Producer = llvm::dwarf::toString(
      U->getUnitDIE().find(llvm::dwarf::DW_AT_producer), nullptr);
  if (!Producer) {
    return WasmLocationConvention::Unknown;
  }
  const llvm::StringRef Name(Producer);
  for (const char *LLVMBased :
       {"clang", "LLVM", "rustc", "flang", "zig", "Swift"}) {
    if (Name.find(LLVMBased) != llvm::StringRef::npos) {
      return WasmLocationConvention::LLVM;
    }
  }
  return WasmLocationConvention::Standard;
}

WasmLocation decodeLocation(llvm::ArrayRef<uint8_t> Bytes, llvm::DWARFUnit *U,
                            WasmLocationConvention Convention) noexcept {
  {
    ExpressionReader R(Bytes);
    if (const auto Value = readConstant(R);
        Value && R.readU8() == llvm::dwarf::DW_OP_stack_value && R.atEnd()) {
      return WasmLocation{WasmLocation::Kind::Constant, *Value, 0, false};
    }
  }
  ExpressionReader R(Bytes);
  auto L = readLocation(R, U, Convention);
  if (!L) {
    return {};
  }
  if (R.atEnd()) {
    if (Convention == WasmLocationConvention::Unknown &&
        (L->K == WasmLocation::Kind::Local ||
         L->K == WasmLocation::Kind::Global)) {
      return {};
    }
    return *L;
  }
  if (R.readU8() != llvm::dwarf::DW_OP_stack_value || !R.atEnd() ||
      (L->K != WasmLocation::Kind::Local &&
       L->K != WasmLocation::Kind::Global)) {
    return {};
  }
  L->Indirect = false;
  return *L;
}

std::optional<IntegerConstant> integerConstant(const llvm::DWARFFormValue &V,
                                               bool Signed) noexcept {
  namespace dwarf = llvm::dwarf;
  switch (V.getForm()) {
  case dwarf::DW_FORM_sdata:
  case dwarf::DW_FORM_implicit_const:
    Signed = true;
    break;
  case dwarf::DW_FORM_udata:
    Signed = false;
    break;
  case dwarf::DW_FORM_data1:
  case dwarf::DW_FORM_data2:
  case dwarf::DW_FORM_data4:
  case dwarf::DW_FORM_data8:
    break;
  default:
    return std::nullopt;
  }
  IntegerConstant Result;
  Result.Signed = Signed;
  if (Signed) {
    if (auto Value = V.getAsSignedConstant()) {
      Result.SignedValue = *Value;
      return Result;
    }
  } else if (auto Value = V.getAsUnsignedConstant()) {
    Result.UnsignedValue = *Value;
    return Result;
  }
  return std::nullopt;
}

int64_t defaultLowerBound(uint64_t Language) noexcept {
  if (const auto Bound = llvm::dwarf::LanguageLowerBound(
          static_cast<llvm::dwarf::SourceLanguage>(Language))) {
    return *Bound;
  }
  return 0;
}

std::vector<LocationEntry>
normalizeLocationEntries(std::vector<LocationEntry> Bounded,
                         llvm::ArrayRef<WasmLocation> Defaults,
                         bool ConstantsUsable) noexcept {
  Bounded.erase(
      std::remove_if(Bounded.begin(), Bounded.end(),
                     [](const LocationEntry &E) { return E.Low >= E.High; }),
      Bounded.end());
  const auto BoundedCount = Bounded.size();
  for (const auto &L : Defaults) {
    Bounded.push_back({0, UINT64_MAX, L});
  }
  auto Rank = [&](size_t Index) -> uint8_t {
    const auto &L = Bounded[Index].L;
    if (Index >= BoundedCount) {
      return 4;
    }
    if (L.K == WasmLocation::Kind::Local && !L.Indirect) {
      return 0;
    }
    if (L.K == WasmLocation::Kind::Constant && ConstantsUsable) {
      return 1;
    }
    return L.K != WasmLocation::Kind::None ? 2 : 3;
  };
  std::set<std::pair<uint8_t, size_t>> Active;
  const auto Spans = sweepIntervals<WasmLocation>(
      llvm::ArrayRef<LocationEntry>(Bounded),
      [&](size_t Index) { Active.emplace(Rank(Index), Index); },
      [&](size_t Index) { Active.erase({Rank(Index), Index}); },
      [&]() -> std::optional<WasmLocation> {
        if (Active.empty()) {
          return std::nullopt;
        }
        return Bounded[Active.begin()->second].L;
      });
  std::vector<LocationEntry> Result;
  Result.reserve(Spans.size());
  for (const auto &S : Spans) {
    Result.push_back({S.Low, S.High, S.Value});
  }
  return Result;
}

std::optional<size_t> findLocationEntry(llvm::ArrayRef<LocationEntry> Entries,
                                        uint64_t Address) noexcept {
  return findInterval(Entries, Address);
}

bool coversRange(llvm::ArrayRef<LocationEntry> Entries, uint64_t Low,
                 uint64_t High) noexcept {
  const auto Start = findLocationEntry(Entries, Low);
  if (!Start) {
    return Low >= High;
  }
  for (size_t I = *Start; I < Entries.size() && Entries[I].Low <= Low; ++I) {
    Low = Entries[I].High;
    if (Low >= High) {
      return true;
    }
  }
  return false;
}

bool DwarfReader::hasDwarf(const AST::Module &Mod) noexcept {
  bool HasInfo = false;
  bool HasAbbrev = false;
  for (const auto &Custom : Mod.getCustomSections()) {
    HasInfo = HasInfo || Custom.getName() == ".debug_info"sv;
    HasAbbrev = HasAbbrev || Custom.getName() == ".debug_abbrev"sv;
  }
  return HasInfo && HasAbbrev;
}

std::unique_ptr<DwarfReader>
DwarfReader::create(const AST::Module &Mod) noexcept {
  if (!hasDwarf(Mod)) {
    return nullptr;
  }
  std::unique_ptr<DwarfReader> R(new DwarfReader());
  static std::atomic<uint64_t> LastModuleId = 0;
  R->ModuleId = ++LastModuleId;
  for (const auto &Custom : Mod.getCustomSections()) {
    std::string_view Name = Custom.getName();
    if (Name.substr(0, 7) != ".debug_") {
      continue;
    }
    const auto Content = Custom.getContent();
    R->Sections[llvm::StringRef(Name.data() + 1, Name.size() - 1)] =
        llvm::MemoryBuffer::getMemBuffer(
            llvm::StringRef(reinterpret_cast<const char *>(Content.data()),
                            Content.size()),
            llvm::StringRef(Name.data(), Name.size()), false);
  }
  auto Errors = R->Errors;
  auto Handler = [Errors](llvm::Error E) {
    ++*Errors;
    llvm::consumeError(std::move(E));
  };
#if LLVM_VERSION_MAJOR < 11
  R->Ctx = llvm::DWARFContext::create(R->Sections, 4, true);
#else
  R->Ctx = llvm::DWARFContext::create(R->Sections, 4, true, Handler, Handler);
#endif
  if (!R->Ctx) {
    return nullptr;
  }
  uint64_t UnitsEnd = 0;
  for (const auto &Unit : R->Ctx->info_section_units()) {
    UnitsEnd = std::max<uint64_t>(UnitsEnd, Unit->getNextUnitOffset());
  }
  if (UnitsEnd != R->Sections["debug_info"]->getBufferSize()) {
    ++*Errors;
  }
  R->CodeOffset = Mod.getCodeSection().getContentOffset();
  R->indexSubprograms(Mod);
  R->parseNames(Mod);
  if (*Errors > 0) {
    spdlog::warn("debug info: found {} errors in the DWARF and name "
                 "sections, some debug info can be missing"sv,
                 *Errors);
  }
  return R;
}

namespace {

struct SubprogramSpan {
  uint64_t Low;
  uint64_t High;
  llvm::DWARFDie Die;
};

void addExact(std::unordered_map<uint64_t, llvm::DWARFDie> &Index,
              uint64_t Address, llvm::DWARFDie Die) noexcept {
  auto [It, Inserted] = Index.emplace(Address, Die);
  if (!Inserted && It->second != Die) {
    It->second = llvm::DWARFDie();
  }
}

/// Turns the ranges into sorted spans that do not overlap. A span that more
/// than one subprogram covers gets an empty DIE.
std::vector<AddressSpan<llvm::DWARFDie>>
disjointSpans(llvm::ArrayRef<SubprogramSpan> Ranges) noexcept {
  std::unordered_map<uint64_t, std::pair<size_t, llvm::DWARFDie>> Active;
  return sweepIntervals<llvm::DWARFDie>(
      Ranges,
      [&](size_t I) {
        auto &Entry = Active[Ranges[I].Die.getOffset()];
        ++Entry.first;
        Entry.second = Ranges[I].Die;
      },
      [&](size_t I) {
        if (auto It = Active.find(Ranges[I].Die.getOffset());
            --It->second.first == 0) {
          Active.erase(It);
        }
      },
      [&]() -> std::optional<llvm::DWARFDie> {
        if (Active.empty()) {
          return std::nullopt;
        }
        return Active.size() == 1 ? Active.begin()->second.second
                                  : llvm::DWARFDie();
      });
}

/// True when Parent holds a variable at a live address in linear memory.
/// The search follows namespaces, functions, and their lexical blocks. In a
/// function, only a static variable can have such an address, and it has a
/// single location expression, so a location list there is not decoded.
bool hasLiveGlobal(llvm::DWARFDie Parent, bool InFunction,
                   uint32_t Depth) noexcept {
  namespace dwarf = llvm::dwarf;
  for (auto C : Parent.children()) {
    switch (C.getTag()) {
    case dwarf::DW_TAG_variable: {
      if (InFunction) {
        auto Attr = C.find(dwarf::DW_AT_location);
        if (!Attr || !Attr->getAsBlock()) {
          continue;
        }
      }
      if (const auto Address = globalAddress(C);
          Address && !isTombstoneDataAddress(*Address)) {
        return true;
      }
      break;
    }
    case dwarf::DW_TAG_namespace:
      if (Depth < MaxDepth && hasLiveGlobal(C, InFunction, Depth + 1)) {
        return true;
      }
      break;
    case dwarf::DW_TAG_subprogram:
    case dwarf::DW_TAG_lexical_block:
      if (Depth < MaxDepth && hasLiveGlobal(C, true, Depth + 1)) {
        return true;
      }
      break;
    default:
      break;
    }
  }
  return false;
}

const llvm::DWARFDie *spanAt(llvm::ArrayRef<AddressSpan<llvm::DWARFDie>> Spans,
                             uint64_t Address) noexcept {
  const auto I = findInterval(Spans, Address);
  return I ? &Spans[*I].Value : nullptr;
}

} // namespace

void DwarfReader::indexSubprograms(const AST::Module &Mod) noexcept {
  namespace dwarf = llvm::dwarf;
  std::unordered_map<uint64_t, llvm::DWARFDie> ByLowPC;
  std::unordered_map<uint64_t, llvm::DWARFDie> ByEntryPC;
  std::vector<SubprogramSpan> Ranges;
  for (const auto &CU : Ctx->compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() != dwarf::DW_TAG_subprogram) {
        continue;
      }
      if (auto LowPC = dwarf::toAddress(Die.find(dwarf::DW_AT_low_pc));
          LowPC && !isTombstoneAddress(*LowPC)) {
        addExact(ByLowPC, *LowPC, Die);
      }
      if (auto EntryPC = dwarf::toAddress(Die.find(dwarf::DW_AT_entry_pc));
          EntryPC && !isTombstoneAddress(*EntryPC)) {
        addExact(ByEntryPC, *EntryPC, Die);
      }
      if (!Die.find(dwarf::DW_AT_low_pc) && !Die.find(dwarf::DW_AT_ranges)) {
        continue;
      }
      if (auto DieRanges = Die.getAddressRanges()) {
        for (const auto &R : *DieRanges) {
          if (R.LowPC < R.HighPC && !isTombstoneAddress(R.LowPC)) {
            Ranges.push_back({R.LowPC, R.HighPC, Die});
          }
        }
      } else {
        llvm::consumeError(DieRanges.takeError());
      }
    }
  }
  for (const auto &CU : Ctx->compile_units()) {
    if (hasLiveGlobal(CU->getUnitDIE(), false, 0)) {
      HasGlobals = true;
      break;
    }
  }
  const auto Spans = disjointSpans(Ranges);
  const auto Segs = Mod.getCodeSection().getContent();
  Subprograms.resize(Segs.size());
  for (size_t I = 0; I < Segs.size(); ++I) {
    llvm::SmallVector<uint64_t, 3> Addresses;
    if (const auto Instrs = Segs[I].getExpr().getInstrs(); !Instrs.empty()) {
      Addresses.push_back(toAddress(Instrs.front().getOffset()));
    }
    Addresses.push_back(toAddress(Segs[I].getSegOffset()));
    Addresses.push_back(toAddress(Segs[I].getBodyOffset()));
    auto exact = [&](const std::unordered_map<uint64_t, llvm::DWARFDie> &Map)
        -> const llvm::DWARFDie * {
      for (const auto Address : Addresses) {
        if (auto It = Map.find(Address); It != Map.end() && It->second) {
          return &It->second;
        }
      }
      return nullptr;
    };
    auto ranged = [&]() -> const llvm::DWARFDie * {
      for (const auto Address : Addresses) {
        if (const auto *Die = spanAt(Spans, Address)) {
          return Die;
        }
      }
      return nullptr;
    };
    auto resolve = [&](const llvm::DWARFDie *Found) {
      if (Found) {
        Subprograms[I] = *Found;
      }
      return Found != nullptr;
    };
    if (!resolve(exact(ByLowPC)) && !resolve(exact(ByEntryPC))) {
      resolve(ranged());
    }
  }
}

void DwarfReader::parseNames(const AST::Module &Mod) noexcept {
  for (const auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != "name") {
      continue;
    }
    bool Valid = true;
    const bool Complete = Loader::forEachNameSubsection(
        Custom.getContent(), [&](Byte Id, Span<const Byte> Subsection) {
          if (!Valid || Id != Loader::FunctionNameSubsection) {
            return;
          }
          Valid = static_cast<bool>(Loader::readNameMap(
              Subsection, [this](uint32_t Index, std::string Name) {
                Names.emplace(Index, std::move(Name));
              }));
        });
    if (!Complete || !Valid) {
      ++*Errors;
    }
  }
}

llvm::DWARFDie
DwarfReader::getSubprogram(uint32_t DefinedIndex) const noexcept {
  if (DefinedIndex >= Subprograms.size()) {
    return {};
  }
  return Subprograms[DefinedIndex];
}

std::string DwarfReader::getFunctionName(uint32_t FuncIndex) const {
  if (auto It = Names.find(FuncIndex); It != Names.end()) {
    return It->second;
  }
  return fmt::format("func[{}]", FuncIndex);
}

} // namespace WasmEdge::LLVM::DebugInfo
