// SPDX-License-Identifier: Apache-2.0
#include "runtime/instance/memory.h"
#include "host/onnc/onncfunc.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Host {

ErrCode ONNCRuntimeAddFloat::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t RuntimeContextOff, uint32_t InAOff,
                                  uint32_t InANDim, uint32_t InADimsOff,
                                  uint32_t InBOff, uint32_t InBNDim,
                                  uint32_t InBDimsOff, uint32_t OutCOff,
                                  uint32_t OutCNDim, uint32_t OutCDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      float *output_C,
  ///      int32_t output_C_ndim,
  ///      const int32_t *output_C_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InADims = MemInst.getPointer<int32_t *>(InADimsOff);
  int32_t *InBDims = MemInst.getPointer<int32_t *>(InBDimsOff);
  int32_t *OutCDims = MemInst.getPointer<int32_t *>(OutCDimsOff);
  float *InA = MemInst.getPointer<float *>(InAOff);
  float *InB = MemInst.getPointer<float *>(InBOff);
  float *OutC = MemInst.getPointer<float *>(OutCOff);

  ONNC_RUNTIME_add_float(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                         InBDims, OutC, OutCNDim, OutCDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeAddInt8::body(Runtime::Instance::MemoryInstance &MemInst,
                                 uint32_t RuntimeContextOff, uint32_t InAOff,
                                 uint32_t InANDim, uint32_t InADimsOff,
                                 uint32_t InBOff, uint32_t InBNDim,
                                 uint32_t InBDimsOff, uint32_t OutCOff,
                                 uint32_t OutCNDim, uint32_t OutCDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const int8_t *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      int8_t *output_C,
  ///      int32_t output_C_ndim,
  ///      const int32_t *output_C_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InADims = MemInst.getPointer<int32_t *>(InADimsOff);
  int32_t *InBDims = MemInst.getPointer<int32_t *>(InBDimsOff);
  int32_t *OutCDims = MemInst.getPointer<int32_t *>(OutCDimsOff);
  int8_t *InA = MemInst.getPointer<int8_t *>(InAOff);
  int8_t *InB = MemInst.getPointer<int8_t *>(InBOff);
  int8_t *OutC = MemInst.getPointer<int8_t *>(OutCOff);

  ONNC_RUNTIME_add_int8(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                        InBDims, OutC, OutCNDim, OutCDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeAveragepoolFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, uint32_t AutoPadOff,
    uint32_t IncludePadCnt, uint32_t KernelShapeOff, uint32_t KernelShapeNum,
    uint32_t PadsOff, uint32_t PadsNum, uint32_t StridesOff,
    uint32_t StridesNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      const char *auto_pad,
  ///      int32_t count_include_pad,
  ///      int32_t *kernel_shape,
  ///      int32_t number_of_kernel_shape,
  ///      int32_t *pads,
  ///      int32_t number_of_pads,
  ///      int32_t *strides,
  ///      int32_t number_of_strides

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);
  char *AutoPad = MemInst.getPointer<char *>(AutoPadOff);
  int32_t *KernelShape = MemInst.getPointer<int32_t *>(KernelShapeOff);
  int32_t *Pads = MemInst.getPointer<int32_t *>(PadsOff);
  int32_t *Strides = MemInst.getPointer<int32_t *>(StridesOff);

  ONNC_RUNTIME_averagepool_float(RuntimeContext, InX, InXNDim, InXDims, OutY,
                                 OutYNDim, OutYDims, AutoPad, IncludePadCnt,
                                 KernelShape, KernelShapeNum, Pads, PadsNum,
                                 Strides, StridesNum);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeBatchnormalizationFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t InScaleOff,
    uint32_t InScaleNDim, uint32_t InScaleDimsOff, uint32_t InBOff,
    uint32_t InBNDim, uint32_t InBDimsOff, uint32_t InMeanOff,
    uint32_t InMeanNDim, uint32_t InMeanDimsOff, uint32_t InVarOff,
    uint32_t InVarNDim, uint32_t InVarDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, uint32_t OutMeanOff,
    uint32_t OutMeanNDim, uint32_t OutMeanDimsOff, uint32_t OutVarOff,
    uint32_t OutVarNDim, uint32_t OutVarDimsOff, uint32_t OutSavedMeanOff,
    uint32_t OutSavedMeanNDim, uint32_t OutSavedMeanDimsOff,
    uint32_t OutSavedVarOff, uint32_t OutSavedVarNDim,
    uint32_t OutSavedVarDimsOff, float Epsilon, float Momentum,
    uint32_t Spatial) {
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

ErrCode ONNCRuntimeBatchnormalizationInt8::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t InScaleOff,
    uint32_t InScaleNDim, uint32_t InScaleDimsOff, uint32_t InBOff,
    uint32_t InBNDim, uint32_t InBDimsOff, uint32_t InMeanOff,
    uint32_t InMeanNDim, uint32_t InMeanDimsOff, uint32_t InVarOff,
    uint32_t InVarNDim, uint32_t InVarDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, uint32_t OutMeanOff,
    uint32_t OutMeanNDim, uint32_t OutMeanDimsOff, uint32_t OutVarOff,
    uint32_t OutVarNDim, uint32_t OutVarDimsOff, uint32_t OutSavedMeanOff,
    uint32_t OutSavedMeanNDim, uint32_t OutSavedMeanDimsOff,
    uint32_t OutSavedVarOff, uint32_t OutSavedVarNDim,
    uint32_t OutSavedVarDimsOff, int32_t Epsilon, int32_t Momentum,
    uint32_t Spatial) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      const int8_t *input_scale,
  ///      int32_t input_scale_ndim,
  ///      const int32_t *input_scale_dims,
  ///      const int8_t *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      const int8_t *input_mean,
  ///      int32_t input_mean_ndim,
  ///      const int32_t *input_mean_dims,
  ///      const int8_t *input_var,
  ///      int32_t input_var_ndim,
  ///      const int32_t *input_var_dims,
  ///      int8_t *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      int8_t *output_mean,
  ///      int32_t output_mean_ndim,
  ///      const int32_t *output_mean_dims,
  ///      int8_t *output_var,
  ///      int32_t output_var_ndim,
  ///      const int32_t *output_var_dims,
  ///      int8_t *output_saved_mean,
  ///      int32_t output_saved_mean_ndim,
  ///      const int32_t *output_saved_mean_dims,
  ///      int8_t *output_saved_var,
  ///      int32_t output_saved_var_ndim,
  ///      const int32_t *output_saved_var_dims,
  ///      int8_t epsilon,
  ///      int8_t momentum,
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
  int8_t *InX = MemInst.getPointer<int8_t *>(InXOff);
  int8_t *InScale = MemInst.getPointer<int8_t *>(InScaleOff);
  int8_t *InB = MemInst.getPointer<int8_t *>(InBOff);
  int8_t *InMean = MemInst.getPointer<int8_t *>(InMeanOff);
  int8_t *InVar = MemInst.getPointer<int8_t *>(InVarOff);
  int8_t *OutY = MemInst.getPointer<int8_t *>(OutYOff);
  int8_t *OutMean = MemInst.getPointerOrNull<int8_t *>(OutMeanOff);
  int8_t *OutVar = MemInst.getPointerOrNull<int8_t *>(OutVarOff);
  int8_t *OutSavedMean = MemInst.getPointerOrNull<int8_t *>(OutSavedMeanOff);
  int8_t *OutSavedVar = MemInst.getPointerOrNull<int8_t *>(OutSavedVarOff);

  ONNC_RUNTIME_batchnormalization_int8(
      RuntimeContext, InX, InXNDim, InXDims, InScale, InScaleNDim, InScaleDims,
      InB, InBNDim, InBDims, InMean, InMeanNDim, InMeanDims, InVar, InVarNDim,
      InVarDims, OutY, OutYNDim, OutYDims, OutMean, OutMeanNDim, OutMeanDims,
      OutVar, OutVarNDim, OutVarDims, OutSavedMean, OutSavedMeanNDim,
      OutSavedMeanDims, OutSavedVar, OutSavedVarNDim, OutSavedVarDims, Epsilon,
      Momentum, Spatial);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeConcatFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InInputsOffOff, uint32_t InInputsNTensor, uint32_t InInputsNDimOff,
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

ErrCode ONNCRuntimeConvFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t InWOff,
    uint32_t InWNDim, uint32_t InWDimsOff, uint32_t InBOff, uint32_t InBNDim,
    uint32_t InBDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff, uint32_t AutoPadOff, uint32_t DelationsOff,
    uint32_t DelationNum, uint32_t Group, uint32_t KernelShapeOff,
    uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
    uint32_t StridesOff, uint32_t StridesNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const float* input_X,
  ///      int32_t input_X_NDim,
  ///      const int32_t* input_X_dims,
  ///      const float* input_W,
  ///      int32_t input_W_NDim,
  ///      const int32_t* input_W_dims,
  ///      const float* input_B,
  ///      int32_t input_B_NDim,
  ///      const int32_t* input_B_dims,
  ///      float* output_Y,
  ///      int32_t output_Y_NDim,
  ///      const int32_t* output_Y_dims,
  ///      const char* auto_pad,
  ///      int32_t* dilations,
  ///      int32_t number_of_dilations,
  ///      int32_t group,
  ///      int32_t* kernel_shape,
  ///      int32_t number_of_kernel_shape,
  ///      int32_t* pads,
  ///      int32_t number_of_pads,
  ///      int32_t* strides,
  ///      int32_t number_of_strides
  /// Optional: input_B

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *InWDims = MemInst.getPointer<int32_t *>(InWDimsOff);
  int32_t *InBDims = MemInst.getPointerOrNull<int32_t *>(InBDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *InW = MemInst.getPointer<float *>(InWOff);
  float *InB = MemInst.getPointerOrNull<float *>(InBOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);
  char *AutoPad = MemInst.getPointer<char *>(AutoPadOff);
  int32_t *Delations = MemInst.getPointer<int32_t *>(DelationsOff);
  int32_t *KernelShape = MemInst.getPointer<int32_t *>(KernelShapeOff);
  int32_t *Pads = MemInst.getPointer<int32_t *>(PadsOff);
  int32_t *Strides = MemInst.getPointer<int32_t *>(StridesOff);

  ONNC_RUNTIME_conv_float(RuntimeContext, InX, InXNDim, InXDims, InW, InWNDim,
                          InWDims, InB, InBNDim, InBDims, OutY, OutYNDim,
                          OutYDims, AutoPad, Delations, DelationNum, Group,
                          KernelShape, KernelShapeNum, Pads, PadsNum, Strides,
                          StridesNum);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeConvInt8::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t InWOff,
    uint32_t InWNDim, uint32_t InWDimsOff, uint32_t InBOff, uint32_t InBNDim,
    uint32_t InBDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff, uint32_t AutoPadOff, uint32_t DelationsOff,
    uint32_t DelationNum, uint32_t Group, uint32_t KernelShapeOff,
    uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
    uint32_t StridesOff, uint32_t StridesNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t* input_X,
  ///      int32_t input_X_NDim,
  ///      const int32_t* input_X_dims,
  ///      const int8_t* input_W,
  ///      int32_t input_W_NDim,
  ///      const int32_t* input_W_dims,
  ///      const int8_t* input_B,
  ///      int32_t input_B_NDim,
  ///      const int32_t* input_B_dims,
  ///      int8_t* output_Y,
  ///      int32_t output_Y_NDim,
  ///      const int32_t* output_Y_dims,
  ///      const char* auto_pad,
  ///      int32_t* dilations,
  ///      int32_t number_of_dilations,
  ///      int32_t group,
  ///      int32_t* kernel_shape,
  ///      int32_t number_of_kernel_shape,
  ///      int32_t* pads,
  ///      int32_t number_of_pads,
  ///      int32_t* strides,
  ///      int32_t number_of_strides
  /// Optional: input_B

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *InWDims = MemInst.getPointer<int32_t *>(InWDimsOff);
  int32_t *InBDims = MemInst.getPointerOrNull<int32_t *>(InBDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  int8_t *InX = MemInst.getPointer<int8_t *>(InXOff);
  int8_t *InW = MemInst.getPointer<int8_t *>(InWOff);
  int8_t *InB = MemInst.getPointerOrNull<int8_t *>(InBOff);
  int8_t *OutY = MemInst.getPointer<int8_t *>(OutYOff);
  char *AutoPad = MemInst.getPointer<char *>(AutoPadOff);
  int32_t *Delations = MemInst.getPointer<int32_t *>(DelationsOff);
  int32_t *KernelShape = MemInst.getPointer<int32_t *>(KernelShapeOff);
  int32_t *Pads = MemInst.getPointer<int32_t *>(PadsOff);
  int32_t *Strides = MemInst.getPointer<int32_t *>(StridesOff);

  ONNC_RUNTIME_conv_int8(RuntimeContext, InX, InXNDim, InXDims, InW, InWNDim,
                         InWDims, InB, InBNDim, InBDims, OutY, OutYNDim,
                         OutYDims, AutoPad, Delations, DelationNum, Group,
                         KernelShape, KernelShapeNum, Pads, PadsNum, Strides,
                         StridesNum);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeGemmFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InAOff, uint32_t InANDim, uint32_t InADimsOff, uint32_t InBOff,
    uint32_t InBNDim, uint32_t InBDimsOff, uint32_t InCOff, uint32_t InCNDim,
    uint32_t InCDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff, float Alpha, float Beta, uint32_t TransA,
    uint32_t TransB) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      const float *input_C,
  ///      int32_t input_C_ndim,
  ///      const int32_t *input_C_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float alpha,
  ///      float beta,
  ///      int32_t transA,
  ///      int32_t transB
  /// Optional: input_C

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InADims = MemInst.getPointer<int32_t *>(InADimsOff);
  int32_t *InBDims = MemInst.getPointer<int32_t *>(InBDimsOff);
  int32_t *InCDims = MemInst.getPointerOrNull<int32_t *>(InCDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  float *InA = MemInst.getPointer<float *>(InAOff);
  float *InB = MemInst.getPointer<float *>(InBOff);
  float *InC = MemInst.getPointerOrNull<float *>(InCOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);

  ONNC_RUNTIME_gemm_float(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                          InBDims, InC, InCNDim, InCDims, OutY, OutYNDim,
                          OutYDims, Alpha, Beta, TransA, TransB);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeGlobalaveragepoolFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);

  ONNC_RUNTIME_globalaveragepool_float(RuntimeContext, InX, InXNDim, InXDims,
                                       OutY, OutYNDim, OutYDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeLrnFloat::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t RuntimeContextOff, uint32_t InXOff,
                                  uint32_t InXNDim, uint32_t InXDimsOff,
                                  uint32_t OutYOff, uint32_t OutYNDim,
                                  uint32_t OutYDimsOff, float Alpha, float Beta,
                                  float Bias, uint32_t Size) {
  /// Arg: void* onnc_runtime_context,
  ///      const float* input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float alpha,
  ///      float beta,
  ///      float bias,
  ///      int32_t size

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);

  ONNC_RUNTIME_lrn_float(RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim,
                         OutYDims, Alpha, Beta, Bias, Size);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeMaxpoolFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, uint32_t OutIndicesOff,
    uint32_t OutIndicesNDim, uint32_t OutIndicesDimsOff, uint32_t AutoPadOff,
    uint32_t KernelShapeOff, uint32_t KernelShapeNum, uint32_t PadsOff,
    uint32_t PadsNum, uint32_t StorageOrder, uint32_t StridesOff,
    uint32_t StridesNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float *output_Indices,
  ///      int32_t output_Indices_ndim,
  ///      const int32_t *output_Indices_dims,
  ///      const char *auto_pad,
  ///      int32_t *kernel_shape,
  ///      int32_t number_of_kernel_shape,
  ///      int32_t *pads,
  ///      int32_t number_of_pads,
  ///      int32_t storage_order,
  ///      int32_t *strides,
  ///      int32_t number_of_strides
  /// Optional: output_Indices

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  int32_t *OutIndicesDims =
      MemInst.getPointerOrNull<int32_t *>(OutIndicesDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);
  float *OutIndices = MemInst.getPointerOrNull<float *>(OutIndicesOff);
  char *AutoPad = MemInst.getPointer<char *>(AutoPadOff);
  int32_t *KernelShape = MemInst.getPointer<int32_t *>(KernelShapeOff);
  int32_t *Pads = MemInst.getPointer<int32_t *>(PadsOff);
  int32_t *Strides = MemInst.getPointer<int32_t *>(StridesOff);

  ONNC_RUNTIME_maxpool_float(
      RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim, OutYDims,
      OutIndices, OutIndicesNDim, OutIndicesDims, AutoPad, KernelShape,
      KernelShapeNum, Pads, PadsNum, StorageOrder, Strides, StridesNum);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeMaxpoolInt8::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, uint32_t OutIndicesOff,
    uint32_t OutIndicesNDim, uint32_t OutIndicesDimsOff, uint32_t AutoPadOff,
    uint32_t KernelShapeOff, uint32_t KernelShapeNum, uint32_t PadsOff,
    uint32_t PadsNum, uint32_t StorageOrder, uint32_t StridesOff,
    uint32_t StridesNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      int8_t *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      int8_t *output_Indices,
  ///      int32_t output_Indices_ndim,
  ///      const int32_t *output_Indices_dims,
  ///      const char *auto_pad,
  ///      int32_t *kernel_shape,
  ///      int32_t number_of_kernel_shape,
  ///      int32_t *pads,
  ///      int32_t number_of_pads,
  ///      int32_t storage_order,
  ///      int32_t *strides,
  ///      int32_t number_of_strides
  /// Optional: output_Indices

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  int32_t *OutIndicesDims =
      MemInst.getPointerOrNull<int32_t *>(OutIndicesDimsOff);
  int8_t *InX = MemInst.getPointer<int8_t *>(InXOff);
  int8_t *OutY = MemInst.getPointer<int8_t *>(OutYOff);
  int8_t *OutIndices = MemInst.getPointerOrNull<int8_t *>(OutIndicesOff);
  char *AutoPad = MemInst.getPointer<char *>(AutoPadOff);
  int32_t *KernelShape = MemInst.getPointer<int32_t *>(KernelShapeOff);
  int32_t *Pads = MemInst.getPointer<int32_t *>(PadsOff);
  int32_t *Strides = MemInst.getPointer<int32_t *>(StridesOff);

  ONNC_RUNTIME_maxpool_int8(
      RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim, OutYDims,
      OutIndices, OutIndicesNDim, OutIndicesDims, AutoPad, KernelShape,
      KernelShapeNum, Pads, PadsNum, StorageOrder, Strides, StridesNum);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeMulFloat::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t RuntimeContextOff, uint32_t InAOff,
                                  uint32_t InANDim, uint32_t InADimsOff,
                                  uint32_t InBOff, uint32_t InBNDim,
                                  uint32_t InBDimsOff, uint32_t OutCOff,
                                  uint32_t OutCNDim, uint32_t OutCDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      float *output_C,
  ///      int32_t output_C_ndim,
  ///      const int32_t *output_C_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InADims = MemInst.getPointer<int32_t *>(InADimsOff);
  int32_t *InBDims = MemInst.getPointer<int32_t *>(InBDimsOff);
  int32_t *OutCDims = MemInst.getPointer<int32_t *>(OutCDimsOff);
  float *InA = MemInst.getPointer<float *>(InAOff);
  float *InB = MemInst.getPointer<float *>(InBOff);
  float *OutC = MemInst.getPointer<float *>(OutCOff);

  ONNC_RUNTIME_mul_float(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                         InBDims, OutC, OutCNDim, OutCDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeMulInt8::body(Runtime::Instance::MemoryInstance &MemInst,
                                 uint32_t RuntimeContextOff, uint32_t InAOff,
                                 uint32_t InANDim, uint32_t InADimsOff,
                                 uint32_t InBOff, uint32_t InBNDim,
                                 uint32_t InBDimsOff, uint32_t OutCOff,
                                 uint32_t OutCNDim, uint32_t OutCDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const int8_t *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      int8_t *output_C,
  ///      int32_t output_C_ndim,
  ///      const int32_t *output_C_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InADims = MemInst.getPointer<int32_t *>(InADimsOff);
  int32_t *InBDims = MemInst.getPointer<int32_t *>(InBDimsOff);
  int32_t *OutCDims = MemInst.getPointer<int32_t *>(OutCDimsOff);
  int8_t *InA = MemInst.getPointer<int8_t *>(InAOff);
  int8_t *InB = MemInst.getPointer<int8_t *>(InBOff);
  int8_t *OutC = MemInst.getPointer<int8_t *>(OutCOff);

  ONNC_RUNTIME_mul_int8(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                        InBDims, OutC, OutCNDim, OutCDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeReluFloat::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t RuntimeContextOff, uint32_t InXOff,
                                   uint32_t InXNDim, uint32_t InXDimsOff,
                                   uint32_t OutYOff, uint32_t OutYNDim,
                                   uint32_t OutYDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const float* input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t* input_X_dims,
  ///      float* output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t* output_Y_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  float *InX = MemInst.getPointer<float *>(InXOff);
  float *OutY = MemInst.getPointer<float *>(OutYOff);

  ONNC_RUNTIME_relu_float(RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim,
                          OutYDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeReluInt8::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t RuntimeContextOff, uint32_t InXOff,
                                  uint32_t InXNDim, uint32_t InXDimsOff,
                                  uint32_t OutYOff, uint32_t OutYNDim,
                                  uint32_t OutYDimsOff) {
  /// Arg: void* onnc_runtime_context,
  ///      const int8_t* input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t* input_X_dims,
  ///      int8_t* output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t* output_Y_dims

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst.getPointer<int32_t *>(InXDimsOff);
  int32_t *OutYDims = MemInst.getPointer<int32_t *>(OutYDimsOff);
  int8_t *InX = MemInst.getPointer<int8_t *>(InXOff);
  int8_t *OutY = MemInst.getPointer<int8_t *>(OutYOff);

  ONNC_RUNTIME_relu_int8(RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim,
                         OutYDims);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeReshapeFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InDataOff, uint32_t InDataNDim, uint32_t InDataDimsOff,
    uint32_t InShapeOff, uint32_t InShapeNDim, uint32_t InShapeDimsOff,
    uint32_t OutReshapedOff, uint32_t OutReshapedNDim,
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

ErrCode ONNCRuntimeSoftmaxFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InOff, uint32_t InNDim, uint32_t InDimsOff, uint32_t OutOff,
    uint32_t OutNDim, uint32_t OutDimsOff, uint32_t Axis) {
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

ErrCode ONNCRuntimeSumFloat::body(Runtime::Instance::MemoryInstance &MemInst,
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

ErrCode ONNCRuntimeTransposeFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InDataOff, uint32_t InDataNDim, uint32_t InDataDimsOff,
    uint32_t OutTransposedOff, uint32_t OutTransposedNDim,
    uint32_t OutTransposedDimsOff, uint32_t PermOff, uint32_t PermNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_data,
  ///      int32_t input_data_ndim,
  ///      const int32_t *input_data_dims,
  ///      float *output_transposed,
  ///      int32_t output_transposed_ndim,
  ///      const int32_t *output_transposed_dims,
  ///      int32_t *perm,
  ///      int32_t number_of_perm

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InDataDims = MemInst.getPointer<int32_t *>(InDataDimsOff);
  int32_t *OutTransposedDims =
      MemInst.getPointer<int32_t *>(OutTransposedDimsOff);
  float *InData = MemInst.getPointer<float *>(InDataOff);
  float *OutTransposed = MemInst.getPointer<float *>(OutTransposedOff);
  int32_t *Perm = MemInst.getPointer<int32_t *>(PermOff);

  ONNC_RUNTIME_transpose_float(RuntimeContext, InData, InDataNDim, InDataDims,
                               OutTransposed, OutTransposedNDim,
                               OutTransposedDims, Perm, PermNum);

  return ErrCode::Success;
}

ErrCode ONNCRuntimeUnsqueezeFloat::body(
    Runtime::Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
    uint32_t InDataOff, uint32_t InDataNDim, uint32_t InDataDimsOff,
    uint32_t OutExpandedOff, uint32_t OutExpandedNDim,
    uint32_t OutExpandedDimsOff, uint32_t AxesOff, uint32_t AxesNum) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_data,
  ///      int32_t input_data_ndim,
  ///      const int32_t *input_data_dims,
  ///      float *output_expanded,
  ///      int32_t output_expanded_ndim,
  ///      const int32_t *output_expanded_dims,
  ///      int32_t *axes,
  ///      int32_t number_of_axes

  void *RuntimeContext = MemInst.getPointer<void *>(RuntimeContextOff);
  int32_t *InDataDims = MemInst.getPointer<int32_t *>(InDataDimsOff);
  int32_t *OutExpandedDims = MemInst.getPointer<int32_t *>(OutExpandedDimsOff);
  float *InData = MemInst.getPointer<float *>(InDataOff);
  float *OutExpanded = MemInst.getPointer<float *>(OutExpandedOff);
  int32_t *Axes = MemInst.getPointer<int32_t *>(AxesOff);

  ONNC_RUNTIME_unsqueeze_float(RuntimeContext, InData, InDataNDim, InDataDims,
                               OutExpanded, OutExpandedNDim, OutExpandedDims,
                               Axes, AxesNum);

  return ErrCode::Success;
}

} // namespace Host
} // namespace SSVM