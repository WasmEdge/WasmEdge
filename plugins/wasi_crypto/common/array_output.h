// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/common/array_output.h - ArrayOutput --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the ArrayOutput class definition.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

/// Functions returning arrays whose size is not constant or too large to be
/// safely allocated on the stack return a handle to an ArrayOutput type.
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#array-outputs
class ArrayOutput {
public:
  ArrayOutput(const ArrayOutput &) noexcept = delete;
  ArrayOutput &operator=(const ArrayOutput &) noexcept = delete;
  ArrayOutput &operator=(ArrayOutput &&) noexcept = delete;
  ArrayOutput(ArrayOutput &&) noexcept = delete;

  ArrayOutput(std::vector<uint8_t> &&Data) noexcept : Data(std::move(Data)) {}

  ArrayOutput(SecretVec &&Data) noexcept : Data(std::move(Data)) {}

  /// Copy the content to the @param Buf buffer.
  /// Multiple calls are possible, the total number of bytes to be read is
  /// guaranteed to always match the data size.
  ///
  /// @returns the number of bytes read. If all pull, return true.
  std::tuple<size_t, bool> pull(Span<uint8_t> Buf) noexcept;

  /// Return ArrayOutput data size.
  size_t len() const noexcept { return Data.size(); }

private:
  const SecretVec Data;
  size_t Pos = 0;
  std::mutex Mutex;
};

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
