// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/ast/component/export.h - Export class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Export node class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/component/type.h"
#include "common/span.h"

#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// export ::= na:<nameattributes> si:<sortidx> et?:<externtype>?
//          => (export na si et?)

/// AST Component::Export node.
class Export {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  SortIndex &getSortIndex() noexcept { return SortIdx; }
  const SortIndex &getSortIndex() const noexcept { return SortIdx; }
  std::optional<ExternDesc> &getDesc() noexcept { return Desc; }
  const std::optional<ExternDesc> &getDesc() const noexcept { return Desc; }
  std::vector<std::string> &getImplements() noexcept { return Implements; }
  Span<const std::string> getImplements() const noexcept { return Implements; }
  std::vector<std::string> &getExternalIds() noexcept { return ExternalIds; }
  Span<const std::string> getExternalIds() const noexcept {
    return ExternalIds;
  }
  std::vector<std::string> &getVersionSuffixes() noexcept {
    return VersionSuffixes;
  }
  Span<const std::string> getVersionSuffixes() const noexcept {
    return VersionSuffixes;
  }

private:
  std::string Name;
  SortIndex SortIdx;
  std::optional<ExternDesc> Desc;
  std::vector<std::string> Implements;
  std::vector<std::string> ExternalIds;
  std::vector<std::string> VersionSuffixes;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
