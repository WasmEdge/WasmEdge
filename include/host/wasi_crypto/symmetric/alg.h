// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/util.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

enum class SymmetricAlgorithm {
  None,
  HmacSha256,
  HmacSha512,
  HkdfSha256Extract,
  HkdfSha512Extract,
  HkdfSha256Expand,
  HkdfSha512Expand,
  Sha256,
  Sha512,
  Sha512_256,
  Aes128Gcm,
  Aes256Gcm,
  ChaCha20Poly1305,
  XChaCha20Poly1305,
  Xoodyak128,
  Xoodyak160
};

template <>
constexpr WasiCryptoExpect<SymmetricAlgorithm>
tryFrom(std::string_view Value) noexcept {
  if (Value == "HKDF-EXTRACT/SHA-256")
    return SymmetricAlgorithm::HkdfSha256Extract;
  if (Value == "HKDF-EXTRACT/SHA-512")
    return SymmetricAlgorithm::HkdfSha512Extract;
  if (Value == "HKDF-EXPAND/SHA-256")
    return SymmetricAlgorithm::HkdfSha256Expand;
  if (Value == "HKDF-EXPAND/SHA-512")
    return SymmetricAlgorithm::HkdfSha512Expand;
  if (Value == "HMAC/SHA-256")
    return SymmetricAlgorithm::HmacSha256;
  if (Value == "HMAC/SHA-512")
    return SymmetricAlgorithm::HmacSha512;
  if (Value == "SHA-256")
    return SymmetricAlgorithm::Sha256;
  if (Value == "SHA-512")
    return SymmetricAlgorithm::Sha512;
  if (Value == "SHA-512/256")
    return SymmetricAlgorithm::Sha512_256;
  if (Value == "AES-128-GCM")
    return SymmetricAlgorithm::Aes128Gcm;
  if (Value == "AES-256-GCM")
    return SymmetricAlgorithm::Aes256Gcm;
  if (Value == "CHACHA20-POLY1305")
    return SymmetricAlgorithm::ChaCha20Poly1305;
  if (Value == "XCHACHA20-POLY1305")
    return SymmetricAlgorithm::XChaCha20Poly1305;
  if (Value == "XOODYAK-128")
    return SymmetricAlgorithm::Xoodyak128;
  if (Value == "XOODYAK-160")
    return SymmetricAlgorithm::Xoodyak160;
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
