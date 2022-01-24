// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "wasi_crypto/api.hpp"
#include "gtest/gtest.h"

using namespace WasmEdge::Host::WASICrypto;
namespace {
std::vector<uint8_t> operator"" _u8v(const char *Str,
                                     std::size_t Len) noexcept {
  std::vector<uint8_t> Res(Len / 2);
  for (size_t I = 0; I < Len; I += 2) {
    std::string Tran{Str + I, 2};
    Res[I / 2] = static_cast<uint8_t>(std::strtol(Tran.c_str(), nullptr, 16));
  }
  return Res;
}
} // namespace

TEST(WasiCryptoTest, KeyExchange) {
  WasiCryptoContext Ctx;
  auto KxKpHandle1 = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
                                         "X25519", std::nullopt)
                         .value();
  auto KxKpRawBytesHandle =
      Ctx.keypairExport(KxKpHandle1, __WASI_KEYPAIR_ENCODING_RAW).value();
  std::vector<uint8_t> KxKpRawBytes(
      Ctx.arrayOutputLen(KxKpRawBytesHandle).value(), 0);
  Ctx.arrayOutputPull(KxKpRawBytesHandle, KxKpRawBytes).value();

  auto Pk1 = Ctx.keypairPublickey(KxKpHandle1).value();
  auto Sk1 = Ctx.keypairSecretkey(KxKpHandle1).value();

  auto KxKpHandle2 = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
                                         "X25519", std::nullopt)
                         .value();
  auto Pk2 = Ctx.keypairPublickey(KxKpHandle2).value();
  auto Sk2 = Ctx.keypairSecretkey(KxKpHandle2).value();

  auto SharedKey1Handle = Ctx.kxDh(Pk1, Sk2).value();
  std::vector<uint8_t> SharedKey1RawBytes(
      Ctx.arrayOutputLen(SharedKey1Handle).value());

  Ctx.arrayOutputPull(SharedKey1Handle, SharedKey1RawBytes).value();

  auto SharedKey2Handle = Ctx.kxDh(Pk2, Sk1).value();
  std::vector<uint8_t> SharedKey2RawBytes(
      Ctx.arrayOutputLen(SharedKey2Handle).value());
  Ctx.arrayOutputPull(SharedKey2Handle, SharedKey2RawBytes).value();

  EXPECT_EQ(SharedKey1RawBytes, SharedKey2RawBytes);

  EXPECT_TRUE(Ctx.keypairClose(KxKpHandle1).has_value());
  EXPECT_TRUE(Ctx.keypairClose(KxKpHandle2).has_value());
}

TEST(WasiCryptoTest, X25519ExternalImport) {
  // from https://datatracker.ietf.org/doc/html/rfc7748#section-6.1
  auto Sk1Raw =
      "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"_u8v;
  auto Pk1Raw =
      "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"_u8v;

  auto Sk2Raw =
      "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb"_u8v;
  auto Pk2Raw =
      "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f"_u8v;

  auto SharedSecret =
      "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742"_u8v;

  WasiCryptoContext Ctx;
  auto Pk1 = Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, "X25519",
                                 Pk1Raw, __WASI_PUBLICKEY_ENCODING_RAW)
                 .value();
  auto Sk1 = Ctx.secretkeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, "X25519",
                                 Sk1Raw, __WASI_SECRETKEY_ENCODING_RAW)
                 .value();

  auto Pk2 = Ctx.publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, "X25519",
                                 Pk2Raw, __WASI_PUBLICKEY_ENCODING_RAW)
                 .value();
  auto Sk2 = Ctx.secretkeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, "X25519",
                                 Sk2Raw, __WASI_SECRETKEY_ENCODING_RAW)
                 .value();

  auto SharedKey1Handle = Ctx.kxDh(Pk1, Sk2).value();
  std::vector<uint8_t> SharedKey1RawBytes(
      Ctx.arrayOutputLen(SharedKey1Handle).value());
  Ctx.arrayOutputPull(SharedKey1Handle, SharedKey1RawBytes).value();

  auto SharedKey2Handle = Ctx.kxDh(Pk2, Sk1).value();
  std::vector<uint8_t> SharedKey2RawBytes(
      Ctx.arrayOutputLen(SharedKey2Handle).value());
  Ctx.arrayOutputPull(SharedKey2Handle, SharedKey2RawBytes).value();

  EXPECT_EQ(SharedKey1RawBytes, SharedSecret);
  EXPECT_EQ(SharedKey2RawBytes, SharedSecret);
}

// Not Implementation
// TEST(WasiCryptoTest, KeyEncapsulation) {
//  WasiCryptoContext Ctx;
//
//  auto KxKpHandle = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
//                                        "Kyber768", std::nullopt)
//                        .value();
//  auto Pk = Ctx.keypairPublickey(KxKpHandle).value();
//  auto Sk = Ctx.keypairSecretkey(KxKpHandle).value();
//
//  auto [SecretHandle, EncapsulatedSecretHandle] =
//  Ctx.kxEncapsulate(Pk).value();
//
//  std::vector<uint8_t>
//  SecretRawBytes(Ctx.arrayOutputLen(SecretHandle).value(),
//                                      0);
//
//  Ctx.arrayOutputPull(SecretHandle, SecretRawBytes).value();
//  std::vector<uint8_t> EncapsulatedSecretRawBytes(
//      Ctx.arrayOutputLen(EncapsulatedSecretHandle).value(), 0);
//
//  Ctx.arrayOutputPull(EncapsulatedSecretHandle, EncapsulatedSecretRawBytes)
//      .value();
//
//  auto DecapsulatedSecretHandle =
//      Ctx.kxDecapsulate(Sk, EncapsulatedSecretRawBytes).value();
//  std::vector<uint8_t> DecapsulatedSecretRawBytes(
//      Ctx.arrayOutputLen(DecapsulatedSecretHandle).value(), 0);
//  Ctx.arrayOutputPull(DecapsulatedSecretHandle, DecapsulatedSecretRawBytes)
//      .value();
//
//  EXPECT_EQ(SecretRawBytes, DecapsulatedSecretRawBytes);
//}
