#include "helper.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/signature/func.h"
#include "wasi_crypto/api.hpp"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, EcdsaP256Sha256) {
  // generate run
  {
    writeString("ECDSA_P256_SHA256", 0);
    writeOptOptions(std::nullopt, 17);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairGenerate>(
                  {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                   17, 17, 25})
                  .value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KpHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(25);

    EXPECT_EQ(
        testRun<AsymmetricCommon::KeypairPublickey>({KpHandle, 0}).value(),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PkHandle = *MemInst.getPointer<__wasi_signature_publickey_t *>(0);

    EXPECT_EQ(testRun<Signatures::StateOpen>({KpHandle, 0}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const StateHandle = *MemInst.getPointer<__wasi_signature_state_t *>(0);

    writeSpan("test"_u8, 0);
    EXPECT_EQ(testRun<Signatures::StateUpdate>({StateHandle, 0, 4}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::StateUpdate>({StateHandle, 0, 4}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Signatures::StateSign>({StateHandle, 0}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const SignatureHandle = *MemInst.getPointer<__wasi_signature_t *>(0);

    EXPECT_EQ(testRun<Signatures::VerificationStateOpen>({PkHandle, 0}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const VerificationStateHandle =
        *MemInst.getPointer<__wasi_signature_verification_state_t *>(0);

    writeSpan("test"_u8, 0);
    EXPECT_EQ(testRun<Signatures::VerificationStateUpdate>(
                  {VerificationStateHandle, 0, 4})
                  .value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::VerificationStateUpdate>(
                  {VerificationStateHandle, 0, 4})
                  .value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Signatures::VerificationStateVerify>(
                  {VerificationStateHandle, SignatureHandle})
                  .value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(
        testRun<Signatures::VerificationStateClose>({VerificationStateHandle})
            .value(),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::StateClose>({StateHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KpHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::PublickeyClose>({PkHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::Close>({SignatureHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // pk
  // 0x04 x:60fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb6
  // y:7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299
  auto UnCompressedPk =
      "0460FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB67903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299"_u8v;
  auto Pcks8UncompressedPk =
      "3059301306072a8648ce3d020106082a8648ce3d0301070342000460FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB67903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299"_u8v;
  auto PemUncompressedPk =
      "-----BEGIN PUBLIC KEY-----\n"
      "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEYP7UuiVanTHJYet0xjVtaMBJuJI7\n"
      "Yfps5mliLmDyn7Z5A/4QCLi8maQa6elWKLxk8vGyDC1+n1F3o8KU1EYimQ==\n"
      "-----END PUBLIC KEY-----\n"_u8;

  // 0x02/0x03 x
  auto CompressedPk =
      "0360FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6"_u8v;
  auto Pcks8CompressedPk =
      "3039301306072a8648ce3d020106082a8648ce3d0301070322000360FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6"_u8;
  auto PemCompressedPk =
      "-----BEGIN PUBLIC KEY-----\n"
      "MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDIgADYP7UuiVanTHJYet0xjVtaMBJuJI7\n"
      "Yfps5mliLmDyn7Y=\n"
      "-----END PUBLIC KEY-----\n"_u8;

  auto Sk =
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721"_u8v;
  auto Pcks8Sk =
      "04418834f4485404c428460c2061c080400c04c181caa192338f408041820aa192338f40c041c11b4c1ac080404108326bea76116e9d459ad70855d9ec75a4d39430f6cdba26c49ee2988ac483d9c8685100d08001183fb52e8956a74c72587add318d5b5a30126e248ed87e9b399a588b983ca7ed9e40ff84022e2f266906ba7a558a2f193cbc6c830b5fa7d45de8f0a5351188a6443433d121501310a118"_u8;
  auto PemSk =
      "-----BEGIN PRIVATE KEY-----\n"
      "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgya+p2EW6dRZrXCFX\n"
      "Z7HWk05Qw9s26JsSe4piKxIPZyGhRANCAARg/tS6JVqdMclh63TGNW1owEm4kjth\n"
      "+mzmaWIuYPKftnkD/hAIuLyZpBrp6VYovGTy8bIMLX6fUXejwpTURiKZ\n"
      "-----END PRIVATE KEY-----\n"_u8;

  auto Kp =
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721"_u8v;
  auto SigRaw =
      "EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8"_u8v;
  // import run
  // notice: kp = sk in ecdsa
  // test data from https://datatracker.ietf.org/doc/html/rfc6979#appendix-A.2.5
  {
    writeString("ECDSA_P256_SHA256", 0);
    writeSpan(Kp, 17);
    EXPECT_EQ(
        testRun<AsymmetricCommon::KeypairImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 17, 17,
             32, static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW), 49})
            .value(),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KpHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(49);

    EXPECT_EQ(
        testRun<AsymmetricCommon::KeypairPublickey>({KpHandle, 0}).value(),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PkHandle = *MemInst.getPointer<__wasi_signature_publickey_t *>(0);

    // EXPECT_EQ(testRun<AsymmetricCommon::PublickeyExport>(.value()
    //               {PkHandle,
    //                static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_SEC), 0}),
    //           __WASI_CRYPTO_ERRNO_SUCCESS);
    // auto PkOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    // EXPECT_EQ(testRun<Common::ArrayOutputLen>({PkOutputHandle, 0}),.value()
    //           __WASI_CRYPTO_ERRNO_SUCCESS);
    // EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    // EXPECT_EQ(testRun<Common::ArrayOutputPull>({PkOutputHandle, 0, 32,
    // 32}),.value()
    //           __WASI_CRYPTO_ERRNO_SUCCESS);
    // EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    // EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0),
    //                                 MemInst.getPointer<uint8_t *>(0) + 32}),
    //           UnCompressedPk);

    EXPECT_EQ(testRun<Signatures::StateOpen>({KpHandle, 0}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const StateHandle = *MemInst.getPointer<__wasi_signature_state_t *>(0);

    writeSpan("sample"_u8, 0);
    EXPECT_EQ(testRun<Signatures::StateUpdate>({StateHandle, 0, 6}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Signatures::StateSign>({StateHandle, 0}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const SigHandle = *MemInst.getPointer<__wasi_signature_t *>(0);

    // EXPECT_EQ(testRun<Signatures::Export>(.value()
    //               {SigHandle,
    //                static_cast<uint32_t>(__WASI_SIGNATURE_ENCODING_RAW), 0}),
    //           __WASI_CRYPTO_ERRNO_SUCCESS);
    // auto SigOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    // EXPECT_EQ(testRun<Common::ArrayOutputLen>({SigOutputHandle, 0}),.value()
    //           __WASI_CRYPTO_ERRNO_SUCCESS);
    // EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
    // EXPECT_EQ(testRun<Common::ArrayOutputPull>({SigOutputHandle, 0, 64,
    // 64}),.value()
    //           __WASI_CRYPTO_ERRNO_SUCCESS);
    // EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(64), 64);
    // std::vector<uint8_t> SigOutput{MemInst.getPointer<uint8_t *>(0),
    //                                MemInst.getPointer<uint8_t *>(0) + 64};
    // EXPECT_EQ(SigOutput, SigRaw);

    EXPECT_EQ(testRun<Signatures::VerificationStateOpen>({PkHandle, 0}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const VerificationStateHandle =
        *MemInst.getPointer<__wasi_signature_verification_state_t *>(0);

    writeSpan("sample"_u8, 0);
    EXPECT_EQ(testRun<Signatures::VerificationStateUpdate>(
                  {VerificationStateHandle, 0, 6})
                  .value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Signatures::VerificationStateVerify>(
                  {VerificationStateHandle, SigHandle})
                  .value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(
        testRun<Signatures::VerificationStateClose>({VerificationStateHandle})
            .value(),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::StateClose>({StateHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KpHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::PublickeyClose>({PkHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::Close>({SigHandle}).value(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // import and export check
  {
    auto EncodingCheckPk = [this](std::vector<uint8_t> ExpectedPk,
                                  __wasi_publickey_encoding_e_t Encoding) {
      writeString("ECDSA_P256_SHA256", 0);
      writeSpan(ExpectedPk, 17);
      EXPECT_EQ(testRun<AsymmetricCommon::PublickeyImport>(
                    {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                     17, 17, ExpectedPk.size(), static_cast<uint32_t>(Encoding),
                     17 + ExpectedPk.size()})
                    .value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto PkHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(
          17 + ExpectedPk.size());

      EXPECT_EQ(testRun<AsymmetricCommon::PublickeyExport>(
                    {PkHandle, static_cast<uint32_t>(Encoding), 0})
                    .value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto PkOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
      EXPECT_EQ(testRun<Common::ArrayOutputLen>({PkOutputHandle, 0}).value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), ExpectedPk.size());
      EXPECT_EQ(testRun<Common::ArrayOutputPull>(
                    {PkOutputHandle, 0, ExpectedPk.size(), ExpectedPk.size()})
                    .value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(ExpectedPk.size()),
                ExpectedPk.size());
      EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0),
                                      MemInst.getPointer<uint8_t *>(0) +
                                          ExpectedPk.size()}),
                ExpectedPk);
    };
    EncodingCheckPk(UnCompressedPk, __WASI_PUBLICKEY_ENCODING_SEC);
    EncodingCheckPk(CompressedPk, __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC);
    EncodingCheckPk(Pcks8UncompressedPk, __WASI_PUBLICKEY_ENCODING_PKCS8);
    // EncodingCheckPk(Pcks8CompressedPk,
    //                 __WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8);
    EncodingCheckPk(PemUncompressedPk, __WASI_PUBLICKEY_ENCODING_PEM);
    // EncodingCheckPk(PemCompressedPk,
    // __WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM);

    auto EncodingCheckSk = [this](std::vector<uint8_t> ExpectedSk,
                                  __wasi_secretkey_encoding_e_t Encoding) {
      writeString("ECDSA_P256_SHA256", 0);
      writeSpan(ExpectedSk, 17);
      EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyImport>(
                    {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                     17, 17, ExpectedSk.size(), static_cast<uint32_t>(Encoding),
                     17 + ExpectedSk.size()})
                    .value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto PkHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(
          17 + ExpectedSk.size());

      EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyExport>(
                    {PkHandle, static_cast<uint32_t>(Encoding), 0})
                    .value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto PkOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
      EXPECT_EQ(testRun<Common::ArrayOutputLen>({PkOutputHandle, 0}).value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), ExpectedSk.size());

      EXPECT_EQ(testRun<Common::ArrayOutputPull>(
                    {PkOutputHandle, 0, ExpectedSk.size(), ExpectedSk.size()})
                    .value(),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(ExpectedSk.size()),
                ExpectedSk.size());
      EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0),
                                      MemInst.getPointer<uint8_t *>(0) +
                                          ExpectedSk.size()}),
                ExpectedSk);
    };
    EncodingCheckSk(Sk, __WASI_SECRETKEY_ENCODING_RAW);
    // EncodingCheckSk(Pcks8Sk, __WASI_SECRETKEY_ENCODING_PKCS8);
    EncodingCheckSk(PemSk, __WASI_SECRETKEY_ENCODING_PEM);
    //   // other encoding now not support
    //   std::vector<__wasi_publickey_encoding_e_t> UnsupportedPkEncoding{
    //       __WASI_PUBLICKEY_ENCODING_PKCS8,
    //       __WASI_PUBLICKEY_ENCODING_PEM,
    //       __WASI_PUBLICKEY_ENCODING_SEC,
    //       __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC,
    //       __WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8,
    //       __WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM,
    //       __WASI_PUBLICKEY_ENCODING_LOCAL};
    //   for (auto Encoding : UnsupportedPkEncoding) {
    //     writeString("Ed25519", 0);
    //     EXPECT_EQ(testRun<AsymmetricCommon::PublickeyImport>(.value()
    //                   {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES),
    //                   0,
    //                    7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //     EXPECT_EQ(testRun<AsymmetricCommon::PublickeyExport>(.value()
    //                   {PkHandle, static_cast<uint32_t>(Encoding), 0}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //   }

    //   EXPECT_EQ(testRun<AsymmetricCommon::PublickeyClose>({PkHandle}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);

    //   writeString("Ed25519", 0);
    //   writeSpan(SkRaw, 7);
    //   EXPECT_EQ(
    //       testRun<AsymmetricCommon::SecretkeyImport>(.value()
    //           {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7,
    //           7,
    //            32, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW),
    //            39}),
    //       __WASI_CRYPTO_ERRNO_SUCCESS);
    //   auto SkHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(39);

    //   EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyExport>(.value()
    //                 {SkHandle,
    //                  static_cast<uint32_t>(__WASI_SECRETKEY_ENCODING_RAW),
    //                  0}),
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   auto SkOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    //   EXPECT_EQ(testRun<Common::ArrayOutputLen>({SkOutputHandle, 0}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    //   EXPECT_EQ(testRun<Common::ArrayOutputPull>({SkOutputHandle, 0,
    //   32,.value() 32}),
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    //   EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0),
    //                                   MemInst.getPointer<uint8_t *>(0) +
    //                                   32}),
    //             SkRaw);

    //   // other encoding now not support
    //   std::vector<__wasi_secretkey_encoding_e_t> UnsupportedSkEncoding{
    //       __WASI_SECRETKEY_ENCODING_PKCS8, __WASI_SECRETKEY_ENCODING_PEM,
    //       __WASI_SECRETKEY_ENCODING_SEC, __WASI_SECRETKEY_ENCODING_LOCAL};
    //   for (auto Encoding : UnsupportedSkEncoding) {
    //     writeString("Ed25519", 0);
    //     EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyImport>(.value()
    //                   {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES),
    //                   0,
    //                    7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //     EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyExport>(.value()
    //                   {SkHandle, static_cast<uint32_t>(Encoding), 0}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //   }
    //   EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyClose>({SkHandle}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);

    //   writeString("Ed25519", 0);
    //   writeSpan(KpRaw, 7);
    //   EXPECT_EQ(
    //       testRun<AsymmetricCommon::KeypairImport>(.value()
    //           {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7,
    //           7,
    //            64, static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW), 71}),
    //       __WASI_CRYPTO_ERRNO_SUCCESS);
    //   auto KpHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(71);
    //   EXPECT_EQ(
    //       testRun<AsymmetricCommon::KeypairExport>(.value()
    //           {KpHandle, static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW),
    //           0}),
    //       __WASI_CRYPTO_ERRNO_SUCCESS);
    //   auto KpOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    //   EXPECT_EQ(testRun<Common::ArrayOutputLen>({KpOutputHandle, 0}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
    //   EXPECT_EQ(testRun<Common::ArrayOutputPull>({KpOutputHandle, 0,
    //   64,.value() 64}),
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(64), 64);
    //   std::vector<uint8_t> KpOutput{MemInst.getPointer<uint8_t *>(0),
    //                                 MemInst.getPointer<uint8_t *>(0) + 64};
    //   EXPECT_EQ(KpOutput, KpRaw);

    //   // other encoding now not support
    //   std::vector<__wasi_keypair_encoding_e_t> UnsupportedKpEncoding{
    //       __WASI_KEYPAIR_ENCODING_PKCS8, __WASI_KEYPAIR_ENCODING_PEM,
    //       __WASI_KEYPAIR_ENCODING_COMPRESSED_PKCS8,
    //       __WASI_KEYPAIR_ENCODING_COMPRESSED_PEM,
    //       __WASI_KEYPAIR_ENCODING_LOCAL};
    //   for (auto Encoding : UnsupportedKpEncoding) {
    //     writeString("Ed25519", 0);
    //     EXPECT_EQ(testRun<AsymmetricCommon::KeypairImport>(.value()
    //                   {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES),
    //                   0,
    //                    7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //     EXPECT_EQ(testRun<AsymmetricCommon::KeypairExport>(.value()
    //                   {KpHandle, static_cast<uint32_t>(Encoding), 0}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //   }
    //   EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KpHandle}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);

    //   // signature
    //   writeString("Ed25519", 0);
    //   writeSpan(SigRaw, 7);
    //   EXPECT_EQ(testRun<Signatures::Import>(.value()
    //                 {0, 7, 7, 64,
    //                  static_cast<uint32_t>(__WASI_SIGNATURE_ENCODING_RAW),
    //                  71}),
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   auto SigHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(71);

    //   EXPECT_EQ(testRun<Signatures::Export>(.value()
    //                 {SigHandle,
    //                  static_cast<uint32_t>(__WASI_SIGNATURE_ENCODING_RAW),
    //                  0}),
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   auto SigOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    //   EXPECT_EQ(testRun<Common::ArrayOutputLen>({SigOutputHandle,
    //   0}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
    //   EXPECT_EQ(testRun<Common::ArrayOutputPull>({SigOutputHandle, 0,
    //   64,.value() 64}),
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    //   EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(64), 64);
    //   std::vector<uint8_t> SigOutput{MemInst.getPointer<uint8_t *>(0),
    //                                  MemInst.getPointer<uint8_t *>(0) + 64};
    //   EXPECT_EQ(SigOutput, SigRaw);

    //   // unsupport sig encoding
    //   std::vector<__wasi_signature_encoding_e_t> UnsupportedSigEncoding{
    //       __WASI_SIGNATURE_ENCODING_DER};
    //   for (auto Encoding : UnsupportedSigEncoding) {
    //     writeString("Ed25519", 0);
    //     EXPECT_EQ(testRun<Signatures::Import>(.value()
    //                   {0, 7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //     EXPECT_EQ(testRun<Signatures::Export>(.value()
    //                   {SigHandle, static_cast<uint32_t>(Encoding), 0}),
    //               __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    //   }
    //   EXPECT_EQ(testRun<Signatures::Close>({SigHandle}),.value()
    //             __WASI_CRYPTO_ERRNO_SUCCESS);
    // }
    // }
  }
}