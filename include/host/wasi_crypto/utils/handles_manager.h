// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/utils/handles_manager.h - HandlesManger definition ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the class definitions of the WasiCrypto HandlesManger, it
/// control handle and inner state
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

#include <limits>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace detail {

/// Reference from MSVC STL , It join the standard library after c++20
/// https://en.cppreference.com/w/cpp/numeric/rotr
template <class T> [[nodiscard]] constexpr T rotr(T Val, int Rotation) noexcept;

/// https://en.cppreference.com/w/cpp/numeric/rotl
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

///
/// @tparam HandleType This is the type of handle, notice they are all `32 byte
/// long`.
/// @tparam ManagerType The shared content for handle to get
///
/// HandlesManager use handle as index to expression inner.
/// The handle internal representation as [- TypeId-|------CurrentNumber------]
/// Eg.For TypeId=1 first handle will be `00000001 00000000 00000000 00000001`,
/// and next handle will get `00000001 00000000 00000000 00000002`
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#handles
template <typename HandleType, typename ManagerType> class BaseHandlesManager {
public:
  static_assert(sizeof(HandleType) == 4, "HandleType must be 4 byte");

  BaseHandlesManager(const BaseHandlesManager &) = delete;
  BaseHandlesManager &operator=(const BaseHandlesManager &) = delete;
  BaseHandlesManager(BaseHandlesManager &&) = default;
  BaseHandlesManager &operator=(BaseHandlesManager &&) = default;

  /// @param TypeId A unique number
  BaseHandlesManager(uint8_t TypeId) noexcept
      : LastHandle{rotr(static_cast<uint32_t>(TypeId), 8)}, TypeId{TypeId} {}

  WasiCryptoExpect<void> close(HandleType Handle) noexcept {
    std::unique_lock<std::shared_mutex> Lock{Mutex};

    if (!Map.erase(static_cast<uint32_t>(Handle)))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_CLOSED);
    return {};
  }

  template <typename... Args>
  WasiCryptoExpect<HandleType> registerManager(Args &&...Manager) noexcept {
    std::unique_lock<std::shared_mutex> Lock{Mutex};

    uint32_t NextHandle = nextHandle(LastHandle);
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
    if (!Map.try_emplace(NextHandle, std::forward<Args>(Manager)...).second) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_TOO_MANY_HANDLES);
    }
    return static_cast<HandleType>(NextHandle);
  }

protected:
  uint32_t nextHandle(uint32_t Handle) noexcept {
    uint32_t AddedValue = (Handle + 1) << 8;
    return rotr(AddedValue | static_cast<uint32_t>(TypeId), 8);
  }

  std::shared_mutex Mutex;
  uint32_t LastHandle;
  std::unordered_map<uint32_t, ManagerType> Map;
  uint8_t TypeId;
};

template <typename T, typename VariantType> struct IsVariantMember;
template <typename T, typename... AllType>
struct IsVariantMember<T, std::variant<AllType...>>
    : public std::disjunction<std::is_same<T, AllType>...> {};

} // namespace detail

/// MangerType need reference count
template <typename HandleType, typename ManagerType,
          std::enable_if_t<std::is_copy_constructible_v<ManagerType>, bool> =
              false>
class RcHandlesManager
    : public detail::BaseHandlesManager<HandleType, ManagerType> {
public:
  /// get return copy
  WasiCryptoExpect<ManagerType> get(HandleType Handle) noexcept {
    std::shared_lock<std::shared_mutex> Lock{this->Mutex};

    auto HandleValue = this->Map.find(static_cast<uint32_t>(Handle));
    if (HandleValue == this->Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return HandleValue->second;
  }

  /// get as different variant type
  template <typename RequiredVariantType>
  WasiCryptoExpect<RequiredVariantType> getAs(HandleType Handle) noexcept {
    std::shared_lock<std::shared_mutex> Lock{this->Mutex};

    auto HandleValue = this->Map.find(static_cast<uint32_t>(Handle));
    if (HandleValue == this->Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return std::visit(
        [](auto &&Value) noexcept -> WasiCryptoExpect<RequiredVariantType> {
          using T = std::decay_t<decltype(Value)>;
          if constexpr (detail::IsVariantMember<T,
                                                RequiredVariantType>::value) {

            return Value;
          } else {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
          }
        },
        HandleValue->second);
  }
};

/// MangerType just use reference
template <typename HandleType, typename ManagerType>
class RefHandlesManager
    : public detail::BaseHandlesManager<HandleType, ManagerType> {
public:
  /// get return reference
  WasiCryptoExpect<std::reference_wrapper<ManagerType>>
  get(HandleType Handle) noexcept {
    std::shared_lock<std::shared_mutex> Lock{this->Mutex};

    auto HandleValue = this->Map.find(static_cast<uint32_t>(Handle));
    if (HandleValue == this->Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return HandleValue->second;
  }
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge