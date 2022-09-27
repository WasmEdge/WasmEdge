// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- Component Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "map"
#include "module.h"
#include "shared_mutex"

namespace WasmEdge {

namespace Executor {
class Executor;
} // namespace Executor

namespace Runtime {

class StoreManager;

namespace Instance {

class ComponentInstance {
  mutable std::shared_mutex Mutex;
  const std::string CompName;

  std::vector<ComponentInstance *> Insts;
  std::vector<ModuleInstance *> CoreInsts;

  std::vector<AST::CoreType> CoreTypes;
  std::vector<AST::Type> Types;

  std::vector<FunctionInstance *> FuncInsts;

public:
  ComponentInstance(std::string_view Name) : CompName{Name} {}

  std::string_view getName() const noexcept {
    std::shared_lock Lock(Mutex);
    return CompName;
  }

  void initInstance() { Insts = {}; }
  void addInstance(ComponentInstance *I) { Insts.push_back(I); }

  void initCoreInstance() { CoreInsts = {}; }
  void addCoreInstance(ModuleInstance *I) { CoreInsts.push_back(I); }

  /// Copy the function types in type section to this module instance.
  void addCoreType(const AST::CoreType &T) {
    std::unique_lock Lock(Mutex);
    CoreTypes.emplace_back(T);
  }
  void addType(const AST::Type &T) {
    std::unique_lock Lock(Mutex);
    Types.emplace_back(T);
  }

  FunctionInstance *getStartFunc(uint32_t Idx) { return FuncInsts[Idx]; }

  friend class Runtime::StoreManager;
  using BeforeModuleDestroyCallback = void(StoreManager *Store,
                                           const ComponentInstance *Comp);
  void linkStore(StoreManager *Store, BeforeModuleDestroyCallback Callback) {
    // Link to store when registration.
    std::unique_lock Lock(Mutex);
    LinkedStore.insert_or_assign(Store, Callback);
  }

  ComponentInstance *getInstance(uint32_t I) { return Insts[I]; }
  ModuleInstance *getCoreInstance(uint32_t I) { return CoreInsts[I]; }
  const AST::CoreType &getCoreType(uint32_t I) { return CoreTypes[I]; }
  const AST::Type &getType(uint32_t I) { return Types[I]; }

private:
  std::map<StoreManager *, std::function<BeforeModuleDestroyCallback>>
      LinkedStore;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
