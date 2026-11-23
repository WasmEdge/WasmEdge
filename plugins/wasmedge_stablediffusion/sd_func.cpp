// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#include "stb_image_resize.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

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

namespace {

struct FreeDeleter {
  void operator()(void *Pointer) const noexcept { free(Pointer); }
};
using PixelPtr = std::unique_ptr<uint8_t, FreeDeleter>;

struct UpscalerDeleter {
  void operator()(upscaler_ctx_t *Ctx) const noexcept {
    free_upscaler_ctx(Ctx);
  }
};

// Unused by the ESRGAN graph of the pinned library.
constexpr uint32_t UpscaleFactor = 4;

// preprocess_canny works in a fixed 10 MiB buffer and needs 32 bytes per pixel.
constexpr uint64_t CannyMaxPixels = (10U * 1024U * 1024U - 64U * 1024U) / 32U;

bool parameterCheck(uint32_t Width, uint32_t Height, uint32_t BatchCount) {
  if (Width == 0 || Width % 64 != 0) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Width must be a multiple of 64 and greater than 0."sv);
    return false;
  }
  if (Height == 0 || Height % 64 != 0) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Height must be a multiple of 64 and greater than 0."sv);
    return false;
  }
  if (static_cast<uint64_t>(Width) * Height * 3U >
      static_cast<uint64_t>(std::numeric_limits<int>::max())) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Width and height are too large."sv);
    return false;
  }
  if (BatchCount == 0 ||
      BatchCount > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Batch count must be greater than 0 and fit in an int."sv);
    return false;
  }
  return true;
}

struct DecodedImage {
  PixelPtr Pixels;
  int Width = 0;
  int Height = 0;
};

// Decode an image given as encoded bytes, or as "path:" and a file path.
DecodedImage decodeImage(Span<const uint8_t> Data, int Channels) {
  constexpr std::string_view PathPrefix = "path:"sv;
  DecodedImage Image;
  int FileChannels = 0;
  if (Data.size() >= PathPrefix.size() &&
      std::equal(PathPrefix.begin(), PathPrefix.end(), Data.begin())) {
    const std::string Path(Data.begin() + PathPrefix.size(), Data.end());
    Image.Pixels.reset(stbi_load(Path.c_str(), &Image.Width, &Image.Height,
                                 &FileChannels, Channels));
  } else if (Data.size() <=
             static_cast<size_t>(std::numeric_limits<int>::max())) {
    Image.Pixels.reset(stbi_load_from_memory(
        Data.data(), static_cast<int>(Data.size()), &Image.Width, &Image.Height,
        &FileChannels, Channels));
  }
  if (Image.Width <= 0 || Image.Height <= 0) {
    Image.Pixels.reset();
  }
  return Image;
}

// Resize the image to Width x Height. Return null on failure.
PixelPtr resizeImage(DecodedImage Image, int Width, int Height, int Channels) {
  if (!Image.Pixels) {
    return {};
  }
  if (Image.Width == Width && Image.Height == Height) {
    return std::move(Image.Pixels);
  }
  if (Width <= 0 || Height <= 0) {
    return {};
  }
  PixelPtr Resized(static_cast<uint8_t *>(
      malloc(static_cast<size_t>(Width) * Height * Channels)));
  if (!Resized ||
      stbir_resize(Image.Pixels.get(), Image.Width, Image.Height, 0,
                   Resized.get(), Width, Height, 0, STBIR_TYPE_UINT8, Channels,
                   STBIR_ALPHA_CHANNEL_NONE, 0, STBIR_EDGE_CLAMP,
                   STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_FILTER_BOX,
                   STBIR_COLORSPACE_SRGB, nullptr) == 0) {
    return {};
  }
  return Resized;
}

// preprocess_canny frees its input when it processes it, but returns null
// without freeing it when it cannot create its workspace. Both cases return
// null when they fail, so the input is handed over and never freed here. A
// failed workspace creation leaks the input instead of freeing it twice.
PixelPtr cannyImage(PixelPtr Image, int Width, int Height) {
  return PixelPtr(preprocess_canny(Image.release(), Width, Height, 0.08f, 0.08f,
                                   0.8f, 1.0f, false));
}

PixelPtr prepareInputImage(Span<const uint8_t> Data, int Width, int Height) {
  DecodedImage Image = decodeImage(Data, 3);
  if (!Image.Pixels) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Load image from input image failed."sv);
    return {};
  }
  PixelPtr Resized = resizeImage(std::move(Image), Width, Height, 3);
  if (!Resized) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Failed to resize the input image."sv);
  }
  return Resized;
}

PixelPtr prepareControlImage(Span<const uint8_t> Data, int Width, int Height,
                             bool CannyPreprocess) {
  DecodedImage Image = decodeImage(Data, 3);
  if (!Image.Pixels) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Load image from control image failed."sv);
    return {};
  }
  if (CannyPreprocess) {
    if (static_cast<uint64_t>(Image.Width) *
            static_cast<uint64_t>(Image.Height) >
        CannyMaxPixels) {
      spdlog::error(
          "[WasmEdge-StableDiffusion] Control image is too large for the canny preprocessor."sv);
      return {};
    }
    Image.Pixels =
        cannyImage(std::move(Image.Pixels), Image.Width, Image.Height);
  }
  PixelPtr Resized = resizeImage(std::move(Image), Width, Height, 3);
  if (!Resized) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Failed to preprocess the control image."sv);
  }
  return Resized;
}

PixelPtr prepareMaskImage(Span<const uint8_t> Data, int Width, int Height) {
  PixelPtr Mask;
  if (Data.empty()) {
    const size_t Size = static_cast<size_t>(Width) * Height;
    Mask.reset(static_cast<uint8_t *>(malloc(Size)));
    if (Mask) {
      std::fill_n(Mask.get(), Size, static_cast<uint8_t>(255));
    }
  } else {
    Mask = resizeImage(decodeImage(Data, 1), Width, Height, 1);
  }
  if (!Mask) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] Load image from mask image failed."sv);
  }
  return Mask;
}

// Owns the result array of txt2img and img2img and the pixels of each image.
class ResultImages {
public:
  ResultImages(sd_image_t *Images, uint32_t Count) noexcept
      : Images(Images), Count(Images != nullptr ? Count : 0) {}
  ResultImages(const ResultImages &) = delete;
  ResultImages &operator=(const ResultImages &) = delete;
  ~ResultImages() noexcept {
    for (uint32_t I = 0; I < Count; ++I) {
      free(Images[I].data);
    }
    free(Images);
  }
  explicit operator bool() const noexcept { return Images != nullptr; }
  uint32_t size() const noexcept { return Count; }
  sd_image_t &operator[](uint32_t I) noexcept { return Images[I]; }

private:
  sd_image_t *Images;
  uint32_t Count;
};

void upscaleResults(ResultImages &Results, const std::string &ModelPath,
                    uint32_t UpscaleRepeats, int32_t NThreads) {
  if (UpscaleRepeats == 0) {
    return;
  }
  std::unique_ptr<upscaler_ctx_t, UpscalerDeleter> UpscalerCtx(
      new_upscaler_ctx(ModelPath.c_str(), NThreads));
  if (!UpscalerCtx) {
    spdlog::error("[WasmEdge-StableDiffusion] Create upscaler ctx failed."sv);
    return;
  }
  for (uint32_t I = 0; I < Results.size(); I++) {
    sd_image_t &Image = Results[I];
    for (uint32_t U = 0; U < UpscaleRepeats && Image.data != nullptr; ++U) {
      sd_image_t Upscaled = upscale(UpscalerCtx.get(), Image, UpscaleFactor);
      if (Upscaled.data == nullptr) {
        spdlog::error("[WasmEdge-StableDiffusion] Upscale failed."sv);
        break;
      }
      free(Image.data);
      Image = Upscaled;
    }
  }
}

void writeResultFiles(ResultImages &Results, std::string_view OutputPath) {
  std::string_view BaseName = OutputPath;
  std::string_view Extension = ".png"sv;
  if (const size_t Last = OutputPath.find_last_of('.');
      Last != std::string_view::npos) {
    const std::string_view LastStr = OutputPath.substr(Last);
    if (LastStr == ".png"sv || LastStr == ".PNG"sv) {
      BaseName = OutputPath.substr(0, Last);
      Extension = LastStr;
    }
  }
  for (uint32_t I = 0; I < Results.size(); I++) {
    const sd_image_t &Image = Results[I];
    if (Image.data == nullptr) {
      continue;
    }
    std::string FileName(BaseName);
    if (I > 0) {
      FileName += "_" + std::to_string(I + 1);
    }
    FileName += Extension;
    stbi_write_png(FileName.c_str(), static_cast<int>(Image.width),
                   static_cast<int>(Image.height),
                   static_cast<int>(Image.channel), Image.data, 0);
    spdlog::info("[WasmEdge-StableDiffusion] Save result image to {}."sv,
                 FileName);
  }
}

ErrNo saveResults(ResultImages &Results, std::string_view OutputPath,
                  Span<uint8_t> OutBuffer, uint32_t &BytesWritten) {
  BytesWritten = 0;
  int Len = 0;
  std::unique_ptr<uint8_t, FreeDeleter> Png;
  if (Results.size() > 0 && Results[0].data != nullptr) {
    Png.reset(stbi_write_png_to_mem(
        Results[0].data, 0, static_cast<int>(Results[0].width),
        static_cast<int>(Results[0].height),
        static_cast<int>(Results[0].channel), &Len, nullptr));
  }
  if (!OutputPath.empty()) {
    writeResultFiles(Results, OutputPath);
  }
  if (!Png || Len < 0) {
    spdlog::error("[WasmEdge-StableDiffusion] Encode result image failed."sv);
    return ErrNo::RuntimeError;
  }
  BytesWritten = static_cast<uint32_t>(Len);
  if (OutBuffer.size() < BytesWritten) {
    spdlog::error("[WasmEdge-StableDiffusion] Output buffer is not enough."sv);
    return ErrNo::RuntimeError;
  }
  std::copy_n(Png.get(), BytesWritten, OutBuffer.data());
  return ErrNo::Success;
}

