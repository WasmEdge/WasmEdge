// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/log.h"
#include "openssl/ec.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
template <auto Fn> using Deleter = std::integral_constant<decltype(Fn), Fn>;

template <typename T, auto Fn>
using OpenSSLUniquePtr = std::unique_ptr<T, Deleter<Fn>>;
} // namespace

#ifdef NDEBUG
#define opensslAssuming(Cond)                                                  \
  do {                                                                         \
    if (!(Cond)) {                                                             \
      ERR_print_errors_cb(                                                     \
          [](const char *_Str, size_t, void *) {                               \
            spdlog::error(_Str);                                               \
            return 1;                                                          \
          },                                                                   \
          nullptr);                                                            \
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);        \
    }                                                                          \
  } while (0)
#else
#define opensslAssuming(Cond)                                                  \
  (static_cast<bool>(Cond)                                                     \
       ? static_cast<void>(0)                                                  \
       : (ERR_print_errors_cb(                                                 \
              [](const char *_Str, size_t, void *) {                           \
                spdlog::error(_Str);                                           \
                return 1;                                                      \
              },                                                               \
              nullptr),                                                        \
          OPENSSL_die("assertion failed: " #Cond, __FILE__, __LINE__)))
#endif

using EvpMdCtxPtr = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;
using EvpPkeyCtxPtr = OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free>;
using EvpCipherCtxPtr = OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>;
using EvpPkeyPtr = OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>;
using BioPtr = OpenSSLUniquePtr<BIO, BIO_free>;
using EcKeyPtr = OpenSSLUniquePtr<EC_KEY, EC_KEY_free>;
using BnPtr = OpenSSLUniquePtr<BIGNUM, BN_free>;
using EcPointPtr = OpenSSLUniquePtr<EC_POINT, EC_POINT_free>;
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
