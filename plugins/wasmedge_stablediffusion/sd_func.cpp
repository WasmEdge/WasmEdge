#include "sd_func.h"
#include "common/spdlog.h"
#include "sd_env.h"
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

sd_image_t *ReadControlImage(Span<uint8_t> ControlImage,
                             uint8_t *control_image_buffer, int Width,
                             int Height, bool canny_preprocess) {
  sd_image_t *control_image = NULL;
  int Channel = 0;
  control_image_buffer = stbi_load_from_memory(
      ControlImage.data(), ControlImage.size(), &Width, &Height, &Channel, 3);
  if (control_image_buffer == NULL) {
    spdlog::error("[StableDiffusion] Load image from control image failed."sv);
    return nullptr;
  }
  control_image = new sd_image_t{(uint32_t)Width, (uint32_t)Height, 3,
                                 control_image_buffer};
  if (canny_preprocess) { // apply preprocessor
    control_image->data = preprocess_canny(
        control_image->data, control_image->width, control_image->height, 0.08f,
        0.08f, 0.8f, 1.0f, false);
  }
  return control_image;
}

Expect<uint32_t> SDCreateContext::body(
    const Runtime::CallingFrame &Frame, uint32_t ModelPathPtr,
    uint32_t ModelPathLen, uint32_t VaePathPtr, uint32_t VaePathLen,
    uint32_t TaesdPathPtr, uint32_t TaesdPathLen, uint32_t ControlNetPathPtr,
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

  // Create context and import graph.
  sd_ctx_t *sd_ctx =
      new_sd_ctx(ModelPathSpan.data(), VaePathSpan.data(), TaesdPathSpan.data(),
                 ControlNetPathSpan.data(), LoraModelDirSpan.data(),
                 EmbedDirSpan.data(), IdEmbedDirSpan.data(),
                 static_cast<bool>(VaeDecodeOnly), static_cast<bool>(VaeTiling),
                 true, NThreads, sd_type_t(Wtype), rng_type_t(RngType),
                 schedule_t(Schedule), ClipOnCpu, ControlNetCpu, VaeOnCpu);
  if (sd_ctx == NULL) {
    spdlog::error("[WasmEdge-StableDiffusion] Failed to create context.");
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  *SessionId = Env.addContext(sd_ctx);

  return static_cast<uint32_t>(ErrNo::Success);
}
Expect<uint32_t> SDTextToImage::body(
    const Runtime::CallingFrame &Frame, uint32_t ImagePtr, uint32_t ImageLen,
    uint32_t SessionId, uint32_t Width, uint32_t Height,
    uint32_t ControlImagePtr, uint32_t ControlImageLen, uint32_t PromptPtr,
    uint32_t PromptLen, uint32_t NegativePromptPtr, uint32_t NegativePromptLen,
    uint32_t ClipSkip, uint32_t CfgScale, uint32_t SampleMethod,
    uint32_t SampleSteps, uint32_t Strength, uint32_t Seed, uint32_t BatchCount,
    uint32_t ControlStrength, uint32_t StyleRatio, uint32_t NormalizeInput,
    uint32_t InputIdImagesPathPtr, uint32_t InputIdImagesPathLen,
    uint32_t canny_preprocess, uint32_t OutBufferPtr, uint32_t OutBufferMaxSize,
    uint32_t BytesWrittenPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input model buffer.
  MEM_SPAN_CHECK(ImageSpan, MemInst, uint8_t, ImagePtr, ImageLen,
                 "Failed when accessing the input image memory."sv)

  MEM_SPAN_CHECK(PromptSpan, MemInst, char, PromptPtr, PromptLen,
                 "Failed when accessing the promp memory."sv)
  MEM_SPAN_CHECK(NegativePromptSpan, MemInst, char, NegativePromptPtr,
                 NegativePromptLen,
                 "Failed when accessing the input negative prompt memory."sv)
  MEM_SPAN_CHECK(
      InputIdImagesPathSpan, MemInst, char, InputIdImagesPathPtr,
      InputIdImagesPathLen,
      "Failed when accessing the input input id images path memory."sv)
  MEM_SPAN_CHECK(OutputBufferSpan, MemInst, uint8_t, OutBufferPtr,
                 OutBufferMaxSize,
                 "Failed when accessing the Output Buffer memory."sv)
  MEM_PTR_CHECK(BytesWritten, MemInst, uint32_t, BytesWrittenPtr,
                "Failed when accessing the return bytes written memory."sv)
  sd_ctx_t *SDCtx = Env.getContext(SessionId);

  uint8_t *InputImageBuffer = nullptr;
  uint8_t *control_image_buffer = nullptr;
  int Channel = 0;
  int ImageWidth = 0;
  int ImageHeight = 0;
  InputImageBuffer =
      stbi_load_from_memory(ImageSpan.data(), ImageSpan.size(), &ImageWidth,
                            &ImageHeight, &Channel, 3);
  sd_image_t InputImage = {Width, Height, 3, InputImageBuffer};
  sd_image_t *ControlImage;
  if (ControlImageLen != 0) {
    MEM_SPAN_CHECK(ControlImageSpan, MemInst, uint8_t, ControlImagePtr,
                   ControlImageLen,
                   "Failed when accessing the control image memory."sv)
    ControlImage = ReadControlImage(ControlImageSpan, control_image_buffer,
                                    Width, Height, canny_preprocess);
  }

  sd_image_t *results;
  results = img2img(SDCtx, InputImage, PromptSpan.data(),
                    NegativePromptSpan.data(), ClipSkip, CfgScale, Width,
                    Height, sample_method_t(SampleMethod), SampleSteps,
                    Strength, Seed, BatchCount, ControlImage, ControlStrength,
                    StyleRatio, NormalizeInput, InputIdImagesPathSpan.data());
  int len;
  unsigned char *png =
      stbi_write_png_to_mem((const unsigned char *)results, 0, results->width,
                            results->height, results->channel, &len, NULL);
  *BytesWritten = len;
  std::copy_n(png, *BytesWritten, OutputBufferSpan.data());
  free(results);
  free(InputImageBuffer);
  free(control_image_buffer);
  return static_cast<uint32_t>(ErrNo::RuntimeError);
}
} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge