// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "gtest/gtest.h"

#include "helper.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, NotImplement) {
  EXPECT_EQ(symmetricKeyGenerateManaged(1, "SHA-256"sv, std::nullopt).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(symmetricKeyStoreManaged(1, 1, {}).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(symmetricKeyReplaceManaged(1, 1, 1).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(symmetricKeyId(1, {}).error(), __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(symmetricKeyFromId(1, {}, 1).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);

  EXPECT_EQ(keypairGenerateManaged(1, __WASI_ALGORITHM_TYPE_SIGNATURES, "foo"sv,
                                   std::nullopt)
                .error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(keypairStoreManaged(1, 1, {}).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(keypairReplaceManaged(1, 1, 1).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(keypairId(1, {}).error(), __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  EXPECT_EQ(keypairFromId(1, {}, 1).error(),
            __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge