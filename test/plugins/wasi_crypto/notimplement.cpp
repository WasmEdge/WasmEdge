// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, NotImplement) {
  WASI_CRYPTO_EXPECT_FAILURE(
      symmetricKeyGenerateManaged(1, "SHA-256"sv, std::nullopt),
      __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(symmetricKeyStoreManaged(1, 1, {}),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(symmetricKeyReplaceManaged(1, 1, 1),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(symmetricKeyId(1, {}),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(symmetricKeyFromId(1, {}, 1),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);

  EXPECT_EQ(keypairGenerateManaged(1, __WASI_ALGORITHM_TYPE_SIGNATURES,
                                   "Ed25519"sv, std::nullopt)
                .error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(keypairStoreManaged(1, 1, {}),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(keypairReplaceManaged(1, 1, 1),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(keypairId(1, {}),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(keypairFromId(1, {}, 1),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);

  WASI_CRYPTO_EXPECT_FAILURE(secretsManagerOpen(std::nullopt),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(secretsManagerClose(InvaildHandle),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  WASI_CRYPTO_EXPECT_FAILURE(secretsManagerInvalidate(InvaildHandle, {}, 0),
                             __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
