//===-- ssvm/executor/memoryinst.h - Memory Instance definition -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the memory instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include <vector>

namespace SSVM {
namespace Executor {

class MemoryInstance {
public:
  MemoryInstance() = default;
  ~MemoryInstance() = default;

  /// Set the memory limit.
  Executor::ErrCode setLimit(bool HasMax, unsigned int Max);

  /// Set the initialization list.
  Executor::ErrCode setInitList(unsigned int Offset,
                                std::vector<unsigned char> &Bytes);

  /// Memory Instance address in store manager.
  unsigned int Addr;

private:
  /// \name Data of memory instance.
  /// @{
  bool HasMaxPage;
  unsigned int MaxPage;
  std::vector<unsigned char> Data;
  /// @}
};

} // namespace Executor
} // namespace SSVM
