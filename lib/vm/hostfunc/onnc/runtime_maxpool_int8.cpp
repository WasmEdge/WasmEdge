// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_maxpool_int8.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeMaxpoolInt8::ONNCRuntimeMaxpoolInt8() {
  initializeFuncType<ONNCRuntimeMaxpoolInt8>();
}

ErrCode ONNCRuntimeMaxpoolInt8::run(VM::EnvironmentManager &EnvMgr,
                                    StackManager &StackMgr,
                                    Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeMaxpoolInt8>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeMaxpoolInt8::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
    uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
    uint32_t OutYDimsOff, uint32_t OutIndicesOff, uint32_t OutIndicesNDim,
    uint32_t OutIndicesDimsOff, uint32_t AutoPadOff, uint32_t KernelShapeOff,
    uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
    uint32_t StorageOrder, uint32_t StridesOff, uint32_t StridesNum) {
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

} // namespace Executor
} // namespace SSVM
