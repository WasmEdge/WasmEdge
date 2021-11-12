// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

namespace {
template <typename... Targs> void dummyCode(Targs &&.../* unused */) {}
} // namespace

KxContext::KxContext(CommonContext &DependencyCtx) : CommonCtx(DependencyCtx) {}

WasiCryptoExpect<__wasi_array_output_t> KxContext::kxDh(__wasi_publickey_t Pk,
                                                      __wasi_secretkey_t Sk) {
  dummyCode(Pk,Sk);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
KxContext::kxEncapsulate(__wasi_publickey_t Pk) {
  dummyCode(Pk);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_array_output_t>
KxContext::kxDecapsulate(__wasi_secretkey_t Sk,
                       Span<uint8_t> EncapsulatedSecret) {
  dummyCode(Sk, EncapsulatedSecret);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
