// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/plugins/wasi_crypto/common/secrets_manager.h -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the Secrets Manager class definition.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "asymmetric_common/keypair.h"
#include "symmetric/key.h"
#include "utils/error.h"
#include "wasi_crypto/api.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

constexpr __wasi_version_t VERSION_UNSPECIFIED = 0xff00000000000000ULL;
constexpr __wasi_version_t VERSION_LATEST = 0xff00000000000001ULL;
constexpr __wasi_version_t VERSION_LATEST_ALT = VERSION_LATEST;
constexpr __wasi_version_t VERSION_ALL = 0xff00000000000002ULL;
constexpr __wasi_version_t VERSION_ALL_ALT = VERSION_ALL;

class SecretsManager {
public:
  SecretsManager() {
    static auto SharedStore = std::make_shared<Inner>();
    Ctx = SharedStore;
  }

  WasiCryptoExpect<void> invalidate(Span<const uint8_t> KeyId,
                                    __wasi_version_t Version) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());

    if (Version == VERSION_ALL || Version == VERSION_ALL_ALT) {
      size_t Erased = 0;
      for (auto It = Ctx->KeyPairs.lower_bound({Id, 0});
           It != Ctx->KeyPairs.end() && It->first.Id == Id;) {
        It = Ctx->KeyPairs.erase(It);
        Erased++;
      }
      for (auto It = Ctx->SymmetricKeys.lower_bound({Id, 0});
           It != Ctx->SymmetricKeys.end() && It->first.Id == Id;) {
        It = Ctx->SymmetricKeys.erase(It);
        Erased++;
      }
      if (Erased == 0) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
      }
      return {};
    }

    if (Version == VERSION_LATEST || Version == VERSION_LATEST_ALT) {
      auto KpVer = getLatestKpVersionImpl(Id);
      auto SkVer = getLatestSkVersionImpl(Id);
      if (!KpVer && !SkVer) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
      }
      if (KpVer) {
        Ctx->KeyPairs.erase({Id, *KpVer});
      }
      if (SkVer) {
        Ctx->SymmetricKeys.erase({Id, *SkVer});
      }
      return {};
    }

    KeyIdentifier Ident{Id, Version};
    const auto KpErased = Ctx->KeyPairs.erase(Ident);
    const auto SkErased = Ctx->SymmetricKeys.erase(Ident);
    if (KpErased == 0 && SkErased == 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return {};
  }

  WasiCryptoExpect<__wasi_version_t>
  storeKp(Span<const uint8_t> KeyId, __wasi_version_t Version,
          AsymmetricCommon::KpVariant Kp) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    KeyIdentifier Ident{Id, Version};
    Ctx->KeyPairHighWaterMarks[Id] =
        std::max(Ctx->KeyPairHighWaterMarks[Id], Version);
    Ctx->KeyPairs.erase(Ident);
    Ctx->KeyPairs.emplace(Ident, std::move(Kp));
    return Version;
  }

  WasiCryptoExpect<__wasi_version_t>
  storeSk(Span<const uint8_t> KeyId, __wasi_version_t Version,
          Symmetric::KeyVariant Sk) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    KeyIdentifier Ident{Id, Version};
    Ctx->SymmetricKeyHighWaterMarks[Id] =
        std::max(Ctx->SymmetricKeyHighWaterMarks[Id], Version);
    Ctx->SymmetricKeys.erase(Ident);
    Ctx->SymmetricKeys.emplace(Ident, std::move(Sk));
    return Version;
  }

  WasiCryptoExpect<__wasi_version_t>
  replaceKp(Span<const uint8_t> KeyId,
            AsymmetricCommon::KpVariant Kp) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    auto HwmIt = Ctx->KeyPairHighWaterMarks.find(Id);
    if (HwmIt == Ctx->KeyPairHighWaterMarks.end()) {
      auto Latest = getLatestKpVersionImpl(Id);
      if (!Latest) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
      }
      Ctx->KeyPairHighWaterMarks[Id] = *Latest;
      HwmIt = Ctx->KeyPairHighWaterMarks.find(Id);
    }

    __wasi_version_t NextVersion = 0;
    ensureOrReturn(!__builtin_add_overflow(HwmIt->second,
                                           static_cast<__wasi_version_t>(1),
                                           &NextVersion),
                   __WASI_CRYPTO_ERRNO_OVERFLOW);
    HwmIt->second = NextVersion;
    KeyIdentifier Ident{Id, NextVersion};
    Ctx->KeyPairs.erase(Ident);
    Ctx->KeyPairs.emplace(Ident, std::move(Kp));
    return NextVersion;
  }

  WasiCryptoExpect<__wasi_version_t>
  replaceSk(Span<const uint8_t> KeyId, Symmetric::KeyVariant Sk) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    auto HwmIt = Ctx->SymmetricKeyHighWaterMarks.find(Id);
    if (HwmIt == Ctx->SymmetricKeyHighWaterMarks.end()) {
      auto Latest = getLatestSkVersionImpl(Id);
      if (!Latest) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
      }
      Ctx->SymmetricKeyHighWaterMarks[Id] = *Latest;
      HwmIt = Ctx->SymmetricKeyHighWaterMarks.find(Id);
    }

    __wasi_version_t NextVersion = 0;
    ensureOrReturn(!__builtin_add_overflow(HwmIt->second,
                                           static_cast<__wasi_version_t>(1),
                                           &NextVersion),
                   __WASI_CRYPTO_ERRNO_OVERFLOW);
    HwmIt->second = NextVersion;
    KeyIdentifier Ident{Id, NextVersion};
    Ctx->SymmetricKeys.erase(Ident);
    Ctx->SymmetricKeys.emplace(Ident, std::move(Sk));
    return NextVersion;
  }

  WasiCryptoExpect<__wasi_version_t>
  getLatestKpVersion(Span<const uint8_t> KeyId) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    auto Res = getLatestKpVersionImpl(Id);
    if (!Res) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return *Res;
  }

  WasiCryptoExpect<__wasi_version_t>
  getLatestSkVersion(Span<const uint8_t> KeyId) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    auto Res = getLatestSkVersionImpl(Id);
    if (!Res) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return *Res;
  }

  WasiCryptoExpect<
      std::pair<AsymmetricCommon::KpVariant, __wasi_version_t>>
  getKpWithVersion(Span<const uint8_t> KeyId,
                   __wasi_version_t Version) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    if (Version == VERSION_LATEST || Version == VERSION_LATEST_ALT) {
      auto Latest = getLatestKpVersionImpl(Id);
      if (!Latest) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
      }
      Version = *Latest;
    }
    KeyIdentifier Ident{Id, Version};
    auto It = Ctx->KeyPairs.find(Ident);
    if (It == Ctx->KeyPairs.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return std::make_pair(It->second, Version);
  }

  WasiCryptoExpect<AsymmetricCommon::KpVariant>
  getKp(Span<const uint8_t> KeyId, __wasi_version_t Version) noexcept {
    return getKpWithVersion(KeyId, Version).map([](auto &&Pair) {
      return Pair.first;
    });
  }

  WasiCryptoExpect<std::pair<Symmetric::KeyVariant, __wasi_version_t>>
  getSkWithVersion(Span<const uint8_t> KeyId,
                   __wasi_version_t Version) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    if (Version == VERSION_LATEST || Version == VERSION_LATEST_ALT) {
      auto Latest = getLatestSkVersionImpl(Id);
      if (!Latest) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
      }
      Version = *Latest;
    }
    KeyIdentifier Ident{Id, Version};
    auto It = Ctx->SymmetricKeys.find(Ident);
    if (It == Ctx->SymmetricKeys.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return std::make_pair(It->second, Version);
  }

  WasiCryptoExpect<Symmetric::KeyVariant>
  getSk(Span<const uint8_t> KeyId, __wasi_version_t Version) noexcept {
    return getSkWithVersion(KeyId, Version).map([](auto &&Pair) {
      return Pair.first;
    });
  }

