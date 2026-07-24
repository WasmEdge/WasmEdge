// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasinn_mlx.h"
#include "wasinnfunc.h"
#include "wasinnmodule.h"

#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
#include "driver/wasi_nn_rpc/wasi_nn_rpcserver/wasi_nn_rpcserver.h"
#endif

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;
using WasmEdge::Host::WASINN::Backend;
using WasmEdge::Host::WASINN::Device;
using WasmEdge::Host::WASINN::ErrNo;
using WasmEdge::Host::WASINN::TensorType;

#if defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO) ||                       \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH) ||                          \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE) ||                         \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML) ||                           \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER) ||                          \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER) ||                        \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS) ||                        \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX) ||                            \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET)
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

std::unique_ptr<WasmEdge::Host::WasiNNModule>
createModule(std::string_view NNRPCURI = "") {
  WasmEdge::Plugin::Plugin::load(
      std::filesystem::u8path("../../../plugins/wasi_nn/" WASMEDGE_LIB_PREFIX
                              "wasmedgePluginWasiNN" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_nn"sv)) {
    WasmEdge::PO::ArgumentParser Parser;
    Plugin->registerOptions(Parser);
    if (NNRPCURI != "") {
      Parser.set_raw_value<std::string>("nn-rpc-uri"sv, std::string(NNRPCURI));
    }
    if (const auto *Module = Plugin->findModule("wasi_nn"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasiNNModule>(Module->create());
    }
  }
  return {};
}

inline std::vector<uint8_t> readEntireFile
    [[maybe_unused]] (const std::string &Path) {
  std::ifstream Fin(Path, std::ios::in | std::ios::binary | std::ios::ate);
  if (!Fin) {
    return {};
  }
  std::vector<uint8_t> Buf(static_cast<std::size_t>(Fin.tellg()));
  Fin.seekg(0, std::ios::beg);
  if (!Fin.read(reinterpret_cast<char *>(Buf.data()),
                static_cast<std::streamsize>(Buf.size()))) {
    return {};
  }
  Fin.close();
  return Buf;
}

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   WasmEdge::Span<const T> Binaries, uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(), MemInst.getPointer<T *>(Ptr));
}

void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 uint32_t Value, uint32_t &Ptr) {
  MemInst.storeValue(Value, Ptr);
  Ptr += 4;
}

void writeFatPointer(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t PtrVal, uint32_t PtrSize, uint32_t &Ptr) {
  writeUInt32(MemInst, PtrVal, Ptr);
  writeUInt32(MemInst, PtrSize, Ptr);
}

#if defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO) ||                       \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH) ||                          \
    defined(WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)
