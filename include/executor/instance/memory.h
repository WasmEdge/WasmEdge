//===-- ssvm/executor/instance/memory.h - Memory Instance definition ------===//
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

#include "executor/common.h"
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class MemoryInstance {
public:
  MemoryInstance() = default;
  ~MemoryInstance() = default;

  /// Set the memory limit.
  ErrCode setLimit(bool HasMax, unsigned int Max);

  /// Set the initialization list.
  ErrCode setInitList(unsigned int Offset, std::vector<unsigned char> &Bytes);

  /// Memory Instance address in store manager.
  unsigned int Addr;

private:
  /// \name Data of memory instance.
  /// @{
  bool HasMaxPage = false;
  unsigned int MaxPage = 0;
  std::vector<unsigned char> Data;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
