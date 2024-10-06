// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale {

class SwsGetContext : public HostFunction<SwsGetContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxPtr,
                       uint32_t SrcW, uint32_t SrcH, uint32_t SrcPixFormatId,
                       uint32_t DesW, uint32_t DesH, uint32_t DesPixFormatId,
                       int32_t Flags, uint32_t SrcFilterId,
                       uint32_t DesFilterId);
};

class SwsFreeContext : public HostFunction<SwsFreeContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxId);
};

class SwsScale : public HostFunction<SwsScale> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxId,
                       uint32_t InputFrameId, int32_t SrcSliceY,
                       int32_t SrcSliceH, uint32_t OutputFrameId);
};

class SwsGetCachedContext : public HostFunction<SwsGetCachedContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsCachedCtxPtr, uint32_t SwsCtxPtr,
                       uint32_t SrcW, uint32_t SrcH, uint32_t SrcPixFormatId,
                       uint32_t DesW, uint32_t DesH, uint32_t DesPixFormatId,
                       int32_t Flags, uint32_t SrcFilterId,
                       uint32_t DesFilterId);
};

class SwsIsSupportedInput : public HostFunction<SwsIsSupportedInput> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class SwsIsSupportedOutput : public HostFunction<SwsIsSupportedOutput> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class SwsIsSupportedEndiannessConversion
    : public HostFunction<SwsIsSupportedEndiannessConversion> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class SwsGetDefaultFilter : public HostFunction<SwsGetDefaultFilter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsFilterPtr, float LumaGBlur,
                       float ChromaGBlur, float LumaSharpen,
                       float ChromaSharpen, float ChromaHShift,
                       float ChromaVShift, int32_t Verbose);
};

class SwsGetLumaH : public HostFunction<SwsGetLumaH> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsGetLumaV : public HostFunction<SwsGetLumaV> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsGetChromaH : public HostFunction<SwsGetChromaH> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsGetChromaV : public HostFunction<SwsGetChromaV> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsFreeFilter : public HostFunction<SwsFreeFilter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsFilterId);
};

class SwsAllocVec : public HostFunction<SwsAllocVec> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsVectorPtr, int32_t Length);
};

class SwsGetGaussianVec : public HostFunction<SwsGetGaussianVec> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsVectorPtr, double Variance, double Quality);
};

class SwsScaleVec : public HostFunction<SwsScaleVec> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsVectorId,
                       double Scalar);
};

class SwsNormalizeVec : public HostFunction<SwsNormalizeVec> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsVectorId,
                       double Height);
};

class SwsGetCoeffVecLength : public HostFunction<SwsGetCoeffVecLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t SwsVectorId);
};

class SwsGetCoeff : public HostFunction<SwsGetCoeff> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t SwsVectorId,
                       uint32_t CoeffBuf, uint32_t Len);
};

class SwsFreeVec : public HostFunction<SwsFreeVec> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsVectorId);
};

class SwscaleVersion : public HostFunction<SwscaleVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class SwscaleConfigurationLength
    : public HostFunction<SwscaleConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SwscaleConfiguration : public HostFunction<SwscaleConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class SwscaleLicenseLength : public HostFunction<SwscaleLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SwscaleLicense : public HostFunction<SwscaleLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
