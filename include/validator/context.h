// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "ast/module.h"
#include "common/errinfo.h"

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

    ComponentContext(const AST::Component::Component *C,
                     std::optional<const ComponentContext *> P = std::nullopt)
        : Component(C), CoreModules(), ChildComponents(), Parent(P) {}
  };

  void enterComponent(const AST::Component::Component &C) {
    if (!ComponentContexts.empty()) {
      ComponentContexts.back().ChildComponents.emplace_back(C);
    }
    std::optional<const ComponentContext *> Parent =
        ComponentContexts.empty() ? std::nullopt
                                  : std::optional{&ComponentContexts.back()};
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
    return ComponentContexts.empty() ? std::nullopt
                                     : ComponentContexts.back().Parent;
  }

  size_t getComponentIndexSize(AST::Component::SortCase SC) const noexcept {
    auto It = getCurrentContext().ComponentIndexSizes.find(SC);
    return (It != getCurrentContext().ComponentIndexSizes.end()) ? It->second
                                                                 : 0;
  }

  size_t getCoreIndexSize(AST::Component::CoreSort CS) const noexcept {
    auto It = getCurrentContext().CoreIndexSizes.find(CS);
    return (It != getCurrentContext().CoreIndexSizes.end()) ? It->second : 0;
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
    auto It = getCurrentContext().TypeSubstitutions.find(ImportName);
    if (It != getCurrentContext().TypeSubstitutions.end()) {
      return It->second;
    }
    return std::nullopt;
  }

private:
  std::vector<ComponentContext> ComponentContexts;
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
