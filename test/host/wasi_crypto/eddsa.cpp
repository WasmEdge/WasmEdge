// SPDX-License-Identifier: Apache-2.0

#include "helper.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/signature/func.h"
#include "wasi_crypto/api.hpp"
#include <cstdint>

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, Ed25519) {
  // use generate data sign and verfiy
  {
    writeString("Ed25519", 0);
    writeOptOptions(std::nullopt, 7);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairGenerate>(
                  {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                   7, 7, 15}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const KpHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(15);

    EXPECT_EQ(testRun<AsymmetricCommon::KeypairPublickey>({KpHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const PkHandle =
        *MemInst.getPointer<__wasi_signature_publickey_t *>(0);

    EXPECT_EQ(testRun<Signatures::StateOpen>({KpHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const StateHandle = *MemInst.getPointer<__wasi_signature_state_t *>(0);

    writeSpan("test"_u8, 0);
    EXPECT_EQ(testRun<Signatures::StateUpdate>({StateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::StateUpdate>({StateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Signatures::StateSign>({StateHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const SignatureHandle = *MemInst.getPointer<__wasi_signature_t *>(0);

    EXPECT_EQ(testRun<Signatures::VerificationStateOpen>({PkHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const VerificationStateHandle =
        *MemInst.getPointer<__wasi_signature_verification_state_t *>(0);

    writeSpan("test"_u8, 0);
    EXPECT_EQ(testRun<Signatures::VerificationStateUpdate>(
                  {VerificationStateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::VerificationStateUpdate>(
                  {VerificationStateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Signatures::VerificationStateVerify>(
                  {VerificationStateHandle, SignatureHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(
        testRun<Signatures::VerificationStateClose>({VerificationStateHandle}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KpHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::PublickeyClose>({PkHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::Close>({SignatureHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // TEST data from https://datatracker.ietf.org/doc/html/rfc8032#section-7.1
  auto PkRaw =
      "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"_u8v;
  auto SkRaw =
      "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"_u8v;
  // sk concat pk
  auto KpRaw =
      "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"_u8v;
  auto SigRaw =
      "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"_u8v;

  // use import data generate and verify
  {
    writeString("Ed25519", 0);
    writeOptOptions(std::nullopt, 7);
    writeSpan(PkRaw, 7);
    EXPECT_EQ(
        testRun<AsymmetricCommon::PublickeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7, 7,
             32, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 39}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PkHandle = *MemInst.getPointer<__wasi_signature_publickey_t *>(39);

    writeString("Ed25519", 0);
    writeSpan(KpRaw, 7);
    EXPECT_EQ(
        testRun<AsymmetricCommon::KeypairImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7, 7,
             64, static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW), 71}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KpHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(71);

    EXPECT_EQ(testRun<Signatures::StateOpen>({KpHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const StateHandle = *MemInst.getPointer<__wasi_signature_state_t *>(0);

    EXPECT_EQ(testRun<Signatures::StateSign>({StateHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const SignatureHandle = *MemInst.getPointer<__wasi_signature_t *>(0);
    EXPECT_EQ(testRun<Signatures::Export>(
                  {SignatureHandle,
                   static_cast<uint32_t>(__WASI_SIGNATURE_ENCODING_RAW), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const SignatureOutputHandle =
        *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SignatureOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_array_output_t *>(0), 64);
    EXPECT_EQ(
        testRun<Common::ArrayOutputPull>({SignatureOutputHandle, 0, 64, 64}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_array_output_t *>(64), 64);
    EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 64),
                                    MemInst.getPointer<uint8_t *>(0) + 64}),
              SigRaw);

    EXPECT_EQ(testRun<Signatures::VerificationStateOpen>({PkHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto const VerificationStateHandle =
        *MemInst.getPointer<__wasi_signature_keypair_t *>(0);

    EXPECT_EQ(testRun<Signatures::VerificationStateVerify>(
                  {VerificationStateHandle, SignatureHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(
        testRun<Signatures::VerificationStateClose>({VerificationStateHandle}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KpHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<AsymmetricCommon::PublickeyClose>({PkHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Signatures::Close>({SignatureHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // import and export check
  {
    writeString("Ed25519", 0);
    writeSpan(PkRaw, 7);
    EXPECT_EQ(
        testRun<AsymmetricCommon::PublickeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7, 7,
             32, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 39}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PkHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(39);

    EXPECT_EQ(testRun<AsymmetricCommon::PublickeyExport>(
                  {PkHandle,
                   static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PkOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({PkOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({PkOutputHandle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0),
                                    MemInst.getPointer<uint8_t *>(0) + 32}),
              PkRaw);

    // other encoding now not support
    std::vector<__wasi_publickey_encoding_e_t> UnsupportedPkEncoding{
        __WASI_PUBLICKEY_ENCODING_PKCS8,
        __WASI_PUBLICKEY_ENCODING_PEM,
        __WASI_PUBLICKEY_ENCODING_SEC,
        __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC,
        __WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8,
        __WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM,
        __WASI_PUBLICKEY_ENCODING_LOCAL};
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

    EXPECT_EQ(testRun<AsymmetricCommon::PublickeyClose>({PkHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("Ed25519", 0);
    writeSpan(SkRaw, 7);
    EXPECT_EQ(
        testRun<AsymmetricCommon::SecretkeyImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7, 7,
             32, static_cast<uint32_t>(__WASI_PUBLICKEY_ENCODING_RAW), 39}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SkHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(39);

    EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyExport>(
                  {SkHandle,
                   static_cast<uint32_t>(__WASI_SECRETKEY_ENCODING_RAW), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SkOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SkOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({SkOutputHandle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(32), 32);
    EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0),
                                    MemInst.getPointer<uint8_t *>(0) + 32}),
              SkRaw);

    // other encoding now not support
    std::vector<__wasi_secretkey_encoding_e_t> UnsupportedSkEncoding{
        __WASI_SECRETKEY_ENCODING_PKCS8, __WASI_SECRETKEY_ENCODING_PEM,
        __WASI_SECRETKEY_ENCODING_SEC, __WASI_SECRETKEY_ENCODING_LOCAL};
    for (auto Encoding : UnsupportedSkEncoding) {
      writeString("Ed25519", 0);
      EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyImport>(
                    {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                     7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyExport>(
                    {SkHandle, static_cast<uint32_t>(Encoding), 0}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    }
    EXPECT_EQ(testRun<AsymmetricCommon::SecretkeyClose>({SkHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("Ed25519", 0);
    writeSpan(KpRaw, 7);
    EXPECT_EQ(
        testRun<AsymmetricCommon::KeypairImport>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0, 7, 7,
             64, static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW), 71}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KpHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(71);
    EXPECT_EQ(
        testRun<AsymmetricCommon::KeypairExport>(
            {KpHandle, static_cast<uint32_t>(__WASI_KEYPAIR_ENCODING_RAW), 0}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KpOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({KpOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({KpOutputHandle, 0, 64, 64}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(64), 64);
    std::vector<uint8_t> KpOutput{MemInst.getPointer<uint8_t *>(0),
                                  MemInst.getPointer<uint8_t *>(0) + 64};
    EXPECT_EQ(KpOutput, KpRaw);

    // other encoding now not support
    std::vector<__wasi_keypair_encoding_e_t> UnsupportedKpEncoding{
        __WASI_KEYPAIR_ENCODING_PKCS8, __WASI_KEYPAIR_ENCODING_PEM,
        __WASI_KEYPAIR_ENCODING_COMPRESSED_PKCS8,
        __WASI_KEYPAIR_ENCODING_COMPRESSED_PEM, __WASI_KEYPAIR_ENCODING_LOCAL};
    for (auto Encoding : UnsupportedKpEncoding) {
      writeString("Ed25519", 0);
      EXPECT_EQ(testRun<AsymmetricCommon::KeypairImport>(
                    {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0,
                     7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      EXPECT_EQ(testRun<AsymmetricCommon::KeypairExport>(
                    {KpHandle, static_cast<uint32_t>(Encoding), 0}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    }
    EXPECT_EQ(testRun<AsymmetricCommon::KeypairClose>({KpHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    // signature
    writeString("Ed25519", 0);
    writeSpan(SigRaw, 7);
    EXPECT_EQ(testRun<Signatures::Import>(
                  {0, 7, 7, 64,
                   static_cast<uint32_t>(__WASI_SIGNATURE_ENCODING_RAW), 71}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SigHandle = *MemInst.getPointer<__wasi_signature_keypair_t *>(71);

    EXPECT_EQ(testRun<Signatures::Export>(
                  {SigHandle,
                   static_cast<uint32_t>(__WASI_SIGNATURE_ENCODING_RAW), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SigOutputHandle = *MemInst.getPointer<__wasi_array_output_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({SigOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
    EXPECT_EQ(testRun<Common::ArrayOutputPull>({SigOutputHandle, 0, 64, 64}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(64), 64);
    std::vector<uint8_t> SigOutput{MemInst.getPointer<uint8_t *>(0),
                                   MemInst.getPointer<uint8_t *>(0) + 64};
    EXPECT_EQ(SigOutput, SigRaw);

    // unsupport sig encoding
    std::vector<__wasi_signature_encoding_e_t> UnsupportedSigEncoding{
        __WASI_SIGNATURE_ENCODING_DER};
    for (auto Encoding : UnsupportedSigEncoding) {
      writeString("Ed25519", 0);
      EXPECT_EQ(testRun<Signatures::Import>(
                    {0, 7, 7, 32, static_cast<uint32_t>(Encoding), 39}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      EXPECT_EQ(testRun<Signatures::Export>(
                    {SigHandle, static_cast<uint32_t>(Encoding), 0}),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
    }
    EXPECT_EQ(testRun<Signatures::Close>({SigHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
}