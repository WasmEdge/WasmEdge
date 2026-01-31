// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/import.h - Import class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Import node class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/component/type.h"

#include <optional>
#include <string>

namespace WasmEdge {
namespace AST {
namespace Component {

// import      ::= in:<importname'> ed:<externdesc> => (import in ed)
// importname' ::= 0x00 len:<u32> in:<importname>                    => in     (if len = |in|)
//               | 0x01 len:<u32> in:<importname> vs:<versionsuffix'> => in vs  (if len = |in|)

/// AST Component::Import node.
class Import {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  ExternDesc &getDesc() noexcept { return Desc; }
  const ExternDesc &getDesc() const noexcept { return Desc; }
  
  std::optional<std::string> &getVersionSuffix() noexcept { return VersionSuffix; }
  const std::optional<std::string> &getVersionSuffix() const noexcept { return VersionSuffix; }

private:
  std::string Name;
  ExternDesc Desc;
  std::optional<std::string> VersionSuffix;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
