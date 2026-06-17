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

// import      ::= in:<importname'> ed:<externdesc>       => (import in ed)
// importname' ::= 0x00 len:<u32> in:<importname>         => in
//               | 0x01 len:<u32> in:<importname>         => in
//               | 0x02 len:<u32> in:<importname>
//                 opts:vec(<nameopt>)                    => in opts
// nameopt     ::= 0x00 len:<u32> n:<interfacename>       => (implements n)
//               | 0x01 len:<u32> vs:<semversuffix>      => (versionsuffix vs)

/// AST Component::Import node.
class Import {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  std::string &getVersionSuffix() noexcept { return VersionSuffix; }
  std::string_view getVersionSuffix() const noexcept { return VersionSuffix; }
  bool hasVersionSuffix() const noexcept { return HasVersionSuffix; }
  void setHasVersionSuffix(bool Has) noexcept { HasVersionSuffix = Has; }
  ExternDesc &getDesc() noexcept { return Desc; }
  const ExternDesc &getDesc() const noexcept { return Desc; }

private:
  std::string Name;
  std::string VersionSuffix;
  bool HasVersionSuffix = false;
  ExternDesc Desc;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
