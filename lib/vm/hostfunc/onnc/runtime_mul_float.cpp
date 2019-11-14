#include "vm/hostfunc/onnc/runtime_mul_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeMulFloat::ONNCRuntimeMulFloat() {
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

ErrCode ONNCRuntimeMulFloat::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                                 std::vector<std::unique_ptr<ValueEntry>> &Res,
                                 StoreManager &Store,
                                 Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      float *output_C,
  ///      int32_t output_C_ndim,
  ///      const int32_t *output_C_dims
  if (Args.size() != 10) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(*Args[9].get());
  unsigned int InAPtr = retrieveValue<uint32_t>(*Args[8].get());
  unsigned int InANDim = retrieveValue<uint32_t>(*Args[7].get());
  unsigned int InADimsPtr = retrieveValue<uint32_t>(*Args[6].get());
  unsigned int InBPtr = retrieveValue<uint32_t>(*Args[5].get());
  unsigned int InBNDim = retrieveValue<uint32_t>(*Args[4].get());
  unsigned int InBDimsPtr = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int OutCPtr = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int OutCNDim = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int OutCDimsPtr = retrieveValue<uint32_t>(*Args[0].get());

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
  int32_t *InADims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InADimsPtr));
  int32_t *InBDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InBDimsPtr));
  int32_t *OutCDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutCDimsPtr));
  float *InA = reinterpret_cast<float *>(MemInst->getPointer(InAPtr));
  float *InB = reinterpret_cast<float *>(MemInst->getPointer(InBPtr));
  float *OutC = reinterpret_cast<float *>(MemInst->getPointer(OutCPtr));

  ONNC_RUNTIME_mul_float(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                         InBDims, OutC, OutCNDim, OutCDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM