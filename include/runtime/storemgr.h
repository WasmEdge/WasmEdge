// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
    for (auto &&Pair : NamedMod) {
      (const_cast<Instance::ModuleInstance *>(Pair.second))->unlinkStore(this);
    }
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
    auto Iter = NamedMod.find(Name);
    if (likely(Iter != NamedMod.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// Find component by name.
  const Instance::ComponentInstance *
  findComponent(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    auto Iter = NamedComp.find(Name);
    if (likely(Iter != NamedComp.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

private:
  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  friend class Executor::Executor;

  /// Register named module into this store.
  Expect<void> registerModule(const Instance::ModuleInstance *ModInst) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedMod.find(ModInst->getModuleName());
    if (likely(Iter != NamedMod.cend())) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedMod.emplace(ModInst->getModuleName(), ModInst);
    // Link the module instance to this store manager.
    (const_cast<Instance::ModuleInstance *>(ModInst))
        ->linkStore(this, [](StoreManager *Store,
                             const Instance::ModuleInstance *Inst) {
          // The unlink callback.
          std::unique_lock CallbackLock(Store->Mutex);
          (Store->NamedMod).erase(std::string(Inst->getModuleName()));
        });
    return {};
  }

  /// Register named component into this store.
  Expect<void> registerComponent(const Instance::ComponentInstance *CompInst) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedComp.find(CompInst->getName());
    if (likely(Iter != NamedComp.cend())) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedComp.emplace(CompInst->getName(), CompInst);
    // Link the component instance to this store manager.
    (const_cast<Instance::ComponentInstance *>(CompInst))
        ->linkStore(this, [](StoreManager *Store,
                             const Instance::ComponentInstance *Inst) {
          // Unlink callback.
          std::unique_lock CallbackLock(Store->Mutex);
          (Store->NamedComp).erase(std::string(Inst->getName()));
        });
    return {};
  }

  /// Collect the instantiation failed module.
  void recycleModule(std::unique_ptr<Instance::ModuleInstance> &&Mod) {
    FailedMod = std::move(Mod);
  }

  /// Collect the instantiation failed module.
  void recycleComponent(std::unique_ptr<Instance::ComponentInstance> &&Comp) {
    FailedComp = std::move(Comp);
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

  std::unique_ptr<Instance::ComponentInstance> FailedComp;
};

} // namespace Runtime
} // namespace WasmEdge
