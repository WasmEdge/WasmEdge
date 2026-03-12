// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"
#include "converter.h"
#include "tree_sitter.h"

namespace WasmEdge::WAT {

Expect<AST::Module> parseWat(std::string_view Source) {
  Parser P;
  Tree T = P.parse(Source);
  if (T.rootNode().isNull()) {
    return Unexpect(ErrCode::Value::WatUnexpectedEnd);
  }
  Converter Conv;
  return Conv.convert(T, Source);
}

} // namespace WasmEdge::WAT
