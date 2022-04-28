// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/utils/evp_wrapper.h - Evp Wrapper ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of openssl evp relative function.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/log.h"
#include "common/span.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/secret_vec.h"
#include "openssl/bio.h"
#include "openssl/ec.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/rand.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// Helper alias for openssl relative struct.
template <auto Fn> using Deleter = std::integral_constant<decltype(Fn), Fn>;

template <typename T, auto Fn>
using OpenSSLUniquePtr = std::unique_ptr<T, Deleter<Fn>>;

using EvpMdCtxPtr = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;
using EvpPkeyCtxPtr = OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free>;
using EvpCipherCtxPtr = OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>;
using EvpPkeyPtr = OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>;
using BioPtr = OpenSSLUniquePtr<BIO, BIO_free>;
using EcKeyPtr = OpenSSLUniquePtr<EC_KEY, EC_KEY_free>;
using BnPtr = OpenSSLUniquePtr<BIGNUM, BN_free>;
using EcPointPtr = OpenSSLUniquePtr<EC_POINT, EC_POINT_free>;
using EcdsaSigPtr = OpenSSLUniquePtr<ECDSA_SIG, ECDSA_SIG_free>;
using RsaPtr = OpenSSLUniquePtr<RSA, RSA_free>;

/// openssl function always return 1 for success and 0/NULL for failure. This
/// used to reduce repeat check
#ifdef NDEBUG
#define opensslCheck(Cond)                                                     \
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
#define opensslCheck(Cond)                                                     \
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

/// OpenSSL encoding parse api is too confusing, simplify them
/// For example, `PEM_read_bio_PUBKEY` equal to `pemReadPUBKEY`
// -------------------------------------------------------------------------  //
EVP_PKEY *pemReadPUBKEY(Span<const uint8_t> Encoded);

WasiCryptoExpect<std::vector<uint8_t>> pemWritePUBKEY(EVP_PKEY *Key);

EVP_PKEY *pemReadPrivateKey(Span<const uint8_t> Encoded);

WasiCryptoExpect<SecretVec> pemWritePrivateKey(EVP_PKEY *Key);

EVP_PKEY *d2iPUBKEY(Span<const uint8_t> Encoded);

WasiCryptoExpect<std::vector<uint8_t>> i2dPUBKEY(EVP_PKEY *Key);

EVP_PKEY *d2iPrivateKey(Span<const uint8_t> Encoded);

WasiCryptoExpect<SecretVec> i2dPrivateKey(EVP_PKEY *Key);

ECDSA_SIG *d2iEcdsaSig(Span<const uint8_t> Encoded);

WasiCryptoExpect<std::vector<uint8_t>> i2dEcdsaSig(ECDSA_SIG *Sig);

// -------------------------------------------------------------------------  //

// transform raw represent ecdsa ( r | s) to ECDSA_SIG, need check `nullptr`
ECDSA_SIG *o2iEcdsaSig(Span<const uint8_t> Encoded);

// transform ECDSA_SIG to raw represent ( r | s)
WasiCryptoExpect<std::vector<uint8_t>> i2oEcdsaSig(ECDSA_SIG *Sig);

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
