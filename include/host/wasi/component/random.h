// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/random.h - Random byte sources -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the random byte sources of the 0.2 and 0.3 `random`
/// interfaces: the system entropy source, and a fast generator.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/hash.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// A guest may ask for any length; one call hands out at most this many.
inline constexpr uint64_t MaxRandomBytesPerCall = 1U << 20;

/// Fill Out from the system entropy source.
inline void fillSecure(std::vector<uint8_t> &Out) noexcept {
  std::random_device Device;
  for (size_t I = 0; I < Out.size(); I += sizeof(unsigned int)) {
    const unsigned int Word = Device();
    std::memcpy(Out.data() + I, &Word, std::min(sizeof(Word), Out.size() - I));
  }
}

/// Fill Out from the fast generator of the insecure interface.
inline void fillInsecure(Hash::RandomEngine &Engine,
                         std::vector<uint8_t> &Out) noexcept {
  for (size_t I = 0; I < Out.size(); I += sizeof(uint64_t)) {
    const uint64_t Word = Engine();
    std::memcpy(Out.data() + I, &Word, std::min(sizeof(Word), Out.size() - I));
  }
}

inline uint64_t secureU64() noexcept {
  std::vector<uint8_t> Out(sizeof(uint64_t));
  fillSecure(Out);
  uint64_t Value = 0;
  std::memcpy(&Value, Out.data(), sizeof(Value));
  return Value;
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
