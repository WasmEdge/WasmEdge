// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Aeads) {
  auto AeadsTest = [this](std::string_view Name,
                          const std::vector<uint8_t> &Nonce, size_t MaxTagSize,
                          const std::vector<uint8_t> &Msg) {
    SCOPED_TRACE(Name);

    WASI_CRYPTO_EXPECT_SUCCESS(KeyHandle,
                               symmetricKeyGenerate(Name, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(OptionsHandle,
                               optionsOpen(__WASI_ALGORITHM_TYPE_SYMMETRIC));
    // Repeatedly set and overwrite the previous option.
    WASI_CRYPTO_EXPECT_TRUE(optionsSet(OptionsHandle, "nonce"sv, "nonce"_u8));
    WASI_CRYPTO_EXPECT_TRUE(optionsSet(OptionsHandle, "nonce"sv, Nonce));
    WASI_CRYPTO_EXPECT_SUCCESS(
        State1Handle, symmetricStateOpen(Name, KeyHandle, OptionsHandle));

    // State nonce equal to the previous set one.
    std::vector<uint8_t> ObservedNonce(Nonce.size());
    symmetricStateOptionsGet(State1Handle, "nonce"sv, ObservedNonce);
    EXPECT_EQ(ObservedNonce, Nonce);
    WASI_CRYPTO_EXPECT_SUCCESS(TagSize, symmetricStateMaxTagLen(State1Handle));
    EXPECT_EQ(TagSize, MaxTagSize);

    std::vector<uint8_t> CiphertextWithTag(Msg.size() + MaxTagSize);

    WASI_CRYPTO_EXPECT_SUCCESS(
        OutTagSize,
        symmetricStateEncrypt(State1Handle, CiphertextWithTag, Msg));
    EXPECT_EQ(OutTagSize, CiphertextWithTag.size());
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(State1Handle));

    {
      WASI_CRYPTO_EXPECT_SUCCESS(
          State2Handle, symmetricStateOpen(Name, KeyHandle, OptionsHandle));
      std::vector<uint8_t> Msg2(Msg.size());
      WASI_CRYPTO_EXPECT_SUCCESS(
          OutputMsg2Size,
          symmetricStateDecrypt(State2Handle, Msg2, CiphertextWithTag));
      EXPECT_EQ(OutputMsg2Size, Msg2.size());
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(State2Handle));
      EXPECT_EQ(Msg2, Msg);
    }

    WASI_CRYPTO_EXPECT_SUCCESS(
        State3Handle, symmetricStateOpen(Name, KeyHandle, OptionsHandle));
    std::vector<uint8_t> Ciphertext(Msg.size());
    WASI_CRYPTO_EXPECT_SUCCESS(TagHandle, symmetricStateEncryptDetached(
                                              State3Handle, Ciphertext, Msg));
    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(State3Handle));
    std::vector<uint8_t> Tag(MaxTagSize);
    WASI_CRYPTO_EXPECT_SUCCESS(OutputTagSize, symmetricTagPull(TagHandle, Tag));
    EXPECT_EQ(OutputTagSize, MaxTagSize);
    EXPECT_EQ(Tag,
              std::vector<uint8_t>(
                  CiphertextWithTag.begin() +
                      static_cast<decltype(CiphertextWithTag)::difference_type>(
                          Msg.size()),
                  CiphertextWithTag.end()));

    WASI_CRYPTO_EXPECT_SUCCESS(
        State4Handle, symmetricStateOpen(Name, KeyHandle, OptionsHandle));
    std::vector<uint8_t> Msg3(Msg.size());
    symmetricStateDecryptDetached(State4Handle, Msg3, Ciphertext, Tag);
    EXPECT_EQ("test"_u8, Msg3);
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(OptionsHandle));

    {
      // Some error cases checking.
      EXPECT_TRUE(
          symmetricStateOpen(Name, InvaildHandle, std::nullopt).error() ==
          __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateOptionsGet(State4Handle, "foo"sv, {}),
          __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      WASI_CRYPTO_EXPECT_FAILURE(
          symmetricStateOptionsGetU64(State4Handle, "foo"sv),
          __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueezeTag(State4Handle),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueezeKey(State4Handle, Name),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateSqueeze(State4Handle, {}),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateRatchet(State4Handle),
                                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    }

    bool IsSymmetricStateCloneImplemented = true;
    // XXX: These cipher didn't implement context duplication from OpenSSL 3.0.0
    // https://github.com/openssl/openssl/issues/20978
    if (0x30000000 <= OPENSSL_VERSION_NUMBER &&
        (Name == "AES-128-GCM"sv || Name == "AES-256-GCM"sv ||
         Name == "CHACHA20-POLY1305"sv)) {
      IsSymmetricStateCloneImplemented = false;
    }

    if (IsSymmetricStateCloneImplemented) {
      // Clone checking.
      WASI_CRYPTO_EXPECT_SUCCESS(NewStateHandle,
                                 symmetricStateClone(State4Handle));
      EXPECT_NE(State4Handle, NewStateHandle);
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(NewStateHandle));
    } else {
      WASI_CRYPTO_EXPECT_FAILURE(symmetricStateClone(State4Handle),
                                 __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }

    WASI_CRYPTO_EXPECT_TRUE(symmetricStateClose(State4Handle));
  };

  AeadsTest("AES-128-GCM"sv, std::vector<uint8_t>(12, 42), 16, "test"_u8);
  AeadsTest("AES-256-GCM"sv, std::vector<uint8_t>(12, 42), 16, "test"_u8);
  AeadsTest("CHACHA20-POLY1305"sv, std::vector<uint8_t>(12, 42), 16, "test"_u8);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
