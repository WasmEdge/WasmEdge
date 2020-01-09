// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_batchnormalization_float.h"
#include "onnc/onnc_runtime.h"

#include <stdint.h>

namespace SSVM {
namespace Executor {

ErrCode ONNCRuntimeBatchnormalizationFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
    uint32_t InXDimsOff, uint32_t InScaleOff, uint32_t InScaleNDim,
    uint32_t InScaleDimsOff, uint32_t InBOff, uint32_t InBNDim,
    uint32_t InBDimsOff, uint32_t InMeanOff, uint32_t InMeanNDim,
    uint32_t InMeanDimsOff, uint32_t InVarOff, uint32_t InVarNDim,
    uint32_t InVarDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff, uint32_t OutMeanOff, uint32_t OutMeanNDim,
    uint32_t OutMeanDimsOff, uint32_t OutVarOff, uint32_t OutVarNDim,
    uint32_t OutVarDimsOff, uint32_t OutSavedMeanOff, uint32_t OutSavedMeanNDim,
    uint32_t OutSavedMeanDimsOff, uint32_t OutSavedVarOff,
    uint32_t OutSavedVarNDim, uint32_t OutSavedVarDimsOff, float Epsilon,
    float Momentum, uint32_t Spatial) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      const float *input_scale,
  ///      int32_t input_scale_ndim,
  ///      const int32_t *input_scale_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      const float *input_mean,
  ///      int32_t input_mean_ndim,
  ///      const int32_t *input_mean_dims,
  ///      const float *input_var,
  ///      int32_t input_var_ndim,
  ///      const int32_t *input_var_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float *output_mean,
  ///      int32_t output_mean_ndim,
  ///      const int32_t *output_mean_dims,
  ///      float *output_var,
  ///      int32_t output_var_ndim,
  ///      const int32_t *output_var_dims,
  ///      float *output_saved_mean,
  ///      int32_t output_saved_mean_ndim,
  ///      const int32_t *output_saved_mean_dims,
  ///      float *output_saved_var,
  ///      int32_t output_saved_var_ndim,
  ///      const int32_t *output_saved_var_dims,
  ///      float epsilon,
  ///      float momentum,
  ///      int32_t spatial
  /// Optional: output_mean, output_var, output_saved_mean, output_saved_var

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *InScaleDims = MemInst.getPointer<int32_t *>(InScaleDimsOff);
  int32_t *InBDims = MemInst.getPointer<int32_t *>(InBDimsOff);
  int32_t *InMeanDims = MemInst.getPointer<int32_t *>(InMeanDimsOff);
  int32_t *InVarDims = MemInst.getPointer<int32_t *>(InVarDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  int32_t *OutMeanDims = MemInst.getPointerOrNull<int32_t *>(OutMeanDimsOff);
  int32_t *OutVarDims = MemInst.getPointerOrNull<int32_t *>(OutVarDimsOff);
  int32_t *OutSavedMeanDims =
      MemInst.getPointerOrNull<int32_t *>(OutSavedMeanDimsOff);
  int32_t *OutSavedVarDims =
      MemInst.getPointerOrNull<int32_t *>(OutSavedVarDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *InScale = MemInst.getPointer<float *>(InScaleOff);
  float *InB = MemInst.getPointer<float *>(InBOff);
  float *InMean = MemInst.getPointer<float *>(InMeanOff);
  float *InVar = MemInst.getPointer<float *>(InVarOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);
  float *OutMean = MemInst.getPointerOrNull<float *>(OutMeanOff);
  float *OutVar = MemInst.getPointerOrNull<float *>(OutVarOff);
  float *OutSavedMean = MemInst.getPointerOrNull<float *>(OutSavedMeanOff);
  float *OutSavedVar = MemInst.getPointerOrNull<float *>(OutSavedVarOff);

  ONNC_RUNTIME_batchnormalization_float(
      RuntimeContext, InX, InXNDim, InXDims, InScale, InScaleNDim, InScaleDims,
      InB, InBNDim, InBDims, InMean, InMeanNDim, InMeanDims, InVar, InVarNDim,
      InVarDims, OutY, OutYNDim, OutYDims, OutMean, OutMeanNDim, OutMeanDims,
      OutVar, OutVarNDim, OutVarDims, OutSavedMean, OutSavedMeanNDim,
      OutSavedMeanDims, OutSavedVar, OutSavedVarNDim, OutSavedVarDims, Epsilon,
      Momentum, Spatial);

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
