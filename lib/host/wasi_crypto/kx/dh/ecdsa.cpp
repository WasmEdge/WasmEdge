// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/dh/ecdsa.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "openssl/pem.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

namespace {
inline const size_t SharedSecretSize = 32;
} // namespace

WasiCryptoExpect<SecretVec>
Ecdsa::SecretKey::dh(const PublicKey &Pk) const noexcept {
  EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  opensslCheck(EVP_PKEY_derive_init(SkCtx.get()));

  // set peer key
  opensslCheck(EVP_PKEY_derive_set_peer(SkCtx.get(), Pk.raw().get()));

  // generate shared secret
  SecretVec Res(SharedSecretSize);
  size_t Size = SharedSecretSize;
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(Size == SharedSecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
