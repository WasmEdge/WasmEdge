#include "vm/hostfunc/wasi/fd_PrestatGet.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <errno.h>
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

ErrCode WasiFdPrestatGet::run(std::vector<Value> &Args, std::vector<Value> &Res,
                              StoreManager &Store,
                              Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), PreStatPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(Args[1]);
  unsigned int PreStatPtr = retrieveValue<uint32_t>(Args[0]);
  int ErrNo = 0;

  __wasi_prestat_t PreStat;
  /// 1. __wasi_prestat_t.pr_type
  if (ErrNo == 0) {
    struct stat SysFStat;
    if (fstat(Fd, &SysFStat) != 0) {
      switch (errno) {
      case EBADF:
        ErrNo = __WASI_EBADF;
        break;
      case EFAULT:
        ErrNo = __WASI_EFAULT;
        break;
      case EIO:
        ErrNo = __WASI_EIO;
        break;
      case EOVERFLOW:
        ErrNo = __WASI_EOVERFLOW;
        break;
      default:
        ErrNo = -1;
        break;
      }
    } else {
      PreStat.pr_type =
          ((SysFStat.st_mode & S_IFMT) == S_IFDIR) ? __WASI_PREOPENTYPE_DIR : 1;
      PreStat.u.dir.pr_name_len = 0;
    }
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
    /// TODO: sizeof(ptr) is 32-bit in wasm now.
    /// byte[0:sizeof(ptr))))] : pr_type(uint8_t)
    uint32_t PrType = (uint32_t)PreStat.pr_type;
    if ((Status = MemInst->storeValue(PrType, PreStatPtr, 4)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[sizeof(ptr):2*sizeof(ptr)] : u.dir.pr_name_len(size_t)
    uint32_t PrNameLen = (uint32_t)PreStat.u.dir.pr_name_len;
    if ((Status = MemInst->storeValue(PrNameLen, PreStatPtr + 4, 4)) !=
        ErrCode::Success) {
      return Status;
    }
  }

  /// Return: errno(u32)
  Res[0] = uint32_t(ErrNo);
  return Status;
}

} // namespace Executor
} // namespace SSVM
