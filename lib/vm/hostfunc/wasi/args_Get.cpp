// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/args_Get.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiArgsGet::WasiArgsGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiArgsGet::run(std::vector<Value> &Args, std::vector<Value> &Res,
                         StoreManager &Store,
                         Instance::ModuleInstance *ModInst) {
  /// Arg: ArgvPtr(u32), ArgvBufPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int ArgvPtr = retrieveValue<uint32_t>(Args[1]);
  unsigned int ArgvBufPtr = retrieveValue<uint32_t>(Args[0]);

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Store **Argv.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  std::vector<unsigned char> ArgvBuf;
  uint32_t ArgvBufOffset = ArgvBufPtr;
  for (auto It = CmdArgs.cbegin(); It != CmdArgs.cend(); It++) {
    /// Concate Argv.
    int off = ArgvBuf.size();
    std::copy(It->cbegin(), It->cend(), std::back_inserter(ArgvBuf));
    ArgvBuf.push_back('\0');

    /// Calcuate Argv[i] offset and store.
    if ((Status = MemInst->storeValue(ArgvBufOffset, ArgvPtr, 4)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Shift one element.
    ArgvPtr += 4;
    ArgvBufOffset += It->length() + 1;
  }

  /// Store ArgvBuf.
  if ((Status = MemInst->setBytes(ArgvBuf, ArgvBufPtr, 0, ArgvBuf.size())) !=
      ErrCode::Success) {
    return Status;
  }

  /// Return: errno(u32)
  Res[0] = uint32_t(0U);
  return Status;
}

} // namespace Executor
} // namespace SSVM
