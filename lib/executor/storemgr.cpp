#include "executor/storemgr.h"

/// Inserter of module instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreMgr::insertModuleInst(std::unique_ptr<ModuleInstance> &Mod,
                           unsigned int &NewId) {
  auto It = ModInsts.end();
  ModInsts.insert(It, std::move(Mod));
  NewId = It - ModInsts.begin();
  return Executor::ErrCode::Success;
}

/// Inserter of function instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreMgr::insertFunctionInst(std::unique_ptr<FunctionInstance> &Func,
                             unsigned int &NewId) {
  auto It = FuncInsts.end();
  FuncInsts.insert(It, std::move(Func));
  NewId = It - FuncInsts.begin();
  return Executor::ErrCode::Success;
}

/// Inserter of table instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::insertTableInst(std::unique_ptr<TableInstance> &Tab,
                                            unsigned int &NewId) {
  auto It = TabInsts.end();
  TabInsts.insert(It, std::move(Tab));
  NewId = It - TabInsts.begin();
  return Executor::ErrCode::Success;
}

/// Inserter of memory instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreMgr::insertMemoryInst(std::unique_ptr<MemoryInstance> &Mem,
                           unsigned int &NewId) {
  auto It = MemInsts.end();
  MemInsts.insert(It, std::move(Mem));
  NewId = It - MemInsts.begin();
  return Executor::ErrCode::Success;
}

/// Inserter of global instance. See "include/executor/storemgr.h".
Executor::ErrCode
StoreMgr::insertGlobalInst(std::unique_ptr<GlobalInstance> &Glob,
                           unsigned int &NewId) {
  auto It = GlobInsts.end();
  GlobInsts.insert(It, std::move(Glob));
  NewId = It - GlobInsts.begin();
  return Executor::ErrCode::Success;
}

/// Getter of module instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::getModule(unsigned int Addr, ModuleInstance *&Mod) {
  if (ModInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Mod = ModInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of function instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::getFunction(unsigned int Addr,
                                        FunctionInstance *&Func) {
  if (FuncInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Func = FuncInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of table instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::getTable(unsigned int Addr, TableInstance *&Tab) {
  if (TabInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Tab = TabInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of memory instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::getMemory(unsigned int Addr, MemoryInstance *&Mem) {
  if (MemInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Mem = MemInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Getter of global instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::getGlobal(unsigned int Addr,
                                      GlobalInstance *&Glob) {
  if (GlobInsts.size() <= Addr)
    return Executor::ErrCode::WrongInstanceAddress;
  Glob = GlobInsts[Addr].get();
  return Executor::ErrCode::Success;
}

/// Finder of function instance. See "include/executor/storemgr.h".
Executor::ErrCode StoreMgr::findFunction(std::string &ModName,
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