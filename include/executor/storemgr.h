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
  Executor::ErrCode queryNewModuleEntry(unsigned int &NewId);
  Executor::ErrCode queryFunctionEntry(unsigned int &NewId);
  Executor::ErrCode queryTableEntry(unsigned int &NewId);
  Executor::ErrCode queryMemoryEntry(unsigned int &NewId);
  Executor::ErrCode queryGlobalEntry(unsigned int &NewId);

  Executor::ErrCode insertModuleInst(unsigned int Id,
                                     std::unique_ptr<ModuleInstance> Mod);
  Executor::ErrCode insertFunctionInst(unsigned int Id,
                                       std::unique_ptr<FunctionInstance> Func);
  Executor::ErrCode insertTableInst(unsigned int Id,
                                    std::unique_ptr<TableInstance> Tab);
  Executor::ErrCode insertMemoryInst(unsigned int Id,
                                     std::unique_ptr<MemoryInstance> Mem);
  Executor::ErrCode insertGlobalInst(unsigned int Id,
                                     std::unique_ptr<GlobalInstance> Glob);
};