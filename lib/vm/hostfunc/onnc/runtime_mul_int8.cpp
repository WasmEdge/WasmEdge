// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_mul_int8.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeMulInt8::ONNCRuntimeMulInt8() {
  initializeFuncType<ONNCRuntimeMulInt8>();
}

ErrCode ONNCRuntimeMulInt8::run(VM::EnvironmentManager &EnvMgr,
                                StackManager &StackMgr,
                                Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeMulInt8>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeMulInt8::body(VM::EnvironmentManager &EnvMgr,
                                 Instance::MemoryInstance &MemInst,
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

} // namespace Executor
} // namespace SSVM
