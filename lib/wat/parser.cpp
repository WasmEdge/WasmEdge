// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/parser.h"
#include "converter.h"
#include "tree_sitter.h"

extern "C" const TSLanguage *tree_sitter_wat();

namespace WasmEdge {
namespace WAT {

Expect<AST::Module> parseWat(std::string_view Source, const Configure &Conf) {
  Parser P(tree_sitter_wat);
  Tree T = P.parse(Source);
  if (T.rootNode().isNull()) {
    return Unexpect(ErrCode::Value::WatUnexpectedEnd);
  }
  Converter Conv(Conf);
  return Conv.convert(T, Source);
}

} // namespace WAT
} // namespace WasmEdge
