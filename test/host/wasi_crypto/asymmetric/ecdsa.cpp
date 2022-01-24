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

// std::vector<uint8_t> operator"" _u8v(const char *Str,
//                                      std::size_t Len) noexcept {
//   std::vector<uint8_t> Res(Len / 2);
//   for (size_t I = 0; I < Len; I += 2) {
//     std::string Tran{Str + I, 2};
//     Res[I / 2] = static_cast<uint8_t>(std::strtol(Tran.c_str(), nullptr,
//     16));
//   }
//   return Res;
// }
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

TEST(WasiCryptoTest, EcdsaSignAndVerify) {
  WasiCryptoContext Ctx;
  std::vector<std::string_view> AlgList{"ECDSA_P256_SHA256",
                                        "ECDSA_K256_SHA256"};
  std::vector<
      std::pair<__wasi_publickey_encoding_e_t, __wasi_keypair_encoding_e_t>>
      EncodingList{
          {__WASI_PUBLICKEY_ENCODING_SEC, __WASI_KEYPAIR_ENCODING_RAW}/* ,
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

//  TODO: https://github.com/WebAssembly/wasi-crypto/pull/42
// TEST(WasiCryptoTest, EcdsaExternalImport) {
//   {
//     std::vector<uint8_t> RawPk{3,   216, 189, 189, 158, 243, 237, 234, 143,
//                                205, 242, 122, 132, 75,  156, 170, 121, 171,
//                                27,  74,  233, 117, 177, 210, 93,  188, 220,
//                                97,  239, 110, 101, 83,  35};
//     WasiCryptoContext Ctx;
//     Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_P256_SHA256"sv,
//                         RawPk, __WASI_PUBLICKEY_ENCODING_RAW)
//         .value();

//     std::vector<uint8_t> RawKp{44,  112, 233, 246, 15,  142, 76,  27,
//                                172, 164, 135, 253, 28,  216, 141, 54,
//                                122, 95,  216, 45,  120, 181, 207, 84,
//                                18,  245, 240, 125, 223, 219, 34,  151};
//     Ctx.keypairImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "ECDSA_P256_SHA256"sv,
//                       RawKp, __WASI_KEYPAIR_ENCODING_RAW)
//         .value();
//   }
// }