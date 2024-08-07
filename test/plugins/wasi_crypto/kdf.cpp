// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Kdf) {
  auto KdfTest = [this](std::string_view ExtractAlg, std::string_view ExpandAlg,
                        const std::vector<uint8_t> &Key,
                        const std::vector<uint8_t> &Salt,
                        const std::vector<uint8_t> &Info, size_t KeySize) {
    WASI_CRYPTO_EXPECT_SUCCESS(KeyHandle, symmetricKeyImport(ExtractAlg, Key));
    WASI_CRYPTO_EXPECT_SUCCESS(
        ExtractStateHandle,
        symmetricStateOpen(ExtractAlg, KeyHandle, std::nullopt));
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(ExtractStateHandle, Salt));
    WASI_CRYPTO_EXPECT_SUCCESS(
        PrkHandle, symmetricStateSqueezeKey(ExtractStateHandle, ExpandAlg));
    WASI_CRYPTO_EXPECT_TRUE(symmetricKeyClose(KeyHandle));
    WASI_CRYPTO_EXPECT_SUCCESS(
        ExpandStateHandle,
        symmetricStateOpen(ExpandAlg, PrkHandle, std::nullopt));
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(ExpandStateHandle, Info));
    std::vector<uint8_t> SqueezeKey(KeySize);
    WASI_CRYPTO_EXPECT_TRUE(
        symmetricStateSqueeze(ExpandStateHandle, SqueezeKey));

    auto BothInvalid = [this](std::string_view Name,
                              __wasi_symmetric_state_t StateHandle) {
      EXPECT_TRUE(
          symmetricStateOpen(Name, InvaildHandle, std::nullopt).error() ==
          __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateOptionsGet(StateHandle, "foo"sv, {}),
          __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateOptionsGetU64(StateHandle, "foo"sv),
          __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueezeTag(StateHandle),
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
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateClone(StateHandle),
                                 __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    };
    BothInvalid(ExpandAlg, ExtractStateHandle);
    BothInvalid(ExtractAlg, ExpandStateHandle);
    WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueeze(ExtractStateHandle, {}),
                               __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    WASI_CRYPTO_EXPECT_FAILURE(
        symmetricStateSqueezeKey(ExpandStateHandle, ExpandAlg),
        __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(ExtractStateHandle));
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(ExpandStateHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(NewKeyHandle,
                               symmetricKeyGenerate(ExtractAlg, std::nullopt));
    WASI_CRYPTO_EXPECT_TRUE(symmetricKeyClose(NewKeyHandle));
    WASI_CRYPTO_EXPECT_FAILURE(symmetricKeyGenerate(ExpandAlg, std::nullopt),
                               __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
  };
  KdfTest("HKDF-EXTRACT/SHA-256"sv, "HKDF-EXPAND/SHA-256"sv, "IKM"_u8,
          "salt"_u8, "info"_u8, 32);
  KdfTest("HKDF-EXTRACT/SHA-512"sv, "HKDF-EXPAND/SHA-512"sv, "IKM"_u8,
          "salt"_u8, "info"_u8, 64);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
