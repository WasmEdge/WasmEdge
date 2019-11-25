#include "vm/hostfunc/onnc/runtime_concat_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeConcatFloat::ONNCRuntimeConcatFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeConcatFloat::run(std::vector<Value> &Args,
                                    std::vector<Value> &Res,
                                    StoreManager &Store,
                                    Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *const *input_inputs,
  ///      int32_t input_inputs_ntensor,
  ///      const int32_t *input_inputs_ndim,
  ///      const int32_t *const *input_inputs_dims,
  ///      float *output_concat_result,
  ///      int32_t output_concat_result_ndim,
  ///      const int32_t *output_concat_result_dims,
  ///      int32_t axis
  if (Args.size() != 9) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int InInputsOffOff = retrieveValue<uint32_t>(Args[7]);
  unsigned int InInputsNTensor = retrieveValue<uint32_t>(Args[6]);
  unsigned int InInputsNDimOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int InInputsDimsOffOff = retrieveValue<uint32_t>(Args[4]);
  unsigned int OutConcatResultOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutConcatResultNDim = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutConcatResultDimsOff = retrieveValue<uint32_t>(Args[1]);
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
  uint32_t *InInputsOff = MemInst->getPointer<uint32_t *>(InInputsOffOff);
  uint32_t *InInputsDimsOff =
      MemInst->getPointer<uint32_t *>(InInputsDimsOffOff);
  int32_t *InInputsNDim = MemInst->getPointer<int32_t *>(InInputsNDimOff);
  int32_t *OutConcatResultDims =
      MemInst->getPointer<int32_t *>(OutConcatResultDimsOff);
  float *OutConcatResult = MemInst->getPointer<float *>(OutConcatResultOff);
  float *InInputs[InInputsNTensor];
  int32_t *InInputsDims[InInputsNTensor];
  for (int i = 0; i < InInputsNTensor; i++) {
    InInputs[i] = MemInst->getPointer<float *>(InInputsOff[i]);
    InInputsDims[i] = MemInst->getPointer<int32_t *>(InInputsDimsOff[i]);
  }

  ONNC_RUNTIME_concat_float(RuntimeContext, InInputs, InInputsNTensor,
                            InInputsNDim, InInputsDims, OutConcatResult,
                            OutConcatResultNDim, OutConcatResultDims, Axis);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
