// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/utils/handles_manager.h --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the class definitions of the WasiCrypto HandlesManager.
/// It controls the handle and the inner states.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace detail {

/// The Handles Manager base class.
///
/// @tparam HandleType This is the type of handle, notice it must be `32-bit
/// long`.
/// @tparam ManagerType The managed content type.
///
/// HandlesManager uses handle as index to represent the managed contents.
///
/// Referenced from:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/implementations/hostcalls/rust/src/handles.rs
template <typename HandleType, typename ManagerType> class BaseHandlesManager {
public:
  BaseHandlesManager(const BaseHandlesManager &) noexcept = delete;
  BaseHandlesManager &operator=(const BaseHandlesManager &) noexcept = delete;
  BaseHandlesManager(BaseHandlesManager &&) noexcept = delete;
  BaseHandlesManager &operator=(BaseHandlesManager &&) noexcept = delete;

  /// @param TypeID A unique number
  explicit BaseHandlesManager(uint8_t TypeID) noexcept
      : LastHandle{TypeID, 0} {}

  WasiCryptoExpect<void> close(HandleType Handle) noexcept {
    std::unique_lock<std::shared_mutex> Lock{Mutex};

    if (!Map.erase(HandleWrapper(Handle)))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_CLOSED);

    return {};
  }

  /// Constructor a new manager.
  template <typename... Args>
  WasiCryptoExpect<HandleType> registerManager(Args &&...Manager) noexcept {
    std::unique_lock<std::shared_mutex> Lock{Mutex};

    // Find a handle that can be used and emplace.
    // Assume that LastHandle is 0x01000000, NextHandle is 0x01000001.
    auto NextHandle = LastHandle.nextHandle();
    while (true) {
      // Try to emplace NextHandle.
      if (Map.try_emplace(NextHandle, std::forward<Args>(Manager)...).second) {
        // If success, emplace which indicate the NextHandle not exists in the
        // managed content. Update the last handle and return it.
        LastHandle = NextHandle;
        return LastHandle.Handle;
      }
      // Otherwise, the NextHandle Map already exists a content, call NextHandle
      // and loop.
      NextHandle = NextHandle.nextHandle();

      // If after looping `many times(2^24 - 1)`, we get 0x01000000 again.
      if (NextHandle == LastHandle) {
        // It indicates the hashmap is full.
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_TOO_MANY_HANDLES);
      }
    }
  }

protected:
  /// The handle internal representation as [-TypeID-|------CurrentNumber------]
  union HandleWrapper {
    static_assert(sizeof(HandleType) == 4, "HandleType must be 4 byte");
    HandleWrapper(uint8_t TypeID, uint32_t CurrentNumber) noexcept
        : TypeID(TypeID), CurrentNumber(CurrentNumber) {}
    explicit HandleWrapper(HandleType Handle) : Handle(Handle) {}

    HandleWrapper nextHandle() noexcept {
      return {TypeID, static_cast<uint32_t>(CurrentNumber + 1)};
    }

    struct Hash {
      size_t operator()(const HandleWrapper &Wrapper) const noexcept {
        return static_cast<size_t>(Wrapper.Handle);
      }
    };

    bool operator==(const HandleWrapper &Wrapper) const noexcept {
      return Wrapper.Handle == this->Handle;
    }

    struct {
      uint8_t TypeID : 8;
      uint32_t CurrentNumber : 24;
    };
    HandleType Handle;
  };

  std::shared_mutex Mutex;
  HandleWrapper LastHandle;
  std::unordered_map<HandleWrapper, ManagerType, typename HandleWrapper::Hash>
      Map;
};

template <typename T, typename VariantType> struct IsVariantMember;
template <typename T, typename... AllType>
struct IsVariantMember<T, std::variant<AllType...>>
    : public std::disjunction<std::is_same<T, AllType>...> {};

} // namespace detail

/// ManagerType need reference count.
template <typename HandleType, typename ManagerType,
          std::enable_if_t<std::is_copy_constructible_v<ManagerType>, bool> =
              false>
class RcHandlesManager
    : public detail::BaseHandlesManager<HandleType, ManagerType> {
  using HandleWrapper =
      typename detail::BaseHandlesManager<HandleType,
                                          ManagerType>::HandleWrapper;

public:
  using detail::BaseHandlesManager<HandleType, ManagerType>::BaseHandlesManager;

  /// Get the return copy.
  WasiCryptoExpect<ManagerType> get(HandleType Handle) noexcept {
    std::shared_lock<std::shared_mutex> Lock{this->Mutex};

    auto HandleValue = this->Map.find(HandleWrapper(Handle));
    if (HandleValue == this->Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return HandleValue->second;
  }

  /// Get as different variant type.
  template <typename RequiredVariantType>
  WasiCryptoExpect<RequiredVariantType> getAs(HandleType Handle) noexcept {
    std::shared_lock<std::shared_mutex> Lock{this->Mutex};

    auto HandleValue = this->Map.find(HandleWrapper(Handle));
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

/// ManagerType just use reference.
template <typename HandleType, typename ManagerType>
class RefHandlesManager
    : public detail::BaseHandlesManager<HandleType, ManagerType> {
  using HandleWrapper =
      typename detail::BaseHandlesManager<HandleType,
                                          ManagerType>::HandleWrapper;

public:
  using detail::BaseHandlesManager<HandleType, ManagerType>::BaseHandlesManager;

  /// Get the return reference.
  WasiCryptoExpect<std::reference_wrapper<ManagerType>>
  get(HandleType Handle) noexcept {
    std::shared_lock<std::shared_mutex> Lock{this->Mutex};

    auto HandleValue = this->Map.find(HandleWrapper(Handle));
    if (HandleValue == this->Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return HandleValue->second;
  }
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
