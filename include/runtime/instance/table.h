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

#include <algorithm>
#include <cstdint>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class TableInstance {
public:
  TableInstance() = delete;
  TableInstance(const ElemType &Elem, const AST::Limit &Lim)
      : Type(Elem), HasMaxSize(Lim.hasMax()), MinSize(Lim.getMin()),
        MaxSize(Lim.getMax()), FuncElem(MinSize) {}
  virtual ~TableInstance() = default;

  /// Getter of element type.
  ElemType getElementType() const { return Type; }

  /// Getter of limit definition.
  bool getHasMax() const { return HasMaxSize; }

  /// Getter of limit definition.
  uint32_t getMin() const { return MinSize; }

  /// Getter of limit definition.
  uint32_t getMax() const { return MaxSize; }

  /// Set the function index initialization list.
  Expect<void> setInitList(const uint32_t Offset,
                           const std::vector<uint32_t> &Addrs) {
    if (Offset + Addrs.size() > MinSize) {
      return Unexpect(ErrCode::TableSizeExceeded);
    }
    std::copy(Addrs.begin(), Addrs.end(), FuncElem.begin() + Offset);
    return {};
  }

  /// Get the elem address.
  Expect<uint32_t> getElemAddr(const uint32_t Idx) const {
    if (Idx >= FuncElem.size()) {
      return Unexpect(ErrCode::AccessForbidMemory);
    }
    return FuncElem[Idx];
  }

private:
  /// \name Data of table instance.
  /// @{
  const ElemType Type;
  const bool HasMaxSize;
  const uint32_t MinSize = 0;
  const uint32_t MaxSize = 0;
  std::vector<uint32_t> FuncElem;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
