#include "vm/hostfunc/onnc/runtime_relu_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeReluFloat::ONNCRuntimeReluFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeReluFloat::run(std::vector<Value> &Args,
                                  std::vector<Value> &Res, StoreManager &Store,
                                  Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float* input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t* input_X_dims,
  ///      float* output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t* output_Y_dims
  if (Args.size() != 7) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(Args[6]);
  unsigned int InXPtr = retrieveValue<uint32_t>(Args[5]);
  unsigned int InXNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int InXDimsPtr = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutYPtr = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[1]);
  unsigned int OutYDimsPtr = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *InXDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InXDimsPtr));
  int32_t *OutYDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutYDimsPtr));
  float *InX = reinterpret_cast<float *>(MemInst->getPointer(InXPtr));
  float *OutY = reinterpret_cast<float *>(MemInst->getPointer(OutYPtr));

  ONNC_RUNTIME_relu_float(RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim,
                          OutYDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
