// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/data.h - Data Instance definition -------===//
//
// Part of the WasmEdge Project.
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

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class DataInstance {
public:
  DataInstance() = delete;
  DataInstance(const addr_t Offset, Span<const Byte> Init) noexcept
      : Off(Offset), Data(Init.begin(), Init.end()) {}

  /// Get offset in data instance.
  addr_t getOffset() const noexcept { return Off; }

  /// Get data in data instance.
  Span<const Byte> getData() const noexcept { return Data; }

  /// Load bytes to value.
  ValVariant loadValue(const addr_t Offset, const addr_t N) const noexcept {
    assuming(N <= 16);
    // Due to applying the Memory64 proposal, we should avoid the overflow issue
    // of the following code.
    // Check the data boundary.
    if (unlikely(std::numeric_limits<addr_t>::max() - Offset < N ||
                 Offset + N > Data.size())) {
      return 0;
    }
    // Load the data to the value.
    EndianValue<uint128_t> Value;
    std::memcpy(&Value.raw(), &Data[Offset], N);
    if constexpr (Endian::native == Endian::big) {
      Value.raw() >>= (128 - N * 8);
    }
    return Value.le();
  }

  /// Clear data in data instance.
  void clear() noexcept { Data.clear(); }

private:
  /// \name Data of data instance.
  /// @{
  const addr_t Off;
  std::vector<Byte> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
