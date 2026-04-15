// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "validator/component_name.h"

#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Validator {

class ComponentContext {
public:
  // ==========================================================================
  // Validation context (one per component scope)
  // ==========================================================================

  struct Context {
    Context(const AST::Component::Component *C,
            const Context *P = nullptr) noexcept
        : Component(C), Parent(P) {}

    const AST::Component::Component *Component;
    const Context *Parent;

    // --- Core sort index spaces ---
    std::vector<const AST::Module *> CoreModules; // core:module
    std::vector<std::unordered_map<std::string, ExternalType>>
        CoreInstances;                                 // core:instance
    std::vector<const AST::SubType *> CoreTypes;       // core:type
    uint32_t CoreFuncCount = 0;                        // core:func
    std::vector<const AST::TableType *> CoreTables;    // core:table
    std::vector<const AST::MemoryType *> CoreMemories; // core:memory
    std::vector<const AST::GlobalType *> CoreGlobals;  // core:global

    // --- Component sort index spaces ---
    std::vector<const AST::Component::Component *> Components; // component
    std::vector<std::unordered_map<std::string,
                                   const AST::Component::ExternDesc *>>
        Instances;                                      // instance
    std::vector<const AST::Component::DefType *> Types; // type
    uint32_t FuncCount = 0;                             // func
    uint32_t ValueCount = 0;                            // value

    // --- Type annotations (keyed by type index) ---
    std::unordered_map<uint32_t, const AST::Component::InstanceType *>
        InstanceTypes;
    std::unordered_map<uint32_t, const AST::Component::ResourceType *>
        ResourceTypes;

    // --- Validation state ---
    std::unordered_map<std::string, uint32_t> TypeSubstitutions;
    std::unordered_set<std::string> ImportedNames;
    std::unordered_set<std::string> ExportedNames;

    // Size queries (used by outer alias validation on parent contexts).
    uint32_t getSortIndexSize(AST::Component::Sort::SortType ST) const noexcept;
    uint32_t
    getCoreSortIndexSize(AST::Component::Sort::CoreSortType ST) const noexcept;

    bool AddImportedName(const ComponentName &Name) noexcept;
  };

  // ==========================================================================
  // Context stack management
  // ==========================================================================

  void reset() noexcept { CompCtxs.clear(); }

  /// Push a new validation scope. Pass a Component for real components,
  /// or nullptr for componenttype/instancetype type definition scopes.
  void enterComponent(const AST::Component::Component *C = nullptr) noexcept {
    const Context *Parent = CompCtxs.empty() ? nullptr : &CompCtxs.back();
    CompCtxs.emplace_back(C, Parent);
  }

  void exitComponent() noexcept {
    assuming(!CompCtxs.empty());
    CompCtxs.pop_back();
  }

  /// Returns true if the current scope is a type definition scope
  /// (componenttype or instancetype), not a real component scope.
  bool isTypeDefinitionScope() const noexcept {
    return !CompCtxs.empty() && CompCtxs.back().Component == nullptr;
  }

  Context &getCurrentContext() noexcept {
    assuming(!CompCtxs.empty());
    return CompCtxs.back();
  }
  const Context &getCurrentContext() const noexcept {
    assuming(!CompCtxs.empty());
    return CompCtxs.back();
  }

  // ==========================================================================
  // Index space size queries (generic dispatch by sort enum)
  // ==========================================================================

  uint32_t getSortIndexSize(AST::Component::Sort::SortType ST) const noexcept {
    return getCurrentContext().getSortIndexSize(ST);
  }
  uint32_t
  getCoreSortIndexSize(AST::Component::Sort::CoreSortType ST) const noexcept {
    return getCurrentContext().getCoreSortIndexSize(ST);
  }

  // ==========================================================================
  // Generic index space increment (for dynamic sort values)
  // ==========================================================================

  uint32_t incSortIndexSize(AST::Component::Sort::SortType ST) noexcept;
  uint32_t incCoreSortIndexSize(AST::Component::Sort::CoreSortType ST) noexcept;

  // ==========================================================================
  // core:module
  // ==========================================================================

  uint32_t addCoreModule(const AST::Module &M) noexcept {
    auto &V = getCurrentContext().CoreModules;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(&M);
    return Idx;
  }

  uint32_t addCoreModule() noexcept {
    auto &V = getCurrentContext().CoreModules;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(nullptr);
    return Idx;
  }

  const AST::Module *getCoreModule(uint32_t Idx) const noexcept {
    return getCurrentContext().CoreModules.at(Idx);
  }

  // ==========================================================================
  // core:instance
  // ==========================================================================

