// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
namespace Fixture {

// One entry of a safetensors header:
// "Name":{"dtype":"Dtype","shape":Shape,"data_offsets":[0,0]}
inline std::string tensor(std::string_view Name, std::string_view Dtype,
                          std::string_view Shape) {
  return "\"" + std::string(Name) + "\":{\"dtype\":\"" + std::string(Dtype) +
         "\",\"shape\":" + std::string(Shape) + ",\"data_offsets\":[0,0]}";
}

// A header with m.lora_down.weight and m.lora_up.weight (F32) of the two
// shapes, then Extra, in braces.
inline std::string lora(std::string_view Down, std::string_view Up,
                        std::string_view Extra = "") {
  return "{" + tensor("m.lora_down.weight", "F32", Down) + "," +
         tensor("m.lora_up.weight", "F32", Up) + std::string(Extra) + "}";
}

// Writes the 8-byte little-endian Length, then Body, then PayloadBytes zero
// bytes (by resizing the file); returns File.
inline std::filesystem::path writeHeaderFile(const std::filesystem::path &File,
                                             uint64_t Length,
                                             std::string_view Body,
                                             uint64_t PayloadBytes = 0) {
  {
    std::array<char, 8> LengthBytes{};
    for (size_t I = 0; I < LengthBytes.size(); ++I) {
      LengthBytes[I] = static_cast<char>((Length >> (8 * I)) & 0xFFU);
    }
    std::ofstream Out(File, std::ios::binary | std::ios::trunc);
    Out.write(LengthBytes.data(),
              static_cast<std::streamsize>(LengthBytes.size()));
    Out.write(Body.data(), static_cast<std::streamsize>(Body.size()));
  }
  std::filesystem::resize_file(File, 8 + Body.size() + PayloadBytes);
  return File;
}

} // namespace Fixture
} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
