// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

template <auto Fn> using Deleter = std::integral_constant<decltype(Fn), Fn>;

template <typename T, auto Fn>
using OpenSSLUniquePtr = std::unique_ptr<T, Deleter<Fn>>;

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
