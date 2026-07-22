// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
// importname' ::= 0x00 len:<u32> in:<importname>   => in  (if len = |in|)

/// AST Component::Import node.
class Import {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  ExternDesc &getDesc() noexcept { return Desc; }
  const ExternDesc &getDesc() const noexcept { return Desc; }
  std::vector<std::string> &getImplements() noexcept { return Implements; }
  Span<const std::string> getImplements() const noexcept { return Implements; }

private:
  std::string Name;
  ExternDesc Desc;
  std::vector<std::string> Implements;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
