// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/keypair.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/common/array_output.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/handles.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class WasiCryptoContext {
public:
  //-------------------------------------------common---------------------------------------

  WasiCryptoExpect<size_t>
  arrayOutputLen(__wasi_array_output_t ArrayOutputHandle);

  WasiCryptoExpect<size_t>
  arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                  Span<uint8_t> BufPtr);

  WasiCryptoExpect<__wasi_options_t>
  optionsOpen(__wasi_algorithm_type_e_t AlgorithmType);

  WasiCryptoExpect<void> optionsClose(__wasi_options_t Handle);

  WasiCryptoExpect<void> optionsSet(__wasi_options_t OptionsHandle,
                                    std::string_view Name,
                                    Span<uint8_t const> Value);

  WasiCryptoExpect<void> optionsSetU64(__wasi_options_t OptionsHandle,
                                       std::string_view Name, uint64_t Value);

  WasiCryptoExpect<void> optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
                                               std::string_view Name,
                                               Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_secrets_manager_t>
  secretsMangerOpen(std::optional<__wasi_options_t> Options);

  WasiCryptoExpect<void>
  secretsMangerClose(__wasi_secrets_manager_t SecretsManger);

  WasiCryptoExpect<void>
  secretsManagerInvalidate(__wasi_secrets_manager_t SecretsManger,
                           Span<uint8_t const> KeyId, __wasi_version_t Version);

  //-------------------------------------------symmetric---------------------------------------

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerate(SymmetricAlgorithm Alg,
                       std::optional<__wasi_options_t> OptionsHandle);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyImport(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  WasiCryptoExpect<__wasi_array_output_t>
  symmetricKeyExport(__wasi_symmetric_key_t KeyHandle);

  WasiCryptoExpect<void> symmetricKeyClose(__wasi_symmetric_key_t SymmetricKey);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerateManaged(__wasi_secrets_manager_t SecretsManager,
                              SymmetricAlgorithm Alg,
                              std::optional<__wasi_options_t> Options);

  WasiCryptoExpect<void>
  symmetricKeyStoreManaged(__wasi_secrets_manager_t SecretsManager,
                           __wasi_symmetric_key_t SymmetricKey,
                           uint8_t_ptr SymmetricKeyId,
                           size_t SymmetricKeyIdMaxLen);

  WasiCryptoExpect<__wasi_version_t>
  symmetricKeyReplaceManaged(__wasi_secrets_manager_t SecretsManager,
                             __wasi_symmetric_key_t SymmetricKeyOld,
                             __wasi_symmetric_key_t SymmetricKeyNew);

  WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
  symmetricKeyId(__wasi_symmetric_key_t SymmetricKey,
                 uint8_t_ptr SymmetricKeyId, size_t SymmetricKeyIdMaxLen);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyFromId(__wasi_secrets_manager_t SecretsManager,
                     Span<uint8_t> SymmetricKey,
                     __wasi_version_t SymmetricKeyVersion);

  WasiCryptoExpect<__wasi_symmetric_state_t>
  symmetricStateOpen(SymmetricAlgorithm Alg,
                     std::optional<__wasi_symmetric_key_t> KeyHandle,
                     std::optional<__wasi_options_t> OptionsHandle);

  WasiCryptoExpect<size_t>
  symmetricStateOptionsGet(__wasi_symmetric_state_t Handle,
                           std::string_view Name, Span<uint8_t> Value);

  WasiCryptoExpect<uint64_t>
  symmetricStateOptionsGetU64(__wasi_symmetric_state_t Handle,
                              std::string_view Name);

  WasiCryptoExpect<void> symmetricStateClose(__wasi_symmetric_state_t Handle);

  WasiCryptoExpect<void> symmetricStateAbsorb(__wasi_symmetric_state_t Handle,
                                              Span<uint8_t const> Data);

  WasiCryptoExpect<void> symmetricStateSqueeze(__wasi_symmetric_state_t Handle,
                                               Span<uint8_t> Out);

  WasiCryptoExpect<__wasi_symmetric_tag_t>
  symmetricStateSqueezeTag(__wasi_symmetric_state_t Handle);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                           SymmetricAlgorithm Alg);

  WasiCryptoExpect<size_t>
  symmetricStateMaxTagLen(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<size_t>
  symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<uint8_t const> Data);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricStateEncryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<uint8_t const> Data);

  WasiCryptoExpect<size_t>
  symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<uint8_t const> Data);

  WasiCryptoExpect<size_t>
  symmetricStateDecryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<uint8_t const> Data,
                                Span<uint8_t> RawTag);

  WasiCryptoExpect<void>
  symmetricStateRatchet(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<size_t> symmetricTagLen(__wasi_symmetric_tag_t TagHandle);

  WasiCryptoExpect<size_t> symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                                            Span<uint8_t> Buf);

  WasiCryptoExpect<void> symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                                            Span<uint8_t const> RawTag);

  WasiCryptoExpect<void> symmetricTagClose(__wasi_symmetric_tag_t TagHandle);

  //-------------------------------------------asymmetric_common---------------------------------------

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerate(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<const uint8_t> Encoded,
                __wasi_keypair_encoding_e_t Encoding);

  // opt
  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerateManaged(__wasi_secrets_manager_t SecretsManager,
                         __wasi_algorithm_type_e_t AlgType,
                         std::string_view AlgStr,
                         std::optional<__wasi_options_t> OptOptions);

  WasiCryptoExpect<void>
  keypairStoreManaged(__wasi_secrets_manager_t SecretsManager,
                      __wasi_keypair_t Keypair, uint8_t_ptr KpIdPtr,
                      size_t KpIdLen);

  WasiCryptoExpect<__wasi_version_t>
  keypairReplaceManaged(__wasi_secrets_manager_t SecretsManager,
                        __wasi_keypair_t KpOld, __wasi_keypair_t KpNew);

  WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
  keypairId(__wasi_keypair_t Kp, uint8_t_ptr KpId, size_t KpIdMaxLen);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromId(__wasi_secrets_manager_t SecretsManager, const_uint8_t_ptr KpId,
                size_t KpIdLen, __wasi_version_t KpIdVersion);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromPkAndSk(__wasi_publickey_t PkHandle, __wasi_secretkey_t SkHandle);
  WasiCryptoExpect<__wasi_array_output_t>
  keypairExport(__wasi_keypair_t KpHandle,
                __wasi_keypair_encoding_e_t KeypairEncoding);

  WasiCryptoExpect<__wasi_publickey_t>
  keypairPublickey(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<__wasi_secretkey_t>
  keypairSecretkey(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<void> keypairClose(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<const uint8_t> Encoded,
                  __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_array_output_t>
  publickeyExport(__wasi_publickey_t PkHandle,
                  __wasi_publickey_encoding_e_t PkEncoding);

  WasiCryptoExpect<void> publickeyVerify(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyFromSecretkey(__wasi_secretkey_t SkHandle);

  WasiCryptoExpect<void> publickeyClose(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_secretkey_t>
  secretkeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<const uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t EncodingEnum);

  WasiCryptoExpect<__wasi_array_output_t>
  secretkeyExport(__wasi_secretkey_t SkHandle,
                  __wasi_secretkey_encoding_e_t SkEncoding);

  WasiCryptoExpect<void> secretkeyClose(__wasi_secretkey_t SkHandle);

  //------------------------------------------key_exchange-------------------------------------

  WasiCryptoExpect<__wasi_array_output_t> kxDh(__wasi_publickey_t KxPkInner,
                                               __wasi_secretkey_t KxSkInner);

  WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
  kxEncapsulate(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_array_output_t>
  kxDecapsulate(__wasi_secretkey_t SkHandle,
                Span<uint8_t const> EncapsulatedSecret);

  //-------------------------------------------signature---------------------------------------

  WasiCryptoExpect<__wasi_array_output_t>
  signatureExport(__wasi_signature_t SigHandle,
                  __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_signature_t>
  signatureImport(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
                  __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<void> signatureClose(__wasi_signature_t SigHandle);

  WasiCryptoExpect<__wasi_signature_state_t>
  signatureStateOpen(__wasi_signature_keypair_t KpHandle);

  WasiCryptoExpect<void>
  signatureStateUpdate(__wasi_signature_state_t StateHandle,
                       Span<uint8_t const> Input);

  WasiCryptoExpect<__wasi_signature_t>
  signatureStateSign(__wasi_signature_state_t StateHandle);

  WasiCryptoExpect<void>
  signatureStateClose(__wasi_signature_state_t StateHandle);

  WasiCryptoExpect<__wasi_signature_verification_state_t>
  signatureVerificationStateOpen(__wasi_signature_publickey_t PkHandle);

  WasiCryptoExpect<void> signatureVerificationStateUpdate(
      __wasi_signature_verification_state_t VerificationHandle,
      Span<uint8_t const> Input);

  WasiCryptoExpect<void> signatureVerificationStateVerify(
      __wasi_signature_verification_state_t VerificationInner,
      __wasi_signature_t SigInner);

  WasiCryptoExpect<void> signatureVerificationStateClose(
      __wasi_signature_verification_state_t VerificationHandle);

private:
  WasiCryptoExpect<__wasi_array_output_t>
  allocateArrayOutput(std::vector<uint8_t> &&Data);

  WasiCryptoExpect<std::shared_ptr<Symmetric::Options>>
  readSymmetricOption(std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<std::shared_ptr<Symmetric::Key>>
  readSymmetricKey(std::optional<__wasi_symmetric_key_t> KeyHandle);

  HandlesManger<__wasi_array_output_t, Common::ArrayOutput> ArrayOutputManger{
      0x00};
  HandlesManger<__wasi_options_t, Common::Options> OptionsManger{0x01};
  HandlesManger<__wasi_keypair_t, Asymmetric::KeyPair> KeypairManger{0x02};
  HandlesManger<__wasi_publickey_t, Asymmetric::PublicKey> PublickeyManger{
      0x03};
  HandlesManger<__wasi_secretkey_t, Asymmetric::SecretKey> SecretkeyManger{
      0x04};
  HandlesManger<__wasi_signature_state_t,
                std::shared_ptr<Signatures::SignState>>
      SignatureStateManger{0x05};
  HandlesManger<__wasi_signature_t, std::shared_ptr<Signatures::Signature>>
      SignatureManger{0x06};
  HandlesManger<__wasi_signature_verification_state_t,
                std::shared_ptr<Signatures::VerificationState>>
      SignatureVerificationStateManger{0x07};
  HandlesManger<__wasi_symmetric_state_t, std::shared_ptr<Symmetric::State>>
      SymmetricStateManger{0x08};
  HandlesManger<__wasi_symmetric_key_t, std::shared_ptr<Symmetric::Key>>
      SymmetricKeyManger{0x09};
  HandlesManger<__wasi_symmetric_tag_t, std::shared_ptr<Symmetric::Tag>>
      SymmetricTagManger{0xa};
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
