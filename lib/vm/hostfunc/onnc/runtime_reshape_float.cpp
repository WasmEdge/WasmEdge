// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_reshape_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ErrCode ONNCRuntimeReshapeFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InDataOff, uint32_t InDataNDim,
    uint32_t InDataDimsOff, uint32_t InShapeOff, uint32_t InShapeNDim,
    uint32_t InShapeDimsOff, uint32_t OutReshapedOff, uint32_t OutReshapedNDim,
    uint32_t OutReshapedDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_data,
  ///      int32_t input_data_ndim,
  ///      const int32_t *input_data_dims,
  ///      const float *input_shape,
  ///      int32_t input_shape_ndim,
  ///      const int32_t *input_shape_dims,
  ///      float *output_reshaped,
  ///      int32_t output_reshaped_ndim,
  ///      const int32_t *output_reshaped_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InDataDims = MemInst.getPointer<int32_t *>(InDataDimsOff);
  int32_t *InShapeDims = MemInst.getPointer<int32_t *>(InShapeDimsOff);
  int32_t *OutReshapedDims = MemInst.getPointer<int32_t *>(OutReshapedDimsOff);
  float *InData = MemInst.getPointer<float *>(InDataOff);
  float *InShape = MemInst.getPointer<float *>(InShapeOff);
  float *OutReshaped = MemInst.getPointer<float *>(OutReshapedOff);

  ONNC_RUNTIME_reshape_float(RuntimeContext, InData, InDataNDim, InDataDims,
                             InShape, InShapeNDim, InShapeDims, OutReshaped,
                             OutReshapedNDim, OutReshapedDims);

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
