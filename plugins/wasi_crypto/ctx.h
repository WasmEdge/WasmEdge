// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/ctx.h - Context class definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the wasi-crypto context.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "asymmetric_common/keypair.h"
#include "asymmetric_common/publickey.h"
#include "asymmetric_common/secretkey.h"
#include "common/array_output.h"
#include "common/options.h"
#include "kx/registered.h"
#include "signatures/registered.h"
#include "signatures/signatures.h"
#include "signatures/signstate.h"
#include "signatures/verificationstate.h"
#include "symmetric/key.h"
#include "symmetric/registered.h"
#include "symmetric/state.h"
#include "symmetric/tag.h"
#include "utils/error.h"
#include "utils/handles_manager.h"

#include "common/span.h"
#include "plugin/plugin.h"

#include <memory>
#include <mutex>
#include <shared_mutex>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

class Context {
public:
  // Singleton
  static std::shared_ptr<Context> getInstance() noexcept {
    std::unique_lock Lock(Mutex);
    std::shared_ptr<Context> CtxPtr = Instance.lock();
    if (!CtxPtr) {
      CtxPtr.reset(new Context());
      Instance = CtxPtr;
    }
    return CtxPtr;
  }

  Context(const Context &) = delete;
  void operator=(const Context &) = delete;

  // Common

