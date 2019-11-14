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

ErrCode
ONNCRuntimeConcatFloat::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                            std::vector<std::unique_ptr<ValueEntry>> &Res,
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
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(*Args[8].get());
  unsigned int InInputsPtrPtr = retrieveValue<uint32_t>(*Args[7].get());
  unsigned int InInputsNTensor = retrieveValue<uint32_t>(*Args[6].get());
  unsigned int InInputsNDimPtr = retrieveValue<uint32_t>(*Args[5].get());
  unsigned int InInputsDimsPtrPtr = retrieveValue<uint32_t>(*Args[4].get());
  unsigned int OutConcatResultPtr = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int OutConcatResultNDim = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int OutConcatResultDimsPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int Axis = retrieveValue<uint32_t>(*Args[0].get());

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  void *RuntimeContext =
      reinterpret_cast<void *>(MemInst->getPointer(RuntimeContextPtr));
  uint32_t *InInputsPtr =
      reinterpret_cast<uint32_t *>(MemInst->getPointer(InInputsPtrPtr));
  uint32_t *InInputsDimsPtr =
      reinterpret_cast<uint32_t *>(MemInst->getPointer(InInputsDimsPtrPtr));
  int32_t *InInputsNDim =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InInputsNDimPtr));
  int32_t *OutConcatResultDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutConcatResultDimsPtr));
  float *OutConcatResult =
      reinterpret_cast<float *>(MemInst->getPointer(OutConcatResultPtr));
  float *InInputs[InInputsNTensor];
  int32_t *InInputsDims[InInputsNTensor];
  for (int i = 0; i < InInputsNTensor; i++) {
    InInputs[i] =
        reinterpret_cast<float *>(MemInst->getPointer(InInputsPtr[i]));
    InInputsDims[i] =
        reinterpret_cast<int32_t *>(MemInst->getPointer(InInputsDimsPtr[i]));
  }

  ONNC_RUNTIME_concat_float(RuntimeContext, InInputs, InInputsNTensor,
                            InInputsNDim, InInputsDims, OutConcatResult,
                            OutConcatResultNDim, OutConcatResultDims, Axis);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM