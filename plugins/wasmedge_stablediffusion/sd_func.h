#pragma once

#include "runtime/callingframe.h"
#include "sd_base.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
class SDCreateContext : public StableDiffusion::Func<SDCreateContext> {
public:
  SDCreateContext(StableDiffusion::SDEnviornment &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t>
  body(const Runtime::CallingFrame &Frame, uint32_t ModelPathPtr,
       uint32_t ModelPathLen, uint32_t VaePathPtr, uint32_t VaePathLen,
       uint32_t TaesdPathPtr, uint32_t TaesdPathLen, uint32_t ControlNetPathPtr,
       uint32_t ControlNetPathLen, uint32_t LoraModelDirPtr,
       uint32_t LoraModelDirLen, uint32_t EmbedDirPtr, uint32_t EmbedDirLen,
       uint32_t IdEmbedDirPtr, uint32_t IdEmbedDirLen, uint32_t VaeDecodeOnly,
       uint32_t VaeTiling, int32_t NThreads, uint32_t Wtype, uint32_t RngType,
       uint32_t Schedule, uint32_t ClipOnCpu, uint32_t ControlNetCpu,
       uint32_t VaeOnCpu, uint32_t SessiontIdPtr);
};
class SDTextToImage : public StableDiffusion::Func<SDTextToImage> {
public:
  SDTextToImage(StableDiffusion::SDEnviornment &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t>
  body(const Runtime::CallingFrame &Frame, uint32_t ImagePtr, uint32_t ImageLen,
       uint32_t SessionId, uint32_t Width, uint32_t Height,
       uint32_t ControlImagePtr, uint32_t ControlImageLen, uint32_t PromptPtr,
       uint32_t PromptLen, uint32_t NegativePromptPtr,
       uint32_t NegativePromptLen, uint32_t ClipSkip, uint32_t CfgScale,
       uint32_t SampleMethod, uint32_t SampleSteps, uint32_t Strength,
       uint32_t Seed, uint32_t BatchCount, uint32_t ControlStrength,
       uint32_t StyleRatio, uint32_t NormalizeInput,
       uint32_t InputIdImagesPathPtr, uint32_t InputIdImagesPathLen,
       uint32_t canny_preprocess, uint32_t OutBufferPtr,
       uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr);
};
class SDConvert : public StableDiffusion::Func<SDConvert> {
public:
  SDConvert(StableDiffusion::SDEnviornment &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t ModelPathPtr, uint32_t ModelPathLen,
                        uint32_t VaeModelPathPtr, uint32_t VaeModelPathLen,
                        uint32_t OutputPathPtr, uint32_t OutputPathLen,
                        uint32_t WType);
};
} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge