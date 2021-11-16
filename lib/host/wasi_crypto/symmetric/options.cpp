// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> SymmetricOptions::set(std::string_view Name,
                                             Span<const uint8_t> Value) {
  if ("context" == Name) {
    Inner->locked([Value](SymmetricOptionsInner &Data) {
      Data.Context = {Value.begin(), Value.end()};
    });
  } else if ("salt" == Name) {
    Inner->locked([Value](SymmetricOptionsInner &Data) {
      Data.Salt = {Value.begin(), Value.end()};
    });

  } else if ("nonce" == Name) {
    Inner->locked([Value](SymmetricOptionsInner &Data) {
      Data.Nonce = {Value.begin(), Value.end()};
    });
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<void> SymmetricOptions::setU64(std::string_view Name,
                                                uint64_t Value) {
  if ("memory_limit" == Name) {
    Inner->locked(
        [Value](SymmetricOptionsInner &Data) { Data.MemoryLimit = Value; });
  } else if ("ops_limit" == Name) {
    Inner->locked(
        [Value](SymmetricOptionsInner &Data) { Data.OpsLimit = Value; });
  } else if ("parallelism" == Name) {
    Inner->locked(
        [Value](SymmetricOptionsInner &Data) { Data.Parallelism = Value; });
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<void>
SymmetricOptions::setGuestBuffer(std::string_view Name,
                                 Span<uint8_t> GuestBuffer) {
  if ("buffer" == Name) {
    Inner->locked([GuestBuffer](SymmetricOptionsInner &Data) {
      Data.GuestBuffer = GuestBuffer;
    });
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
SymmetricOptions::get(std::string_view Name) {
  std::optional<std::vector<uint8_t>> Res;
  if ("context" == Name) {
    Res =
        Inner->locked([](SymmetricOptionsInner &Data) { return Data.Context; });
  } else if ("salt" == Name) {
    Res = Inner->locked([](SymmetricOptionsInner &Data) { return Data.Salt; });
  } else if ("nonce" == Name) {
    Res = Inner->locked([](SymmetricOptionsInner &Data) { return Data.Nonce; });
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }
  if (!Res) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return *Res;
}

WasiCryptoExpect<uint64_t> SymmetricOptions::getU64(std::string_view Name) {
  std::optional<uint64_t> Res;

  if ("memory_limit" == Name) {
    Res = Inner->locked(
        [](SymmetricOptionsInner &Data) { return Data.MemoryLimit; });
  } else if ("ops_limit" == Name) {
    Res =
        Inner->locked([](SymmetricOptionsInner &Data) { return Data.OpsLimit; });
  } else if ("parallelism" == Name) {
    Res = Inner->locked(
        [](SymmetricOptionsInner &Data) { return Data.Parallelism; });
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  if (!Res) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return *Res;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge