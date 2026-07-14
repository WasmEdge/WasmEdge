// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "kx/kem/mlkem.h"

// The whole translation unit is gated: on OpenSSL < 3.5 this compiles to an
// empty object file and ML-KEM stays unregistered.
#if OPENSSL_VERSION_NUMBER >= 0x30500000L

// Needed for the complete definition of EncapsulatedSecret (forward declared
// in mlkem.h).
#include "kx/kx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

// TODO(followup #5110): RAW = FIPS 203 64-byte seed d||z; EVP_PKEY KEM calls
// here. PR1 registers the algorithm and wires up dispatch only; every method
// is a NOT_IMPLEMENTED stub.

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::PublicKey>
MlKem<Bits>::PublicKey::import(Span<const uint8_t>,
                               __wasi_publickey_encoding_e_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<std::vector<uint8_t>> MlKem<Bits>::PublicKey::exportData(
    __wasi_publickey_encoding_e_t) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<void> MlKem<Bits>::PublicKey::verify() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<EncapsulatedSecret>
MlKem<Bits>::PublicKey::encapsulate() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
WasiCryptoExpect<std::vector<uint8_t>>
MlKem<Bits>::SecretKey::decapsulate(Span<const uint8_t>) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::KeyPair>
MlKem<Bits>::KeyPair::generate(OptionalRef<const Options>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Bits>
WasiCryptoExpect<typename MlKem<Bits>::SecretKey>
MlKem<Bits>::KeyPair::secretKey() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
