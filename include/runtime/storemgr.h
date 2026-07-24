// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/storemgr.h - Store Manager definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Store Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/component/component.h"
#include "runtime/instance/module.h"

#include <mutex>
#include <shared_mutex>
#include <vector>

namespace WasmEdge {

namespace Executor {
class Executor;
}

namespace Runtime {

class StoreManager {
public:
  StoreManager() = default;
  ~StoreManager() {
    // When destroying this store manager, unlink all the registered module
    // instances.
    reset();
  }

  /// Get the length of the list of registered modules.
  uint32_t getModuleListSize() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(NamedMod.size());
  }

  /// Get list of registered modules.
  template <typename CallbackT> auto getModuleList(CallbackT &&CallBack) const {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(NamedMod);
  }

  /// Run Callback with the named module instance (or nullptr) while holding the
  /// shared lock, so the caller can pin a dependency before a concurrent
  /// unregisterModule destroys it.
  template <typename CallbackT>
  auto withModuleLocked(std::string_view Name, CallbackT &&Callback) const {
    return getModuleList([&](const auto &Mods) {
      const Instance::ModuleInstance *Found = nullptr;
      if (auto Iter = Mods.find(Name); likely(Iter != Mods.cend())) {
        Found = Iter->second;
      }
      return std::forward<CallbackT>(Callback)(Found);
    });
  }

  /// Find module by name. Returns the pointer with the lock already released;
  /// use withModuleLocked to pin or dereference the result race-free.
  const Instance::ModuleInstance *findModule(std::string_view Name) const {
    return withModuleLocked(
        Name, [](const Instance::ModuleInstance *Found) { return Found; });
  }

  /// Find component by name.
  const Instance::ComponentInstance *
  findComponent(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto Iter = NamedComp.find(Name); likely(Iter != NamedComp.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// Reset this store manager and unlink all the registered module instances.
  void reset() noexcept {
    std::unique_lock Lock(Mutex);
    for (auto &&[Name, ModInst] : NamedMod) {
      (const_cast<Instance::ModuleInstance *>(ModInst))
          ->unlinkStore(this, Name);
    }
    NamedMod.clear();
    NamedComp.clear();
  }

  /// Register a named module in this store.
  Expect<void> registerModule(const Instance::ModuleInstance *ModInst) {
    return registerModule(ModInst, ModInst->getModuleName());
  }

  /// Register a module instance in this store under the given alias name.
  Expect<void> registerModule(const Instance::ModuleInstance *ModInst,
                              std::string_view Name) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedMod.find(Name);
    if (likely(Iter != NamedMod.cend())) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedMod.emplace(std::string(Name), ModInst);
    // Link the module instance to this store manager.
    (const_cast<Instance::ModuleInstance *>(ModInst))
        ->linkStore(this, Name,
                    [](const Instance::ModuleInstance::LinkedStoreKey &Key,
                       const Instance::ModuleInstance *) {
                      // The unlink callback: erase the alias name from the
                      // store.
                      std::unique_lock CallbackLock(Key.first->Mutex);
                      (Key.first->NamedMod).erase(Key.second);
                    });
    return {};
  }

  /// Unregister a named module from this store.
  Expect<void> unregisterModule(std::string_view Name) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedMod.find(Name);
    if (Iter == NamedMod.cend()) {
      return Unexpect(ErrCode::Value::UnknownImport);
    }
    (const_cast<Instance::ModuleInstance *>(Iter->second))
        ->unlinkStore(this, Name);
    NamedMod.erase(Iter);
    return {};
  }

  /// Register a named component in this store.
  /// Register a named host component function (test harnesses / embedders).
  Expect<void>
  registerComponentFunction(std::string_view Name,
                            Instance::Component::FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    if (NamedCompFunc.find(Name) != NamedCompFunc.cend()) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedCompFunc.emplace(std::string(Name), Func);
    return {};
  }

  Instance::Component::FunctionInstance *
  findComponentFunction(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto It = NamedCompFunc.find(Name); It != NamedCompFunc.cend()) {
      return It->second;
    }
    return nullptr;
  }

  /// Register a named component definition; provides component-sort imports
  /// of root instantiations (test harnesses / embedders).
  Expect<void>
  registerComponentDefinition(std::string_view Name,
                              const AST::Component::Component *Comp) {
    std::unique_lock Lock(Mutex);
    if (NamedCompDef.find(Name) != NamedCompDef.cend()) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedCompDef.emplace(std::string(Name), Comp);
    return {};
  }

  const AST::Component::Component *
  findComponentDefinition(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto It = NamedCompDef.find(Name); It != NamedCompDef.cend()) {
      return It->second;
    }
    return nullptr;
  }

  Expect<void> registerComponent(const Instance::ComponentInstance *CompInst) {
    std::unique_lock Lock(Mutex);
    // Re-registration shadows the previous instance (wast-runner semantics:
    // later same-named definitions replace earlier ones).
    NamedComp.insert_or_assign(std::string(CompInst->getComponentName()),
                               CompInst);
    return {};
  }

private:
  friend class Executor::Executor;

  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  /// Collect the instantiation failed module.
  void recycleModule(std::unique_ptr<Instance::ModuleInstance> &&Mod) {
    if (FailedMod) {
      auto *OldMod = FailedMod.release();
      if (OldMod) {
        OldMod->terminate();
      }
    }
    FailedMod = std::move(Mod);
  }

  /// \name Module name mapping.
  std::map<std::string, const Instance::ModuleInstance *, std::less<>> NamedMod;
  /// \name Component name mapping.
  std::map<std::string, const Instance::ComponentInstance *, std::less<>>
      NamedComp;
  std::map<std::string, Instance::Component::FunctionInstance *, std::less<>>
      NamedCompFunc;
  std::map<std::string, const AST::Component::Component *, std::less<>>
      NamedCompDef;

  /// \name Last instantiation failed module.
  /// According to the current spec, instances should remain referenceable even
  /// if instantiation failed. Therefore, store the failed module instance here
  /// to keep the instances alive.
  /// FIXME: Is this necessary to be a vector?
  std::unique_ptr<Instance::ModuleInstance> FailedMod;
};

} // namespace Runtime
} // namespace WasmEdge
