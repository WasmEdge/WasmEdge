// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ast/module.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/Support/MemoryBuffer.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

/// An address interval [Low, High) with a value.
template <typename T> struct AddressSpan {
  uint64_t Low;
  uint64_t High;
  T Value;
};

/// The index of the interval that holds Address. The intervals must be
/// sorted and must not overlap.
template <typename Interval>
std::optional<size_t> findInterval(llvm::ArrayRef<Interval> Intervals,
                                   uint64_t Address) noexcept {
  const auto It =
      std::upper_bound(Intervals.begin(), Intervals.end(), Address,
                       [](uint64_t A, const Interval &I) { return A < I.Low; });
  if (It == Intervals.begin() || Address >= std::prev(It)->High) {
    return std::nullopt;
  }
  return static_cast<size_t>(std::prev(It) - Intervals.begin());
}

/// Turns non-empty intervals into sorted spans that do not overlap. At each
/// endpoint, in address order, Enter gets each interval that starts there
/// and Leave gets each interval that ends there. Winner then gives the value
/// up to the next endpoint, or std::nullopt for a gap. Adjacent spans with
/// equal values merge.
template <typename T, typename Interval, typename EnterFn, typename LeaveFn,
          typename WinnerFn>
std::vector<AddressSpan<T>> sweepIntervals(llvm::ArrayRef<Interval> Intervals,
                                           EnterFn Enter, LeaveFn Leave,
                                           WinnerFn Winner) {
  std::vector<std::pair<uint64_t, size_t>> Events;
  Events.reserve(Intervals.size() * 2);
  for (size_t I = 0; I < Intervals.size(); ++I) {
    Events.emplace_back(Intervals[I].Low, I);
    Events.emplace_back(Intervals[I].High, I);
  }
  std::sort(Events.begin(), Events.end());
  std::vector<AddressSpan<T>> Result;
  for (size_t I = 0; I < Events.size();) {
    const auto Addr = Events[I].first;
    for (; I < Events.size() && Events[I].first == Addr; ++I) {
      if (Intervals[Events[I].second].Low == Addr) {
        Enter(Events[I].second);
      } else {
        Leave(Events[I].second);
      }
    }
    if (I == Events.size()) {
      break;
    }
    std::optional<T> Value = Winner();
    if (!Value) {
      continue;
    }
    const auto Next = Events[I].first;
    if (!Result.empty() && Result.back().High == Addr &&
        Result.back().Value == *Value) {
      Result.back().High = Next;
    } else {
      Result.push_back({Addr, Next, std::move(*Value)});
    }
  }
  return Result;
}

/// Where a value is. For Constant, Index holds the two's complement bits of
/// the value.
struct WasmLocation {
  enum class Kind : uint8_t {
    None,
    Local,
    Global,
    FrameBaseOffset,
    Address,
    Constant
  };
  Kind K = Kind::None;
  uint64_t Index = 0;
  int64_t Offset = 0;
  bool Indirect = false;
};

inline bool sameLocation(const WasmLocation &L,
                         const WasmLocation &R) noexcept {
  return L.K == R.K && L.Index == R.Index && L.Offset == R.Offset &&
         L.Indirect == R.Indirect;
}

inline bool operator==(const WasmLocation &L, const WasmLocation &R) noexcept {
  return sameLocation(L, R);
}

/// True for an address that the linker gives to discarded data. Address 0
/// is valid in linear memory.
inline bool isTombstoneDataAddress(uint64_t Address) noexcept {
  return Address == UINT32_C(0xFFFFFFFF) || Address == UINT32_C(0xFFFFFFFE) ||
         Address == UINT64_MAX;
}

/// True for an address that the linker gives to discarded code. Older
/// linkers use 0, which is never the address of a function body.
inline bool isTombstoneAddress(uint64_t Address) noexcept {
  return Address == 0 || isTombstoneDataAddress(Address);
}

/// What a DW_OP_WASM_location without DW_OP_stack_value describes. LLVM
/// marks a value in a wasm local or global with DW_OP_stack_value, and
/// writes the operation alone for a local that holds the memory address of
/// the value. The WebAssembly DWARF convention writes the operation alone
/// for a value in a wasm local or global.
enum class WasmLocationConvention : uint8_t {
  /// The operation alone gives the memory address of the value.
  LLVM,
  /// The operation alone gives the value.
  Standard,
  /// The unit does not say. The operation alone gives no location.
  Unknown,
};

/// The convention of U, from its DW_AT_producer. A producer based on LLVM
/// uses the LLVM convention, and any other producer uses the standard one.
/// A unit without a producer has an unknown convention.
WasmLocationConvention locationConvention(llvm::DWARFUnit *U) noexcept;

WasmLocation decodeLocation(llvm::ArrayRef<uint8_t> Bytes, llvm::DWARFUnit *U,
                            WasmLocationConvention Convention) noexcept;

