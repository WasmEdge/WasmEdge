// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "gtest/gtest.h"
#include "host/wasi_crypto/signature/signature.h"

#include <vector>
#include <iostream>
using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

namespace {

WasmEdge::Span<uint8_t const> operator"" _u8(const char *Str,
                                             std::size_t Len) noexcept {
  return {reinterpret_cast<uint8_t const *>(Str), Len};
}

//std::ostream &operator<<(std::ostream &Os, const std::vector<uint8_t> &Vec) {
//  for (size_t Index = 0; Index <= Vec.size(); Index += 15) {
//    std::cout << "              ";
//    auto Diff = Vec.size() - Index;
//    if (Diff >= 15) {
//      for (auto B = Vec.begin() + Index; B < Vec.begin() + Index + 15; ++B) {
//        Os << std::setw(2) << std::setfill('0') << std::hex
//           << static_cast<unsigned int>(*B) << ":";
//      }
//    } else {
//      for (auto B = Vec.begin() + Index; B < Vec.end(); ++B) {
//        Os << std::setw(2) << std::setfill('0') << std::hex
//           << static_cast<unsigned int>(*B) << ":";
//      }
//    }
//    std::cout << "\n";
//  }
//  return Os;
//}
} // namespace

TEST(WasiCryptoTest, TestSignaturesEcdsa) {
  WasiCryptoContext Ctx;

  std::string_view Alg = "ECDSA_P256_SHA256";

  auto KpHandle =
      Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, std::nullopt)
          .value();
  auto PkHandle = Ctx.keypairPublickey(KpHandle).value();

  auto const PkSerialized =
      Ctx.publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_RAW).value();
  auto Raw = std::vector<uint8_t>(Ctx.arrayOutputLen(PkSerialized).value(), 0);
  Ctx.arrayOutputPull(PkSerialized, Raw).value();
  PkHandle = Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, Raw,
                                 __WASI_PUBLICKEY_ENCODING_RAW)
                 .value();

  auto const KpSerialized =
      Ctx.keypairExport(KpHandle, __WASI_KEYPAIR_ENCODING_RAW).value();
  Raw = std::vector<uint8_t>(Ctx.arrayOutputLen(KpSerialized).value(), 0);
  Ctx.arrayOutputPull(KpSerialized, Raw).value();


  auto const Kp2Handle =
      Ctx.keypairImport(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, Raw,
                        __WASI_KEYPAIR_ENCODING_RAW)
          .value();
  KpHandle = Kp2Handle;

  auto const StateHandle = Ctx.signatureStateOpen(KpHandle).value();
  EXPECT_TRUE(Ctx.signatureStateUpdate(StateHandle, "test"_u8).has_value());
  auto const SignatureHandle = Ctx.signatureStateSign(StateHandle).value();

  auto const VerificationStateHandle =
      Ctx.signatureVerificationStateOpen(PkHandle).value();
  EXPECT_TRUE(
      Ctx.signatureVerificationStateUpdate(VerificationStateHandle, "test"_u8)
          .has_value());
  EXPECT_TRUE(Ctx.signatureVerificationStateVerify(VerificationStateHandle,
                                                   SignatureHandle)
                  .has_value());

  EXPECT_TRUE(
      Ctx.signatureVerificationStateClose(VerificationStateHandle).has_value());
  EXPECT_TRUE(Ctx.signatureStateClose(StateHandle).has_value());
  EXPECT_TRUE(Ctx.keypairClose(KpHandle).has_value());
  EXPECT_TRUE(Ctx.publickeyClose(PkHandle).has_value());
  EXPECT_TRUE(Ctx.signatureClose(SignatureHandle).has_value());
}

TEST(WasiCryptoTest, TestSignaturesEddsa) {
  WasiCryptoContext Ctx;

  auto const KpHandle = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_SIGNATURES,
                                            "Ed25519", std::nullopt)
                            .value();
  auto const PkHandle = Ctx.keypairPublickey(KpHandle).value();

  auto const StateHandle = Ctx.signatureStateOpen(KpHandle).value();
  EXPECT_TRUE(Ctx.signatureStateUpdate(StateHandle, "test"_u8).has_value());
  auto const SignatureHandle = Ctx.signatureStateSign(StateHandle).value();

  auto const VerificationStateHandle =
      Ctx.signatureVerificationStateOpen(PkHandle).value();
  EXPECT_TRUE(
      Ctx.signatureVerificationStateUpdate(VerificationStateHandle, "test"_u8)
          .has_value());
  EXPECT_TRUE(Ctx.signatureVerificationStateVerify(VerificationStateHandle,
                                                   SignatureHandle)
                  .has_value());
  EXPECT_TRUE(
      Ctx.signatureVerificationStateClose(VerificationStateHandle).has_value());
  EXPECT_TRUE(Ctx.signatureStateClose(StateHandle).has_value());
  EXPECT_TRUE(Ctx.keypairClose(KpHandle).has_value());
  EXPECT_TRUE(Ctx.publickeyClose(PkHandle).has_value());
  EXPECT_TRUE(Ctx.signatureClose(SignatureHandle).has_value());
}

TEST(WasiCryptoTest, TestSignaturesRsa) {

  std::string_view Alg = "RSA_PKCS1_2048_SHA256";
  WasiCryptoContext Ctx;

  auto KpHandle =
      Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, std::nullopt)
          .value();
  auto PkHandle = Ctx.keypairPublickey(KpHandle).value();

  auto const PkSerialized =
      Ctx.publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_RAW).value();
  auto Raw = std::vector<uint8_t>(Ctx.arrayOutputLen(PkSerialized).value(), 0);
  Ctx.arrayOutputPull(PkSerialized, Raw).value();
  PkHandle = Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, Raw,
                                 __WASI_PUBLICKEY_ENCODING_RAW)
                 .value();

  auto const KpSerialized =
      Ctx.keypairExport(KpHandle, __WASI_KEYPAIR_ENCODING_RAW).value();
  Raw = std::vector<uint8_t>(Ctx.arrayOutputLen(KpSerialized).value(), 0);
  Ctx.arrayOutputPull(KpSerialized, Raw).value();
  auto const Kp2Handle =
      Ctx.keypairImport(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, Raw,
                        __WASI_KEYPAIR_ENCODING_RAW)
          .value();
  KpHandle = Kp2Handle;

  auto const StateHandle = Ctx.signatureStateOpen(KpHandle).value();
  EXPECT_TRUE(Ctx.signatureStateUpdate(StateHandle, "test"_u8).has_value());
  auto const SignatureHandle = Ctx.signatureStateSign(StateHandle).value();

  auto const VerificationStateHandle =
      Ctx.signatureVerificationStateOpen(PkHandle).value();
  EXPECT_TRUE(
      Ctx.signatureVerificationStateUpdate(VerificationStateHandle, "test"_u8)
          .has_value());
  EXPECT_TRUE(Ctx.signatureVerificationStateVerify(VerificationStateHandle,
                                                   SignatureHandle)
                  .has_value());
  EXPECT_TRUE(
      Ctx.signatureVerificationStateClose(VerificationStateHandle).has_value());
  EXPECT_TRUE(Ctx.signatureStateClose(StateHandle).has_value());
  EXPECT_TRUE(Ctx.keypairClose(KpHandle).has_value());
  EXPECT_TRUE(Ctx.publickeyClose(PkHandle).has_value());
  EXPECT_TRUE(Ctx.signatureClose(SignatureHandle).has_value());
}
