#include "vm/hostfunc/onnc/runtime_unsqueeze_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeUnsqueezeFloat::ONNCRuntimeUnsqueezeFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeUnsqueezeFloat::run(VM::EnvironmentManager &EnvMgr,
                                       std::vector<Value> &Args,
                                       std::vector<Value> &Res,
                                       StoreManager &Store,
                                       Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_data,
  ///      int32_t input_data_ndim,
  ///      const int32_t *input_data_dims,
  ///      float *output_expanded,
  ///      int32_t output_expanded_ndim,
  ///      const int32_t *output_expanded_dims,
  ///      int32_t *axes,
  ///      int32_t number_of_axes
  if (Args.size() != 9) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int InDataOff = retrieveValue<uint32_t>(Args[7]);
  unsigned int InDataNDim = retrieveValue<uint32_t>(Args[6]);
  unsigned int InDataDimsOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int OutExpandedOff = retrieveValue<uint32_t>(Args[4]);
  unsigned int OutExpandedNDim = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutExpandedDimsOff = retrieveValue<uint32_t>(Args[2]);
  unsigned int AxesOff = retrieveValue<uint32_t>(Args[1]);
  unsigned int AxesNum = retrieveValue<uint32_t>(Args[0]);

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  void *RuntimeContext = MemInst->getPointer<void *>(RuntimeContextOff);
  int32_t *InDataDims = MemInst->getPointer<int32_t *>(InDataDimsOff);
  int32_t *OutExpandedDims = MemInst->getPointer<int32_t *>(OutExpandedDimsOff);
  float *InData = MemInst->getPointer<float *>(InDataOff);
  float *OutExpanded = MemInst->getPointer<float *>(OutExpandedOff);
  int32_t *Axes = MemInst->getPointer<int32_t *>(AxesOff);

  ONNC_RUNTIME_unsqueeze_float(RuntimeContext, InData, InDataNDim, InDataDims,
                               OutExpanded, OutExpandedNDim, OutExpandedDims,
                               Axes, AxesNum);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
