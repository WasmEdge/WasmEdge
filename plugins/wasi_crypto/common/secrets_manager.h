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

#include <map>
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
    if (Ctx->KeyPairs.erase(Ident) == 0 && Ctx->SymmetricKeys.erase(Ident) == 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_FOUND);
    }
    return {};
  }

  WasiCryptoExpect<void> storeKp(Span<const uint8_t> KeyId,
                                 __wasi_version_t Version,
                                 const AsymmetricCommon::KpVariant &Kp) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    // Since KpVariant (and its alternatives) have deleted assignment operators,
    // we use erase and emplace to update the map.
    Ctx->KeyPairs.erase(Ident);
    Ctx->KeyPairs.emplace(Ident, Kp);
    return {};
  }

  WasiCryptoExpect<void> storeSk(Span<const uint8_t> KeyId,
                                 __wasi_version_t Version,
                                 const Symmetric::KeyVariant &Sk) noexcept {
    std::unique_lock Lock(Ctx->Mutex);
    KeyIdentifier Ident{std::vector<uint8_t>(KeyId.begin(), KeyId.end()),
                        Version};
    Ctx->SymmetricKeys.erase(Ident);
    Ctx->SymmetricKeys.emplace(Ident, Sk);
    return {};
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
      if (Version != Rhs.Version) {
        return Version < Rhs.Version;
      }
      return Id < Rhs.Id;
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
