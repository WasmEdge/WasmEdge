// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_add_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeAddFloat::ONNCRuntimeAddFloat() {
  initializeFuncType<ONNCRuntimeAddFloat>();
}

ErrCode ONNCRuntimeAddFloat::run(VM::EnvironmentManager &EnvMgr,
                                 StackManager &StackMgr,
                                 Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeAddFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeAddFloat::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst,
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

} // namespace Executor
} // namespace SSVM
