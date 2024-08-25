// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/kx/options.h - Key exchange Options --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the Key Exchange Options class definition.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"

#include "common/span.h"

#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

class Options {
public:
  WasiCryptoExpect<void> set(std::string_view Name,
                             Span<const uint8_t> Value) noexcept;

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value) noexcept;

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> Buffer) noexcept;

  WasiCryptoExpect<size_t> get(std::string_view Name,
                               Span<uint8_t> Value) const noexcept;

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name) const noexcept;
};

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
