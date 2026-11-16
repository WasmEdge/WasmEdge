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
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS
TEST(WasiNNTest, ChatTTSBackend) {
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

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
