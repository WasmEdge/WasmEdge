// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"
#include "converter.h"
#include "tree_sitter.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iterator>
#include <string>

extern "C" const TSLanguage *tree_sitter_wat();

namespace WasmEdge::WAT {

using namespace std::literals;

Expect<AST::Module> parseWat(std::string_view Source, const Configure &Conf) {
  Parser P(tree_sitter_wat);
  Tree T = P.parse(Source);
  if (T.rootNode().isNull()) {
    // tree-sitter returns a null root for several distinct cases: a parser
    // allocation failure (extremely rare; would normally OOM the process),
    // a TSLanguage ABI mismatch (impossible here -- compile-time pinned to the
    // committed parser.c), and the much more common case of input that ended
    // before a complete top-level (module) form. Log at debug so operators
    // chasing rare infra failures get a breadcrumb without spamming logs on
    // every malformed WAT input (the Loader already reports those with file
    // context).
    spdlog::debug("WAT: tree-sitter returned a null root node "
                  "(truncated input, allocation failure, or ABI mismatch)"sv);
    return Unexpect(ErrCode::Value::WatUnexpectedEnd);
  }
  Converter Conv(Conf);
  return Conv.convert(T, Source);
}

Expect<AST::Module> parseWatFile(const std::filesystem::path &Path,
                                 const Configure &Conf) {
  // Binary mode preserves the exact on-disk bytes (no CRLF->LF translation on
  // Windows), keeping tree-sitter's byte offsets aligned with the file.
  std::ifstream Ifs(Path, std::ios::binary);
  if (!Ifs) {
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  std::string Source((std::istreambuf_iterator<char>(Ifs)),
                     std::istreambuf_iterator<char>());
  return parseWat(Source, Conf);
}

} // namespace WasmEdge::WAT
