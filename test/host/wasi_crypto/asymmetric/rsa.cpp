
#include "host/wasi_crypto/ctx.h"
#include "wasi_crypto/api.hpp"
#include "gtest/gtest.h"
#include <cstdint>

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

namespace {

WasmEdge::Span<uint8_t const> operator"" _u8(const char *Str,
                                             std::size_t Len) noexcept {
  return {reinterpret_cast<uint8_t const *>(Str), Len};
}
} // namespace

TEST(WasiCryptoTest, TestSignaturesRsa) {
  std::vector<std::string_view> AlgList{
      "RSA_PKCS1_2048_SHA256", "RSA_PKCS1_2048_SHA384", "RSA_PKCS1_2048_SHA512",
      "RSA_PKCS1_3072_SHA384", "RSA_PKCS1_3072_SHA512", "RSA_PKCS1_4096_SHA512",
      "RSA_PSS_2048_SHA256",   "RSA_PSS_2048_SHA384",   "RSA_PSS_2048_SHA512",
      "RSA_PSS_3072_SHA384",   "RSA_PSS_3072_SHA512",   "RSA_PSS_4096_SHA512"};

  std::vector<
      std::pair<__wasi_publickey_encoding_e_t, __wasi_keypair_encoding_e_t>>
      EncodingList{
          {__WASI_PUBLICKEY_ENCODING_PEM, __WASI_KEYPAIR_ENCODING_PEM},
          {__WASI_PUBLICKEY_ENCODING_PKCS8, __WASI_KEYPAIR_ENCODING_PKCS8}};

  for (auto Alg : AlgList) {
    for (auto [PkEncoding, KpEncoding] : EncodingList) {
      WasiCryptoContext Ctx;

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

TEST(WasiCryptoTest, ImportRsaPkcs8Key) {
  std::vector<uint8_t> Pkcs8Pk{
      48,  130, 1,   34,  48,  13,  6,   9,   42,  134, 72,  134, 247, 13,  1,
      1,   1,   5,   0,   3,   130, 1,   15,  0,   48,  130, 1,   10,  2,   130,
      1,   1,   0,   192, 134, 230, 170, 21,  20,  55,  219, 217, 208, 122, 48,
      236, 90,  141, 158, 251, 86,  71,  199, 49,  188, 85,  69,  28,  15,  216,
      92,  103, 24,  215, 244, 77,  166, 98,  106, 118, 223, 233, 29,  120, 115,
      242, 58,  167, 255, 242, 173, 237, 97,  163, 124, 207, 226, 174, 91,  51,
      231, 107, 173, 29,  182, 169, 180, 137, 116, 247, 186, 245, 56,  23,  164,
      79,  128, 189, 37,  167, 100, 111, 81,  241, 142, 189, 145, 152, 112, 101,
      183, 252, 69,  61,  220, 160, 35,  226, 186, 55,  74,  120, 26,  4,   200,
      232, 121, 112, 190, 32,  111, 59,  254, 86,  220, 232, 19,  96,  76,  246,
      252, 145, 64,  29,  173, 241, 102, 50,  136, 55,  136, 44,  54,  173, 65,
      215, 21,  108, 100, 174, 174, 62,  113, 182, 55,  13,  238, 231, 2,   49,
      16,  65,  104, 169, 211, 89,  108, 108, 215, 136, 6,   175, 208, 4,   63,
      216, 69,  226, 77,  249, 167, 2,   4,   246, 203, 239, 69,  250, 246, 37,
      189, 224, 1,   38,  115, 239, 58,  152, 215, 184, 46,  119, 81,  192, 219,
      67,  214, 186, 179, 57,  53,  168, 89,  49,  59,  111, 58,  215, 202, 183,
      225, 65,  31,  163, 203, 206, 13,  192, 68,  191, 18,  90,  202, 77,  98,
      76,  202, 231, 119, 107, 234, 14,  109, 165, 0,   229, 12,  21,  96,  29,
      44,  184, 192, 244, 47,  52,  9,   220, 143, 48,  49,  168, 174, 11,  155,
      252, 85,  215, 105, 2,   3,   1,   0,   1};

  WasiCryptoContext Ctx;
  Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES,
                      "RSA_PKCS1_2048_SHA256"sv, Pkcs8Pk,
                      __WASI_PUBLICKEY_ENCODING_PKCS8)
      .value();
}

TEST(WasiCryptoTest, ImportRsaPemKey) {
  std::vector<uint8_t> PemPk{
      45,  45,  45,  45,  45,  66,  69,  71,  73,  78,  32,  80,  85,  66,  76,
      73,  67,  32,  75,  69,  89,  45,  45,  45,  45,  45,  10,  77,  73,  73,
      66,  73,  106, 65,  78,  66,  103, 107, 113, 104, 107, 105, 71,  57,  119,
      48,  66,  65,  81,  69,  70,  65,  65,  79,  67,  65,  81,  56,  65,  77,
      73,  73,  66,  67,  103, 75,  67,  65,  81,  69,  65,  110, 119, 70,  88,
      81,  73,  114, 75,  104, 82,  76,  47,  122, 69,  114, 122, 86,  105, 114,
      106, 10,  55,  106, 85,  112, 47,  47,  71,  76,  102, 118, 101, 86,  81,
      121, 115, 100, 114, 87,  68,  102, 77,  98,  110, 77,  100, 98,  120, 106,
      51,  86,  108, 68,  104, 100, 83,  52,  98,  75,  69,  108, 112, 80,  51,
      67,  90,  48,  68,  75,  74,  77,  78,  84,  118, 89,  49,  103, 110, 80,
      54,  102, 67,  53,  48,  101, 10,  100, 109, 57,  107, 76,  77,  109, 105,
      86,  65,  88,  101, 83,  102, 89,  70,  89,  119, 52,  122, 100, 111, 120,
      71,  67,  68,  70,  74,  68,  83,  71,  117, 81,  85,  43,  108, 105, 119,
      51,  110, 47,  69,  103, 85,  67,  117, 75,  57,  79,  68,  103, 122, 86,
      118, 119, 119, 65,  67,  75,  104, 88,  114, 85,  43,  10,  67,  54,  72,
      48,  88,  103, 109, 105, 119, 110, 70,  78,  83,  120, 66,  73,  111, 105,
      54,  111, 76,  102, 52,  67,  117, 79,  119, 99,  77,  85,  107, 65,  54,
      117, 86,  118, 67,  75,  71,  109, 105, 50,  104, 47,  115, 112, 118, 83,
      108, 86,  97,  71,  120, 117, 103, 103, 90,  103, 87,  43,  90,  65,  97,
      121, 10,  80,  106, 57,  76,  52,  110, 103, 47,  116, 113, 71,  78,  47,
      69,  82,  57,  120, 80,  56,  83,  109, 51,  122, 82,  84,  114, 79,  52,
      51,  107, 47,  105, 68,  112, 67,  107, 120, 72,  107, 99,  113, 108, 117,
      80,  57,  98,  121, 79,  110, 82,  114, 49,  50,  75,  80,  74,  117, 52,
      112, 48,  121, 116, 81,  52,  10,  122, 110, 68,  50,  67,  84,  97,  56,
      121, 88,  87,  77,  53,  110, 65,  84,  71,  118, 110, 67,  79,  97,  122,
      74,  76,  112, 107, 111, 65,  88,  74,  103, 53,  101, 57,  114, 50,  70,
      98,  104, 106, 86,  101, 111, 120, 106, 67,  57,  49,  90,  69,  43,  104,
      108, 85,  104, 72,  82,  105, 86,  106, 101, 119, 69,  10,  84,  119, 73,
      68,  65,  81,  65,  66,  10,  45,  45,  45,  45,  45,  69,  78,  68,  32,
      80,  85,  66,  76,  73,  67,  32,  75,  69,  89,  45,  45,  45,  45,  45,
      10};

  WasiCryptoContext Ctx;
  auto Handle = Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_SIGNATURES,
                                    "RSA_PKCS1_2048_SHA256"sv, PemPk,
                                    __WASI_PUBLICKEY_ENCODING_PEM)
                    .value();
  auto EqualHandle =
      Ctx.publickeyExport(Handle, __WASI_PUBLICKEY_ENCODING_PEM).value();
  std::vector<uint8_t> New(Ctx.arrayOutputLen(EqualHandle).value());
  Ctx.arrayOutputPull(EqualHandle, New);

  EXPECT_EQ(PemPk, New);
}