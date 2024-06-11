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
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_stablediffusion/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginStableDiffusion" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_stablediffusion"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_stablediffusion"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
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
  auto *SBMod = dynamic_cast<WasmEdge::Host::SDModule *>(createModule());
  EXPECT_FALSE(SBMod == nullptr);
  EXPECT_EQ(SBMod->getFuncExportNum(), 3U);
  EXPECT_NE(SBMod->findFuncExports("create_context"), nullptr);
  EXPECT_NE(SBMod->findFuncExports("text_to_image"), nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);

  // Get the function "convert".
  auto *FuncInst = SBMod->findFuncExports("convert");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncConvert =
      dynamic_cast<WasmEdge::Host::StableDiffusion::SDConvert &>(
          FuncInst->getHostFunc());
  std::string Prompt = "a lovely cat";
  std::vector<char> TensorData(Prompt.begin(), Prompt.end());
  std::string ModelPathString = "./stableDiffusion/sd-v1-4.ckpt";
  std::vector<char> ModelPath(ModelPathString.begin(), ModelPathString.end());
  std::string QuantModelPathString = "./stableDiffusion/sd-v1-4-Q4_K.gguf";
  std::vector<char> QuantModelPath(QuantModelPathString.begin(),
                                   QuantModelPathString.end());

  // Test: convert -- convert successfully.
  {
    uint32_t ModelPathPtr = UINT32_C(0);
    writeBinaries<char>(MemInst, ModelPath, ModelPathPtr);
    uint32_t QuantModelPathPtr = ModelPathPtr + ModelPath.size();
    writeBinaries<char>(MemInst, QuantModelPath, QuantModelPathPtr);
    EXPECT_TRUE(HostFuncConvert.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            ModelPathPtr, static_cast<uint32_t>(ModelPath.size()), 0, 0,
            QuantModelPathPtr, static_cast<uint32_t>(QuantModelPath.size()),
            12}, // SD_TYPE_Q4_K    = 12
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    std::ifstream Fin(QuantModelPath.data(),
                      std::ios::in | std::ios::binary | std::ios::ate);
    EXPECT_FALSE(Fin.fail());
    Fin.close();
  }

  delete SBMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
