#include "vm/hostfunc/onnc/runtime_maxpool_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeMaxpoolFloat::ONNCRuntimeMaxpoolFloat() {
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
ONNCRuntimeMaxpoolFloat::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                             std::vector<std::unique_ptr<ValueEntry>> &Res,
                             StoreManager &Store,
                             Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float *output_Indices,
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
  if (Args.size() != 18) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(*Args[17].get());
  unsigned int InXPtr = retrieveValue<uint32_t>(*Args[16].get());
  unsigned int InXNDim = retrieveValue<uint32_t>(*Args[15].get());
  unsigned int InXDimsPtr = retrieveValue<uint32_t>(*Args[14].get());
  unsigned int OutYPtr = retrieveValue<uint32_t>(*Args[13].get());
  unsigned int OutYNDim = retrieveValue<uint32_t>(*Args[12].get());
  unsigned int OutYDimsPtr = retrieveValue<uint32_t>(*Args[11].get());
  unsigned int OutIndicesPtr = retrieveValue<uint32_t>(*Args[10].get());
  unsigned int OutIndicesNDim = retrieveValue<uint32_t>(*Args[9].get());
  unsigned int OutIndicesDimsPtr = retrieveValue<uint32_t>(*Args[8].get());
  unsigned int AutoPadPtr = retrieveValue<uint32_t>(*Args[7].get());
  unsigned int KernelShapePtr = retrieveValue<uint32_t>(*Args[6].get());
  unsigned int KernelShapeNum = retrieveValue<uint32_t>(*Args[5].get());
  unsigned int PadsPtr = retrieveValue<uint32_t>(*Args[4].get());
  unsigned int PadsNum = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int StorageOrder = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int StridesPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int StridesNum = retrieveValue<uint32_t>(*Args[0].get());

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
  int32_t *OutIndicesDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutIndicesDimsPtr));
  float *InX = reinterpret_cast<float *>(MemInst->getPointer(InXPtr));
  float *OutY = reinterpret_cast<float *>(MemInst->getPointer(OutYPtr));
  float *OutIndices =
      reinterpret_cast<float *>(MemInst->getPointer(OutIndicesPtr));
  char *AutoPad = reinterpret_cast<char *>(MemInst->getPointer(AutoPadPtr));
  int32_t *KernelShape =
      reinterpret_cast<int32_t *>(MemInst->getPointer(KernelShapePtr));
  int32_t *Pads = reinterpret_cast<int32_t *>(MemInst->getPointer(PadsPtr));
  int32_t *Strides =
      reinterpret_cast<int32_t *>(MemInst->getPointer(StridesPtr));

  ONNC_RUNTIME_maxpool_float(
      RuntimeContext, InX, InXNDim, InXDims, OutY, OutYNDim, OutYDims,
      OutIndices, OutIndicesNDim, OutIndicesDims, AutoPad, KernelShape,
      KernelShapeNum, Pads, PadsNum, StorageOrder, Strides, StridesNum);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM