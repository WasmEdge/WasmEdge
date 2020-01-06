// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_globalaveragepool_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeGlobalaveragepoolFloat::ONNCRuntimeGlobalaveragepoolFloat() {
  initializeFuncType<ONNCRuntimeGlobalaveragepoolFloat>();
}

ErrCode
ONNCRuntimeGlobalaveragepoolFloat::run(VM::EnvironmentManager &EnvMgr,
                                       StackManager &StackMgr,
                                       Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeGlobalaveragepoolFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeGlobalaveragepoolFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
    uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff) {
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

} // namespace Executor
} // namespace SSVM
