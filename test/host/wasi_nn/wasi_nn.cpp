// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC
#include "common/defines.h"
#include <gtest/gtest.h>

#include "host/wasi_nn/wasinnenv.h"
#include "host/wasi_nn/wasinnfunc.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <vector>

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
using WasmEdge::Host::WASINN::ErrNo;
namespace {

inline std::vector<uint8_t> readBinariesFromDisk(const std::string file_path) {
  std::ifstream instream(file_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)),
                            std::istreambuf_iterator<char>());
  return data;
}

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   std::vector<T> Binaries, uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(), MemInst.getPointer<T *>(Ptr));
}

void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 uint32_t value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  *BufPtr = value;
  Ptr += 4;
}

void writeFatPointer(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t PtrVal, uint32_t PtrSize, uint32_t &Ptr) {
  writeUInt32(MemInst, PtrVal, Ptr);
  writeUInt32(MemInst, PtrSize, Ptr);
}

template <typename T>
std::vector<size_t> classSort(const std::vector<T> &array) {
  std::vector<size_t> indices(array.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(),
            [&array](int left, int right) -> bool {
              // sort indices according to corresponding array element
              return array[left] >= array[right];
            });

  return indices;
}
} // namespace

TEST(WasiNNTest, OpenVINOBackend) {
  WasmEdge::Host::WASINN::WasiNNEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(400));
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  std::vector<uint8_t> XmlRead, WeightRead, TensorData;
  std::vector<uint32_t> TensorDim{1, 3, 224, 224};
  uint32_t BuilderPtr = UINT32_C(0);
  uint32_t LoadEntryPtr;
  uint32_t SetInputEntryPtr;
  uint32_t StorePtr = UINT32_C(65536);

  LoadEntryPtr = BuilderPtr;
  XmlRead = readBinariesFromDisk("./wasinn_openvino_fixtures/mobilenet.xml");
  WeightRead = readBinariesFromDisk("./wasinn_openvino_fixtures/mobilenet.bin");

  writeFatPointer(MemInst, StorePtr, XmlRead.size(), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + XmlRead.size(), WeightRead.size(),
                  BuilderPtr);

  // wasi-nn load test
  WasmEdge::Host::WasiNNLoad WasiNNLoad(Env);
  // meaningless binaries
  {
    EXPECT_TRUE(WasiNNLoad.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Busy));
  }
  writeBinaries<uint8_t>(MemInst, XmlRead, StorePtr);
  writeBinaries<uint8_t>(MemInst, WeightRead, StorePtr + XmlRead.size());
  StorePtr += (XmlRead.size() + WeightRead.size());

  // wrong builders' length
  {
    EXPECT_TRUE(WasiNNLoad.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(4), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // unsupported device. CPU 0, GPU 1, TPU 2
  {
    EXPECT_TRUE(WasiNNLoad.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(3), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // successful load
  {
    EXPECT_TRUE(WasiNNLoad.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }
  // second graph
  {
    EXPECT_TRUE(WasiNNLoad.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            LoadEntryPtr, UINT32_C(2), UINT32_C(0), UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // wasi-nn init_exec_ctx test
  WasmEdge::Host::WasiNNInitExecCtx WasiNNInitExecCtx(Env);
  // graph id exceeds
  {
    EXPECT_TRUE(WasiNNInitExecCtx.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(2), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  WasmEdge::Host::WASINN::WasiNNEnvironment EmptyEnv;
  EmptyEnv.NNGraph.emplace_back(WasmEdge::Host::WASINN::Backend::OpenVINO);
  WasmEdge::Host::WasiNNInitExecCtx EmptyWasiNNInitExecCtx(EmptyEnv);
  // graph id exceeds
  {
    EXPECT_TRUE(EmptyWasiNNInitExecCtx.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::MissingMemory));
  }
  // successful init ctx
  {
    EXPECT_TRUE(WasiNNInitExecCtx.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 0);
    BuilderPtr += 4;
  }
  // second ctx init
  {
    EXPECT_TRUE(WasiNNInitExecCtx.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 1);
    BuilderPtr += 4;
  }

  // wasi-nn setinput test
  SetInputEntryPtr = BuilderPtr;
  TensorData = readBinariesFromDisk("./wasinn_openvino_fixtures/tensor.bgr");
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
  writeBinaries<uint8_t>(MemInst, TensorData, StorePtr + TensorDim.size() * 4);
  EmptyEnv.NNContext.emplace_back(EmptyEnv.NNGraph[0], nullptr);
  WasmEdge::Host::WasiNNSetInput EmptyWasiNNSetInput(EmptyEnv);
  // context id exceed
  {
    EXPECT_TRUE(
        EmptyWasiNNSetInput.run(&MemInst,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    UINT32_C(3), UINT32_C(0), SetInputEntryPtr},
                                Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // empty ctx
  {
    EXPECT_TRUE(
        EmptyWasiNNSetInput.run(&MemInst,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                                Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::MissingMemory));
  }
  WasmEdge::Host::WasiNNSetInput WasiNNSetInput(Env);
  // input index exceed
  {
    EXPECT_TRUE(
        WasiNNSetInput.run(&MemInst,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(0), UINT32_C(10), SetInputEntryPtr},
                           Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // tensor type not FP32
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(2), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        WasiNNSetInput.run(&MemInst,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(0), UINT32_C(0), SetInputEntryPtr},
                           Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // successful run
  BuilderPtr = SetInputEntryPtr;
  writeFatPointer(MemInst, StorePtr, TensorDim.size(), BuilderPtr);
  writeUInt32(MemInst, UINT32_C(1), BuilderPtr);
  writeFatPointer(MemInst, StorePtr + TensorDim.size() * 4, TensorData.size(),
                  BuilderPtr);
  {
    EXPECT_TRUE(
        WasiNNSetInput.run(&MemInst,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                           Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  StorePtr += (TensorDim.size() * 4 + TensorData.size());

  // compute test
  WasmEdge::Host::WasiNNCompute WasiNNCompute(Env);
  WasmEdge::Host::WasiNNCompute EmptyWasiNNCompute(EmptyEnv);
  // context id exceed
  {
    EXPECT_TRUE(WasiNNCompute.run(
        &MemInst, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // empty ctx compute
  {
    EXPECT_TRUE(EmptyWasiNNCompute.run(
        &MemInst, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Busy));
  }
  // successful run
  {
    EXPECT_TRUE(WasiNNCompute.run(
        &MemInst, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }

  WasmEdge::Host::WasiNNGetOuput WasiNNGetOuput(Env);
  // WasmEdge::Host::WasiNNGetOuput EmptyWasiNNGetOuput(EmptyEnv);
  // no compute ctx request
  {
    EXPECT_TRUE(WasiNNGetOuput.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    // FIXME should allow a request for output with no computation before?
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), UINT32_C(4004));
  }
  // output index exceed
  {
    EXPECT_TRUE(WasiNNGetOuput.run(
        &MemInst,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(10), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
  }
  // successful run
  {
    EXPECT_TRUE(WasiNNGetOuput.run(
        &MemInst,
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
    // the probability of class i is placed at buffer[i]
    for (size_t I = 0; I < CorrectClasses.size(); I++) {
      EXPECT_EQ(SortedIndex[I], CorrectClasses[I]);
    }
  }
}
#endif // WASMEDGE_WASINN_BUILD_OPENVINO