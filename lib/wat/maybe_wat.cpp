// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/parser.h"
#include "wat/wat_util.h"

#include <cstddef>
#include <string_view>

namespace WasmEdge {
namespace WAT {

bool maybeWAT(Span<const uint8_t> Input) noexcept {
  // WAT text starts with '(' after an optional UTF-8 BOM, whitespace, and ';;'
  // line comments. Real .wat and .wast headers often have these three items. A
  // WASM binary starts with '\0asm' (NUL), so it can never match. A '(;' block
  // comment also starts with '(', and this function accepts it. The parser
  // does the full validation.
  const std::string_view Text(reinterpret_cast<const char *>(Input.data()),
                              Input.size());
  const size_t N = Text.size();
  size_t I = hasUTF8BOM(Text) ? 3 : 0;
  while (I < N) {
    const uint8_t B = Input[I];
    if (isWatSpace(B)) {
      ++I;
    } else if (B == ';' && I + 1 < N && Input[I + 1] == ';') {
      // This is a ';;' line comment. Skip to the end of the line.
      I = lineCommentEnd(Text, I + 2);
    } else {
      break;
    }
  }
  return I < N && Input[I] == '(';
}

} // namespace WAT
} // namespace WasmEdge
