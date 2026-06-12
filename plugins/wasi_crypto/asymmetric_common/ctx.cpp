// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ctx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<__wasi_array_output_t>
Context::publickeyExport(__wasi_publickey_t PkHandle,
                         __wasi_publickey_encoding_e_t Encoding) noexcept {
  return PublicKeyManager.get(PkHandle)
      .and_then([Encoding](auto &&Pk) {
        return AsymmetricCommon::pkExportData(std::forward<decltype(Pk)>(Pk),
                                              Encoding);
      })
      .and_then([this](auto &&Data) {
        return ArrayOutputManager.registerManager(
            std::forward<decltype(Data)>(Data));
      });
}

WasiCryptoExpect<void>
Context::publickeyVerify(__wasi_publickey_t PkHandle) noexcept {
  return PublicKeyManager.get(PkHandle).and_then(AsymmetricCommon::pkVerify);
}

WasiCryptoExpect<void>
Context::publickeyClose(__wasi_publickey_t PkHandle) noexcept {
  return PublicKeyManager.close(PkHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
Context::secretkeyExport(__wasi_secretkey_t SkHandle,
                         __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return SecretKeyManager.get(SkHandle)
      .and_then([Encoding](auto &&Sk) {
        return AsymmetricCommon::skExportData(std::forward<decltype(Sk)>(Sk),
                                              Encoding);
      })
      .and_then([this](auto &&Data) noexcept {
        return ArrayOutputManager.registerManager(
            std::forward<decltype(Data)>(Data));
      });
}

WasiCryptoExpect<void>
Context::secretkeyClose(__wasi_secretkey_t SkHandle) noexcept {
  return SecretKeyManager.close(SkHandle);
}

WasiCryptoExpect<__wasi_publickey_t>
Context::publickeyFromSecretkey(__wasi_secretkey_t SkHandle) noexcept {
  return SecretKeyManager.get(SkHandle)
      .and_then(AsymmetricCommon::skPublicKey)
      .and_then([this](auto &&Pk) noexcept {
        return PublicKeyManager.registerManager(std::forward<decltype(Pk)>(Pk));
      });
}

WasiCryptoExpect<__wasi_array_output_t>
Context::keypairExport(__wasi_keypair_t KpHandle,
                       __wasi_keypair_encoding_e_t Encoding) noexcept {
  return KeyPairManager.get(KpHandle)
      .and_then([Encoding](auto &&Kp) noexcept {
        return AsymmetricCommon::kpExportData(std::forward<decltype(Kp)>(Kp),
                                              Encoding);
      })
      .and_then([this](auto &&Data) noexcept {
        return ArrayOutputManager.registerManager(
            std::forward<decltype(Data)>(Data));
      });
}

WasiCryptoExpect<__wasi_publickey_t>
Context::keypairPublickey(__wasi_keypair_t KpHandle) noexcept {
  return KeyPairManager.get(KpHandle)
      .and_then(AsymmetricCommon::kpPublicKey)
      .and_then([this](auto &&Pk) noexcept {
        return PublicKeyManager.registerManager(std::forward<decltype(Pk)>(Pk));
      });
}

WasiCryptoExpect<__wasi_secretkey_t>
Context::keypairSecretkey(__wasi_keypair_t KpHandle) noexcept {
  return KeyPairManager.get(KpHandle)
      .and_then(AsymmetricCommon::kpSecretKey)
      .and_then([this](auto &&Sk) noexcept {
        return SecretKeyManager.registerManager(std::forward<decltype(Sk)>(Sk));
      });
}

WasiCryptoExpect<void>
Context::keypairClose(__wasi_keypair_t KpHandle) noexcept {
  return KeyPairManager.close(KpHandle);
}

WasiCryptoExpect<__wasi_keypair_t>
Context::keypairFromPkAndSk(__wasi_publickey_t PkHandle,
                            __wasi_secretkey_t SkHandle) noexcept {
  auto Pk = PublicKeyManager.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto Sk = SecretKeyManager.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return AsymmetricCommon::kpFromPkAndSk(*Pk, *Sk).and_then(
      [this](auto &&Kp) noexcept {
        return KeyPairManager.registerManager(std::forward<decltype(Kp)>(Kp));
      });
}

WasiCryptoExpect<__wasi_keypair_t>
Context::keypairGenerate(AsymmetricCommon::Algorithm Alg,
                         __wasi_opt_options_t OptOptionsHandle) noexcept {
  return mapAndTransposeOptional(
             OptOptionsHandle,
             [this](__wasi_options_t OptionsHandle) noexcept {
               return OptionsManager.get(OptionsHandle);
             })
      .and_then([Alg](auto &&OptOptions) noexcept {
        return AsymmetricCommon::generateKp(
            Alg, asOptionalRef(std::forward<decltype(OptOptions)>(OptOptions)));
      })
      .and_then([this](auto &&Kp) noexcept {
        return KeyPairManager.registerManager(std::forward<decltype(Kp)>(Kp));
      });
}

WasiCryptoExpect<__wasi_keypair_t>
Context::keypairImport(AsymmetricCommon::Algorithm Alg,
                       Span<const uint8_t> Encoded,
                       __wasi_keypair_encoding_e_t Encoding) noexcept {
  return AsymmetricCommon::importKp(Alg, Encoded, Encoding)
      .and_then([this](auto &&Kp) noexcept {
        return KeyPairManager.registerManager(std::forward<decltype(Kp)>(Kp));
      });
}

WasiCryptoExpect<__wasi_publickey_t>
Context::publickeyImport(AsymmetricCommon::Algorithm Alg,
                         Span<const uint8_t> Encoded,
                         __wasi_publickey_encoding_e_t Encoding) noexcept {
  return AsymmetricCommon::importPk(Alg, Encoded, Encoding)
      .and_then([this](auto &&Pk) noexcept {
        return PublicKeyManager.registerManager(std::forward<decltype(Pk)>(Pk));
      });
}

WasiCryptoExpect<__wasi_secretkey_t>
Context::secretkeyImport(AsymmetricCommon::Algorithm Alg,
                         Span<const uint8_t> Encoded,
                         __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return AsymmetricCommon::importSk(Alg, Encoded, Encoding)
      .and_then([this](auto &&Sk) noexcept {
        return SecretKeyManager.registerManager(std::forward<decltype(Sk)>(Sk));
      });
}

WasiCryptoExpect<__wasi_keypair_t> Context::keypairGenerateManaged(
    __wasi_secrets_manager_t, AsymmetricCommon::Algorithm Alg,
    __wasi_opt_options_t OptOptionsHandle) noexcept {
  return OptionsManager.get(OptOptionsHandle).and_then([&](auto &&Options) {
    return AsymmetricCommon::generateKp(Alg, Options).and_then([&](auto &&Kp) {
      return KeyPairManager.registerManager(std::move(Kp));
    });
  });
}

WasiCryptoExpect<void> Context::keypairStoreManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, __wasi_keypair_t KpHandle,
    Span<uint8_t> KpId) noexcept {
  return SecretsManagerManager.get(SecretsManagerHandle)
      .and_then([&](auto &&Sm) noexcept {
        return KeyPairManager.get(KpHandle).and_then([&](auto &&Kp) noexcept {
          return Sm.storeKp(KpId, 0, Kp);
        });
      });
}

WasiCryptoExpect<__wasi_version_t>
Context::keypairReplaceManaged(__wasi_secrets_manager_t SecretsManagerHandle,
                               __wasi_keypair_t OldKpHandle,
                               __wasi_keypair_t NewKpHandle) noexcept {
  // Simple implementation: store the new key with version 0.
  // In a real implementation, this would handle versioning.
  return KeyPairManager.get(NewKpHandle).and_then([&](auto &&Kp) noexcept {
    return SecretsManagerManager.get(SecretsManagerHandle)
        .and_then([&](auto &&Sm) noexcept {
          // We don't have the ID of OldKpHandle easily here without extra
          // tracking. For now, return NOT_IMPLEMENTED for replace if we can't
          // find the ID.
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
        });
  });
}

WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
Context::keypairId(__wasi_keypair_t, Span<uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t>
Context::keypairFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                       Span<const uint8_t> KpId,
                       __wasi_version_t KpIdVersion) noexcept {
  return SecretsManagerManager.get(SecretsManagerHandle)
      .and_then([&](auto &&Sm) noexcept {
        return Sm.getKp(KpId, KpIdVersion).and_then([&](auto &&Kp) noexcept {
          return KeyPairManager.registerManager(Kp);
        });
      });
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
