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
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class MemoryInstance {
private:
  using Byte = unsigned char;
  using Bytes = std::vector<Byte>;

public:
  MemoryInstance() = default;
  ~MemoryInstance() = default;

  /// Set the memory limit.
  ErrCode setLimit(bool HasMax, unsigned int Max);

  /// Set the initialization list.
  ErrCode setInitList(unsigned int Offset, Bytes &InitBytes);

  /// Memory Instance address in store manager.
  unsigned int Addr;

  /// Get length of memory.data
  unsigned int getDataLength() const { return Data.size(); }

  /// Get slice of Data[start:start+length-1]
  ErrCode getBytes(std::unique_ptr<Bytes> &Slice, int Start, int Length);

  /// Replace the bytes of Data[start:start+length-1]
  ErrCode setBytes(Bytes &TheBytes, int Start, int Length);

private:
  /// \name Data of memory instance.
  /// @{
  bool HasMaxPage = false;
  unsigned int MaxPage = 0;
  Bytes Data;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
