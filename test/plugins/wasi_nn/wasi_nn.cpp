// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinnfunc.h"
#include "wasinnmodule.h"

#include "common/types.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <numeric>
#include <string>
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
              // Sort indices according to corresponding array element.
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
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

  // Test: load -- wrong builders' length.
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
  NNGraphTmp.emplace_back(Backend::OpenVINO);
  NNGraphTmp.back().setReady();
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
  writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                  BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst,
                  StorePtr + static_cast<uint32_t>(TensorDim.size()) * 4,
                  static_cast<uint32_t>(TensorData.size()), BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);

  // Swap to the tmp. env.
  NNContextTmp.emplace_back(0, NNGraphTmp[0]);
  NNContextTmp.back().setReady();
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

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

  // Test: load -- wrong builders' length.
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
  NNGraphTmp.emplace_back(Backend::PyTorch);
  NNGraphTmp.back().setReady();
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

  NNContextTmp.emplace_back(0, NNGraphTmp[0]);
  NNContextTmp.back().setReady();

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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
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

  // Test: load -- wrong builders' length.
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
  NNGraphTmp.emplace_back(Backend::TensorflowLite);
  NNGraphTmp.back().setReady();
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

  NNContextTmp.emplace_back(0, NNGraphTmp[0]);
  NNContextTmp.back().setReady();

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
                          ? "./wasinn_ggml_fixtures/orca_mini.gguf"
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

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
      --nn-preload=default:GGML:AUTO:${DIR}/test/plugins/wasi_nn/wasinn_ggml_fixtures/orca_mini.gguf
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
  auto &HostFuncLoadByName =
      dynamic_cast<WasmEdge::Host::WasiNNLoadByName &>(FuncInst->getHostFunc());
  // Get the function "load_by_name_with_config".
  FuncInst = NNMod->findFuncExports("load_by_name_with_config");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoadByNameWithConfig =
      dynamic_cast<WasmEdge::Host::WasiNNLoadByNameWithConfig &>(
          FuncInst->getHostFunc());
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

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
      --nn-preload=default:GGML:AUTO:${DIR}/test/plugins/wasi_nn/wasinn_ggml_fixtures/orca_mini.gguf
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
  auto &HostFuncLoadByName =
      dynamic_cast<WasmEdge::Host::WasiNNLoadByName &>(FuncInst->getHostFunc());
  // Get the function "load_by_name_with_config".
  FuncInst = NNMod->findFuncExports("load_by_name_with_config");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncLoadByNameWithConfig =
      dynamic_cast<WasmEdge::Host::WasiNNLoadByNameWithConfig &>(
          FuncInst->getHostFunc());
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
  FuncInst = NNMod->findFuncExports("get_output_single");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetOutputSingle =
      dynamic_cast<WasmEdge::Host::WasiNNGetOutputSingle &>(
          FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute_single");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncComputeSingle =
      dynamic_cast<WasmEdge::Host::WasiNNComputeSingle &>(
          FuncInst->getHostFunc());

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
      readEntireFile("./wasinn_whisper_fixtures/ggml-base.bin");
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

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

  // Test: init_execution_context -- init second context.
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

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

  // Test: init_execution_context -- init context successfully.
  {
    EXPECT_TRUE(HostFuncInit.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // First json input with parameters overridden
  Text = "{\"text\": \"This is a test.\", \"noise_scale\": 0.0, "
         "\"length_scale\": 2.0, \"noise_w\": 0.0, \"sentence_silence\": 1.0, "
         "\"phoneme_silence\": {\"t\": 0.0}}";
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
    // Should output more than 50000 bytes.
    EXPECT_GE(BytesWritten, 50000);
  }

  // Second json input to check if one-time overriding is working properly
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
    // Should output less than 40000 bytes.
    EXPECT_LT(BytesWritten, 40000);
    EXPECT_EQ(BytesWritten, 34048);
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());
  // Get the function "unload".
  FuncInst = NNMod->findFuncExports("unload");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncUnload =
      dynamic_cast<WasmEdge::Host::WasiNNUnload &>(FuncInst->getHostFunc());

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

  // Test: init_execution_context -- init second context.
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
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());

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
  std::string Config =
      "{\"model_type\":\"tiny_llama_1.1B_chat_v1.0\", "
      "\"tokenizer\":\"" +
      Tokenizer +
      "\", \"q_bits\": 4, \"group_size\": 128, \"is_quantized\": false}";
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

  // Test: init_execution_context -- init second context.
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
  auto &HostFuncLoad =
      dynamic_cast<WasmEdge::Host::WasiNNLoad &>(FuncInst->getHostFunc());
  // Get the function "init_execution_context".
  FuncInst = NNMod->findFuncExports("init_execution_context");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncInit = dynamic_cast<WasmEdge::Host::WasiNNInitExecCtx &>(
      FuncInst->getHostFunc());
  // Get the function "set_input".
  FuncInst = NNMod->findFuncExports("set_input");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncSetInput =
      dynamic_cast<WasmEdge::Host::WasiNNSetInput &>(FuncInst->getHostFunc());
  // Get the function "get_output".
  FuncInst = NNMod->findFuncExports("get_output");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncGetOutput =
      dynamic_cast<WasmEdge::Host::WasiNNGetOutput &>(FuncInst->getHostFunc());
  // Get the function "compute".
  FuncInst = NNMod->findFuncExports("compute");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncCompute =
      dynamic_cast<WasmEdge::Host::WasiNNCompute &>(FuncInst->getHostFunc());
  // Get the function "unload".
  FuncInst = NNMod->findFuncExports("unload");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncUnload =
      dynamic_cast<WasmEdge::Host::WasiNNUnload &>(FuncInst->getHostFunc());
  // Get the function "compute_single".
  FuncInst = NNMod->findFuncExports("compute_single");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncComputeSingle =
      dynamic_cast<WasmEdge::Host::WasiNNComputeSingle &>(
          FuncInst->getHostFunc());
  // Get the function "get_output_single".
  FuncInst = NNMod->findFuncExports("get_output_single");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncGetOutputSingle =
      dynamic_cast<WasmEdge::Host::WasiNNGetOutputSingle &>(
          FuncInst->getHostFunc());
  // Get the function "fini_single".
  FuncInst = NNMod->findFuncExports("fini_single");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncFiniSingle =
      dynamic_cast<WasmEdge::Host::WasiNNFiniSingle &>(FuncInst->getHostFunc());

  // --- Test Data & Pointer Setup ---
  const std::string ModelPath = "./wasinn_bitnet_fixtures/ggml-model-i2_s.gguf";
  const std::string ModelPreloadStr = "preload:" + ModelPath;
  const std::string MetadataStr = R"({"n-predict": 128})";
  const std::string Prompt = "Once upon a time, ";

  uint32_t BuilderPtr = 0;
  uint32_t LoadEntryPtr = 0;
  uint32_t SetInputEntryPtr = 0;
  uint32_t StorePtr = 65536;
  const uint32_t OutBoundPtr = UINT32_C(61000) * UINT32_C(65536);
  std::array<WasmEdge::ValVariant, 1> Errno = {0};
  uint32_t GraphId = 0;
  uint32_t CtxId = 0;

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
  // Test: load -- load successfully.
  {
    std::vector<uint8_t> ModelPreloadVec(ModelPreloadStr.begin(),
                                         ModelPreloadStr.end());
    std::vector<uint8_t> MetadataVec(MetadataStr.begin(), MetadataStr.end());
    BuilderPtr = LoadEntryPtr;
    writeFatPointer(MemInst, StorePtr, ModelPreloadVec.size(), BuilderPtr);
    writeFatPointer(MemInst, StorePtr + ModelPreloadVec.size(),
                    MetadataVec.size(), BuilderPtr);
    writeBinaries<uint8_t>(MemInst, ModelPreloadVec, StorePtr);
    writeBinaries<uint8_t>(MemInst, MetadataVec,
                           StorePtr + ModelPreloadVec.size());
    StorePtr += ModelPreloadVec.size() + MetadataVec.size();
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
  // Test: init_execution_context -- init context successfully.
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
  // Test: get_output -- get output successfully.
  {
    uint32_t BytesNeeded = 0;
    ASSERT_TRUE(
        HostFuncGetOutput.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  CtxId, 0, StorePtr, 0, BuilderPtr},
                              Errno));
    ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    BytesNeeded = *MemInst.getPointer<uint32_t *>(BuilderPtr);
    EXPECT_GT(BytesNeeded, 10);
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