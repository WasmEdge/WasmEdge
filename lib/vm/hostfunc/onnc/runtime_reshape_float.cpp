#include "vm/hostfunc/onnc/runtime_reshape_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeReshapeFloat::ONNCRuntimeReshapeFloat() {
  appendParamDef(AST::ValType::I32);
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

ErrCode ONNCRuntimeReshapeFloat::run(std::vector<Value> &Args,
                                     std::vector<Value> &Res,
                                     StoreManager &Store,
                                     Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_data,
  ///      int32_t input_data_ndim,
  ///      const int32_t *input_data_dims,
  ///      const float *input_shape,
  ///      int32_t input_shape_ndim,
  ///      const int32_t *input_shape_dims,
  ///      float *output_reshaped,
  ///      int32_t output_reshaped_ndim,
  ///      const int32_t *output_reshaped_dims
  if (Args.size() != 10) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[9]);
  unsigned int InDataOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int InDataNDim = retrieveValue<uint32_t>(Args[7]);
  unsigned int InDataDimsOff = retrieveValue<uint32_t>(Args[6]);
  unsigned int InShapeOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int InShapeNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int InShapeDimsOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutReshapedOff = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutReshapedNDim = retrieveValue<uint32_t>(Args[1]);
  unsigned int OutReshapedDimsOff = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *InShapeDims = MemInst->getPointer<int32_t *>(InShapeDimsOff);
  int32_t *OutReshapedDims = MemInst->getPointer<int32_t *>(OutReshapedDimsOff);
  float *InData = MemInst->getPointer<float *>(InDataOff);
  float *InShape = MemInst->getPointer<float *>(InShapeOff);
  float *OutReshaped = MemInst->getPointer<float *>(OutReshapedOff);

  ONNC_RUNTIME_reshape_float(RuntimeContext, InData, InDataNDim, InDataDims,
                             InShape, InShapeNDim, InShapeDims, OutReshaped,
                             OutReshapedNDim, OutReshapedDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
