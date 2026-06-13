// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"

namespace WasmEdge {
namespace WAT {

bool maybeWAT(Span<const uint8_t> Input) noexcept {
  // Heuristic: after an optional UTF-8 BOM, leading whitespace and line
  // comments, WAT text starts with '(' -- either an opening '(module ...)' or a
  // block comment '(; ... ;)'. WASM binary starts with the '\0asm' magic. This
  // only routes the input; the real parser performs full validation.
  // Empty input is neither WAT nor WASM, so there is nothing to route. Bail out
  // first: for an empty Span data() may be null, and forming End as
  // data() + size() would be pointer arithmetic on null (undefined behavior
  // even when adding zero).
  if (Input.empty()) {
    return false;
  }
  const uint8_t *It = Input.data();
  const uint8_t *const End = It + Input.size();

  // Skip an optional UTF-8 BOM (EF BB BF).
  if (End - It >= 3 && It[0] == 0xEFU && It[1] == 0xBBU && It[2] == 0xBFU) {
    It += 3;
  }

  const auto IsSpace = [](uint8_t B) noexcept {
    return B == ' ' || B == '\t' || B == '\n' || B == '\r';
  };
  while (It != End) {
    if (IsSpace(*It)) {
      ++It;
    } else if (*It == ';' && End - It >= 2 && It[1] == ';') {
      // Line comment ';; ...' runs to the end of the line; stop at the first
      // line terminator (LF or CR) so CR-only and CRLF endings are handled.
      It += 2;
      while (It != End && *It != '\n' && *It != '\r') {
        ++It;
      }
    } else {
      // First significant byte: '(' (module or block comment) means WAT.
      break;
    }
  }
  return It != End && *It == '(';
}

} // namespace WAT
} // namespace WasmEdge
