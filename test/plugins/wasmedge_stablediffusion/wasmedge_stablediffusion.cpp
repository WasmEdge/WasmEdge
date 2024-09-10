#include "common/defines.h"
#include "runtime/instance/module.h"
#include "sd_func.h"
#include "sd_module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
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
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
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

// TODO: unit tests for every functions.

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncConvert =
      dynamic_cast<WasmEdge::Host::StableDiffusion::SDConvert &>(
          FuncInst->getHostFunc());
  // Get the function "create_context".
  FuncInst = SBMod->findFuncExports("create_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCreateContext =
      dynamic_cast<WasmEdge::Host::StableDiffusion::SDCreateContext &>(
          FuncInst->getHostFunc());
  // Get the function "text_to_image".
  FuncInst = SBMod->findFuncExports("text_to_image");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncTextToImage =
      dynamic_cast<WasmEdge::Host::StableDiffusion::SDTextToImage &>(
          FuncInst->getHostFunc());
  // Get the function "image_to_image".
  FuncInst = SBMod->findFuncExports("image_to_image");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncImageToImage =
      dynamic_cast<WasmEdge::Host::StableDiffusion::SDImageToImage &>(
          FuncInst->getHostFunc());

  std::string Prompt = "a lovely cat";
  std::string Prompt2 = "with blue eyes";
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
  std::string ModelPathString = "./stableDiffusion/sd-v1-4.ckpt";
  std::vector<char> ModelPath(ModelPathString.begin(), ModelPathString.end());
  std::string QuantModelPathString = "./stableDiffusion/sd-v1-4-Q8_0.gguf";
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
            34,          // Wtype
            1,           // RngType
            0,           // Schedule
            0,           // ClipOnCpu
            0,           // ControlNetCpu
            0,           // VaeOnCpu
            SessionPtr}, // SessiontIdPtr
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    SessionId = *MemInst.getPointer<uint32_t *>(SessionPtr);
    EXPECT_EQ(SessionId, 0);
  }

  // Test: text_to_image -- generate image from text.
  {
    uint32_t PromptPtr = UINT32_C(0);
    uint32_t OutputPathPtr = PromptPtr + PromptData.size();
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData, PromptPtr);
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
            34,          // Wtype
            1,           // RngType
            0,           // Schedule
            0,           // ClipOnCpu
            0,           // ControlNetCpu
            0,           // VaeOnCpu
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
    uint32_t OutputPathPtr = InputPathPtr + InputPath.size();
    uint32_t BytesWrittenPtr = OutputPathPtr + OutputPath2.size();
    OutputPtr = BytesWrittenPtr + 4;
    writeBinaries<char>(MemInst, PromptData2, PromptPtr);
    writeBinaries<char>(MemInst, InputPath, InputPathPtr);
    writeBinaries<char>(MemInst, OutputPath2, OutputPathPtr);
    EXPECT_TRUE(HostFuncImageToImage.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputPathPtr,                              // ImagePtr
            static_cast<uint32_t>(InputPath.size()),   // ImageLen
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
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
