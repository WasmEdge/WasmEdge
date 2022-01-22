// SPDX-License-Identifier: Apache-2.0

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

TEST(WasiCryptoTest, EddsaImportKey) {
  WasiCryptoContext Ctx;

  {
    std::vector<uint8_t> PkRaw{116, 42,  60,  63,  190, 75,  50,  93,
                               64,  19,  239, 154, 140, 59,  223, 61,
                               28,  89,  193, 93,  224, 183, 9,   133,
                               57,  125, 25,  19,  144, 164, 254, 121};
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
    std::vector<uint8_t> SkRaw{139, 70,  42,  88,  234, 132, 47,  28,
                               192, 116, 117, 86,  142, 229, 234, 209,
                               209, 76,  223, 252, 131, 174, 49,  248,
                               137, 143, 186, 49,  21,  15,  67,  22};
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
    std::vector<uint8_t> KpRaw{
        139, 70,  42,  88,  234, 132, 47,  28,  192, 116, 117, 86,  142,
        229, 234, 209, 209, 76,  223, 252, 131, 174, 49,  248, 137, 143,
        186, 49,  21,  15,  67,  22,  116, 42,  60,  63,  190, 75,  50,
        93,  64,  19,  239, 154, 140, 59,  223, 61,  28,  89,  193, 93,
        224, 183, 9,   133, 57,  125, 25,  19,  144, 164, 254, 121};

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