ErrNo finishResults(ResultImages &Results, Span<const char> UpscaleModelPath,
                    uint32_t UpscaleRepeats, int32_t NThreads,
                    std::string_view OutputPath, Span<uint8_t> OutBuffer,
                    uint32_t &BytesWritten) {
  if (!UpscaleModelPath.empty()) {
    upscaleResults(
        Results, std::string(UpscaleModelPath.begin(), UpscaleModelPath.end()),
        UpscaleRepeats, NThreads);
  }
  return saveResults(Results, OutputPath, OutBuffer, BytesWritten);
}

} // namespace

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
    spdlog::error("[WasmEdge-StableDiffusion] Model not found."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  Fin.close();
  // Convert model.
  bool Ret = ::convert(ModelPath.data(), VaeModelPath.data(), OutputPath.data(),
                       static_cast<sd_type_t>(WType));
  if (!Ret) {
    spdlog::error("[WasmEdge-StableDiffusion] Failed to convert model."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> SDCreateContext::body(
    const Runtime::CallingFrame &Frame, uint32_t ModelPathPtr,
    uint32_t ModelPathLen, uint32_t clipLPathPtr, uint32_t clipLPathLen,
    uint32_t clipGPathPtr, uint32_t clipGPathLen, uint32_t t5xxlPathPtr,
    uint32_t t5xxlPathLen, uint32_t diffusionModelPathPtr,
    uint32_t diffusionModelPathLen, uint32_t VaePathPtr, uint32_t VaePathLen,
    uint32_t TaesdPathPtr, uint32_t TaesdPathLen, uint32_t ControlNetPathPtr,
    uint32_t ControlNetPathLen, uint32_t LoraModelDirPtr,
    uint32_t LoraModelDirLen, uint32_t EmbedDirPtr, uint32_t EmbedDirLen,
    uint32_t IdEmbedDirPtr, uint32_t IdEmbedDirLen, uint32_t VaeDecodeOnly,
    uint32_t VaeTiling, int32_t NThreads, uint32_t Wtype, uint32_t RngType,
    uint32_t Schedule, uint32_t ClipOnCpu, uint32_t ControlNetCpu,
    uint32_t VaeOnCpu, uint32_t DiffusionFlashAttn, uint32_t SessiontIdPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)
  // Check the input model buffer.
  MEM_SPAN_CHECK(ModelPathSpan, MemInst, char, ModelPathPtr, ModelPathLen,
                 "Failed when accessing the input model path memory."sv)
  MEM_SPAN_CHECK(clipLPathSpan, MemInst, char, clipLPathPtr, clipLPathLen,
                 "Failed when accessing the input clipL path memory."sv)
  MEM_SPAN_CHECK(clipGPathSpan, MemInst, char, clipGPathPtr, clipGPathLen,
                 "Failed when accessing the input clipG path memory."sv)
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
  std::string clipGPath =
      std::string(clipGPathSpan.begin(), clipGPathSpan.end());
  std::string t5xxlPath =
      std::string(t5xxlPathSpan.begin(), t5xxlPathSpan.end());
  std::string diffusionModelPath =
      std::string(diffusionModelPathSpan.begin(), diffusionModelPathSpan.end());
  if (NThreads == -1) {
    NThreads = get_num_physical_cores();
  }
  // Check parameters
  if (ModelPathLen == 0 && diffusionModelPathLen == 0) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] The following arguments are required: ModelPath / DiffusionModelPath"sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  // Create context and import graph.
  spdlog::debug("[WasmEdge-StableDiffusion] Create context."sv);
  sd_ctx_t *Ctx = new_sd_ctx(
      ModelPath.data(), clipLPath.data(), clipGPath.data(), t5xxlPath.data(),
      diffusionModelPath.data(), VaePath.data(), TaesdPath.data(),
      ControlNetPath.data(), LoraModelDir.data(), EmbedDir.data(),
      IdEmbedDir.data(), static_cast<bool>(VaeDecodeOnly),
      static_cast<bool>(VaeTiling), false, NThreads,
      static_cast<sd_type_t>(Wtype), static_cast<rng_type_t>(RngType),
      static_cast<schedule_t>(Schedule), ClipOnCpu, ControlNetCpu, VaeOnCpu,
      DiffusionFlashAttn);
  if (Ctx == nullptr) {
    spdlog::error("[WasmEdge-StableDiffusion] Failed to create context."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  *SessionId = Env.addContext(Ctx, NThreads);
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
    uint32_t InputIdImagesDirLen, uint32_t CannyPreprocess,
    uint32_t UpscaleModelPathPtr, uint32_t UpscaleModelPathLen,
    uint32_t UpscaleRepeats, uint32_t SkipLayersPtr, uint32_t SkipLayersLen,
    float SlgScale, float SkipLayerStart, float SkipLayerEnd,
    uint32_t OutputPathPtr, uint32_t OutputPathLen, uint32_t OutBufferPtr,
    uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr) {
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
  MEM_SPAN_CHECK(SkipLayersSpan, MemInst, int32_t, SkipLayersPtr, SkipLayersLen,
                 "Failed when accessing the SkipLayers memory."sv)
  MEM_SPAN_CHECK(UpscaleModelSpan, MemInst, char, UpscaleModelPathPtr,
                 UpscaleModelPathLen,
                 "Failed when accessing the Upscaler Image memory."sv)
  MEM_SPAN_CHECK(ControlImageSpan, MemInst, uint8_t, ControlImagePtr,
                 ControlImageLen,
                 "Failed when accessing the control image memory."sv)
  if (!parameterCheck(Width, Height, BatchCount)) {
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  SESSION_CHECK(SDCtx, SessionId, "Session ID is invalid."sv,
                ErrNo::InvalidArgument)
  std::string Prompt(PromptSpan.begin(), PromptSpan.end());
  std::string NegativePrompt(NegativePromptSpan.begin(),
                             NegativePromptSpan.end());
  std::string InputIdImagesDir(InputIdImagesDirSpan.begin(),
                               InputIdImagesDirSpan.end());
  // Read control image
  PixelPtr ControlPixels;
  std::optional<sd_image_t> ControlImage;
  if (ControlImageLen != 0) {
    ControlPixels =
        prepareControlImage(ControlImageSpan, static_cast<int>(Width),
                            static_cast<int>(Height), CannyPreprocess);
    if (!ControlPixels) {
      return static_cast<uint32_t>(ErrNo::InvalidArgument);
    }
    ControlImage = sd_image_t{Width, Height, 3, ControlPixels.get()};
  }
  // Generate images
  spdlog::info("[WasmEdge-StableDiffusion] Start to generate image."sv);
  ResultImages Results(
      txt2img(SDCtx, Prompt.data(), NegativePrompt.data(), ClipSkip, CfgScale,
              Guidance, Width, Height, sample_method_t(SampleMethod),
              SampleSteps, Seed, BatchCount,
              ControlImage ? &*ControlImage : nullptr, ControlStrength,
              StyleRatio, NormalizeInput, InputIdImagesDir.data(),
              SkipLayersSpan.data(), SkipLayersSpan.size(), SlgScale,
              SkipLayerStart, SkipLayerEnd),
      BatchCount);
  ControlPixels.reset();
  if (!Results) {
    spdlog::error("[WasmEdge-StableDiffusion] Generate failed."sv);
    Env.freeContext(SessionId);
    return static_cast<uint32_t>(ErrNo::RuntimeError);
  }
  return static_cast<uint32_t>(finishResults(
      Results, UpscaleModelSpan, UpscaleRepeats, Env.getNThreads(SessionId),
      std::string_view(OutputPathSpan.data(), OutputPathSpan.size()),
      OutputBufferSpan, *BytesWritten));
}

Expect<uint32_t> SDImageToImage::body(
    const Runtime::CallingFrame &Frame, uint32_t ImagePtr, uint32_t ImageLen,
    uint32_t MaskImagePtr, uint32_t MaskImageLen, uint32_t SessionId,
    float Guidance, uint32_t Width, uint32_t Height, uint32_t ControlImagePtr,
    uint32_t ControlImageLen, uint32_t PromptPtr, uint32_t PromptLen,
    uint32_t NegativePromptPtr, uint32_t NegativePromptLen, int32_t ClipSkip,
    float CfgScale, uint32_t SampleMethod, uint32_t SampleSteps, float Strength,
    uint32_t Seed, uint32_t BatchCount, float ControlStrength, float StyleRatio,
    uint32_t NormalizeInput, uint32_t InputIdImagesDirPtr,
    uint32_t InputIdImagesDirLen, uint32_t CannyPreprocess,
    uint32_t UpscaleModelPathPtr, uint32_t UpscaleModelPathLen,
    uint32_t UpscaleRepeats, uint32_t SkipLayersPtr, uint32_t SkipLayersLen,
    float SlgScale, float SkipLayerStart, float SkipLayerEnd,
    uint32_t OutputPathPtr, uint32_t OutputPathLen, uint32_t OutBufferPtr,
    uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)
  // Check the input parameter valid.
  MEM_SPAN_CHECK(ImageSpan, MemInst, uint8_t, ImagePtr, ImageLen,
                 "Failed when accessing the input image memory."sv)
  MEM_SPAN_CHECK(MaskImageSpan, MemInst, uint8_t, MaskImagePtr, MaskImageLen,
                 "Failed when accessing the input mask image memory."sv)
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
  MEM_SPAN_CHECK(SkipLayersSpan, MemInst, int32_t, SkipLayersPtr, SkipLayersLen,
                 "Failed when accessing the SkipLayers memory."sv)
  MEM_SPAN_CHECK(UpscaleModelSpan, MemInst, char, UpscaleModelPathPtr,
                 UpscaleModelPathLen,
                 "Failed when accessing the Upscaler Image memory."sv)
  MEM_SPAN_CHECK(ControlImageSpan, MemInst, uint8_t, ControlImagePtr,
                 ControlImageLen,
                 "Failed when accessing the control image memory."sv)
  if (!parameterCheck(Width, Height, BatchCount)) {
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  SESSION_CHECK(SDCtx, SessionId, "Session ID is invalid."sv,
                ErrNo::InvalidArgument)
  std::string Prompt(PromptSpan.begin(), PromptSpan.end());
  std::string NegativePrompt(NegativePromptSpan.begin(),
                             NegativePromptSpan.end());
  std::string InputIdImagesDir(InputIdImagesDirSpan.begin(),
                               InputIdImagesDirSpan.end());
  // Read input image
  PixelPtr InputPixels = prepareInputImage(ImageSpan, static_cast<int>(Width),
                                           static_cast<int>(Height));
  if (!InputPixels) {
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  sd_image_t InputImage = {Width, Height, 3, InputPixels.get()};
  // Read control image
  PixelPtr ControlPixels;
  std::optional<sd_image_t> ControlImage;
  if (ControlImageLen != 0) {
    ControlPixels =
        prepareControlImage(ControlImageSpan, static_cast<int>(Width),
                            static_cast<int>(Height), CannyPreprocess);
    if (!ControlPixels) {
      return static_cast<uint32_t>(ErrNo::InvalidArgument);
    }
    ControlImage = sd_image_t{Width, Height, 3, ControlPixels.get()};
  }
  // Read mask image
  PixelPtr MaskPixels = prepareMaskImage(MaskImageSpan, static_cast<int>(Width),
                                         static_cast<int>(Height));
  if (!MaskPixels) {
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  sd_image_t MaskImage = {Width, Height, 1, MaskPixels.get()};
  // Generate images
  spdlog::info("[WasmEdge-StableDiffusion] Start to generate image."sv);
  ResultImages Results(
      img2img(SDCtx, InputImage, MaskImage, Prompt.data(),
              NegativePrompt.data(), ClipSkip, CfgScale, Guidance, Width,
              Height, sample_method_t(SampleMethod), SampleSteps, Strength,
              Seed, BatchCount, ControlImage ? &*ControlImage : nullptr,
              ControlStrength, StyleRatio, NormalizeInput,
              InputIdImagesDir.data(), SkipLayersSpan.data(),
              SkipLayersSpan.size(), SlgScale, SkipLayerStart, SkipLayerEnd),
      BatchCount);
  ControlPixels.reset();
  MaskPixels.reset();
  InputPixels.reset();
  if (!Results) {
    spdlog::error("[WasmEdge-StableDiffusion] Generate failed."sv);
    Env.freeContext(SessionId);
    return static_cast<uint32_t>(ErrNo::RuntimeError);
  }
  return static_cast<uint32_t>(finishResults(
      Results, UpscaleModelSpan, UpscaleRepeats, Env.getNThreads(SessionId),
      std::string_view(OutputPathSpan.data(), OutputPathSpan.size()),
      OutputBufferSpan, *BytesWritten));
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
