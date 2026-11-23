#include "common/defines.h"
#include "common/filesystem.h"
#include "runtime/instance/module.h"
#include "sd_func.h"
#include "sd_module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

using WasmEdge::Host::StableDiffusion::ErrNo;

namespace {

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

std::unique_ptr<WasmEdge::Host::SDModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(WasmEdge::u8path(
      "../../../plugins/wasmedge_stablediffusion/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeStableDiffusion" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_stablediffusion"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_stablediffusion"sv)) {
      return dynamicPointerCast<WasmEdge::Host::SDModule>(Module->create());
    }
  }
  return {};
}
} // namespace

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   WasmEdge::Span<const T> Binaries, uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(), MemInst.getPointer<T *>(Ptr));
}

void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 uint32_t Value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

void writeFatPointer(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t PtrVal, uint32_t PtrSize, uint32_t &Ptr) {
  writeUInt32(MemInst, PtrVal, Ptr);
  writeUInt32(MemInst, PtrSize, Ptr);
}

// TODO: add unit tests for every function.

TEST(WasmEdgeStableDiffusionTest, ModuleFunctions) {
  // Create the stable diffusion module instance.
  auto SBMod = createModule();
  ASSERT_TRUE(SBMod);
  EXPECT_EQ(SBMod->getFuncExportNum(), 4U);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(2097024)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  uint32_t SessionPtr = UINT32_C(0);
  uint32_t SessionId = UINT32_C(0);
  uint32_t OutputPtr = UINT32_C(0);
  // uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);

  // Get the function "convert".
  auto *FuncInst = SBMod->findFuncExports("convert");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncConvert = FuncInst->getHostFunc();
  // Get the function "create_context".
  FuncInst = SBMod->findFuncExports("create_context");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCreateContext = FuncInst->getHostFunc();
  // Get the function "text_to_image".
  FuncInst = SBMod->findFuncExports("text_to_image");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncTextToImage = FuncInst->getHostFunc();
  // Get the function "image_to_image".
  FuncInst = SBMod->findFuncExports("image_to_image");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncImageToImage = FuncInst->getHostFunc();

  std::string Prompt = "a lovely cat";
  std::string Prompt2 = "with blue eyes";
  std::vector<uint32_t> SkipLayers = {7, 8, 9};
  std::string OutputPathString = "./stableDiffusion/output.png";
  std::vector<char> OutputPath(OutputPathString.begin(),
                               OutputPathString.end());
  std::string InputPathString = "path:" + OutputPathString;
  std::vector<char> InputPath(InputPathString.begin(), InputPathString.end());
  std::string OutputPathString2 = "./stableDiffusion/output2.png";
  std::vector<char> OutputPath2(OutputPathString2.begin(),
                                OutputPathString2.end());
  std::vector<char> PromptData(Prompt.begin(), Prompt.end());
  std::vector<char> PromptData2(Prompt2.begin(), Prompt2.end());
  std::string ModelPathString = "./stableDiffusion/sd-v1-5-fp16.safetensors";
  std::vector<char> ModelPath(ModelPathString.begin(), ModelPathString.end());
  std::string QuantModelPathString = "./stableDiffusion/sd-v1-5-Q8_0.gguf";
  std::vector<char> QuantModelPath(QuantModelPathString.begin(),
                                   QuantModelPathString.end());

  uint32_t ModelPathPtr = UINT32_C(0);
  uint32_t QuantModelPathPtr = ModelPathPtr + ModelPath.size();
  writeBinaries<char>(MemInst, ModelPath, ModelPathPtr);
  writeBinaries<char>(MemInst, QuantModelPath, QuantModelPathPtr);
  // Test: convert -- convert successfully.
  {
    EXPECT_TRUE(HostFuncConvert.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            ModelPathPtr, static_cast<uint32_t>(ModelPath.size()), 0, 0,
            QuantModelPathPtr, static_cast<uint32_t>(QuantModelPath.size()),
            8}, // SD_TYPE_Q8_0    = 8
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_TRUE(std::filesystem::exists(QuantModelPathString));
  }

  // Test: create_context -- create context for text to image.
  {
    EXPECT_TRUE(HostFuncCreateContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            QuantModelPathPtr,                            // ModelPathPtr
            static_cast<uint32_t>(QuantModelPath.size()), // ModelPathLen
            0,                                            // ClipLPathPtr
            0,                                            // ClipLPathLen
            0,                                            // ClipGPathPtr
            0,                                            // ClipGPathLen
            0,                                            // T5xxlPathPtr
            0,                                            // T5xxlPathLen
            0,           // DiffusionModelPathPtr
            0,           // DiffusionModelPathLen
            0,           // VaePathPtr
            0,           // VaePathLen
            0,           // TaesdPathPtr
            0,           // TaesdPathLen
            0,           // ControlNetPathPtr
            0,           // ControlNetPathLen
            0,           // LoraModelDirPtr
            0,           // LoraModelDirLen
            0,           // EmbedDirPtr
            0,           // EmbedDirLen
            0,           // IdEmbedDirPtr
            0,           // IdEmbedDirLen
            1,           // VaeDecodeOnly
            0,           // VaeTiling
            -1,          // NThreads
            36,          // Wtype
            1,           // RngType
            0,           // Schedule
            0,           // ClipOnCpu
            0,           // ControlNetCpu
            0,           // VaeOnCpu
            0,           // DiffusionFlashAttn
            SessionPtr}, // SessiontIdPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    SessionId = *MemInst.getPointer<uint32_t *>(SessionPtr);
    EXPECT_EQ(SessionId, 0);
  }

  // Test: text_to_image -- generate image from text.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t SkipLayersPtr = PromptPtr + PromptData.size();
    uint32_t OutputPathPtr = SkipLayersPtr + SkipLayers.size() * 4;
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData, PromptPtr);
    writeBinaries<uint32_t>(MemInst, SkipLayers, SkipLayersPtr);
    writeBinaries<char>(MemInst, OutputPath, OutputPathPtr);
    EXPECT_TRUE(HostFuncTextToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            PromptPtr,                                // PromptPtr
            static_cast<uint32_t>(PromptData.size()), // PromptLen
            SessionId,                                // SessionId
            0,                                        // ControlImagePtr
            0,                                        // ControlImageLen
            0,                                        // NegativePromptPtr
            0,                                        // NegativePromptLen
            3.5f,                                     // Guidance
            256,                                      // Width
            256,                                      // Height
            -1,                                       // ClipSkip
            7.0f,                                     // CfgScale
            0,                                        // SampleMethod
            1,                                        // SampleSteps
            42,                                       // Seed
            1,                                        // BatchCount
            0.90f,                                    // ControlStrength
            20.0f,                                    // StyleRatio
            0,                                        // NormalizeInput
            0,                                        // InputIdImagesDirPtr
            0,                                        // InputIdImagesDirLen
            0,                                        // CannyPreprocess
            0,                                        // UpscaleModelPathPtr
            0,                                        // UpscaleModelPathLen
            1,                                        // UpscaleRepeats
            SkipLayersPtr,                            // SkipLayersPtr
            static_cast<uint32_t>(SkipLayers.size()), // SkipLayersLen
            0.0f,                                     // SlgScale
            0.01f,                                    // SkipLayerStart
            0.2,                                      // SkipLayerEnd
            OutputPathPtr,                            // OutputPathPtr
            static_cast<uint32_t>(OutputPath.size()), // OutputPathLen
            OutputPtr,                                // OutBufferPtr
            1048512,                                  // OutBufferMaxSize
            BytesWrittenPtr},                         // BytesWrittenPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BytesWrittenPtr);
    EXPECT_GE(BytesWritten, 50);
    EXPECT_TRUE(std::filesystem::exists(OutputPathString));
  }

  // Test: text_to_image -- reuse context to generate image from text.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t SkipLayersPtr = PromptPtr + PromptData.size();
    uint32_t OutputPathPtr = SkipLayersPtr + SkipLayers.size() * 4;
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData, PromptPtr);
    writeBinaries<uint32_t>(MemInst, SkipLayers, SkipLayersPtr);
    writeBinaries<char>(MemInst, OutputPath, OutputPathPtr);
    EXPECT_TRUE(HostFuncTextToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            PromptPtr,                                // PromptPtr
            static_cast<uint32_t>(PromptData.size()), // PromptLen
            SessionId,                                // SessionId
            0,                                        // ControlImagePtr
            0,                                        // ControlImageLen
            0,                                        // NegativePromptPtr
            0,                                        // NegativePromptLen
            3.5f,                                     // Guidance
            256,                                      // Width
            256,                                      // Height
            -1,                                       // ClipSkip
            7.0f,                                     // CfgScale
            0,                                        // SampleMethod
            20,                                       // SampleSteps
            42,                                       // Seed
            1,                                        // BatchCount
            0.90f,                                    // ControlStrength
            20.0f,                                    // StyleRatio
            0,                                        // NormalizeInput
            0,                                        // InputIdImagesDirPtr
            0,                                        // InputIdImagesDirLen
            0,                                        // CannyPreprocess
            0,                                        // UpscaleModelPathPtr
            0,                                        // UpscaleModelPathLen
            1,                                        // UpscaleRepeats
            SkipLayersPtr,                            // SkipLayersPtr
            static_cast<uint32_t>(SkipLayers.size()), // SkipLayersLen
            0.0f,                                     // SlgScale
            0.01f,                                    // SkipLayerStart
            0.2,                                      // SkipLayerEnd
            OutputPathPtr,                            // OutputPathPtr
            static_cast<uint32_t>(OutputPath.size()), // OutputPathLen
            OutputPtr,                                // OutBufferPtr
            1048512,                                  // OutBufferMaxSize
            BytesWrittenPtr},                         // BytesWrittenPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BytesWrittenPtr);
    EXPECT_GE(BytesWritten, 50);
    EXPECT_TRUE(std::filesystem::exists(OutputPathString));
  }
  writeBinaries<char>(MemInst, ModelPath, ModelPathPtr);
  writeBinaries<char>(MemInst, QuantModelPath, QuantModelPathPtr);
  // Test: create_context -- create context for image to image.
  {
    EXPECT_TRUE(HostFuncCreateContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            QuantModelPathPtr,                            // ModelPathPtr
            static_cast<uint32_t>(QuantModelPath.size()), // ModelPathLen
            0,                                            // ClipLPathPtr
            0,                                            // ClipLPathLen
            0,                                            // ClipGPathPtr
            0,                                            // ClipGPathLen
            0,                                            // T5xxlPathPtr
            0,                                            // T5xxlPathLen
            0,           // DiffusionModelPathPtr
            0,           // DiffusionModelPathLen
            0,           // VaePathPtr
            0,           // VaePathLen
            0,           // TaesdPathPtr
            0,           // TaesdPathLen
            0,           // ControlNetPathPtr
            0,           // ControlNetPathLen
            0,           // LoraModelDirPtr
            0,           // LoraModelDirLen
            0,           // EmbedDirPtr
            0,           // EmbedDirLen
            0,           // IdEmbedDirPtr
            0,           // IdEmbedDirLen
            0,           // VaeDecodeOnly
            0,           // VaeTiling
            -1,          // NThreads
            36,          // Wtype
            1,           // RngType
            0,           // Schedule
            0,           // ClipOnCpu
            0,           // ControlNetCpu
            0,           // VaeOnCpu
            0,           // DiffusionFlashAttn
            SessionPtr}, // SessiontIdPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    SessionId = *MemInst.getPointer<uint32_t *>(SessionPtr);
    EXPECT_EQ(SessionId, 1);
  }
  // Test: image_to_image -- generate image from image.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t InputPathPtr = PromptPtr + PromptData2.size();
    uint32_t SkipLayersPtr = InputPathPtr + InputPath.size();
    uint32_t OutputPathPtr = SkipLayersPtr + SkipLayers.size() * 4;
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath2.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData2, PromptPtr);
    writeBinaries<char>(MemInst, InputPath, InputPathPtr);
    writeBinaries<uint32_t>(MemInst, SkipLayers, SkipLayersPtr);
    writeBinaries<char>(MemInst, OutputPath2, OutputPathPtr);
    EXPECT_TRUE(HostFuncImageToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputPathPtr,                              // ImagePtr
            static_cast<uint32_t>(InputPath.size()),   // ImageLen
            0,                                         // MaskInputPtr
            0,                                         // MaskInputLen
            SessionId,                                 // SessionId
            3.5f,                                      // Guidance
            256,                                       // Width
            256,                                       // Height
            0,                                         // ControlImagePtr
            0,                                         // ControlImageLen
            PromptPtr,                                 // PromptPtr
            static_cast<uint32_t>(PromptData2.size()), // PromptLen
            0,                                         // NegativePromptPtr
            0,                                         // NegativePromptLen
            -1,                                        // ClipSkip
            7.0f,                                      // CfgScale
            0,                                         // SampleMethod
            1,                                         // SampleSteps
            0.75f,                                     // Strength
            42,                                        // Seed
            1,                                         // BatchCount
            0.9f,                                      // ControlStrength
            20.0f,                                     // StyleRatio
            0,                                         // NormalizeInput
            0,                                         // InputIdImagesDirPtr
            0,                                         // InputIdImagesDirLen
            0,                                         // CannyPreprocess
            0,                                         // UpscaleModelPathPtr
            0,                                         // UpscaleModelPathLen
            1,                                         // UpscaleRepeats
            SkipLayersPtr,                             // SkipLayersPtr
            static_cast<uint32_t>(SkipLayers.size()),  // SkipLayersLen
            0.0f,                                      // SlgScale
            0.01f,                                     // SkipLayerStart
            0.2,                                       // SkipLayerEnd
            OutputPathPtr,                             // OutputPathPtr
            static_cast<uint32_t>(OutputPath2.size()), // OutputPathLen
            OutputPtr,                                 // OutBufferPtr
            1048512,                                   // OutBufferMaxSize
            BytesWrittenPtr},                          // BytesWrittenPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BytesWrittenPtr);
    EXPECT_GE(BytesWritten, 50);
    EXPECT_TRUE(std::filesystem::exists(OutputPathString2));
  }
  // Test: image_to_image -- reuse context to generate image from image.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t InputPathPtr = PromptPtr + PromptData2.size();
    uint32_t SkipLayersPtr = InputPathPtr + InputPath.size();
    uint32_t OutputPathPtr = SkipLayersPtr + SkipLayers.size() * 4;
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath2.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData2, PromptPtr);
    writeBinaries<char>(MemInst, InputPath, InputPathPtr);
    writeBinaries<uint32_t>(MemInst, SkipLayers, SkipLayersPtr);
    writeBinaries<char>(MemInst, OutputPath2, OutputPathPtr);
    EXPECT_TRUE(HostFuncImageToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputPathPtr,                              // ImagePtr
            static_cast<uint32_t>(InputPath.size()),   // ImageLen
            0,                                         // MaskInputPtr
            0,                                         // MaskInputLen
            SessionId,                                 // SessionId
            3.5f,                                      // Guidance
            256,                                       // Width
            256,                                       // Height
            0,                                         // ControlImagePtr
            0,                                         // ControlImageLen
            PromptPtr,                                 // PromptPtr
            static_cast<uint32_t>(PromptData2.size()), // PromptLen
            0,                                         // NegativePromptPtr
            0,                                         // NegativePromptLen
            -1,                                        // ClipSkip
            7.0f,                                      // CfgScale
            0,                                         // SampleMethod
            20,                                        // SampleSteps
            0.75f,                                     // Strength
            42,                                        // Seed
            1,                                         // BatchCount
            0.9f,                                      // ControlStrength
            20.0f,                                     // StyleRatio
            0,                                         // NormalizeInput
            0,                                         // InputIdImagesDirPtr
            0,                                         // InputIdImagesDirLen
            0,                                         // CannyPreprocess
            0,                                         // UpscaleModelPathPtr
            0,                                         // UpscaleModelPathLen
            1,                                         // UpscaleRepeats
            SkipLayersPtr,                             // SkipLayersPtr
            static_cast<uint32_t>(SkipLayers.size()),  // SkipLayersLen
            0.0f,                                      // SlgScale
            0.01f,                                     // SkipLayerStart
            0.2,                                       // SkipLayerEnd
            OutputPathPtr,                             // OutputPathPtr
            static_cast<uint32_t>(OutputPath2.size()), // OutputPathLen
            OutputPtr,                                 // OutBufferPtr
            1048512,                                   // OutBufferMaxSize
            BytesWrittenPtr},                          // BytesWrittenPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BytesWrittenPtr);
    EXPECT_GE(BytesWritten, 50);
    EXPECT_TRUE(std::filesystem::exists(OutputPathString2));
  }

  // Test: text_to_image -- non exist SessionId.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t SkipLayersPtr = PromptPtr + PromptData.size();
    uint32_t OutputPathPtr = SkipLayersPtr + SkipLayers.size() * 4;
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData, PromptPtr);
    writeBinaries<uint32_t>(MemInst, SkipLayers, SkipLayersPtr);
    writeBinaries<char>(MemInst, OutputPath, OutputPathPtr);
    EXPECT_TRUE(HostFuncTextToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            PromptPtr,                                // PromptPtr
            static_cast<uint32_t>(PromptData.size()), // PromptLen
            99,                                       // SessionId
            0,                                        // ControlImagePtr
            0,                                        // ControlImageLen
            0,                                        // NegativePromptPtr
            0,                                        // NegativePromptLen
            3.5f,                                     // Guidance
            256,                                      // Width
            256,                                      // Height
            -1,                                       // ClipSkip
            7.0f,                                     // CfgScale
            0,                                        // SampleMethod
            1,                                        // SampleSteps
            42,                                       // Seed
            1,                                        // BatchCount
            0.90f,                                    // ControlStrength
            20.0f,                                    // StyleRatio
            0,                                        // NormalizeInput
            0,                                        // InputIdImagesDirPtr
            0,                                        // InputIdImagesDirLen
            0,                                        // CannyPreprocess
            0,                                        // UpscaleModelPathPtr
            0,                                        // UpscaleModelPathLen
            1,                                        // UpscaleRepeats
            SkipLayersPtr,                            // SkipLayersPtr
            static_cast<uint32_t>(SkipLayers.size()), // SkipLayersLen
            0.0f,                                     // SlgScale
            0.01f,                                    // SkipLayerStart
            0.2,                                      // SkipLayerEnd
            OutputPathPtr,                            // OutputPathPtr
            static_cast<uint32_t>(OutputPath.size()), // OutputPathLen
            OutputPtr,                                // OutBufferPtr
            1048512,                                  // OutBufferMaxSize
            BytesWrittenPtr},                         // BytesWrittenPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: image_to_image -- non exist SessionId.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t InputPathPtr = PromptPtr + PromptData2.size();
    uint32_t SkipLayersPtr = InputPathPtr + InputPath.size();
    uint32_t OutputPathPtr = SkipLayersPtr + SkipLayers.size() * 4;
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath2.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData2, PromptPtr);
    writeBinaries<char>(MemInst, InputPath, InputPathPtr);
    writeBinaries<uint32_t>(MemInst, SkipLayers, SkipLayersPtr);
    writeBinaries<char>(MemInst, OutputPath2, OutputPathPtr);
    EXPECT_TRUE(HostFuncImageToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputPathPtr,                              // ImagePtr
            static_cast<uint32_t>(InputPath.size()),   // ImageLen
            0,                                         // MaskInputPtr
            0,                                         // MaskInputLen
            -1,                                        // SessionId
            3.5f,                                      // Guidance
            256,                                       // Width
            256,                                       // Height
            0,                                         // ControlImagePtr
            0,                                         // ControlImageLen
            PromptPtr,                                 // PromptPtr
            static_cast<uint32_t>(PromptData2.size()), // PromptLen
            0,                                         // NegativePromptPtr
            0,                                         // NegativePromptLen
            -1,                                        // ClipSkip
            7.0f,                                      // CfgScale
            0,                                         // SampleMethod
            20,                                        // SampleSteps
            0.75f,                                     // Strength
            42,                                        // Seed
            1,                                         // BatchCount
            0.9f,                                      // ControlStrength
            20.0f,                                     // StyleRatio
            0,                                         // NormalizeInput
            0,                                         // InputIdImagesDirPtr
            0,                                         // InputIdImagesDirLen
            0,                                         // CannyPreprocess
            0,                                         // UpscaleModelPathPtr
            0,                                         // UpscaleModelPathLen
            1,                                         // UpscaleRepeats
            SkipLayersPtr,                             // SkipLayersPtr
            static_cast<uint32_t>(SkipLayers.size()),  // SkipLayersLen
            0.0f,                                      // SlgScale
            0.01f,                                     // SkipLayerStart
            0.2,                                       // SkipLayerEnd
            OutputPathPtr,                             // OutputPathPtr
            static_cast<uint32_t>(OutputPath2.size()), // OutputPathLen
            OutputPtr,                                 // OutBufferPtr
            1048512,                                   // OutBufferMaxSize
            BytesWrittenPtr},                          // BytesWrittenPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
}

