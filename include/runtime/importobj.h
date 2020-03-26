// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/importobj.h - Import object interface ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of import object class. Inherit this class
/// to make host module.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "hostfunc.h"
#include "instance/function.h"
#include "instance/table.h"
#include "instance/memory.h"
#include "instance/global.h"

#include <map>
#include <memory>
#include <string>

namespace SSVM {
namespace Runtime {

class ImportObject {
public:
  template <typename T>
  using InstMap = typename std::map<std::string, std::unique_ptr<T>>;

  ImportObject() = delete;
  ImportObject(const std::string &Name) : ModName(Name) {}
  virtual ~ImportObject() = default;

  const std::string &getModuleName() const { return ModName; }

  void addHostFunc(const std::string &Name,
                   std::unique_ptr<HostFunctionBase> &&Func) {
    addHostFunc(Name, Func);
  }
  void addHostFunc(const std::string &Name,
                   std::unique_ptr<HostFunctionBase> &Func) {
    Funcs.emplace(Name,
                  std::make_unique<Runtime::Instance::FunctionInstance>(Func));
  }

  void addHostTable(const std::string &Name,
                    std::unique_ptr<Instance::TableInstance> &&Tab) {
    addHostTable(Name, Tab);
  }
  void addHostTable(const std::string &Name,
                    std::unique_ptr<Instance::TableInstance> &Tab) {
    Tabs.emplace(Name, std::move(Tab));
  }

  void addHostMemory(const std::string &Name,
                     std::unique_ptr<Instance::MemoryInstance> &&Mem) {
    addHostMemory(Name, Mem);
  }
  void addHostMemory(const std::string &Name,
                     std::unique_ptr<Instance::MemoryInstance> &Mem) {
    Mems.emplace(Name, std::move(Mem));
  }

  void addHostGlobal(const std::string &Name,
                     std::unique_ptr<Instance::GlobalInstance> &&Glob) {
    addHostGlobal(Name, Glob);
  }
  void addHostGlobal(const std::string &Name,
                     std::unique_ptr<Instance::GlobalInstance> &Glob) {
    Globs.emplace(Name, std::move(Glob));
  }

  const InstMap<Instance::FunctionInstance> &getFuncs() const { return Funcs; }

  const InstMap<Instance::TableInstance> &getTables() const { return Tabs; }

  const InstMap<Instance::MemoryInstance> &getMems() const { return Mems; }

  const InstMap<Instance::GlobalInstance> &getGlobals() const { return Globs; }

protected:
  const std::string ModName;

  InstMap<Instance::FunctionInstance> Funcs;
  InstMap<Instance::TableInstance> Tabs;
  InstMap<Instance::MemoryInstance> Mems;
  InstMap<Instance::GlobalInstance> Globs;
};

} // namespace Runtime
} // namespace SSVM