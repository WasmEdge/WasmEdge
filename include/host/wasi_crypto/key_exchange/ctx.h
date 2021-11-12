// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/common/ctx.h"
#include "host/wasi_crypto/error.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxContext {
public:
  KxContext(CommonContext &DependencyCtx);

  WasiCryptoExpect<__wasi_array_output_t> kxDh(__wasi_publickey_t Pk,
                                             __wasi_secretkey_t Sk);

  WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
  kxEncapsulate(__wasi_publickey_t Pk);

  WasiCryptoExpect<__wasi_array_output_t>
  kxDecapsulate(__wasi_secretkey_t Sk, Span<uint8_t> EncapsulatedSecret);

private:
  CommonContext &CommonCtx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