template <typename T>
std::vector<size_t> classSort(WasmEdge::Span<const T> Array) {
  std::vector<size_t> Indices(Array.size());
  std::iota(Indices.begin(), Indices.end(), 0);
  std::sort(Indices.begin(), Indices.end(),
            [&Array](size_t Left, size_t Right) -> bool {
              // Sort indices according to the corresponding array elements.
              return Array[Left] > Array[Right];
            });
  return Indices;
}
#endif
} // namespace
#endif

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
TEST(WasiNNTest, OpenVINOBackend) {
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

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
  std::vector<uint8_t> TensorDataLegecy =
      readEntireFile("./wasinn_openvino_fixtures/tensor-1x224x224x3-f32.bgr");
  std::vector<uint8_t> XmlRead =
      readEntireFile("./wasinn_openvino_fixtures/mobilenet.xml");
  std::vector<uint8_t> WeightRead =
      readEntireFile("./wasinn_openvino_fixtures/mobilenet.bin");

  // Convert the NHWC to NCHW format.
  // For historical reasons, the OpenVINO model expects the input tensor in
  // NCHW format, while the input tensor is in NHWC format.
  // https://github.com/intel/openvino-rs/blob/v0.3.3/crates/openvino/tests/fixtures/mobilenet/build.sh#L39
  // https://github.com/intel/openvino-rs/blob/v0.8.0/crates/openvino/tests/classify-mobilenet.rs#L34
  ASSERT_EQ(TensorDataLegecy.size(), 3 * 224 * 224 * 4);
  std::vector<uint8_t> TensorData(TensorDataLegecy.size());

  for (size_t C = 0; C < 3; ++C) {
    for (size_t H = 0; H < 224; ++H) {
      for (size_t W = 0; W < 224; ++W) {
        size_t Loc = H * 224 + W;
        for (size_t B = 0; B < 4; ++B) {
          TensorData[(C * 224 * 224 + Loc) * 4 + B] =
              TensorDataLegecy[(3 * Loc + C) * 4 + B];
        }
      }
    }
  }

  std::vector<uint32_t> TensorDim{1, 3, 224, 224};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(410 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Temp. values.
  WasmEdge::Host::WASINN::ResourceTable<WasmEdge::Host::WASINN::Graph>
      NNGraphTmp;
  WasmEdge::Host::WASINN::ResourceTable<WasmEdge::Host::WASINN::Context>
      NNContextTmp;
  std::shared_ptr<WasmEdge::Host::WASINN::Graph> TmpGraph;

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // OpenVINO WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::RuntimeError));
  }

  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- OpenVINO model xml ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr, static_cast<uint32_t>(XmlRead.size()),
                  BuilderPtr);
  writeFatPointer(MemInst, StorePtr + static_cast<uint32_t>(XmlRead.size()),
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- OpenVINO model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(XmlRead.size()),
                  BuilderPtr);
  writeFatPointer(MemInst, OutBoundPtr,
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong builder count.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(XmlRead.size()),
                  BuilderPtr);
  writeFatPointer(MemInst, StorePtr + static_cast<uint32_t>(XmlRead.size()),
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  writeBinaries<uint8_t>(MemInst, XmlRead, StorePtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr + XmlRead.size());
  StorePtr += (XmlRead.size() + WeightRead.size());
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(4), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- unsupported device. CPU 0, GPU 1, TPU 2
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::AUTO), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- load successfully.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::OpenVINO),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
  TmpGraph = std::make_shared<WasmEdge::Host::WASINN::Graph>(Backend::OpenVINO);
  NNGraphTmp.insert(TmpGraph);
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

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- initialize the second context.
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Swap to the tmp. env.
  NNContextTmp.insert(
      std::make_shared<WasmEdge::Host::WASINN::Context>(0, TmpGraph));
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(2), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::RuntimeError));
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
    const auto OutputClassification =
        MemInst.getSpan<const float>(StorePtr, 1001).subspan(1);
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
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

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
  WasmEdge::Host::WASINN::ResourceTable<WasmEdge::Host::WASINN::Graph>
      NNGraphTmp;
  WasmEdge::Host::WASINN::ResourceTable<WasmEdge::Host::WASINN::Context>
      NNContextTmp;
  std::shared_ptr<WasmEdge::Host::WASINN::Graph> TmpGraph;

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // Torch WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- Torch model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr,
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong builder count.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(WeightRead.size()),
                  BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += WeightRead.size();
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- unsupported device. CPU 0, GPU 1, TPU 2
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::AUTO), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- load successfully.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::PyTorch),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
  TmpGraph = std::make_shared<WasmEdge::Host::WASINN::Graph>(Backend::PyTorch);
  NNGraphTmp.insert(TmpGraph);
  // Test: init_execution_context -- graph id exceeds.
  // TODO: add a non-null test for PyTorch.
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

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- initialize the second context.
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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

  NNContextTmp.insert(
      std::make_shared<WasmEdge::Host::WASINN::Context>(0, TmpGraph));

  // Test: set_input -- tensor type not FP32.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(2), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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
    const auto OutputClassification =
        MemInst.getSpan<const float>(StorePtr, 1000);
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
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

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
  spdlog::info("Read {}"sv, TensorData.size());
  std::vector<uint32_t> TensorDim{1, 224, 224, 3};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(410 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Temp. values.
  WasmEdge::Host::WASINN::ResourceTable<WasmEdge::Host::WASINN::Graph>
      NNGraphTmp;
  WasmEdge::Host::WASINN::ResourceTable<WasmEdge::Host::WASINN::Context>
      NNContextTmp;
  std::shared_ptr<WasmEdge::Host::WASINN::Graph> TmpGraph;

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // Torch WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
                             static_cast<uint32_t>(Device::CPU), OutBoundPtr},
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
                             static_cast<uint32_t>(Device::CPU), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr,
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(1),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             static_cast<uint32_t>(Device::CPU), BuilderPtr},
                         Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong builder count.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(WeightRead.size()),
                  BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += WeightRead.size();
  {
    EXPECT_TRUE(
        HostFuncLoad.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             LoadEntryPtr, UINT32_C(2),
                             static_cast<uint32_t>(Backend::TensorflowLite),
                             static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
                             static_cast<uint32_t>(Device::AUTO), BuilderPtr},
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
                             static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
                             static_cast<uint32_t>(Device::CPU), BuilderPtr},
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
  TmpGraph =
      std::make_shared<WasmEdge::Host::WASINN::Graph>(Backend::TensorflowLite);
  NNGraphTmp.insert(TmpGraph);
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

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- initialize the second context.
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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

  NNContextTmp.insert(
      std::make_shared<WasmEdge::Host::WASINN::Context>(0, TmpGraph));

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  // Tensor type U8
  writeUInt32(MemInst, UINT32_C(3), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
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
    const auto OutputClassification =
        MemInst.getSpan<const uint8_t>(StorePtr, 965);
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

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
TEST(WasiNNTest, GGMLBackend) {
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Load the files.
  std::string Prompt = "Once upon a time, ";
  std::vector<uint8_t> TensorData(Prompt.begin(), Prompt.end());
  std::string Model = WasmEdge::Endian::native == WasmEdge::Endian::little
                          ? "./wasinn_ggml_fixtures/stories260K.gguf"
                          : "./wasinn_ggml_fixtures/granite-3.gguf";
  std::vector<uint8_t> WeightRead = readEntireFile(Model);

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(61000 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // GGML WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- GGML model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr,
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong metadata encoding when builders length > 1.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(WeightRead.size()),
                  BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += static_cast<uint32_t>(WeightRead.size());
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidEncoding));
  }

  // Test: load -- load successfully.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // GGML WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: get_output_single -- a freshly initialized context has an empty
  // streamed-token vector, so it reports zero bytes instead of dereferencing
  // past the end of the vector.
  {
    auto *SingleInst = NNMod->findFuncExports("get_output_single");
    ASSERT_NE(SingleInst, nullptr);
    auto &HostFuncGetOutputSingle = SingleInst->getHostFunc();
    EXPECT_TRUE(HostFuncGetOutputSingle.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0U);
  }

  // GGML WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData,
                         StorePtr +
                             static_cast<uint32_t>(TensorDim.size()) * 4);

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

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += static_cast<uint32_t>(TensorDim.size() * 4 + TensorData.size());

  // GGML WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: compute -- compute until finish or context full.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_TRUE(
        Errno[0].get<int32_t>() == static_cast<uint32_t>(ErrNo::Success) ||
        Errno[0].get<int32_t>() == static_cast<uint32_t>(ErrNo::ContextFull));
  }

  // GGML WASI-NN get_output tests.
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

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    // Should output more than 50 bytes.
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 50);
  }

  // Test: get_output -- an undersized buffer is rejected without any partial
  // write. The required size is still reported through BytesWritten.
  {
    const uint32_t Needed = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    ASSERT_GE(Needed, 2U);
    constexpr uint8_t GuardByte = 0xE7U;
    const uint32_t GuardLen = Needed + 16U;
    for (uint32_t I = 0; I < GuardLen; ++I) {
      *MemInst.getPointer<uint8_t *>(StorePtr + I) = GuardByte;
    }
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, Needed - 1U, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::TooLarge));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), Needed);
    bool Untouched = true;
    for (uint32_t I = 0; I < GuardLen; ++I) {
      Untouched = Untouched &&
                  (*MemInst.getPointer<uint8_t *>(StorePtr + I) == GuardByte);
    }
    EXPECT_TRUE(Untouched);
  }

  // Test: get_output -- an exact-size buffer succeeds.
  {
    const uint32_t Needed = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, Needed, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), Needed);
  }

  // Get the unload, finalize_execution_context, get_output_single, and
  // fini_single host functions for the lifetime tests below.
  FuncInst = NNMod->findFuncExports("unload");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncUnload = FuncInst->getHostFunc();
  FuncInst = NNMod->findFuncExports("finalize_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncFinalize = FuncInst->getHostFunc();
  FuncInst = NNMod->findFuncExports("get_output_single");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutputSingleDrain = FuncInst->getHostFunc();
  FuncInst = NNMod->findFuncExports("fini_single");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncFiniSingle = FuncInst->getHostFunc();

  // Test: init_execution_context -- create a second, token-less context so
  // the graph stays pinned by two contexts across the unload below.
  uint32_t NoTokenCtx = UINT32_C(0);
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    NoTokenCtx = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_EQ(NoTokenCtx, UINT32_C(1));
  }

  // Test: unload -- unload while both contexts are alive: returns
  // immediately and the live contexts keep the model alive to drain.
  {
    EXPECT_TRUE(HostFuncUnload.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: unload -- a second unload of the same handle is rejected; the
  // handle died with the first call.
  {
    EXPECT_TRUE(HostFuncUnload.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: init_execution_context -- the unloaded graph cannot create new
  // contexts.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: get_output -- drain the still-alive context after unload. The
  // context pins the graph, so the buffered output stays readable.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 50);
  }

  // Test: get_output_single -- drain via the model after unload. Unlike
  // get_output's buffered copy, this detokenizes through the graph-owned
  // llama context, so it only works if the model survived the unload.
  {
    EXPECT_TRUE(HostFuncGetOutputSingleDrain.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, UINT32_C(65532), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: fini_single -- reset the still-alive context's streaming state
  // after unload.
  {
    EXPECT_TRUE(HostFuncFiniSingle.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: finalize_execution_context -- finalize both contexts. The second
  // release drops the last pin and destroys the backend payload.
  {
    EXPECT_TRUE(HostFuncFinalize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_TRUE(HostFuncFinalize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NoTokenCtx},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: finalize_execution_context -- a finalized handle stays dead.
  {
    EXPECT_TRUE(HostFuncFinalize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- reloading yields a fresh handle: id 0 is never reused, so
  // stale handles keep failing instead of aliasing the new graph.
  uint32_t FreshGraphId = UINT32_C(0);
  {
    *MemInst.getPointer<uint32_t *>(BuilderPtr) = UINT32_C(0xFFFFFFFF);
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    FreshGraphId = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_EQ(FreshGraphId, UINT32_C(1));
  }

  // Test: concurrent host ops -- one thread hammers dead and unknown handles
  // while another runs real inference on the fresh graph. The dead handles
  // must keep failing cleanly and the live path must stay unaffected.
  {
    uint32_t FreshCtx = UINT32_C(0);
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FreshGraphId, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    FreshCtx = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_EQ(FreshCtx, UINT32_C(2));

    // Re-marshal the input tensor: the earlier get_output tests overwrote the
    // prompt bytes at StorePtr with model output and guard bytes.
    BuilderPtr = SetInputEntryPtr;
    writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                    BuilderPtr);
    writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
    writeFatPointer(MemInst,
                    StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                    static_cast<uint32_t>(TensorData.size()), BuilderPtr);
    writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
    writeBinaries<uint8_t>(MemInst, TensorData,
                           StorePtr +
                               static_cast<uint32_t>(TensorDim.size()) * 4);
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 FreshCtx, UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));

    std::thread StaleThread([&]() {
      WasmEdge::Runtime::Instance::ModuleInstance StaleMod("");
      StaleMod.addHostMemory(
          "memory",
          std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
              WasmEdge::AST::MemoryType(1)));
      WasmEdge::Runtime::CallingFrame StaleFrame(nullptr, &StaleMod);
      std::array<WasmEdge::ValVariant, 1> StaleErrno = {UINT32_C(0)};
      // A bounded number of rounds keeps the rejection logging finite while
      // still overlapping the live computes on the main thread.
      for (int Round = 0; Round < 100; ++Round) {
        // The unloaded graph handle.
        EXPECT_TRUE(
            HostFuncInit.run(StaleFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0)},
                             StaleErrno));
        EXPECT_EQ(StaleErrno[0].get<int32_t>(),
                  static_cast<uint32_t>(ErrNo::InvalidArgument));
        // The finalized context handle.
        EXPECT_TRUE(HostFuncCompute.run(
            StaleFrame,
            std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
            StaleErrno));
        EXPECT_EQ(StaleErrno[0].get<int32_t>(),
                  static_cast<uint32_t>(ErrNo::InvalidArgument));
        // A never-allocated handle.
        EXPECT_TRUE(HostFuncUnload.run(
            StaleFrame,
            std::initializer_list<WasmEdge::ValVariant>{UINT32_C(9999)},
            StaleErrno));
        EXPECT_EQ(StaleErrno[0].get<int32_t>(),
                  static_cast<uint32_t>(ErrNo::InvalidArgument));
      }
    });
    // Compute until finish or context full, as the earlier compute test does.
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FreshCtx},
        Errno));
    EXPECT_TRUE(
        Errno[0].get<int32_t>() == static_cast<uint32_t>(ErrNo::Success) ||
        Errno[0].get<int32_t>() == static_cast<uint32_t>(ErrNo::ContextFull));
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FreshCtx, UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    StaleThread.join();

    // Unload the fresh graph, drain the context once more, then finalize it.
    EXPECT_TRUE(HostFuncUnload.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FreshGraphId},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FreshCtx, UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_TRUE(HostFuncFinalize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FreshCtx},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
}

TEST(WasiNNTest, DrainAfterUnloadHostOpTiers) {
  // Pin each op's hostOpPolicy tier so a later edit cannot move set_input
  // (NotDetached) or compute/init (Ready) onto a drain tier and let them
  // reconfigure or re-run an unloaded graph.
  using WasmEdge::Host::WASINN::GraphReq;
  using WasmEdge::Host::WASINN::HostOp;
  using WasmEdge::Host::WASINN::hostOpPolicy;
  // Reconfigure- and compute-class ops require a live graph.
  EXPECT_EQ(hostOpPolicy(HostOp::SetInput).Req, GraphReq::NotDetached);
  EXPECT_EQ(hostOpPolicy(HostOp::InitExecCtx).Req, GraphReq::Ready);
  EXPECT_EQ(hostOpPolicy(HostOp::Compute).Req, GraphReq::Ready);
  EXPECT_EQ(hostOpPolicy(HostOp::ComputeSingle).Req, GraphReq::Ready);
  // Drain-class ops keep working after unload while a context finishes.
  EXPECT_EQ(hostOpPolicy(HostOp::GetOutput).Req, GraphReq::Any);
  EXPECT_EQ(hostOpPolicy(HostOp::GetOutputSingle).Req, GraphReq::Drainable);
  EXPECT_EQ(hostOpPolicy(HostOp::FiniSingle).Req, GraphReq::Any);
}

TEST(WasiNNTest, GraphStatusDetachedIsTerminal) {
  // unload marks a graph Detached without the op mutex, so an in-flight
  // set_input reload can finish afterwards; its trailing setReady/setInvalid
  // must not overwrite Detached or a surviving context would regain compute
  // rights on an unloaded graph.
  using WasmEdge::Host::WASINN::Graph;
  using WasmEdge::Host::WASINN::GraphStatus;

  Graph Reloaded(Backend::GGML);
  EXPECT_EQ(Reloaded.status(), GraphStatus::Ready);
  Reloaded.setDetached();
  Reloaded.setReady();
  EXPECT_EQ(Reloaded.status(), GraphStatus::Detached);

  Graph FailedReload(Backend::GGML);
  FailedReload.setDetached();
  FailedReload.setInvalid();
  EXPECT_EQ(FailedReload.status(), GraphStatus::Detached);

  // Outside Detached, the Ready <-> Invalid reload transitions stay allowed.
  Graph Live(Backend::GGML);
  Live.setInvalid();
  EXPECT_EQ(Live.status(), GraphStatus::Invalid);
  Live.setReady();
  EXPECT_EQ(Live.status(), GraphStatus::Ready);
  Live.setDetached();
  EXPECT_EQ(Live.status(), GraphStatus::Detached);
}

TEST(WasiNNTest, LoadByNameConcurrentBuildsShareOneGraph) {
  // Two load_by_name calls for one name can miss the cache and build
  // concurrently (MdMutex is not held across a load); the second finisher
  // must adopt the first one's cached graph instead of keeping a duplicate.
  auto NNMod = createModule();
  if (!NNMod) {
    GTEST_SKIP() << "wasi_nn plugin not found";
  }
  auto &Env = NNMod->getEnv();
  const std::string Name = "concurrent-build";
  Env.RawMdMap.emplace(
      Name, std::make_tuple(std::vector<std::vector<uint8_t>>{{0x00}},
                            Backend::GGML, Device::CPU));

  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(30);
  auto SpinUntil = [&Deadline](auto Cond) {
    while (!Cond()) {
      if (std::chrono::steady_clock::now() > Deadline) {
        return false;
      }
      std::this_thread::yield();
    }
    return true;
  };

  // A stand-in for a slow backend load: park each caller until released by
  // entry order, then publish an empty graph wrapper like the real loader.
  std::atomic<uint32_t> Entered{0};
  std::atomic<uint32_t> ReleaseUpTo{0};
  std::array<uint32_t, 2> PublishedIds = {0, 0};
  auto SlowLoad = [&](WasmEdge::Host::WASINN::WasiNNEnvironment &E,
                      WasmEdge::Span<const WasmEdge::Span<uint8_t>>, Backend BE,
                      Device, std::string_view MdName,
                      uint32_t &GraphIdOut) -> WasmEdge::Expect<ErrNo> {
    const uint32_t Order = Entered.fetch_add(1) + 1;
    while (ReleaseUpTo.load() < Order) {
      std::this_thread::yield();
    }
    auto G = std::make_shared<WasmEdge::Host::WASINN::Graph>(BE);
    G->setModelName(std::string(MdName));
    auto Id = E.NNGraph.insert(std::move(G));
    EXPECT_TRUE(Id.has_value());
    GraphIdOut = *Id;
    PublishedIds[Order - 1] = *Id;
    return ErrNo::Success;
  };

  uint32_t FirstId = UINT32_C(0xFFFFFFFF);
  uint32_t SecondId = UINT32_C(0xFFFFFFFF);
  std::atomic<uint32_t> DoneCount{0};
  std::thread First([&]() {
    auto Res = Env.mdBuild(Name, FirstId, SlowLoad);
    EXPECT_TRUE(Res.has_value());
    DoneCount.fetch_add(1);
  });
  std::thread Second([&]() {
    auto Res = Env.mdBuild(Name, SecondId, SlowLoad);
    EXPECT_TRUE(Res.has_value());
    DoneCount.fetch_add(1);
  });

  // Both callers sit inside their builds before either caches: this is the
  // race window between a shared cache miss and the first cache write.
  EXPECT_TRUE(SpinUntil([&]() { return Entered.load() == 2; }));
  // The first entrant finishes its whole build and caches its graph.
  ReleaseUpTo.store(1);
  EXPECT_TRUE(SpinUntil([&]() { return DoneCount.load() == 1; }));
  // The second entrant finishes and must fold onto the cached winner.
  ReleaseUpTo.store(2);
  First.join();
  Second.join();

  // Exactly one graph survives for the name: both callers hold the winner's
  // id, the loser's build was dropped, and the cache resolves to the winner.
  EXPECT_EQ(FirstId, SecondId);
  const uint32_t WinnerId = FirstId;
  EXPECT_NE(Env.NNGraph.get(WinnerId), nullptr);
  EXPECT_EQ(Env.NNGraph.size(), 1U);
  const uint32_t LoserId =
      PublishedIds[0] == WinnerId ? PublishedIds[1] : PublishedIds[0];
  EXPECT_NE(LoserId, WinnerId);
  EXPECT_EQ(Env.NNGraph.get(LoserId), nullptr);
  uint32_t CachedId = UINT32_C(0xFFFFFFFF);
  EXPECT_TRUE(Env.mdGet(Name, CachedId));
  EXPECT_EQ(CachedId, WinnerId);
}

TEST(WasiNNTest, LoadByNameSkipsDetachedGraphStillInTable) {
  // unload publishes Detached before dropping the table entry, so a racing
  // load_by_name can observe a Detached yet still-tabled graph. The name
  // cache must treat it as unloaded - neither resolving it nor folding a
  // build onto it - or the guest would get an already-rejected handle.
  auto NNMod = createModule();
  if (!NNMod) {
    GTEST_SKIP() << "wasi_nn plugin not found";
  }
  auto &Env = NNMod->getEnv();
  const std::string Name = "detached-window";
  Env.RawMdMap.emplace(
      Name, std::make_tuple(std::vector<std::vector<uint8_t>>{{0x00}},
                            Backend::GGML, Device::CPU));
  auto StubLoad = [](WasmEdge::Host::WASINN::WasiNNEnvironment &E,
                     WasmEdge::Span<const WasmEdge::Span<uint8_t>>, Backend BE,
                     Device, std::string_view MdName,
                     uint32_t &GraphIdOut) -> WasmEdge::Expect<ErrNo> {
    auto G = std::make_shared<WasmEdge::Host::WASINN::Graph>(BE);
    G->setModelName(std::string(MdName));
    auto Id = E.NNGraph.insert(std::move(G));
    EXPECT_TRUE(Id.has_value());
    GraphIdOut = *Id;
    return ErrNo::Success;
  };

  uint32_t BuiltId = UINT32_C(0xFFFFFFFF);
  auto Res = Env.mdBuild(Name, BuiltId, StubLoad);
  ASSERT_TRUE(Res.has_value());
  EXPECT_EQ(*Res, ErrNo::Success);
  uint32_t ResolvedId = UINT32_C(0xFFFFFFFF);
  EXPECT_TRUE(Env.mdGet(Name, ResolvedId));
  EXPECT_EQ(ResolvedId, BuiltId);

  // Enter the mid-unload window: Detached is visible while the table entry
  // and the cache entry still exist.
  auto G = Env.NNGraph.get(BuiltId);
  ASSERT_NE(G, nullptr);
  G->setDetached();
  EXPECT_NE(Env.NNGraph.get(BuiltId), nullptr);
  EXPECT_FALSE(Env.mdGet(Name, ResolvedId));

  // A Ready-tier graph-keyed op rejects the half-detached graph even though
  // its handle still resolves.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  auto *FuncInst = NNMod->findFuncExports("init_execution_context"sv);
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};
  EXPECT_TRUE(FuncInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{BuiltId, UINT32_C(0)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // A build finishing inside the window publishes its own graph instead of
  // folding onto the dying one.
  uint32_t RebuiltId = UINT32_C(0xFFFFFFFF);
  Res = Env.mdBuild(Name, RebuiltId, StubLoad);
  ASSERT_TRUE(Res.has_value());
  EXPECT_EQ(*Res, ErrNo::Success);
  EXPECT_NE(RebuiltId, BuiltId);
  EXPECT_TRUE(Env.mdGet(Name, ResolvedId));
  EXPECT_EQ(ResolvedId, RebuiltId);
}

TEST(WasiNNTest, GGMLBackendDrainAfterInvalidReloadUnload) {
  // A failed metadata reload leaves the graph Invalid with a null llama
  // context; unload then makes it Detached, which Drainable admits, so the
  // backend drain must reject the null context instead of dereferencing it.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::string Prompt = "Once upon a time, ";
  std::vector<uint8_t> TensorData(Prompt.begin(), Prompt.end());
  std::string Model = WasmEdge::Endian::native == WasmEdge::Endian::little
                          ? "./wasinn_ggml_fixtures/stories260K.gguf"
                          : "./wasinn_ggml_fixtures/granite-3.gguf";
  std::vector<uint8_t> WeightRead = readEntireFile(Model);
  ASSERT_FALSE(WeightRead.empty());

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  const uint32_t LoadEntryPtr = UINT32_C(0);
  const uint32_t SetInputEntryPtr = UINT32_C(16);
  const uint32_t BytesWrittenPtr = UINT32_C(40);
  uint32_t StorePtr = UINT32_C(65536);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  auto GetHostFunc =
      [&](std::string_view FuncName) -> WasmEdge::Runtime::HostFunctionBase & {
    auto *FuncInst = NNMod->findFuncExports(FuncName);
    EXPECT_NE(FuncInst, nullptr);
    EXPECT_TRUE(FuncInst->isHostFunction());
    return FuncInst->getHostFunc();
  };
  auto &HostFuncLoad = GetHostFunc("load"sv);
  auto &HostFuncInit = GetHostFunc("init_execution_context"sv);
  auto &HostFuncSetInput = GetHostFunc("set_input"sv);
  auto &HostFuncComputeSingle = GetHostFunc("compute_single"sv);
  auto &HostFuncGetOutputSingle = GetHostFunc("get_output_single"sv);
  auto &HostFuncFiniSingle = GetHostFunc("fini_single"sv);
  auto &HostFuncUnload = GetHostFunc("unload"sv);
  auto &HostFuncFinalize = GetHostFunc("finalize_execution_context"sv);

  // Load the model and stream one token so the context buffers output.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(WeightRead.size()),
                  BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += static_cast<uint32_t>(WeightRead.size());
  EXPECT_TRUE(HostFuncLoad.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::GGML),
          static_cast<uint32_t>(Device::CPU), BuilderPtr},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  const uint32_t GraphId = *MemInst.getPointer<uint32_t *>(BuilderPtr);
  BuilderPtr += 4;
  EXPECT_TRUE(HostFuncInit.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{GraphId, BuilderPtr}, Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  const uint32_t CtxId = *MemInst.getPointer<uint32_t *>(BuilderPtr);

  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData,
                         StorePtr +
                             static_cast<uint32_t>(TensorDim.size()) * 4);
  EXPECT_TRUE(HostFuncSetInput.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       CtxId, UINT32_C(0), SetInputEntryPtr},
                                   Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(HostFuncComputeSingle.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CtxId}, Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(HostFuncGetOutputSingle.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CtxId, UINT32_C(0), StorePtr, UINT32_C(65532), BytesWrittenPtr},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));

  // A context-parameter reload that must fail: llama rejects a context whose
  // batch and ubatch sizes are both zero, so set_input leaves the graph
  // Invalid and the llama context null.
  const std::string BadMetadata =
      "{\"embedding\": true, \"batch-size\": 0, \"ubatch-size\": 0}";
  std::vector<uint8_t> BadMetadataData(BadMetadata.begin(), BadMetadata.end());
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(BadMetadataData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, BadMetadataData,
                         StorePtr +
                             static_cast<uint32_t>(TensorDim.size()) * 4);
  EXPECT_TRUE(HostFuncSetInput.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       CtxId, UINT32_C(1), SetInputEntryPtr},
                                   Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Invalid is rejected by the Drainable tier before the unload.
  EXPECT_TRUE(HostFuncGetOutputSingle.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CtxId, UINT32_C(0), StorePtr, UINT32_C(65532), BytesWrittenPtr},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // unload admits the drain tier again; the null context must be rejected by
  // the backend instead of crashing the drain.
  EXPECT_TRUE(HostFuncUnload.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{GraphId}, Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(HostFuncGetOutputSingle.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CtxId, UINT32_C(0), StorePtr, UINT32_C(65532), BytesWrittenPtr},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // The context still resets and finalizes cleanly without a llama context.
  EXPECT_TRUE(HostFuncFiniSingle.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CtxId}, Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(HostFuncFinalize.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CtxId}, Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
}

namespace {
// Table bookkeeping is covered backend-free in resource_table_test.cpp; these
// check the host-function wiring: guards reject an id that is not a live
// graph/context before dispatching to any backend, so no model is needed.
void expectRejectsUnknownId(std::string_view FuncName) {
  auto NNMod = createModule();
  if (!NNMod) {
    GTEST_SKIP() << "wasi_nn plugin not found";
  }

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  auto *FuncInst = NNMod->findFuncExports(FuncName);
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFunc = FuncInst->getHostFunc();
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // The module is fresh, so no id exists; 9999 is plainly unknown.
  const uint32_t UnknownId = 9999;
  EXPECT_TRUE(HostFunc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UnknownId},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(),
            static_cast<uint32_t>(ErrNo::InvalidArgument));
}
} // namespace

