// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/storemgr.h - Store Manager definition ---------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Store Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "instance/data.h"
#include "instance/elem.h"
#include "instance/function.h"
#include "instance/global.h"
#include "instance/memory.h"
#include "instance/module.h"
#include "instance/table.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace SSVM {
namespace Runtime {

namespace {
/// Return true if T is entities which can be imported.
template <typename T>
inline constexpr const bool IsImportEntityV =
    std::is_same_v<T, Instance::FunctionInstance> ||
    std::is_same_v<T, Instance::TableInstance> ||
    std::is_same_v<T, Instance::MemoryInstance> ||
    std::is_same_v<T, Instance::GlobalInstance>;

/// Return true if T is entities.
template <typename T>
inline constexpr const bool IsEntityV =
    IsImportEntityV<T> || std::is_same_v<T, Instance::ElementInstance> ||
    std::is_same_v<T, Instance::DataInstance>;

/// Return true if T is instances.
template <typename T>
inline constexpr const bool IsInstanceV =
    IsEntityV<T> || std::is_same_v<T, Instance::ModuleInstance>;
} // namespace

class StoreManager {
public:
  StoreManager()
      : NumMod(0), NumFunc(0), NumTab(0), NumMem(0), NumGlob(0), NumElem(0),
        NumData(0) {}
  ~StoreManager() = default;

  /// Import instances and move owner to store manager.
  uint32_t importModule(std::unique_ptr<Instance::ModuleInstance> &Mod) {
    Mod->Addr = ModInsts.size();
    return importInstance(Mod, ImpModInsts, ModInsts);
  }
  uint32_t importFunction(std::unique_ptr<Instance::FunctionInstance> &Func) {
    return importInstance(Func, ImpFuncInsts, FuncInsts);
  }
  uint32_t importTable(std::unique_ptr<Instance::TableInstance> &Tab) {
    return importInstance(Tab, ImpTabInsts, TabInsts);
  }
  uint32_t importMemory(std::unique_ptr<Instance::MemoryInstance> &Mem) {
    return importInstance(Mem, ImpMemInsts, MemInsts);
  }
  uint32_t importGlobal(std::unique_ptr<Instance::GlobalInstance> &Glob) {
    return importInstance(Glob, ImpGlobInsts, GlobInsts);
  }
  uint32_t importElement(std::unique_ptr<Instance::ElementInstance> &Elem) {
    return importInstance(Elem, ImpElemInsts, ElemInsts);
  }
  uint32_t importData(std::unique_ptr<Instance::DataInstance> &Data) {
    return importInstance(Data, ImpDataInsts, DataInsts);
  }

  /// Import host instances but not move ownership.
  uint32_t importHostFunction(Instance::FunctionInstance &Func) {
    return importHostInstance(Func, FuncInsts);
  }
  uint32_t importHostTable(Instance::TableInstance &Tab) {
    return importHostInstance(Tab, TabInsts);
  }
  uint32_t importHostMemory(Instance::MemoryInstance &Mem) {
    return importHostInstance(Mem, MemInsts);
  }
  uint32_t importHostGlobal(Instance::GlobalInstance &Glob) {
    return importHostInstance(Glob, GlobInsts);
  }

  /// Insert instances for instantiation and move ownership to store manager.
  uint32_t pushModule(std::unique_ptr<Instance::ModuleInstance> &Mod) {
    ++NumMod;
    Mod->Addr = ModInsts.size();
    return importInstance(Mod, ImpModInsts, ModInsts);
  }
  uint32_t pushFunction(std::unique_ptr<Instance::FunctionInstance> &Func) {
    ++NumFunc;
    return importInstance(Func, ImpFuncInsts, FuncInsts);
  }
  uint32_t pushTable(std::unique_ptr<Instance::TableInstance> &Tab) {
    ++NumTab;
    return importInstance(Tab, ImpTabInsts, TabInsts);
  }
  uint32_t pushMemory(std::unique_ptr<Instance::MemoryInstance> &Mem) {
    ++NumMem;
    return importInstance(Mem, ImpMemInsts, MemInsts);
  }
  uint32_t pushGlobal(std::unique_ptr<Instance::GlobalInstance> &Glob) {
    ++NumGlob;
    return importInstance(Glob, ImpGlobInsts, GlobInsts);
  }
  uint32_t pushElement(std::unique_ptr<Instance::ElementInstance> &Elem) {
    ++NumElem;
    return importInstance(Elem, ImpElemInsts, ElemInsts);
  }
  uint32_t pushData(std::unique_ptr<Instance::DataInstance> &Data) {
    ++NumData;
    return importInstance(Data, ImpDataInsts, DataInsts);
  }

  /// Pop temp. module. Dangerous function for used when instantiating only.
  void popModule() {
    if (NumMod > 0) {
      --NumMod;
      ImpModInsts.pop_back();
      ModInsts.pop_back();
    }
  }

  /// Get instance from store manager by address.
  Expect<Instance::ModuleInstance *> getModule(const uint32_t Addr) {
    return getInstance(Addr, ModInsts);
  }
  Expect<Instance::FunctionInstance *> getFunction(const uint32_t Addr) {
    return getInstance(Addr, FuncInsts);
  }
  Expect<Instance::TableInstance *> getTable(const uint32_t Addr) {
    return getInstance(Addr, TabInsts);
  }
  Expect<Instance::MemoryInstance *> getMemory(const uint32_t Addr) {
    return getInstance(Addr, MemInsts);
  }
  Expect<Instance::GlobalInstance *> getGlobal(const uint32_t Addr) {
    return getInstance(Addr, GlobInsts);
  }
  Expect<Instance::ElementInstance *> getElement(const uint32_t Addr) {
    return getInstance(Addr, ElemInsts);
  }
  Expect<Instance::DataInstance *> getData(const uint32_t Addr) {
    return getInstance(Addr, DataInsts);
  }

