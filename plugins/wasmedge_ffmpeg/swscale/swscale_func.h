#pragma once

#include "runtime/callingframe.h"
#include "swscale_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale {

class SwsGetContext : public WasmEdgeFFmpegSWScale<SwsGetContext> {
public:
  SwsGetContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxPtr,
                       uint32_t SrcW, uint32_t SrcH, uint32_t SrcPixFormatId,
                       uint32_t DesW, uint32_t DesH, uint32_t DesPixFormatId,
                       int32_t Flags, uint32_t SrcFilterId,
                       uint32_t DesFilterId);
};

class SwsFreeContext : public WasmEdgeFFmpegSWScale<SwsFreeContext> {
public:
  SwsFreeContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxId);
};

class SwsScale : public WasmEdgeFFmpegSWScale<SwsScale> {
public:
  SwsScale(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxId,
                       uint32_t InputFrameId, int32_t SrcSliceY,
                       int32_t SrcSliceH, uint32_t OutputFrameId);
};

class SwsGetCachedContext : public WasmEdgeFFmpegSWScale<SwsGetCachedContext> {
public:
  SwsGetCachedContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsCachedCtxPtr, uint32_t SwsCtxPtr,
                       uint32_t SrcW, uint32_t SrcH, uint32_t SrcPixFormatId,
                       uint32_t DesW, uint32_t DesH, uint32_t DesPixFormatId,
                       int32_t Flags, uint32_t SrcFilterId,
                       uint32_t DesFilterId);
};

class SwsIsSupportedInput : public WasmEdgeFFmpegSWScale<SwsIsSupportedInput> {
public:
  SwsIsSupportedInput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class SwsIsSupportedOutput
    : public WasmEdgeFFmpegSWScale<SwsIsSupportedOutput> {
public:
  SwsIsSupportedOutput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class SwsIsSupportedEndiannessConversion
    : public WasmEdgeFFmpegSWScale<SwsIsSupportedEndiannessConversion> {
public:
  SwsIsSupportedEndiannessConversion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class SwsGetDefaultFilter : public WasmEdgeFFmpegSWScale<SwsGetDefaultFilter> {
public:
  SwsGetDefaultFilter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsFilterPtr, float LumaGBlur,
                       float ChromaGBlur, float LumaSharpen,
                       float ChromaSharpen, float ChromaHShift,
                       float ChromaVShift, int32_t Verbose);
};

class SwsGetLumaH : public WasmEdgeFFmpegSWScale<SwsGetLumaH> {
public:
  SwsGetLumaH(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsGetLumaV : public WasmEdgeFFmpegSWScale<SwsGetLumaV> {
public:
  SwsGetLumaV(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsGetChromaH : public WasmEdgeFFmpegSWScale<SwsGetChromaH> {
public:
  SwsGetChromaH(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsGetChromaV : public WasmEdgeFFmpegSWScale<SwsGetChromaV> {
public:
  SwsGetChromaV(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsFilterId,
                       uint32_t SwsVectorPtr);
};

class SwsFreeFilter : public WasmEdgeFFmpegSWScale<SwsFreeFilter> {
public:
  SwsFreeFilter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsFilterId);
};

class SwsAllocVec : public WasmEdgeFFmpegSWScale<SwsAllocVec> {
public:
  SwsAllocVec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsVectorPtr, int32_t Length);
};

class SwsGetGaussianVec : public WasmEdgeFFmpegSWScale<SwsGetGaussianVec> {
public:
  SwsGetGaussianVec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsVectorPtr, double Variance, double Quality);
};

class SwsScaleVec : public WasmEdgeFFmpegSWScale<SwsScaleVec> {
public:
  SwsScaleVec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsVectorId,
                       double Scalar);
};

class SwsNormalizeVec : public WasmEdgeFFmpegSWScale<SwsNormalizeVec> {
public:
  SwsNormalizeVec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwsVectorId,
                       double Height);
};

class SwsGetCoeffVecLength
    : public WasmEdgeFFmpegSWScale<SwsGetCoeffVecLength> {
public:
  SwsGetCoeffVecLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t SwsVectorId);
};

class SwsGetCoeff : public WasmEdgeFFmpegSWScale<SwsGetCoeff> {
public:
  SwsGetCoeff(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t SwsVectorId,
                       uint32_t CoeffBuf, uint32_t Len);
};

class SwsFreeVec : public WasmEdgeFFmpegSWScale<SwsFreeVec> {
public:
  SwsFreeVec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SwsVectorId);
};

class SwscaleVersion : public WasmEdgeFFmpegSWScale<SwscaleVersion> {
public:
  SwscaleVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class SwscaleConfigurationLength
    : public WasmEdgeFFmpegSWScale<SwscaleConfigurationLength> {
public:
  SwscaleConfigurationLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SwscaleConfiguration
    : public WasmEdgeFFmpegSWScale<SwscaleConfiguration> {
public:
  SwscaleConfiguration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class SwscaleLicenseLength
    : public WasmEdgeFFmpegSWScale<SwscaleLicenseLength> {
public:
  SwscaleLicenseLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SwscaleLicense : public WasmEdgeFFmpegSWScale<SwscaleLicense> {
public:
  SwscaleLicense(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
