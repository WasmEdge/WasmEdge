// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/mock/wasi_crypto_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiCryptoCommonModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasiCryptoCommonModuleMock()
      : Runtime::Instance::ModuleInstance("wasi_ephemeral_crypto_common") {
    using namespace WasiCryptoMock;
    addHostFunc("array_output_len", std::make_unique<Common::ArrayOutputLen>());
    addHostFunc("array_output_pull",
                std::make_unique<Common::ArrayOutputPull>());
    addHostFunc("options_open", std::make_unique<Common::OptionsOpen>());
    addHostFunc("options_close", std::make_unique<Common::OptionsClose>());
    addHostFunc("options_set", std::make_unique<Common::OptionsSet>());
    addHostFunc("options_set_u64", std::make_unique<Common::OptionsSetU64>());
    addHostFunc("options_set_guest_buffer",
                std::make_unique<Common::OptionsSetGuestBuffer>());
    addHostFunc("secrets_manager_open",
                std::make_unique<Common::SecretsManagerOpen>());
    addHostFunc("secrets_manager_close",
                std::make_unique<Common::SecretsManagerClose>());
    addHostFunc("secrets_manager_invalidate",
                std::make_unique<Common::SecretsManagerInvalidate>());
  }
};

class WasiCryptoAsymmetricCommonModuleMock
    : public Runtime::Instance::ModuleInstance {
public:
  WasiCryptoAsymmetricCommonModuleMock()
      : Runtime::Instance::ModuleInstance(
            "wasi_ephemeral_crypto_asymmetric_common") {
    using namespace WasiCryptoMock;
    addHostFunc("keypair_generate",
                std::make_unique<AsymmetricCommon::KeypairGenerate>());
    addHostFunc("keypair_import",
                std::make_unique<AsymmetricCommon::KeypairImport>());
    addHostFunc("keypair_generate_managed",
                std::make_unique<AsymmetricCommon::KeypairGenerateManaged>());
    addHostFunc("keypair_store_managed",
                std::make_unique<AsymmetricCommon::KeypairStoreManaged>());
    addHostFunc("keypair_replace_managed",
                std::make_unique<AsymmetricCommon::KeypairReplaceManaged>());
    addHostFunc("keypair_id", std::make_unique<AsymmetricCommon::KeypairId>());
    addHostFunc("keypair_from_id",
                std::make_unique<AsymmetricCommon::KeypairFromId>());
    addHostFunc("keypair_from_pk_and_sk",
                std::make_unique<AsymmetricCommon::KeypairFromPkAndSk>());
    addHostFunc("keypair_export",
                std::make_unique<AsymmetricCommon::KeypairExport>());
    addHostFunc("keypair_publickey",
                std::make_unique<AsymmetricCommon::KeypairPublickey>());
    addHostFunc("keypair_secretkey",
                std::make_unique<AsymmetricCommon::KeypairSecretkey>());
    addHostFunc("keypair_close",
                std::make_unique<AsymmetricCommon::KeypairClose>());
    addHostFunc("publickey_import",
                std::make_unique<AsymmetricCommon::PublickeyImport>());
    addHostFunc("publickey_export",
                std::make_unique<AsymmetricCommon::PublickeyExport>());
    addHostFunc("publickey_verify",
                std::make_unique<AsymmetricCommon::PublickeyVerify>());
    addHostFunc("publickey_from_secretkey",
                std::make_unique<AsymmetricCommon::PublickeyFromSecretkey>());
    addHostFunc("publickey_close",
                std::make_unique<AsymmetricCommon::PublickeyClose>());
    addHostFunc("secretkey_import",
                std::make_unique<AsymmetricCommon::SecretkeyImport>());
    addHostFunc("secretkey_export",
                std::make_unique<AsymmetricCommon::SecretkeyExport>());
    addHostFunc("secretkey_close",
                std::make_unique<AsymmetricCommon::SecretkeyClose>());
  }
};

class WasiCryptoKxModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasiCryptoKxModuleMock()
      : Runtime::Instance::ModuleInstance("wasi_ephemeral_crypto_kx") {
    using namespace WasiCryptoMock;
    addHostFunc("kx_dh", std::make_unique<Kx::Dh>());
    addHostFunc("kx_encapsulate", std::make_unique<Kx::Encapsulate>());
    addHostFunc("kx_decapsulate", std::make_unique<Kx::Decapsulate>());
  }
};

class WasiCryptoSignaturesModuleMock
    : public Runtime::Instance::ModuleInstance {
public:
  WasiCryptoSignaturesModuleMock()
      : Runtime::Instance::ModuleInstance("wasi_ephemeral_crypto_signatures") {
    using namespace WasiCryptoMock;
    addHostFunc("signature_export", std::make_unique<Signatures::Export>());
    addHostFunc("signature_import", std::make_unique<Signatures::Import>());
    addHostFunc("signature_state_open",
                std::make_unique<Signatures::StateOpen>());
    addHostFunc("signature_state_update",
                std::make_unique<Signatures::StateUpdate>());
    addHostFunc("signature_state_sign",
                std::make_unique<Signatures::StateSign>());
    addHostFunc("signature_state_close",
                std::make_unique<Signatures::StateClose>());
    addHostFunc("signature_verification_state_open",
                std::make_unique<Signatures::VerificationStateOpen>());
    addHostFunc("signature_verification_state_update",
                std::make_unique<Signatures::VerificationStateUpdate>());
    addHostFunc("signature_verification_state_verify",
                std::make_unique<Signatures::VerificationStateVerify>());
    addHostFunc("signature_verification_state_close",
                std::make_unique<Signatures::VerificationStateClose>());
    addHostFunc("signature_close", std::make_unique<Signatures::Close>());
  }
};

class WasiCryptoSymmetricModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasiCryptoSymmetricModuleMock()
      : Runtime::Instance::ModuleInstance("wasi_ephemeral_crypto_symmetric") {
    using namespace WasiCryptoMock;
    addHostFunc("symmetric_key_generate",
                std::make_unique<Symmetric::KeyGenerate>());
    addHostFunc("symmetric_key_import",
                std::make_unique<Symmetric::KeyImport>());
    addHostFunc("symmetric_key_export",
                std::make_unique<Symmetric::KeyExport>());
    addHostFunc("symmetric_key_close", std::make_unique<Symmetric::KeyClose>());
    addHostFunc("symmetric_key_generate_managed",
                std::make_unique<Symmetric::KeyGenerateManaged>());
    addHostFunc("symmetric_key_store_managed",
                std::make_unique<Symmetric::KeyStoreManaged>());
    addHostFunc("symmetric_key_replace_managed",
                std::make_unique<Symmetric::KeyReplaceManaged>());
    addHostFunc("symmetric_key_id", std::make_unique<Symmetric::KeyId>());
    addHostFunc("symmetric_key_from_id",
                std::make_unique<Symmetric::KeyFromId>());
    addHostFunc("symmetric_state_open",
                std::make_unique<Symmetric::StateOpen>());
    addHostFunc("symmetric_state_clone",
                std::make_unique<Symmetric::StateClone>());
    addHostFunc("symmetric_state_options_get",
                std::make_unique<Symmetric::StateOptionsGet>());
    addHostFunc("symmetric_state_options_get_u64",
                std::make_unique<Symmetric::StateOptionsGetU64>());
    addHostFunc("symmetric_state_close",
                std::make_unique<Symmetric::StateClose>());
    addHostFunc("symmetric_state_absorb",
                std::make_unique<Symmetric::StateAbsorb>());
    addHostFunc("symmetric_state_squeeze",
                std::make_unique<Symmetric::StateSqueeze>());
    addHostFunc("symmetric_state_squeeze_tag",
                std::make_unique<Symmetric::StateSqueezeTag>());
    addHostFunc("symmetric_state_squeeze_key",
                std::make_unique<Symmetric::StateSqueezeKey>());
    addHostFunc("symmetric_state_max_tag_len",
                std::make_unique<Symmetric::StateMaxTagLen>());
    addHostFunc("symmetric_state_encrypt",
                std::make_unique<Symmetric::StateEncrypt>());
    addHostFunc("symmetric_state_encrypt_detached",
                std::make_unique<Symmetric::StateEncryptDetached>());
    addHostFunc("symmetric_state_decrypt",
                std::make_unique<Symmetric::StateDecrypt>());
    addHostFunc("symmetric_state_decrypt_detached",
                std::make_unique<Symmetric::StateDecryptDetached>());
    addHostFunc("symmetric_state_ratchet",
                std::make_unique<Symmetric::StateRatchet>());
    addHostFunc("symmetric_tag_len", std::make_unique<Symmetric::TagLen>());
    addHostFunc("symmetric_tag_pull", std::make_unique<Symmetric::TagPull>());
    addHostFunc("symmetric_tag_verify",
                std::make_unique<Symmetric::TagVerify>());
    addHostFunc("symmetric_tag_close", std::make_unique<Symmetric::TagClose>());
  }
};

} // namespace Host
} // namespace WasmEdge
