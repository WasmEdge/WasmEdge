// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "validator/component_name.h"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Validator {

/// Component model validation context.
///
/// Index spaces follow the same pattern as core WASM (FormChecker):
/// each sort has a sequential vector where the position IS the index.
/// Imports, aliases, and inline definitions all push_back into the same
/// vector, so indices are assigned in order of appearance.
class ComponentContext {
public:
  // ==========================================================================
  // Type definitions
  // ==========================================================================

  /// Resolved core function signature for type comparison.
  struct FuncSig {
    std::vector<ValType> Params;
    std::vector<ValType> Returns;
    bool operator==(const FuncSig &O) const {
      return Params == O.Params && Returns == O.Returns;
    }
    bool operator!=(const FuncSig &O) const { return !(*this == O); }
  };

  /// Rich type info for core exports/imports (ExternalType + optional sig).
  struct CoreExternInfo {
    ExternalType ExtType = ExternalType::Function;
    std::optional<FuncSig> Sig;
  };

  /// Core module type import entry.
  struct CoreImportInfo {
    std::string ModuleName;
    std::string FieldName;
    CoreExternInfo Type;
  };

  // ==========================================================================
  // Index space entry types
  // ==========================================================================

  /// core:module index space entry.
  struct CoreModuleEntry {
    const AST::Module *InlineMod = nullptr; ///< Non-null for inline modules.
    uint32_t TypeIdx = UINT32_MAX;          ///< Core type index (for imports).
  };

  /// core:instance export map: name → type info.
  using CoreInstanceExports = std::unordered_map<std::string, CoreExternInfo>;

  /// core:type index space entry.
  struct CoreTypeEntry {
    bool IsModuleType = false;
    std::vector<CoreImportInfo> ModuleImports;
    std::vector<std::pair<std::string, CoreExternInfo>> ModuleExports;
  };

  /// instance index space entry.
  struct InstanceEntry {
    std::unordered_map<std::string, const AST::Component::ExternDesc *> Exports;
    /// Owned ExternDescs for exports synthesized without explicit descriptors.
    std::vector<std::unique_ptr<AST::Component::ExternDesc>> OwnedDescs;
    uint64_t TypeSize = 1; ///< Effective type size of this instance.
  };

  /// type index space entry.
  struct TypeEntry {
    const AST::Component::InstanceType *InstType = nullptr;
    const AST::Component::ResourceType *ResType = nullptr;
    bool IsDefValType = false;   ///< True if this type is a DefValType.
    bool IsFuncType = false;     ///< True if this type is a FuncType.
    bool IsComponentType = false; ///< True if this type is a ComponentType.
    uint64_t TypeSize = 1;       ///< Effective type size for limit checking.
  };

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
    std::vector<CoreModuleEntry> CoreModules;       // core:module
    std::vector<CoreInstanceExports> CoreInstances; // core:instance
    std::vector<CoreTypeEntry> CoreTypes;           // core:type
    std::vector<FuncSig> CoreFuncs;                    // core:func
    std::vector<const AST::TableType *> CoreTables;    // core:table
    std::vector<const AST::MemoryType *> CoreMemories; // core:memory
    std::vector<const AST::GlobalType *> CoreGlobals;  // core:global
    std::vector<FuncSig> CoreTags;                     // core:tag

    // --- Component sort index spaces ---
    std::vector<const AST::Component::Component *> Components; // component
    std::vector<InstanceEntry> Instances;   // instance
    std::vector<TypeEntry> Types;           // type
    uint32_t FuncCount = 0;                 // func
    uint32_t ValueCount = 0;                // value

    // --- Validation state ---
    std::unordered_map<std::string, uint32_t> TypeSubstitutions;
    std::unordered_set<std::string> ImportedNames;
    std::unordered_set<std::string> ExportedNames;
    uint64_t ComponentTypeSize = 1; ///< Effective type size of this component.
    std::vector<uint64_t> ComponentTypeSizes; ///< Per nested component type sizes.

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

  void enterComponent(const AST::Component::Component &C) noexcept {
    const Context *Parent = CompCtxs.empty() ? nullptr : &CompCtxs.back();
    CompCtxs.emplace_back(&C, Parent);
  }

  void exitComponent() noexcept {
    assuming(!CompCtxs.empty());
    CompCtxs.pop_back();
  }

  /// Push a fresh scope for component/instance type validation.
  /// Uses nullptr Component but keeps parent linkage for outer aliases.
  void enterTypeScope() noexcept {
    const Context *Parent = CompCtxs.empty() ? nullptr : &CompCtxs.back();
    CompCtxs.emplace_back(nullptr, Parent);
  }

  /// Pop a type validation scope.
  void exitTypeScope() noexcept {
    assuming(!CompCtxs.empty());
    CompCtxs.pop_back();
  }

