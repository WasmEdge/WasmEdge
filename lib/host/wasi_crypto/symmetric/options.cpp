// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/options.h"

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using namespace std::literals;

WasiCryptoExpect<void> Options::set(std::string_view Name,
                                    Span<const uint8_t> Value) {
  std::unique_lock<std::shared_mutex> Lock{Mutex};

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

WasiCryptoExpect<void> Options::setU64(std::string_view Name, uint64_t Value) {
  std::unique_lock<std::shared_mutex> Lock{Mutex};

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

WasiCryptoExpect<void> Options::setGuestBuffer(std::string_view Name,
                                               Span<uint8_t> Buffer) {
  std::unique_lock<std::shared_mutex> Lock{Mutex};

  if ("buffer"sv == Name) {
    GuestBuffer = Buffer;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
Options::get(std::string_view Name) const {
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

WasiCryptoExpect<uint64_t> Options::getU64(std::string_view Name) const {
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