inline WasmLocation decodeLocation(llvm::ArrayRef<uint8_t> Bytes,
                                   llvm::DWARFUnit *U) noexcept {
  return decodeLocation(Bytes, U, locationConvention(U));
}

struct LocationEntry {
  uint64_t Low;
  uint64_t High;
  WasmLocation L;
};

/// Turns a location list into sorted intervals that do not overlap. DWARF 5
/// section 2.6.2 makes overlapping bounded entries describe the variable at
/// the same time, but native debug info holds one location per address. Each
/// interval holds the first bounded entry in a wasm local, else the first
/// constant entry if ConstantsUsable, else the first other bounded entry
/// that the translator supports, else the first bounded entry, which marks
/// the value unavailable. The translator can describe a changing location
/// only in wasm locals and in constants of a type that it can emit, so these
/// win. A default entry fills only the gaps that no bounded entry covers.
/// Adjacent intervals with the same location merge, even when they come
/// from different entries.
std::vector<LocationEntry>
normalizeLocationEntries(std::vector<LocationEntry> Bounded,
                         llvm::ArrayRef<WasmLocation> Defaults,
                         bool ConstantsUsable = false) noexcept;

std::optional<size_t> findLocationEntry(llvm::ArrayRef<LocationEntry> Entries,
                                        uint64_t Address) noexcept;

/// True when the entries together cover every address in [Low, High). The
/// entries must be sorted and must not overlap, as normalizeLocationEntries
/// returns them.
bool coversRange(llvm::ArrayRef<LocationEntry> Entries, uint64_t Low,
                 uint64_t High) noexcept;

/// An integer constant with the signedness that its form or its type gives.
struct IntegerConstant {
  bool Signed = false;
  int64_t SignedValue = 0;
  uint64_t UnsignedValue = 0;

  /// The two's complement bits of the value.
  uint64_t bits() const noexcept {
    return Signed ? static_cast<uint64_t>(SignedValue) : UnsignedValue;
  }

  /// The value as int64_t, or nothing if it is unsigned and above INT64_MAX.
  std::optional<int64_t> toInt64() const noexcept {
    if (Signed) {
      return SignedValue;
    }
    if (UnsignedValue > static_cast<uint64_t>(INT64_MAX)) {
      return std::nullopt;
    }
    return static_cast<int64_t>(UnsignedValue);
  }
};

/// Reads an integer constant, such as an array bound or a constant value. An
/// sdata value, or a fixed-size value of a signed type, is sign-extended. An
/// expression, a block, or a reference gives no value.
std::optional<IntegerConstant> integerConstant(const llvm::DWARFFormValue &V,
                                               bool Signed) noexcept;

/// The lower bound that DWARF 5 table 7.17 gives an array without
/// DW_AT_lower_bound.
int64_t defaultLowerBound(uint64_t Language) noexcept;

class DwarfReader {
public:
  /// True if Mod has the DWARF sections that create needs.
  static bool hasDwarf(const AST::Module &Mod) noexcept;

  static std::unique_ptr<DwarfReader> create(const AST::Module &Mod) noexcept;

  llvm::DWARFContext &getContext() noexcept { return *Ctx; }
  /// Wasm DWARF code addresses are 32-bit offsets into the code section. The
  /// result wraps modulo 2^32, so an instruction offset that the loader keeps
  /// in 32 bits gives the right address also beyond 4 GiB.
  uint64_t toAddress(uint64_t FileOffset) const noexcept {
    return static_cast<uint32_t>(FileOffset - CodeOffset);
  }
  llvm::DWARFDie getSubprogram(uint32_t DefinedIndex) const noexcept;
  std::string getFunctionName(uint32_t FuncIndex) const;
  uint32_t getErrorCount() const noexcept { return *Errors; }
  bool hasGlobalVariables() const noexcept { return HasGlobals; }
  /// A number that no other reader in this process has. The LazyJIT objects
  /// of one module share their reader, so the number names the module.
  uint64_t getModuleId() const noexcept { return ModuleId; }

private:
  DwarfReader() = default;
  void indexSubprograms(const AST::Module &Mod) noexcept;
  /// Reads the function names. To recover from a repeated index, the first
  /// name wins. After a malformed subsection, the names before the error
  /// stay, the reader ignores the later function subsections, and it counts
  /// an error.
  void parseNames(const AST::Module &Mod) noexcept;

  llvm::StringMap<std::unique_ptr<llvm::MemoryBuffer>> Sections;
  std::unique_ptr<llvm::DWARFContext> Ctx;
  std::shared_ptr<uint32_t> Errors = std::make_shared<uint32_t>(0);
  uint64_t CodeOffset = 0;
  uint64_t ModuleId = 0;
  bool HasGlobals = false;
  std::vector<llvm::DWARFDie> Subprograms;
  std::unordered_map<uint32_t, std::string> Names;
};

} // namespace WasmEdge::LLVM::DebugInfo
