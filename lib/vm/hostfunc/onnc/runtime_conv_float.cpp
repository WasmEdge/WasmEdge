// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_conv_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeConvFloat::ONNCRuntimeConvFloat() {
  initializeFuncType<ONNCRuntimeConvFloat>();
}

ErrCode ONNCRuntimeConvFloat::run(VM::EnvironmentManager &EnvMgr,
                                  StackManager &StackMgr,
                                  Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeConvFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeConvFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
    uint32_t InXDimsOff, uint32_t InWOff, uint32_t InWNDim, uint32_t InWDimsOff,
    uint32_t InBOff, uint32_t InBNDim, uint32_t InBDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, uint32_t AutoPadOff,
    uint32_t DelationsOff, uint32_t DelationNum, uint32_t Group,
    uint32_t KernelShapeOff, uint32_t KernelShapeNum, uint32_t PadsOff,
    uint32_t PadsNum, uint32_t StridesOff, uint32_t StridesNum) {
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

} // namespace Executor
} // namespace SSVM
