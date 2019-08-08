//===-- ssvm/executor/tableinst.h - Table Instance definition -------------===//
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
#include "common.h"
#include <vector>

namespace SSVM {

class TableInstance {
public:
  TableInstance() = default;
  ~TableInstance() = default;

  /// Set the element type.
  Executor::ErrCode setElemType(AST::ElemType &Elem);

  /// Set the table limit.
  Executor::ErrCode setLimit(bool HasMax, unsigned int Max);

  /// Set the initialization list.
  Executor::ErrCode setInitList(unsigned int Offset,
                                std::vector<unsigned int> &Addrs);

  /// Table Instance ID in store manager.
  unsigned int Id;

private:
  /// \name Data of table instance.
  /// @{
  AST::ElemType Type;
  bool HasMaxSize;
  unsigned int MaxSize;
  std::vector<unsigned int> FuncElem;
  /// @}
};

} // namespace SSVM