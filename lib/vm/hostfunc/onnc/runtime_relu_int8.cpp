// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_relu_int8.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ErrCode ONNCRuntimeReluInt8::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst,
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

} // namespace Executor
} // namespace SSVM
