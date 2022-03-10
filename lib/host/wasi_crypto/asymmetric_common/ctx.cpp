// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<__wasi_array_output_t>
Context::publickeyExport(__wasi_publickey_t PkHandle,
                         __wasi_publickey_encoding_e_t PkEncoding) noexcept {
  return PublicKeyManager.get(PkHandle)
      .and_then([=](auto &&Pk) {
        return AsymmetricCommon::pkExportData(Pk, PkEncoding);
      })
      .and_then([this](auto &&Data) {
        return ArrayOutputManger.registerManager(std::move(Data));
      });
}

WasiCryptoExpect<void>
Context::publickeyVerify(__wasi_publickey_t PkHandle) noexcept {
  return PublicKeyManager.get(PkHandle).and_then(AsymmetricCommon::pkVerify);
}

WasiCryptoExpect<void>
Context::publickeyClose(__wasi_publickey_t PkHandle) noexcept {
  return PublicKeyManager.close(PkHandle);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
