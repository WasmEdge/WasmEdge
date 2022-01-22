#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/signature/signature.h"
#include "wasi_crypto/api.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>
#include <vector>
using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

namespace {

WasmEdge::Span<uint8_t const> operator"" _u8(const char *Str,
                                             std::size_t Len) noexcept {
  return {reinterpret_cast<uint8_t const *>(Str), Len};
}

// std::ostream &operator<<(std::ostream &Os, const std::vector<uint8_t> &Vec) {
//   for (size_t Index = 0; Index <= Vec.size(); Index += 15) {
//     std::cout << "              ";
//     auto Diff = Vec.size() - Index;
//     if (Diff >= 15) {
//       for (auto B = Vec.begin() + Index; B < Vec.begin() + Index + 15; ++B) {
//         Os << std::setw(2) << std::setfill('0') << std::hex
//            << static_cast<unsigned int>(*B) << ":";
//       }
//     } else {
//       for (auto B = Vec.begin() + Index; B < Vec.end(); ++B) {
//         Os << std::setw(2) << std::setfill('0') << std::hex
//            << static_cast<unsigned int>(*B) << ":";
//       }
//     }
//     std::cout << "\n";
//   }
//   return Os;
// }
} // namespace

TEST(WasiCryptoTest, TestSignaturesEcdsa) {
  WasiCryptoContext Ctx;
  std::vector<std::string_view> AlgList{"ECDSA_P256_SHA256",
                                        "ECDSA_K256_SHA256"};
  std::vector<
      std::pair<__wasi_publickey_encoding_e_t, __wasi_keypair_encoding_e_t>>
      EncodingList{
          {__WASI_PUBLICKEY_ENCODING_RAW, __WASI_KEYPAIR_ENCODING_RAW}/* ,
          {__WASI_PUBLICKEY_ENCODING_PEM, __WASI_KEYPAIR_ENCODING_PEM},
          {__WASI_PUBLICKEY_ENCODING_PKCS8, __WASI_KEYPAIR_ENCODING_PKCS8} */};

  for (auto Alg : AlgList) {
    for (auto [PkEncoding, KpEncoding] : EncodingList) {
      auto KpHandle = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg,
                                          std::nullopt)
                          .value();
      auto PkHandle = Ctx.keypairPublickey(KpHandle).value();

      auto const PkSerialized =
          Ctx.publickeyExport(PkHandle, PkEncoding).value();
      auto Raw =
          std::vector<uint8_t>(Ctx.arrayOutputLen(PkSerialized).value(), 0);
      Ctx.arrayOutputPull(PkSerialized, Raw).value();
      PkHandle = Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, Alg, Raw,
                                     PkEncoding)
                     .value();

      auto const KpSerialized = Ctx.keypairExport(KpHandle, KpEncoding).value();
      Raw = std::vector<uint8_t>(Ctx.arrayOutputLen(KpSerialized).value(), 0);
      Ctx.arrayOutputPull(KpSerialized, Raw).value();

      auto const Kp2Handle = Ctx.keypairImport(__WASI_ALGORITHM_TYPE_SIGNATURES,
                                               Alg, Raw, KpEncoding)
                                 .value();
      KpHandle = Kp2Handle;

      auto const StateHandle = Ctx.signatureStateOpen(KpHandle).value();
      EXPECT_TRUE(Ctx.signatureStateUpdate(StateHandle, "test"_u8).has_value());
      auto const SignatureHandle = Ctx.signatureStateSign(StateHandle).value();

      auto const VerificationStateHandle =
          Ctx.signatureVerificationStateOpen(PkHandle).value();
      EXPECT_TRUE(Ctx.signatureVerificationStateUpdate(VerificationStateHandle,
                                                       "test"_u8)
                      .has_value());
      EXPECT_TRUE(Ctx.signatureVerificationStateVerify(VerificationStateHandle,
                                                       SignatureHandle)
                      .has_value());

      EXPECT_TRUE(Ctx.signatureVerificationStateClose(VerificationStateHandle)
                      .has_value());
      EXPECT_TRUE(Ctx.signatureStateClose(StateHandle).has_value());
      EXPECT_TRUE(Ctx.keypairClose(KpHandle).has_value());
      EXPECT_TRUE(Ctx.publickeyClose(PkHandle).has_value());
      EXPECT_TRUE(Ctx.signatureClose(SignatureHandle).has_value());
    }
  }
}

TEST(WasiCryptoTest, TestEcdsaImportRawKey) {
  std::vector<uint8_t> RawPk{2,   35,  90,  26,  79,  34,  43,  87,  91,
                             209, 64,  129, 168, 232, 219, 160, 155, 202,
                             213, 162, 117, 150, 56,  115, 78,  114, 214,
                             182, 212, 225, 46,  7,   106};
  WasiCryptoContext Ctx;
  Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_P256_SHA256"sv,
                      RawPk, __WASI_PUBLICKEY_ENCODING_RAW)
      .value();

  std::vector<uint8_t> RawKp{199, 204, 76,  94,  189, 111, 171, 116,
                             201, 72,  203, 252, 231, 101, 196, 61,
                             139, 253, 106, 33,  247, 85,  12,  254,
                             243, 90,  41,  109, 170, 119, 1,   222};
  Ctx.keypairImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_P256_SHA256"sv,
                    RawKp, __WASI_KEYPAIR_ENCODING_RAW)
      .value();
}