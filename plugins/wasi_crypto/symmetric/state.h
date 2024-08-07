// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/state.h - Symmetric State --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the symmetric state related classes, and provide a
/// unified interface which can be used to implement the algorithm operations.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "symmetric/key.h"
#include "symmetric/registered.h"
#include "symmetric/tag.h"
#include "utils/error.h"

#include "common/span.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// State created from key, and performs symmetric operations with using the
/// underlying algorithms.
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#state
using StateVariant = RegistedAlg::State;

WasiCryptoExpect<StateVariant>
openState(Algorithm Alg, OptionalRef<const KeyVariant> OptKeyVariant,
          OptionalRef<const Options> OptOptions) noexcept;

WasiCryptoExpect<size_t> stateOptionsGet(const StateVariant &StateVariant,
                                         std::string_view Name,
                                         Span<uint8_t> Value) noexcept;

WasiCryptoExpect<uint64_t> stateOptionsGetU64(const StateVariant &StateVariant,
                                              std::string_view Name) noexcept;

/// Absorb data into the state.
WasiCryptoExpect<void> stateAbsorb(StateVariant &StateVariant,
                                   Span<const uint8_t> Data) noexcept;

/// Squeeze bytes from the state.
WasiCryptoExpect<void> stateSqueeze(StateVariant &StateVariant,
                                    Span<uint8_t> Out) noexcept;

/// Compute and return a tag for all the data injected into the state so far.
WasiCryptoExpect<Tag> stateSqueezeTag(StateVariant &StateVariant) noexcept;

/// Use the current state to produce a key for a target algorithm.
WasiCryptoExpect<KeyVariant> stateSqueezeKey(StateVariant &StateVariant,
                                             Algorithm KeyAlg) noexcept;

/// Encrypt data with an attached tag.
WasiCryptoExpect<size_t> stateEncrypt(StateVariant &StateVariant,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) noexcept;

/// Encrypt data and return the ciphertext and the authentication tag
/// separately.
WasiCryptoExpect<Tag> stateEncryptDetached(StateVariant &StateVariant,
                                           Span<uint8_t> Out,
                                           Span<const uint8_t> Data) noexcept;

/// Decrypt a ciphertext with an attached tag.
WasiCryptoExpect<size_t> stateDecrypt(StateVariant &StateVariant,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) noexcept;

/// Verify an authentication tag and decrypt the corresponding ciphertext if
/// the verification passes.
WasiCryptoExpect<size_t>
stateDecryptDetached(StateVariant &StateVariant, Span<uint8_t> Out,
                     Span<const uint8_t> Data,
                     Span<const uint8_t> RawTag) noexcept;

/// Returns the length required to encode the authentication tag and optional
/// padding bytes.
WasiCryptoExpect<size_t>
stateMaxTagLen(const StateVariant &StateVariant) noexcept;

/// Make the state impossible to recover the previous state.
WasiCryptoExpect<void> stateRatchet(StateVariant &StateVariant) noexcept;

/// Clone the state.
WasiCryptoExpect<StateVariant>
stateClone(const StateVariant &ClonedStateVariant) noexcept;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
