// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "asymmetric_common/module.h"
#include "common/module.h"
#include "ctx.h"
#include "helper.h"
#include "kx/module.h"
#include "signatures/module.h"
#include "symmetric/module.h"
#include "utils/error.h"

#include "common/span.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <type_traits>
#include <utility>
#include <vector>

#define WASI_CRYPTO_EXPECT_SUCCESS(Expr, Function)                             \
  auto &&Expr##__Result = Function;                                            \
  EXPECT_TRUE(Expr##__Result)                                                  \
      << "Wasi Crypto Error code: " << Errno[0].get<int32_t>();                \
  auto &&Expr = Expr##__Result.value()

#define WASI_CRYPTO_EXPECT_FAILURE(Function, ErrorCode)                        \
  do {                                                                         \
    auto Result = Function;                                                    \
    EXPECT_FALSE(Result) << "The function result should be error but success"; \
    EXPECT_EQ(Result.error(), ErrorCode);                                      \
  } while (0)

#define WASI_CRYPTO_EXPECT_TRUE(Function)                                      \
  EXPECT_TRUE(Function) << "Wasi Crypto Error code: " << Errno[0].get<int32_t>()

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

std::vector<uint8_t> operator"" _u8(const char *Str, std::size_t Len);

std::vector<uint8_t> operator"" _u8v(const char *Str, std::size_t Len);

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

/// Designed for testing.
class WasiCryptoTest : public ::testing::Test {
public:
  WasiCryptoTest() : Mod(""), CallFrame(nullptr, &Mod) {
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(1)));
    MemInst = Mod.findMemoryExports("memory");

    using namespace std::literals::string_view_literals;
    Plugin::Plugin::load(std::filesystem::u8path(
        "../../../plugins/wasi_crypto/" WASMEDGE_LIB_PREFIX
        "wasmedgePluginWasiCrypto" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_crypto"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasi_crypto_asymmetric_common"sv)) {
        WasiCryptoAsymCommonMod = dynamicPointerCast<
            WasmEdge::Host::WasiCryptoAsymmetricCommonModule>(Module->create());
      }
      if (const auto *Module = Plugin->findModule("wasi_crypto_common"sv)) {
        WasiCryptoCommonMod =
            dynamicPointerCast<WasmEdge::Host::WasiCryptoCommonModule>(
                Module->create());
      }
      if (const auto *Module = Plugin->findModule("wasi_crypto_kx"sv)) {
        WasiCryptoKxMod =
            dynamicPointerCast<WasmEdge::Host::WasiCryptoKxModule>(
                Module->create());
      }
      if (const auto *Module = Plugin->findModule("wasi_crypto_signatures"sv)) {
        WasiCryptoSignMod =
            dynamicPointerCast<WasmEdge::Host::WasiCryptoSignaturesModule>(
                Module->create());
      }
      if (const auto *Module = Plugin->findModule("wasi_crypto_symmetric"sv)) {
        WasiCryptoSymmMod =
            dynamicPointerCast<WasmEdge::Host::WasiCryptoSymmetricModule>(
                Module->create());
      }
    }
  }

