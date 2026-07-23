// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/secrets_manager.h"
#include <gtest/gtest.h>
#include <vector>

using namespace WasmEdge::Host::WasiCrypto;

TEST(SecretsManagerTest, InvalidateAndCheck) {
  // Instantiate the SecretsManager directly, bypassing the Context singleton
  Common::SecretsManager manager(std::nullopt);

  std::vector<uint8_t> KeyId = {'k', 'e', 'y', '_', '1'};
  __wasi_version_t Version = 1;

  // Test invalidation
  auto InvRes = manager.invalidate(KeyId, Version);
  ASSERT_TRUE(InvRes);

  // Verify the internal state reflects the invalidation
  ASSERT_TRUE(manager.isInvalidated(KeyId, Version));
  
  // Verify it doesn't incorrectly invalidate other versions or keys
  ASSERT_FALSE(manager.isInvalidated(KeyId, 2)); 
  std::vector<uint8_t> OtherKey = {'o', 't', 'h', 'e', 'r'};
  ASSERT_FALSE(manager.isInvalidated(OtherKey, Version)); 
}