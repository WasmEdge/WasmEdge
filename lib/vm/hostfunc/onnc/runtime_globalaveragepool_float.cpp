// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_globalaveragepool_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeGlobalaveragepoolFloat::ONNCRuntimeGlobalaveragepoolFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeGlobalaveragepoolFloat::run(
    std::vector<Value> &Args, std::vector<Value> &Res, StoreManager &Store,
    Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims
  if (Args.size() != 7) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[14]);
  unsigned int InXOff = retrieveValue<uint32_t>(Args[13]);
  unsigned int InXNDim = retrieveValue<uint32_t>(Args[12]);
  unsigned int InXDimsOff = retrieveValue<uint32_t>(Args[11]);
  unsigned int OutYOff = retrieveValue<uint32_t>(Args[10]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[9]);
  unsigned int OutYDimsOff = retrieveValue<uint32_t>(Args[8]);

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
  float *InX = MemInst->getPointer<float *>(InXOff);
  float *OutY = MemInst->getPointer<float *>(OutYOff);

  ONNC_RUNTIME_globalaveragepool_float(RuntimeContext, InX, InXNDim, InXDims,
                                       OutY, OutYNDim, OutYDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
