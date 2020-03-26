// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace SSVM {
namespace Host {

class ONNCRuntimeAddFloat : public Runtime::HostFunction<ONNCRuntimeAddFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InAOff, uint32_t InANDim,
               uint32_t InADimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t OutCOff, uint32_t OutCNDim,
               uint32_t OutCDimsOff);
};

class ONNCRuntimeAddInt8 : public Runtime::HostFunction<ONNCRuntimeAddInt8> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InAOff, uint32_t InANDim,
               uint32_t InADimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t OutCOff, uint32_t OutCNDim,
               uint32_t OutCDimsOff);
};

class ONNCRuntimeAveragepoolFloat
    : public Runtime::HostFunction<ONNCRuntimeAveragepoolFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t AutoPadOff,
               uint32_t IncludePadCnt, uint32_t KernelShapeOff,
               uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
               uint32_t StridesOff, uint32_t StridesNum);
};

class ONNCRuntimeBatchnormalizationFloat
    : public Runtime::HostFunction<ONNCRuntimeBatchnormalizationFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t InScaleOff, uint32_t InScaleNDim,
               uint32_t InScaleDimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t InMeanOff, uint32_t InMeanNDim,
               uint32_t InMeanDimsOff, uint32_t InVarOff, uint32_t InVarNDim,
               uint32_t InVarDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t OutMeanOff, uint32_t OutMeanNDim,
               uint32_t OutMeanDimsOff, uint32_t OutVarOff, uint32_t OutVarNDim,
               uint32_t OutVarDimsOff, uint32_t OutSavedMeanOff,
               uint32_t OutSavedMeanNDim, uint32_t OutSavedMeanDimsOff,
               uint32_t OutSavedVarOff, uint32_t OutSavedVarNDim,
               uint32_t OutSavedVarDimsOff, float Epsilon, float Momentum,
               uint32_t Spatial);
};

class ONNCRuntimeBatchnormalizationInt8
    : public Runtime::HostFunction<ONNCRuntimeBatchnormalizationInt8> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t InScaleOff, uint32_t InScaleNDim,
               uint32_t InScaleDimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t InMeanOff, uint32_t InMeanNDim,
               uint32_t InMeanDimsOff, uint32_t InVarOff, uint32_t InVarNDim,
               uint32_t InVarDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t OutMeanOff, uint32_t OutMeanNDim,
               uint32_t OutMeanDimsOff, uint32_t OutVarOff, uint32_t OutVarNDim,
               uint32_t OutVarDimsOff, uint32_t OutSavedMeanOff,
               uint32_t OutSavedMeanNDim, uint32_t OutSavedMeanDimsOff,
               uint32_t OutSavedVarOff, uint32_t OutSavedVarNDim,
               uint32_t OutSavedVarDimsOff, int32_t Epsilon, int32_t Momentum,
               uint32_t Spatial);
};

class ONNCRuntimeConcatFloat
    : public Runtime::HostFunction<ONNCRuntimeConcatFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InInputsOffOff,
               uint32_t InInputsNTensor, uint32_t InInputsNDimOff,
               uint32_t InInputsDimsOffOff, uint32_t OutConcatResultOff,
               uint32_t OutConcatResultNDim, uint32_t OutConcatResultDimsOff,
               uint32_t Axis);
};

class ONNCRuntimeConvFloat
    : public Runtime::HostFunction<ONNCRuntimeConvFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t InWOff, uint32_t InWNDim,
               uint32_t InWDimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t AutoPadOff, uint32_t DelationsOff,
               uint32_t DelationNum, uint32_t Group, uint32_t KernelShapeOff,
               uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
               uint32_t StridesOff, uint32_t StridesNum);
};

class ONNCRuntimeConvInt8 : public Runtime::HostFunction<ONNCRuntimeConvInt8> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t InWOff, uint32_t InWNDim,
               uint32_t InWDimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t AutoPadOff, uint32_t DelationsOff,
               uint32_t DelationNum, uint32_t Group, uint32_t KernelShapeOff,
               uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
               uint32_t StridesOff, uint32_t StridesNum);
};

class ONNCRuntimeGemmFloat
    : public Runtime::HostFunction<ONNCRuntimeGemmFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InAOff, uint32_t InANDim,
               uint32_t InADimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t InCOff, uint32_t InCNDim,
               uint32_t InCDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, float Alpha, float Beta, uint32_t TransA,
               uint32_t TransB);
};

