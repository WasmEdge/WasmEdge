// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using namespace std::literals;

WasiCryptoExpect<void> Option::set(std::string_view Name,
                                   Span<const uint8_t> Value) {
  std::unique_lock Lock{Mutex};

  std::optional<std::vector<uint8_t>> *Res;
  if ("context"sv == Name) {
    Res = &Context;
  } else if ("salt"sv == Name) {
    Res = &Salt;
  } else if ("nonce"sv == Name) {
    Res = &Nonce;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }
  *Res = {Value.begin(), Value.end()};
  return {};
}

WasiCryptoExpect<void> Option::setU64(std::string_view Name, uint64_t Value) {
  std::unique_lock Lock{Mutex};

  std::optional<uint64_t> *Res;
  if ("memory_limit"sv == Name) {
    Res = &MemoryLimit;
  } else if ("ops_limit"sv == Name) {
    Res = &OpsLimit;
  } else if ("parallelism"sv == Name) {
    Res = &Parallelism;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }
  *Res = Value;
  return {};
}

WasiCryptoExpect<void> Option::setGuestBuffer(std::string_view Name,
                                              Span<uint8_t> Buffer) {
  std::unique_lock Lock{Mutex};

  if ("buffer"sv == Name) {
    GuestBuffer = Buffer;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
Option::get(std::string_view Name) const {
  std::shared_lock Lock{Mutex};

  std::optional<std::vector<uint8_t>> const *Res;
  if ("context"sv == Name) {
    Res = &Context;
  } else if ("salt"sv == Name) {
    Res = &Salt;
  } else if ("nonce"sv == Name) {
    Res = &Nonce;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  if (!Res->has_value()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return **Res;
}

WasiCryptoExpect<uint64_t> Option::getU64(std::string_view Name) const {
  std::shared_lock Lock{Mutex};

  std::optional<uint64_t> const *Res;

  if ("memory_limit"sv == Name) {
    Res = &MemoryLimit;
  } else if ("ops_limit"sv == Name) {
    Res = &OpsLimit;
  } else if ("parallelism"sv == Name) {
    Res = &Parallelism;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  if (!Res->has_value()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return **Res;
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge