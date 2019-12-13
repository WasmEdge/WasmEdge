// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/onnc/runtime_add_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeAddFloat::ONNCRuntimeAddFloat() {
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

ErrCode ONNCRuntimeAddFloat::run(VM::EnvironmentManager &EnvMgr,
                                 std::vector<Value> &Args,
                                 std::vector<Value> &Res, StoreManager &Store,
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
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[9]);
  unsigned int InAOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int InANDim = retrieveValue<uint32_t>(Args[7]);
  unsigned int InADimsOff = retrieveValue<uint32_t>(Args[6]);
  unsigned int InBOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int InBNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int InBDimsOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int OutCOff = retrieveValue<uint32_t>(Args[2]);
  unsigned int OutCNDim = retrieveValue<uint32_t>(Args[1]);
  unsigned int OutCDimsOff = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *InADims = MemInst->getPointer<int32_t *>(InADimsOff);
  int32_t *InBDims = MemInst->getPointer<int32_t *>(InBDimsOff);
  int32_t *OutCDims = MemInst->getPointer<int32_t *>(OutCDimsOff);
  float *InA = MemInst->getPointer<float *>(InAOff);
  float *InB = MemInst->getPointer<float *>(InBOff);
  float *OutC = MemInst->getPointer<float *>(OutCOff);

  ONNC_RUNTIME_add_float(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                         InBDims, OutC, OutCNDim, OutCDims);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
