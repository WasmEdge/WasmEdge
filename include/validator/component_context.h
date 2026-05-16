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
    std::vector<const AST::SubType *> CoreFuncs;       // core:func
    std::vector<const AST::TableType *> CoreTables;    // core:table
    std::vector<const AST::MemoryType *> CoreMemories; // core:memory
    std::vector<const AST::GlobalType *> CoreGlobals;  // core:global
    uint32_t CoreTagCount = 0;                         // core:tag

    // --- Component sort index spaces ---
    std::vector<const AST::Component::Component *> Components; // component
    // Instance exports: name → {sort, optional resolved InstanceType,
    // optional nested instance idx} so alias-export and ascription subtype
    // checks can follow chains without re-deriving from ExternDesc.
    struct InstanceExport {
      AST::Component::Sort::SortType ST;
      const AST::Component::InstanceType *IT;
      std::optional<uint32_t> NestedInstIdx;
    };
    std::vector<std::unordered_map<std::string, InstanceExport>>
        Instances;                                      // instance
    std::vector<const AST::Component::DefType *> Types; // type
    std::vector<const AST::Component::FuncType *>
        Funcs;               // func (element i = FuncType* or nullptr)
    uint32_t ValueCount = 0; // value

    // --- Type annotations (keyed by type index) ---
    std::unordered_map<uint32_t, const AST::Component::InstanceType *>
        InstanceTypes;
    std::unordered_map<uint32_t, const AST::Component::ResourceType *>
        ResourceTypes;

    // Type indices that refer to locally-defined resources (not imported / not
    // aliased from an outer scope). Required by canon resource.new /
    // resource.rep, which are only valid for locally-defined resources.
    std::unordered_set<uint32_t> DefinedResources;

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

  /// Push a new validation scope for a real component.
  void enterComponent(const AST::Component::Component *C) noexcept {
    const Context *Parent = CompCtxs.empty() ? nullptr : &CompCtxs.back();
    CompCtxs.emplace_back(C, Parent);
  }

  /// Push a new validation scope for a type definition
  /// (componenttype, instancetype, or moduletype).
  void enterTypeDefinition() noexcept {
    const Context *Parent = CompCtxs.empty() ? nullptr : &CompCtxs.back();
    CompCtxs.emplace_back(nullptr, Parent);
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
  uint32_t addCoreFunc(const AST::SubType *ST = nullptr) noexcept {
    auto &V = getCurrentContext().CoreFuncs;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(ST);
    return Idx;
  }
  const AST::SubType *getCoreFunc(uint32_t Idx) const noexcept {
    const auto &V = getCurrentContext().CoreFuncs;
    return Idx < V.size() ? V[Idx] : nullptr;
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
  uint32_t addCoreTag() noexcept { return getCurrentContext().CoreTagCount++; }

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

  using InstanceExport = Context::InstanceExport;

  const std::unordered_map<std::string, InstanceExport> &
  getInstance(uint32_t Idx) const noexcept {
    return getCurrentContext().Instances.at(Idx);
  }

  void addInstanceExport(
      uint32_t InstIdx, std::string_view Name,
      AST::Component::Sort::SortType ST,
      const AST::Component::InstanceType *IT = nullptr,
      std::optional<uint32_t> NestedInstIdx = std::nullopt) noexcept {
    getCurrentContext().Instances.at(InstIdx)[std::string(Name)] = {
        ST, IT, NestedInstIdx};
  }

  // ==========================================================================
  // type
  // ==========================================================================

  uint32_t addType(const AST::Component::DefType *DT = nullptr) noexcept {
    return addTypeImpl(DT, /*IsLocal=*/true);
  }

  /// Like addType but marks the resource as imported rather than locally
  /// defined. Used for resource type imports and outer-alias resources.
  uint32_t addTypeImported(const AST::Component::DefType *DT) noexcept {
    return addTypeImpl(DT, /*IsLocal=*/false);
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

  /// Returns true iff the type index is a resource defined in the current
  /// scope (not imported, not an outer-alias). Required for canon
  /// resource.new and resource.rep validation.
  bool isLocalResource(uint32_t Idx) const noexcept {
    const auto &Ctx = getCurrentContext();
    return Ctx.DefinedResources.find(Idx) != Ctx.DefinedResources.end();
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

  uint32_t addFunc(const AST::Component::FuncType *FT = nullptr) noexcept {
    auto &V = getCurrentContext().Funcs;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(FT);
    return Idx;
  }
  const AST::Component::FuncType *getFunc(uint32_t Idx) const noexcept {
    const auto &V = getCurrentContext().Funcs;
    return Idx < V.size() ? V[Idx] : nullptr;
  }
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
  uint32_t addTypeImpl(const AST::Component::DefType *DT,
                       bool IsLocal) noexcept {
    auto &Ctx = getCurrentContext();
    uint32_t Idx = static_cast<uint32_t>(Ctx.Types.size());
    Ctx.Types.push_back(DT);
    if (DT != nullptr) {
      if (DT->isInstanceType()) {
        Ctx.InstanceTypes[Idx] = &DT->getInstanceType();
      } else if (DT->isResourceType()) {
        Ctx.ResourceTypes[Idx] = &DT->getResourceType();
        if (IsLocal) {
          Ctx.DefinedResources.insert(Idx);
        }
      }
    }
    return Idx;
  }

  std::deque<Context> CompCtxs;
};

} // namespace Validator
} // namespace WasmEdge
