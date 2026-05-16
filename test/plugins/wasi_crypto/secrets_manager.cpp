// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, SecretsManager) {
  // Test cover for opening and closing.
  // Basic lifecycle.
  {
    // Open without options.
    WASI_CRYPTO_EXPECT_SUCCESS(SecretsManagerHandleDefault,
                               secretsManagerOpen(std::nullopt));
    ASSERT_NE(SecretsManagerHandleDefault, 0u);

    // Reopen already opened manager without closing.
    WASI_CRYPTO_EXPECT_FAILURE(secretsManagerOpen(std::nullopt),
                               __WASI_CRYPTO_ERRNO_PROHIBITED_OPERATION);

    // Open with empty options.
    WASI_CRYPTO_EXPECT_SUCCESS(
        OptOptionalHandleEmptyOptions,
        optionsOpen(static_cast<__wasi_algorithm_type_e_t>(
            SecretsManager::AlgorithmType)));
    WASI_CRYPTO_EXPECT_SUCCESS(
        SecretsManagerHandleEmptyOptions,
        secretsManagerOpen(
            OptOptionalHandleEmptyOptions)); // Gets default manager.
    WASI_CRYPTO_EXPECT_TRUE(
        secretsManagerClose(SecretsManagerHandleEmptyOptions));
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(OptOptionalHandleEmptyOptions));

    // Open with options.
    WASI_CRYPTO_EXPECT_SUCCESS(
        OptOptionalHandle, optionsOpen(static_cast<__wasi_algorithm_type_e_t>(
                               SecretsManager::AlgorithmType)));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(OptOptionalHandle, "password"sv, "very strong password"_u8));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSetU64(OptOptionalHandle, "auto_lock"sv, 5000));

    WASI_CRYPTO_EXPECT_SUCCESS(SecretsManagerHandleWithOptions,
                               secretsManagerOpen(OptOptionalHandle));
    ASSERT_NE(SecretsManagerHandleWithOptions, 0u);
    ASSERT_NE(SecretsManagerHandleDefault, SecretsManagerHandleWithOptions);

    // Reopen already opened manager.
    WASI_CRYPTO_EXPECT_FAILURE(secretsManagerOpen(OptOptionalHandle),
                               __WASI_CRYPTO_ERRNO_PROHIBITED_OPERATION);

    // Cleanup managers.
    WASI_CRYPTO_EXPECT_TRUE(secretsManagerClose(SecretsManagerHandleDefault));
    WASI_CRYPTO_EXPECT_TRUE(
        secretsManagerClose(SecretsManagerHandleWithOptions));

    // Reopen after close.
    WASI_CRYPTO_EXPECT_SUCCESS(
        SecretsManagerHandleDefaultAfterClose,
        secretsManagerOpen(std::nullopt)); // Same manager as HandleDefault.
    ASSERT_NE(SecretsManagerHandleDefaultAfterClose,
              SecretsManagerHandleDefault);

    WASI_CRYPTO_EXPECT_SUCCESS(
        SecretsManagerHandleWithOptionsAfterClose,
        secretsManagerOpen(
            OptOptionalHandle)); // Same manager as HandleWithOptions.
    ASSERT_NE(SecretsManagerHandleWithOptionsAfterClose,
              SecretsManagerHandleWithOptions);

    // Cleanup.
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(OptOptionalHandle));
    WASI_CRYPTO_EXPECT_TRUE(
        secretsManagerClose(SecretsManagerHandleDefaultAfterClose));
    WASI_CRYPTO_EXPECT_TRUE(
        secretsManagerClose(SecretsManagerHandleWithOptionsAfterClose));

    // Double close already closed manager.
    WASI_CRYPTO_EXPECT_FAILURE(
        secretsManagerClose(SecretsManagerHandleWithOptionsAfterClose),
        __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  // Password boundary.
  {
    // Correct password.
    WASI_CRYPTO_EXPECT_SUCCESS(
        OptOptionsHandleVariantPassword,
        optionsOpen(static_cast<__wasi_algorithm_type_e_t>(
            SecretsManager::AlgorithmType)));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(OptOptionsHandleVariantPassword, "password"sv,
                   "very strong password"_u8)); // should be actual strong.
    WASI_CRYPTO_EXPECT_SUCCESS(
        SecretsManagerHandleCorrectPassword,
        secretsManagerOpen(OptOptionsHandleVariantPassword));
    WASI_CRYPTO_EXPECT_TRUE(
        secretsManagerClose(SecretsManagerHandleCorrectPassword));

    // Wrong password.
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(OptOptionsHandleVariantPassword, "password"sv,
                   "strong but wrong password"_u8)); // should be actual strong.
    WASI_CRYPTO_EXPECT_FAILURE(
        secretsManagerOpen(OptOptionsHandleVariantPassword),
        __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(OptOptionsHandleVariantPassword));

    // empty password string.
    WASI_CRYPTO_EXPECT_SUCCESS(
        OptOptionsHandlePasswordEdgeCase,
        optionsOpen(static_cast<__wasi_algorithm_type_e_t>(
            SecretsManager::AlgorithmType)));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(OptOptionsHandlePasswordEdgeCase, "password"sv, ""_u8));
    WASI_CRYPTO_EXPECT_FAILURE(
        secretsManagerOpen(OptOptionsHandlePasswordEdgeCase),
        __WASI_CRYPTO_ERRNO_PROHIBITED_OPERATION);

    // Null bytes in password.
    WASI_CRYPTO_EXPECT_TRUE(optionsSet(OptOptionsHandlePasswordEdgeCase,
                                       "password"sv, "p@$$w0rd!\n\t\x00"_u8));
    WASI_CRYPTO_EXPECT_FAILURE(
        secretsManagerOpen(OptOptionsHandlePasswordEdgeCase),
        __WASI_CRYPTO_ERRNO_PROHIBITED_OPERATION);

    // Very long password.
    std::vector<uint8_t> LongPassword(10'000, 'A');
    WASI_CRYPTO_EXPECT_TRUE(optionsSet(OptOptionsHandlePasswordEdgeCase,
                                       "password"sv, LongPassword));
    WASI_CRYPTO_EXPECT_FAILURE(
        secretsManagerOpen(OptOptionsHandlePasswordEdgeCase),
        __WASI_CRYPTO_ERRNO_OVERFLOW);

    WASI_CRYPTO_EXPECT_TRUE(optionsClose(OptOptionsHandlePasswordEdgeCase));
  }

  // Auto_lock boundary
  {
    // Auto_lock = 0 -> manager never lock.
    WASI_CRYPTO_EXPECT_SUCCESS(
        OptOptionsHandleAutoLockDisabled,
        optionsOpen(static_cast<__wasi_algorithm_type_e_t>(
            SecretsManager::AlgorithmType)));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSetU64(OptOptionsHandleAutoLockDisabled, "auto_lock"sv, 0));
    WASI_CRYPTO_EXPECT_SUCCESS(
        SecretsManagerHandleAutoLockDisabled,
        secretsManagerOpen(OptOptionsHandleAutoLockDisabled));
    WASI_CRYPTO_EXPECT_TRUE(
        secretsManagerClose(SecretsManagerHandleAutoLockDisabled));
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(OptOptionsHandleAutoLockDisabled));

    // TODO: add test cover access after lock.
  }

  // Test cover for invalidate.
  // TODO: add tests cover for invalidate.
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
