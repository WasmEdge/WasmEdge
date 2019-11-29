// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_FdstatGet.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdFdstatGet::WasiFdFdstatGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdFdstatGet::run(std::vector<Value> &Args, std::vector<Value> &Res,
                             StoreManager &Store,
                             Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), FdStatPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(Args[1]);
  unsigned int FdStatPtr = retrieveValue<uint32_t>(Args[0]);
  int ErrNo = 0;

  __wasi_fdstat_t FdStat;
  /// 1. __wasi_fdstat_t.fs_filetype
  if (ErrNo == 0) {
    struct stat SysFStat;
    ErrNo = fstat(Fd, &SysFStat);
    FdStat.fs_filetype = __WASI_FILETYPE_UNKNOWN;
    switch (SysFStat.st_mode & S_IFMT) {
    case S_IFSOCK:
      FdStat.fs_filetype = __WASI_FILETYPE_SOCKET_DGRAM;
      break;
    case S_IFLNK:
      FdStat.fs_filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
      break;
    case S_IFREG:
      FdStat.fs_filetype = __WASI_FILETYPE_REGULAR_FILE;
      break;
    case S_IFBLK:
      FdStat.fs_filetype = __WASI_FILETYPE_BLOCK_DEVICE;
      break;
    case S_IFDIR:
      FdStat.fs_filetype = __WASI_FILETYPE_DIRECTORY;
      break;
    case S_IFCHR:
      FdStat.fs_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
      break;
    default:
      break;
    }
  }

  /// 2. __wasi_fdstat_t.fs_flags
  if (ErrNo == 0) {
    int FdFlags = fcntl(Fd, F_GETFL);
    ErrNo = (FdFlags == -1) ? (-1) : 0;
    FdStat.fs_flags = FdFlags & O_ACCMODE;
    FdStat.fs_flags |= ((FdFlags & O_APPEND) ? __WASI_FDFLAG_APPEND : 0);
    FdStat.fs_flags |= ((FdFlags & O_DSYNC) ? __WASI_FDFLAG_DSYNC : 0);
    FdStat.fs_flags |= ((FdFlags & O_NONBLOCK) ? __WASI_FDFLAG_NONBLOCK : 0);
    FdStat.fs_flags |=
        ((FdFlags & O_SYNC) ? (__WASI_FDFLAG_RSYNC | __WASI_FDFLAG_SYNC) : 0);
  }

  /// 3. __wasi_fdstat_t.fs_rights_base
  if (ErrNo == 0) {
    /// TODO: get base rights
    FdStat.fs_rights_base = 0x000000001FFFFFFFULL;
  }

  /// 4. __wasi_fdstat_t.fs_rights_inheriting
  if (ErrNo == 0) {
    /// TODO: get inheriting rights
    FdStat.fs_rights_inheriting = 0x000000001FFFFFFFULL;
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
    uint64_t ConstZero = 0ULL;
    /// byte[0] : fs_filetype(uint8_t)
    uint32_t FsFileType = (uint32_t)FdStat.fs_filetype;
    if ((Status = MemInst->storeValue(FsFileType, FdStatPtr, 1)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[1] : ZERO
    if ((Status = MemInst->storeValue(ConstZero, FdStatPtr + 1, 1)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[2:3] : fs_flags(uint16_t)
    uint32_t FsFlags = (uint32_t)FdStat.fs_flags;
    if ((Status = MemInst->storeValue(FsFlags, FdStatPtr + 2, 2)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[4:7] : ZERO
    if ((Status = MemInst->storeValue(ConstZero, FdStatPtr + 4, 4)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[8:15] : fs_rights_base
    uint64_t FsRightsBase = FdStat.fs_rights_base;
    if ((Status = MemInst->storeValue(FsRightsBase, FdStatPtr + 8, 8)) !=
        ErrCode::Success) {
      return Status;
    }
    /// byte[16:23] : fs_rights_inheriting
    uint64_t FsRightsInheriting = FdStat.fs_rights_inheriting;
    if ((Status = MemInst->storeValue(FsRightsInheriting, FdStatPtr + 16, 8)) !=
        ErrCode::Success) {
      return Status;
    }
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
