// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/storemgr.h - Component store -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Store Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "common/errcode.h"
#include "runtime/instance/component/component.h"

#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// The component model store. It holds the named component instances, the
/// host component functions, and the component definitions.
class StoreManager {
public:
  StoreManager() = default;

  /// Find a component instance by name.
  const Instance::ComponentInstance *find(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto It = NamedInst.find(Name); likely(It != NamedInst.cend())) {
      return It->second;
    }
    return nullptr;
  }

  /// Register a named component instance. A re-registration shadows the
  /// previous instance, which is the wast-runner semantics.
  Expect<void> registerInstance(const Instance::ComponentInstance *CompInst) {
    std::unique_lock Lock(Mutex);
    NamedInst.insert_or_assign(std::string(CompInst->getComponentName()),
                               CompInst);
    return {};
  }

  /// Register a named host component function, for a test harness or an
  /// embedder.
  Expect<void> registerFunction(std::string_view Name,
                                Instance::Component::FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    if (NamedFunc.find(Name) != NamedFunc.cend()) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedFunc.emplace(std::string(Name), Func);
    return {};
  }

  Instance::Component::FunctionInstance *
  findFunction(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto It = NamedFunc.find(Name); It != NamedFunc.cend()) {
      return It->second;
    }
    return nullptr;
  }

  /// Register a named component definition. It supplies the component-sort
  /// imports of a root instantiation, for a test harness or an embedder.
  Expect<void> registerDefinition(std::string_view Name,
                                  const AST::Component::Component *Comp) {
    std::unique_lock Lock(Mutex);
    if (NamedDef.find(Name) != NamedDef.cend()) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedDef.emplace(std::string(Name), Comp);
    return {};
  }

  const AST::Component::Component *findDefinition(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto It = NamedDef.find(Name); It != NamedDef.cend()) {
      return It->second;
    }
    return nullptr;
  }

  /// Drop every registration. The instances themselves are owned elsewhere.
  void reset() noexcept {
    std::unique_lock Lock(Mutex);
    NamedInst.clear();
    NamedFunc.clear();
    NamedDef.clear();
  }

private:
  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  std::map<std::string, const Instance::ComponentInstance *, std::less<>>
      NamedInst;
  std::map<std::string, Instance::Component::FunctionInstance *, std::less<>>
      NamedFunc;
  std::map<std::string, const AST::Component::Component *, std::less<>>
      NamedDef;
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
