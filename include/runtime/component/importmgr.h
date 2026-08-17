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
#include "common/types.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/module.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// The import source of a nested instantiation. A root instantiation reads
/// the store instead.
class ImportManager {
public:
  /// Export a named component function.
  void exportFunction(std::string_view Name,
                      Instance::ComponentFunctionInstance *Inst) noexcept {
    NamedFunc.emplace(Name, Inst);
  }

  /// Export a named value.
  void exportValue(std::string_view Name, ComponentValVariant V) noexcept {
    NamedValue.insert_or_assign(std::string(Name), std::move(V));
  }

  /// Export a named type with its optional resource identity.
  void exportType(
      std::string_view Name, const AST::Component::DefType *Ty,
      const Instance::Component::ResourceTypeInstance *RT = nullptr) noexcept {
    NamedType.emplace(std::string(Name), std::make_pair(Ty, RT));
  }

  /// Export a named component definition with its lexical environment.
  void exportComponent(std::string_view Name,
                       const AST::Component::Component *C,
                       const Instance::ComponentInstance *Env) noexcept {
    NamedComp.emplace(std::string(Name),
                      Instance::ComponentInstance::ComponentEntry{C, Env});
  }

  /// Export a named component instance.
  void
  exportComponentInstance(std::string_view Name,
                          const Instance::ComponentInstance *Inst) noexcept {
    NamedCompInst.emplace(Name, Inst);
  }

  /// Export a named core module definition.
  void exportCoreModule(std::string_view Name, const AST::Module *M) noexcept {
    NamedCoreMod.emplace(Name, M);
  }

  /// Export a named core module instance.
  void exportCoreModuleInstance(std::string_view Name,
                                const Instance::ModuleInstance *Inst) noexcept {
    NamedCoreModInst.emplace(Name, Inst);
  }

  /// Find the exported entries by name.
  Instance::ComponentFunctionInstance *
  findFunction(std::string_view Name) const noexcept {
    return findExport(NamedFunc, Name);
  }
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto It = NamedValue.find(Name);
    return It != NamedValue.end() ? &It->second : nullptr;
  }
  const AST::Component::DefType *
  findType(std::string_view Name) const noexcept {
    auto It = NamedType.find(Name);
    return It != NamedType.end() ? It->second.first : nullptr;
  }
  const Instance::Component::ResourceTypeInstance *
  findTypeResource(std::string_view Name) const noexcept {
    auto It = NamedType.find(Name);
    return It != NamedType.end() ? It->second.second : nullptr;
  }
  const Instance::ComponentInstance::ComponentEntry *
  findComponentEntry(std::string_view Name) const noexcept {
    auto It = NamedComp.find(Name);
    return It != NamedComp.end() ? &It->second : nullptr;
  }
  const Instance::ComponentInstance *
  findComponentInstance(std::string_view Name) const noexcept {
    return findExport(NamedCompInst, Name);
  }
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findExport(NamedCoreMod, Name);
  }
  const Instance::ModuleInstance *
  findCoreModuleInstance(std::string_view Name) const noexcept {
    return findExport(NamedCoreModInst, Name);
  }

private:
  /// Find export template.
  template <typename T>
  T *findExport(const std::map<std::string, T *, std::less<>> &Map,
                std::string_view ExtName) const noexcept {
    auto Iter = Map.find(ExtName);
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
  std::map<std::string,
           std::pair<const AST::Component::DefType *,
                     const Instance::Component::ResourceTypeInstance *>,
           std::less<>>
      NamedType;
  std::map<std::string, Instance::ComponentInstance::ComponentEntry,
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
