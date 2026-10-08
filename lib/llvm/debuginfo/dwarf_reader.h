// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ast/module.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
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
  std::vector<llvm::DWARFDie> Subprograms;
  std::unordered_map<uint32_t, std::string> Names;
};

} // namespace WasmEdge::LLVM::DebugInfo
