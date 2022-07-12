// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/state.h - Symmetric State Definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains symmetric state relative class, provide a unified
/// interface used to implement the algorithm operation
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/registed.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// State created from key, and performs symmetric operations using the
/// underlying algorithm
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

/// absorb data into the state.
WasiCryptoExpect<void> stateAbsorb(StateVariant &StateVariant,
                                   Span<const uint8_t> Data) noexcept;

///  squeeze bytes from the state.
WasiCryptoExpect<void> stateSqueeze(StateVariant &StateVariant,
                                    Span<uint8_t> Out) noexcept;

/// compute and return a tag for all the data injected into the state so far.
WasiCryptoExpect<Tag> stateSqueezeTag(StateVariant &StateVariant) noexcept;

/// use the current state to produce a key for a target algorithm.
WasiCryptoExpect<KeyVariant> stateSqueezeKey(StateVariant &StateVariant,
                                             Algorithm KeyAlg) noexcept;

/// encrypt data with an attached tag.
WasiCryptoExpect<size_t> stateEncrypt(StateVariant &StateVariant,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) noexcept;

/// encrypt data and return the ciphertext and the authentication tag
/// separately.
WasiCryptoExpect<Tag> stateEncryptDetached(StateVariant &StateVariant,
                                           Span<uint8_t> Out,
                                           Span<const uint8_t> Data) noexcept;

/// decrypt a ciphertext with an attached tag
WasiCryptoExpect<size_t> stateDecrypt(StateVariant &StateVariant,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) noexcept;

/// verify an authentication tag and decrypt the corresponding ciphertext if
/// verification passes
WasiCryptoExpect<size_t>
stateDecryptDetached(StateVariant &StateVariant, Span<uint8_t> Out,
                     Span<const uint8_t> Data,
                     Span<const uint8_t> RawTag) noexcept;

///  returns the length required to encode the authentication tag and optional
///  padding bytes
WasiCryptoExpect<size_t>
stateMaxTagLen(const StateVariant &StateVariant) noexcept;

/// make state impossible to recover the previous state
WasiCryptoExpect<void> stateRatchet(StateVariant &StateVariant) noexcept;

/// clone state
WasiCryptoExpect<StateVariant>
stateClone(const StateVariant &ClonedStateVariant) noexcept;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge