#pragma once

#include "common.h"
#include "functioninst.h"
#include "globalinst.h"
#include "memoryinst.h"
#include "moduleinst.h"
#include "tableinst.h"
#include <memory>

class StoreMgr {
public:
  Executor::ErrCode insertModuleInst(unsigned int &NewId,
                                     std::unique_ptr<ModuleInstance> Mod);
  Executor::ErrCode insertFunctionInst(unsigned int &NewId,
                                       std::unique_ptr<FunctionInstance> Func);
  Executor::ErrCode insertTableInst(unsigned int &NewId,
                                    std::unique_ptr<TableInstance> Tab);
  Executor::ErrCode insertMemoryInst(unsigned int &NewId,
                                     std::unique_ptr<MemoryInstance> Mem);
  Executor::ErrCode insertGlobalInst(unsigned int &NewId,
                                     std::unique_ptr<GlobalInstance> Glob);

  Executor::ErrCode getModule(unsigned int Idx, ModuleInstance *&Mod);
  Executor::ErrCode getFunction(unsigned int Idx, ModuleInstance *&Func);
  Executor::ErrCode getTable(unsigned int Idx, ModuleInstance *&Tab);
  Executor::ErrCode getMemory(unsigned int Idx, ModuleInstance *&Mem);
  Executor::ErrCode getGlobal(unsigned int Idx, ModuleInstance *&Glob);
};