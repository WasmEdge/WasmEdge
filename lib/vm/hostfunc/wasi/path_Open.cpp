#include "vm/hostfunc/wasi/path_Open.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <fcntl.h>

namespace SSVM {
namespace Executor {

WasiPathOpen::WasiPathOpen(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I64);
  appendParamDef(AST::ValType::I64);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiPathOpen::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                          std::vector<std::unique_ptr<ValueEntry>> &Res,
                          StoreManager &Store,
                          Instance::ModuleInstance *ModInst) {
  /// Arg: DirFd(u32), DirFlags(u32), PathPtr(u32), PathLen(u32), OFlags(u32),
  /// FsRightsBase(u64), FsRightsInheriting(u64), FsFlags(u32), FdPtr(u32)
  if (Args.size() != 9) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  /// unsigned int DirFd = retrieveValue<uint32_t>(*Args[8].get());
  /// unsigned int DirFlags = retrieveValue<uint32_t>(*Args[7].get());
  unsigned int PathPtr = retrieveValue<uint32_t>(*Args[6].get());
  unsigned int PathLen = retrieveValue<uint32_t>(*Args[5].get());
  unsigned int OFlags = retrieveValue<uint32_t>(*Args[4].get());
  /// unsigned int FsRightsBase = retrieveValue<uint64_t>(*Args[3].get());
  /// unsigned int FsRightsInheriting = retrieveValue<uint64_t>(*Args[2].get());
  /// unsigned int FsFlags = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int FdPtr = retrieveValue<uint32_t>(*Args[0].get());
  int ErrNo = 0;

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Get file path.
  std::vector<unsigned char> Data;
  if ((Status = MemInst->getBytes(Data, PathPtr, PathLen)) !=
      ErrCode::Success) {
    return Status;
  }
  std::string Path(Data.begin(), Data.end());

  /// Open file and store Fd.
  int Fd = open(Path.c_str(), OFlags);
  if (Fd == -1) {
    ErrNo = 1;
  } else {
    if ((Status = MemInst->storeValue((uint32_t)Fd, FdPtr, 4)) !=
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