// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Signatures) {
  // Use the generated data to sign and verify.
  auto SigTest = [this](__wasi_algorithm_type_e_t AlgType,
                        std::string_view Alg) {
    SCOPED_TRACE(Alg);
    WASI_CRYPTO_EXPECT_SUCCESS(KpHandle,
                               keypairGenerate(AlgType, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(StateHandle, signatureStateOpen(KpHandle));
    WASI_CRYPTO_EXPECT_TRUE(signatureStateUpdate(StateHandle, "test"_u8));
    WASI_CRYPTO_EXPECT_TRUE(signatureStateUpdate(StateHandle, "test"_u8));
    WASI_CRYPTO_EXPECT_SUCCESS(SigHandle, signatureStateSign(StateHandle));
    WASI_CRYPTO_EXPECT_TRUE(signatureStateClose(StateHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
    WASI_CRYPTO_EXPECT_SUCCESS(VerifictionStateHandle,
                               signatureVerificationStateOpen(PkHandle));
    WASI_CRYPTO_EXPECT_TRUE(
        signatureVerificationStateUpdate(VerifictionStateHandle, "test"_u8));
    WASI_CRYPTO_EXPECT_TRUE(
        signatureVerificationStateUpdate(VerifictionStateHandle, "test"_u8));
    WASI_CRYPTO_EXPECT_TRUE(
        signatureVerificationStateVerify(VerifictionStateHandle, SigHandle));
    WASI_CRYPTO_EXPECT_TRUE(
        signatureVerificationStateClose(VerifictionStateHandle));
  };
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_P256_SHA256"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_K256_SHA256"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_P384_SHA384"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "Ed25519"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PKCS1_2048_SHA256"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PKCS1_2048_SHA384"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PKCS1_2048_SHA512"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PKCS1_3072_SHA384"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PKCS1_3072_SHA512"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PKCS1_4096_SHA512"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PSS_2048_SHA256"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PSS_2048_SHA384"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PSS_2048_SHA512"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PSS_3072_SHA384"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PSS_3072_SHA512"sv);
  SigTest(__WASI_ALGORITHM_TYPE_SIGNATURES, "RSA_PSS_4096_SHA512"sv);

  auto SigEncodingTest =
      [this](
          std::string_view Alg,
          std::map<__wasi_signature_encoding_e_t, std::vector<uint8_t>> Data) {
        SCOPED_TRACE(Alg);
        for (auto &[Encoding, Sig] : Data) {
          SCOPED_TRACE(Encoding);
          WASI_CRYPTO_EXPECT_SUCCESS(SigHandle,
                                     signatureImport(Alg, Sig, Encoding));
          WASI_CRYPTO_EXPECT_SUCCESS(ExportSigHandle,
                                     signatureExport(SigHandle, Encoding));
          WASI_CRYPTO_EXPECT_SUCCESS(ExportSigSize,
                                     arrayOutputLen(ExportSigHandle));
          std::vector<uint8_t> ExportSig(ExportSigSize);
          WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(ExportSigHandle, ExportSig));
          EXPECT_EQ(Sig, ExportSig);
        }
      };
  SigEncodingTest(
      "ECDSA_K256_SHA256"sv,
      {{__WASI_SIGNATURE_ENCODING_RAW,
        "9D92E9FDCA3DDF2E1DDCA1E3B7A79A250B6E4AFFCABF5F9FF4D960B152AB8300E9EB978BD3DA89C42BBFE5A2C2AEB0AF1DD178FB4BCD0833B587D118F59BBB4D"_u8v},
       {__WASI_SIGNATURE_ENCODING_DER,
        "30460221009d92e9fdca3ddf2e1ddca1e3b7a79a250b6e4affcabf5f9f"
        "f4d960b152ab8300022100e9eb978bd3da89c42bbfe5a2c2aeb0af1dd1"
        "78fb4bcd0833b587d118f59bbb4d"_u8v}});
  SigEncodingTest(
      "ECDSA_P256_SHA256"sv,
      {{__WASI_SIGNATURE_ENCODING_RAW,
        "80D5D4769AE4F3998DD6B8B01177DE855204122A361F2189F9567C806DE2673E2FBFD3FF018338875B1D144F583EB6E8DC16CF6EEB2BB5C19A3202464ABB58BD"_u8v},
       {__WASI_SIGNATURE_ENCODING_DER,
        "304502210080d5d4769ae4f3998dd6b8b01177de855204122a361f2189"
        "f9567c806de2673e02202fbfd3ff018338875b1d144f583eb6e8dc16cf"
        "6eeb2bb5c19a3202464abb58bd"_u8v}});
  SigEncodingTest(
      "Ed25519"sv,
      {{__WASI_SIGNATURE_ENCODING_RAW,
        "d4fbdb52bfa726b44d1786a8c0d171c3e62ca83c9e5bbe63de0bb2483f8fd6cc1429ab72cafc41ab56af02ff8fcc43b99bfe4c7ae940f60f38ebaa9d311c4007"_u8v}});
  SigEncodingTest(
      "RSA_PSS_2048_SHA256"sv,
      {{__WASI_SIGNATURE_ENCODING_RAW,
        "4f01e0c12b08625ecac89a69231906edf826380f37c959a96690d046316d68ff"
        "ce9d5c471694fcebfc6b45534864689256e4fc81c78e583f675d0c94b4496474"
        "51e81beff01a11a516d5e5ce3f1a910437cb8a3a5096b19fb15f4524a35b23d8"
        "9cdba12cf5b71aac1047b28c562df7c5542c34ce23a182cf7e0e231934b17294"
        "799d44877a1d68ef1b8f073619b7618e6b7c22db20030d98cf591ffc3d4da5f5"
        "8613ecd5ecfc3b40a1d02f40891ca43695cd4c088b05a8054c89c595a47e2748"
        "16f35384226f74459ee63e25a1bfc03c360490552ec38343f8ace502f065303b"
        "00bc0ec320711b211fde92e57feb9013c3609342495ec0d7cabdec21e54acc38"_u8v}});
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
