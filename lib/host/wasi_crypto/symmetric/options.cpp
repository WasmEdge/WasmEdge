// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> SymmetricOptions::Inner::set(std::string_view Name,
                                                    Span<const uint8_t> Value) {
  std::optional<std::vector<uint8_t>> *Res;
  if ("context" == Name) {
    Res = &Context;
  } else if ("salt" == Name) {
    Res = &Salt;
  } else if ("nonce" == Name) {
    Res = &Nonce;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }
  *Res = {Value.begin(), Value.end()};
  return {};
}

WasiCryptoExpect<void> SymmetricOptions::Inner::setU64(std::string_view Name,
                                                       uint64_t Value) {
  std::optional<uint64_t> *Res;
  if ("memory_limit" == Name) {
    Res = &MemoryLimit;
  } else if ("ops_limit" == Name) {
    Res = &OpsLimit;
  } else if ("parallelism" == Name) {
    Res = &Parallelism;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }
  *Res = Value;
  return {};
}

WasiCryptoExpect<void>
SymmetricOptions::Inner::setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> Buffer) {
  if ("buffer" == Name) {
    GuestBuffer = Buffer;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
SymmetricOptions::Inner::get(std::string_view Name) {
  std::optional<std::vector<uint8_t>> *Res;
  if ("context" == Name) {
    Res = &Context;
  } else if ("salt" == Name) {
    Res = &Salt;
  } else if ("nonce" == Name) {
    Res = &Nonce;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }
  if (!Res->has_value()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return **Res;
}

WasiCryptoExpect<uint64_t>
SymmetricOptions::Inner::getU64(std::string_view Name) {
  std::optional<uint64_t> *Res;

  if ("memory_limit" == Name) {
    Res = &MemoryLimit;
  } else if ("ops_limit" == Name) {
    Res = &OpsLimit;
  } else if ("parallelism" == Name) {
    Res = &Parallelism;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  if (!Res->has_value()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return **Res;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge