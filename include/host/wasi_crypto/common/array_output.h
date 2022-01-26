// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wasi_crypto/array_output.h - ArrayOutput definition ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the class definitions of the WasiCrypto ArrayOutput.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"

#include <atomic>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {
/// Functions returning arrays whose size is not constant or too large to be
/// safely allocated on the stack return a handle to an ArrayOutput type.
class ArrayOutput {
public:
  ArrayOutput(std::vector<uint8_t> &&Data) : Data(std::move(Data)) {}

  /// Copy the content to the @param Buf buffer
  /// Multiple calls are possible, the total number of bytes to be read is
  /// guaranteed to always match data size
  std::tuple<size_t, bool> pull(Span<uint8_t> Buf);

  /// Return data size
  size_t len();

private:
  const std::vector<uint8_t> Data;
  std::atomic<size_t> Pos = 0;
};

} // namespace Common

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge