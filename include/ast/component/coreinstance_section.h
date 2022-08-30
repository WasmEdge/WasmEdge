// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== CoreInstance Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Core Instance node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/export_section.h"
#include "common/span.h"

#include <cinttypes>
#include <string>
#include <vector>

namespace WasmEdge {
namespace AST {

class CoreInstantiateArg {
public:
  void setName(std::string_view s) noexcept { Name = s; }
  const std::string &getName() const noexcept { return Name; }
  void setIndex(uint32_t I) noexcept { Idx = I; }
  const uint32_t &getIndex() const noexcept { return Idx; }

private:
  std::string Name;
  uint32_t Idx;
};

namespace CoreInstance {

class Instantiate {
public:
  /// Setter/Getter of module index
  void setModuleIdx(uint32_t Idx) noexcept { ModuleIdx = Idx; }
  uint32_t getModuleIdx() const noexcept { return ModuleIdx; }

  /// Getter of instantiate arguments
  Span<const CoreInstantiateArg> getInstantiateArgs() const noexcept {
    return InstantiateArgs;
  }
  std::vector<CoreInstantiateArg> &getInstantiateArgs() noexcept {
    return InstantiateArgs;
  }

private:
  uint32_t ModuleIdx;
  std::vector<CoreInstantiateArg> InstantiateArgs;
};
class Export {
public:
  /// Getter of exports
  Span<const ExportDecl> getExports() const noexcept { return Exports; }
  std::vector<ExportDecl> &getExports() noexcept { return Exports; }

private:
  std::vector<ExportDecl> Exports;
};

using T = std::variant<Instantiate, Export>;

} // namespace CoreInstance

} // namespace AST
} // namespace WasmEdge