  WasiCryptoExpect<size_t>
  arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) noexcept;

  WasiCryptoExpect<size_t>
  arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                  Span<uint8_t> Buf) noexcept;

  WasiCryptoExpect<__wasi_options_t>
  optionsOpen(__wasi_algorithm_type_e_t AlgType) noexcept;

  WasiCryptoExpect<void> optionsClose(__wasi_options_t OptionsHandle) noexcept;

  WasiCryptoExpect<void> optionsSet(__wasi_options_t OptionsHandle,
                                    std::string_view Name,
                                    Span<const uint8_t> Value) noexcept;

  WasiCryptoExpect<void> optionsSetU64(__wasi_options_t OptionsHandle,
                                       std::string_view Name,
                                       uint64_t Value) noexcept;

  WasiCryptoExpect<void> optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
                                               std::string_view Name,
                                               Span<uint8_t> Buf) noexcept;

  WasiCryptoExpect<__wasi_secrets_manager_t>
  secretsManagerOpen(__wasi_opt_options_t OptOptionsHandle) noexcept;

  WasiCryptoExpect<void>
  secretsManagerClose(__wasi_secrets_manager_t SecretsManagerHandle) noexcept;

  WasiCryptoExpect<void>
  secretsManagerInvalidate(__wasi_secrets_manager_t SecretsManagerHandle,
                           Span<const uint8_t> KeyId,
                           __wasi_version_t Version) noexcept;

  // Symmetric

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerate(Symmetric::Algorithm Alg,
                       __wasi_opt_options_t OptOptionsHandle) noexcept;

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyImport(Symmetric::Algorithm Alg,
                     Span<const uint8_t> Raw) noexcept;

  WasiCryptoExpect<__wasi_array_output_t>
  symmetricKeyExport(__wasi_symmetric_key_t KeyHandle) noexcept;

  WasiCryptoExpect<void>
  symmetricKeyClose(__wasi_symmetric_key_t KeyHandle) noexcept;

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerateManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                              Symmetric::Algorithm Alg,
                              __wasi_opt_options_t OptOptionsHandle) noexcept;

  WasiCryptoExpect<void>
  symmetricKeyStoreManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                           __wasi_symmetric_key_t KeyHandle,
                           Span<uint8_t> KeyId) noexcept;

  WasiCryptoExpect<__wasi_version_t>
  symmetricKeyReplaceManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                             __wasi_symmetric_key_t OldKeyHandle,
                             __wasi_symmetric_key_t NewKeyHandle) noexcept;

  WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
  symmetricKeyId(__wasi_symmetric_key_t KeyHandle,
                 Span<uint8_t> KeyId) noexcept;

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                     Span<uint8_t> KeyId, __wasi_version_t KeyVersion) noexcept;

  WasiCryptoExpect<__wasi_symmetric_state_t>
  symmetricStateOpen(Symmetric::Algorithm Alg,
                     __wasi_opt_symmetric_key_t OptKeyHandle,
                     __wasi_opt_options_t OptOptionsHandle) noexcept;

  WasiCryptoExpect<__wasi_symmetric_state_t>
  symmetricStateClone(__wasi_symmetric_state_t StateHandle) noexcept;

  WasiCryptoExpect<size_t>
  symmetricStateOptionsGet(__wasi_symmetric_state_t StateHandle,
                           std::string_view Name, Span<uint8_t> Value) noexcept;

  WasiCryptoExpect<uint64_t>
  symmetricStateOptionsGetU64(__wasi_symmetric_state_t StateHandle,
                              std::string_view Name) noexcept;

  WasiCryptoExpect<void>
  symmetricStateClose(__wasi_symmetric_state_t StateHandle) noexcept;

  WasiCryptoExpect<void>
  symmetricStateAbsorb(__wasi_symmetric_state_t StateHandle,
                       Span<const uint8_t> Data) noexcept;

  WasiCryptoExpect<void>
  symmetricStateSqueeze(__wasi_symmetric_state_t StateHandle,
                        Span<uint8_t> Out) noexcept;

  WasiCryptoExpect<__wasi_symmetric_tag_t>
  symmetricStateSqueezeTag(__wasi_symmetric_state_t StateHandle) noexcept;

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                           Symmetric::Algorithm Alg) noexcept;

  WasiCryptoExpect<size_t>
  symmetricStateMaxTagLen(__wasi_symmetric_state_t StateHandle) noexcept;

  WasiCryptoExpect<size_t>
  symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<const uint8_t> Data) noexcept;

  WasiCryptoExpect<__wasi_symmetric_tag_t>
  symmetricStateEncryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out,
                                Span<const uint8_t> Data) noexcept;

  WasiCryptoExpect<size_t>
  symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<const uint8_t> Data) noexcept;

  WasiCryptoExpect<size_t>
  symmetricStateDecryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<const uint8_t> Data,
                                Span<const uint8_t> RawTag) noexcept;

  WasiCryptoExpect<void>
  symmetricStateRatchet(__wasi_symmetric_state_t StateHandle) noexcept;

  WasiCryptoExpect<size_t>
  symmetricTagLen(__wasi_symmetric_tag_t TagHandle) noexcept;

  WasiCryptoExpect<size_t> symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                                            Span<uint8_t> Buf) noexcept;

  WasiCryptoExpect<void>
  symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                     Span<const uint8_t> RawTag) noexcept;

  WasiCryptoExpect<void>
  symmetricTagClose(__wasi_symmetric_tag_t TagHandle) noexcept;

  // Asymmetric_common

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerate(AsymmetricCommon::Algorithm Alg,
                  __wasi_opt_options_t OptOptionsHandle) noexcept;

  WasiCryptoExpect<__wasi_keypair_t>
  keypairImport(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
                __wasi_keypair_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerateManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                         AsymmetricCommon::Algorithm Alg,
                         __wasi_opt_options_t OptOptionsHandle) noexcept;

  WasiCryptoExpect<void>
  keypairStoreManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                      __wasi_keypair_t KpHandle, Span<uint8_t> KpId) noexcept;

  WasiCryptoExpect<__wasi_version_t>
  keypairReplaceManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                        __wasi_keypair_t OldKpHandle,
                        __wasi_keypair_t NewKpHandle) noexcept;

  WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
  keypairId(__wasi_keypair_t KpHandle, Span<uint8_t> KpId) noexcept;

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                Span<const uint8_t> KpId,
                __wasi_version_t KpIdVersion) noexcept;

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromPkAndSk(__wasi_publickey_t PkHandle,
                     __wasi_secretkey_t SkHandle) noexcept;

  WasiCryptoExpect<__wasi_array_output_t>
  keypairExport(__wasi_keypair_t KpHandle,
                __wasi_keypair_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<__wasi_publickey_t>
  keypairPublickey(__wasi_keypair_t KpHandle) noexcept;

  WasiCryptoExpect<__wasi_secretkey_t>
  keypairSecretkey(__wasi_keypair_t KpHandle) noexcept;

  WasiCryptoExpect<void> keypairClose(__wasi_keypair_t KpHandle) noexcept;

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyImport(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_publickey_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<__wasi_array_output_t>
  publickeyExport(__wasi_publickey_t PkHandle,
                  __wasi_publickey_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<void> publickeyVerify(__wasi_publickey_t PkHandle) noexcept;

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyFromSecretkey(__wasi_secretkey_t SkHandle) noexcept;

  WasiCryptoExpect<void> publickeyClose(__wasi_publickey_t PkHandle) noexcept;

  WasiCryptoExpect<__wasi_secretkey_t>
  secretkeyImport(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<__wasi_array_output_t>
  secretkeyExport(__wasi_secretkey_t SkHandle,
                  __wasi_secretkey_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<void> secretkeyClose(__wasi_secretkey_t SkHandle) noexcept;

  // Key_exchange

  WasiCryptoExpect<__wasi_array_output_t>
  kxDh(__wasi_kx_publickey_t PkHandle, __wasi_kx_secretkey_t SkHandle) noexcept;

  WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
  kxEncapsulate(__wasi_kx_publickey_t PkHandle) noexcept;

  WasiCryptoExpect<__wasi_array_output_t>
  kxDecapsulate(__wasi_kx_secretkey_t SkHandle,
                Span<const uint8_t> EncapsulatedSecret) noexcept;

  // Signature

  WasiCryptoExpect<__wasi_array_output_t>
  signatureExport(__wasi_signature_t SigHandle,
                  __wasi_signature_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<__wasi_signature_t>
  signatureImport(Signatures::Algorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_signature_encoding_e_t Encoding) noexcept;

  WasiCryptoExpect<void> signatureClose(__wasi_signature_t SigHandle) noexcept;

  WasiCryptoExpect<__wasi_signature_state_t>
  signatureStateOpen(__wasi_signature_keypair_t KpHandle) noexcept;

  WasiCryptoExpect<void>
  signatureStateUpdate(__wasi_signature_state_t StateHandle,
                       Span<const uint8_t> Input) noexcept;

  WasiCryptoExpect<__wasi_signature_t>
  signatureStateSign(__wasi_signature_state_t StateHandle) noexcept;

  WasiCryptoExpect<void>
  signatureStateClose(__wasi_signature_state_t StateHandle) noexcept;

  WasiCryptoExpect<__wasi_signature_verification_state_t>
  signatureVerificationStateOpen(
      __wasi_signature_publickey_t PkHandle) noexcept;

  WasiCryptoExpect<void> signatureVerificationStateUpdate(
      __wasi_signature_verification_state_t StateHandle,
      Span<const uint8_t> Input) noexcept;

  WasiCryptoExpect<void> signatureVerificationStateVerify(
      __wasi_signature_verification_state_t StateHandle,
      __wasi_signature_t SigHandle) noexcept;

  WasiCryptoExpect<void> signatureVerificationStateClose(
      __wasi_signature_verification_state_t StateHandle) noexcept;

private:
  Context() noexcept {}

  RefHandlesManager<__wasi_array_output_t, Common::ArrayOutput>
      ArrayOutputManager{0x00};
  RcHandlesManager<__wasi_options_t, Common::Options> OptionsManager{0x01};
  RefHandlesManager<__wasi_symmetric_tag_t, Symmetric::Tag> SymmetricTagManager{
      0xa};
  RcHandlesManager<__wasi_symmetric_key_t, Symmetric::KeyVariant>
      SymmetricKeyManager{0x09};
  RcHandlesManager<__wasi_symmetric_state_t, Symmetric::StateVariant>
      SymmetricStateManager{0x08};
  RcHandlesManager<__wasi_publickey_t, AsymmetricCommon::PkVariant>
      PublicKeyManager{0x03};
  RcHandlesManager<__wasi_secretkey_t, AsymmetricCommon::SkVariant>
      SecretKeyManager{0x04};
  RcHandlesManager<__wasi_keypair_t, AsymmetricCommon::KpVariant>
      KeyPairManager{0x05};
  RcHandlesManager<__wasi_signature_t, Signatures::SigVariant> SignatureManager{
      0x06};
  RcHandlesManager<__wasi_signature_state_t, Signatures::SignStateVariant>
      SignStateManager{0x07};
  RcHandlesManager<__wasi_signature_state_t,
                   Signatures::VerificationStateVariant>
      VerificationStateManager{0x02};

  static std::shared_mutex Mutex;
  static std::weak_ptr<Context> Instance;
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
