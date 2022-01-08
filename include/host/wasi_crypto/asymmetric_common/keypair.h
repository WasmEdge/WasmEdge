// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/signature/keypair.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Asymmetric {

using KeyPair = std::variant<std::shared_ptr<Signatures::KeyPair>,
                             std::shared_ptr<Kx::KeyPair>>;

WasiCryptoExpect<KeyPair> keypairGenerate(__wasi_algorithm_type_e_t AlgType,
                                          std::string_view AlgStr,
                                          Common::Options OptOptions);

WasiCryptoExpect<KeyPair> keyPairImport(__wasi_algorithm_type_e_t AlgType,
                                        std::string_view AlgStr,
                                        Span<const uint8_t> Encoded,
                                        __wasi_keypair_encoding_e_t Encoding);

WasiCryptoExpect<KeyPair> keyPairFromPkAndSk(PublicKey, SecretKey);

WasiCryptoExpect<std::vector<uint8_t>>
keyPairExportData(KeyPair KeyPair, __wasi_keypair_encoding_e_t Encoding);

WasiCryptoExpect<PublicKey> keyPairPublicKey(KeyPair KeyPair);

WasiCryptoExpect<SecretKey> keyPairSecretKey(KeyPair KeyPair);
} // namespace Asymmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