using WasmEdge::Span;
using namespace std::literals::string_view_literals;

namespace {

// Calls text_to_image and image_to_image with one sampling step, and reads
// the results back from guest memory.
class SDGenerator {
public:
  void setUp() {
    SBMod = createModule();
    ASSERT_TRUE(SBMod);
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(256)));
    MemInst = Mod.findMemoryExports("memory");
    ASSERT_NE(MemInst, nullptr);

    const std::string ModelPath = "./stableDiffusion/sd-v1-5-fp16.safetensors";
    writeString(ModelPath, ImagePtr);
    ASSERT_EQ(call("create_context", {ImagePtr,
                                      static_cast<uint32_t>(ModelPath.size()),
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      UINT32_C(0),  // VaeDecodeOnly
                                      UINT32_C(0),  // VaeTiling
                                      INT32_C(-1),  // NThreads
                                      UINT32_C(36), // Wtype
                                      UINT32_C(1),  // RngType
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      BytesWrittenPtr}),
              ErrNo::Success);
    SessionId = *MemInst->getPointer<uint32_t *>(BytesWrittenPtr);
  }

  ErrNo textToImage(uint32_t Width, uint32_t Height, uint32_t BatchCount,
                    std::string_view OutputPath, uint32_t OutCap) {
    writeString(Prompt, PromptPtr);
    writeString(OutputPath, OutputPathPtr);
    return call("text_to_image", {PromptPtr,
                                  static_cast<uint32_t>(Prompt.size()),
                                  SessionId,
                                  0,
                                  0,
                                  0,
                                  0,
                                  3.5f,
                                  Width,
                                  Height,
                                  INT32_C(-1),
                                  7.0f,
                                  0,
                                  UINT32_C(1),
                                  UINT32_C(42),
                                  BatchCount,
                                  0.9f,
                                  20.0f,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  UINT32_C(1),
                                  0,
                                  0,
                                  0.0f,
                                  0.01f,
                                  0.2f,
                                  OutputPathPtr,
                                  static_cast<uint32_t>(OutputPath.size()),
                                  OutBufferPtr,
                                  OutCap,
                                  BytesWrittenPtr});
  }

  ErrNo imageToImage(Span<const uint8_t> Image, uint32_t Width,
                     uint32_t Height) {
    writeString(Prompt, PromptPtr);
    std::copy(Image.begin(), Image.end(),
              MemInst->getPointer<uint8_t *>(ImagePtr));
    return call("image_to_image", {ImagePtr,
                                   static_cast<uint32_t>(Image.size()),
                                   0,
                                   0,
                                   SessionId,
                                   3.5f,
                                   Width,
                                   Height,
                                   0,
                                   0,
                                   PromptPtr,
                                   static_cast<uint32_t>(Prompt.size()),
                                   0,
                                   0,
                                   INT32_C(-1),
                                   7.0f,
                                   0,
                                   UINT32_C(1),
                                   0.75f,
                                   UINT32_C(42),
                                   UINT32_C(1),
                                   0.9f,
                                   20.0f,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   UINT32_C(1),
                                   0,
                                   0,
                                   0.0f,
                                   0.01f,
                                   0.2f,
                                   0,
                                   0,
                                   OutBufferPtr,
                                   OutBufferCap,
                                   BytesWrittenPtr});
  }

  uint32_t bytesWritten() {
    return *MemInst->getPointer<uint32_t *>(BytesWrittenPtr);
  }

  std::vector<uint8_t> output() {
    const auto *Begin = MemInst->getPointer<const uint8_t *>(OutBufferPtr);
    return std::vector<uint8_t>(Begin, Begin + bytesWritten());
  }

  static constexpr uint32_t OutBufferCap = UINT32_C(4) << 20;

private:
  ErrNo call(std::string_view Name,
             std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *FuncInst = SBMod->findFuncExports(Name);
    if (FuncInst == nullptr) {
      ADD_FAILURE() << "missing host function " << Name;
      return static_cast<ErrNo>(UINT32_MAX);
    }
    std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};
    if (!FuncInst->getHostFunc().run(CallFrame, Args, Errno)) {
      ADD_FAILURE() << "host function " << Name << " failed";
      return static_cast<ErrNo>(UINT32_MAX);
    }
    return static_cast<ErrNo>(Errno[0].get<uint32_t>());
  }

  void writeString(std::string_view Str, uint32_t Ptr) {
    std::copy(Str.begin(), Str.end(), MemInst->getPointer<char *>(Ptr));
  }

  static constexpr uint32_t PromptPtr = UINT32_C(0);
  static constexpr uint32_t OutputPathPtr = UINT32_C(256);
  static constexpr uint32_t BytesWrittenPtr = UINT32_C(1024);
  static constexpr uint32_t ImagePtr = UINT32_C(4096);
  static constexpr uint32_t OutBufferPtr = UINT32_C(8) << 20;
  static constexpr std::string_view Prompt = "a lovely cat";

  std::unique_ptr<WasmEdge::Host::SDModule> SBMod;
  WasmEdge::Runtime::Instance::ModuleInstance Mod{""};
  WasmEdge::Runtime::Instance::MemoryInstance *MemInst = nullptr;
  WasmEdge::Runtime::CallingFrame CallFrame{nullptr, &Mod};
  uint32_t SessionId = 0;
};

