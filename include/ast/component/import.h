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

#include "ast/component/extern_name.h"
#include "ast/component/sort.h"
#include "ast/component/type.h"

#include <optional>
#include <string>

namespace WasmEdge {
namespace AST {
namespace Component {

// import      ::= in:<importname'> ed:<externdesc> => (import in ed)
// importname' ::= 0x00|0x01 len:<u32> in:<importname>  => in
//               | 0x02 len:<u32> in:<importname> opts:vec(<nameopt>) => in opts

/// AST Component::Import node.
class Import {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  ExternDesc &getDesc() noexcept { return Desc; }
  const ExternDesc &getDesc() const noexcept { return Desc; }
  std::optional<std::string_view> getVersionSuffix() const noexcept {
    return Annotations.getVersionSuffix();
  }
  std::optional<std::string> &getVersionSuffixMut() noexcept {
    return Annotations.getVersionSuffixMut();
  }

private:
  std::string Name;
  ExternDesc Desc;
  ExternNameAnnotations Annotations;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
