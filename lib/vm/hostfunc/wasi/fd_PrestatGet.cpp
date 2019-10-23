#include "vm/hostfunc/wasi/fd_PrestatGet.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdPrestatGet::WasiFdPrestatGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdPrestatGet::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                              std::vector<std::unique_ptr<ValueEntry>> &Res,
                              StoreManager &Store,
                              Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), PreStatPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int PreStatPtr = retrieveValue<uint32_t>(*Args[0].get());
  int ErrNo = 0;

  __wasi_prestat_t PreStat;
  /// 1. __wasi_prestat_t.pr_type
  if (ErrNo == 0) {
    struct stat SysFStat;
    ErrNo = fstat(Fd, &SysFStat);
    PreStat.pr_type =
        ((SysFStat.st_mode & S_IFMT) == S_IFDIR) ? __WASI_PREOPENTYPE_DIR : 1;
  }

  /// 2. __wasi_prestat_t.u.dir.pr_name_len
  if (ErrNo == 0 && PreStat.pr_type == __WASI_PREOPENTYPE_DIR) {
    /// Store current working dir and change to Fd working dir.
    char *CurrPath = getcwd(NULL, 0);
    ErrNo = fchdir(Fd);
    /// Get Fd working dir and change back to current working dir.
    char *FdPath = getcwd(NULL, 0);
    ErrNo = chdir(CurrPath);
    /// Store buf length of Fd working dir.
    PreStat.u.dir.pr_name_len = strlen(FdPath) + 1;
    if (CurrPath != nullptr) {
      free(FdPath);
    }
    if (FdPath != nullptr) {
      free(CurrPath);
    }
  }

  /// Store to memory instance
  if (ErrNo == 0) {
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }
    /// byte[0:sizeof(void *)] : pr_type(uint8_t)
    uint64_t PrType = (uint64_t)PreStat.pr_type;
    if ((Status = MemInst->storeValue(PreStatPtr, sizeof(void *), PrType)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[sizeof(void *):2*sizeof(void *)] : u.dir.pr_name_len(size_t)
    uint64_t PrNameLen = (uint32_t)PreStat.u.dir.pr_name_len;
    if ((Status = MemInst->storeValue(PreStatPtr + sizeof(void *),
                                      sizeof(void *), PrNameLen)) !=
        ErrCode::Success) {
      return Status;
    }
  }

  /// Return: errno(u32)
  if (ErrNo == 0) {
    Res.push_back(std::make_unique<ValueEntry>(0U));
  } else {
    /// TODO: errno
    Res.push_back(std::make_unique<ValueEntry>(1U));
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM