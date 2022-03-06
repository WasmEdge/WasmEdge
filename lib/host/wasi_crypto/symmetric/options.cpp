// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/options.h"

#include <algorithm>
#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {
using namespace std::literals;

namespace {
constexpr std::array<std::string_view, 3> ValueName{"context"sv, "salt"sv,
                                                    "nonce"sv};

std::string toLower(std::string_view Name) noexcept {
  std::string Ret{Name};
  std::transform(Ret.begin(), Ret.end(), Ret.begin(),
                 [](char C) { return std::tolower(C); });
  return Ret;
}

bool valueNameMatch(std::string_view Name) noexcept {
  return std::any_of(ValueName.begin(), ValueName.end(),
                     [Name](auto &&Input) { return Name == Input; });
}

constexpr std::array<std::string_view, 3> U64ValueName{
    "memory_limit"sv, "ops_limit"sv, "parallelism"sv};

bool u64ValueNameMatch(std::string_view Name) noexcept {
  return std::any_of(U64ValueName.begin(), U64ValueName.end(),
                     [Name](auto &&Input) { return Name == Input; });
}

} // namespace

WasiCryptoExpect<void> Options::set(std::string_view Name,
                                    Span<const uint8_t> Value) noexcept {
  std::string ActuallyName = toLower(Name);

  ensureOrReturn(valueNameMatch(ActuallyName),
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

  {
    std::unique_lock<std::shared_mutex> Lock{Inner->Mutex};
    Inner->ValueMap.try_emplace(
        ActuallyName, std::vector<uint8_t>{Value.begin(), Value.end()});
  }
  return {};
}

WasiCryptoExpect<void> Options::setU64(std::string_view Name,
                                       uint64_t Value) noexcept {
  std::string ActuallyName = toLower(Name);
  ensureOrReturn(u64ValueNameMatch(ActuallyName),
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  {
    std::unique_lock<std::shared_mutex> Lock{Inner->Mutex};
    Inner->U64ValueMap.try_emplace(ActuallyName, Value);
  }
  return {};
}

WasiCryptoExpect<void> Options::setGuestBuffer(std::string_view Name,
                                               Span<uint8_t> Buffer) noexcept {
  std::string ActuallyName = toLower(Name);
  ensureOrReturn(ActuallyName == "buffer"sv,
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  {
    std::unique_lock<std::shared_mutex> Lock{Inner->Mutex};
    Inner->GuestBuffer = Buffer;
  }
  return {};
}

WasiCryptoExpect<size_t> Options::get(std::string_view Name,
                                      Span<uint8_t> Value) const noexcept {
  std::string ActuallyName = toLower(Name);
  ensureOrReturn(valueNameMatch(ActuallyName),
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  {
    std::shared_lock<std::shared_mutex> Lock{Inner->Mutex};
    if (auto It = Inner->ValueMap.find(ActuallyName);
        It != Inner->ValueMap.end()) {
      ensureOrReturn(It->second.size() <= Value.size(),
                     __WASI_CRYPTO_ERRNO_OVERFLOW);
      std::copy(It->second.begin(), It->second.end(), Value.begin());
      return It->second.size();
    }
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
}

WasiCryptoExpect<uint64_t>
Options::getU64(std::string_view Name) const noexcept {
  std::string ActuallyName = toLower(Name);
  ensureOrReturn(u64ValueNameMatch(ActuallyName),
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  {
    std::shared_lock<std::shared_mutex> Lock{Inner->Mutex};
    if (auto It = Inner->U64ValueMap.find(ActuallyName);
        It != Inner->U64ValueMap.end()) {
      return It->second;
    }
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge