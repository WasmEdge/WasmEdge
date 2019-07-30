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
#include <memory>

class MemoryInstance {
public:
  MemoryInstance() = default;
  ~MemoryInstance() = default;

  /// Set the memory limit.
  Executor::ErrCode setLimit(bool HasMax, unsigned int Max);

  /// Set the initialization list.
  Executor::ErrCode setInitList(unsigned int Offset,
                                std::vector<unsigned char> &Bytes);

  /// Memory Instance ID in store manager.
  unsigned int Id;

private:
  /// \name Data of memory instance.
  /// @{
  bool HasMax;
  unsigned int Max;
  std::vector<unsigned char> Data;
  /// @}
};