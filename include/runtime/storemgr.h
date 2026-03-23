// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

  /// Find module by name.
  const Instance::ModuleInstance *findModule(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto Iter = NamedMod.find(Name); likely(Iter != NamedMod.cend())) {
      return Iter->second;
    }
    return nullptr;
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
    std::shared_lock Lock(Mutex);
    for (auto &&[Name, ModInst] : NamedMod) {
      (const_cast<Instance::ModuleInstance *>(ModInst))
          ->unlinkStore(this, Name);
    }
    NamedMod.clear();
  }

  /// Register named module into this store.
  Expect<void> registerModule(const Instance::ModuleInstance *ModInst) {
    return registerModule(ModInst, ModInst->getModuleName());
  }

  /// Register a module instance into this store under the given alias name.
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

  /// Register named component into this store.
  Expect<void> registerComponent(const Instance::ComponentInstance *CompInst) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedComp.find(CompInst->getComponentName());
    if (likely(Iter != NamedComp.cend())) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedComp.emplace(CompInst->getComponentName(), CompInst);
    return {};
  }

private:
  friend class Executor::Executor;

  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  /// Collect the instantiation failed module.
  void recycleModule(std::unique_ptr<Instance::ModuleInstance> &&Mod) {
    FailedMod = std::move(Mod);
  }

  /// \name Module name mapping.
  std::map<std::string, const Instance::ModuleInstance *, std::less<>> NamedMod;
  /// \name Component name mapping.
  std::map<std::string, const Instance::ComponentInstance *, std::less<>>
      NamedComp;

  /// \name Last instantiation failed module.
  /// According to the current spec, the instances should be able to be
  /// referenced even if instantiation failed. Therefore store the failed module
  /// instance here to keep the instances.
  /// FIXME: Is this necessary to be a vector?
  std::unique_ptr<Instance::ModuleInstance> FailedMod;
};

} // namespace Runtime
} // namespace WasmEdge
