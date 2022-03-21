// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "helper.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/symmetric/func.h"
#include <optional>

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
      EXPECT_EQ(symmetricStateOptionsGet(StateHandle, "foo"sv, {}).error(),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      EXPECT_EQ(symmetricStateOptionsGetU64(StateHandle, "foo"sv).error(),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      EXPECT_EQ(symmetricStateSqueezeTag(StateHandle).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateMaxTagLen(StateHandle).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateEncrypt(StateHandle, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateEncryptDetached(StateHandle, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateDecrypt(StateHandle, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateDecryptDetached(StateHandle, {}, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateRatchet(StateHandle).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    };
    BothInvalid(ExpandAlg, ExtractStateHandle);
    BothInvalid(ExtractAlg, ExpandStateHandle);
    EXPECT_EQ(symmetricStateSqueeze(ExtractStateHandle, {}).error(),
              __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    EXPECT_EQ(symmetricStateSqueezeKey(ExpandStateHandle, ExpandAlg).error(),
              __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(ExtractStateHandle));
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(ExpandStateHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(NewKeyHandle,
                               symmetricKeyGenerate(ExtractAlg, std::nullopt));
    WASI_CRYPTO_EXPECT_TRUE(symmetricKeyClose(NewKeyHandle));
    EXPECT_EQ(symmetricKeyGenerate(ExpandAlg, std::nullopt).error(),
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