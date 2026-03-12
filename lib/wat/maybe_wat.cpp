// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"

#include <cstdint>

namespace WasmEdge {
namespace WAT {

bool maybeWAT(Span<const uint8_t> Input) noexcept {
  // Minimal valid WAT is "(module)" — 8 bytes.
  if (Input.size() < 8) {
    return false;
  }

  // Skip leading whitespace (space, tab, newline, carriage return).
  size_t I = 0;
  while (I < Input.size() && (Input[I] == ' ' || Input[I] == '\t' ||
                              Input[I] == '\n' || Input[I] == '\r')) {
    ++I;
  }

  // WAT must start with '('.
  if (I >= Input.size() || Input[I] != '(') {
    return false;
  }

  // Validate UTF-8 with no null bytes. WASM binaries start with \0asm, so
  // rejecting null codepoints ensures they are never misidentified as text.
  Span<const uint8_t> Rest = Input;
  while (!Rest.empty()) {
    uint32_t Codepoint;
    unsigned Remaining;

    if (Rest[0] < 0x80) {
      // 0xxxxxxx — single-byte (ASCII), but reject null bytes
      if (unlikely(Rest[0] == 0x00)) {
        return false;
      }
      Rest = Rest.subspan(1);
      continue;
    } else if ((Rest[0] & 0xE0) == 0xC0) {
      // 110xxxxx — 2-byte sequence
      Codepoint = Rest[0] & 0x1F;
      Remaining = 1;
    } else if ((Rest[0] & 0xF0) == 0xE0) {
      // 1110xxxx — 3-byte sequence
      Codepoint = Rest[0] & 0x0F;
      Remaining = 2;
    } else if ((Rest[0] & 0xF8) == 0xF0) {
      // 11110xxx — 4-byte sequence
      Codepoint = Rest[0] & 0x07;
      Remaining = 3;
    } else {
      // Invalid leading byte (continuation byte or 5+byte leader)
      return false;
    }

    Rest = Rest.subspan(1);

    // Check that enough continuation bytes remain
    if (unlikely(Remaining > Rest.size())) {
      return false;
    }

    // Consume continuation bytes
    for (unsigned J = 0; J < Remaining; ++J) {
      if (unlikely((Rest[0] & 0xC0) != 0x80)) {
        return false;
      }
      Codepoint = (Codepoint << 6) | (Rest[0] & 0x3F);
      Rest = Rest.subspan(1);
    }

    // Reject overlong encodings
    if (unlikely(Remaining == 1 && Codepoint < 0x80)) {
      return false;
    }
    if (unlikely(Remaining == 2 && Codepoint < 0x800)) {
      return false;
    }
    if (unlikely(Remaining == 3 && Codepoint < 0x10000)) {
      return false;
    }

    // Reject surrogate halves (U+D800–U+DFFF)
    if (unlikely(Codepoint >= 0xD800 && Codepoint <= 0xDFFF)) {
      return false;
    }

    // Reject codepoints above U+10FFFF
    if (unlikely(Codepoint > 0x10FFFF)) {
      return false;
    }
  }

  return true;
}

} // namespace WAT
} // namespace WasmEdge
