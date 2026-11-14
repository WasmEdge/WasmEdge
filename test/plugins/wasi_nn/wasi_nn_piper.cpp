// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "helper.h"
#include "wasinn_piper.h"

#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
TEST(WasiNNTest, PiperBackend) {
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

  // Loads a Piper graph from the given config and returns the errno.
  auto LoadPiperConfig = [&](const std::string &PiperConfig) {
    std::vector<uint8_t> Data(PiperConfig.begin(), PiperConfig.end());
    BuilderPtr = LoadEntryPtr;
    writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(Data.size()),
                    BuilderPtr);
    writeBinaries<uint8_t>(MemInst, Data, StorePtr);
    EXPECT_TRUE(HostFuncLoad.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     LoadEntryPtr, UINT32_C(1),
                                     static_cast<uint32_t>(Backend::Piper),
                                     UINT32_C(0), BuilderPtr},
                                 Errno));
    return static_cast<uint32_t>(Errno[0].get<int32_t>());
  };

  // Test: load -- the model is not an ONNX file (onnxruntime throws).
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx.json\", "
                "\"config\": \"./wasinn_piper_fixtures/test_voice.onnx.json\", "
                "\"espeak_data\": \"./wasinn_piper_fixtures/espeak-ng-data\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- the voice config is not JSON (libpiper throws).
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"config\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"espeak_data\": \"./wasinn_piper_fixtures/espeak-ng-data\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- the model path is rejected by the OS (NAME_MAX exceeded).
  EXPECT_EQ(LoadPiperConfig("{\"model\": \"./wasinn_piper_fixtures/" +
                            std::string(300, 'a') + "\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- the voice config is a directory.
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"config\": \"./wasinn_piper_fixtures\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- espeak_data does not exist.
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"espeak_data\": \"./wasinn_piper_fixtures/missing\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- espeak_data is a file.
  EXPECT_EQ(
      LoadPiperConfig(
          "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
          "\"espeak_data\": \"./wasinn_piper_fixtures/test_voice.onnx\"}"),
      static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- espeak_data is a directory without espeak-ng data.
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"espeak_data\": \".\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- espeak_data is longer than espeak-ng's path buffer.
  constexpr auto ESpeakPathLimit =
      WasmEdge::Host::WASINN::Piper::ESpeakMaxDataPathSize;
  std::string LongESpeakData = "./wasinn_piper_fixtures/espeak-ng-data";
  while (LongESpeakData.size() <= ESpeakPathLimit) {
    LongESpeakData += "/.";
  }
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"espeak_data\": \"" +
                LongESpeakData + "\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- espeak_data names a parent directory that fits the buffer
  // only without the "/espeak-ng-data" suffix espeak-ng appends.
  std::string LongParentESpeakData = "./wasinn_piper_fixtures";
  while (LongParentESpeakData.size() + 14 < ESpeakPathLimit) {
    LongParentESpeakData += "/.";
  }
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"espeak_data\": \"" +
                LongParentESpeakData + "\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: load -- synthesis options out of range.
  for (const char *Option : {"\"speaker_id\": -1", "\"speaker_id\": 4294967296",
                             "\"length_scale\": 0", "\"length_scale\": 11",
                             "\"noise_w\": -11", "\"noise_scale\": 1e300"}) {
    EXPECT_EQ(
        LoadPiperConfig(
            "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
            "\"espeak_data\": \"./wasinn_piper_fixtures/espeak-ng-data\", " +
            std::string(Option) + "}"),
        static_cast<uint32_t>(ErrNo::InvalidArgument))
        << Option;
  }

  // Test: load -- load successfully.
  std::string Config =
      "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
      "\"espeak_data\": \"./wasinn_piper_fixtures/espeak-ng-data\"}";
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
  Config = "{\"model\": "
           "\"./wasinn_piper_fixtures/test_voice.onnx\",\"espeak_data\": "
           "\"./wasinn_piper_fixtures/espeak-ng-data\",\"json_input\":true}";
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

  // Sets a JSON input on context 1 and returns the errno.
  auto SetJsonInput = [&](const std::string &Json) {
    std::vector<uint8_t> Data(Json.begin(), Json.end());
    auto DimBytes = static_cast<uint32_t>(
        TensorDim.size() * sizeof(decltype(TensorDim)::value_type));
    SetInputEntryPtr = BuilderPtr;
    writeFatPointer(MemInst, StorePtr, static_cast<uint32_t>(TensorDim.size()),
                    BuilderPtr);
    writeUInt32(MemInst, 3, BuilderPtr);
    writeFatPointer(MemInst, StorePtr + DimBytes,
                    static_cast<uint32_t>(Data.size()), BuilderPtr);
    writeBinaries<uint32_t>(MemInst, TensorDim, StorePtr);
    writeBinaries<uint8_t>(MemInst, Data, StorePtr + DimBytes);
    EXPECT_TRUE(
        HostFuncSetInput.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(1), UINT32_C(0), SetInputEntryPtr},
                             Errno));
    StorePtr += DimBytes + static_cast<uint32_t>(Data.size());
    return static_cast<uint32_t>(Errno[0].get<int32_t>());
  };

  // Test: set_input -- JSON input without the text field.
  EXPECT_EQ(SetJsonInput("{\"speaker_id\": 0}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: set_input -- JSON input with a wrongly typed option.
  EXPECT_EQ(SetJsonInput("{\"text\": \"x\", \"length_scale\": \"fast\"}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: set_input -- JSON input that is not an object.
  EXPECT_EQ(SetJsonInput("[\"This is a test.\"]"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: set_input -- JSON input with synthesis options out of range.
  EXPECT_EQ(
      SetJsonInput("{\"text\": \"This is a test.\", \"length_scale\": -1}"),
      static_cast<uint32_t>(ErrNo::InvalidArgument));
  EXPECT_EQ(SetJsonInput("{\"text\": \"This is a test.\", \"noise_w\": 1e300}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));
  EXPECT_EQ(
      SetJsonInput("{\"text\": \"This is a test.\", \"length_scale\": 11}"),
      static_cast<uint32_t>(ErrNo::InvalidArgument));
  EXPECT_EQ(SetJsonInput("{\"text\": \"This is a test.\", \"noise_w\": 11}"),
            static_cast<uint32_t>(ErrNo::InvalidArgument));

  // Test: compute -- a rejected input leaves the previous one in effect.
  {
    EXPECT_TRUE(HostFuncCompute.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
  }
  {
    EXPECT_TRUE(HostFuncGetOutput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(1), UINT32_C(0), StorePtr, 65532, BuilderPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 44100);
  }

  // Test: load -- espeak_data may name the parent of espeak-ng-data.
  EXPECT_EQ(LoadPiperConfig(
                "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\", "
                "\"espeak_data\": \"./wasinn_piper_fixtures\"}"),
            static_cast<uint32_t>(ErrNo::Success));
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(BuilderPtr), 2);

  // Test: load -- no espeak_data and espeak-ng's own lookup finds no data.
  {
    const char *SavedPath = std::getenv("ESPEAK_DATA_PATH");
    std::string Saved = SavedPath != nullptr ? SavedPath : "";
    setenv("ESPEAK_DATA_PATH", ".", 1);
    EXPECT_EQ(LoadPiperConfig(
                  "{\"model\": \"./wasinn_piper_fixtures/test_voice.onnx\"}"),
              static_cast<uint32_t>(ErrNo::InvalidArgument));
    if (SavedPath != nullptr) {
      setenv("ESPEAK_DATA_PATH", Saved.c_str(), 1);
    } else {
      unsetenv("ESPEAK_DATA_PATH");
    }
  }
}
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
