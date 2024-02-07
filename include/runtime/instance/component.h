// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/component.h - Component Instance definition //
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component instance definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "ast/module.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "runtime/instance/module.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <vector>

namespace WasmEdge {

namespace Runtime {

namespace Instance {
class ComponentInstance {
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}

  std::string_view getComponentName() const noexcept { return CompName; }

  void addModule(const AST::Module &M) noexcept { ModList.emplace_back(M); }
  const AST::Module &getModule(uint32_t Index) const noexcept {
    return ModList[Index];
  }

  void addComponent(const AST::Component::Component &C) noexcept {
    CompList.emplace_back(C);
  }
  const AST::Component::Component &getComponent(uint32_t Index) const noexcept {
    return CompList[Index];
  }

  void addModuleInstance(const Instance::ModuleInstance *Inst) noexcept {
    ModInstList.push_back(std::move(Inst));
  }
  void
  addModuleInstance(std::unique_ptr<Instance::ModuleInstance> Inst) noexcept {
    ModInstList.push_back(Inst.get());
    OwnedModInstList.push_back(std::move(Inst));
  }
  const Instance::ModuleInstance *
  getModuleInstance(uint32_t Index) const noexcept {
    if (ModInstList[Index] == nullptr) {
      spdlog::error("oops, null module instance");
    }
    return ModInstList[Index];
  }

  void addComponentInstance(const Instance::ComponentInstance *Inst) noexcept {
    CompInstList.push_back(Inst);
  }
  void addComponentInstance(
      std::unique_ptr<Instance::ComponentInstance> Inst) noexcept {
    CompInstList.push_back(Inst.get());
    OwnedCompInstList.push_back(std::move(Inst));
  }
  const Instance::ComponentInstance *
  getComponentInstance(uint32_t Index) const noexcept {
    return CompInstList[Index];
  }

  void addFunctionInstance(Instance::FunctionInstance *Inst) noexcept {
    FuncInstList.push_back(Inst);
  }
  Instance::FunctionInstance *
  getFunctionInstance(uint32_t Index) const noexcept {
    return FuncInstList[Index];
  }

  void addComponentFunctionInstance(Instance::FunctionInstance *Inst) noexcept {
    CompFuncInstList.push_back(Inst);
  }
  Instance::FunctionInstance *
  getComponentFunctionInstance(uint32_t Index) const noexcept {
    return CompFuncInstList[Index];
  }

  void addExport(std::string_view Name, Instance::FunctionInstance *Inst) {
    ExportFuncMap.emplace(Name, Inst);
  }
  FunctionInstance *findFuncExports(std::string_view Name) const noexcept {
    return ExportFuncMap.at(std::string(Name));
  }

  void addCoreType(const AST::Component::CoreDefType &Ty) {
    CoreTypes.emplace_back(Ty);
  }
  void addType(const AST::Component::DefType &Ty) { Types.emplace_back(Ty); }
  void mapCoreType(std::string_view Name, uint32_t Index) {
    CoreTypeMap[std::string(Name)] = CoreTypes[Index];
  }
  void mapType(std::string_view Name, uint32_t Index) {
    TypeMap[std::string(Name)] = Types[Index];
  }

private:
  std::string CompName;

  std::map<std::string, Instance::FunctionInstance *, std::less<>>
      ExportFuncMap;

  std::vector<AST::Module> ModList;
  std::vector<AST::Component::Component> CompList;

  std::vector<std::unique_ptr<Instance::ModuleInstance>> OwnedModInstList;
  std::vector<const Instance::ModuleInstance *> ModInstList;
  std::vector<std::unique_ptr<Instance::ComponentInstance>> OwnedCompInstList;
  std::vector<const Instance::ComponentInstance *> CompInstList;

  std::vector<Instance::FunctionInstance *> FuncInstList;

  std::vector<Instance::FunctionInstance *> CompFuncInstList;

  std::vector<AST::Component::CoreDefType> CoreTypes;
  std::vector<AST::Component::DefType> Types;

  std::map<std::string, AST::Component::CoreDefType, std::less<>> CoreTypeMap;
  std::map<std::string, AST::Component::DefType, std::less<>> TypeMap;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
