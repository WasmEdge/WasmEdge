#include "vm/hostfunc/onnc/runtime_transpose_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeTransposeFloat::ONNCRuntimeTransposeFloat() {
  initializeFuncType<ONNCRuntimeTransposeFloat>();
}

ErrCode ONNCRuntimeTransposeFloat::run(VM::EnvironmentManager &EnvMgr,
                                       StackManager &StackMgr,
                                       Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeTransposeFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeTransposeFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InDataOff, uint32_t InDataNDim,
    uint32_t InDataDimsOff, uint32_t OutTransposedOff,
    uint32_t OutTransposedNDim, uint32_t OutTransposedDimsOff, uint32_t PermOff,
    uint32_t PermNum) {
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

} // namespace Executor
} // namespace SSVM
