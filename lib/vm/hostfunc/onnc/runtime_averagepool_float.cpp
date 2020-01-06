// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_averagepool_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeAveragepoolFloat::ONNCRuntimeAveragepoolFloat() {
  initializeFuncType<ONNCRuntimeAveragepoolFloat>();
}

ErrCode ONNCRuntimeAveragepoolFloat::run(VM::EnvironmentManager &EnvMgr,
                                         StackManager &StackMgr,
                                         Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeAveragepoolFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeAveragepoolFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
    uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff, uint32_t AutoPadOff, uint32_t IncludePadCnt,
    uint32_t KernelShapeOff, uint32_t KernelShapeNum, uint32_t PadsOff,
    uint32_t PadsNum, uint32_t StridesOff, uint32_t StridesNum) {
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

} // namespace Executor
} // namespace SSVM
