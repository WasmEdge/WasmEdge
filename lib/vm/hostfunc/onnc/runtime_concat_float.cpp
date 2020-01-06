// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_concat_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeConcatFloat::ONNCRuntimeConcatFloat() {
  initializeFuncType<ONNCRuntimeConcatFloat>();
}

ErrCode ONNCRuntimeConcatFloat::run(VM::EnvironmentManager &EnvMgr,
                                    StackManager &StackMgr,
                                    Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeConcatFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeConcatFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InInputsOffOff,
    uint32_t InInputsNTensor, uint32_t InInputsNDimOff,
    uint32_t InInputsDimsOffOff, uint32_t OutConcatResultOff,
    uint32_t OutConcatResultNDim, uint32_t OutConcatResultDimsOff,
    uint32_t Axis) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *const *input_inputs,
  ///      int32_t input_inputs_ntensor,
  ///      const int32_t *input_inputs_ndim,
  ///      const int32_t *const *input_inputs_dims,
  ///      float *output_concat_result,
  ///      int32_t output_concat_result_ndim,
  ///      const int32_t *output_concat_result_dims,
  ///      int32_t axis

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  uint32_t *InInputsOff = MemInst.getPointer<uint32_t *>(InInputsOffOff);
  uint32_t *InInputsDimsOff =
      MemInst.getPointer<uint32_t *>(InInputsDimsOffOff);
  int32_t *InInputsNDim = MemInst.getPointer<int32_t *>(InInputsNDimOff);
  int32_t *OutConcatResultDims =
      MemInst.getPointer<int32_t *>(OutConcatResultDimsOff);
  float *OutConcatResult = MemInst.getPointer<float *>(OutConcatResultOff);
  float *InInputs[InInputsNTensor];
  int32_t *InInputsDims[InInputsNTensor];
  for (int i = 0; i < InInputsNTensor; i++) {
    InInputs[i] = MemInst.getPointer<float *>(InInputsOff[i]);
    InInputsDims[i] = MemInst.getPointer<int32_t *>(InInputsDimsOff[i]);
  }

  ONNC_RUNTIME_concat_float(RuntimeContext, InInputs, InInputsNTensor,
                            InInputsNDim, InInputsDims, OutConcatResult,
                            OutConcatResultNDim, OutConcatResultDims, Axis);

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