// Reads the width and height from the IHDR chunk of a PNG stream.
bool readPngSize(Span<const uint8_t> Png, uint32_t &Width, uint32_t &Height) {
  constexpr std::array<uint8_t, 8> Magic = {0x89U, 0x50U, 0x4EU, 0x47U,
                                            0x0DU, 0x0AU, 0x1AU, 0x0AU};
  if (Png.size() < 24 || !std::equal(Magic.begin(), Magic.end(), Png.begin())) {
    return false;
  }
  auto ReadBE = [&Png](size_t Offset) {
    return (static_cast<uint32_t>(Png[Offset]) << 24) |
           (static_cast<uint32_t>(Png[Offset + 1]) << 16) |
           (static_cast<uint32_t>(Png[Offset + 2]) << 8) |
           static_cast<uint32_t>(Png[Offset + 3]);
  };
  Width = ReadBE(16);
  Height = ReadBE(20);
  return true;
}

std::vector<uint8_t> readFile(const std::filesystem::path &Path) {
  std::ifstream File(Path, std::ios::binary);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(File),
                              std::istreambuf_iterator<char>());
}

void expectPngSize(Span<const uint8_t> Png, uint32_t Width, uint32_t Height) {
  uint32_t PngWidth = 0;
  uint32_t PngHeight = 0;
  ASSERT_TRUE(readPngSize(Png, PngWidth, PngHeight));
  EXPECT_EQ(PngWidth, Width);
  EXPECT_EQ(PngHeight, Height);
}

} // namespace

