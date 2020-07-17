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
#include "instance/global.h"
#include "instance/memory.h"
#include "instance/table.h"

#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace SSVM {
namespace Runtime {

class ImportObject {
public:
  template <typename T>
  using InstMap = typename std::map<std::string, std::unique_ptr<T>>;

  ImportObject() = delete;
  ImportObject(std::string_view Name) : ModName(Name) {}
  virtual ~ImportObject() = default;

  std::string_view getModuleName() const { return ModName; }

  void addHostFunc(std::string_view Name,
                   std::unique_ptr<HostFunctionBase> &&Func) {
    Funcs.emplace(Name, std::make_unique<Runtime::Instance::FunctionInstance>(
                            std::move(Func)));
  }

  void addHostTable(std::string_view Name,
                    std::unique_ptr<Instance::TableInstance> &&Tab) {
    addHostTable(Name, Tab);
  }
  void addHostTable(std::string_view Name,
                    std::unique_ptr<Instance::TableInstance> &Tab) {
    Tabs.emplace(Name, std::move(Tab));
  }

  void addHostMemory(std::string_view Name,
                     std::unique_ptr<Instance::MemoryInstance> &&Mem) {
    addHostMemory(Name, Mem);
  }
  void addHostMemory(std::string_view Name,
                     std::unique_ptr<Instance::MemoryInstance> &Mem) {
    Mems.emplace(Name, std::move(Mem));
  }

  void addHostGlobal(std::string_view Name,
                     std::unique_ptr<Instance::GlobalInstance> &&Glob) {
    addHostGlobal(Name, Glob);
  }
  void addHostGlobal(std::string_view Name,
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
