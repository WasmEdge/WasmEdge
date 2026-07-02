// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

class SecretsManager {
public:
  SecretsManager() : Ctx(std::make_shared<Inner>()) {}

  WasiCryptoExpect<void> invalidate(Span<const uint8_t> KeyId,
                                    __wasi_version_t Version) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    if (Ctx->KeyPairs.erase(Ident) == 0 &&
        Ctx->SymmetricKeys.erase(Ident) == 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return {};
  }

  WasiCryptoExpect<__wasi_version_t>
  storeKp(Span<const uint8_t> KeyId, __wasi_version_t Version,
          const AsymmetricCommon::KpVariant &Kp) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    // Since KpVariant (and its alternatives) have deleted assignment operators,
    // we use erase and emplace to update the map.
    Ctx->KeyPairs.erase(Ident);
    Ctx->KeyPairs.emplace(Ident, Kp);
    return Version;
  }

  WasiCryptoExpect<__wasi_version_t>
  storeSk(Span<const uint8_t> KeyId, __wasi_version_t Version,
          const Symmetric::KeyVariant &Sk) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    Ctx->SymmetricKeys.erase(Ident);
    Ctx->SymmetricKeys.emplace(Ident, Sk);
    return Version;
  }

  WasiCryptoExpect<__wasi_version_t>
  getLatestKpVersion(Span<const uint8_t> KeyId) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    __wasi_version_t LatestVersion = 0;
    bool Found = false;
    for (auto It = Ctx->KeyPairs.lower_bound({Id, 0});
         It != Ctx->KeyPairs.end() && It->first.Id == Id; ++It) {
      LatestVersion = std::max(LatestVersion, It->first.Version);
      Found = true;
    }
    if (!Found) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return LatestVersion;
  }

  WasiCryptoExpect<__wasi_version_t>
  getLatestSkVersion(Span<const uint8_t> KeyId) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    std::vector<uint8_t> Id(KeyId.begin(), KeyId.end());
    __wasi_version_t LatestVersion = 0;
    bool Found = false;
    for (auto It = Ctx->SymmetricKeys.lower_bound({Id, 0});
         It != Ctx->SymmetricKeys.end() && It->first.Id == Id; ++It) {
      LatestVersion = std::max(LatestVersion, It->first.Version);
      Found = true;
    }
    if (!Found) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return LatestVersion;
  }

  WasiCryptoExpect<AsymmetricCommon::KpVariant>
  getKp(Span<const uint8_t> KeyId, __wasi_version_t Version) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    auto It = Ctx->KeyPairs.find(Ident);
    if (It == Ctx->KeyPairs.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return It->second;
  }

  WasiCryptoExpect<Symmetric::KeyVariant>
  getSk(Span<const uint8_t> KeyId, __wasi_version_t Version) noexcept {
    std::shared_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    auto It = Ctx->SymmetricKeys.find(Ident);
    if (It == Ctx->SymmetricKeys.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return It->second;
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
    std::shared_mutex Mutex;
  };
  std::shared_ptr<Inner> Ctx;
};

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
