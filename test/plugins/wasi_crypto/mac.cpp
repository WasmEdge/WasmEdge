// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Mac) {
  auto MacTest = [this](std::string_view Name,
                        const std::vector<uint8_t> &ImportKey,
                        const std::vector<uint8_t> &AbsorbData1,
                        const std::vector<uint8_t> &AbsorbData2,
                        const std::vector<uint8_t> &ExpectedTag1,
                        const std::vector<uint8_t> &ExpectedTag2) {
    SCOPED_TRACE(Name);
    // Generate key hmac.
    {
      WASI_CRYPTO_EXPECT_SUCCESS(KeyHandle,
                                 symmetricKeyGenerate(Name, std::nullopt));

      // Key size checking.
      WASI_CRYPTO_EXPECT_SUCCESS(KeyOutputHandle,
                                 symmetricKeyExport(KeyHandle));
      WASI_CRYPTO_EXPECT_SUCCESS(KeySize, arrayOutputLen(KeyOutputHandle));
      EXPECT_EQ(KeySize, ImportKey.size());

      WASI_CRYPTO_EXPECT_SUCCESS(
          StateHandle, symmetricStateOpen(Name, KeyHandle, std::nullopt));
      WASI_CRYPTO_EXPECT_TRUE(symmetricKeyClose(KeyHandle));

      // Equivalent to a single call.
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(StateHandle, AbsorbData1));
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(StateHandle, AbsorbData2));

      WASI_CRYPTO_EXPECT_SUCCESS(TagHandle,
                                 symmetricStateSqueezeTag(StateHandle));
      std::vector<uint8_t> Tag(ExpectedTag1.size());

      WASI_CRYPTO_EXPECT_SUCCESS(TagPullSize, symmetricTagPull(TagHandle, Tag));
      EXPECT_EQ(TagPullSize, ExpectedTag1.size());
      WASI_CRYPTO_EXPECT_TRUE(symmetricTagClose(TagHandle));

      WASI_CRYPTO_EXPECT_SUCCESS(NewTagHandle,
                                 symmetricStateSqueezeTag(StateHandle));

      WASI_CRYPTO_EXPECT_TRUE(symmetricTagVerify(NewTagHandle, Tag));
      WASI_CRYPTO_EXPECT_TRUE(symmetricTagClose(NewTagHandle));

      // Error case checking.
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateOpen(Name, std::nullopt, std::nullopt),
          __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueeze(StateHandle, {}),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueezeKey(StateHandle, Name),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateMaxTagLen(StateHandle),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateEncrypt(StateHandle, {}, {}),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateEncryptDetached(StateHandle, {}, {}),
          __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateDecrypt(StateHandle, {}, {}),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateDecryptDetached(StateHandle, {}, {}, {}),
          __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateRatchet(StateHandle),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);

      // Clone checking.
      WASI_CRYPTO_EXPECT_SUCCESS(NewStateHandle,
                                 symmetricStateClone(StateHandle));
      EXPECT_NE(StateHandle, NewStateHandle);
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(NewStateHandle));

      WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(StateHandle));
    }

    // Import key hmac.
    {
      WASI_CRYPTO_EXPECT_SUCCESS(KeyHandle,
                                 symmetricKeyImport(Name, ImportKey));
      WASI_CRYPTO_EXPECT_SUCCESS(
          StateHandle, symmetricStateOpen(Name, KeyHandle, std::nullopt));
      WASI_CRYPTO_EXPECT_TRUE(symmetricKeyClose(KeyHandle));
      {
        // Absorb "data".
        WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(StateHandle, AbsorbData1));

        // SqueezeTag "data".
        WASI_CRYPTO_EXPECT_SUCCESS(TagHandle,
                                   symmetricStateSqueezeTag(StateHandle));
        std::vector<uint8_t> Tag(ExpectedTag1.size());
        WASI_CRYPTO_EXPECT_SUCCESS(TagPullSize,
                                   symmetricTagPull(TagHandle, Tag));
        EXPECT_EQ(TagPullSize, Tag.size());
        EXPECT_EQ(Tag, ExpectedTag1);
      }

      {
        // Abosorb "more_data".
        WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(StateHandle, AbsorbData2));

        // SqueezeTag "datamore_data".
        WASI_CRYPTO_EXPECT_SUCCESS(TagHandle,
                                   symmetricStateSqueezeTag(StateHandle));
        std::vector<uint8_t> Tag(ExpectedTag2.size());
        WASI_CRYPTO_EXPECT_SUCCESS(TagPullSize,
                                   symmetricTagPull(TagHandle, Tag));
        EXPECT_EQ(TagPullSize, Tag.size());
        EXPECT_EQ(Tag, ExpectedTag2);
      }
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(StateHandle));
    }
  };
  MacTest(
      "HMAC/SHA-256"sv, "00000000000000000000000000000000"_u8, "data"_u8,
      "more_data"_u8,
      "7f12a3d914ec4d1ee67dd35ff04df5a725d11a6bb78a4aafd1093f5bfbd86887"_u8v,
      "77af4875ffb3932cba0c8bc5da18410c42c85eeb07072918629675e054fbc42d"_u8v);
  MacTest(
      "HMAC/SHA-512"sv,
      "0000000000000000000000000000000000000000000000000000000000000000"_u8,
      "data"_u8, "more_data"_u8,
      "52fbafda16189e63730604e49c747c8281d2420e7aae34c927927e7c3cddfcea62fea554d1962a0c0d1c8177884787d8b2a88bd396d5780e3fb82b11ab33c5cc"_u8v,
      "36d2dbfb50768b963fe243535bcda302750297b361b7eb079978b27177adc40338dab5c244ae90e2f11a3518ac31126a52eb5ec715c0a9476b98f73e7ff7682e"_u8v);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
