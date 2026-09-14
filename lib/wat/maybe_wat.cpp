// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/parser.h"

#include <cstddef>

namespace WasmEdge {
namespace WAT {

bool maybeWAT(Span<const uint8_t> Input) noexcept {
  // WAT text starts with '(' after an optional UTF-8 BOM, whitespace, and ';;'
  // line comments. Real .wat and .wast headers often have these three items. A
  // WASM binary starts with '\0asm' (NUL), so it can never match. A '(;' block
  // comment also starts with '(', and this function accepts it. The parser
  // does the full validation.
  size_t I = 0;
  const size_t N = Input.size();
  // Skip an optional UTF-8 BOM.
  if (N >= 3 && Input[0] == 0xEF && Input[1] == 0xBB && Input[2] == 0xBF) {
    I = 3;
  }
  while (I < N) {
    const uint8_t B = Input[I];
    if (B == ' ' || B == '\t' || B == '\n' || B == '\r') {
      ++I;
    } else if (B == ';' && I + 1 < N && Input[I + 1] == ';') {
      // This is a ';;' line comment. Skip to the end of the line.
      I += 2;
      while (I < N && Input[I] != '\n') {
        ++I;
      }
    } else {
      break;
    }
  }
  return I < N && Input[I] == '(';
}

} // namespace WAT
} // namespace WasmEdge
