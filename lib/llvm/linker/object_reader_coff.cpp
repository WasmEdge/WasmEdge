// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/object_reader.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Config/llvm-config.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {

std::optional<std::map<std::string, std::string>>
parseCOFFExports(std::string_view Input) {
  constexpr llvm::StringLiteral COFFExportPrefix = "export:";
  constexpr size_t QuotedTokenDelimiterCount = 2;
  llvm::StringRef Directives(Input.data(), Input.size());
  std::map<std::string, std::string> Exports;
  while (!Directives.trim().empty()) {
    Directives = Directives.ltrim();
    llvm::StringRef Token;
    if (Directives.front() == '"') {
      const auto End = Directives.drop_front().find('"');
      if (End == llvm::StringRef::npos) {
        return std::nullopt;
      }
      Token = Directives.substr(1, End);
      Directives = Directives.drop_front(End + QuotedTokenDelimiterCount);
    } else {
      std::tie(Token, Directives) = Directives.split(' ');
    }
    if (Token.empty() || (Token.front() != '/' && Token.front() != '-'))
      continue;
    Token = Token.drop_front();
    if (Token.size() < COFFExportPrefix.size() ||
#if LLVM_VERSION_MAJOR < 13
        !Token.take_front(COFFExportPrefix.size())
             .equals_lower(COFFExportPrefix)) {
#else
        !Token.take_front(COFFExportPrefix.size())
             .equals_insensitive(COFFExportPrefix)) {
#endif
      continue;
    }
    Token = Token.drop_front(COFFExportPrefix.size());
    Token = Token.split(',').first;
    auto [ExportName, SymbolName] = Token.split('=');
    auto Unquote = [](llvm::StringRef &Name) {
      const bool StartsQuoted = !Name.empty() && Name.front() == '"';
      const bool EndsQuoted = !Name.empty() && Name.back() == '"';
      if (StartsQuoted != EndsQuoted)
        return false;
      if (StartsQuoted)
        Name = Name.drop_front().drop_back();
      return true;
    };
    if (!Unquote(ExportName) || !Unquote(SymbolName) || ExportName.empty()) {
      return std::nullopt;
    }
    const auto Target =
        SymbolName.empty() ? ExportName.str() : SymbolName.str();
    if (const auto [Found, Inserted] =
            Exports.emplace(ExportName.str(), Target);
        !Inserted && Found->second != Target) {
      return std::nullopt;
    }
  }
  return Exports;
}

} // namespace Internal

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
