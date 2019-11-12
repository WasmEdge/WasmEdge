#include "vm/hostfunc/wasi/fd_Read.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdRead::WasiFdRead(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdRead::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                        std::vector<std::unique_ptr<ValueEntry>> &Res,
                        StoreManager &Store,
                        Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), IOVSPtr(u32), IOVSCnt(u32), NReadPtr(u32)
  if (Args.size() != 4) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int IOVSPtr = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int IOVSCnt = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int NReadPtr = retrieveValue<uint32_t>(*Args[0].get());
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

  /// Sequencially reading.
  unsigned int NRead = 0;
  std::vector<unsigned char> Data;
  for (unsigned int I = 0; I < IOVSCnt && ErrNo == 0; I++) {
    uint64_t CIOVecBufPtr = 0;
    uint64_t CIOVecBufLen = 0;
    /// TODO: sizeof(ptr) is 32-bit in wasm now.
    /// Get data offset.
    if ((Status = MemInst->loadValue(CIOVecBufPtr, IOVSPtr, 4)) !=
        ErrCode::Success) {
      return Status;
    }
    /// Get data length.
    if ((Status = MemInst->loadValue(CIOVecBufLen, IOVSPtr + 4, 4)) !=
        ErrCode::Success) {
      return Status;
    }
    /// Read data from Fd.
    Data.resize(CIOVecBufLen);
    unsigned int SizeRead = read(Fd, &Data[0], (uint32_t)CIOVecBufLen);

    /// Store data.
    if (SizeRead == -1) {
      ErrNo = 1;
    } else {
    if ((Status = MemInst->setBytes(Data, (uint32_t)CIOVecBufPtr, 0,
                                      SizeRead)) != ErrCode::Success) {
      return Status;
    }
      NRead += SizeRead;
    }
    Data.clear();
    /// Shift one element.
    /// TODO: sizeof(__wasi_ciovec_t) is 8 in 32-bit wasm.
    IOVSPtr += 8;
  }

  /// Store read bytes length.
  if ((Status = MemInst->storeValue(NRead, NReadPtr, 4)) != ErrCode::Success) {
    return Status;
  }

  /// Return: errno(u32)
  if (ErrNo == 0) {
    Res[0]->setValue(0U);
  } else {
    /// TODO: errno
    Res[0]->setValue(1U);
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM