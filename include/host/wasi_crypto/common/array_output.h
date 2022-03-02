// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/common/array_output.h - ArrayOutput definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the class definitions of the wasi-crypto ArrayOutput.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/utils/error.h"

#include <atomic>
#include <memory>
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
  ArrayOutput(std::vector<uint8_t> &&Data) noexcept : Data(std::move(Data)) {}

  /// Copy the content to the @param Buf buffer.
  /// Multiple calls are possible, the total number of bytes to be read is
  /// guaranteed to always match data size
  ///
  /// @returns the number of bytes read. If all pull, return true.
  std::tuple<size_t, bool> pull(Span<uint8_t> Buf) noexcept;

  /// Return ArrayOutput data size
  size_t len() const noexcept { return Data.size(); }

private:
  const std::vector<uint8_t> Data;
  std::atomic<size_t> Pos = 0;
};

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge