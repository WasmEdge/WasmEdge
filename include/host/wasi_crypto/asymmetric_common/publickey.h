// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/signature/publickey.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Asymmetric {

using PublicKey = std::variant<std::shared_ptr<Signatures::PublicKey>,
                               std::shared_ptr<Kx::PublicKey>>;

WasiCryptoExpect<PublicKey>
publicKeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<uint8_t const> Encoded,
                __wasi_publickey_encoding_e_t Encoding);

WasiCryptoExpect<std::vector<uint8_t>>
publicKeyExportData(PublicKey PublicKey,
                    __wasi_publickey_encoding_e_t Encoding);

WasiCryptoExpect<void> publicKeyVerify(PublicKey PublicKey);

} // namespace Asymmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
