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
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(Args[9]);
  unsigned int InDataPtr = retrieveValue<uint32_t>(Args[8]);
  unsigned int InDataNDim = retrieveValue<uint32_t>(Args[7]);
  unsigned int InDataDimsPtr = retrieveValue<uint32_t>(Args[6]);
  unsigned int InShapePtr = retrieveValue<uint32_t>(Args[5]);
  unsigned int InShapeNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int InShapeDimsPtr = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutReshapedPtr = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutReshapedNDim = retrieveValue<uint32_t>(Args[1]);
  unsigned int OutReshapedDimsPtr = retrieveValue<uint32_t>(Args[0]);

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  void *RuntimeContext =
      reinterpret_cast<void *>(MemInst->getPointer(RuntimeContextPtr));
  int32_t *InDataDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InDataDimsPtr));
  int32_t *InShapeDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InShapeDimsPtr));
  int32_t *OutReshapedDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutReshapedDimsPtr));
  float *InData = reinterpret_cast<float *>(MemInst->getPointer(InDataPtr));
  float *InShape = reinterpret_cast<float *>(MemInst->getPointer(InShapePtr));
  float *OutReshaped =
      reinterpret_cast<float *>(MemInst->getPointer(OutReshapedPtr));

  ONNC_RUNTIME_reshape_float(RuntimeContext, InData, InDataNDim, InDataDims,
                             InShape, InShapeNDim, InShapeDims, OutReshaped,
                             OutReshapedNDim, OutReshapedDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
