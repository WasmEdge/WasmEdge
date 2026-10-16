// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/parser.h"
#include "converter.h"
#include "tree_sitter.h"
#include "wat/wat_util.h"

namespace WasmEdge {
namespace WAT {

Expect<AST::Module> parseWat(std::string_view Source, const Configure &Conf) {
  if (hasUTF8BOM(Source)) {
    Source.remove_prefix(3);
  }
  Parser P(tree_sitter_wat);
  Tree T = P.parse(Source);
  Converter Conv(Conf);
  return Conv.convert(T, Source);
}

Expect<AST::Module> parseWat(Span<const uint8_t> Source,
                             const Configure &Conf) {
  return parseWat(
      std::string_view(reinterpret_cast<const char *>(Source.data()),
                       Source.size()),
      Conf);
}

} // namespace WAT
} // namespace WasmEdge
