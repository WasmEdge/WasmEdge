// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/instantiator.h ------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The state of one component instantiation: the destination instance and
/// where its imports come from.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "runtime/component/importmgr.h"
#include "runtime/component/storemgr.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"
#include "runtime/instance/tag.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {

/// One component instantiation in progress: where its imports resolve, and
/// the index spaces only instantiation reads. The instance keeps what a later
/// call or instantiation can still reach: the types, the components, the
/// core modules, and the exports.
class Instantiator {
public:
  /// Root instantiation.
  Instantiator(Runtime::Component::StoreManager &StoreMgr,
               Runtime::Instance::ComponentInstance &CompInst) noexcept
      : Store(&StoreMgr), Inst(CompInst) {}

  /// Nested instantiation.
  Instantiator(Runtime::Component::ImportManager &Imports,
               Runtime::Instance::ComponentInstance &CompInst) noexcept
      : ImportMgr(&Imports), Inst(CompInst) {}

  /// True for a root instantiation, whose imports the embedder provides.
  bool isRoot() const noexcept { return Store != nullptr; }

  Runtime::Component::StoreManager &getStore() const noexcept { return *Store; }
  Runtime::Component::ImportManager &getImportManager() const noexcept {
    return *ImportMgr;
  }
  Runtime::Instance::ComponentInstance &getInstance() const noexcept {
    return Inst;
  }

  /// \name Index spaces of the instantiation. An add* hands the instance the
  /// ownership; an import* borrows from another instance.
  /// @{
  void addValue(ComponentValVariant V) noexcept {
    Values.push_back(std::move(V));
  }
  Expect<const ComponentValVariant *> getValue(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= Values.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return &Values[Idx];
  }

  void addFunction(std::unique_ptr<Runtime::Instance::ComponentFunctionInstance>
                       &&Func) noexcept {
    Funcs.push_back(Inst.addFunction(std::move(Func)));
  }
  void
  importFunction(Runtime::Instance::ComponentFunctionInstance *Func) noexcept {
    Funcs.push_back(Func);
  }
  Expect<Runtime::Instance::ComponentFunctionInstance *>
  getFunction(uint32_t Idx) const noexcept {
    return getEntry(Funcs, Idx);
  }

  void
  addComponentInstance(std::unique_ptr<Runtime::Instance::ComponentInstance>
                           &&CompInst) noexcept {
    CompInsts.push_back(Inst.addComponentInstance(std::move(CompInst)));
  }
  void importComponentInstance(
      const Runtime::Instance::ComponentInstance *CompInst) noexcept {
    CompInsts.push_back(CompInst);
  }
  Expect<const Runtime::Instance::ComponentInstance *>
  getComponentInstance(uint32_t Idx) const noexcept {
    return getEntry(CompInsts, Idx);
  }

  void addCoreHostFunction(
      std::unique_ptr<Runtime::HostFunctionBase> &&Host) noexcept {
    CoreFuncs.push_back(Inst.addCoreHostFunction(std::move(Host)));
  }
  void importCoreFunction(Runtime::Instance::FunctionInstance *Func) noexcept {
    CoreFuncs.push_back(Func);
  }
  void importCoreTable(Runtime::Instance::TableInstance *Table) noexcept {
    CoreTables.push_back(Table);
  }
  void importCoreMemory(Runtime::Instance::MemoryInstance *Memory) noexcept {
    CoreMemories.push_back(Memory);
  }
  void importCoreGlobal(Runtime::Instance::GlobalInstance *Global) noexcept {
    CoreGlobals.push_back(Global);
  }
  void importCoreTag(Runtime::Instance::TagInstance *Tag) noexcept {
    CoreTags.push_back(Tag);
  }
  Expect<Runtime::Instance::FunctionInstance *>
  getCoreFunction(uint32_t Idx) const noexcept {
    return getEntry(CoreFuncs, Idx);
  }
  Expect<Runtime::Instance::TableInstance *>
  getCoreTable(uint32_t Idx) const noexcept {
    return getEntry(CoreTables, Idx);
  }
  Expect<Runtime::Instance::MemoryInstance *>
  getCoreMemory(uint32_t Idx) const noexcept {
    return getEntry(CoreMemories, Idx);
  }
  Expect<Runtime::Instance::GlobalInstance *>
  getCoreGlobal(uint32_t Idx) const noexcept {
    return getEntry(CoreGlobals, Idx);
  }
  Expect<Runtime::Instance::TagInstance *>
  getCoreTag(uint32_t Idx) const noexcept {
    return getEntry(CoreTags, Idx);
  }

  void addCoreModuleInstance(
      std::unique_ptr<Runtime::Instance::ModuleInstance> &&ModInst) noexcept {
    CoreModInsts.push_back(Inst.addCoreModuleInstance(std::move(ModInst)));
  }
  void importCoreModuleInstance(
      const Runtime::Instance::ModuleInstance *ModInst) noexcept {
    CoreModInsts.push_back(ModInst);
  }
  Expect<const Runtime::Instance::ModuleInstance *>
  getCoreModuleInstance(uint32_t Idx) const noexcept {
    return getEntry(CoreModInsts, Idx);
  }
  /// @}

private:
  /// The pointer at Idx of an index space, checked like the instance's own.
  template <typename T>
  Expect<T> getEntry(const std::vector<T> &Space, uint32_t Idx) const noexcept {
    if (unlikely(Idx >= Space.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return Space[Idx];
  }

  Runtime::Component::StoreManager *Store = nullptr;
  Runtime::Component::ImportManager *ImportMgr = nullptr;
  Runtime::Instance::ComponentInstance &Inst;

  std::vector<ComponentValVariant> Values;
  std::vector<Runtime::Instance::ComponentFunctionInstance *> Funcs;
  std::vector<const Runtime::Instance::ComponentInstance *> CompInsts;
  std::vector<Runtime::Instance::FunctionInstance *> CoreFuncs;
  std::vector<Runtime::Instance::TableInstance *> CoreTables;
  std::vector<Runtime::Instance::MemoryInstance *> CoreMemories;
  std::vector<Runtime::Instance::GlobalInstance *> CoreGlobals;
  std::vector<Runtime::Instance::TagInstance *> CoreTags;
  std::vector<const Runtime::Instance::ModuleInstance *> CoreModInsts;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
