// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "kx/kem/mlkem.h"

// The whole translation unit is gated: on OpenSSL < 3.5 this compiles to an
// empty object file and ML-KEM stays unregistered.
#if OPENSSL_VERSION_NUMBER >= 0x30500000L

// Needed for the complete definition of EncapsulatedSecret (forward declared
// in mlkem.h).
#include "kx/kx.h"

#include <openssl/core_names.h>
#include <openssl/params.h>

#include <algorithm>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::PublicKey>
MlKem<Bits>::PublicKey::import(
    Span<const uint8_t> Encoded,
    __wasi_publickey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == PkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);

    EvpPkeyCtxPtr PkCtx{EVP_PKEY_CTX_new_from_name(nullptr, name(), nullptr)};
    ensureOrReturn(PkCtx, __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
    ensureOrReturn(EVP_PKEY_fromdata_init(PkCtx.get()) > 0,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    OSSL_PARAM Params[2];
    Params[0] = OSSL_PARAM_construct_octet_string(
        OSSL_PKEY_PARAM_PUB_KEY, const_cast<uint8_t *>(Encoded.data()),
        Encoded.size());
    Params[1] = OSSL_PARAM_construct_end();

    EVP_PKEY *Pk = nullptr;
    ensureOrReturn(
        EVP_PKEY_fromdata(PkCtx.get(), &Pk, EVP_PKEY_PUBLIC_KEY, Params) > 0,
        __WASI_CRYPTO_ERRNO_INVALID_KEY);

    return EvpPkeyPtr{Pk};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int Bits>
WasiCryptoExpect<std::vector<uint8_t>> MlKem<Bits>::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(PkSize);

    size_t Size = PkSize;
    ensureOrReturn(
        EVP_PKEY_get_octet_string_param(Ctx.get(), OSSL_PKEY_PARAM_PUB_KEY,
                                        Res.data(), PkSize, &Size) > 0,
        __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int Bits>
WasiCryptoExpect<void> MlKem<Bits>::PublicKey::verify() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<EncapsulatedSecret>
MlKem<Bits>::PublicKey::encapsulate() const noexcept {
  EvpPkeyCtxPtr EncCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  ensureOrReturn(EncCtx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(EVP_PKEY_encapsulate_init(EncCtx.get(), nullptr) > 0,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  std::vector<uint8_t> Ciphertext(CtSize);
  SecretVec Secret(SecretSize);
  size_t CiphertextLen = CtSize;
  size_t SecretLen = SecretSize;
  ensureOrReturn(EVP_PKEY_encapsulate(EncCtx.get(), Ciphertext.data(),
                                      &CiphertextLen, Secret.data(),
                                      &SecretLen) > 0,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(CiphertextLen == CtSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(SecretLen == SecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return EncapsulatedSecret{std::move(Ciphertext), std::move(Secret)};
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::SecretKey>
MlKem<Bits>::SecretKey::import(
    Span<const uint8_t> Encoded,
    __wasi_secretkey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    // A 64-byte input is the FIPS 203 seed d||z, anything of the expanded
    // length is the decapsulation key dk itself.
    const char *ParamName = nullptr;
    if (Encoded.size() == SeedSize) {
      ParamName = OSSL_PKEY_PARAM_ML_KEM_SEED;
    } else if (Encoded.size() == SkSize) {
      ParamName = OSSL_PKEY_PARAM_PRIV_KEY;
    } else {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
    }

    EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new_from_name(nullptr, name(), nullptr)};
    ensureOrReturn(SkCtx, __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
    ensureOrReturn(EVP_PKEY_fromdata_init(SkCtx.get()) > 0,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    OSSL_PARAM Params[2];
    Params[0] = OSSL_PARAM_construct_octet_string(
        ParamName, const_cast<uint8_t *>(Encoded.data()), Encoded.size());
    Params[1] = OSSL_PARAM_construct_end();

    EVP_PKEY *Sk = nullptr;
    ensureOrReturn(
        EVP_PKEY_fromdata(SkCtx.get(), &Sk, EVP_PKEY_KEYPAIR, Params) > 0,
        __WASI_CRYPTO_ERRNO_INVALID_KEY);

    return EvpPkeyPtr{Sk};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int Bits>
WasiCryptoExpect<SecretVec> MlKem<Bits>::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    SecretVec Res(SkSize);

    size_t Size = SkSize;
    ensureOrReturn(
        EVP_PKEY_get_octet_string_param(Ctx.get(), OSSL_PKEY_PARAM_PRIV_KEY,
                                        Res.data(), SkSize, &Size) > 0,
        __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::PublicKey>
MlKem<Bits>::SecretKey::publicKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

template <int Bits>
WasiCryptoExpect<SecretVec>
MlKem<Bits>::SecretKey::dh(const PublicKey &) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::KeyPair>
MlKem<Bits>::SecretKey::toKeyPair(const PublicKey &) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<SecretVec> MlKem<Bits>::SecretKey::decapsulate(
    Span<const uint8_t> EncapsulatedSecretData) const noexcept {
  ensureOrReturn(EncapsulatedSecretData.size() == CtSize,
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  EvpPkeyCtxPtr DecCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  ensureOrReturn(DecCtx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(EVP_PKEY_decapsulate_init(DecCtx.get(), nullptr) > 0,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  SecretVec Secret(SecretSize);
  size_t SecretLen = SecretSize;
  ensureOrReturn(EVP_PKEY_decapsulate(DecCtx.get(), Secret.data(), &SecretLen,
                                      EncapsulatedSecretData.data(),
                                      EncapsulatedSecretData.size()) > 0,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(SecretLen == SecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Secret;
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::KeyPair>
MlKem<Bits>::KeyPair::generate(OptionalRef<const Options>) noexcept {
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_from_name(nullptr, name(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  ensureOrReturn(EVP_PKEY_keygen_init(Ctx.get()) > 0,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EVP_PKEY *Kp = nullptr;
  ensureOrReturn(EVP_PKEY_keygen(Ctx.get(), &Kp) > 0,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return EvpPkeyPtr{Kp};
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::KeyPair>
MlKem<Bits>::KeyPair::import(Span<const uint8_t> Encoded,
                             __wasi_keypair_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == KpSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);

    EvpPkeyCtxPtr KpCtx{EVP_PKEY_CTX_new_from_name(nullptr, name(), nullptr)};
    ensureOrReturn(KpCtx, __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
    ensureOrReturn(EVP_PKEY_fromdata_init(KpCtx.get()) > 0,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    OSSL_PARAM Params[2];
    Params[0] = OSSL_PARAM_construct_octet_string(
        OSSL_PKEY_PARAM_PRIV_KEY,
        const_cast<uint8_t *>(Encoded.data() + PkSize), SkSize);
    Params[1] = OSSL_PARAM_construct_end();

    EVP_PKEY *Kp = nullptr;
    ensureOrReturn(
        EVP_PKEY_fromdata(KpCtx.get(), &Kp, EVP_PKEY_KEYPAIR, Params) > 0,
        __WASI_CRYPTO_ERRNO_INVALID_KEY);
    EvpPkeyPtr Res{Kp};

    // The decapsulation key embeds its own encapsulation key, so the supplied
    // ek half has to match the one dk derives. Reject inputs whose halves
    // belong to different keypairs instead of trusting dk alone.
    std::vector<uint8_t> DerivedPk(PkSize);
    size_t Size = PkSize;
    ensureOrReturn(
        EVP_PKEY_get_octet_string_param(Res.get(), OSSL_PKEY_PARAM_PUB_KEY,
                                        DerivedPk.data(), PkSize, &Size) > 0,
        __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    ensureOrReturn(
        std::equal(DerivedPk.begin(), DerivedPk.end(), Encoded.begin()),
        __WASI_CRYPTO_ERRNO_INVALID_KEY);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::PublicKey>
MlKem<Bits>::KeyPair::publicKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::SecretKey>
MlKem<Bits>::KeyPair::secretKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

template <int Bits>
WasiCryptoExpect<SecretVec> MlKem<Bits>::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    SecretVec Res(KpSize);

    size_t Size = PkSize;
    ensureOrReturn(
        EVP_PKEY_get_octet_string_param(Ctx.get(), OSSL_PKEY_PARAM_PUB_KEY,
                                        Res.data(), PkSize, &Size) > 0,
        __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    Size = SkSize;
    ensureOrReturn(
        EVP_PKEY_get_octet_string_param(Ctx.get(), OSSL_PKEY_PARAM_PRIV_KEY,
                                        Res.data() + PkSize, SkSize, &Size) > 0,
        __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template class MlKem<512>;
template class MlKem<768>;
template class MlKem<1024>;

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge

#endif // OPENSSL_VERSION_NUMBER >= 0x30500000L
