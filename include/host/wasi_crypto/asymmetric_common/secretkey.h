// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/secretkey.h"
#include "host/wasi_crypto/signature/secretkey.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Asymmetric {

using SecretKey = std::variant<std::shared_ptr<Kx::SecretKey>,
                               std::shared_ptr<Signatures::SecretKey>>;

WasiCryptoExpect<SecretKey>
secretKeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<const uint8_t> Encoded,
                __wasi_secretkey_encoding_e_t Encoding);

WasiCryptoExpect<std::vector<uint8_t>>
secretKeyExportData(SecretKey SecretKey,
                    __wasi_secretkey_encoding_e_t SkEncoding);

WasiCryptoExpect<PublicKey> secretKeyPublicKey(SecretKey SecretKey);

} // namespace Asymmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge