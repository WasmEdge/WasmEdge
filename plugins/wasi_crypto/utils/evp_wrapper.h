// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/utils/evp_wrapper.h - Evp Wrapper ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of OpenSSL evp relative function.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"
#include "utils/secret_vec.h"

#include "common/span.h"
#include "common/spdlog.h"

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// Helper alias for OpenSSL relative struct.
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

/// OpenSSL functions always return 1 for success and 0/NULL for failure. This
/// is used to reduce repeating checking.
#ifdef NDEBUG
#define opensslCheck(Cond)                                                     \
  do {                                                                         \
    if (!(Cond)) {                                                             \
      using namespace std::literals;                                           \
      ERR_print_errors_cb(                                                     \
          [](const char *ErrStr, size_t ErrLen, void *) {                      \
            spdlog::error("{}"sv, std::string_view(ErrStr, ErrLen));           \
            return 1;                                                          \
          },                                                                   \
          nullptr);                                                            \
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);        \
    }                                                                          \
  } while (false)
#else
#define opensslCheck(Cond)                                                     \
  do {                                                                         \
    if (!(Cond)) {                                                             \
      using namespace std::literals;                                           \
      ERR_print_errors_cb(                                                     \
          [](const char *ErrStr, size_t ErrLen, void *) {                      \
            spdlog::error("{}"sv, std::string_view(ErrStr, ErrLen));           \
            return 1;                                                          \
          },                                                                   \
          nullptr);                                                            \
      OPENSSL_die("assertion failed: " #Cond, __FILE__, __LINE__);             \
    }                                                                          \
  } while (false)
#endif

/// OpenSSL encoding parse api is too confusing, simplify them.
/// For example, `PEM_read_bio_PUBKEY` is equal to `pemReadPUBKEY`.

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
// Transform raw represent ecdsa ( r | s) to ECDSA_SIG. Need to check `nullptr`.
ECDSA_SIG *o2iEcdsaSig(Span<const uint8_t> Encoded);

// Transform ECDSA_SIG to raw represent ( r | s).
WasiCryptoExpect<std::vector<uint8_t>> i2oEcdsaSig(ECDSA_SIG *Sig);

// This is a wrapper for EVP_PKEY, since EVP_PKEY inner use lock to guarantee
// thread-safe `EVP_PKEY_up_ref` (you will find them in crypto/evp/p_lib.c in
// OpenSSL v1.1.1), use shared_ptr for `EVP_PKEY` is wasted.
// It only provide limits function to correct use.
class SharedEvpPkey {
public:
  SharedEvpPkey(EvpPkeyPtr Pkey) noexcept : Pkey(Pkey.release()) {}
  ~SharedEvpPkey() noexcept;

  SharedEvpPkey(const SharedEvpPkey &Rhs) noexcept;
  SharedEvpPkey(SharedEvpPkey &&Rhs) noexcept;
  // Assigning to existing SharedEvpPkey is not thread-safe, delete them.
  SharedEvpPkey &operator=(const SharedEvpPkey &Rhs) noexcept = delete;
  SharedEvpPkey &operator=(SharedEvpPkey &&Rhs) noexcept = delete;

  EVP_PKEY *get() const noexcept;

  explicit operator bool() const noexcept;

private:
  EVP_PKEY *Pkey;
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