TEST(WasmEdgeStableDiffusionTest, ResultsAndInputs) {
  SDGenerator Gen;
  ASSERT_NO_FATAL_FAILURE(Gen.setUp());
  const std::filesystem::path Dir = "./stableDiffusion/results";
  std::filesystem::remove_all(Dir);
  std::filesystem::create_directories(Dir);

  // Every image of a batch is written next to the first one, and the guest
  // buffer holds the PNG of the first image.
  const std::string BatchPath = (Dir / "batch.png").string();
  ASSERT_EQ(Gen.textToImage(256, 256, 3, BatchPath, Gen.OutBufferCap),
            ErrNo::Success);
  const std::vector<uint8_t> First = Gen.output();
  expectPngSize(First, 256, 256);
  for (const char *Name : {"batch_2.png", "batch_3.png"}) {
    SCOPED_TRACE(Name);
    expectPngSize(readFile(Dir / Name), 256, 256);
  }

  // An input image of another size is resized, whether it is given as bytes
  // or as a path.
  const std::vector<uint8_t> Input = readFile(Dir / "batch_2.png");
  ASSERT_EQ(Gen.imageToImage(Input, 128, 128), ErrNo::Success);
  expectPngSize(Gen.output(), 128, 128);
  const std::string InputPath = "path:" + (Dir / "batch_2.png").string();
  ASSERT_EQ(
      Gen.imageToImage(Span<const uint8_t>(
                           reinterpret_cast<const uint8_t *>(InputPath.data()),
                           InputPath.size()),
                       128, 128),
      ErrNo::Success);
  expectPngSize(Gen.output(), 128, 128);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
