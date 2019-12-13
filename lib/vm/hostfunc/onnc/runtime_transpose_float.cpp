#include "vm/hostfunc/onnc/runtime_transpose_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeTransposeFloat::ONNCRuntimeTransposeFloat() {
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

ErrCode ONNCRuntimeTransposeFloat::run(VM::EnvironmentManager &EnvMgr,
                                       std::vector<Value> &Args,
                                       std::vector<Value> &Res,
                                       StoreManager &Store,
                                       Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_data,
  ///      int32_t input_data_ndim,
  ///      const int32_t *input_data_dims,
  ///      float *output_transposed,
  ///      int32_t output_transposed_ndim,
  ///      const int32_t *output_transposed_dims,
  ///      int32_t *perm,
  ///      int32_t number_of_perm
  if (Args.size() != 9) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int InDataOff = retrieveValue<uint32_t>(Args[7]);
  unsigned int InDataNDim = retrieveValue<uint32_t>(Args[6]);
  unsigned int InDataDimsOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int OutTransposedOff = retrieveValue<uint32_t>(Args[4]);
  unsigned int OutTransposedNDim = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutTransposedDimsOff = retrieveValue<uint32_t>(Args[2]);
  unsigned int PermOff = retrieveValue<uint32_t>(Args[1]);
  unsigned int PermNum = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *OutTransposedDims =
      MemInst->getPointer<int32_t *>(OutTransposedDimsOff);
  float *InData = MemInst->getPointer<float *>(InDataOff);
  float *OutTransposed = MemInst->getPointer<float *>(OutTransposedOff);
  int32_t *Perm = MemInst->getPointer<int32_t *>(PermOff);

  ONNC_RUNTIME_transpose_float(RuntimeContext, InData, InDataNDim, InDataDims,
                               OutTransposed, OutTransposedNDim,
                               OutTransposedDims, Perm, PermNum);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
