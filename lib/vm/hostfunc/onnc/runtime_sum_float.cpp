// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_sum_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeSumFloat::ONNCRuntimeSumFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeSumFloat::run(std::vector<Value> &Args,
                                 std::vector<Value> &Res, StoreManager &Store,
                                 Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *const *input_data_0,
  ///      int32_t input_data_0_ntensor,
  ///      const int32_t *input_data_0_ndim,
  ///      const int32_t *const *input_data_0_dims,
  ///      float *output_sum,
  ///      int32_t output_sum_ndim,
  ///      const int32_t *output_sum_dims
  if (Args.size() != 8) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[7]);
  unsigned int InDataOffOff = retrieveValue<uint32_t>(Args[6]);
  unsigned int InDataNTensor = retrieveValue<uint32_t>(Args[5]);
  unsigned int InDataNDimOff = retrieveValue<uint32_t>(Args[4]);
  unsigned int InDataDimsOffOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutSumOff = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutSumNDim = retrieveValue<uint32_t>(Args[1]);
  unsigned int OutSumDimsOff = retrieveValue<uint32_t>(Args[0]);

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
  uint32_t *InDataOff = MemInst->getPointer<uint32_t *>(InDataOffOff);
  uint32_t *InDataDimsOff = MemInst->getPointer<uint32_t *>(InDataDimsOffOff);
  int32_t *InDataNDim = MemInst->getPointer<int32_t *>(InDataNDimOff);
  int32_t *OutSumDims = MemInst->getPointer<int32_t *>(OutSumDimsOff);
  float *OutSum = MemInst->getPointer<float *>(OutSumOff);
  float *InData[InDataNTensor];
  int32_t *InDataDims[InDataNTensor];
  for (int i = 0; i < InDataNTensor; i++) {
    InData[i] = MemInst->getPointer<float *>(InDataOff[i]);
    InDataDims[i] = MemInst->getPointer<int32_t *>(InDataDimsOff[i]);
  }

  ONNC_RUNTIME_sum_float(RuntimeContext, InData, InDataNTensor, InDataNDim,
                         InDataDims, OutSum, OutSumNDim, OutSumDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
