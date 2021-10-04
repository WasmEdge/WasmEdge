// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/aot/blake3.h - Blake3 hash class definition --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Blake3 class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "common/value.h"
#include <blake3.h>
#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace AOT {

/// Hash data with blake3 algorithm.
class Blake3 {
public:
  Blake3() noexcept;
  void update(Span<const Byte> Data) noexcept;
  void finalize(Span<Byte> Output) noexcept;

private:
  blake3_hasher Hasher;
};

} // namespace AOT
} // namespace WasmEdge