TEST(WasiNNTest, UnloadRejectsInvalidGraphId) {
  expectRejectsUnknownId("unload"sv);
}

TEST(WasiNNTest, FinalizeRejectsInvalidContextId) {
  expectRejectsUnknownId("finalize_execution_context"sv);
}

TEST(WasiNNTest, ComputeRejectsInvalidContextId) {
  expectRejectsUnknownId("compute"sv);
}

namespace {
uint32_t runLoadByName(uint32_t NamePtr, uint32_t NameLen) {
  auto NNMod = createModule();
  if (!NNMod) {
    return UINT32_C(0xFFFFFFFF);
  }
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  auto &HostFunc = dynamic_cast<WasmEdge::Host::WasiNNLoadByName &>(
      NNMod->findFuncExports("load_by_name")->getHostFunc());
  std::array<WasmEdge::ValVariant, 1> Rets = {UINT32_C(0)};
  HostFunc.run(CallFrame,
               std::initializer_list<WasmEdge::ValVariant>{NamePtr, NameLen,
                                                           UINT32_C(0)},
               Rets);
  return Rets[0].get<uint32_t>();
}

uint32_t runLoadByNameWithConfig(uint32_t NamePtr, uint32_t NameLen,
                                 uint32_t ConfigPtr, uint32_t ConfigLen) {
  auto NNMod = createModule();
  if (!NNMod) {
    return UINT32_C(0xFFFFFFFF);
  }
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  auto &HostFunc = dynamic_cast<WasmEdge::Host::WasiNNLoadByNameWithConfig &>(
      NNMod->findFuncExports("load_by_name_with_config")->getHostFunc());
  std::array<WasmEdge::ValVariant, 1> Rets = {UINT32_C(0)};
  HostFunc.run(CallFrame,
               std::initializer_list<WasmEdge::ValVariant>{
                   NamePtr, NameLen, ConfigPtr, ConfigLen, UINT32_C(0)},
               Rets);
  return Rets[0].get<uint32_t>();
}
} // namespace

