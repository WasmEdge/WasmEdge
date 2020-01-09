// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_softmax_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ErrCode ONNCRuntimeSoftmaxFloat::body(VM::EnvironmentManager &EnvMgr,
                                      Instance::MemoryInstance &MemInst,
                                      uint32_t RuntimeContextOff,
                                      uint32_t InOff, uint32_t InNDim,
                                      uint32_t InDimsOff, uint32_t OutOff,
                                      uint32_t OutNDim, uint32_t OutDimsOff,
                                      uint32_t Axis) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_input,
  ///      int32_t input_input_ndim,
  ///      const int32_t *input_input_dims,
  ///      float *output_output,
  ///      int32_t output_output_ndim,
  ///      const int32_t *output_output_dims,
  ///      int32_t axis

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InDims = MemInst.getPointer<int32_t *>(InDimsOff);
  int32_t *OutDims = MemInst.getPointer<int32_t *>(OutDimsOff);
  float *In = MemInst.getPointer<float *>(InOff);
  float *Out = MemInst.getPointer<float *>(OutOff);

  ONNC_RUNTIME_softmax_float(RuntimeContext, In, InNDim, InDims, Out, OutNDim,
                             OutDims, Axis);

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
