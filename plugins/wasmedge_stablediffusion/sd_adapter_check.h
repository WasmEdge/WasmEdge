// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

enum class AdapterKind : uint8_t { Lora, Embedding };

// Checks a safetensors file from its size and header text only. It accepts only
// the layouts that have a rule, because the library does not check these
// tensors. For an accepted embedding, *Rows (if given) is the maximum token
// count of one mention: the largest first dimension of its 2-D tensors, or 1.
ErrNo checkAdapterHeader(AdapterKind Kind, uint64_t FileSize,
                         std::string_view HeaderJson,
                         uint64_t *Rows = nullptr) noexcept;
// Reads the 8-byte little-endian header length and the header, then checks
// them.
ErrNo checkAdapterFile(AdapterKind Kind, const std::filesystem::path &HostPath,
                       uint64_t *Rows = nullptr) noexcept;
// Returns true only for a safetensors file with a readable JSON header of at
// most Limits::MaxSafetensorsHeaderBytes, no tensor name that ends in
// ".comfy_quant", and no tensor of dtype F8_E4M3 or F8_E5M2. It checks no other
// dtype. All other files give false.
bool isUnquantizedSafetensors(const std::filesystem::path &HostPath) noexcept;

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