protected:
  void writeDummyMemoryContent();

  void writeString(std::string_view String, uint32_t Ptr);

  void writeSpan(Span<const uint8_t> Content, uint32_t Ptr);

  void writeOptKey(std::optional<int32_t> OptKey, uint32_t Ptr);

  void writeOptOptions(std::optional<__wasi_options_t> OptOptions,
                       uint32_t Ptr);

  // Common

  WasiCryptoExpect<__wasi_size_t>
  arrayOutputLen(__wasi_array_output_t ArrayOutputHandle);

  WasiCryptoExpect<__wasi_size_t>
  arrayOutputPull(__wasi_array_output_t ArrayOutputHandle, Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_options_t>
  optionsOpen(__wasi_algorithm_type_e_t AlgType);

  WasiCryptoExpect<void> optionsClose(__wasi_options_t OptionsHandle);

  WasiCryptoExpect<void> optionsSet(__wasi_options_t OptionsHandle,
                                    std::string_view Name,
                                    Span<const uint8_t> Value);

  WasiCryptoExpect<void> optionsSetU64(__wasi_options_t OptionsHandle,
                                       std::string_view Name, uint64_t Value);

  // Not supported, Buf placing must be on page.
  //   WasiCryptoExpect<void>
  //   optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
  //                         std::string_view Name, Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_secrets_manager_t>
  secretsManagerOpen(std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<void>
  secretsManagerClose(__wasi_secrets_manager_t SecretsManagerHandle);

  WasiCryptoExpect<void>
  secretsManagerInvalidate(__wasi_secrets_manager_t SecretsManagerHandle,
                           Span<const uint8_t> KeyId, __wasi_version_t Version);

  // Symmetric

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerate(std::string_view Alg,
                       std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyImport(std::string_view Alg, Span<const uint8_t> Raw);

  WasiCryptoExpect<__wasi_array_output_t>
  symmetricKeyExport(__wasi_symmetric_key_t KeyHandle);

  WasiCryptoExpect<void> symmetricKeyClose(__wasi_symmetric_key_t KeyHandle);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerateManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                              std::string_view Alg,
                              std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<void>
  symmetricKeyStoreManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                           __wasi_symmetric_key_t KeyHandle,
                           Span<uint8_t> KeyId);

  WasiCryptoExpect<__wasi_version_t>
  symmetricKeyReplaceManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                             __wasi_symmetric_key_t OldKeyHandle,
                             __wasi_symmetric_key_t NewKeyHandle);

  WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
  symmetricKeyId(__wasi_symmetric_key_t KeyHandle, Span<uint8_t> KeyId);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                     Span<uint8_t> KeyId, __wasi_version_t KeyVersion);

  WasiCryptoExpect<__wasi_symmetric_state_t>
  symmetricStateOpen(std::string_view Alg,
                     std::optional<__wasi_symmetric_key_t> OptKeyHandle,
                     std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<__wasi_size_t>
  symmetricStateOptionsGet(__wasi_symmetric_state_t StateHandle,
                           std::string_view Name, Span<uint8_t> Value);

  WasiCryptoExpect<uint64_t>
  symmetricStateOptionsGetU64(__wasi_symmetric_state_t StateHandle,
                              std::string_view Name);

  WasiCryptoExpect<void>
  symmetricStateClose(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<void>
  symmetricStateAbsorb(__wasi_symmetric_state_t StateHandle,
                       Span<const uint8_t> Data);

  WasiCryptoExpect<__wasi_symmetric_state_t>
  symmetricStateClone(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<void>
  symmetricStateSqueeze(__wasi_symmetric_state_t StateHandle,
                        Span<uint8_t> Out);

  WasiCryptoExpect<__wasi_symmetric_tag_t>
  symmetricStateSqueezeTag(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                           std::string_view Alg);

  WasiCryptoExpect<__wasi_size_t>
  symmetricStateMaxTagLen(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<__wasi_size_t>
  symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<const uint8_t> Data);

  WasiCryptoExpect<__wasi_symmetric_tag_t>
  symmetricStateEncryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<const uint8_t> Data);

  WasiCryptoExpect<__wasi_size_t>
  symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<const uint8_t> Data);

  WasiCryptoExpect<__wasi_size_t>
  symmetricStateDecryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<const uint8_t> Data,
                                Span<uint8_t> RawTag);

  WasiCryptoExpect<void>
  symmetricStateRatchet(__wasi_symmetric_state_t StateHandle);

  WasiCryptoExpect<__wasi_size_t>
  symmetricMaxTagLen(__wasi_symmetric_tag_t TagHandle);

  WasiCryptoExpect<__wasi_size_t>
  symmetricTagPull(__wasi_symmetric_tag_t TagHandle, Span<uint8_t> Buf);

  WasiCryptoExpect<void> symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                                            Span<const uint8_t> RawTag);

  WasiCryptoExpect<void> symmetricTagClose(__wasi_symmetric_tag_t TagHandle);

  // Asymmetric_common

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerate(__wasi_algorithm_type_e_t AlgType, std::string_view Alg,
                  std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairImport(__wasi_algorithm_type_e_t AlgType, std::string_view Alg,
                Span<const uint8_t> Encoded,
                __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerateManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                         __wasi_algorithm_type_e_t AlgType,
                         std::string_view Alg,
                         std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<void>
  keypairStoreManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                      __wasi_keypair_t KpHandle, Span<uint8_t> KpId);

  WasiCryptoExpect<__wasi_version_t>
  keypairReplaceManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                        __wasi_keypair_t OldKpHandle,
                        __wasi_keypair_t NewKpHandle);

  WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
  keypairId(__wasi_keypair_t KpHandle, Span<uint8_t> KpId);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                Span<const uint8_t> KpId, __wasi_version_t KpIdVersion);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromPkAndSk(__wasi_publickey_t PkHandle, __wasi_secretkey_t SkHandle);

  WasiCryptoExpect<__wasi_array_output_t>
  keypairExport(__wasi_keypair_t KpHandle,
                __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_publickey_t>
  keypairPublickey(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<__wasi_secretkey_t>
  keypairSecretkey(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<void> keypairClose(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view Alg,
                  Span<const uint8_t> Encoded,
                  __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_array_output_t>
  publickeyExport(__wasi_publickey_t PkHandle,
                  __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<void> publickeyVerify(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyFromSecretkey(__wasi_secretkey_t SkHandle);

  WasiCryptoExpect<void> publickeyClose(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_secretkey_t>
  secretkeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view Alg,
                  Span<const uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_array_output_t>
  secretkeyExport(__wasi_secretkey_t SkHandle,
                  __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<void> secretkeyClose(__wasi_secretkey_t SkHandle);

  // Key_exchange

  WasiCryptoExpect<__wasi_array_output_t> kxDh(__wasi_kx_publickey_t PkHandle,
                                               __wasi_kx_secretkey_t SkHandle);

  WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
  kxEncapsulate(__wasi_kx_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_array_output_t>
  kxDecapsulate(__wasi_kx_secretkey_t SkHandle,
                Span<const uint8_t> EncapsulatedSecret);

  // Signature

  WasiCryptoExpect<__wasi_array_output_t>
  signatureExport(__wasi_signature_t SigHandle,
                  __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_signature_t>
  signatureImport(std::string_view Alg, Span<const uint8_t> Encoded,
                  __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<void> signatureClose(__wasi_signature_t SigHandle);

  WasiCryptoExpect<__wasi_signature_state_t>
  signatureStateOpen(__wasi_signature_keypair_t KpHandle);

  WasiCryptoExpect<void>
  signatureStateUpdate(__wasi_signature_state_t StateHandle,
                       Span<const uint8_t> Input);

  WasiCryptoExpect<__wasi_signature_t>
  signatureStateSign(__wasi_signature_state_t StateHandle);

  WasiCryptoExpect<void>
  signatureStateClose(__wasi_signature_state_t StateHandle);

  WasiCryptoExpect<__wasi_signature_verification_state_t>
  signatureVerificationStateOpen(__wasi_signature_publickey_t PkHandle);

  WasiCryptoExpect<void> signatureVerificationStateUpdate(
      __wasi_signature_verification_state_t StateHandle,
      Span<const uint8_t> Input);

  WasiCryptoExpect<void> signatureVerificationStateVerify(
      __wasi_signature_verification_state_t StateHandle,
      __wasi_signature_t SigHandle);

  WasiCryptoExpect<void> signatureVerificationStateClose(
      __wasi_signature_verification_state_t StateHandle);

  int32_t InvaildHandle = 9999;

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod;
  WasmEdge::Runtime::Instance::MemoryInstance *MemInst;
  WasmEdge::Runtime::CallingFrame CallFrame;

  std::array<WasmEdge::ValVariant, 1> Errno;

  std::unique_ptr<Host::WasiCryptoAsymmetricCommonModule>
      WasiCryptoAsymCommonMod;
  std::unique_ptr<Host::WasiCryptoCommonModule> WasiCryptoCommonMod;
  std::unique_ptr<Host::WasiCryptoKxModule> WasiCryptoKxMod;
  std::unique_ptr<Host::WasiCryptoSignaturesModule> WasiCryptoSignMod;
  std::unique_ptr<Host::WasiCryptoSymmetricModule> WasiCryptoSymmMod;
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
