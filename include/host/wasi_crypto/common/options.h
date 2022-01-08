// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "wasi_crypto/api.hpp"
#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

/// Some functions support options. For example, options can be used to access
/// features that are only relevant to specific ciphers and hash functions.
///
/// Options are represented as a (key, value) map, keys being strings. They are
/// attached to a context, such as a cipher state. Applications can set, but
/// also read the value associated with a key in order to either get the default
/// value, or obtain runtime information.
///
/// For example, in the context of an AEAD, the nonce option can be set by the
/// application in order to set the nonce. But if the runtime can safely compute
/// a nonce for each encryption, an application may not set the nonce, and
/// retrieve the actual nonce set by the runtime by reading the nonce option.
///
/// An option can be reused, but is tied to algorithm type.
using Options = std::variant<std::shared_ptr<Symmetric::Option>,
                             std::shared_ptr<Kx::Options>,
                             std::shared_ptr<Signatures::Options>>;

Options optionsOpen(__wasi_algorithm_type_e_t Alg);

WasiCryptoExpect<void> optionsSet(Options Options, std::string_view Name,
                                  Span<const uint8_t> Value);

WasiCryptoExpect<void> optionsSetU64(Options Options, std::string_view Name,
                                     uint64_t Value);

WasiCryptoExpect<void> optionsSetGuestBuffer(Options Options,
                                             std::string_view Name,
                                             Span<uint8_t> Value);

WasiCryptoExpect<std::vector<uint8_t>> optionsGet(Options Options,
                                                  std::string_view Name);

WasiCryptoExpect<uint64_t> optionsGetU64(Options Options,
                                         std::string_view Name);
} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge