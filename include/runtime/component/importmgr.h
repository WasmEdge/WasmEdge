// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/importmgr.h - Import Manager definition ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Import Manager:
/// the named instantiation arguments a nested component or core module
/// instantiation resolves its imports against.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/module.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// The import source of a nested instantiation. A root instantiation reads
/// the store instead. Validation keeps the argument names unique.
class ImportManager {
public:
  /// \name Add the named arguments.
  /// @{
  void addFunction(std::string_view Name,
                   Instance::ComponentFunctionInstance *Inst) noexcept {
    const auto [Iter, Inserted] = NamedFunc.emplace(Name, Inst);
    assuming(Inserted);
  }
  void addValue(std::string_view Name, ComponentValVariant Val) noexcept {
    const auto [Iter, Inserted] =
        NamedValue.emplace(std::string(Name), std::move(Val));
    assuming(Inserted);
  }
  void
  addType(std::string_view Name,
          const Instance::ComponentInstance::TypeDefinition &Def) noexcept {
    const auto [Iter, Inserted] = NamedType.emplace(std::string(Name), Def);
    assuming(Inserted);
  }
  void addComponent(
      std::string_view Name,
      const Instance::ComponentInstance::ComponentDefinition &Def) noexcept {
    const auto [Iter, Inserted] = NamedComp.emplace(std::string(Name), Def);
    assuming(Inserted);
  }
  void addComponentInstance(std::string_view Name,
                            const Instance::ComponentInstance *Inst) noexcept {
    const auto [Iter, Inserted] = NamedCompInst.emplace(Name, Inst);
    assuming(Inserted);
  }
  void addCoreModule(std::string_view Name, const AST::Module *Mod) noexcept {
    const auto [Iter, Inserted] = NamedCoreMod.emplace(Name, Mod);
    assuming(Inserted);
  }
  void addCoreModuleInstance(std::string_view Name,
                             const Instance::ModuleInstance *Inst) noexcept {
    const auto [Iter, Inserted] = NamedCoreModInst.emplace(Name, Inst);
    assuming(Inserted);
  }
  /// @}

  /// \name Find the named arguments.
  /// @{
  Instance::ComponentFunctionInstance *
  findFunction(std::string_view Name) const noexcept {
    return findNamed(NamedFunc, Name);
  }
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto Iter = NamedValue.find(Name);
    return Iter != NamedValue.end() ? &Iter->second : nullptr;
  }
  const Instance::ComponentInstance::TypeDefinition *
  findTypeDefinition(std::string_view Name) const noexcept {
    auto Iter = NamedType.find(Name);
    return Iter != NamedType.end() ? &Iter->second : nullptr;
  }
  const Instance::ComponentInstance::ComponentDefinition *
  findComponentDefinition(std::string_view Name) const noexcept {
    auto Iter = NamedComp.find(Name);
    return Iter != NamedComp.end() ? &Iter->second : nullptr;
  }
  const Instance::ComponentInstance *
  findComponentInstance(std::string_view Name) const noexcept {
    return findNamed(NamedCompInst, Name);
  }
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findNamed(NamedCoreMod, Name);
  }
  const Instance::ModuleInstance *
  findCoreModuleInstance(std::string_view Name) const noexcept {
    return findNamed(NamedCoreModInst, Name);
  }
  /// @}

private:
  /// Find a named pointer entry.
  template <typename T>
  T *findNamed(const std::map<std::string, T *, std::less<>> &Map,
               std::string_view Name) const noexcept {
    auto Iter = Map.find(Name);
    if (likely(Iter != Map.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// \name Data of import manager.
  /// @{
  std::map<std::string, Instance::ComponentFunctionInstance *, std::less<>>
      NamedFunc;
  std::map<std::string, ComponentValVariant, std::less<>> NamedValue;
  std::map<std::string, Instance::ComponentInstance::TypeDefinition,
           std::less<>>
      NamedType;
  std::map<std::string, Instance::ComponentInstance::ComponentDefinition,
           std::less<>>
      NamedComp;
  std::map<std::string, const Instance::ComponentInstance *, std::less<>>
      NamedCompInst;
  std::map<std::string, const AST::Module *, std::less<>> NamedCoreMod;
  std::map<std::string, const Instance::ModuleInstance *, std::less<>>
      NamedCoreModInst;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
