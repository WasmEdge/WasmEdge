// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include <string>

namespace WasmEdge {
namespace AST {
namespace Component {

// export      ::= en:<exportname'> si:<sortidx> ed?:<externdesc>?
//               => (export en si ed?)
// exportname' ::= 0x00 len:<u32> en:<exportname>
//                   => en (if len = |en|)
//               | 0x01 len:<u32> en:<exportname> vs:<versionsuffix'>
//                   => en vs (if len = |en|)

/// AST Component::Export node.
class Export {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  SortIndex &getSortIndex() noexcept { return SortIdx; }
  const SortIndex &getSortIndex() const noexcept { return SortIdx; }
  std::optional<ExternDesc> &getDesc() noexcept { return Desc; }
  const std::optional<ExternDesc> &getDesc() const noexcept { return Desc; }
  
  std::string &getVersionSuffix() noexcept { return VersionSuffix; }
  std::string_view getVersionSuffix() const noexcept { return VersionSuffix; }

private:
  std::string Name;
  SortIndex SortIdx;
  std::optional<ExternDesc> Desc;
  std::string VersionSuffix;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
