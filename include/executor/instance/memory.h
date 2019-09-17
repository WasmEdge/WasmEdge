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
#include "support/casting.h"
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
  ErrCode setLimit(unsigned int Min, bool HasMax, unsigned int Max);

  /// Get length of memory.data
  unsigned int getDataLength() const { return Data.size(); }

  /// Get slice of Data[Start : Start + Length - 1]
  ErrCode getBytes(Bytes &Slice, unsigned int Start, unsigned int Length);

  /// Replace the bytes of Data[Offset : Offset + Slice.size() - 1]
  ErrCode setBytes(Bytes &Slice, unsigned int Offset);

  /// Template of loading bytes and convert to a value.
  ///
  /// Load the length of vector and construct into a value.
  /// Only output value of int32, uint32, int64, uint64, float, and double are
  /// allowed.
  ///
  /// \param Offset the start offset in data array.
  /// \param Length the load length from data. Need to <= sizeof(T).
  /// \param Value the constructed output value.
  ///
  /// \returns ErrCode.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmType<T>::value, ErrCode>
  loadValue(unsigned int Offset, unsigned int Length, T &Value);

  /// Template of loading bytes and convert to a value.
  ///
  /// Destruct and Store the value to length of vector.
  /// Only input value of uint32, uint64, float, and double are allowed.
  ///
  /// \param Offset the start offset in data array.
  /// \param Length the store length to data. Need to <= sizeof(T).
  /// \param Value the value want to store into data array.
  ///
  /// \returns ErrCode.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmBuiltIn<T>::value, ErrCode>
  storeValue(unsigned int Offset, unsigned int Length, const T &Value);

  /// Memory Instance address in store manager.
  unsigned int Addr;

private:
  /// Check access size is valid and adjust vector.
  ErrCode checkDataSize(unsigned int accessSize);

  /// \name Data of memory instance.
  /// @{
  bool HasMaxPage = false;
  unsigned int MinPage = 0;
  unsigned int MaxPage = 0;
  Bytes Data;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
