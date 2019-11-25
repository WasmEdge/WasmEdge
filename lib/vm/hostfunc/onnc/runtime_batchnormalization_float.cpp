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
  /// Optional: output_mean, output_var, output_saved_mean, output_saved_var
  if (Args.size() != 34) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextOff = retrieveValue<uint32_t>(Args[33]);
  unsigned int InXOff = retrieveValue<uint32_t>(Args[32]);
  unsigned int InXNDim = retrieveValue<uint32_t>(Args[31]);
  unsigned int InXDimsOff = retrieveValue<uint32_t>(Args[30]);
  unsigned int InScaleOff = retrieveValue<uint32_t>(Args[29]);
  unsigned int InScaleNDim = retrieveValue<uint32_t>(Args[28]);
  unsigned int InScaleDimsOff = retrieveValue<uint32_t>(Args[27]);
  unsigned int InBOff = retrieveValue<uint32_t>(Args[26]);
  unsigned int InBNDim = retrieveValue<uint32_t>(Args[25]);
  unsigned int InBDimsOff = retrieveValue<uint32_t>(Args[24]);
  unsigned int InMeanOff = retrieveValue<uint32_t>(Args[23]);
  unsigned int InMeanNDim = retrieveValue<uint32_t>(Args[22]);
  unsigned int InMeanDimsOff = retrieveValue<uint32_t>(Args[21]);
  unsigned int InVarOff = retrieveValue<uint32_t>(Args[20]);
  unsigned int InVarNDim = retrieveValue<uint32_t>(Args[19]);
  unsigned int InVarDimsOff = retrieveValue<uint32_t>(Args[18]);
  unsigned int OutYOff = retrieveValue<uint32_t>(Args[17]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[16]);
  unsigned int OutYDimsOff = retrieveValue<uint32_t>(Args[15]);
  unsigned int OutMeanOff = retrieveValue<uint32_t>(Args[14]);
  unsigned int OutMeanNDim = retrieveValue<uint32_t>(Args[13]);
  unsigned int OutMeanDimsOff = retrieveValue<uint32_t>(Args[12]);
  unsigned int OutVarOff = retrieveValue<uint32_t>(Args[11]);
  unsigned int OutVarNDim = retrieveValue<uint32_t>(Args[10]);
  unsigned int OutVarDimsOff = retrieveValue<uint32_t>(Args[9]);
  unsigned int OutSavedMeanOff = retrieveValue<uint32_t>(Args[8]);
  unsigned int OutSavedMeanNDim = retrieveValue<uint32_t>(Args[7]);
  unsigned int OutSavedMeanDimsOff = retrieveValue<uint32_t>(Args[6]);
  unsigned int OutSavedVarOff = retrieveValue<uint32_t>(Args[5]);
  unsigned int OutSavedVarNDim = retrieveValue<uint32_t>(Args[4]);
  unsigned int OutSavedVarDimsOff = retrieveValue<uint32_t>(Args[3]);
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

  void *RuntimeContext = MemInst->getPointer<void *>(RuntimeContextOff);
  int32_t *InXDims = MemInst->getPointer<int32_t *>(InXDimsOff);
  int32_t *InScaleDims = MemInst->getPointer<int32_t *>(InScaleDimsOff);
  int32_t *InBDims = MemInst->getPointer<int32_t *>(InBDimsOff);
  int32_t *InMeanDims = MemInst->getPointer<int32_t *>(InMeanDimsOff);
  int32_t *InVarDims = MemInst->getPointer<int32_t *>(InVarDimsOff);
  int32_t *OutYDims = MemInst->getPointer<int32_t *>(OutYDimsOff);
  int32_t *OutMeanDims = MemInst->getPointerOrNull<int32_t *>(OutMeanDimsOff);
  int32_t *OutVarDims = MemInst->getPointerOrNull<int32_t *>(OutVarDimsOff);
  int32_t *OutSavedMeanDims =
      MemInst->getPointerOrNull<int32_t *>(OutSavedMeanDimsOff);
  int32_t *OutSavedVarDims =
      MemInst->getPointerOrNull<int32_t *>(OutSavedVarDimsOff);
  float *InX = MemInst->getPointer<float *>(InXOff);
  float *InScale = MemInst->getPointer<float *>(InScaleOff);
  float *InB = MemInst->getPointer<float *>(InBOff);
  float *InMean = MemInst->getPointer<float *>(InMeanOff);
  float *InVar = MemInst->getPointer<float *>(InVarOff);
  float *OutY = MemInst->getPointer<float *>(OutYOff);
  float *OutMean = MemInst->getPointerOrNull<float *>(OutMeanOff);
  float *OutVar = MemInst->getPointerOrNull<float *>(OutVarOff);
  float *OutSavedMean = MemInst->getPointerOrNull<float *>(OutSavedMeanOff);
  float *OutSavedVar = MemInst->getPointerOrNull<float *>(OutSavedVarOff);

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
