// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"

#include <algorithm>

namespace WasmEdge {
namespace WAT {

bool maybeWAT(Span<const uint8_t> Input) noexcept {
  // Heuristic: skip leading whitespace, check if first non-whitespace byte
  // is '('. WAT text always starts with '('; WASM binary starts with \0asm.
  // The real parser handles full validation.
  auto It = std::find_if_not(Input.begin(), Input.end(), [](uint8_t B) {
    return B == ' ' || B == '\t' || B == '\n' || B == '\r';
  });
  return It != Input.end() && *It == '(';
}

} // namespace WAT
} // namespace WasmEdge
