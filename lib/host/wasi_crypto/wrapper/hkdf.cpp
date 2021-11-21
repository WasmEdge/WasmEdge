// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/hkdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Hkdf> Hkdf::make(SymmetricAlgorithm Alg) {
  // init ctx
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
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

  // init options
  //  if (OptOptions != nullptr) {
  //    auto Data = OptOptions->Inner.lock();
  //    if (auto &Salt = Data->Salt; Salt) {
  //      if (EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Salt->data(),
  //      Salt->size())
  //      <=
  //          0) {
  //        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  //      }
  //    }
  //  }

  return Hkdf{Alg, std::move(Ctx)};
}

WasiCryptoExpect<void> Hkdf::setKey(Span<uint8_t> Key) {
  if (EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), Key.data(), Key.size()) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<void> Hkdf::absorb(Span<const uint8_t> Data) {
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

WasiCryptoExpect<Span<const uint8_t>> Hkdf::squeezeKey() {
  // check Size
  size_t Size;
  if (EVP_PKEY_derive(Ctx.get(), nullptr, &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // allocate
  uint8_t Data[Size];
  size_t NewSize;
  if (EVP_PKEY_derive(Ctx.get(), Data, &NewSize) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // check
  if (NewSize != Size) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  Span<uint8_t const> Res{*&Data, NewSize};
  return Res;
}

WasiCryptoExpect<void> Hkdf::squeeze(Span<uint8_t> Out) {
  // check Size
  size_t Size;
  //  if (EVP_PKEY_derive(Ctx.get(), nullptr, &Size) <= 0) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  //  }
  //
  //  if (Out.size() < Size) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  //  }

  if (EVP_PKEY_derive(Ctx.get(), Out.data(), &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

Hkdf::Hkdf(SymmetricAlgorithm Alg,
           OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx)
    : Alg(Alg), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
