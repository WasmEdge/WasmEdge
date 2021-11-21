// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoContext::keypairGenerate(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  std::optional<Options> OptOptions;
  if (OptOptionsHandle) {
    auto Res = OptionsManger.get(*OptOptionsHandle);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    OptOptions = std::move(*Res);
  }

  auto Kp = KeyPair::generate(AlgType, AlgStr, OptOptions);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  return KeypairManger.registerManger(*Kp);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoContext::keypairImport(__wasi_algorithm_type_e_t AlgType,
                                 std::string_view AlgStr, Span<uint8_t> Encoded,
                                 __wasi_keypair_encoding_e_t Encoding) {
  auto Kp = KeyPair::import(AlgType, AlgStr, Encoded, Encoding);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  return KeypairManger.registerManger(*Kp);
}

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoContext::keypairGenerateManaged(
    __wasi_secrets_manager_t, __wasi_algorithm_type_e_t, std::string_view,
    std::optional<__wasi_options_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> WasiCryptoContext::keypairStoreManaged(
    __wasi_secrets_manager_t, __wasi_keypair_t, uint8_t_ptr, __wasi_size_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_version_t>
WasiCryptoContext::keypairReplaceManaged(__wasi_secrets_manager_t,
                                         __wasi_keypair_t, __wasi_keypair_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
WasiCryptoContext::keypairId(__wasi_keypair_t, uint8_t_ptr, __wasi_size_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoContext::keypairFromId(__wasi_secrets_manager_t, const_uint8_t_ptr,
                                 __wasi_size_t, __wasi_version_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoContext::keypairFromPkAndSk(__wasi_publickey_t PkHandle,
                                      __wasi_secretkey_t SkHandle) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto Sk = SecretkeyManger.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto Kp = KeyPair::fromPkAndSk(*Pk, *Sk);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  return KeypairManger.registerManger(*Kp);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::keypairExport(__wasi_keypair_t KpHandle,
                                 __wasi_keypair_encoding_e_t KeypairEncoding) {
  auto Kp = KeypairManger.get(KpHandle);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  auto Encoded = Kp->exportData(KeypairEncoding);
  if (!Encoded) {
    return WasiCryptoUnexpect(Encoded);
  }

  return allocateArrayOutput(std::move(*Encoded));
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoContext::keypairPublickey(__wasi_keypair_t KpHandle) {
  auto Kp = KeypairManger.get(KpHandle);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  auto Pk = Kp->publicKey();
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return PublickeyManger.registerManger(std::move(*Pk));
}

WasiCryptoExpect<__wasi_secretkey_t>
WasiCryptoContext::keypairSecretkey(__wasi_keypair_t KpHandle) {
  auto Kp = KeypairManger.get(KpHandle);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  auto Sk = Kp->secretKey();
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return SecretkeyManger.registerManger(std::move(*Sk));
}

WasiCryptoExpect<void>
WasiCryptoContext::keypairClose(__wasi_keypair_t KpHandle) {
  return KeypairManger.close(KpHandle);
}

WasiCryptoExpect<__wasi_publickey_t> WasiCryptoContext::publickeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_publickey_encoding_e_t Encoding) {
  auto Pk = PublicKey::import(AlgType, AlgStr, Encoded, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return PublickeyManger.registerManger(*Pk);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::publickeyExport(__wasi_publickey_t PkHandle,
                                   __wasi_publickey_encoding_e_t PkEncoding) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto Encoded = Pk->exportData(PkEncoding);
  if (!Encoded) {
    return WasiCryptoUnexpect(Encoded);
  }

  return allocateArrayOutput(std::move(*Encoded));
}

WasiCryptoExpect<void>
WasiCryptoContext::publickeyVerify(__wasi_publickey_t PkHandle) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return Pk->verify();
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoContext::publickeyFromSecretkey(__wasi_secretkey_t SkHandle) {
  auto Sk = SecretkeyManger.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto Pk = Sk->publicKey();
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return PublickeyManger.registerManger(*Pk);
}

WasiCryptoExpect<void>
WasiCryptoContext::publickeyClose(__wasi_publickey_t PkHandle) {
  return PublickeyManger.close(PkHandle);
}

WasiCryptoExpect<__wasi_secretkey_t> WasiCryptoContext::secretkeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_secretkey_encoding_e_t EncodingEnum) {
  auto Pk = SecretKey::import(AlgType, AlgStr, Encoded, EncodingEnum);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return SecretkeyManger.registerManger(*Pk);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::secretkeyExport(__wasi_secretkey_t SkHandle,
                                   __wasi_secretkey_encoding_e_t SkEncoding) {
  auto Sk = SecretkeyManger.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto Res = Sk->exportData(SkEncoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return allocateArrayOutput(std::move(*Res));
}

WasiCryptoExpect<void>
WasiCryptoContext::secretkeyClose(__wasi_secretkey_t SkHandle) {
  return SecretkeyManger.close(SkHandle);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
