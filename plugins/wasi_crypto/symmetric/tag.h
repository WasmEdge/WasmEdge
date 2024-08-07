// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/tag.h - Symmetric Tag class ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the Symmetric Tag definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "utils/error.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Authentication tag, that can be verified without channels using the provided
/// APIs. Very small and no streaming.
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#authentication-tags
class Tag {
public:
  Tag(Tag &&Data) noexcept = default;
  Tag &operator=(Tag &&Data) noexcept = default;
  Tag(const Tag &Data) noexcept = delete;
  Tag &operator=(const Tag &Data) noexcept = delete;

  Tag(SecretVec &&Data) noexcept : Data(std::move(Data)) {}

  size_t len() const noexcept { return Data.size(); }

  /// The function MUST return `__WASI_CRYPTO_ERRNO_INVALID_TAG` if the
  /// tags don't match.
  WasiCryptoExpect<void> verify(Span<const uint8_t> RawTag) const noexcept;

  WasiCryptoExpect<size_t> pull(Span<uint8_t> Raw) const noexcept;

private:
  SecretVec Data;
};

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