  uint32_t addCoreInstance() noexcept {
    auto &V = getCurrentContext().CoreInstances;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  const std::unordered_map<std::string, ExternalType> &
  getCoreInstance(uint32_t Idx) const noexcept {
    return getCurrentContext().CoreInstances.at(Idx);
  }

  void addCoreInstanceExport(uint32_t InstIdx, std::string_view Name,
                             ExternalType ET) {
    getCurrentContext().CoreInstances.at(InstIdx)[std::string(Name)] = ET;
  }

  // ==========================================================================
  // core:type / core:func / core:table / core:memory / core:global / core:tag
  // ==========================================================================

  uint32_t addCoreType(const AST::SubType *ST = nullptr) noexcept {
    auto &V = getCurrentContext().CoreTypes;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(ST);
    return Idx;
  }
  uint32_t addCoreFunc() noexcept {
    return getCurrentContext().CoreFuncCount++;
  }

  uint32_t addCoreTable(const AST::TableType *TT = nullptr) noexcept {
    auto &V = getCurrentContext().CoreTables;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(TT);
    return Idx;
  }
  uint32_t addCoreMemory(const AST::MemoryType *MT = nullptr) noexcept {
    auto &V = getCurrentContext().CoreMemories;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(MT);
    return Idx;
  }
  uint32_t addCoreGlobal(const AST::GlobalType *GT = nullptr) noexcept {
    auto &V = getCurrentContext().CoreGlobals;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(GT);
    return Idx;
  }

  // ==========================================================================
  // component
  // ==========================================================================

  uint32_t addComponent(const AST::Component::Component &C) noexcept {
    auto &V = getCurrentContext().Components;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(&C);
    return Idx;
  }

  uint32_t addComponent() noexcept {
    auto &V = getCurrentContext().Components;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(nullptr);
    return Idx;
  }

  const AST::Component::Component *getComponent(uint32_t Idx) const noexcept {
    return getCurrentContext().Components.at(Idx);
  }

  // ==========================================================================
  // instance
  // ==========================================================================

  uint32_t addInstance() noexcept {
    auto &V = getCurrentContext().Instances;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  const std::unordered_map<std::string, const AST::Component::ExternDesc *> &
  getInstance(uint32_t Idx) const noexcept {
    return getCurrentContext().Instances.at(Idx);
  }

  void addInstanceExport(uint32_t InstIdx, std::string_view Name,
                         const AST::Component::ExternDesc &ED) {
    getCurrentContext().Instances.at(InstIdx)[std::string(Name)] = &ED;
  }

  // ==========================================================================
  // type
  // ==========================================================================

  uint32_t addType(const AST::Component::DefType *DT = nullptr) noexcept {
    auto &Ctx = getCurrentContext();
    uint32_t Idx = static_cast<uint32_t>(Ctx.Types.size());
    Ctx.Types.push_back(DT);
    if (DT != nullptr) {
      if (DT->isInstanceType()) {
        Ctx.InstanceTypes[Idx] = &DT->getInstanceType();
      }
      if (DT->isResourceType()) {
        Ctx.ResourceTypes[Idx] = &DT->getResourceType();
      }
    }
    return Idx;
  }

  const AST::Component::DefType *getDefType(uint32_t Idx) const noexcept {
    const auto &Ctx = getCurrentContext();
    if (Idx < Ctx.Types.size()) {
      return Ctx.Types[Idx];
    }
    return nullptr;
  }

  bool isResourceType(uint32_t Idx) const noexcept {
    const auto &Ctx = getCurrentContext();
    return Ctx.ResourceTypes.find(Idx) != Ctx.ResourceTypes.end();
  }

  const AST::Component::InstanceType *
  getInstanceType(uint32_t Idx) const noexcept {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.InstanceTypes.find(Idx);
    return It != Ctx.InstanceTypes.end() ? It->second : nullptr;
  }

  const AST::Component::ResourceType *
  getResourceType(uint32_t Idx) const noexcept {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ResourceTypes.find(Idx);
    return It != Ctx.ResourceTypes.end() ? It->second : nullptr;
  }

  // ==========================================================================
  // func / value
  // ==========================================================================

  uint32_t addFunc() noexcept { return getCurrentContext().FuncCount++; }
  uint32_t addValue() noexcept { return getCurrentContext().ValueCount++; }

  // ==========================================================================
  // Validation state
  // ==========================================================================

  void substituteTypeImport(const std::string &ImportName,
                            uint32_t TypeIdx) noexcept {
    getCurrentContext().TypeSubstitutions[ImportName] = TypeIdx;
  }

  std::optional<uint32_t>
  getSubstitutedType(const std::string &ImportName) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.TypeSubstitutions.find(ImportName);
    if (It != Ctx.TypeSubstitutions.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  bool addImportedName(const ComponentName &Name) noexcept {
    return getCurrentContext().AddImportedName(Name);
  }

  /// Returns false if the export name already exists (duplicate).
  bool addExportedName(std::string_view Name) noexcept {
    return getCurrentContext().ExportedNames.emplace(Name).second;
  }

private:
  std::deque<Context> CompCtxs;
};

} // namespace Validator
} // namespace WasmEdge
