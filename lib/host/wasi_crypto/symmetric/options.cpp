// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

SymmetricOptions::SymmetricOptions()
    : Inner(SymmetricOptionsInner()) {}

WasiCryptoExpect<void> SymmetricOptions::set(std::string_view Name,
                                            Span<const uint8_t> Value) {
  using namespace std::literals;

  auto Data = Inner.lock();
  std::optional<std::vector<uint8_t>> *Res;

  if ("context"sv == Name) {
    Res = &Data->Context;
  } else if ("salt"sv == Name) {
    Res = &Data->Salt;
  } else if ("nonce"sv == Name) {
    Res = &Data->Nonce;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  *Res = std::vector<uint8_t>{Value.begin(), Value.end()};
  return {};
}

WasiCryptoExpect<void> SymmetricOptions::setU64(std::string_view Name,
                                               uint64_t Value) {
  using namespace std::literals;

  auto Data = Inner.lock();
  std::optional<uint64_t> *Res;

  if ("memory_limit"sv == Name) {
    Res = &Data->MemoryLimit;
  } else if ("ops_limit"sv == Name) {
    Res = &Data->OpsLimit;
  } else if ("parallelism"sv == Name) {
    Res = &Data->Parallelism;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  *Res = Value;
  return {};
}

WasiCryptoExpect<void>
SymmetricOptions::setGuestBuffer(std::string_view Name,
                                Span<uint8_t> GuestBuffer) {
  using namespace std::literals;

  auto Data = Inner.lock();

  if ("buffer"sv == Name) {
    Data->GuestBuffer = GuestBuffer;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
SymmetricOptions::get(std::string_view Name) {
  using namespace std::literals;

  auto Data = Inner.lock();
  std::optional<std::vector<uint8_t>> *Res;

  if ("context"sv == Name) {
    Res = &Data->Context;
  } else if ("salt"sv == Name) {
    Res = &Data->Salt;
  } else if ("nonce"sv == Name) {
    Res = &Data->Nonce;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return **Res;
}

WasiCryptoExpect<uint64_t> SymmetricOptions::getU64(std::string_view Name) {
  using namespace std::literals;

  auto Data = Inner.lock();
  std::optional<uint64_t> *Res;

  if ("memory_limit"sv == Name) {
    Res = &Data->MemoryLimit;
  } else if ("ops_limit"sv == Name) {
    Res = &Data->OpsLimit;
  } else if ("parallelism"sv == Name) {
    Res = &Data->Parallelism;
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return **Res;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge