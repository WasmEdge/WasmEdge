// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "common/errinfo.h"

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Validator {

class Context {
public:
  struct ComponentContext {
    const AST::Component::Component *Component;
    std::vector<AST::Module> CoreModules;
    std::vector<AST::Component::Component> ChildComponents;
    std::optional<const ComponentContext *> Parent;
    std::unordered_map<AST::Component::SortCase, size_t> ComponentIndexSizes;
    std::unordered_map<AST::Component::CoreSort, size_t> CoreIndexSizes;
    std::unordered_map<std::string, uint32_t> TypeSubstitutions;

    std::unordered_map<uint32_t, std::unordered_map<std::string_view,
                                                    AST::Component::ExternDesc>>
        ComponentInstanceExports;
    std::unordered_map<uint32_t,
                       std::unordered_map<std::string_view, ExternalType>>
        CoreInstanceExports;
    std::unordered_map<uint32_t, AST::Component::InstanceType>
        ComponentInstanceTypes;
    std::unordered_map<uint32_t, AST::Component::ResourceType>
        ComponentResourceTypes;

    ComponentContext(const AST::Component::Component *C,
                     std::optional<const ComponentContext *> P = std::nullopt)
        : Component(C), CoreModules(), ChildComponents(), Parent(P) {}

    size_t getComponentIndexSize(AST::Component::SortCase SC) const noexcept {
      auto It = ComponentIndexSizes.find(SC);
      if (It != ComponentIndexSizes.end()) {
        return It->second;
      } else {
        return 0;
      }
    }

    size_t getCoreIndexSize(AST::Component::CoreSort CS) const noexcept {
      auto It = CoreIndexSizes.find(CS);
      if (It != CoreIndexSizes.end()) {
        return It->second;
      } else {
        return 0;
      }
    }
  };

  void enterComponent(const AST::Component::Component &C) {
    if (!ComponentContexts.empty()) {
      ComponentContexts.back().ChildComponents.emplace_back(C);
    }

    std::optional<const ComponentContext *> Parent;
    if (ComponentContexts.empty()) {
      Parent = std::nullopt;
    } else {
      Parent = std::optional{&ComponentContexts.back()};
    }

    ComponentContexts.emplace_back(&C, Parent);
  }

  void exitComponent() {
    if (!ComponentContexts.empty()) {
      ComponentContexts.pop_back();
    } else {
      assumingUnreachable();
    }
  }

  ComponentContext &getCurrentContext() { return ComponentContexts.back(); }

  const ComponentContext &getCurrentContext() const {
    return ComponentContexts.back();
  }

  void addCoreModule(const AST::Module &M) {
    getCurrentContext().CoreModules.emplace_back(M);
  }

  const AST::Module &getCoreModule(uint32_t Index) const {
    return getCurrentContext().CoreModules.at(Index);
  }

  size_t getCoreModuleCount() const {
    return getCurrentContext().CoreModules.size();
  }

  const AST::Component::Component &getComponent(uint32_t Index) const {
    return getCurrentContext().ChildComponents.at(Index);
  }

  size_t getComponentCount() const {
    return getCurrentContext().ChildComponents.size();
  }

  std::optional<const ComponentContext *> getParentContext() const {
    if (ComponentContexts.empty()) {
      return std::nullopt;
    }
    return ComponentContexts.back().Parent;
  }

  size_t getComponentIndexSize(AST::Component::SortCase SC) const noexcept {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentIndexSizes.find(SC);
    if (It != Ctx.ComponentIndexSizes.end()) {
      return It->second;
    } else {
      return 0;
    }
  }

  size_t getCoreIndexSize(AST::Component::CoreSort CS) const noexcept {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.CoreIndexSizes.find(CS);
    if (It != Ctx.CoreIndexSizes.end()) {
      return It->second;
    } else {
      return 0;
    }
  }

  void incComponentIndexSize(AST::Component::SortCase SC) noexcept {
    getCurrentContext().ComponentIndexSizes[SC]++;
  }

  void incCoreIndexSize(AST::Component::CoreSort CS) noexcept {
    getCurrentContext().CoreIndexSizes[CS]++;
  }

  void substituteTypeImport(const std::string &ImportName, uint32_t TypeIdx) {
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
    Ctx.ComponentInstanceExports[InstanceIdx][ExportName] = ED;
  }

  std::unordered_map<std::string_view, AST::Component::ExternDesc>
  getComponentInstanceExports(uint32_t InstanceIdx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentInstanceExports.find(InstanceIdx);
    if (It != Ctx.ComponentInstanceExports.end()) {
      return It->second;
    }
    return {};
  }

  size_t getComponentInstanceExportsSize() const {
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
    return static_cast<size_t>(MaxIdx) + 1;
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

  size_t getCoreInstanceExportsSize() const {
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
    return static_cast<size_t>(MaxIdx) + 1;
  }

  void addComponentInstanceType(uint32_t Idx,
                                const AST::Component::InstanceType &IT) {
    auto &Ctx = getCurrentContext();
    Ctx.ComponentInstanceTypes[Idx] = IT;
  }

  const AST::Component::InstanceType *
  getComponentInstanceType(uint32_t Idx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentInstanceTypes.find(Idx);
    if (It != Ctx.ComponentInstanceTypes.end()) {
      return &It->second;
    } else {
      return nullptr;
    }
  }

  void addComponentResourceType(uint32_t Idx,
                                const AST::Component::ResourceType &IT) {
    auto &Ctx = getCurrentContext();
    Ctx.ComponentResourceTypes[Idx] = IT;
  }

  const AST::Component::ResourceType *
  getComponentResourceType(uint32_t Idx) const {
    const auto &Ctx = getCurrentContext();
    auto It = Ctx.ComponentResourceTypes.find(Idx);

    if (It != Ctx.ComponentResourceTypes.end()) {
      return &It->second;
    } else {
      return nullptr;
    }
  }

private:
  std::deque<ComponentContext> ComponentContexts;
};

class ComponentContextGuard {
public:
  ComponentContextGuard(const AST::Component::Component &C, Context &Ctx)
      : Ctx(Ctx) {
    Ctx.enterComponent(C);
  }

  ~ComponentContextGuard() { Ctx.exitComponent(); }

private:
  Context &Ctx;
};

} // namespace Validator
} // namespace WasmEdge
