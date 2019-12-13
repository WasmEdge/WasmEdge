// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_relu_int8.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeReluInt8::ONNCRuntimeReluInt8() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeReluInt8::run(VM::EnvironmentManager &EnvMgr,
                                 std::vector<Value> &Args,
                                 std::vector<Value> &Res, StoreManager &Store,
                                 Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t* input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t* input_X_dims,
  ///      int8_t* output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t* output_Y_dims
  if (Args.size() != 7) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[6]);
  unsigned int InXOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int InXNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int InXDimsOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutYOff = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[1]);
  unsigned int OutYDimsOff = retrieveValue<uint32_t>(Args[0]);

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  void *RuntimeContext = MemInst->getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst->getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst->getPointer<int32_t *>(OutYDimsOff);
  int8_t *InX = MemInst->getPointer<int8_t *>(InXOff);
  int8_t *OutY = MemInst->getPointer<int8_t *>(OutYOff);

  ONNC_RUNTIME_relu_int8(RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim,
                         OutYDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