TEST(WasiNNTest, WasiNNLoadByNameRejectsNameBeyondMemory) {
  uint32_t E = runLoadByName(/*NamePtr=*/65500U, /*NameLen=*/64U);
  if (E == 0xFFFFFFFFU) {
    GTEST_SKIP() << "wasi_nn plugin not found";
  }
  EXPECT_EQ(E, static_cast<uint32_t>(ErrNo::InvalidArgument));

  uint32_t ZeroLenName = runLoadByName(/*NamePtr=*/0xFFFFFFFFU, /*NameLen=*/0U);
  EXPECT_EQ(ZeroLenName, static_cast<uint32_t>(ErrNo::InvalidArgument));
}

TEST(WasiNNTest, WasiNNLoadByNameWithConfigRejectsOutOfBoundsExtent) {
  uint32_t NameOOB =
      runLoadByNameWithConfig(/*NamePtr=*/65500U, /*NameLen=*/64U,
                              /*ConfigPtr=*/0U, /*ConfigLen=*/8U);
  if (NameOOB == 0xFFFFFFFFU) {
    GTEST_SKIP() << "wasi_nn plugin not found";
  }
  EXPECT_EQ(NameOOB, static_cast<uint32_t>(ErrNo::InvalidArgument));

  uint32_t ConfigOOB = runLoadByNameWithConfig(
      /*NamePtr=*/0U, /*NameLen=*/8U, /*ConfigPtr=*/65500U, /*ConfigLen=*/64U);
  EXPECT_EQ(ConfigOOB, static_cast<uint32_t>(ErrNo::InvalidArgument));

  uint32_t ZeroLenName = runLoadByNameWithConfig(
      /*NamePtr=*/0xFFFFFFFFU, /*NameLen=*/0U, /*ConfigPtr=*/0U,
      /*ConfigLen=*/0U);
  EXPECT_EQ(ZeroLenName, static_cast<uint32_t>(ErrNo::InvalidArgument));

  uint32_t ZeroLenConfig = runLoadByNameWithConfig(
      /*NamePtr=*/0U, /*NameLen=*/0U, /*ConfigPtr=*/0xFFFFFFFFU,
      /*ConfigLen=*/0U);
  EXPECT_EQ(ZeroLenConfig, static_cast<uint32_t>(ErrNo::InvalidArgument));
}
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
TEST(WasiNNTest, GGMLBackendWithRPC) {
  // wasi_nn_rpcserver has to be started outside this test,
  // and the URI has to be set to $WASI_NN_RPC_TEST_URI.
  // nn-preload has to be specified for "default".
  /*
    DIR=/tmp/build
    export WASI_NN_RPC_TEST_URI=unix://${DIR}/wasi_nn_rpc.sock
    export WASMEDGE_PLUGIN_PATH=${DIR}/plugins/wasi_nn
    ${DIR}/tools/wasmedge/wasi_nn_rpcserver \
      --nn-rpc-uri=$WASI_NN_RPC_TEST_URI \
      --nn-preload=default:GGML:AUTO:${DIR}/test/plugins/wasi_nn/wasinn_ggml_fixtures/stories260K.gguf
  */
  const auto NNRPCURI = ::getenv("WASI_NN_RPC_TEST_URI");
  if (NNRPCURI == nullptr) {
    GTEST_SKIP() << "WASI_NN_RPC_TEST_URI is unset";
  }

  // Create the wasi_nn module instance.
  auto NNMod = createModule(NNRPCURI);
  ASSERT_TRUE(NNMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::string Prompt = "Once upon a time, ";
  std::vector<uint8_t> TensorData(Prompt.begin(), Prompt.end());

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load_by_name".
  auto FuncInst = NNMod->findFuncExports("load_by_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoadByName = FuncInst->getHostFunc();
  // Get the function "load_by_name_with_config".
  FuncInst = NNMod->findFuncExports("load_by_name_with_config");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoadByNameWithConfig = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // Test: load_by_name -- load successfully.
  {
    std::string Name = "default";
    std::vector<char> NameVec(Name.begin(), Name.end());
    writeBinaries<char>(MemInst, NameVec, LoadEntryPtr);
    EXPECT_TRUE(HostFuncLoadByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, static_cast<uint32_t>(NameVec.size()), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: load_by_name_with_config -- load successfully.
  {
    std::string Name = "default";
    std::string Config = "{}";
    std::vector<char> NameVec(Name.begin(), Name.end());
    std::vector<char> ConfigVec(Config.begin(), Config.end());
    uint32_t ConfigPtr = LoadEntryPtr + NameVec.size();
    writeBinaries<char>(MemInst, NameVec, LoadEntryPtr);
    writeBinaries<char>(MemInst, ConfigVec, ConfigPtr);
    EXPECT_TRUE(HostFuncLoadByNameWithConfig.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, static_cast<uint32_t>(NameVec.size()), ConfigPtr,
            static_cast<uint32_t>(ConfigVec.size()), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // GGML WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // GGML WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Test: set_input -- context id exceeds.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(3), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // GGML WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: compute -- compute until finish or context full.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    // FIXME: ErrNo propagation is not supported yet
    //    EXPECT_TRUE(
    //        Errno[0].get<int32_t>() == static_cast<uint32_t>(ErrNo::Success)
    //        || Errno[0].get<int32_t>() ==
    //        static_cast<uint32_t>(ErrNo::ContextFull));
  }

  // GGML WASI-NN get_output tests.
  // Test: get_output -- output bytes ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, OutBoundPtr},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: get_output -- output buffer ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), OutBoundPtr, 65532, BuilderPtr},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    // Should output more than 50 bytes.
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 50);
  }
}

TEST(WasiNNTest, GGMLBackendComputeSingleWithRPC) {
  // wasi_nn_rpcserver has to be started outside this test,
  // and the URI has to be set to $WASI_NN_RPC_TEST_URI.
  // nn-preload has to be specified for "default".
  /*
    DIR=/tmp/build
    export WASI_NN_RPC_TEST_URI=unix://${DIR}/wasi_nn_rpc.sock
    export WASMEDGE_PLUGIN_PATH=${DIR}/plugins/wasi_nn
    ${DIR}/tools/wasmedge/wasi_nn_rpcserver \
      --nn-rpc-uri=$WASI_NN_RPC_TEST_URI \
      --nn-preload=default:GGML:AUTO:${DIR}/test/plugins/wasi_nn/wasinn_ggml_fixtures/stories260K.gguf
  */
  const auto NNRPCURI = ::getenv("WASI_NN_RPC_TEST_URI");
  if (NNRPCURI == nullptr) {
    GTEST_SKIP() << "WASI_NN_RPC_TEST_URI is unset";
  }

  // Create the wasmedge_process module instance.
  auto NNMod = createModule(NNRPCURI);
  ASSERT_TRUE(NNMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::string Prompt = "Once upon a time, ";
  std::vector<uint8_t> TensorData(Prompt.begin(), Prompt.end());

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load_by_name".
  auto FuncInst = NNMod->findFuncExports("load_by_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoadByName = FuncInst->getHostFunc();
  // Get the function "load_by_name_with_config".
  FuncInst = NNMod->findFuncExports("load_by_name_with_config");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoadByNameWithConfig = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output_single");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutputSingle = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute_single");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncComputeSingle = FuncInst->getHostFunc();

  // Test: load_by_name -- load successfully.
  {
    std::string Name = "default";
    std::vector<char> NameVec(Name.begin(), Name.end());
    writeBinaries<char>(MemInst, NameVec, LoadEntryPtr);
    EXPECT_TRUE(HostFuncLoadByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, static_cast<uint32_t>(NameVec.size()), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: load_by_name_with_config -- load successfully.
  {
    std::string Name = "default";
    std::string Config = "{}";
    std::vector<char> NameVec(Name.begin(), Name.end());
    std::vector<char> ConfigVec(Config.begin(), Config.end());
    uint32_t ConfigPtr = LoadEntryPtr + NameVec.size();
    writeBinaries<char>(MemInst, NameVec, LoadEntryPtr);
    writeBinaries<char>(MemInst, ConfigVec, ConfigPtr);
    EXPECT_TRUE(HostFuncLoadByNameWithConfig.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, static_cast<uint32_t>(NameVec.size()), ConfigPtr,
            static_cast<uint32_t>(ConfigVec.size()), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // GGML WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Test: set_input -- set input successfully.
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // GGML WASI-NN compute_single tests.
  // Test: compute_single -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncComputeSingle.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: compute_single -- call compute_single once follow by a
  // get_output_single.
  {
    // compute_single
    EXPECT_TRUE(HostFuncComputeSingle.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    // get_output_single
    EXPECT_TRUE(HostFuncGetOutputSingle.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // GGML WASI-NN get_output_single tests.
  // Test: get_output_single -- output bytes ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutputSingle.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, OutBoundPtr},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: get_output -- output buffer ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncGetOutputSingle.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), OutBoundPtr, 65532, BuilderPtr},
        Errno));
    EXPECT_NE(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
}

namespace {
uint64_t hostFuncCallerPages(uint64_t MemoryBytes) {
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  WasmEdge::WasiNNRPC::Server::HostFuncCaller Caller(Mod, "noop", MemoryBytes);
  return Caller.getMemInst().getPageSize();
}
} // namespace

TEST(WasiNNTest, RPCServerHostFuncCallerSizesMemoryByBytes) {
  EXPECT_EQ(hostFuncCallerPages(0U), UINT64_C(0));
  EXPECT_EQ(hostFuncCallerPages(1U), UINT64_C(1));
  EXPECT_EQ(hostFuncCallerPages(UINT32_C(65536)), UINT64_C(1));
  EXPECT_EQ(hostFuncCallerPages(UINT32_C(65537)), UINT64_C(2));
  EXPECT_EQ(hostFuncCallerPages(UINT64_C(60) * UINT64_C(1024)), UINT64_C(1));
}
#endif // WASMEDGE_BUILD_WASI_NN_RPC
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
TEST(WasiNNTest, WhisperBackend) {
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::vector<uint8_t> TensorData =
      readEntireFile("./wasinn_whisper_fixtures/test.wav");
  std::vector<uint8_t> WeightRead =
      readEntireFile("./wasinn_whisper_fixtures/ggml-tiny.bin");
  std::vector<uint32_t> TensorDim{1, static_cast<uint32_t>(TensorData.size())};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // Whisper WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::Whisper),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::Whisper),
            static_cast<uint32_t>(Device::CPU), OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, UINT32_C(1), static_cast<uint32_t>(Backend::Whisper),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- Whisper model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr,
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::Whisper),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- load successfully.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, WeightRead.size(), BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  StorePtr += WeightRead.size();
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::Whisper),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Whisper WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: init_execution_context -- initialize the second context.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Whisper WASI-NN set_input tests.
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
  // Test: set_input -- set input successfully.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // Whisper WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Whisper WASI-NN get_output tests.
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
  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    // Should output more than 50 bytes.
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 50);
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
TEST(WasiNNTest, PiperBackend) {
  // Create the wasmedge_process module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

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
  std::string Text = "This is a test.";
  std::vector<uint8_t> TensorData(Text.begin(), Text.end());

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(410 * 65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // Piper WASI-NN load tests.
  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::Piper),
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
                                     static_cast<uint32_t>(Backend::Piper),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- Piper config ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr, 1, BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::Piper),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- wrong config encoding.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, 0, BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::Piper),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidEncoding));
  }

  // Test: load -- load successfully.
  std::string Config =
      "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
      "\"espeak_data\": \"./wasinn_piper_fixtures/piper/espeak-ng-data\"}";
  std::vector<uint8_t> ConfigData(Config.begin(), Config.end());
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, ConfigData.size(), BuilderPtr);
  writeBinaries<uint8_t>(MemInst, ConfigData, StorePtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::Piper),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Piper WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // Piper WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, 3, BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + TensorDim.size() *
                                 sizeof(decltype(TensorDim)::value_type),
                  TensorData.size(), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(
      MemInst, TensorData,
      StorePtr + TensorDim.size() * sizeof(decltype(TensorDim)::value_type));

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
  // Test: set_input -- set input successfully.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += TensorDim.size() * sizeof(decltype(TensorDim)::value_type) +
              TensorData.size();

  // Piper WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Piper WASI-NN get_output tests.
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
  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    // Should output more than 10000 bytes.
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 10000);
  }

  // Piper json_input tests.
  // Test: load -- load successfully.
  Config =
      "{\"model\": "
      "\"./wasinn_piper_fixtures/test_voice.onnx\",\"espeak_data\": "
      "\"./wasinn_piper_fixtures/piper/espeak-ng-data\",\"json_input\":true}";
  ConfigData = {Config.begin(), Config.end()};
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, ConfigData.size(), BuilderPtr);
  writeBinaries<uint8_t>(MemInst, ConfigData, StorePtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::Piper),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // Test: init_execution_context -- initialize context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // First JSON input with parameters overridden.
  Text = "{\"text\": \"This is a test.\", \"noise_scale\": 0.0, "
         "\"length_scale\": 2.0, \"noise_w\": 0.0}";
  TensorData = {Text.begin(), Text.end()};
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, 3, BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + TensorDim.size() *
                                 sizeof(decltype(TensorDim)::value_type),
                  TensorData.size(), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(
      MemInst, TensorData,
      StorePtr + TensorDim.size() * sizeof(decltype(TensorDim)::value_type));

  // Test: set_input -- set input successfully.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += TensorDim.size() * sizeof(decltype(TensorDim)::value_type) +
              TensorData.size();

  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(1), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    // Should output more than 40000 bytes.
    EXPECT_GE(BytesWritten, 40000);
  }

  // Second JSON input to check if one-time overriding works properly.
  Text = "{\"text\": \"This is a test.\", \"output_type\": \"raw\", "
         "\"noise_scale\": 0.0, \"noise_w\": 0.0}";
  TensorData = {Text.begin(), Text.end()};
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, 3, BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + TensorDim.size() *
                                 sizeof(decltype(TensorDim)::value_type),
                  TensorData.size(), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(
      MemInst, TensorData,
      StorePtr + TensorDim.size() * sizeof(decltype(TensorDim)::value_type));

  // Test: set_input -- set input successfully.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += TensorDim.size() * sizeof(decltype(TensorDim)::value_type) +
              TensorData.size();

  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(1), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 30000);
    // Should output less than 50000 bytes.
    EXPECT_LT(BytesWritten, 50000);
    EXPECT_EQ(BytesWritten, 44100);
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS
TEST(WasiNNTest, ChatTTSBackend) {
  // Create the wasmedge_process module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Load the files.
  std::string Prompt = "This is test prompt.";
  std::vector<uint8_t> TensorData(Prompt.begin(), Prompt.end());
  std::string config =
      "{\"prompt\":\"[oral_2][laugh_0][break_6]\",\"spk_emb\":\"random\","
      "\"temperature\":0.5,\"top_k\":0,\"top_p\":0.9}";
  std::vector<uint8_t> ConfigData(config.begin(), config.end());

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();
  // Get the function "unload".
  FuncInst = NNMod->findFuncExports("unload");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncUnload = FuncInst->getHostFunc();

  // ChatTTS WASI-NN load tests.
  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::ChatTTS),
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
                                     static_cast<uint32_t>(Backend::ChatTTS),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- load successfully.
  BuilderPtr = LoadEntryPtr;
  {
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::ChatTTS),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }
  // ChatTTS WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: init_execution_context -- initialize the second context.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // ChatTTS WASI-NN set_input tests.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, static_cast<uint32_t>(TensorType::U8), BuilderPtr);
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
  // Test: set_input -- set input successfully.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // ChatTTS WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // Test: setInput -- set metadata successfully.
  SetInputEntryPtr = BuilderPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, static_cast<uint32_t>(TensorType::U8), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, ConfigData.size(),
                  BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, ConfigData, StorePtr + TensorDim.size() * 4);
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(1), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + ConfigData.size());

  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // ChatTTS WASI-NN get_output tests.
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
  // Test: get_output -- a zero-size probe reports the required size through
  // TooLarge, then a correctly sized buffer succeeds.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(0),
                                                    StorePtr, 0, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::TooLarge));
    // Should output more than 50 bytes.
    const uint32_t BytesNeeded = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesNeeded, 50);
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, BytesNeeded, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), BytesNeeded);
  }

  // ChatTTS WASI-NN unload tests.
  // Test: unload -- unload successfully.
  {
    EXPECT_TRUE(HostFuncUnload.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS

// A raw-bytes MLX load writes the model to a temporary safetensors file before
// conversion. Concurrent loads must not share that path, or one build can
// truncate the file another build is still reading. uniqueModelFileName is
// backend-agnostic string logic, so this guard runs in every build.
TEST(WasiNNTest, MLXTempPathIsUniquePerBuild) {
  constexpr std::size_t Count = 64;
  std::vector<std::string> Paths(Count);
  std::vector<std::thread> Workers;
  Workers.reserve(Count);
  for (std::size_t I = 0; I < Count; ++I) {
    Workers.emplace_back([&Paths, I]() {
      Paths[I] = WasmEdge::Host::WASINN::MLX::uniqueModelFileName(0);
    });
  }
  for (auto &Worker : Workers) {
    Worker.join();
  }
  const std::set<std::string> Distinct(Paths.begin(), Paths.end());
  EXPECT_EQ(Distinct.size(), Count);
}

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
TEST(WasiNNTest, MLXBackend) {
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Load the files.
  std::string Prompt = "How are you?";
  std::string Tokenizer = "./wasinn_mlx_fixtures/tokenizer.json";
  std::vector<uint8_t> TensorData(Prompt.begin(), Prompt.end());
  std::vector<uint8_t> WeightRead =
      readEntireFile("./wasinn_mlx_fixtures/model.safetensors");

  std::vector<uint32_t> TensorDim{1};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr = UINT32_C(0);
  uint32_t SetInputEntryPtr = UINT32_C(0);
  uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  uint32_t StorePtr = UINT32_C(65536);

  // Return value.
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute = FuncInst->getHostFunc();

  // MLX WASI-NN load tests.
  // Test: load -- meaningless binaries.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::MLX),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- graph id ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::MLX),
            static_cast<uint32_t>(Device::CPU), OutBoundPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, UINT32_C(1), static_cast<uint32_t>(Backend::MLX),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: load -- MLX model bin ptr out of bounds.
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, OutBoundPtr,
                  static_cast<uint32_t>(WeightRead.size()), BuilderPtr);
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(1), static_cast<uint32_t>(Backend::MLX),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- load successfully.
  std::string Config = "{\"model_type\":\"tiny_llama_1.1B_chat_v1.0\", "
                       "\"tokenizer\":\"" +
                       Tokenizer +
                       "\", \"q_bits\": 4, \"group_size\": 64, "
                       "\"is_quantized\": true, \"max_token\": 64}";
  std::vector<uint8_t> ConfigData(Config.begin(), Config.end());
  BuilderPtr = LoadEntryPtr;
  writeFatPointer(MemInst, StorePtr, WeightRead.size(), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + WeightRead.size(), ConfigData.size(),
                  BuilderPtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr);
  writeBinaries<uint8_t>(MemInst, ConfigData, StorePtr + WeightRead.size());
  StorePtr += WeightRead.size() + ConfigData.size();
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), static_cast<uint32_t>(Backend::MLX),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // MLX WASI-NN init_execution_context tests.
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }

  // Test: init_execution_context -- initialize the second context.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }

  // MLX WASI-NN set_input tests.
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
  // Test: set_input -- set input successfully.
  {
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // MLX WASI-NN compute tests.
  // Test: compute -- context id exceeds.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: compute -- compute successfully.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // MLX WASI-NN get_output tests.
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
  // Test: get_output -- get output successfully.
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    // Should output more than 50 bytes.
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GE(BytesWritten, 50);
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
TEST(WasiNNTest, BitNetBackend) {
  // Create the wasi_nn module instance.
  auto NNMod = createModule();
  ASSERT_TRUE(NNMod);

  // Create the calling frame with a memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // --- Get host functions ---
  // Get the function "load".
  auto *FuncInst = NNMod->findFuncExports("load");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncLoad = FuncInst->getHostFunc();
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncInit = FuncInst->getHostFunc();
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncSetInput = FuncInst->getHostFunc();
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncGetOutput = FuncInst->getHostFunc();
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncCompute = FuncInst->getHostFunc();
  // Get the function "unload".
  FuncInst = NNMod->findFuncExports("unload");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncUnload = FuncInst->getHostFunc();
  // Get the function "compute_single".
  FuncInst = NNMod->findFuncExports("compute_single");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncComputeSingle = FuncInst->getHostFunc();
  // Get the function "get_output_single".
  FuncInst = NNMod->findFuncExports("get_output_single");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncGetOutputSingle = FuncInst->getHostFunc();
  // Get the function "fini_single".
  FuncInst = NNMod->findFuncExports("fini_single");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncFiniSingle = FuncInst->getHostFunc();

  // --- Test Data & Pointer Setup ---
  const std::string ModelPath = "./wasinn_bitnet_fixtures/ggml-model-i2_s.gguf";
  const std::string ModelPreloadStr = "preload:" + ModelPath;
  const std::string MetadataStr = R"({"n-predict": 128})";
  const std::string InvalidBatchSizeMetadataStr =
      R"({"batch-size": 4294967295})";
  const std::string Prompt = "Once upon a time, ";

  uint32_t BuilderPtr = 0;
  uint32_t LoadEntryPtr = 0;
  uint32_t SetInputEntryPtr = 0;
  uint32_t StorePtr = 65536;
  const uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  std::array<WasmEdge::ValVariant, 1> Errno = {0};
  uint32_t GraphId = 0;
  uint32_t CtxId = 0;

  auto WriteLoadBuilders = [&](const std::string &Metadata) {
    std::vector<uint8_t> ModelPreloadVec(ModelPreloadStr.begin(),
                                         ModelPreloadStr.end());
    std::vector<uint8_t> MetadataVec(Metadata.begin(), Metadata.end());
    BuilderPtr = LoadEntryPtr;
    writeFatPointer(MemInst, StorePtr, ModelPreloadVec.size(), BuilderPtr);
    writeFatPointer(MemInst, StorePtr + ModelPreloadVec.size(),
                    MetadataVec.size(), BuilderPtr);
    writeBinaries<uint8_t>(MemInst, ModelPreloadVec, StorePtr);
    writeBinaries<uint8_t>(MemInst, MetadataVec,
                           StorePtr + ModelPreloadVec.size());
  };

  // BitNet WASI-NN load tests
  // Test: load -- empty builder array.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, 0, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- graph builder ptr out of bounds.
  {
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutBoundPtr, 1, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- model bin ptr out of bounds.
  {
    BuilderPtr = LoadEntryPtr;
    writeFatPointer(MemInst, OutBoundPtr, 10, BuilderPtr);
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, 1, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- invalid metadata encoding.
  {
    const std::string InvalidMetadataStr = R"({"n-predict": "not-a-number")";
    std::vector<uint8_t> ModelPreloadVec(ModelPreloadStr.begin(),
                                         ModelPreloadStr.end());
    std::vector<uint8_t> InvalidMetadataVec(InvalidMetadataStr.begin(),
                                            InvalidMetadataStr.end());
    BuilderPtr = LoadEntryPtr;
    writeFatPointer(MemInst, StorePtr, ModelPreloadVec.size(), BuilderPtr);
    writeFatPointer(MemInst, StorePtr + ModelPreloadVec.size(),
                    InvalidMetadataVec.size(), BuilderPtr);
    writeBinaries<uint8_t>(MemInst, ModelPreloadVec, StorePtr);
    writeBinaries<uint8_t>(MemInst, InvalidMetadataVec,
                           StorePtr + ModelPreloadVec.size());
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, 2, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidEncoding));
  }
  // Test: load -- invalid batch-size metadata.
  {
    WriteLoadBuilders(InvalidBatchSizeMetadataStr);
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, 2, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- invalid tensor-split metadata.
  {
    std::string TensorSplitMetadataStr = R"({"tensor-split": ")";
    constexpr size_t TooManyTensorSplits = 256;
    for (size_t I = 0; I < TooManyTensorSplits; I++) {
      if (I > 0) {
        TensorSplitMetadataStr += ",";
      }
      TensorSplitMetadataStr += "1";
    }
    TensorSplitMetadataStr += R"("})";
    WriteLoadBuilders(TensorSplitMetadataStr);
    EXPECT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, 2, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: load -- load successfully.
  {
    WriteLoadBuilders(MetadataStr);
    StorePtr += ModelPreloadStr.size() + MetadataStr.size();
    ASSERT_TRUE(HostFuncLoad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, 2, static_cast<uint32_t>(Backend::BitNet),
            static_cast<uint32_t>(Device::CPU), BuilderPtr},
        Errno))
        << "Load failed. Ensure model file exists at: " << ModelPath;
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    GraphId = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    BuilderPtr += 4;
  }

  // BitNet WASI-NN init_execution_context tests
  // Test: init_execution_context -- graph id invalid.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{999, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: init_execution_context -- initialize context successfully.
  {
    ASSERT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{GraphId, BuilderPtr},
        Errno));
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    CtxId = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    BuilderPtr += 4;
  }

  // BitNet WASI-NN set_input tests
  SetInputEntryPtr = BuilderPtr;
  {
    std::vector<uint8_t> PromptData(Prompt.begin(), Prompt.end());
    std::vector<uint32_t> PromptDim = {
        static_cast<uint32_t>(PromptData.size())};

    writeFatPointer(MemInst, StorePtr, PromptDim.size(), BuilderPtr);
    writeUInt32(MemInst, static_cast<uint32_t>(TensorType::U8), BuilderPtr);
    writeFatPointer(MemInst, StorePtr + PromptDim.size() * 4, PromptData.size(),
                    BuilderPtr);
    writeBinaries<uint32_t>(MemInst, PromptDim, StorePtr);
    writeBinaries<uint8_t>(MemInst, PromptData,
                           StorePtr + PromptDim.size() * 4);
  }
  // Test: set_input -- invalid context id.
  {
    EXPECT_TRUE(HostFuncSetInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{999, 0, SetInputEntryPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: set_input -- invalid tensor index.
  {
    EXPECT_TRUE(HostFuncSetInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CtxId, 2, SetInputEntryPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: set_input -- set input successfully.
  {
    ASSERT_TRUE(HostFuncSetInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CtxId, 0, SetInputEntryPtr},
        Errno));
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  // BitNet WASI-NN compute and get_output tests
  // Test: compute -- invalid context ID.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{999}, Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: compute -- compute successfully.
  {
    ASSERT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CtxId}, Errno));
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  // Test: get_output -- output buffer pointer out of bounds.
  {
    EXPECT_TRUE(
        HostFuncGetOutput.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  CtxId, 0, OutBoundPtr, 5, BuilderPtr},
                              Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: get_output -- bytes written pointer out of bounds.
  {
    EXPECT_TRUE(
        HostFuncGetOutput.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  CtxId, 0, StorePtr, 5, OutBoundPtr},
                              Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: get_output -- a zero-size probe reports the required size through
  // TooLarge, then a correctly sized buffer succeeds.
  {
    uint32_t BytesNeeded = 0;
    ASSERT_TRUE(
        HostFuncGetOutput.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  CtxId, 0, StorePtr, 0, BuilderPtr},
                              Errno));
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::TooLarge));
    BytesNeeded = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GT(BytesNeeded, 10);
    ASSERT_TRUE(
        HostFuncGetOutput.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  CtxId, 0, StorePtr, BytesNeeded, BuilderPtr},
                              Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), BytesNeeded);
  }

  // BitNet compute_single tests
  {
    std::string FullStreamedOutput = "";
    const int MaxStreamTokens = 20;

    // Test: set_input -- set prompt to start a new streaming sequence.
    {
      ASSERT_TRUE(
          HostFuncSetInput.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   CtxId, 0, SetInputEntryPtr},
                               Errno));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    }

    // Test: compute_single and get_output_single in a loop.
    for (int i = 0; i < MaxStreamTokens; ++i) {
      ASSERT_TRUE(HostFuncComputeSingle.run(
          CallFrame, std::initializer_list<WasmEdge::ValVariant>{CtxId},
          Errno));
      if (Errno[0].get<int32_t>() ==
          static_cast<uint32_t>(ErrNo::EndOfSequence)) {
        break;
      }
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));

      uint32_t SingleTokenBytes = 0;
      ASSERT_TRUE(HostFuncGetOutputSingle.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{CtxId, 0, StorePtr, 32,
                                                      BuilderPtr},
          Errno));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      SingleTokenBytes = *MemInst.getPointer<uint32_t *>(BuilderPtr);
      if (SingleTokenBytes > 0) {
        auto TokenSpan = *MemInst.getBytes(StorePtr, SingleTokenBytes);
        FullStreamedOutput += std::string(
            reinterpret_cast<const char *>(TokenSpan.data()), TokenSpan.size());
      }
    }
    EXPECT_GT(FullStreamedOutput.length(), 10);

    // Test: fini_single -- finalize the streaming session.
    {
      ASSERT_TRUE(HostFuncFiniSingle.run(
          CallFrame, std::initializer_list<WasmEdge::ValVariant>{CtxId},
          Errno));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    }
  }

  // BitNet WASI-NN unload tests.
  // Test: unload -- invalid graph id.
  {
    EXPECT_TRUE(HostFuncUnload.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{999}, Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // Test: unload -- unload successfully and verify.
  {
    ASSERT_TRUE(HostFuncUnload.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{GraphId},
        Errno));
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{GraphId, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
