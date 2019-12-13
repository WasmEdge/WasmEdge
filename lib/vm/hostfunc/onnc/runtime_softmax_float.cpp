// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_softmax_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeSoftmaxFloat::ONNCRuntimeSoftmaxFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeSoftmaxFloat::run(VM::EnvironmentManager &EnvMgr,
                                     std::vector<Value> &Args,
                                     std::vector<Value> &Res,
                                     StoreManager &Store,
                                     Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_input,
  ///      int32_t input_input_ndim,
  ///      const int32_t *input_input_dims,
  ///      float *output_output,
  ///      int32_t output_output_ndim,
  ///      const int32_t *output_output_dims,
  ///      int32_t axis
  if (Args.size() != 8) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[7]);
  unsigned int InOff = retrieveValue<uint32_t>(Args[6]);
  unsigned int InNDim = retrieveValue<uint32_t>(Args[5]);
  unsigned int InDimsOff = retrieveValue<uint32_t>(Args[4]);
  unsigned int OutOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutNDim = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutDimsOff = retrieveValue<uint32_t>(Args[1]);
  unsigned int Axis = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *InDims = MemInst->getPointer<int32_t *>(InDimsOff);
  int32_t *OutDims = MemInst->getPointer<int32_t *>(OutDimsOff);
  float *In = MemInst->getPointer<float *>(InOff);
  float *Out = MemInst->getPointer<float *>(OutOff);

  ONNC_RUNTIME_softmax_float(RuntimeContext, In, InNDim, InDims, Out, OutNDim,
                             OutDims, Axis);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
