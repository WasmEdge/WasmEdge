#include "executor/storemgr.h"

namespace SSVM {
namespace Executor {

/// Inserter of module instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreManager::insertModuleInst(std::unique_ptr<ModuleInstance> &Mod,
                               unsigned int &NewId) {
  ModInsts.push_back(std::move(Mod));
  NewId = ModInsts.size() - 1;
  return Executor::ErrCode::Success;
}

/// Inserter of function instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreManager::insertFunctionInst(std::unique_ptr<FunctionInstance> &Func,
                                 unsigned int &NewId) {
  FuncInsts.push_back(std::move(Func));
  NewId = FuncInsts.size() - 1;
  return Executor::ErrCode::Success;
}

/// Inserter of table instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::insertTableInst(std::unique_ptr<TableInstance> &Tab,
                                                unsigned int &NewId) {
  TabInsts.push_back(std::move(Tab));
  NewId = TabInsts.size() - 1;
  return Executor::ErrCode::Success;
}

/// Inserter of memory instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreManager::insertMemoryInst(std::unique_ptr<MemoryInstance> &Mem,
                               unsigned int &NewId) {
  MemInsts.push_back(std::move(Mem));
  NewId = MemInsts.size() - 1;
  return Executor::ErrCode::Success;
}

/// Inserter of global instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreManager::insertGlobalInst(std::unique_ptr<GlobalInstance> &Glob,
                               unsigned int &NewId) {
  GlobInsts.push_back(std::move(Glob));
  NewId = GlobInsts.size() - 1;
  return Executor::ErrCode::Success;
}

/// Getter of module instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::getModule(unsigned int Addr, ModuleInstance *&Mod) {
  if (ModInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Mod = ModInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of function instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::getFunction(unsigned int Addr,
                                            FunctionInstance *&Func) {
  if (FuncInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Func = FuncInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of table instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::getTable(unsigned int Addr, TableInstance *&Tab) {
  if (TabInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Tab = TabInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of memory instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::getMemory(unsigned int Addr, MemoryInstance *&Mem) {
  if (MemInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Mem = MemInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of global instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::getGlobal(unsigned int Addr,
                                          GlobalInstance *&Glob) {
  if (GlobInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Glob = GlobInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Finder of function instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreManager::findFunction(std::string &ModName,
                                             std::string &FuncName,
                                             FunctionInstance *&Func) {
  for (auto It = FuncInsts.begin(); It != FuncInsts.end(); It++) {
    if ((*It)->isName(ModName, FuncName)) {
      Func = (*It).get();
      return Executor::ErrCode::Success;
    }
  }
  return Executor::ErrCode::WrongInstanceAddress;
}

} // namespace Executor
} // namespace SSVM
