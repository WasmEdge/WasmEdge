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

ErrCode
ONNCRuntimeTransposeFloat::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                               std::vector<std::unique_ptr<ValueEntry>> &Res,
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
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(*Args[8].get());
  unsigned int InDataPtr = retrieveValue<uint32_t>(*Args[7].get());
  unsigned int InDataNDim = retrieveValue<uint32_t>(*Args[6].get());
  unsigned int InDataDimsPtr = retrieveValue<uint32_t>(*Args[5].get());
  unsigned int OutTransposedPtr = retrieveValue<uint32_t>(*Args[4].get());
  unsigned int OutTransposedNDim = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int OutTransposedDimsPtr = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int PermPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int PermNum = retrieveValue<uint32_t>(*Args[0].get());

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
  int32_t *OutTransposedDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutTransposedDimsPtr));
  float *InData = reinterpret_cast<float *>(MemInst->getPointer(InDataPtr));
  float *OutTransposed =
      reinterpret_cast<float *>(MemInst->getPointer(OutTransposedPtr));
  int32_t *Perm = reinterpret_cast<int32_t *>(MemInst->getPointer(PermPtr));

  ONNC_RUNTIME_transpose_float(RuntimeContext, InData, InDataNDim, InDataDims,
                               OutTransposed, OutTransposedNDim,
                               OutTransposedDims, Perm, PermNum);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM