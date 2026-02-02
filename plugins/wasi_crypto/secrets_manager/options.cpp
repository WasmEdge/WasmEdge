// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "secrets_manager/options.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace SecretsManager {
using namespace std::literals;

namespace {
constexpr std::array<std::string_view, 1> ValidNames{"password"sv};

std::string toLower(std::string_view Name) noexcept {
  std::string Ret{Name};
  std::transform(Ret.begin(), Ret.end(), Ret.begin(),
                 [](char C) { return static_cast<char>(std::tolower(C)); });
  return Ret;
}

bool isValidName(std::string_view Name) noexcept {
  return std::find(ValidNames.begin(), ValidNames.end(), Name) !=
         ValidNames.end();
}

constexpr std::array<std::string_view, 1> ValidU64Names{"auto_lock"sv};

bool isValidU64Name(std::string_view Name) noexcept {
  return std::find(ValidU64Names.begin(), ValidU64Names.end(), Name) !=
         ValidU64Names.end();
}

} // namespace

WasiCryptoExpect<void> Options::set(std::string_view Name,
                                    Span<const uint8_t> Value) noexcept {
  std::string ActuallyName = toLower(Name);

  ensureOrReturn(isValidName(ActuallyName),
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

  {
    std::unique_lock<std::shared_mutex> Lock{Inner->Mutex};
    Inner->ValueMap.insert_or_assign(ActuallyName,
                                     std::vector(Value.begin(), Value.end()));
  }
  return {};
}

WasiCryptoExpect<void> Options::setU64(std::string_view Name,
                                       uint64_t Value) noexcept {
  std::string ActuallyName = toLower(Name);
  ensureOrReturn(isValidU64Name(ActuallyName),
                 __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  {
    std::unique_lock<std::shared_mutex> Lock{Inner->Mutex};
    Inner->U64ValueMap.insert_or_assign(ActuallyName, Value);
  }
  return {};
}

WasiCryptoExpect<void> Options::setGuestBuffer(std::string_view,
                                               Span<uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<size_t> Options::get(std::string_view Name,
                                      Span<uint8_t> Value) const noexcept {
  std::string ActuallyName = toLower(Name);
  ensureOrReturn(isValidName(ActuallyName),
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
  ensureOrReturn(isValidU64Name(ActuallyName),
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

} // namespace SecretsManager
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
