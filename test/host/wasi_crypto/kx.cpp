// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "gtest/gtest.h"

using namespace WasmEdge::Host::WASICrypto;

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

// Not Implementation
//TEST(WasiCryptoTest, KeyEncapsulation) {
//  WasiCryptoContext Ctx;
//
//  auto KxKpHandle = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
//                                        "Kyber768", std::nullopt)
//                        .value();
//  auto Pk = Ctx.keypairPublickey(KxKpHandle).value();
//  auto Sk = Ctx.keypairSecretkey(KxKpHandle).value();
//
//  auto [SecretHandle, EncapsulatedSecretHandle] = Ctx.kxEncapsulate(Pk).value();
//
//  std::vector<uint8_t> SecretRawBytes(Ctx.arrayOutputLen(SecretHandle).value(),
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
