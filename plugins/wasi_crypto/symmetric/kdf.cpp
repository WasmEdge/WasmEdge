// SPDX-License-Identifier: Apache-2.0

#include "symmetric/kdf.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/secret_vec.h"

#include <openssl/kdf.h>
#include <openssl/rand.h>

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> constexpr uint32_t Hkdf<ShaNid>::getKeySize() noexcept {
  static_assert(ShaNid == NID_sha256 || ShaNid == NID_sha512);

  if constexpr (ShaNid == NID_sha256)
    return 32;
  if constexpr (ShaNid == NID_sha512)
    return 64;
}

template <int ShaNid>
constexpr const EVP_MD *Hkdf<ShaNid>::getShaCtx() noexcept {
  return EVP_get_digestbynid(ShaNid);
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Expand::Key>
Hkdf<ShaNid>::Expand::Key::generate(OptionalRef<const Options>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Expand::Key>
Hkdf<ShaNid>::Expand::Key::import(Span<const uint8_t> Raw) noexcept {
  ensureOrReturn(Raw.size() == getKeySize(), __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return SecretVec{Raw};
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Expand::State>
Hkdf<ShaNid>::Expand::State::open(const Key &Key,
                                  OptionalRef<const Options>) noexcept {
  return openStateImpl(Key.ref(), EVP_PKEY_HKDEF_MODE_EXPAND_ONLY);
}

template <int ShaNid>
WasiCryptoExpect<void>
Hkdf<ShaNid>::Expand::State::absorb(Span<const uint8_t> Data) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};
  opensslCheck(
      EVP_PKEY_CTX_add1_hkdf_info(Ctx->RawCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int ShaNid>
WasiCryptoExpect<void>
Hkdf<ShaNid>::Expand::State::squeeze(Span<uint8_t> Out) noexcept {
  size_t KeyLen = Out.size();

  {
    std::scoped_lock Lock{Ctx->Mutex};
    ensureOrReturn(EVP_PKEY_derive(Ctx->RawCtx.get(), Out.data(), &KeyLen),
                   __WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  return {};
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Expand::State>
Hkdf<ShaNid>::Expand::State::clone() const noexcept {
  // not supported for a keygen operation.
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Extract::Key>
Hkdf<ShaNid>::Extract::Key::generate(OptionalRef<const Options>) noexcept {
  return SecretVec::random<getKeySize()>();
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Extract::Key>
Hkdf<ShaNid>::Extract::Key::import(Span<const uint8_t> Raw) noexcept {
  return SecretVec{Raw};
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Extract::State>
Hkdf<ShaNid>::Extract::State::open(const Key &Key,
                                   OptionalRef<const Options>) noexcept {
  return openStateImpl(Key.ref(), EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY);
}

template <int ShaNid>
WasiCryptoExpect<void>
Hkdf<ShaNid>::Extract::State::absorb(Span<const uint8_t> Data) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};

  Ctx->Salt.insert(Ctx->Salt.end(), Data.begin(), Data.end());
  return {};
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Expand::Key>
Hkdf<ShaNid>::Extract::State::squeezeKey() noexcept {
  std::scoped_lock Lock{Ctx->Mutex};

  opensslCheck(EVP_PKEY_CTX_set1_hkdf_salt(Ctx->RawCtx.get(), Ctx->Salt.data(),
                                           Ctx->Salt.size()));

  SecretVec Data(getKeySize());

  size_t ActualOutSize;
  opensslCheck(EVP_PKEY_derive(Ctx->RawCtx.get(), Data.data(), &ActualOutSize));
  ensureOrReturn(ActualOutSize == getKeySize(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Data;
}

template <int ShaNid>
WasiCryptoExpect<typename Hkdf<ShaNid>::Extract::State>
Hkdf<ShaNid>::Extract::State::clone() const noexcept {
  // not supported for a keygen operation.
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int ShaNid>
WasiCryptoExpect<EvpPkeyCtxPtr>
Hkdf<ShaNid>::openStateImpl(Span<const uint8_t> Key, int Mode) noexcept {
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};
  opensslCheck(EVP_PKEY_derive_init(Ctx.get()));
  opensslCheck(EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), getShaCtx()));
  opensslCheck(EVP_PKEY_CTX_hkdf_mode(Ctx.get(), Mode));
  ensureOrReturn(EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), Key.data(), Key.size()),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return Ctx;
}

template class Hkdf<NID_sha256>::Extract;
template class Hkdf<NID_sha512>::Extract;
template class Hkdf<NID_sha256>::Expand;
template class Hkdf<NID_sha512>::Expand;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
