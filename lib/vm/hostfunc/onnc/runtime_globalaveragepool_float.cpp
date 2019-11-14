#include "vm/hostfunc/onnc/runtime_globalaveragepool_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeGlobalaveragepoolFloat::ONNCRuntimeGlobalaveragepoolFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeGlobalaveragepoolFloat::run(
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    std::vector<std::unique_ptr<ValueEntry>> &Res, StoreManager &Store,
    Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims
  if (Args.size() != 7) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(*Args[14].get());
  unsigned int InXPtr = retrieveValue<uint32_t>(*Args[13].get());
  unsigned int InXNDim = retrieveValue<uint32_t>(*Args[12].get());
  unsigned int InXDimsPtr = retrieveValue<uint32_t>(*Args[11].get());
  unsigned int OutYPtr = retrieveValue<uint32_t>(*Args[10].get());
  unsigned int OutYNDim = retrieveValue<uint32_t>(*Args[9].get());
  unsigned int OutYDimsPtr = retrieveValue<uint32_t>(*Args[8].get());

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

  ONNC_RUNTIME_globalaveragepool_float(RuntimeContext, InX, InXNDim, InXDims,
                                       OutY, OutYNDim, OutYDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM