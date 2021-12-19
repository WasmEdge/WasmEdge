// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/hkdf.h"
#include "openssl/err.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<HkdfCtx> HkdfCtx::import(SymmetricAlgorithm Alg,
                                          Span<const uint8_t> Key) {

  // init ctx
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};

  if (EVP_PKEY_derive_init(Ctx.get()) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // init md and mode
  EVP_MD const *Md;
  int Mode;
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
    Md = EVP_sha256();
    Mode = EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
    break;
  case SymmetricAlgorithm::HkdfSha256Expand:
    Md = EVP_sha256();
    Mode = EVP_PKEY_HKDEF_MODE_EXPAND_ONLY;
    break;
  case SymmetricAlgorithm::HkdfSha512Extract:
    Md = EVP_sha512();
    Mode = EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
    break;
  case SymmetricAlgorithm::HkdfSha512Expand:
    Md = EVP_sha512();
    Mode = EVP_PKEY_HKDEF_MODE_EXPAND_ONLY;
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }

  if (EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), Md) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (EVP_PKEY_CTX_hkdf_mode(Ctx.get(), Mode) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), Key.data(), Key.size()) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return HkdfCtx{Alg, std::move(Ctx)};
}

WasiCryptoExpect<void> HkdfCtx::absorb(Span<const uint8_t> Data) {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
    if (EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Data.data(), Data.size()) <= 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
    }
    return {};
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    if (EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Data.data(), Data.size()) <= 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
    }
    return {};
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<std::vector<uint8_t>> HkdfCtx::squeezeKey() {
  // check Size
  size_t Size;
  if (EVP_PKEY_derive(Ctx.get(), nullptr, &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // allocate
  std::vector<uint8_t> Data;
  Data.reserve(Size);
  Data.resize(Size);

  if (EVP_PKEY_derive(Ctx.get(), Data.data(), &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Data;
}

WasiCryptoExpect<void> HkdfCtx::squeeze(Span<uint8_t> Out) {
  size_t Size = Out.size();
  if (EVP_PKEY_derive(Ctx.get(), Out.data(), &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
