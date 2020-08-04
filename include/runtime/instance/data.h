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

#include "common/types.h"
#include "support/span.h"

#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class DataInstance {
public:
  DataInstance() = delete;
  DataInstance(Span<const Byte> Init) : Data(Init.begin(), Init.end()) {}

  /// Get data in data instance.
  Span<const Byte> getData() const noexcept { return Data; }

  /// Clear data in data instance.
  void clear() { Data.clear(); }

private:
  /// \name Data of data instance.
  /// @{
  std::vector<Byte> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