  /// Get exported instances of instantiated module.
  const std::map<std::string, uint32_t, std::less<>> getFuncExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getFuncExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getTableExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getTableExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getMemExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getMemExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getGlobalExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getGlobalExports();
    }
    return {};
  }

  /// Get active instance of instantiated module.
  Expect<Instance::ModuleInstance *> getActiveModule() const {
    if (NumMod > 0) {
      return ModInsts.back();
    }
    /// Error logging need to be handled in caller.
    return Unexpect(ErrCode::WrongInstanceAddress);
  }

  /// Find module by name.
  Expect<Instance::ModuleInstance *> findModule(std::string_view Name) {
    for (auto It : ModInsts) {
      if (It->getModuleName() == Name) {
        return It;
      }
    }
    /// Error logging need to be handled in caller.
    return Unexpect(ErrCode::WrongInstanceAddress);
  }

  /// Reset store.
  void reset(bool IsResetRegistered = false) {
    if (IsResetRegistered) {
      NumMod = 0;
      NumFunc = 0;
      NumTab = 0;
      NumMem = 0;
      NumGlob = 0;
      NumElem = 0;
      NumData = 0;
      ModInsts.clear();
      FuncInsts.clear();
      TabInsts.clear();
      MemInsts.clear();
      GlobInsts.clear();
      ElemInsts.clear();
      DataInsts.clear();
      ImpModInsts.clear();
      ImpFuncInsts.clear();
      ImpTabInsts.clear();
      ImpMemInsts.clear();
      ImpGlobInsts.clear();
      ImpElemInsts.clear();
      ImpDataInsts.clear();
    } else {
      while (NumMod > 0) {
        --NumMod;
        ImpModInsts.pop_back();
        ModInsts.pop_back();
      }
      while (NumFunc > 0) {
        --NumFunc;
        ImpFuncInsts.pop_back();
        FuncInsts.pop_back();
      }
      while (NumTab > 0) {
        --NumTab;
        ImpTabInsts.pop_back();
        TabInsts.pop_back();
      }
      while (NumMem > 0) {
        --NumMem;
        ImpMemInsts.pop_back();
        MemInsts.pop_back();
      }
      while (NumGlob > 0) {
        --NumGlob;
        ImpGlobInsts.pop_back();
        GlobInsts.pop_back();
      }
      while (NumElem > 0) {
        --NumElem;
        ImpElemInsts.pop_back();
        ElemInsts.pop_back();
      }
      while (NumData > 0) {
        --NumData;
        ImpDataInsts.pop_back();
        DataInsts.pop_back();
      }
    }
  }

private:
  /// Helper function for importing instances and move ownership.
  template <typename T>
  std::enable_if_t<IsInstanceV<T>, uint32_t>
  importInstance(std::unique_ptr<T> &Inst,
                 std::vector<std::unique_ptr<T>> &ImpInstsVec,
                 std::vector<T *> &InstsVec) {
    uint32_t Addr = InstsVec.size();
    InstsVec.push_back(Inst.get());
    ImpInstsVec.push_back(std::move(Inst));
    return Addr;
  }

  /// Helper function for importing host instances.
  template <typename T>
  std::enable_if_t<IsImportEntityV<T>, uint32_t>
  importHostInstance(T &Inst, std::vector<T *> &InstsVec) {
    uint32_t Addr = InstsVec.size();
    InstsVec.push_back(&Inst);
    return Addr;
  }

  /// Helper function for getting instance from instance vector.
  template <typename T>
  std::enable_if_t<IsInstanceV<T>, Expect<T *>>
  getInstance(const uint32_t Addr, const std::vector<T *> &InstsVec) {
    if (Addr >= InstsVec.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return InstsVec[Addr];
  }

  /// \name Store owned instances by StoreManager.
  /// @{
  std::vector<std::unique_ptr<Instance::ModuleInstance>> ImpModInsts;
  std::vector<std::unique_ptr<Instance::FunctionInstance>> ImpFuncInsts;
  std::vector<std::unique_ptr<Instance::TableInstance>> ImpTabInsts;
  std::vector<std::unique_ptr<Instance::MemoryInstance>> ImpMemInsts;
  std::vector<std::unique_ptr<Instance::GlobalInstance>> ImpGlobInsts;
  std::vector<std::unique_ptr<Instance::ElementInstance>> ImpElemInsts;
  std::vector<std::unique_ptr<Instance::DataInstance>> ImpDataInsts;
  /// @}

  /// \name Pointers to imported instances from modules or import objects.
  /// @{
  std::vector<Instance::ModuleInstance *> ModInsts;
  std::vector<Instance::FunctionInstance *> FuncInsts;
  std::vector<Instance::TableInstance *> TabInsts;
  std::vector<Instance::MemoryInstance *> MemInsts;
  std::vector<Instance::GlobalInstance *> GlobInsts;
  std::vector<Instance::ElementInstance *> ElemInsts;
  std::vector<Instance::DataInstance *> DataInsts;
  /// @}

  /// \name Data for instantiated module.
  /// @{
  uint32_t NumMod;
  uint32_t NumFunc;
  uint32_t NumTab;
  uint32_t NumMem;
  uint32_t NumGlob;
  uint32_t NumElem;
  uint32_t NumData;
  /// @}
};

} // namespace Runtime
} // namespace SSVM
