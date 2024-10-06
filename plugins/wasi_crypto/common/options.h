// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/common/options.h - Options definition ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the Options definition of wasi-crypto.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "kx/options.h"
#include "signatures/options.h"
#include "symmetric/options.h"
#include "wasi_crypto/api.hpp"

#include "common/span.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

/// Some functions support options. For example, options can be used to access
/// the features that are only relevant to specific ciphers and hash functions.
///
/// Options are represented as a (key, value) map, keys are strings. They are
/// attached to a context, such as a cipher state. Applications can set, and
/// also read the value associated with a key in order to either get the default
/// value or obtain the runtime information.
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#options
using Options =
    std::variant<Symmetric::Options, Kx::Options, Signatures::Options>;

Options optionsOpen(__wasi_algorithm_type_e_t Alg) noexcept;

/// Set byte vectors.
WasiCryptoExpect<void> optionsSet(Options &Options, std::string_view Name,
                                  Span<const uint8_t> Value) noexcept;

/// Set unsigned integers.
WasiCryptoExpect<void> optionsSetU64(Options &Options, std::string_view Name,
                                     uint64_t Value) noexcept;

/// Set memory buffers.
WasiCryptoExpect<void> optionsSetGuestBuffer(Options &Options,
                                             std::string_view Name,
                                             Span<uint8_t> Value) noexcept;
} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
