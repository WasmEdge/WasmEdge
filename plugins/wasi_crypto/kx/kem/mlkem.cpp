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

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::PublicKey>
MlKem<Bits>::PublicKey::import(Span<const uint8_t>,
                               __wasi_publickey_encoding_e_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<std::vector<uint8_t>> MlKem<Bits>::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(PkSize);

    size_t Size = PkSize;
    opensslCheck(EVP_PKEY_get_octet_string_param(
        Ctx.get(), OSSL_PKEY_PARAM_PUB_KEY, Res.data(), PkSize, &Size));
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
  opensslCheck(EVP_PKEY_encapsulate_init(EncCtx.get(), nullptr));

  std::vector<uint8_t> Ciphertext(CtSize);
  std::vector<uint8_t> Secret(SecretSize);
  size_t CiphertextLen = CtSize;
  size_t SecretLen = SecretSize;
  ensureOrReturn(EVP_PKEY_encapsulate(EncCtx.get(), Ciphertext.data(),
                                      &CiphertextLen, Secret.data(),
                                      &SecretLen),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(CiphertextLen == CtSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(SecretLen == SecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return EncapsulatedSecret{std::move(Ciphertext), std::move(Secret)};
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::SecretKey>
MlKem<Bits>::SecretKey::import(Span<const uint8_t>,
                               __wasi_secretkey_encoding_e_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<SecretVec> MlKem<Bits>::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
WasiCryptoExpect<std::vector<uint8_t>> MlKem<Bits>::SecretKey::decapsulate(
    Span<const uint8_t> EncapsulatedSecretData) const noexcept {
  ensureOrReturn(EncapsulatedSecretData.size() == CtSize,
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  EvpPkeyCtxPtr DecCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  ensureOrReturn(DecCtx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  opensslCheck(EVP_PKEY_decapsulate_init(DecCtx.get(), nullptr));

  std::vector<uint8_t> Secret(SecretSize);
  size_t SecretLen = SecretSize;
  ensureOrReturn(EVP_PKEY_decapsulate(DecCtx.get(), Secret.data(), &SecretLen,
                                      EncapsulatedSecretData.data(),
                                      EncapsulatedSecretData.size()),
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
  opensslCheck(EVP_PKEY_keygen_init(Ctx.get()));

  EVP_PKEY *Kp = nullptr;
  opensslCheck(EVP_PKEY_keygen(Ctx.get(), &Kp));

  return EvpPkeyPtr{Kp};
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::KeyPair>
MlKem<Bits>::KeyPair::import(Span<const uint8_t>,
                             __wasi_keypair_encoding_e_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
WasiCryptoExpect<SecretVec>
MlKem<Bits>::KeyPair::exportData(__wasi_keypair_encoding_e_t) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template class MlKem<512>;
template class MlKem<768>;
template class MlKem<1024>;

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge

#endif // OPENSSL_VERSION_NUMBER >= 0x30500000L
