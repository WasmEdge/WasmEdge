// SPDX-License-Identifier: Apache-2.0
#pragma once
#define OPENSSL_API_COMPAT 0x10100000L

#include "common/log.h"
#include "openssl/err.h"
#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
#ifdef NDEBUG
#define opensslAssuming(R)                                                     \
  (static_cast<bool>(R) ? static_cast<void>(0) : __builtin_unreachable())
#else
#define opensslAssuming(cond)                                                  \
  (static_cast<bool>(cond)                                                     \
       ? static_cast<void>(0)                                                  \
       : (ERR_print_errors_fp(stderr),                                         \
          OPENSSL_die("assertion failed: " #cond, __FILE__, __LINE__)))
#endif


template <auto Fn> using Deleter = std::integral_constant<decltype(Fn), Fn>;

template <typename T, auto Fn>
using OpenSSLUniquePtr = std::unique_ptr<T, Deleter<Fn>>;


} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