private:
  struct KeyIdentifier {
    std::vector<uint8_t> Id;
    __wasi_version_t Version;
    bool operator<(const KeyIdentifier &Rhs) const {
      if (Id != Rhs.Id) {
        return Id < Rhs.Id;
      }
      return Version < Rhs.Version;
    }
  };

  struct Inner {
    std::map<KeyIdentifier, AsymmetricCommon::KpVariant> KeyPairs;
    std::map<KeyIdentifier, Symmetric::KeyVariant> SymmetricKeys;
    std::map<std::vector<uint8_t>, __wasi_version_t> KeyPairHighWaterMarks;
    std::map<std::vector<uint8_t>, __wasi_version_t> SymmetricKeyHighWaterMarks;
    std::shared_mutex Mutex;
  };

  std::optional<__wasi_version_t>
  getLatestKpVersionImpl(const std::vector<uint8_t> &Id) const noexcept {
    __wasi_version_t LatestVersion = 0;
    bool Found = false;
    for (auto It = Ctx->KeyPairs.lower_bound({Id, 0});
         It != Ctx->KeyPairs.end() && It->first.Id == Id; ++It) {
      LatestVersion = std::max(LatestVersion, It->first.Version);
      Found = true;
    }
    if (!Found) {
      return std::nullopt;
    }
    return LatestVersion;
  }

  std::optional<__wasi_version_t>
  getLatestSkVersionImpl(const std::vector<uint8_t> &Id) const noexcept {
    __wasi_version_t LatestVersion = 0;
    bool Found = false;
    for (auto It = Ctx->SymmetricKeys.lower_bound({Id, 0});
         It != Ctx->SymmetricKeys.end() && It->first.Id == Id; ++It) {
      LatestVersion = std::max(LatestVersion, It->first.Version);
      Found = true;
    }
    if (!Found) {
      return std::nullopt;
    }
    return LatestVersion;
  }

  std::shared_ptr<Inner> Ctx;
};

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