class ONNCRuntimeGlobalaveragepoolFloat
    : public Runtime::HostFunction<ONNCRuntimeGlobalaveragepoolFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff);
};

class ONNCRuntimeLrnFloat : public Runtime::HostFunction<ONNCRuntimeLrnFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, float Alpha, float Beta, float Bias,
               uint32_t Size);
};

class ONNCRuntimeMaxpoolFloat
    : public Runtime::HostFunction<ONNCRuntimeMaxpoolFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t OutIndicesOff,
               uint32_t OutIndicesNDim, uint32_t OutIndicesDimsOff,
               uint32_t AutoPadOff, uint32_t KernelShapeOff,
               uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
               uint32_t StorageOrder, uint32_t StridesOff, uint32_t StridesNum);
};

class ONNCRuntimeMaxpoolInt8
    : public Runtime::HostFunction<ONNCRuntimeMaxpoolInt8> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t OutIndicesOff,
               uint32_t OutIndicesNDim, uint32_t OutIndicesDimsOff,
               uint32_t AutoPadOff, uint32_t KernelShapeOff,
               uint32_t KernelShapeNum, uint32_t PadsOff, uint32_t PadsNum,
               uint32_t StorageOrder, uint32_t StridesOff, uint32_t StridesNum);
};

class ONNCRuntimeMulFloat : public Runtime::HostFunction<ONNCRuntimeMulFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InAOff, uint32_t InANDim,
               uint32_t InADimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t OutCOff, uint32_t OutCNDim,
               uint32_t OutCDimsOff);
};

class ONNCRuntimeMulInt8 : public Runtime::HostFunction<ONNCRuntimeMulInt8> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InAOff, uint32_t InANDim,
               uint32_t InADimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t OutCOff, uint32_t OutCNDim,
               uint32_t OutCDimsOff);
};

class ONNCRuntimeReluFloat
    : public Runtime::HostFunction<ONNCRuntimeReluFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff);
};

class ONNCRuntimeReluInt8 : public Runtime::HostFunction<ONNCRuntimeReluInt8> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InXOff, uint32_t InXNDim,
               uint32_t InXDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff);
};

class ONNCRuntimeReshapeFloat
    : public Runtime::HostFunction<ONNCRuntimeReshapeFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InDataOff,
               uint32_t InDataNDim, uint32_t InDataDimsOff, uint32_t InShapeOff,
               uint32_t InShapeNDim, uint32_t InShapeDimsOff,
               uint32_t OutReshapedOff, uint32_t OutReshapedNDim,
               uint32_t OutReshapedDimsOff);
};

class ONNCRuntimeSoftmaxFloat
    : public Runtime::HostFunction<ONNCRuntimeSoftmaxFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InOff, uint32_t InNDim,
               uint32_t InDimsOff, uint32_t OutOff, uint32_t OutNDim,
               uint32_t OutDimsOff, uint32_t Axis);
};

class ONNCRuntimeSumFloat : public Runtime::HostFunction<ONNCRuntimeSumFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InDataOffOff,
               uint32_t InDataNTensor, uint32_t InDataNDimOff,
               uint32_t InDataDimsOffOff, uint32_t OutSumOff,
               uint32_t OutSumNDim, uint32_t OutSumDimsOff);
};

class ONNCRuntimeTransposeFloat
    : public Runtime::HostFunction<ONNCRuntimeTransposeFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InDataOff,
               uint32_t InDataNDim, uint32_t InDataDimsOff,
               uint32_t OutTransposedOff, uint32_t OutTransposedNDim,
               uint32_t OutTransposedDimsOff, uint32_t PermOff,
               uint32_t PermNum);
};

class ONNCRuntimeUnsqueezeFloat
    : public Runtime::HostFunction<ONNCRuntimeUnsqueezeFloat> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t RuntimeContextOff, uint32_t InDataOff,
               uint32_t InDataNDim, uint32_t InDataDimsOff,
               uint32_t OutExpandedOff, uint32_t OutExpandedNDim,
               uint32_t OutExpandedDimsOff, uint32_t AxesOff, uint32_t AxesNum);
};

} // namespace Host
} // namespace SSVM