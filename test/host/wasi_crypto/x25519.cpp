// SPDX-License-Identifier: Apache-2.0

#include "helper.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/key_exchange/func.h"
#include "wasi_crypto/api.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <vector>

using namespace WasmEdge::Host::WASICrypto;

TEST_F(WasiCryptoTest, X25519) {
  // generate run
  {
    writeString("X25519", 0);
    writeOptOptions(std::nullopt, 6);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairGenerate>(
                  {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0,
                   6, 6, 14}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KxKpHandle1 = *MemInst.getPointer<__wasi_keypair_t *>(14);

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairExport>(
                  {KxKpHandle1,
                   static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KxKpRawBytesHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);

    EXPECT_EQ(testRun<Common::ArrayOutputLen>({KxKpRawBytesHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({KxKpRawBytesHandle, 0, 64, 64}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(64), 64);
    std::vector<uint8_t> KxKpRawBytes{MemInst.getPointer<__wasi_size_t *>(0),
                                      MemInst.getPointer<__wasi_size_t *>(0) +
                                          64};

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairPublickey>({KxKpHandle1, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Pk1 = *MemInst.getPointer<__wasi_publickey_t *>(0);

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairSecretkey>({KxKpHandle1, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Sk1 = *MemInst.getPointer<__wasi_secretkey_t *>(0);

    writeString("X25519", 0);
    writeOptOptions(std::nullopt, 6);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairGenerate>(
                  {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0,
                   6, 6, 14}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KxKpHandle2 = *MemInst.getPointer<__wasi_keypair_t *>(14);

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairPublickey>({KxKpHandle2, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Pk2 = *MemInst.getPointer<__wasi_publickey_t *>(0);

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairSecretkey>({KxKpHandle2, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Sk2 = *MemInst.getPointer<__wasi_secretkey_t *>(0);

    EXPECT_EQ(testRun<Kx::Dh>({Pk1, Sk2, 0}), __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SharedKey1Handle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SharedKey1Handle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({SharedKey1Handle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    std::vector<uint8_t> SharedKey1RawBytes{
        MemInst.getPointer<__wasi_size_t *>(0),
        MemInst.getPointer<__wasi_size_t *>(0) + 32};

    EXPECT_EQ(testRun<Kx::Dh>({Pk2, Sk1, 0}), __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SharedKey2Handle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SharedKey2Handle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({SharedKey2Handle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    std::vector<uint8_t> SharedKey2RawBytes{
        MemInst.getPointer<__wasi_size_t *>(0),
        MemInst.getPointer<__wasi_size_t *>(0) + 32};
    EXPECT_EQ(SharedKey1RawBytes, SharedKey2RawBytes);

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KxKpHandle1}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KxKpHandle2}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // import run
  {
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

    writeString("X25519", 0);
    writeSpan(Pk1Raw, 6);
    EXPECT_EQ(
        testRun<AsymmetricCommon::PublickeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0, 6, 6,
             32, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 38}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Pk1 = *MemInst.getPointer<__wasi_kx_publickey_t *>(38);

    writeString("X25519", 0);
    writeSpan(Sk1Raw, 6);
    EXPECT_EQ(
        testRun<AsymmetricCommon::SecretkeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0, 6, 6,
             32, static_cast<uint32_t>(__WASI_SECRETKEY_ENCODING_RAW), 38}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Sk1 = *MemInst.getPointer<__wasi_kx_secretkey_t *>(38);
    {
      EXPECT_EQ(
          testRun<AsymmetricCommon::SecretkeyExport>(
              {Sk1, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 0}),
          __WASI_CRYPTO_ERRNO_SUCCESS);
      auto Sk1OutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
      EXPECT_EQ(testRun<Common::ArrayOutputLen>({Sk1OutputHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
      EXPECT_EQ(testRun<Common::ArrayOutputPull>({Sk1OutputHandle, 0, 32, 32}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
      std::vector<uint8_t> Sk1Output{MemInst.getPointer<uint8_t *>(0),
                                     MemInst.getPointer<uint8_t *>(0) + 32};
      EXPECT_EQ(Sk1Output, Sk1Raw);
    }

    writeString("X25519", 0);
    writeSpan(Pk2Raw, 6);
    EXPECT_EQ(
        testRun<AsymmetricCommon::PublickeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0, 6, 6,
             32, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 38}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Pk2 = *MemInst.getPointer<__wasi_kx_publickey_t *>(38);
    {
      EXPECT_EQ(
          testRun<AsymmetricCommon::PublickeyExport>(
              {Pk2, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 0}),
          __WASI_CRYPTO_ERRNO_SUCCESS);
      auto Pk2OutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
      EXPECT_EQ(testRun<Common::ArrayOutputLen>({Pk2OutputHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
      EXPECT_EQ(testRun<Common::ArrayOutputPull>({Pk2OutputHandle, 0, 32, 32}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
      std::vector<uint8_t> Pk2Output{MemInst.getPointer<uint8_t *>(0),
                                     MemInst.getPointer<uint8_t *>(0) + 32};
      EXPECT_EQ(Pk2Output, Pk2Raw);
    }

    writeString("X25519", 0);
    writeSpan(Sk2Raw, 6);
    EXPECT_EQ(
        testRun<AsymmetricCommon::SecretkeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0, 6, 6,
             32, static_cast<uint32_t>(__WASI_SECRETKEY_ENCODING_RAW), 38}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto Sk2 = *MemInst.getPointer<__wasi_kx_secretkey_t *>(38);

    // shared secret 1
    EXPECT_EQ(testRun<Kx::Dh>({Pk1, Sk2, 0}), __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SharedKey1Handle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SharedKey1Handle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({SharedKey1Handle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    std::vector<uint8_t> SharedKey1RawBytes{MemInst.getPointer<uint8_t *>(0),
                                            MemInst.getPointer<uint8_t *>(0) +
                                                32};
    EXPECT_EQ(SharedKey1RawBytes, SharedSecret);

    // shared secret 2
    EXPECT_EQ(testRun<Kx::Dh>({Pk2, Sk1, 0}), __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SharedKey2Handle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SharedKey2Handle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({SharedKey2Handle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    std::vector<uint8_t> SharedKey2RawBytes{MemInst.getPointer<uint8_t *>(0),
                                            MemInst.getPointer<uint8_t *>(0) +
                                                32};
    EXPECT_EQ(SharedKey2RawBytes, SharedSecret);
  }

  // encoding check
  {
    // export pk equal to input pk
    {
      EXPECT_EQ(
          testRun<AsymmetricCommon::PublickeyExport>(
              {Pk1, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 0}),
          __WASI_CRYPTO_ERRNO_SUCCESS);
      auto Pk1OutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
      EXPECT_EQ(testRun<Common::ArrayOutputLen>({Pk1OutputHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
      EXPECT_EQ(testRun<Common::ArrayOutputPull>({Pk1OutputHandle, 0, 32, 32}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
      std::vector<uint8_t> Pk1Output{MemInst.getPointer<uint8_t *>(0),
                                     MemInst.getPointer<uint8_t *>(0) + 32};
      EXPECT_EQ(Pk1Output, Pk1Raw);
    }

        {
      EXPECT_EQ(
          testRun<AsymmetricCommon::SecretkeyExport>(
              {Sk2, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 0}),
          __WASI_CRYPTO_ERRNO_SUCCESS);
      auto Sk2OutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
      EXPECT_EQ(testRun<Common::ArrayOutputLen>({Sk2OutputHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
      EXPECT_EQ(testRun<Common::ArrayOutputPull>({Sk2OutputHandle, 0, 32, 32}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
      std::vector<uint8_t> Sk2Output{MemInst.getPointer<uint8_t *>(0),
                                     MemInst.getPointer<uint8_t *>(0) + 32};
      EXPECT_EQ(Sk2Output, Sk2Raw);
    }


    for (auto Encoding : UnsupportedPkEncoding) {
      writeString("Ed25519", 0);
      EXPECT_EQ(testRun<AsymmetricCommon::PublickeyImport>(
                    {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                     7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      EXPECT_EQ(testRun<AsymmetricCommon::PublickeyExport>(
                    {PkHandle, static_cast<uint32_t>(Encoding), 0}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    }
  }
}

// Not Implementation
// TEST_F(WasiCryptoTest, KeyEncapsulation) {
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
