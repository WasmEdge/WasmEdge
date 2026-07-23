// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once
#include "common/options.h"
#include "common/span.h"
#include "utils/error.h"
#include "wasi_crypto/api.hpp"
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

constexpr __wasi_version_t VERSION_UNSPECIFIED = 0xff00000000000000ULL;
constexpr __wasi_version_t VERSION_LATEST = 0xff00000000000001ULL;
constexpr __wasi_version_t VERSION_ALL = 0xff00000000000002ULL;

class SecretsManager {
public:
  explicit SecretsManager(std::optional<Options> OptOptions) noexcept
      : ConfigOptions(std::move(OptOptions)) {}

  WasiCryptoExpect<void> invalidate(Span<const uint8_t> KeyId,
                                    __wasi_version_t Version) noexcept {
    std::unique_lock<std::shared_mutex> Lock(Mutex);
    std::string KeyIdStr(KeyId.begin(), KeyId.end());
    InvalidatedKeys[KeyIdStr].insert(Version);
    return {};
  }

  bool isInvalidated(Span<const uint8_t> KeyId,
                     __wasi_version_t Version) const noexcept {
    std::shared_lock<std::shared_mutex> Lock(Mutex);
    std::string KeyIdStr(KeyId.begin(), KeyId.end());

    auto It = InvalidatedKeys.find(KeyIdStr);
    if (It != InvalidatedKeys.end()) {
      const auto &versions = It->second;
      if (versions.count(VERSION_ALL) > 0 || versions.count(Version) > 0) {
        return true;
      }
      if (versions.count(VERSION_LATEST) > 0 &&
          Version != VERSION_UNSPECIFIED) {
        return true;
      }
    }
    return false;
  }

private:
  std::optional<Options> ConfigOptions;
  mutable std::shared_mutex Mutex;
  std::unordered_map<std::string, std::unordered_set<__wasi_version_t>>
      InvalidatedKeys;
};

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge