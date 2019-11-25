#include "vm/hostfunc/onnc/runtime_conv_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeConvFloat::ONNCRuntimeConvFloat() {
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
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeConvFloat::run(std::vector<Value> &Args,
                                  std::vector<Value> &Res, StoreManager &Store,
                                  Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float* input_X,
  ///      int32_t input_X_NDim,
  ///      const int32_t* input_X_dims,
  ///      const float* input_W,
  ///      int32_t input_W_NDim,
  ///      const int32_t* input_W_dims,
  ///      const float* input_B,
  ///      int32_t input_B_NDim,
  ///      const int32_t* input_B_dims,
  ///      float* output_Y,
  ///      int32_t output_Y_NDim,
  ///      const int32_t* output_Y_dims,
  ///      const char* auto_pad,
  ///      int32_t* dilations,
  ///      int32_t number_of_dilations,
  ///      int32_t group,
  ///      int32_t* kernel_shape,
  ///      int32_t number_of_kernel_shape,
  ///      int32_t* pads,
  ///      int32_t number_of_pads,
  ///      int32_t* strides,
  ///      int32_t number_of_strides
  /// Optional: input_B
  if (Args.size() != 23) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[22]);
  unsigned int InXOff = retrieveValue<uint32_t>(Args[21]);
  unsigned int InXNDim = retrieveValue<uint32_t>(Args[20]);
  unsigned int InXDimsOff = retrieveValue<uint32_t>(Args[19]);
  unsigned int InWOff = retrieveValue<uint32_t>(Args[18]);
  unsigned int InWNDim = retrieveValue<uint32_t>(Args[17]);
  unsigned int InWDimsOff = retrieveValue<uint32_t>(Args[16]);
  unsigned int InBOff = retrieveValue<uint32_t>(Args[15]);
  unsigned int InBNDim = retrieveValue<uint32_t>(Args[14]);
  unsigned int InBDimsOff = retrieveValue<uint32_t>(Args[13]);
  unsigned int OutYOff = retrieveValue<uint32_t>(Args[12]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[11]);
  unsigned int OutYDimsOff = retrieveValue<uint32_t>(Args[10]);
  unsigned int AutoPadOff = retrieveValue<uint32_t>(Args[9]);
  unsigned int DelationsOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int DelationNum = retrieveValue<uint32_t>(Args[7]);
  unsigned int Group = retrieveValue<uint32_t>(Args[6]);
  unsigned int KernelShapeOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int KernelShapeNum = retrieveValue<uint32_t>(Args[4]);
  unsigned int PadsOff = retrieveValue<uint32_t>(Args[3]);
  unsigned int PadsNum = retrieveValue<uint32_t>(Args[2]);
  unsigned int StridesOff = retrieveValue<uint32_t>(Args[1]);
  unsigned int StridesNum = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *InXDims = MemInst->getPointer<int32_t *>(InXDimsOff);
  int32_t *InWDims = MemInst->getPointer<int32_t *>(InWDimsOff);
  int32_t *InBDims = MemInst->getPointerOrNull<int32_t *>(InBDimsOff);
  int32_t *OutYDims = MemInst->getPointer<int32_t *>(OutYDimsOff);
  float *InX = MemInst->getPointer<float *>(InXOff);
  float *InW = MemInst->getPointer<float *>(InWOff);
  float *InB = MemInst->getPointerOrNull<float *>(InBOff);
  float *OutY = MemInst->getPointer<float *>(OutYOff);
  char *AutoPad = MemInst->getPointer<char *>(AutoPadOff);
  int32_t *Delations = MemInst->getPointer<int32_t *>(DelationsOff);
  int32_t *KernelShape = MemInst->getPointer<int32_t *>(KernelShapeOff);
  int32_t *Pads = MemInst->getPointer<int32_t *>(PadsOff);
  int32_t *Strides = MemInst->getPointer<int32_t *>(StridesOff);

  ONNC_RUNTIME_conv_float(RuntimeContext, InX, InXNDim, InXDims, InW, InWNDim,
                          InWDims, InB, InBNDim, InBDims, OutY, OutYNDim,
                          OutYDims, AutoPad, Delations, DelationNum, Group,
                          KernelShape, KernelShapeNum, Pads, PadsNum, Strides,
                          StridesNum);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
