// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/instance/table.h - Table Instance definition --------===//
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

#include "ast/common.h"
#include "executor/common.h"
#include "executor/instance/entity.h"
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class TableInstance : public Entity {
public:
  TableInstance() = default;
  virtual ~TableInstance() = default;

  /// Set the element type.
  ErrCode setElemType(AST::ElemType &Elem);

  /// Set the table limit.
  ErrCode setLimit(unsigned int Min, bool HasMax, unsigned int Max);

  /// Set the initialization list.
  ErrCode setInitList(unsigned int Offset, std::vector<unsigned int> &Addrs);

  /// Get the elem address.
  ErrCode getElemAddr(unsigned int Idx, unsigned int &Addr);

private:
  /// \name Data of table instance.
  /// @{
  AST::ElemType Type;
  bool HasMaxSize = false;
  unsigned int MinSize = 0;
  unsigned int MaxSize = 0;
  std::vector<unsigned int> FuncElem;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