  /// Get the number of component nesting levels (for outer alias validation).
  uint32_t getDepth() const noexcept {
    return static_cast<uint32_t>(CompCtxs.size());
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
  // Index space size queries (generic, dispatch by sort enum)
  // ==========================================================================

  uint32_t getSortIndexSize(AST::Component::Sort::SortType ST) const noexcept {
    return getCurrentContext().getSortIndexSize(ST);
  }
  uint32_t
  getCoreSortIndexSize(AST::Component::Sort::CoreSortType ST) const noexcept {
    return getCurrentContext().getCoreSortIndexSize(ST);
  }

  // ==========================================================================
  // Index space operations: core:module
  //   Sources: CoreModuleSection (inline), ImportSection (imported),
  //            AliasSection (export/outer alias)
  // ==========================================================================

  /// Push an inline core module. Returns assigned index.
  uint32_t addCoreModule(const AST::Module &M) noexcept {
    auto &V = getCurrentContext().CoreModules;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(CoreModuleEntry{&M, UINT32_MAX});
    return Idx;
  }

  /// Push an imported/aliased core module with its type index.
  /// Returns assigned index.
  uint32_t addCoreModule(uint32_t TypeIdx) noexcept {
    auto &V = getCurrentContext().CoreModules;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(CoreModuleEntry{nullptr, TypeIdx});
    return Idx;
  }

  /// Push a placeholder core module (e.g. outer alias). Returns index.
  uint32_t addCoreModule() noexcept {
    auto &V = getCurrentContext().CoreModules;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  const CoreModuleEntry &getCoreModule(uint32_t Idx) const noexcept {
    return getCurrentContext().CoreModules.at(Idx);
  }

  // ==========================================================================
  // Index space operations: core:instance
  //   Sources: CoreInstanceSection (instantiate/inline-export)
  // ==========================================================================

  /// Push a new core instance. Returns assigned index.
  uint32_t addCoreInstance() noexcept {
    auto &V = getCurrentContext().CoreInstances;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  const CoreInstanceExports &getCoreInstance(uint32_t Idx) const noexcept {
    return getCurrentContext().CoreInstances.at(Idx);
  }

  /// Register an export on an existing core instance.
  void addCoreInstanceExport(uint32_t InstIdx, std::string_view Name,
                             CoreExternInfo Info) {
    getCurrentContext().CoreInstances.at(InstIdx)[std::string(Name)] =
        std::move(Info);
  }
  void addCoreInstanceExport(uint32_t InstIdx, std::string_view Name,
                             ExternalType ET) {
    addCoreInstanceExport(InstIdx, Name, CoreExternInfo{ET, std::nullopt});
  }

  // ==========================================================================
  // Index space operations: core:type
  //   Sources: CoreTypeSection (rectype / moduletype)
  // ==========================================================================

  /// Push a rec type (non-module). Returns assigned index.
  uint32_t addCoreType() noexcept {
    auto &V = getCurrentContext().CoreTypes;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  /// Push a module type with resolved import/export info. Returns index.
  uint32_t addCoreType(
      std::vector<CoreImportInfo> Imports,
      std::vector<std::pair<std::string, CoreExternInfo>> Exports) noexcept {
    auto &V = getCurrentContext().CoreTypes;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    CoreTypeEntry E;
    E.IsModuleType = true;
    E.ModuleImports = std::move(Imports);
    E.ModuleExports = std::move(Exports);
    V.push_back(std::move(E));
    return Idx;
  }

  const CoreTypeEntry &getCoreType(uint32_t Idx) const noexcept {
    return getCurrentContext().CoreTypes.at(Idx);
  }

  // ==========================================================================
  // Index space operations: core:func / core:table / core:memory / core:global
  //   Sources: AliasSection (core export), CanonSection (lower/resource),
  //            CoreInstanceSection (inline export)
  // ==========================================================================

  uint32_t addCoreFunc(FuncSig Sig = {}) noexcept {
    auto &V = getCurrentContext().CoreFuncs;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(std::move(Sig));
    return Idx;
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
  uint32_t addCoreTag(FuncSig Sig = {}) noexcept {
    auto &V = getCurrentContext().CoreTags;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(std::move(Sig));
    return Idx;
  }

  // ==========================================================================
  // Index space operations: component
  //   Sources: ComponentSection (inline), ImportSection, AliasSection
  // ==========================================================================

  /// Push an inline component. Returns assigned index.
  uint32_t addComponent(const AST::Component::Component &C) noexcept {
    auto &V = getCurrentContext().Components;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(&C);
    return Idx;
  }

  /// Push an imported/aliased component (no AST). Returns assigned index.
  uint32_t addComponent() noexcept {
    auto &V = getCurrentContext().Components;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(nullptr);
    return Idx;
  }

  /// Get component pointer at index (nullptr for imported/aliased).
  const AST::Component::Component *getComponent(uint32_t Idx) const noexcept {
    return getCurrentContext().Components.at(Idx);
  }

  // ==========================================================================
  // Index space operations: instance
  //   Sources: InstanceSection, ImportSection, AliasSection, ExportSection
  // ==========================================================================

  /// Push a new component instance. Returns assigned index.
  uint32_t addInstance() noexcept {
    auto &V = getCurrentContext().Instances;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  const InstanceEntry &getInstance(uint32_t Idx) const noexcept {
    return getCurrentContext().Instances.at(Idx);
  }

  /// Register an export on an existing component instance.
  void addInstanceExport(uint32_t InstIdx, std::string_view Name,
                         const AST::Component::ExternDesc &ED) {
    getCurrentContext().Instances.at(InstIdx).Exports[std::string(Name)] = &ED;
  }

  /// Register a synthetic (owned) export on an existing component instance.
  void addSyntheticInstanceExport(uint32_t InstIdx, std::string_view Name,
                                  AST::Component::ExternDesc &&Desc) {
    auto &Inst = getCurrentContext().Instances.at(InstIdx);
    auto Owned = std::make_unique<AST::Component::ExternDesc>(std::move(Desc));
    Inst.Exports[std::string(Name)] = Owned.get();
    Inst.OwnedDescs.emplace_back(std::move(Owned));
  }

  // ==========================================================================
  // Index space operations: type
  //   Sources: TypeSection, ImportSection, AliasSection, ExportSection
  // ==========================================================================

  /// Push a plain type entry. Returns assigned index.
  uint32_t addType() noexcept {
    auto &V = getCurrentContext().Types;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.emplace_back();
    return Idx;
  }

  /// Push a DefValType entry. Returns assigned index.
  uint32_t addDefValType(uint64_t TSize = 1) noexcept {
    auto &V = getCurrentContext().Types;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    TypeEntry E;
    E.IsDefValType = true;
    E.TypeSize = TSize;
    V.push_back(E);
    return Idx;
  }

  /// Push a FuncType entry. Returns assigned index.
  uint32_t addFuncType(uint64_t TSize = 1) noexcept {
    auto &V = getCurrentContext().Types;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    TypeEntry E;
    E.IsFuncType = true;
    E.TypeSize = TSize;
    V.push_back(E);
    return Idx;
  }

  /// Push a ComponentType entry. Returns assigned index.
  uint32_t addComponentType(uint64_t TSize = 1) noexcept {
    auto &V = getCurrentContext().Types;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    TypeEntry E;
    E.IsComponentType = true;
    E.TypeSize = TSize;
    V.push_back(E);
    return Idx;
  }

  /// Push an instance type entry. Returns assigned index.
  uint32_t addType(const AST::Component::InstanceType &IT,
                   uint64_t TSize = 1) noexcept {
    auto &V = getCurrentContext().Types;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(TypeEntry{&IT, nullptr, false, false, false, TSize});
    return Idx;
  }

  /// Push a resource type entry. Returns assigned index.
  uint32_t addType(const AST::Component::ResourceType &RT) noexcept {
    auto &V = getCurrentContext().Types;
    uint32_t Idx = static_cast<uint32_t>(V.size());
    V.push_back(TypeEntry{nullptr, &RT, false, false, false, 1});
    return Idx;
  }

  const TypeEntry &getType(uint32_t Idx) const noexcept {
    return getCurrentContext().Types.at(Idx);
  }

  // ==========================================================================
  // Index space operations: func / value
  //   Counter-only for now.
  //   Sources: ImportSection, CanonSection (lift), AliasSection, ExportSection
  // ==========================================================================

  uint32_t addFunc() noexcept { return getCurrentContext().FuncCount++; }
  uint32_t addValue() noexcept { return getCurrentContext().ValueCount++; }

  // ==========================================================================
  // Generic index space dispatch (for dynamic sort values)
  //
  //   Used when the sort type is only known at runtime, e.g. alias validation
  //   where the sort comes from the alias's sort field.  Each pushes a default
  //   entry into the corresponding index space.
  // ==========================================================================

  uint32_t incSortIndexSize(AST::Component::Sort::SortType ST) noexcept;
  uint32_t incCoreSortIndexSize(AST::Component::Sort::CoreSortType ST) noexcept;

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

  bool AddImportedName(const ComponentName &Name) noexcept {
    return getCurrentContext().AddImportedName(Name);
  }

private:
  std::deque<Context> CompCtxs;
};

} // namespace Validator
} // namespace WasmEdge
