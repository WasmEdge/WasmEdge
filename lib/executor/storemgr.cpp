#include "executor/storemgr.h"

namespace SSVM {
namespace Executor {

/// Inserter of module instance. See "include/executor/storemgr.h".
ErrCode
StoreManager::insertModuleInst(std::unique_ptr<Instance::ModuleInstance> &Mod,
                               unsigned int &NewId) {
  Mod->Addr = ModInsts.size();
  NewId = ModInsts.size();
  ModInsts.push_back(std::move(Mod));
  return ErrCode::Success;
}

/// Inserter of function instance. See "include/executor/storemgr.h".
ErrCode StoreManager::insertFunctionInst(
    std::unique_ptr<Instance::FunctionInstance> &Func, unsigned int &NewId) {
  Func->Addr = FuncInsts.size();
  NewId = FuncInsts.size();
  FuncInsts.push_back(std::move(Func));
  return ErrCode::Success;
}

/// Inserter of table instance. See "include/executor/storemgr.h".
ErrCode
StoreManager::insertTableInst(std::unique_ptr<Instance::TableInstance> &Tab,
                              unsigned int &NewId) {
  Tab->Addr = TabInsts.size();
  NewId = TabInsts.size();
  TabInsts.push_back(std::move(Tab));
  return ErrCode::Success;
}

/// Inserter of memory instance. See "include/executor/storemgr.h".
ErrCode
StoreManager::insertMemoryInst(std::unique_ptr<Instance::MemoryInstance> &Mem,
                               unsigned int &NewId) {
  Mem->Addr = MemInsts.size();
  NewId = MemInsts.size();
  MemInsts.push_back(std::move(Mem));
  return ErrCode::Success;
}

/// Inserter of global instance. See "include/executor/storemgr.h".
ErrCode
StoreManager::insertGlobalInst(std::unique_ptr<Instance::GlobalInstance> &Glob,
                               unsigned int &NewId) {
  Glob->Addr = GlobInsts.size();
  NewId = GlobInsts.size();
  GlobInsts.push_back(std::move(Glob));
  return ErrCode::Success;
}

/// Getter of module instance. See "include/executor/storemgr.h".
ErrCode StoreManager::getModule(unsigned int Addr,
                                Instance::ModuleInstance *&Mod) {
  if (ModInsts.size() <= Addr)
    return ErrCode::WrongInstanceAddress;
  Mod = ModInsts[Addr].get();
  return ErrCode::Success;
}

/// Getter of function instance. See "include/executor/storemgr.h".
ErrCode StoreManager::getFunction(unsigned int Addr,
                                  Instance::FunctionInstance *&Func) {
  if (FuncInsts.size() <= Addr)
    return ErrCode::WrongInstanceAddress;
  Func = FuncInsts[Addr].get();
  return ErrCode::Success;
}

/// Getter of table instance. See "include/executor/storemgr.h".
ErrCode StoreManager::getTable(unsigned int Addr,
                               Instance::TableInstance *&Tab) {
  if (TabInsts.size() <= Addr)
    return ErrCode::WrongInstanceAddress;
  Tab = TabInsts[Addr].get();
  return ErrCode::Success;
}

/// Getter of memory instance. See "include/executor/storemgr.h".
ErrCode StoreManager::getMemory(unsigned int Addr,
                                Instance::MemoryInstance *&Mem) {
  if (MemInsts.size() <= Addr)
    return ErrCode::WrongInstanceAddress;
  Mem = MemInsts[Addr].get();
  return ErrCode::Success;
}

/// Getter of global instance. See "include/executor/storemgr.h".
ErrCode StoreManager::getGlobal(unsigned int Addr,
                                Instance::GlobalInstance *&Glob) {
  if (GlobInsts.size() <= Addr)
    return ErrCode::WrongInstanceAddress;
  Glob = GlobInsts[Addr].get();
  return ErrCode::Success;
}

/// Finder of function instance. See "include/executor/storemgr.h".
ErrCode StoreManager::findFunction(const std::string &ModName,
                                   const std::string &FuncName,
                                   Instance::FunctionInstance *&Func) {
  for (auto It = FuncInsts.begin(); It != FuncInsts.end(); It++) {
    if ((*It)->isName(ModName, FuncName)) {
      Func = (*It).get();
      return ErrCode::Success;
    }
  }
  return ErrCode::WrongInstanceAddress;
}

} // namespace Executor
} // namespace SSVM
