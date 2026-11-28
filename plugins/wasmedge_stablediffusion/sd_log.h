// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

// Makes guest text safe for one log line: keeps at most 64 bytes, replaces each
// byte outside 0x20 to 0x7e with '?', and appends "..." if it cut the text.
inline std::string logText(std::string_view Text) {
  constexpr size_t MaxBytes = 64;
  std::string Safe(Text.substr(0, MaxBytes));
  for (char &C : Safe) {
    const auto Byte = static_cast<unsigned char>(C);
    if (Byte < 0x20 || Byte > 0x7e) {
      C = '?';
    }
  }
  if (Text.size() > MaxBytes) {
    Safe += "...";
  }
  return Safe;
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
