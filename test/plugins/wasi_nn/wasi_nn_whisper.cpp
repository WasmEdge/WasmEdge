// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "helper.h"

#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {

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

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
