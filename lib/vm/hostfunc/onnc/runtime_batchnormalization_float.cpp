#include "vm/hostfunc/onnc/runtime_batchnormalization_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeBatchnormalizationFloat::ONNCRuntimeBatchnormalizationFloat() {
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
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::F32);
  appendParamDef(AST::ValType::F32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeBatchnormalizationFloat::run(
    std::vector<Value> &Args, std::vector<Value> &Res, StoreManager &Store,
    Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_X,
  ///      int32_t input_X_ndim,
  ///      const int32_t *input_X_dims,
  ///      const float *input_scale,
  ///      int32_t input_scale_ndim,
  ///      const int32_t *input_scale_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      const float *input_mean,
  ///      int32_t input_mean_ndim,
  ///      const int32_t *input_mean_dims,
  ///      const float *input_var,
  ///      int32_t input_var_ndim,
  ///      const int32_t *input_var_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float *output_mean,
  ///      int32_t output_mean_ndim,
  ///      const int32_t *output_mean_dims,
  ///      float *output_var,
  ///      int32_t output_var_ndim,
  ///      const int32_t *output_var_dims,
  ///      float *output_saved_mean,
  ///      int32_t output_saved_mean_ndim,
  ///      const int32_t *output_saved_mean_dims,
  ///      float *output_saved_var,
  ///      int32_t output_saved_var_ndim,
  ///      const int32_t *output_saved_var_dims,
  ///      float epsilon,
  ///      float momentum,
  ///      int32_t spatial
  if (Args.size() != 34) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(Args[33]);
  unsigned int InXPtr = retrieveValue<uint32_t>(Args[32]);
  unsigned int InXNDim = retrieveValue<uint32_t>(Args[31]);
  unsigned int InXDimsPtr = retrieveValue<uint32_t>(Args[30]);
  unsigned int InScalePtr = retrieveValue<uint32_t>(Args[29]);
  unsigned int InScaleNDim = retrieveValue<uint32_t>(Args[28]);
  unsigned int InScaleDimsPtr = retrieveValue<uint32_t>(Args[27]);
  unsigned int InBPtr = retrieveValue<uint32_t>(Args[26]);
  unsigned int InBNDim = retrieveValue<uint32_t>(Args[25]);
  unsigned int InBDimsPtr = retrieveValue<uint32_t>(Args[24]);
  unsigned int InMeanPtr = retrieveValue<uint32_t>(Args[23]);
  unsigned int InMeanNDim = retrieveValue<uint32_t>(Args[22]);
  unsigned int InMeanDimsPtr = retrieveValue<uint32_t>(Args[21]);
  unsigned int InVarPtr = retrieveValue<uint32_t>(Args[20]);
  unsigned int InVarNDim = retrieveValue<uint32_t>(Args[19]);
  unsigned int InVarDimsPtr = retrieveValue<uint32_t>(Args[18]);
  unsigned int OutYPtr = retrieveValue<uint32_t>(Args[17]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[16]);
  unsigned int OutYDimsPtr = retrieveValue<uint32_t>(Args[15]);
  unsigned int OutMeanPtr = retrieveValue<uint32_t>(Args[14]);
  unsigned int OutMeanNDim = retrieveValue<uint32_t>(Args[13]);
  unsigned int OutMeanDimsPtr = retrieveValue<uint32_t>(Args[12]);
  unsigned int OutVarPtr = retrieveValue<uint32_t>(Args[11]);
  unsigned int OutVarNDim = retrieveValue<uint32_t>(Args[10]);
  unsigned int OutVarDimsPtr = retrieveValue<uint32_t>(Args[9]);
  unsigned int OutSavedMeanPtr = retrieveValue<uint32_t>(Args[8]);
  unsigned int OutSavedMeanNDim = retrieveValue<uint32_t>(Args[7]);
  unsigned int OutSavedMeanDimsPtr = retrieveValue<uint32_t>(Args[6]);
  unsigned int OutSavedVarPtr = retrieveValue<uint32_t>(Args[5]);
  unsigned int OutSavedVarNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int OutSavedVarDimsPtr = retrieveValue<uint32_t>(Args[3]);
  float Epsilon = retrieveValue<float>(Args[2]);
  float Momentum = retrieveValue<float>(Args[1]);
  unsigned int Spatial = retrieveValue<uint32_t>(Args[0]);

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
  int32_t *InScaleDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InScaleDimsPtr));
  int32_t *InBDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InBDimsPtr));
  int32_t *InMeanDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InMeanDimsPtr));
  int32_t *InVarDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InVarDimsPtr));
  int32_t *OutYDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutYDimsPtr));
  int32_t *OutMeanDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutMeanDimsPtr));
  int32_t *OutVarDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutVarDimsPtr));
  int32_t *OutSavedMeanDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutSavedMeanDimsPtr));
  int32_t *OutSavedVarDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutSavedVarDimsPtr));
  float *InX = reinterpret_cast<float *>(MemInst->getPointer(InXPtr));
  float *InScale = reinterpret_cast<float *>(MemInst->getPointer(InScalePtr));
  float *InB = reinterpret_cast<float *>(MemInst->getPointer(InBPtr));
  float *InMean = reinterpret_cast<float *>(MemInst->getPointer(InMeanPtr));
  float *InVar = reinterpret_cast<float *>(MemInst->getPointer(InVarPtr));
  float *OutY = reinterpret_cast<float *>(MemInst->getPointer(OutYPtr));
  float *OutMean = reinterpret_cast<float *>(MemInst->getPointer(OutMeanPtr));
  float *OutVar = reinterpret_cast<float *>(MemInst->getPointer(OutVarPtr));
  float *OutSavedMean =
      reinterpret_cast<float *>(MemInst->getPointer(OutSavedMeanPtr));
  float *OutSavedVar =
      reinterpret_cast<float *>(MemInst->getPointer(OutSavedVarPtr));

  ONNC_RUNTIME_batchnormalization_float(
      RuntimeContext, InX, InXNDim, InXDims, InScale, InScaleNDim, InScaleDims,
      InB, InBNDim, InBDims, InMean, InMeanNDim, InMeanDims, InVar, InVarNDim,
      InVarDims, OutY, OutYNDim, OutYDims, OutMean, OutMeanNDim, OutMeanDims,
      OutVar, OutVarNDim, OutVarDims, OutSavedMean, OutSavedMeanNDim,
      OutSavedMeanDims, OutSavedVar, OutSavedVarNDim, OutSavedVarDims, Epsilon,
      Momentum, Spatial);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
