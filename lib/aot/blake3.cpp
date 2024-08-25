// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "aot/blake3.h"

#include "common/config.h"
#include "common/defines.h"

namespace WasmEdge {
namespace AOT {

namespace {}

Blake3::Blake3() noexcept { blake3_hasher_init(&Hasher); }

void Blake3::update(Span<const Byte> Data) noexcept {
  blake3_hasher_update(&Hasher, Data.data(), Data.size());
}

void Blake3::finalize(Span<Byte> Output) noexcept {
  blake3_hasher_finalize(&Hasher, Output.data(), Output.size());
}

} // namespace AOT
} // namespace WasmEdge
