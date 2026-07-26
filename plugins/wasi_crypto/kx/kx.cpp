// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "kx/kx.h"

#include "utils/evp_wrapper.h"

#include <openssl/evp.h>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

namespace {
template <typename T> struct DhTrait;

template <typename SkType, typename PkType>
struct DhTrait<WasiCryptoExpect<SecretVec> (SkType::*)(const PkType &)
                   const noexcept> {
  using Pk = PkType;
};
template <typename T> using PkType = typename DhTrait<decltype(&T::dh)>::Pk;
} // namespace

WasiCryptoExpect<SecretVec> dh(const PkVariant &PkVariant,
                               const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Pk,
         const auto &Sk) noexcept -> WasiCryptoExpect<SecretVec> {
        using InPkType = std::decay_t<decltype(Pk)>;
        using ExpectPkType = PkType<std::decay_t<decltype(Sk)>>;
        if constexpr (std::is_same_v<InPkType, ExpectPkType>) {
          return Sk.dh(Pk);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
        }
      },
      PkVariant, SkVariant);
}

WasiCryptoExpect<EncapsulatedSecret> encapsulate(PkVariant &PkVariant) noexcept {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
  return std::visit(
      [](auto &&Pk) noexcept -> WasiCryptoExpect<EncapsulatedSecret> {
        EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new(Pk.raw().get(), nullptr)};
        ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
        if (EVP_PKEY_encapsulate_init(Ctx.get(), nullptr) <= 0) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
        }
        size_t SecretLen = 0;
        size_t CiphertextLen = 0;
        if (EVP_PKEY_encapsulate(Ctx.get(), nullptr, &SecretLen, nullptr,
                                 &CiphertextLen) <= 0) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
        }
        std::vector<uint8_t> Secret(SecretLen);
        std::vector<uint8_t> Ciphertext(CiphertextLen);
        if (EVP_PKEY_encapsulate(Ctx.get(), Secret.data(), &SecretLen,
                                 Ciphertext.data(), &CiphertextLen) <= 0) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
        }
        Secret.resize(SecretLen);
        Ciphertext.resize(CiphertextLen);
        return EncapsulatedSecret{std::move(Ciphertext), std::move(Secret)};
      },
      PkVariant);
#else
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
#endif
}

WasiCryptoExpect<std::vector<uint8_t>>
decapsulate(SkVariant &SkVariant,
            Span<const uint8_t> EncapsulatedSecretPayload) noexcept {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
  return std::visit(
      [&](auto &&Sk) noexcept -> WasiCryptoExpect<std::vector<uint8_t>> {
        EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new(Sk.raw().get(), nullptr)};
        ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
        if (EVP_PKEY_decapsulate_init(Ctx.get(), nullptr) <= 0) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
        }
        size_t SecretLen = 0;
        if (EVP_PKEY_decapsulate(Ctx.get(), nullptr, &SecretLen,
                                 EncapsulatedSecretPayload.data(),
                                 EncapsulatedSecretPayload.size()) <= 0) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
        }
        std::vector<uint8_t> Secret(SecretLen);
        if (EVP_PKEY_decapsulate(Ctx.get(), Secret.data(), &SecretLen,
                                 EncapsulatedSecretPayload.data(),
                                 EncapsulatedSecretPayload.size()) <= 0) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
        }
        Secret.resize(SecretLen);
        return Secret;
      },
      SkVariant);
#else
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
#endif
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
