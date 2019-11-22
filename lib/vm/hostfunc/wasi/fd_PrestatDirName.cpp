#include "vm/hostfunc/wasi/fd_PrestatDirName.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdPrestatDirName::WasiFdPrestatDirName(VM::WasiEnvironment &Env)
    : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdPrestatDirName::run(std::vector<Value> &Args,
                                  std::vector<Value> &Res, StoreManager &Store,
                                  Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), PathBufPtr(u32), PathLenPtr(u32)
  if (Args.size() != 3) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(Args[2]);
  unsigned int PathBufPtr = retrieveValue<uint32_t>(Args[1]);
  unsigned int PathLen = retrieveValue<uint32_t>(Args[0]);
  int ErrNo = 0;

  /// Store current working dir and change to Fd working dir.
  char *CurrPath = getcwd(NULL, 0);
  if (ErrNo == 0) {
    ErrNo = fchdir(Fd);
  }
  /// Get Fd working dir and change back to current working dir.
  char *FdPath = getcwd(NULL, 0);
  if (ErrNo == 0) {
    ErrNo = chdir(CurrPath);
  }

  /// Store Path and PathLen.
  if (FdPath != nullptr) {
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }
    std::vector<unsigned char> PathBuf;
    PathBuf.insert(PathBuf.end(), FdPath, FdPath + strlen(FdPath));
    PathBuf.push_back('\0');
    /*
    if ((Status = MemInst->setBytes(PathBuf, PathBufPtr, 0, PathLen)) !=
        ErrCode::Success) {
      return Status;
    }
    */
  } else {
    ErrNo = -1;
  }

  /// Free char*.
  if (CurrPath != nullptr) {
    free(FdPath);
  }
  if (FdPath != nullptr) {
    free(CurrPath);
  }

  /// Return: errno(u32)
  if (ErrNo == 0) {
    Res[0] = uint32_t(0U);
  } else {
    /// TODO: errno
    Res[0] = uint32_t(1U);
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
