// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/dwarf_reader.h"

#include "common/spdlog.h"
#include "loader/name_section.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#include <llvm/Support/Error.h>

#include <fmt/format.h>

#include <algorithm>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;

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
