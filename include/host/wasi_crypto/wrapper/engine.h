// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

template <typename T> class Engine {};

template <typename T> class OpenSSLEngine : Engine<T> {};

template <> class OpenSSLEngine<Sha2> {};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
