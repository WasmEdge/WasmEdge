// SPDX-License-Identifier: Apache-2.0
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
#include "executor/instance/entity.h"
#include "support/casting.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class MemoryInstance : public Entity {
private:
  using Byte = unsigned char;
  using Bytes = std::vector<Byte>;

public:
  MemoryInstance() : Data(65536){};
  ~MemoryInstance() = default;

  /// Set the memory limit.
  ErrCode setLimit(unsigned int Min, bool HasMax, unsigned int Max);

  /// Get page size of memory.data
  unsigned int getDataPageSize() const { return CurrPage; }

  /// Grow page
  ErrCode growPage(unsigned int Count);

  /// Get slice of Data[Offset : Offset + Length - 1]
  ErrCode getBytes(Bytes &Slice, unsigned int Offset, unsigned int Length);

  /// Replace the bytes of Data[Offset :] by Slice[Start : Start + Legnth - 1]
  ErrCode setBytes(Bytes &Slice, unsigned int Offset, unsigned int Start,
                   unsigned int Length);

  /// Get pointer to specific offset of memory or null.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointerOrNull(unsigned int Offset);

  /// Get pointer to specific offset of memory.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointer(unsigned int Offset);

  /// Template of loading bytes and convert to a value.
  ///
  /// Load the length of vector and construct into a value.
  /// Only output value of int32, uint32, int64, uint64, float, and double are
  /// allowed.
  ///
  /// \param Value the constructed output value.
  /// \param Offset the start offset in data array.
  /// \param Length the load length from data. Need to <= sizeof(T).
  ///
  /// \returns ErrCode.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmTypeV<T>, ErrCode>
  loadValue(T &Value, unsigned int Offset, unsigned int Length);

  /// Template of loading bytes and convert to a value.
  ///
  /// Destruct and Store the value to length of vector.
  /// Only input value of uint32, uint64, float, and double are allowed.
  ///
  /// \param Value the value want to store into data array.
  /// \param Offset the start offset in data array.
  /// \param Length the store length to data. Need to <= sizeof(T).
  ///
  /// \returns ErrCode.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmBuiltInV<T>, ErrCode>
  storeValue(const T &Value, unsigned int Offset, unsigned int Length);

private:
  /// Check access size is valid and adjust vector.
  ErrCode checkDataSize(unsigned int AccessSize);

  /// \name Data of memory instance.
  /// @{
  bool HasMaxPage = false;
  unsigned int MinPage = 0;
  unsigned int MaxPage = 0;
  unsigned int CurrPage = 0;
  Bytes Data;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM

#include "memory.ipp"
