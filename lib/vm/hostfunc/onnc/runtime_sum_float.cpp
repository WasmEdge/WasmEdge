// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_sum_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ErrCode ONNCRuntimeSumFloat::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst,
                                  uint32_t RuntimeContextOff,
                                  uint32_t InDataOffOff, uint32_t InDataNTensor,
                                  uint32_t InDataNDimOff,
                                  uint32_t InDataDimsOffOff, uint32_t OutSumOff,
                                  uint32_t OutSumNDim, uint32_t OutSumDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *const *input_data_0,
  ///      int32_t input_data_0_ntensor,
  ///      const int32_t *input_data_0_ndim,
  ///      const int32_t *const *input_data_0_dims,
  ///      float *output_sum,
  ///      int32_t output_sum_ndim,
  ///      const int32_t *output_sum_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  uint32_t *InDataOff = MemInst.getPointer<uint32_t *>(InDataOffOff);
  uint32_t *InDataDimsOff = MemInst.getPointer<uint32_t *>(InDataDimsOffOff);
  int32_t *InDataNDim = MemInst.getPointer<int32_t *>(InDataNDimOff);
  int32_t *OutSumDims = MemInst.getPointer<int32_t *>(OutSumDimsOff);
  float *OutSum = MemInst.getPointer<float *>(OutSumOff);
  float *InData[InDataNTensor];
  int32_t *InDataDims[InDataNTensor];
  for (int i = 0; i < InDataNTensor; i++) {
    InData[i] = MemInst.getPointer<float *>(InDataOff[i]);
    InDataDims[i] = MemInst.getPointer<int32_t *>(InDataDimsOff[i]);
  }

  ONNC_RUNTIME_sum_float(RuntimeContext, InData, InDataNTensor, InDataNDim,
                         InDataDims, OutSum, OutSumNDim, OutSumDims);

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
