// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/ctx.h"
#include "host/wasi_crypto/key_exchange/ctx.h"
#include "gtest/gtest.h"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST(WasiCryptoTest, KeyExchange) {
  CommonContext CommonCtx;
  AsymmetricCommonContext AsyComCtx{CommonCtx};
  KxContext KxCtx{CommonCtx};

  auto KxKpHandle1 = AsyComCtx
                         .keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
                                          "X25519", std::nullopt)
                         .value();
  auto KxKpRawBytesHandle =
      AsyComCtx.keypairExport(KxKpHandle1, __WASI_KEYPAIR_ENCODING_RAW).value();
  std::vector<uint8_t> KxKpRawBytes(
      CommonCtx.arrayOutputLen(KxKpRawBytesHandle).value(), 0);
  CommonCtx.arrayOutputPull(KxKpRawBytesHandle, KxKpRawBytes).value();

  auto Pk1 = AsyComCtx.keypairPublickey(KxKpHandle1).value();
  auto Sk1 = AsyComCtx.keypairSecretkey(KxKpHandle1).value();

  auto KxKpHandle2 = AsyComCtx
                         .keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
                                          "X25519", std::nullopt)
                         .value();
  auto Pk2 = AsyComCtx.keypairPublickey(KxKpHandle2).value();
  auto Sk2 = AsyComCtx.keypairSecretkey(KxKpHandle2).value();

  auto SharedKey1Handle = KxCtx.kxDh(Pk1, Sk2).value();
  std::vector<uint8_t> SharedKey1RawBytes(
      CommonCtx.arrayOutputLen(KxKpRawBytesHandle).value(), 0);

  CommonCtx.arrayOutputPull(SharedKey1Handle, SharedKey1RawBytes).value();

  auto SharedKey2Handle = KxCtx.kxDh(Pk2, Sk1).value();
  std::vector<uint8_t> SharedKey2RawBytes(
      CommonCtx.arrayOutputLen(KxKpRawBytesHandle).value(), 0);
  CommonCtx.arrayOutputPull(SharedKey2Handle, SharedKey2RawBytes).value();

  EXPECT_EQ(SharedKey1RawBytes, SharedKey2RawBytes);

  EXPECT_TRUE(AsyComCtx.keypairClose(KxKpHandle1).has_value());
  EXPECT_TRUE(AsyComCtx.keypairClose(KxKpHandle2).has_value());
}

TEST(WasiCryptoTest, KeyEncapsulation) {

  CommonContext C;
  AsymmetricCommonContext Ctx{C};
  KxContext KxCtx{C};

  auto KxKpHandle = Ctx.keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
                                        "Kyber768", std::nullopt)
                        .value();
  auto Pk = Ctx.keypairPublickey(KxKpHandle).value();
  auto Sk = Ctx.keypairSecretkey(KxKpHandle).value();

  auto [SecretHandle, EncapsulatedSecretHandle] =
      KxCtx.kxEncapsulate(Pk).value();

  std::vector<uint8_t> SecretRawBytes(C.arrayOutputLen(SecretHandle).value(),
                                      0);

  C.arrayOutputPull(SecretHandle, SecretRawBytes).value();
  std::vector<uint8_t> EncapsulatedSecretRawBytes(
      C.arrayOutputLen(EncapsulatedSecretHandle).value(), 0);

  C.arrayOutputPull(EncapsulatedSecretHandle, EncapsulatedSecretRawBytes)
      .value();

  auto DecapsulatedSecretHandle =
      KxCtx.kxDecapsulate(Sk, EncapsulatedSecretRawBytes).value();
  std::vector<uint8_t> DecapsulatedSecretRawBytes(
      C.arrayOutputLen(DecapsulatedSecretHandle).value(), 0);
  C.arrayOutputPull(DecapsulatedSecretHandle, DecapsulatedSecretRawBytes)
      .value();

  EXPECT_EQ(SecretRawBytes, DecapsulatedSecretRawBytes);
}
