// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"

#include <limits>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
// Reference from MSVC STL , It entered the standard library after c++20
template <class T> [[nodiscard]] constexpr T rotr(T Val, int Rotation) noexcept;

template <class T>
[[nodiscard]] constexpr T rotl(const T Val, const int Rotation) noexcept {
  constexpr auto Digits = std::numeric_limits<T>::digits;
  const auto Remainder = Rotation % Digits;
  if (Remainder > 0) {
    return static_cast<T>(static_cast<T>(Val << Remainder) |
                          static_cast<T>(Val >> (Digits - Remainder)));
  }
  if (Remainder == 0) {
    return Val;
  }
  // Remainder < 0
  return rotr(Val, -Remainder);
}

template <class T>
[[nodiscard]] constexpr T rotr(const T Val, const int Rotation) noexcept {
  constexpr auto Digits = std::numeric_limits<T>::digits;
  const auto Remainder = Rotation % Digits;
  if (Remainder > 0) {
    return static_cast<T>(static_cast<T>(Val >> Remainder) |
                          static_cast<T>(Val << (Digits - Remainder)));
  }
  if (Remainder == 0) {
    return Val;
  }
  // Remainder < 0
  return rotl(Val, -Remainder);
}

} // namespace

///
/// @tparam HandleType This is the type of handle, notice they are all `32 byte
/// long`(int32_t).
/// @tparam MangerType The shared content for handle to get
///
/// HandlesManger use handle as index to expression inner.
/// The handle internal representation as [-TypeId-|------CurrentNumber------]
/// Eg.For TypeId=1 first handle will be `00000001 00000000 00000000 00000001`,
/// and next handle will get `00000001 00000000 00000000 00000002`
///
template <typename HandleType, typename MangerType,
          std::enable_if_t<std::is_copy_constructible_v<MangerType>, bool> =
              true>
class HandlesManger {
public:
  HandlesManger(const HandlesManger &) = delete;
  HandlesManger &operator=(const HandlesManger &) = delete;
  HandlesManger(HandlesManger &&) = default;
  HandlesManger &operator=(HandlesManger &&) = default;

  /// @param TypeId A unique number
  HandlesManger(uint8_t TypeId)
      : LastHandle{rotr(static_cast<HandleType>(TypeId), 8)}, TypeId{TypeId} {}

  WasiCryptoExpect<void> close(HandleType Handle) {
    std::unique_lock<std::shared_mutex> Lock{Mutex};

    if (!Map.erase(Handle))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_CLOSED);
    return {};
  }

  WasiCryptoExpect<HandleType> registerManger(MangerType &&Manger) {
    std::unique_lock<std::shared_mutex> Lock{Mutex};

    auto NextHandle = nextHandle(LastHandle);

    while (true) {
      // not have
      if (Map.find(NextHandle) == Map.end()) {
        break;
      }
      if (NextHandle == LastHandle) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_TOO_MANY_HANDLES);
      }
      NextHandle = nextHandle(LastHandle);
    }
    LastHandle = NextHandle;
    if (!Map.emplace(NextHandle, std::forward<MangerType>(Manger)).second) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
    }
    return NextHandle;
  }

  WasiCryptoExpect<MangerType> get(HandleType Handle) {
    std::shared_lock<std::shared_mutex> Lock{Mutex};

    auto HandleValue = Map.find(Handle);
    if (HandleValue == Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return HandleValue->second;
  }

private:
  HandleType nextHandle(HandleType Handle) {
    auto AddedValue = (Handle + 1) << 8;
    return rotr(AddedValue | static_cast<HandleType>(TypeId), 8);
  }

  std::shared_mutex Mutex;
  HandleType LastHandle;
  std::unordered_map<HandleType, MangerType> Map;
  uint8_t TypeId;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge