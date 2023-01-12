// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/types.h"
#include "runtime/instance/module.h"
#include "wasinnfunc.h"
#include "wasinnmodule.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <gtest/gtest.h>
#include <numeric>
#include <vector>

using WasmEdge::Host::WASINN::Backend;
using WasmEdge::Host::WASINN::ErrNo;

#if defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO) ||                       \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH) ||                          \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)
namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasi_nn/"
      "libwasmedgePluginWasiNN" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_nn"sv)) {
    if (const auto *Module = Plugin->findModule("wasi_nn"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

inline std::vector<uint8_t> readEntireFile(const std::string &Path) {
  std::ifstream Fin(Path, std::ios::binary | std::ios::ate);
  if (!Fin) {
    return {};
  }
  Fin.seekg(0, std::ios::end);
  std::vector<uint8_t> Buf(static_cast<uint32_t>(Fin.tellg()));
  Fin.seekg(0, std::ios::beg);
  if (!Fin.read(reinterpret_cast<char *>(Buf.data()),
                static_cast<uint32_t>(Buf.size()))) {
    return {};
  }
  Fin.close();
  return Buf;
}

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   std::vector<T> Binaries, uint32_t Ptr) noexcept {
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

template <typename T>
std::vector<size_t> classSort(const std::vector<T> &Array) {
  std::vector<size_t> Indices(Array.size());
  std::iota(Indices.begin(), Indices.end(), 0);
  std::sort(Indices.begin(), Indices.end(),
            [&Array](size_t Left, size_t Right) -> bool {
              // Sort indices according to corresponding array element.
              return Array[Left] > Array[Right];
            });
  return Indices;
}
} // namespace
#endif

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
TEST(WasiNNTest, OpenVINOBackend) {
  // Create the wasmedge_process module instance.
  auto *NNMod = dynamic_cast<WasmEdge::Host::WasiNNModule *>(createModule());
  ASSERT_TRUE(NNMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(400)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Load the files.
  std::vector<uint8_t> TensorData =
      readEntireFile("./wasinn_openvino_fixtures/tensor-1x224x224x3-f32.bgr");
  std::vector<uint8_t> XmlRead =
      readEntireFile("./wasinn_openvino_fixtures/mobilenet.xml");
  std::vector<uint8_t> WeightRead =
      readEntireFile("./wasinn_openvino_fixtures/mobilenet.bin");

  std::vector<uint32_t> TensorDim{1, 3, 224, 224};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(410 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Temp. values.
  std::vector<WasmEdge::Host::WASINN::Graph> NNGraphTmp;
  std::vector<WasmEdge::Host::WASINN::Context> NNContextTmp;

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad =
      dynamic_cast<WasmEdge::Host::WasiNNLoad &>(FuncInst->getHostFunc());
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = dynamic_cast<WasmEdge::Host::WasiNNInitExecCtx &>(
      FuncInst->getHostFunc());
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput =
      dynamic_cast<WasmEdge::Host::WasiNNSetInput &>(FuncInst->getHostFunc());
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput =
      dynamic_cast<WasmEdge::Host::WasiNNGetOuput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

  // OpenVINO WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Busy));
  }

  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- OpenVINO model xml ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr, XmlRead.size(), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + XmlRead.size(), WeightRead.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- OpenVINO model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, XmlRead.size(), BuilderPtr);
  writeFatPointer(MemInst, OutBoundPtr, WeightRead.size(), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong builders' length.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, XmlRead.size(), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + XmlRead.size(), WeightRead.size(),
                  BuilderPtr);
  writeBinaries<uint8_t>(MemInst, XmlRead, StorePtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr + XmlRead.size());
  StorePtr += (XmlRead.size() + WeightRead.size());
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(4), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- unsupported device. CPU 0, GPU 1, TPU 2
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(3), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- load successfully.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: load -- load second graph.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // OpenVINO WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Swap to the tmp. env.
  NNGraphTmp.emplace_back(Backend::OpenVINO);
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);
  // Test: init_execution_context -- graph id exceeds.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::MissingMemory));
  }
  // Swap back.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: init_execution_context -- init context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- init second context.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // OpenVINO WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Swap to the tmp. env.
  NNContextTmp.emplace_back(NNGraphTmp[0]);
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);
  // Test: set_input -- context id exceeds.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(3), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: set_input -- empty context.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::MissingMemory));
  }
  // Swap back.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: set_input -- input index exceeds.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(10), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: set_input -- tensor type not FP32.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(2), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // OpenVINO WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Swap to the tmp. env.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);
  // Test: compute -- empty context.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Busy));
  }
  // Swap back.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // OpenVINO WASI-NN get_output tests.
  // Test: get_output -- output bytes ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- output buffer ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), OutBoundPtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- output index exceeds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(10), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(1), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), UINT32_C(4004));
    std::vector<float> OutputClassification(
        MemInst.getPointer<float *>(StorePtr, 1001) + 1,
        MemInst.getPointer<float *>(StorePtr, 1001) + 1001);
    std::vector<size_t> SortedIndex, CorrectClasses{963, 762, 909, 926, 567};
    SortedIndex = classSort<float>(OutputClassification);
    // The probability of class i is placed at buffer[i].
    for (size_t I = 0; I < CorrectClasses.size(); I++) {
      EXPECT_EQ(SortedIndex[I], CorrectClasses[I]);
    }
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
TEST(WasiNNTest, PyTorchBackend) {
  // Create the wasmedge_process module instance.
  auto *NNMod = dynamic_cast<WasmEdge::Host::WasiNNModule *>(createModule());
  EXPECT_FALSE(NNMod == nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(400)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Load the files.
  std::vector<uint8_t> TensorData =
      readEntireFile("./wasinn_pytorch_fixtures/image-1x3x224x224.rgb");
  std::vector<uint8_t> WeightRead =
      readEntireFile("./wasinn_pytorch_fixtures/mobilenet.pt");

  std::vector<uint32_t> TensorDim{1, 3, 224, 224};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(410 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Temp. values.
  std::vector<WasmEdge::Host::WASINN::Graph> NNGraphTmp;
  std::vector<WasmEdge::Host::WASINN::Context> NNContextTmp;

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad =
      dynamic_cast<WasmEdge::Host::WasiNNLoad &>(FuncInst->getHostFunc());
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = dynamic_cast<WasmEdge::Host::WasiNNInitExecCtx &>(
      FuncInst->getHostFunc());
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput =
      dynamic_cast<WasmEdge::Host::WasiNNSetInput &>(FuncInst->getHostFunc());
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput =
      dynamic_cast<WasmEdge::Host::WasiNNGetOuput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

  // Torch WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), OutBoundPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     OutBoundPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- Torch model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr, WeightRead.size(), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong builders' length.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, WeightRead.size(), BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += WeightRead.size();
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(2),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- unsupported device. CPU 0, GPU 1, TPU 2
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(3), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- load successfully.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: load -- load second graph.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::PyTorch),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // Torch WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Swap to the tmp. env.
  NNGraphTmp.emplace_back(Backend::PyTorch);
  // Test: init_execution_context -- graph id exceeds.
  // TODO: not null test for pytorch now
  //   NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  //   NNContextTmp.swap(NNMod->getEnv().NNContext);
  //   {
  //     EXPECT_TRUE(HostFuncInit.run(
  //         CallFrame,
  //         std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0),
  //         BuilderPtr}, Errno));
  //     EXPECT_EQ(Errno[0].get<int32_t>(),
  //               static_cast<uint32_t>(ErrNo::MissingMemory));
  //   }
  // Swap back.
  //   NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  //   NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: init_execution_context -- init context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- init second context.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // Torch WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Test: set_input -- context id exceeds.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(3), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  NNContextTmp.emplace_back(NNGraphTmp[0]);

  // Test: set_input -- tensor type not FP32.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(2), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // Torch WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Swap to the tmp. env.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);
  // Test: compute -- empty context.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Swap back.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Torch WASI-NN get_output tests.
  // Test: get_output -- output bytes ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- output buffer ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), OutBoundPtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- output index exceeds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(10), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(1), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), UINT32_C(4000));
    std::vector<float> OutputClassification(
        MemInst.getPointer<float *>(StorePtr, 1000),
        MemInst.getPointer<float *>(StorePtr, 1000) + 1000);
    std::vector<size_t> SortedIndex, CorrectClasses{954, 940, 951, 950, 953};
    SortedIndex = classSort<float>(OutputClassification);
    // The probability of class i is placed at buffer[i].
    for (size_t I = 0; I < CorrectClasses.size(); I++) {
      EXPECT_EQ(SortedIndex[I], CorrectClasses[I]);
    }
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
TEST(WasiNNTest, TFLiteBackend) {
  // Create the wasmedge_process module instance.
  auto *NNMod = dynamic_cast<WasmEdge::Host::WasiNNModule *>(createModule());
  EXPECT_FALSE(NNMod == nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(400)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Load the files.
  std::vector<uint8_t> TensorData =
      readEntireFile("./wasinn_tflite_fixtures/birdx224x224x3.rgb");
  std::vector<uint8_t> WeightRead =
      readEntireFile("./wasinn_tflite_fixtures/"
                     "lite-model_aiy_vision_classifier_birds_V1_3.tflite");
  spdlog::info("Read {}", TensorData.size());
  std::vector<uint32_t> TensorDim{1, 3, 224, 224};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(410 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Temp. values.
  std::vector<WasmEdge::Host::WASINN::Graph> NNGraphTmp;
  std::vector<WasmEdge::Host::WASINN::Context> NNContextTmp;

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad =
      dynamic_cast<WasmEdge::Host::WasiNNLoad &>(FuncInst->getHostFunc());
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = dynamic_cast<WasmEdge::Host::WasiNNInitExecCtx &>(
      FuncInst->getHostFunc());
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput =
      dynamic_cast<WasmEdge::Host::WasiNNSetInput &>(FuncInst->getHostFunc());
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput =
      dynamic_cast<WasmEdge::Host::WasiNNGetOuput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

  // Torch WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), OutBoundPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             OutBoundPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr, WeightRead.size(), BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong builders' length.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, WeightRead.size(), BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += WeightRead.size();
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(2),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- unsupported device. CPU 0, GPU 1, TPU 2
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(3), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- load successfully.
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: load -- load second graph.
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             UINT32_C(0), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Swap to the tmp. env.
  // Test: init_execution_context -- graph id exceeds.
  NNGraphTmp.emplace_back(Backend::TensorflowLite);
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::MissingMemory));
  }
  // Swap back.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: init_execution_context -- init context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- init second context.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // Torch WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Test: set_input -- context id exceeds.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(3), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  NNContextTmp.emplace_back(NNGraphTmp[0]);

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  // Tensor type U8
  writeUInt32(MemInst, UINT32_C(2), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // Torch WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Swap to the tmp. env.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);
  // Test: compute -- empty context.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::MissingMemory));
  }
  // Swap back.
  NNGraphTmp.swap(NNMod->getEnv().NNGraph);
  NNContextTmp.swap(NNMod->getEnv().NNContext);

  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  // WASI-NN get_output tests.
  // Test: get_output -- output bytes ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- output buffer ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), OutBoundPtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- output index exceeds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(10), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(1), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), UINT32_C(965));
    std::vector<uint8_t> OutputClassification(
        MemInst.getPointer<uint8_t *>(StorePtr, 965),
        MemInst.getPointer<uint8_t *>(StorePtr, 965) + 965);
    std::vector<size_t> SortedIndex, CorrectClasses{166, 158, 34, 778, 819};
    // FIXME: classSort causing segmentation fault
    SortedIndex = classSort<uint8_t>(OutputClassification);

    // The probability of class i is placed at buffer[i].
    for (size_t I = 0; I < CorrectClasses.size(); I++) {
      EXPECT_EQ(OutputClassification[SortedIndex[I]],
                OutputClassification[CorrectClasses[I]]);
    }
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE