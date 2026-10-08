// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ast/module.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
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
