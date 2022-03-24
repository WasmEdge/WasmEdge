// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "helper.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/signatures/func.h"

#include <cstdint>
#include <optional>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Signatures) {
  // use generate data sign and verfiy
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
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge