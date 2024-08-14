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

#include "runtime/instance/component.h"
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
    if (auto Iter = SoftNamedMod.find(Name);
        likely(Iter != SoftNamedMod.cend())) {
      return Iter->second;
    }
    if (auto Iter = NamedMod.find(Name); likely(Iter != NamedMod.cend())) {
      return Iter->second;
    }
    return nullptr;
  }
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
    for (auto &&Pair : NamedMod) {
      (const_cast<Instance::ModuleInstance *>(Pair.second))->unlinkStore(this);
    }
    NamedMod.clear();
  }

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

  void addNamedModule(std::string_view Name,
                      const Instance::ModuleInstance *Inst) {
    std::unique_lock Lock(Mutex);
    SoftNamedMod.emplace(Name, Inst);
  }

  Expect<void> registerComponent(std::string_view Name,
                                 const Instance::ComponentInstance *Inst) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedComp.find(Name);
    if (likely(Iter != NamedComp.cend())) {
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
    NamedComp.emplace(Name, Inst);
    return {};
  }
  Expect<void> registerComponent(const Instance::ComponentInstance *Inst) {
    return registerComponent(Inst->getComponentName(), Inst);
  }

private:
  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  friend class Executor::Executor;

  /// Collect the instantiation failed module.
  void recycleModule(std::unique_ptr<Instance::ModuleInstance> &&Mod) {
    FailedMod = std::move(Mod);
  }

  /// \name Module name mapping.
  std::map<std::string, const Instance::ModuleInstance *, std::less<>> NamedMod;
  /// \name Soft module are those temporary added by component linking process
  std::map<std::string, const Instance::ModuleInstance *, std::less<>>
      SoftNamedMod;
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
