// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/kx/kem/mlkem.h - ML-KEM alg implement ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the ML-KEM (FIPS 203) key
/// encapsulation mechanism algorithm.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "kx/options.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <openssl/opensslv.h>

#include <cstdint>
#include <utility>
#include <vector>

// ML-KEM depends on the KEM primitives introduced in OpenSSL 3.5. On older
// OpenSSL the whole class is compiled out and the algorithm stays
// unregistered, so behavior is unchanged.
#if OPENSSL_VERSION_NUMBER >= 0x30500000L

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

// Defined in kx/kx.h. Forward declared here to avoid an include cycle
// (kx.h -> registered.h -> mlkem.h). The complete type is only required in
// mlkem.cpp, which includes kx.h.
struct EncapsulatedSecret;

/// ML-KEM (FIPS 203) key encapsulation mechanism.
///
/// \tparam Bits security parameter set: 512, 768, or 1024.
template <int Bits> class MlKem {
public:
  /// OpenSSL algorithm name, e.g. "ML-KEM-768".
  static constexpr const char *name() noexcept {
    if constexpr (Bits == 512) {
      return "ML-KEM-512";
    } else if constexpr (Bits == 768) {
      return "ML-KEM-768";
    } else {
      return "ML-KEM-1024";
    }
  }

  class PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    PublicKey(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<const uint8_t> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<void> verify() const noexcept;

    WasiCryptoExpect<EncapsulatedSecret> encapsulate() const noexcept;

    const auto &raw() const { return Ctx; }

  private:
    SharedEvpPkey Ctx;
  };

  class KeyPair;

  class SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    SecretKey(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<SecretKey>
    import(Span<const uint8_t> Encoded,
           __wasi_secretkey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    // ML-KEM has no Diffie-Hellman, but the generic kx dispatchers extract the
    // peer public-key type from this signature, so it must exist.
    WasiCryptoExpect<SecretVec> dh(const PublicKey &Pk) const noexcept;

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &Pk) const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    decapsulate(Span<const uint8_t> EncapsulatedSecretData) const noexcept;

  private:
    SharedEvpPkey Ctx;
  };

  class KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    KeyPair(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const Options> Options) noexcept;

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Raw,
           __wasi_keypair_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretKey> secretKey() const noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept;

  private:
    SharedEvpPkey Ctx;
  };
};

using MlKem512 = MlKem<512>;
using MlKem768 = MlKem<768>;
using MlKem1024 = MlKem<1024>;

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge

#endif // OPENSSL_VERSION_NUMBER >= 0x30500000L
