// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/component.h"
#include "ast/module.h"

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Validator {

class ComponentContext {
public:
  struct Context {
    const AST::Component::Component *Component;
    std::vector<const AST::Module *> CoreModules;
    std::vector<const AST::Component::Component *> ChildComponents;
    const Context *Parent;
    std::vector<uint32_t> SortIndexSizes;
    std::vector<uint32_t> CoreSortIndexSizes;
    std::unordered_map<std::string, uint32_t> TypeSubstitutions;
    std::unordered_map<uint32_t,
                       std::unordered_map<std::string_view,
                                          const AST::Component::ExternDesc *>>
        ComponentInstanceExports;
    std::unordered_map<uint32_t,
                       std::unordered_map<std::string_view, ExternalType>>
        CoreInstanceExports;
    std::unordered_map<uint32_t, const AST::Component::InstanceType *>
        ComponentInstanceTypes;
    std::unordered_map<uint32_t, const AST::Component::ResourceType *>
        ComponentResourceTypes;

    Context(const AST::Component::Component *C,
            const Context *P = nullptr) noexcept
        : Component(C), Parent(P), SortIndexSizes(static_cast<uint32_t>(
                                       AST::Component::Sort::SortType::Max)),
          CoreSortIndexSizes(
              static_cast<uint32_t>(AST::Component::Sort::CoreSortType::Max)) {}

    uint32_t
    getSortIndexSize(const AST::Component::Sort::SortType ST) const noexcept {
      return SortIndexSizes[static_cast<uint32_t>(ST)];
    }

    uint32_t getCoreSortIndexSize(
        const AST::Component::Sort::CoreSortType ST) const noexcept {
      return CoreSortIndexSizes[static_cast<uint32_t>(ST)];
    }
  };

  void enterComponent(const AST::Component::Component &C) noexcept {
    const Context *Parent = nullptr;
    if (!CompCtxs.empty()) {
      CompCtxs.back().ChildComponents.emplace_back(&C);
      Parent = &CompCtxs.back();
    }
    CompCtxs.emplace_back(&C, Parent);
  }

  void exitComponent() noexcept {
    assuming(!CompCtxs.empty());
    if (!CompCtxs.empty()) {
      CompCtxs.pop_back();
    }
  }

  Context &getCurrentContext() noexcept {
    assuming(!CompCtxs.empty());
    return CompCtxs.back();
  }

  const Context &getCurrentContext() const noexcept {
    assuming(!CompCtxs.empty());
    return CompCtxs.back();
  }

  void addCoreModule(const AST::Module &M) noexcept {
    getCurrentContext().CoreModules.emplace_back(&M);
  }

  const AST::Module &getCoreModule(const uint32_t Index) const noexcept {
    return *getCurrentContext().CoreModules.at(Index);
  }

  uint32_t getCoreModuleCount() const noexcept {
    return static_cast<uint32_t>(getCurrentContext().CoreModules.size());
  }

  const AST::Component::Component &
  getComponent(const uint32_t Index) const noexcept {
    return *getCurrentContext().ChildComponents.at(Index);
  }

  uint32_t getComponentCount() const noexcept {
    return static_cast<uint32_t>(getCurrentContext().ChildComponents.size());
  }

  const Context *getParentContext() const noexcept {
    return getCurrentContext().Parent;
  }

  uint32_t
  getSortIndexSize(const AST::Component::Sort::SortType ST) const noexcept {
    return getCurrentContext().getSortIndexSize(ST);
  }

  uint32_t getCoreSortIndexSize(
      const AST::Component::Sort::CoreSortType ST) const noexcept {
    return getCurrentContext().getCoreSortIndexSize(ST);
  }

  void incSortIndexSize(const AST::Component::Sort::SortType ST) noexcept {
    getCurrentContext().SortIndexSizes[static_cast<uint32_t>(ST)]++;
  }

  void
  incCoreSortIndexSize(const AST::Component::Sort::CoreSortType ST) noexcept {
    getCurrentContext().CoreSortIndexSizes[static_cast<uint32_t>(ST)]++;
  }

  void substituteTypeImport(const std::string &ImportName,
                            const uint32_t TypeIdx) noexcept {
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

  void addComponentInstanceExport(uint32_t InstanceIdx,
                                  const std::string_view &ExportName,
                                  const AST::Component::ExternDesc &ED) {
    auto &Ctx = getCurrentContext();
    Ctx.ComponentInstanceExports[InstanceIdx][ExportName] = &ED;
  }

  std::unordered_map<std::string_view, const AST::Component::ExternDesc *>
  getComponentInstanceExports(uint32_t InstanceIdx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentInstanceExports.find(InstanceIdx);
    if (It != Ctx.ComponentInstanceExports.end()) {
      return It->second;
    }
    return {};
  }

  uint32_t getComponentInstanceExportsSize() const {
    const auto &Exports = getCurrentContext().ComponentInstanceExports;
    if (Exports.empty()) {
      return 0;
    }

    uint32_t MaxIdx = 0;
    for (const auto &Pair : Exports) {
      if (Pair.first > MaxIdx) {
        MaxIdx = Pair.first;
      }
    }
    return MaxIdx + 1;
  }

  void addCoreInstanceExport(uint32_t InstanceIdx,
                             const std::string_view &ExportName,
                             const ExternalType &ET) {
    auto &Ctx = getCurrentContext();
    Ctx.CoreInstanceExports[InstanceIdx][ExportName] = ET;
  }

  std::unordered_map<std::string_view, ExternalType>
  getCoreInstanceExports(uint32_t InstanceIdx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.CoreInstanceExports.find(InstanceIdx);
    if (It != Ctx.CoreInstanceExports.end()) {
      return It->second;
    }
    return {};
  }

  uint32_t getCoreInstanceExportsSize() const {
    const auto &Exports = getCurrentContext().CoreInstanceExports;
    if (Exports.empty()) {
      return 0;
    }

    uint32_t MaxIdx = 0;
    for (const auto &Pair : Exports) {
      if (Pair.first > MaxIdx) {
        MaxIdx = Pair.first;
      }
    }
    return MaxIdx + 1;
  }

  void addComponentInstanceType(uint32_t Idx,
                                const AST::Component::InstanceType &IT) {
    auto &Ctx = getCurrentContext();
    Ctx.ComponentInstanceTypes[Idx] = &IT;
  }

  const AST::Component::InstanceType *
  getComponentInstanceType(uint32_t Idx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentInstanceTypes.find(Idx);
    if (It != Ctx.ComponentInstanceTypes.end()) {
      return It->second;
    } else {
      return nullptr;
    }
  }

  void addComponentResourceType(uint32_t Idx,
                                const AST::Component::ResourceType &IT) {
    auto &Ctx = getCurrentContext();
    Ctx.ComponentResourceTypes[Idx] = &IT;
  }

  const AST::Component::ResourceType *
  getComponentResourceType(uint32_t Idx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentResourceTypes.find(Idx);

    if (It != Ctx.ComponentResourceTypes.end()) {
      return It->second;
    } else {
      return nullptr;
    }
  }

private:
  std::deque<Context> CompCtxs;
};

} // namespace Validator
} // namespace WasmEdge
