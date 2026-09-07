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
#include "common/span.h"

#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// import         ::= na:<nameattributes> et:<externtype>
//                  => (import na et)
// nameattributes ::= 0x00 len:<u32> en:<externname> => en (if len = |en|)
//                  | 0x01 len:<u32> en:<externname> => en (if len = |en|)
//                  | 0x02 len:<u32> en:<externname> a*:vec(<attribute>)
//                  => en a* (if len = |en|) 🏷️/🔗
// attribute      ::= 0x00 len:<u32> in:<interfacename>
//                  => (implements in) (if len = |in|) 🏷️
//                  | 0x01 len:<u32> vs:<semversuffix>
//                  => (versionsuffix vs) (if len = |vs|) 🔗
//                  | 0x02 n:<name> => (external-id n) 🏷️

/// AST Component::Import node.
class Import {
public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  ExternDesc &getDesc() noexcept { return Desc; }
  const ExternDesc &getDesc() const noexcept { return Desc; }
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
  ExternDesc Desc;
  std::vector<std::string> Implements;
  std::vector<std::string> ExternalIds;
  std::vector<std::string> VersionSuffixes;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
