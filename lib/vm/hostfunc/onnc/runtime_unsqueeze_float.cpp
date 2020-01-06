#include "vm/hostfunc/onnc/runtime_unsqueeze_float.h"
#include "onnc/onnc_runtime.h"

namespace SSVM {
namespace Executor {

ONNCRuntimeUnsqueezeFloat::ONNCRuntimeUnsqueezeFloat() {
  initializeFuncType<ONNCRuntimeUnsqueezeFloat>();
}

ErrCode ONNCRuntimeUnsqueezeFloat::run(VM::EnvironmentManager &EnvMgr,
                                       StackManager &StackMgr,
                                       Instance::MemoryInstance &MemInst) {
  return invoke<ONNCRuntimeUnsqueezeFloat>(EnvMgr, StackMgr, MemInst);
}

ErrCode ONNCRuntimeUnsqueezeFloat::body(
    VM::EnvironmentManager &EnvMgr, Instance::MemoryInstance &MemInst,
    uint32_t RuntimeContextOff, uint32_t InDataOff, uint32_t InDataNDim,
    uint32_t InDataDimsOff, uint32_t OutExpandedOff, uint32_t OutExpandedNDim,
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

} // namespace Executor
} // namespace SSVM
