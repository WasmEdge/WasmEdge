// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "openssl/err.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
template <auto Fn> using Deleter = std::integral_constant<decltype(Fn), Fn>;

template <typename T, auto Fn>
using OpenSSLUniquePtr = std::unique_ptr<T, Deleter<Fn>>;
} // namespace

inline const std::map<int, const EVP_MD *> ShaMap{
    {256, EVP_sha256()},
    {384, EVP_sha384()},
    {512, EVP_sha512()},
};

#ifdef NDEBUG
#define opensslAssuming(Cond)                                                  \
  (static_cast<bool>(Cond) ? static_cast<void>(0) : __builtin_unreachable())
#else
#define opensslAssuming(Cond)                                                  \
  (static_cast<bool>(Cond)                                                     \
       ? static_cast<void>(0)                                                  \
       : (ERR_print_errors_fp(stderr),                                         \
          OPENSSL_die("assertion failed: " #Cond, __FILE__, __LINE__)))
#endif

using EvpMdCtxPtr = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;
using EvpPkeyCtxPtr = OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free>;
using EvpCipherCtxPtr = OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>;
using EvpPkeyPtr = OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>;
using BioPtr = OpenSSLUniquePtr<BIO, BIO_free>;
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
