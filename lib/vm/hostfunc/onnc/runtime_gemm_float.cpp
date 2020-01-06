// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_gemm_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeGemmFloat::ONNCRuntimeGemmFloat() {
  initializeFuncType<ONNCRuntimeGemmFloat>();
}

ErrCode ONNCRuntimeGemmFloat::run(VM::EnvironmentManager &EnvMgr,
                                  StackManager &StackMgr,
                                  Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeGemmFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeGemmFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InAOff, uint32_t InANDim,
    uint32_t InADimsOff, uint32_t InBOff, uint32_t InBNDim, uint32_t InBDimsOff,
    uint32_t InCOff, uint32_t InCNDim, uint32_t InCDimsOff, uint32_t OutYOff,
    uint32_t OutYNDim, uint32_t OutYDimsOff, float Alpha, float Beta,
    uint32_t TransA, uint32_t TransB) {
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

} // namespace Executor
} // namespace SSVM
