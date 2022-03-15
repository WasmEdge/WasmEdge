// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/signatures/factory.h"
#include "host/wasi_crypto/signatures/signatures.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<__wasi_array_output_t>
Context::signatureExport(__wasi_signature_t SigHandle,
                         __wasi_signature_encoding_e_t Encoding) noexcept {
  return SignatureManager.get(SigHandle)
      .and_then([=](auto &&SigVariant) noexcept {
        return Signatures::sigExportData(SigVariant, Encoding);
      })
      .and_then([this](auto &&Data) noexcept {
        return ArrayOutputManger.registerManager(std::move(Data));
      });
}

WasiCryptoExpect<void>
Context::signatureClose(__wasi_signature_t SigHandle) noexcept {
  return SignatureManager.close(SigHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
Context::signatureImport(Signatures::Algorithm Alg, Span<const uint8_t> Encoded,
                         __wasi_signature_encoding_e_t Encoding) noexcept {
  return Signatures::sigImport(Alg, Encoded, Encoding)
      .and_then([this](auto &&Sig) noexcept {
        return SignatureManager.registerManager(std::move(Sig));
      });
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
