// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, KxDh) {

  auto KxDhTest = [this](std::string_view Alg, const std::vector<uint8_t> &Pk1,
                         const std::vector<uint8_t> &Sk1,
                         const std::vector<uint8_t> &Pk2,
                         const std::vector<uint8_t> &Sk2,
                         const std::vector<uint8_t> &SharedSecret) {
    SCOPED_TRACE(Alg);
    WASI_CRYPTO_EXPECT_SUCCESS(
        Pk1Handle, publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Pk1,
                                   __WASI_PUBLICKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Sk1Handle, secretkeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Sk1,
                                   __WASI_SECRETKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Pk2Handle, publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Pk2,
                                   __WASI_PUBLICKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Sk2Handle, secretkeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Sk2,
                                   __WASI_SECRETKEY_ENCODING_RAW));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Handle, kxDh(Pk1Handle, Sk2Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Size,
                               arrayOutputLen(SharedKey1Handle));
    EXPECT_EQ(SharedKey1Size, 32);
    std::vector<uint8_t> SharedKey1(32);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1PullSize,
                               arrayOutputPull(SharedKey1Handle, SharedKey1));
    EXPECT_EQ(SharedKey1PullSize, 32);
    EXPECT_EQ(SharedKey1, SharedSecret);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Handle, kxDh(Pk2Handle, Sk1Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Size,
                               arrayOutputLen(SharedKey2Handle));
    EXPECT_EQ(SharedKey2Size, 32);
    std::vector<uint8_t> SharedKey2(32);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SharedKey2Handle, SharedKey2));
    EXPECT_EQ(SharedKey2, SharedSecret);

    /// It's only supported in OpenSSL 3.0.
    /// See: https://github.com/openssl/openssl/issues/7616
    WASI_CRYPTO_EXPECT_FAILURE(kxEncapsulate(Pk1Handle),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_FAILURE(kxDecapsulate(Sk1Handle, {}),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk1Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk2Handle));

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk2Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk1Handle));
  };

  // From: https://datatracker.ietf.org/doc/html/rfc7748#section-6.1
  KxDhTest(
      "X25519"sv,
      "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"_u8v,
      "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"_u8v,
      "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f"_u8v,
      "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb"_u8v,
      "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742"_u8v);

  auto NewKxDhTest = [this](std::string_view Alg) {
    SCOPED_TRACE(Alg);

    WASI_CRYPTO_EXPECT_SUCCESS(
        Kp1Handle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Kp2Handle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));

    WASI_CRYPTO_EXPECT_SUCCESS(Pk1Handle, keypairPublickey(Kp1Handle));
    WASI_CRYPTO_EXPECT_SUCCESS(Sk1Handle, keypairSecretkey(Kp1Handle));
    WASI_CRYPTO_EXPECT_SUCCESS(Pk2Handle, keypairPublickey(Kp2Handle));
    WASI_CRYPTO_EXPECT_SUCCESS(Sk2Handle, keypairSecretkey(Kp2Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Handle, kxDh(Pk1Handle, Sk2Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Size,
                               arrayOutputLen(SharedKey1Handle));
    EXPECT_EQ(SharedKey1Size, 32);
    std::vector<uint8_t> SharedKey1(32);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1PullSize,
                               arrayOutputPull(SharedKey1Handle, SharedKey1));
    EXPECT_EQ(SharedKey1PullSize, 32);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Handle, kxDh(Pk2Handle, Sk1Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Size,
                               arrayOutputLen(SharedKey2Handle));
    EXPECT_EQ(SharedKey2Size, 32);
    std::vector<uint8_t> SharedKey2(32);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SharedKey2Handle, SharedKey2));

    EXPECT_EQ(SharedKey1, SharedKey2);

    /// It's only supported in OpenSSL 3.0.
    /// See: https://github.com/openssl/openssl/issues/7616
    WASI_CRYPTO_EXPECT_FAILURE(kxEncapsulate(Pk1Handle),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_FAILURE(kxDecapsulate(Sk1Handle, {}),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk1Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk2Handle));

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk2Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk1Handle));
  };
  NewKxDhTest("P256-SHA256"sv);
  NewKxDhTest("P384-SHA384"sv);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
