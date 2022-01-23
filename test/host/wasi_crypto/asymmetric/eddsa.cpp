// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/signature/alg.h"
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

std::vector<uint8_t> operator"" _u8v(const char *Str,
                                     std::size_t Len) noexcept {
  std::vector<uint8_t> Res(Len / 2);
  for (size_t I = 0; I < Len; I += 2) {
    std::string Tran{Str + I, 2};
    Res[I / 2] = static_cast<uint8_t>(std::strtol(Tran.c_str(), nullptr, 16));
  }
  return Res;
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

TEST(WasiCryptoTest, EddsaExternalImport) {
  WasiCryptoContext Ctx;
  // TEST1 from https://datatracker.ietf.org/doc/html/rfc8032#section-7.1
  {
    auto PkRaw =
        "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"_u8v;
    auto PkHandle =
        Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "Ed25519"sv,
                            PkRaw, __WASI_PUBLICKEY_ENCODING_RAW)
            .value();

    std::vector<uint8_t> NewPkRaw(32);
    Ctx.arrayOutputPull(
           Ctx.publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_RAW).value(),
           NewPkRaw)
        .value();

    EXPECT_EQ(PkRaw, NewPkRaw);
  }

  {
    auto SkRaw =
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"_u8v;
    auto SkHandle =
        Ctx.secretkeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "Ed25519"sv,
                            SkRaw, __WASI_SECRETKEY_ENCODING_RAW)
            .value();

    std::vector<uint8_t> NewSkRaw(32);
    Ctx.arrayOutputPull(
           Ctx.secretkeyExport(SkHandle, __WASI_SECRETKEY_ENCODING_RAW).value(),
           NewSkRaw)
        .value();

    EXPECT_EQ(SkRaw, NewSkRaw);
  }

  {
    // sk concat pk
    auto KpRaw =
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"_u8v;
    auto KpHandle =
        Ctx.keypairImport(__WASI_ALGORITHM_TYPE_SIGNATURES, "Ed25519"sv, KpRaw,
                          __WASI_KEYPAIR_ENCODING_RAW)
            .value();

    std::vector<uint8_t> NewKpRaw(64);
    Ctx.arrayOutputPull(
           Ctx.keypairExport(KpHandle, __WASI_KEYPAIR_ENCODING_RAW).value(),
           NewKpRaw)
        .value();

    EXPECT_EQ(KpRaw, NewKpRaw);
  }

  {
    auto SigRaw =
        "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"_u8v;
    auto SigHandle = Ctx.signatureImport(SignatureAlgorithm::Ed25519, SigRaw,
                                         __WASI_SIGNATURE_ENCODING_RAW)
                         .value();

    std::vector<uint8_t> NewSigRaw(64);
    Ctx.arrayOutputPull(
           Ctx.signatureExport(SigHandle, __WASI_SIGNATURE_ENCODING_RAW)
               .value(),
           NewSigRaw)
        .value();

    EXPECT_EQ(SigRaw, NewSigRaw);
  }
}

TEST(WasiCryptoTest, EddsaSignAndVerfiy) {
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
