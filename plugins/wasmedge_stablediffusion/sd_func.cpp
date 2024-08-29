// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "sd_func.h"
#include "common/spdlog.h"
#include "sd_env.h"
#include "spdlog/spdlog.h"
#include "stable-diffusion.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-StableDiffusion] Memory instance not found."sv);  \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define SESSION_CHECK(Out, SessionID, Message, ErrNo)                          \
  auto *Out = Env.getContext(SessionID);                                       \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-StableDiffusion] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo);                                       \
  }

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-StableDiffusion] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_SV_CHECK(OutSV, MemInst, BufPtr, BufLen, Message)                  \
  auto OutSV = MemInst->getStringView(BufPtr, BufLen);                         \
  if (unlikely(OutSV.size() != BufLen)) {                                      \
    spdlog::error("[WasmEdge-StableDiffusion] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message)                  \
  Type *OutPtr = MemInst->getPointer<Type *>(Offset);                          \
  if (unlikely(OutPtr == nullptr)) {                                           \
    spdlog::error("[WasmEdge-StableDiffusion] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

bool parameterCheck(SDEnviornment &Env, uint32_t Width, uint32_t Height,
                    uint32_t SessionId) {
  if (SessionId >= Env.getContextSize()) {
    spdlog::error("[WasmEdge-StableDiffusion] Session ID is invalid.");
    return false;
  }
  if (Width % 64 != 0) {
    spdlog::error("[WasmEdge-StableDiffusion] Width must be a multiple of 64 "
                  "and greater than 0.");
    return false;
  }
  if (Height % 64 != 0) {
    spdlog::error("[WasmEdge-StableDiffusion] Height must be a multiple of 64 "
                  "and greater than 0.");
    return false;
  }
  return true;
}

sd_image_t *readControlImage(Span<uint8_t> ControlImage,
                             uint8_t *ControlImageBuf, int Width, int Height,
                             bool CannyPreprocess) {
  sd_image_t *ControlImg = nullptr;
  int Channel = 0;
  std::string ControlImagePath(ControlImage.begin(), ControlImage.end());

  if (ControlImagePath.substr(0, 5) == "path:"sv) {
    ControlImageBuf = stbi_load(ControlImagePath.substr(5).data(), &Width,
                                &Height, &Channel, 3);
  } else {
    ControlImageBuf = stbi_load_from_memory(
        ControlImage.data(), ControlImage.size(), &Width, &Height, &Channel, 3);
  }

  if (ControlImageBuf == nullptr) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Load image from control image failed."sv);
    return nullptr;
  }
  ControlImg =
      new sd_image_t{static_cast<uint32_t>(Width),
                     static_cast<uint32_t>(Height), 3, ControlImageBuf};
  if (CannyPreprocess) { // apply preprocessor
    ControlImg->data =
        preprocess_canny(ControlImg->data, ControlImg->width,
                         ControlImg->height, 0.08f, 0.08f, 0.8f, 1.0f, false);
  }
  return ControlImg;
}

Expect<uint32_t> SDConvert::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ModelPathPtr, uint32_t ModelPathLen,
                                 uint32_t VaeModelPathPtr,
                                 uint32_t VaeModelPathLen,
                                 uint32_t OutputPathPtr, uint32_t OutputPathLen,
                                 uint32_t WType) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input parameter value.
  MEM_SPAN_CHECK(ModelPathSpan, MemInst, char, ModelPathPtr, ModelPathLen,
                 "Failed when accessing the input model path memory."sv)
  MEM_SPAN_CHECK(VaeModelPathSpan, MemInst, char, VaeModelPathPtr,
                 VaeModelPathLen,
                 "Failed when accessing the input vae model path memory."sv)
  MEM_SPAN_CHECK(OutputPathSpan, MemInst, char, OutputPathPtr, OutputPathLen,
                 "Failed when accessing the output path memory."sv)
  std::string ModelPath = std::string(
      ModelPathSpan.begin(), ModelPathSpan.begin() + ModelPathSpan.size());
  std::string VaeModelPath =
      std::string(VaeModelPathSpan.begin(),
                  VaeModelPathSpan.begin() + VaeModelPathSpan.size());
  std::string OutputPath = std::string(
      OutputPathSpan.begin(), OutputPathSpan.begin() + OutputPathSpan.size());

  spdlog::info("[WasmEdge-StableDiffusion] Convert model: {} to {}."sv,
               ModelPath.data(), OutputPath.data());
  std::ifstream Fin(ModelPath.data(), std::ios::in | std::ios::binary);
  if (!Fin) {
    Fin.close();
    spdlog::error("[WasmEdge-StableDiffusion] Model not found.");
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  Fin.close();
  // Convert model.
  bool Ret = ::convert(ModelPath.data(), VaeModelPath.data(), OutputPath.data(),
                       static_cast<sd_type_t>(WType));
  if (!Ret) {
    spdlog::error("[WasmEdge-StableDiffusion] Failed to convert model.");
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> SDCreateContext::body(
    const Runtime::CallingFrame &Frame, uint32_t ModelPathPtr,
    uint32_t ModelPathLen, uint32_t clipLPathPtr, uint32_t clipLPathLen,
    uint32_t t5xxlPathPtr, uint32_t t5xxlPathLen,
    uint32_t diffusionModelPathPtr, uint32_t diffusionModelPathLen,
    uint32_t VaePathPtr, uint32_t VaePathLen, uint32_t TaesdPathPtr,
    uint32_t TaesdPathLen, uint32_t ControlNetPathPtr,
    uint32_t ControlNetPathLen, uint32_t LoraModelDirPtr,
    uint32_t LoraModelDirLen, uint32_t EmbedDirPtr, uint32_t EmbedDirLen,
    uint32_t IdEmbedDirPtr, uint32_t IdEmbedDirLen, uint32_t VaeDecodeOnly,
    uint32_t VaeTiling, int32_t NThreads, uint32_t Wtype, uint32_t RngType,
    uint32_t Schedule, uint32_t ClipOnCpu, uint32_t ControlNetCpu,
    uint32_t VaeOnCpu, uint32_t SessiontIdPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input model buffer.
  MEM_SPAN_CHECK(ModelPathSpan, MemInst, char, ModelPathPtr, ModelPathLen,
                 "Failed when accessing the input model path memory."sv)
  MEM_SPAN_CHECK(clipLPathSpan, MemInst, char, clipLPathPtr, clipLPathLen,
                 "Failed when accessing the input clipL path memory."sv)
  MEM_SPAN_CHECK(t5xxlPathSpan, MemInst, char, t5xxlPathPtr, t5xxlPathLen,
                 "Failed when accessing the input t5xxl path memory."sv)
  MEM_SPAN_CHECK(
      diffusionModelPathSpan, MemInst, char, diffusionModelPathPtr,
      diffusionModelPathLen,
      "Failed when accessing the input diffusion model path memory."sv)
  MEM_SPAN_CHECK(VaePathSpan, MemInst, char, VaePathPtr, VaePathLen,
                 "Failed when accessing the input vae path memory."sv)
  MEM_SPAN_CHECK(ControlNetPathSpan, MemInst, char, ControlNetPathPtr,
                 ControlNetPathLen,
                 "Failed when accessing the input control net path memory."sv)
  MEM_SPAN_CHECK(LoraModelDirSpan, MemInst, char, LoraModelDirPtr,
                 LoraModelDirLen,
                 "Failed when accessing the input lora model path memory."sv)
  MEM_SPAN_CHECK(TaesdPathSpan, MemInst, char, TaesdPathPtr, TaesdPathLen,
                 "Failed when accessing the input taesd path memory."sv)
  MEM_SPAN_CHECK(EmbedDirSpan, MemInst, char, EmbedDirPtr, EmbedDirLen,
                 "Failed when accessing the input embedded directory memory."sv)
  MEM_SPAN_CHECK(
      IdEmbedDirSpan, MemInst, char, IdEmbedDirPtr, IdEmbedDirLen,
      "Failed when accessing the input id dembed directory memory."sv)
  MEM_PTR_CHECK(SessionId, MemInst, uint32_t, SessiontIdPtr,
                "Failed when accessing the return SessionID memory."sv)

  std::string ModelPath =
      std::string(ModelPathSpan.begin(), ModelPathSpan.end());
  std::string VaePath = std::string(VaePathSpan.begin(), VaePathSpan.end());
  std::string TaesdPath =
      std::string(TaesdPathSpan.begin(), TaesdPathSpan.end());
  std::string ControlNetPath =
      std::string(ControlNetPathSpan.begin(), ControlNetPathSpan.end());
  std::string LoraModelDir =
      std::string(LoraModelDirSpan.begin(), LoraModelDirSpan.end());
  std::string EmbedDir = std::string(EmbedDirSpan.begin(), EmbedDirSpan.end());
  std::string IdEmbedDir =
      std::string(IdEmbedDirSpan.begin(), IdEmbedDirSpan.end());
  std::string clipLPath =
      std::string(clipLPathSpan.begin(), clipLPathSpan.end());
  std::string t5xxlPath =
      std::string(t5xxlPathSpan.begin(), t5xxlPathSpan.end());
  std::string diffusionModelPath =
      std::string(diffusionModelPathSpan.begin(), diffusionModelPathSpan.end());
  if (NThreads == -1) {
    NThreads = get_num_physical_cores();
  }

  spdlog::info("[WasmEdge-StableDiffusion] Create context."sv);
  // Create context and import graph.

  sd_ctx_t *Ctx = new_sd_ctx(
      ModelPath.data(), clipLPath.data(), t5xxlPath.data(),
      diffusionModelPath.data(), VaePath.data(), TaesdPath.data(),
      ControlNetPath.data(), LoraModelDir.data(), EmbedDir.data(),
      IdEmbedDir.data(), static_cast<bool>(VaeDecodeOnly),
      static_cast<bool>(VaeTiling), true, NThreads,
      static_cast<sd_type_t>(Wtype), static_cast<rng_type_t>(RngType),
      static_cast<schedule_t>(Schedule), ClipOnCpu, ControlNetCpu, VaeOnCpu);
  if (Ctx == nullptr) {
    spdlog::error("[WasmEdge-StableDiffusion] Failed to create context.");
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  *SessionId = Env.addContext(Ctx);

  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> SDTextToImage::body(
    const Runtime::CallingFrame &Frame, uint32_t PromptPtr, uint32_t PromptLen,
    uint32_t SessionId, uint32_t ControlImagePtr, uint32_t ControlImageLen,
    uint32_t NegativePromptPtr, uint32_t NegativePromptLen, float Guidance,
    uint32_t Width, uint32_t Height, int32_t ClipSkip, float CfgScale,
    uint32_t SampleMethod, uint32_t SampleSteps, uint32_t Seed,
    uint32_t BatchCount, float ControlStrength, float StyleRatio,
    uint32_t NormalizeInput, uint32_t InputIdImagesDirPtr,
    uint32_t InputIdImagesDirLen, uint32_t CannyPreprocess, uint32_t, uint32_t,
    uint32_t, uint32_t OutputPathPtr, uint32_t OutputPathLen,
    uint32_t OutBufferPtr, uint32_t OutBufferMaxSize,
    uint32_t BytesWrittenPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)
  // Check the input model buffer.
  MEM_SPAN_CHECK(PromptSpan, MemInst, char, PromptPtr, PromptLen,
                 "Failed when accessing the promp memory."sv)
  MEM_SPAN_CHECK(NegativePromptSpan, MemInst, char, NegativePromptPtr,
                 NegativePromptLen,
                 "Failed when accessing the input negative prompt memory."sv)
  MEM_SPAN_CHECK(InputIdImagesDirSpan, MemInst, char, InputIdImagesDirPtr,
                 InputIdImagesDirLen,
                 "Failed when accessing the input id images path memory."sv)
  MEM_SPAN_CHECK(OutputBufferSpan, MemInst, uint8_t, OutBufferPtr,
                 OutBufferMaxSize,
                 "Failed when accessing the Output Buffer memory."sv)
  MEM_PTR_CHECK(BytesWritten, MemInst, uint32_t, BytesWrittenPtr,
                "Failed when accessing the return bytes written memory."sv)
  MEM_SPAN_CHECK(OutputPathSpan, MemInst, char, OutputPathPtr, OutputPathLen,
                 "Failed when accessing the output path memory."sv)
  std::string Prompt(PromptSpan.begin(), PromptSpan.end());
  std::string NegativePrompt(NegativePromptSpan.begin(),
                             NegativePromptSpan.end());
  std::string InputIdImagesDir(InputIdImagesDirSpan.begin(),
                               InputIdImagesDirSpan.end());
  std::string OutputPath(OutputPathSpan.begin(), OutputPathSpan.end());
  if (!parameterCheck(Env, Width, Height, SessionId)) {
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  sd_ctx_t *SDCtx = Env.getContext(SessionId);
  sd_image_t *Results = nullptr;
  sd_image_t *ControlImage = nullptr;
  uint8_t *ControlImageBuffer = nullptr;
  if (ControlImageLen != 0) {
    MEM_SPAN_CHECK(ControlImageSpan, MemInst, uint8_t, ControlImagePtr,
                   ControlImageLen,
                   "Failed when accessing the control image memory."sv)
    ControlImage = readControlImage(ControlImageSpan, ControlImageBuffer, Width,
                                    Height, CannyPreprocess);
  }
  spdlog::info("[WasmEdge-StableDiffusion] Start to generate image."sv);
  Results =
      txt2img(SDCtx, Prompt.data(), NegativePrompt.data(), ClipSkip, CfgScale,
              Guidance, Width, Height, sample_method_t(SampleMethod),
              SampleSteps, Seed, BatchCount, ControlImage, ControlStrength,
              StyleRatio, NormalizeInput, InputIdImagesDir.data());
  // TODO upscale image
  int Len;
  unsigned char *Png = stbi_write_png_to_mem(
      reinterpret_cast<const unsigned char *>(Results->data), 0, Results->width,
      Results->height, Results->channel, &Len, nullptr);
  if (OutputPathLen != 0) {
    stbi_write_png(OutputPath.data(), Results->width, Results->height,
                   Results->channel, Results->data, 0, nullptr);
  }
  *BytesWritten = Len;
  if (OutBufferMaxSize < *BytesWritten) {
    spdlog::error("[WasmEdge-StableDiffusion] Output buffer is not enough."sv);
    free(Png);
    free(Results);
    free(ControlImageBuffer);
    return static_cast<uint32_t>(ErrNo::RuntimeError);
  }
  std::copy_n(Png, *BytesWritten, OutputBufferSpan.data());
  free(Png);
  free(Results);
  free(ControlImageBuffer);
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> SDImageToImage::body(
    const Runtime::CallingFrame &Frame, uint32_t ImagePtr, uint32_t ImageLen,
    uint32_t SessionId, float Guidance, uint32_t Width, uint32_t Height,
    uint32_t ControlImagePtr, uint32_t ControlImageLen, uint32_t PromptPtr,
    uint32_t PromptLen, uint32_t NegativePromptPtr, uint32_t NegativePromptLen,
    int32_t ClipSkip, float CfgScale, uint32_t SampleMethod,
    uint32_t SampleSteps, float Strength, uint32_t Seed, uint32_t BatchCount,
    float ControlStrength, float StyleRatio, uint32_t NormalizeInput,
    uint32_t InputIdImagesDirPtr, uint32_t InputIdImagesDirLen,
    uint32_t CannyPreprocess, uint32_t, uint32_t, uint32_t,
    uint32_t OutputPathPtr, uint32_t OutputPathLen, uint32_t OutBufferPtr,
    uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input parameter valid.
  MEM_SPAN_CHECK(ImageSpan, MemInst, uint8_t, ImagePtr, ImageLen,
                 "Failed when accessing the input image memory."sv)

  MEM_SPAN_CHECK(PromptSpan, MemInst, char, PromptPtr, PromptLen,
                 "Failed when accessing the promp memory."sv)
  MEM_SPAN_CHECK(NegativePromptSpan, MemInst, char, NegativePromptPtr,
                 NegativePromptLen,
                 "Failed when accessing the input negative prompt memory."sv)
  MEM_SPAN_CHECK(InputIdImagesDirSpan, MemInst, char, InputIdImagesDirPtr,
                 InputIdImagesDirLen,
                 "Failed when accessing the input id images path memory."sv)
  MEM_SPAN_CHECK(OutputBufferSpan, MemInst, uint8_t, OutBufferPtr,
                 OutBufferMaxSize,
                 "Failed when accessing the Output Buffer memory."sv)
  MEM_PTR_CHECK(BytesWritten, MemInst, uint32_t, BytesWrittenPtr,
                "Failed when accessing the return bytes written memory."sv)
  MEM_SPAN_CHECK(OutputPathSpan, MemInst, char, OutputPathPtr, OutputPathLen,
                 "Failed when accessing the output path memory."sv)
  if (!parameterCheck(Env, Width, Height, SessionId)) {
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  sd_ctx_t *SDCtx = Env.getContext(SessionId);
  std::string Prompt(PromptSpan.begin(), PromptSpan.end());
  std::string NegativePrompt(NegativePromptSpan.begin(),
                             NegativePromptSpan.end());
  std::string InputIdImagesDir(InputIdImagesDirSpan.begin(),
                               InputIdImagesDirSpan.end());
  std::string OutputPath(OutputPathSpan.begin(), OutputPathSpan.end());
  uint8_t *InputImageBuffer = nullptr;
  uint8_t *ControlImageBuffer = nullptr;
  int Channel = 0;
  int ImageWidth = 0;
  int ImageHeight = 0;
  std::string ImagePath(ImageSpan.begin(), ImageSpan.end());
  if (ImagePath.substr(0, 5) == "path:"sv) {
    InputImageBuffer = stbi_load(ImagePath.substr(5).data(), &ImageWidth,
                                 &ImageHeight, &Channel, 3);
    if (InputImageBuffer == nullptr) {
      spdlog::error(
          "[WasmEdge-StableDiffusion] Load image from input image failed."sv);
      return static_cast<uint32_t>(ErrNo::InvalidArgument);
    }
  } else {
    InputImageBuffer =
        stbi_load_from_memory(ImageSpan.data(), ImageSpan.size(), &ImageWidth,
                              &ImageHeight, &Channel, 3);
  }
  // TODO: Resize image when image size not matches width and height
  sd_image_t InputImage = {Width, Height, 3, InputImageBuffer};
  sd_image_t *ControlImage = nullptr;
  if (ControlImageLen != 0) {
    MEM_SPAN_CHECK(ControlImageSpan, MemInst, uint8_t, ControlImagePtr,
                   ControlImageLen,
                   "Failed when accessing the control image memory."sv)
    ControlImage = readControlImage(ControlImageSpan, ControlImageBuffer, Width,
                                    Height, CannyPreprocess);
  }
  sd_image_t *Results = nullptr;
  spdlog::info("[WasmEdge-StableDiffusion] Start to generate image."sv);
  Results = img2img(SDCtx, InputImage, Prompt.data(), NegativePrompt.data(),
                    ClipSkip, CfgScale, Guidance, Width, Height,
                    sample_method_t(SampleMethod), SampleSteps, Strength, Seed,
                    BatchCount, ControlImage, ControlStrength, StyleRatio,
                    NormalizeInput, InputIdImagesDir.data());
  // TODO: upscale image
  int Len;
  unsigned char *Png = stbi_write_png_to_mem(
      reinterpret_cast<const unsigned char *>(Results->data), 0, Results->width,
      Results->height, Results->channel, &Len, nullptr);
  if (OutputPathLen != 0) {
    stbi_write_png(OutputPath.data(), Results->width, Results->height,
                   Results->channel, Results->data, 0, nullptr);
  }
  *BytesWritten = Len;
  if (OutBufferMaxSize < *BytesWritten) {
    spdlog::error("[WasmEdge-StableDiffusion] Output buffer is not enough."sv);
    free(Png);
    free(Results);
    free(InputImageBuffer);
    free(ControlImageBuffer);
    return static_cast<uint32_t>(ErrNo::RuntimeError);
  }
  std::copy_n(Png, *BytesWritten, OutputBufferSpan.data());
  free(Png);
  free(Results);
  free(InputImageBuffer);
  free(ControlImageBuffer);
  return static_cast<uint32_t>(ErrNo::Success);
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
