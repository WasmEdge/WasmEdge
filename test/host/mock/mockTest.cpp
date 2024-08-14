// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "host/mock/wasi_crypto_module.h"
#include "host/mock/wasi_logging_module.h"
#include "host/mock/wasi_nn_module.h"
#include "host/mock/wasmedge_image_module.h"
#include "host/mock/wasmedge_process_module.h"
#include "host/mock/wasmedge_tensorflow_module.h"
#include "host/mock/wasmedge_tensorflowlite_module.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

TEST(HostMockTest, WasiCrypto) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno;

  {
    WasmEdge::Host::WasiCryptoMock::Common::ArrayOutputLen
        WasiCryptoCommonArrayOutputLen;
    WasmEdge::Host::WasiCryptoMock::Common::ArrayOutputPull
        WasiCryptoCommonArrayOutputPull;
    WasmEdge::Host::WasiCryptoMock::Common::OptionsOpen
        WasiCryptoCommonOptionsOpen;
    WasmEdge::Host::WasiCryptoMock::Common::OptionsClose
        WasiCryptoCommonOptionsClose;
    WasmEdge::Host::WasiCryptoMock::Common::OptionsSet
        WasiCryptoCommonOptionsSet;
    WasmEdge::Host::WasiCryptoMock::Common::OptionsSetU64
        WasiCryptoCommonOptionsSetU64;
    WasmEdge::Host::WasiCryptoMock::Common::OptionsSetGuestBuffer
        WasiCryptoCommonOptionsSetGuestBuffer;
    WasmEdge::Host::WasiCryptoMock::Common::SecretsManagerOpen
        WasiCryptoCommonSecretsManagerOpen;
    WasmEdge::Host::WasiCryptoMock::Common::SecretsManagerClose
        WasiCryptoCommonSecretsManagerClose;
    WasmEdge::Host::WasiCryptoMock::Common::SecretsManagerInvalidate
        WasiCryptoCommonSecretsManagerInvalidate;

    EXPECT_TRUE(WasiCryptoCommonArrayOutputLen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonArrayOutputPull.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonOptionsOpen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonOptionsClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonOptionsSet.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonOptionsSetU64.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonOptionsSetGuestBuffer.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonSecretsManagerOpen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonSecretsManagerClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoCommonSecretsManagerInvalidate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

    WasmEdge::Host::WasiCryptoCommonModuleMock WasiCryptoCommonModule;
    EXPECT_EQ(WasiCryptoCommonModule.getModuleName(),
              "wasi_ephemeral_crypto_common");
    EXPECT_EQ(WasiCryptoCommonModule.getFuncExportNum(), 10U);
  }

  {
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairGenerate
        WasiCryptoAsymmetricCommonKeypairGenerate;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairImport
        WasiCryptoAsymmetricCommonKeypairImport;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairGenerateManaged
        WasiCryptoAsymmetricCommonKeypairGenerateManaged;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairStoreManaged
        WasiCryptoAsymmetricCommonKeypairStoreManaged;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairReplaceManaged
        WasiCryptoAsymmetricCommonKeypairReplaceManaged;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairId
        WasiCryptoAsymmetricCommonKeypairId;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairFromId
        WasiCryptoAsymmetricCommonKeypairFromId;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairFromPkAndSk
        WasiCryptoAsymmetricCommonKeypairFromPkAndSk;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairExport
        WasiCryptoAsymmetricCommonKeypairExport;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairPublickey
        WasiCryptoAsymmetricCommonKeypairPublickey;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairSecretkey
        WasiCryptoAsymmetricCommonKeypairSecretkey;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::KeypairClose
        WasiCryptoAsymmetricCommonKeypairClose;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::PublickeyImport
        WasiCryptoAsymmetricCommonPublickeyImport;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::PublickeyExport
        WasiCryptoAsymmetricCommonPublickeyExport;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::PublickeyVerify
        WasiCryptoAsymmetricCommonPublickeyVerify;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::PublickeyFromSecretkey
        WasiCryptoAsymmetricCommonPublickeyFromSecretkey;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::PublickeyClose
        WasiCryptoAsymmetricCommonPublickeyClose;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::SecretkeyImport
        WasiCryptoAsymmetricCommonSecretkeyImport;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::SecretkeyExport
        WasiCryptoAsymmetricCommonSecretkeyExport;
    WasmEdge::Host::WasiCryptoMock::AsymmetricCommon::SecretkeyClose
        WasiCryptoAsymmetricCommonSecretkeyClose;

    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairGenerate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairImport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 7>{0, 0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairGenerateManaged.run(
        CallFrame, std::array<WasmEdge::ValVariant, 6>{0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairStoreManaged.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairReplaceManaged.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairId.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairFromId.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairFromPkAndSk.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairExport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairPublickey.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairSecretkey.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonKeypairClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonPublickeyImport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 7>{0, 0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonPublickeyExport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonPublickeyVerify.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonPublickeyFromSecretkey.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonPublickeyClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonSecretkeyImport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 7>{0, 0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonSecretkeyExport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoAsymmetricCommonSecretkeyClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

    WasmEdge::Host::WasiCryptoAsymmetricCommonModuleMock
        WasiCryptoAsymmetricCommonModule;
    EXPECT_EQ(WasiCryptoAsymmetricCommonModule.getModuleName(),
              "wasi_ephemeral_crypto_asymmetric_common");
    EXPECT_EQ(WasiCryptoAsymmetricCommonModule.getFuncExportNum(), 20U);
  }

  {
    WasmEdge::Host::WasiCryptoMock::Kx::Dh WasiCryptoKxDh;
    WasmEdge::Host::WasiCryptoMock::Kx::Encapsulate WasiCryptoKxEncapsulate;
    WasmEdge::Host::WasiCryptoMock::Kx::Decapsulate WasiCryptoKxDecapsulate;

    EXPECT_TRUE(WasiCryptoKxDh.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoKxEncapsulate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoKxDecapsulate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

    WasmEdge::Host::WasiCryptoKxModuleMock WasiCryptoKxModule;
    EXPECT_EQ(WasiCryptoKxModule.getModuleName(), "wasi_ephemeral_crypto_kx");
    EXPECT_EQ(WasiCryptoKxModule.getFuncExportNum(), 3U);
  }

  {
    WasmEdge::Host::WasiCryptoMock::Signatures::Export
        WasiCryptoSignaturesExport;
    WasmEdge::Host::WasiCryptoMock::Signatures::Import
        WasiCryptoSignaturesImport;
    WasmEdge::Host::WasiCryptoMock::Signatures::StateOpen
        WasiCryptoSignaturesStateOpen;
    WasmEdge::Host::WasiCryptoMock::Signatures::StateUpdate
        WasiCryptoSignaturesStateUpdate;
    WasmEdge::Host::WasiCryptoMock::Signatures::StateSign
        WasiCryptoSignaturesStateSign;
    WasmEdge::Host::WasiCryptoMock::Signatures::StateClose
        WasiCryptoSignaturesStateClose;
    WasmEdge::Host::WasiCryptoMock::Signatures::VerificationStateOpen
        WasiCryptoSignaturesVerificationStateOpen;
    WasmEdge::Host::WasiCryptoMock::Signatures::VerificationStateUpdate
        WasiCryptoSignaturesVerificationStateUpdate;
    WasmEdge::Host::WasiCryptoMock::Signatures::VerificationStateVerify
        WasiCryptoSignaturesVerificationStateVerify;
    WasmEdge::Host::WasiCryptoMock::Signatures::VerificationStateClose
        WasiCryptoSignaturesVerificationStateClose;
    WasmEdge::Host::WasiCryptoMock::Signatures::Close WasiCryptoSignaturesClose;

    EXPECT_TRUE(WasiCryptoSignaturesExport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesImport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 6>{0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesStateOpen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesStateUpdate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesStateSign.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesStateClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesVerificationStateOpen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesVerificationStateUpdate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesVerificationStateVerify.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesVerificationStateClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSignaturesClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

    WasmEdge::Host::WasiCryptoSignaturesModuleMock WasiCryptoSignaturesModule;
    EXPECT_EQ(WasiCryptoSignaturesModule.getModuleName(),
              "wasi_ephemeral_crypto_signatures");
    EXPECT_EQ(WasiCryptoSignaturesModule.getFuncExportNum(), 11U);
  }

  {
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyGenerate
        WasiCryptoSymmetricKeyGenerate;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyImport
        WasiCryptoSymmetricKeyImport;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyExport
        WasiCryptoSymmetricKeyExport;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyClose
        WasiCryptoSymmetricKeyClose;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyGenerateManaged
        WasiCryptoSymmetricKeyGenerateManaged;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyStoreManaged
        WasiCryptoSymmetricKeyStoreManaged;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyReplaceManaged
        WasiCryptoSymmetricKeyReplaceManaged;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyId WasiCryptoSymmetricKeyId;
    WasmEdge::Host::WasiCryptoMock::Symmetric::KeyFromId
        WasiCryptoSymmetricKeyFromId;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateOpen
        WasiCryptoSymmetricStateOpen;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateClone
        WasiCryptoSymmetricStateClone;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateOptionsGet
        WasiCryptoSymmetricStateOptionsGet;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateOptionsGetU64
        WasiCryptoSymmetricStateOptionsGetU64;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateClose
        WasiCryptoSymmetricStateClose;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateAbsorb
        WasiCryptoSymmetricStateAbsorb;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateSqueeze
        WasiCryptoSymmetricStateSqueeze;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateSqueezeTag
        WasiCryptoSymmetricStateSqueezeTag;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateSqueezeKey
        WasiCryptoSymmetricStateSqueezeKey;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateMaxTagLen
        WasiCryptoSymmetricStateMaxTagLen;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateEncrypt
        WasiCryptoSymmetricStateEncrypt;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateEncryptDetached
        WasiCryptoSymmetricStateEncryptDetached;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateDecrypt
        WasiCryptoSymmetricStateDecrypt;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateDecryptDetached
        WasiCryptoSymmetricStateDecryptDetached;
    WasmEdge::Host::WasiCryptoMock::Symmetric::StateRatchet
        WasiCryptoSymmetricStateRatchet;
    WasmEdge::Host::WasiCryptoMock::Symmetric::TagLen WasiCryptoSymmetricTagLen;
    WasmEdge::Host::WasiCryptoMock::Symmetric::TagPull
        WasiCryptoSymmetricTagPull;
    WasmEdge::Host::WasiCryptoMock::Symmetric::TagVerify
        WasiCryptoSymmetricTagVerify;
    WasmEdge::Host::WasiCryptoMock::Symmetric::TagClose
        WasiCryptoSymmetricTagClose;

    EXPECT_TRUE(WasiCryptoSymmetricKeyGenerate.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyImport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyExport.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyGenerateManaged.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyStoreManaged.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyReplaceManaged.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyId.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricKeyFromId.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateOpen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateClone.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateOptionsGet.run(
        CallFrame, std::array<WasmEdge::ValVariant, 6>{0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateOptionsGetU64.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateAbsorb.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateSqueeze.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateSqueezeTag.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateSqueezeKey.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateMaxTagLen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateEncrypt.run(
        CallFrame, std::array<WasmEdge::ValVariant, 6>{0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateEncryptDetached.run(
        CallFrame, std::array<WasmEdge::ValVariant, 6>{0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateDecrypt.run(
        CallFrame, std::array<WasmEdge::ValVariant, 6>{0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateDecryptDetached.run(
        CallFrame, std::array<WasmEdge::ValVariant, 8>{0, 0, 0, 0, 0, 0, 0, 0},
        Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricStateRatchet.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricTagLen.run(
        CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricTagPull.run(
        CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricTagVerify.run(
        CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
    EXPECT_TRUE(WasiCryptoSymmetricTagClose.run(
        CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
    EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

    WasmEdge::Host::WasiCryptoSymmetricModuleMock WasiCryptoSymmetricModule;
    EXPECT_EQ(WasiCryptoSymmetricModule.getModuleName(),
              "wasi_ephemeral_crypto_symmetric");
    EXPECT_EQ(WasiCryptoSymmetricModule.getFuncExportNum(), 28U);
  }
}

TEST(HostMockTest, WasiNN) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno;

  WasmEdge::Host::WasiNNMock::Load WasiNNLoad;
  WasmEdge::Host::WasiNNMock::InitExecCtx WasiNNInitExecCtx;
  WasmEdge::Host::WasiNNMock::SetInput WasiNNSetInput;
  WasmEdge::Host::WasiNNMock::GetOutput WasiNNGetOutput;
  WasmEdge::Host::WasiNNMock::Compute WasiNNCompute;

  EXPECT_TRUE(WasiNNLoad.run(
      CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(WasiNNInitExecCtx.run(
      CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(WasiNNSetInput.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(WasiNNGetOutput.run(
      CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(WasiNNCompute.run(CallFrame,
                                std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

  WasmEdge::Host::WasiNNModuleMock WasiNNModule;
  EXPECT_EQ(WasiNNModule.getModuleName(), "wasi_ephemeral_nn");
  EXPECT_EQ(WasiNNModule.getFuncExportNum(), 5U);
}

TEST(HostMockTest, WasmEdgeProcess) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno;

  WasmEdge::Host::WasmEdgeProcessMock::SetProgName ProcessSetProgName;
  WasmEdge::Host::WasmEdgeProcessMock::AddArg ProcessAddArg;
  WasmEdge::Host::WasmEdgeProcessMock::AddEnv ProcessAddEnv;
  WasmEdge::Host::WasmEdgeProcessMock::AddStdIn ProcessAddStdIn;
  WasmEdge::Host::WasmEdgeProcessMock::SetTimeOut ProcessSetTimeOut;
  WasmEdge::Host::WasmEdgeProcessMock::Run ProcessRun;
  WasmEdge::Host::WasmEdgeProcessMock::GetExitCode ProcessGetExitCode;
  WasmEdge::Host::WasmEdgeProcessMock::GetStdOutLen ProcessGetStdOutLen;
  WasmEdge::Host::WasmEdgeProcessMock::GetStdOut ProcessGetStdOut;
  WasmEdge::Host::WasmEdgeProcessMock::GetStdErrLen ProcessGetStdErrLen;
  WasmEdge::Host::WasmEdgeProcessMock::GetStdErr ProcessGetStdErr;

  EXPECT_FALSE(ProcessSetProgName.run(
      CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, {}));
  EXPECT_FALSE(ProcessAddArg.run(
      CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, {}));
  EXPECT_FALSE(ProcessAddEnv.run(
      CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, {}));
  EXPECT_FALSE(ProcessAddStdIn.run(
      CallFrame, std::array<WasmEdge::ValVariant, 2>{0, 0}, {}));
  EXPECT_FALSE(ProcessSetTimeOut.run(
      CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, {}));
  EXPECT_FALSE(ProcessRun.run(CallFrame, {}, Errno));
  EXPECT_FALSE(ProcessGetExitCode.run(CallFrame, {}, Errno));
  EXPECT_FALSE(ProcessGetStdOutLen.run(CallFrame, {}, Errno));
  EXPECT_FALSE(ProcessGetStdOut.run(
      CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, {}));
  EXPECT_FALSE(ProcessGetStdErrLen.run(CallFrame, {}, Errno));
  EXPECT_FALSE(ProcessGetStdErr.run(
      CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, {}));

  WasmEdge::Host::WasmEdgeProcessModuleMock ProcessModule;
  EXPECT_EQ(ProcessModule.getModuleName(), "wasmedge_process");
  EXPECT_EQ(ProcessModule.getFuncExportNum(), 11U);
}

TEST(HostMockTest, WasiLogging) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiLoggingMock::Log WasiLoggingLog;

  EXPECT_FALSE(WasiLoggingLog.run(
      CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, {}));

  WasmEdge::Host::WasiLoggingModuleMock WasiLoggingModule;
  EXPECT_EQ(WasiLoggingModule.getModuleName(), "wasi:logging/logging");
  EXPECT_EQ(WasiLoggingModule.getFuncExportNum(), 1U);
}

TEST(HostMockTest, WasmEdgeTensorflow) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno;

  WasmEdge::Host::WasmEdgeTensorflowMock::CreateSession TFCreateSession;
  WasmEdge::Host::WasmEdgeTensorflowMock::DeleteSession TFDeleteSession;
  WasmEdge::Host::WasmEdgeTensorflowMock::RunSession TFRunSession;
  WasmEdge::Host::WasmEdgeTensorflowMock::GetOutputTensor TFGetOutputTensor;
  WasmEdge::Host::WasmEdgeTensorflowMock::GetTensorLen TFGetTensorLen;
  WasmEdge::Host::WasmEdgeTensorflowMock::GetTensorData TFGetTensorData;
  WasmEdge::Host::WasmEdgeTensorflowMock::AppendInput TFAppendInput;
  WasmEdge::Host::WasmEdgeTensorflowMock::AppendOutput TFAppendOutput;
  WasmEdge::Host::WasmEdgeTensorflowMock::ClearInput TFClearInput;
  WasmEdge::Host::WasmEdgeTensorflowMock::ClearOutput TFClearOutput;

  EXPECT_TRUE(TFCreateSession.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFDeleteSession.run(
      CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFRunSession.run(CallFrame,
                               std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFGetOutputTensor.run(
      CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFGetTensorLen.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFGetTensorData.run(
      CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFAppendInput.run(
      CallFrame, std::array<WasmEdge::ValVariant, 8>{0, 0, 0, 0, 0, 0, 0, 0},
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFAppendOutput.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFClearInput.run(CallFrame,
                               std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFClearOutput.run(CallFrame,
                                std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

  WasmEdge::Host::WasmEdgeTensorflowModuleMock TensorflowModule;
  EXPECT_EQ(TensorflowModule.getModuleName(), "wasmedge_tensorflow");
  EXPECT_EQ(TensorflowModule.getFuncExportNum(), 10U);
}

TEST(HostMockTest, WasmEdgeTensorflowLite) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno;

  WasmEdge::Host::WasmEdgeTensorflowLiteMock::CreateSession TFLiteCreateSession;
  WasmEdge::Host::WasmEdgeTensorflowLiteMock::DeleteSession TFLiteDeleteSession;
  WasmEdge::Host::WasmEdgeTensorflowLiteMock::RunSession TFLiteRunSession;
  WasmEdge::Host::WasmEdgeTensorflowLiteMock::GetOutputTensor
      TFLiteGetOutputTensor;
  WasmEdge::Host::WasmEdgeTensorflowLiteMock::GetTensorLen TFLiteGetTensorLen;
  WasmEdge::Host::WasmEdgeTensorflowLiteMock::GetTensorData TFLiteGetTensorData;
  WasmEdge::Host::WasmEdgeTensorflowLiteMock::AppendInput TFLiteAppendInput;

  EXPECT_TRUE(TFLiteCreateSession.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFLiteDeleteSession.run(
      CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFLiteRunSession.run(
      CallFrame, std::array<WasmEdge::ValVariant, 1>{0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFLiteGetOutputTensor.run(
      CallFrame, std::array<WasmEdge::ValVariant, 4>{0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFLiteGetTensorLen.run(
      CallFrame, std::array<WasmEdge::ValVariant, 3>{0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFLiteGetTensorData.run(
      CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(TFLiteAppendInput.run(
      CallFrame, std::array<WasmEdge::ValVariant, 5>{0, 0, 0, 0, 0}, Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

  WasmEdge::Host::WasmEdgeTensorflowLiteModuleMock TensorflowLiteModule;
  EXPECT_EQ(TensorflowLiteModule.getModuleName(), "wasmedge_tensorflowlite");
  EXPECT_EQ(TensorflowLiteModule.getFuncExportNum(), 7U);
}

TEST(HostMockTest, WasmEdgeImage) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno;

  WasmEdge::Host::WasmEdgeImageMock::LoadJPG ImageLoadJPG;
  WasmEdge::Host::WasmEdgeImageMock::LoadPNG ImageLoadPNG;

  EXPECT_TRUE(ImageLoadJPG.run(
      CallFrame, std::array<WasmEdge::ValVariant, 7>{0, 0, 0, 0, 0, 0, 0},
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);
  EXPECT_TRUE(ImageLoadPNG.run(
      CallFrame, std::array<WasmEdge::ValVariant, 7>{0, 0, 0, 0, 0, 0, 0},
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), 1U);

  WasmEdge::Host::WasmEdgeImageModuleMock ImageModule;
  EXPECT_EQ(ImageModule.getModuleName(), "wasmedge_image");
  EXPECT_EQ(ImageModule.getFuncExportNum(), 2U);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
