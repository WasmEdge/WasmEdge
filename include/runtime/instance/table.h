// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/table.h - Table Instance definition ---------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the table instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/type.h"
#include "common/errcode.h"
#include "common/types.h"
#include "support/log.h"
#include "support/span.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class TableInstance {
public:
  TableInstance() = delete;
  TableInstance(const RefType &Ref, const AST::Limit &Lim)
      : Type(Ref), HasMaxSize(Lim.hasMax()), MinSize(Lim.getMin()),
        MaxSize(Lim.getMax()), Refs(MinSize), RefsInit(MinSize, false) {}
  virtual ~TableInstance() = default;

  /// Getter of reference type.
  RefType getReferenceType() const { return Type; }

  /// Getter of limit definition.
  bool getHasMax() const { return HasMaxSize; }

  /// Getter of limit definition.
  uint32_t getMin() const { return MinSize; }

  /// Getter of limit definition.
  uint32_t getMax() const { return MaxSize; }

  /// Set the function index initialization list.
  Expect<void> setInitList(const uint32_t Offset, Span<const uint32_t> Addrs) {
    /// Boundary checked during validation.
    std::copy(Addrs.begin(), Addrs.end(), Refs.begin() + Offset);
    std::fill(RefsInit.begin() + Offset,
              RefsInit.begin() + Offset + Addrs.size(), true);
    return {};
  }

  /// Check is out of bound.
  bool checkAccessBound(uint32_t Offset, uint32_t Length) const noexcept {
    const uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    return AccessLen <= MinSize;
  }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return ((MinSize > 0) ? (MinSize - 1) : 0);
  }

  /// Get the elem address.
  Expect<uint32_t> getRefAddr(const uint32_t Idx) const {
    if (Idx >= Refs.size()) {
      LOG(ERROR) << ErrCode::UndefinedElement;
      LOG(ERROR) << ErrInfo::InfoBoundary(Idx, 1, getBoundIdx());
      return Unexpect(ErrCode::UndefinedElement);
    }
    if (Symbol) {
      return Symbol[Idx];
    } else {
      if (RefsInit[Idx]) {
        return Refs[Idx];
      } else {
        LOG(ERROR) << ErrCode::UninitializedElement;
        return Unexpect(ErrCode::UninitializedElement);
      }
    }
  }

  /// Getter of symbol
  const auto &getSymbol() const noexcept { return Symbol; }
  /// Setter of symbol
  void setSymbol(DLSymbol<uint32_t> S) noexcept { Symbol = std::move(S); }

private:
  /// \name Data of table instance.
  /// @{
  const RefType Type;
  const bool HasMaxSize;
  const uint32_t MinSize = 0;
  const uint32_t MaxSize = 0;
  std::vector<uint32_t> Refs;
  std::vector<bool> RefsInit;
  DLSymbol<uint32_t> Symbol;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
