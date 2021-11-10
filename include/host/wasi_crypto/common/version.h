// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi_crypto/api.hpp"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
/// Version of a managed key.
///
/// A version can be an arbitrary `u64` integer, with the expection of some
/// reserved values.
namespace Version {

/// Key doesn't support versioning.
inline constexpr __wasi_version_t UNSPECIFIED = 0xff00000000000000;
/// Use the latest version of a key.
inline constexpr __wasi_version_t LASTEST = 0xff00000000000001;
/// Perform an operation over all versions of a key
inline constexpr __wasi_version_t ALL = 0xff00000000000002;

} // namespace Version
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge