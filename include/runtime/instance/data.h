// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/data.h - Data Instance definition -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the data instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "common/types.h"

#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class DataInstance {
public:
  DataInstance() = delete;
  DataInstance(const uint32_t Offset, Span<const Byte> Init)
      : Off(Offset), Data(Init.begin(), Init.end()) {}

  /// Get offset in data instance.
  uint32_t getOffset() const noexcept { return Off; }

  /// Get data in data instance.
  Span<const Byte> getData() const noexcept { return Data; }

  /// Clear data in data instance.
  void clear() { Data.clear(); }

private:
  /// \name Data of data instance.
  /// @{
  const uint32_t Off;
  std::vector<Byte> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
