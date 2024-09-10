// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llmc_func.h"
#include "llmc_module.h"

#include "common/defines.h"
#include "common/types.h"
#include "plugin/plugin.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

using WasmEdge::Host::WasmEdgeLLMC::ErrNo;

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

std::unique_ptr<WasmEdge::Host::WasmEdgeLLMCModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_llmc/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeLLMC" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_llmc"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_llmc"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeLLMCModule>(
          Module->create());
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

TEST(WasmEdgeLLMTest, TrainGPT2) {
  // Create wasmedge_llmc module instance.
  auto LLMCMod = createModule();
  ASSERT_TRUE(LLMCMod);
  EXPECT_EQ(LLMCMod->getFuncExportNum(), 4U);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(60000)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  EXPECT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  auto *ModelCreate = LLMCMod->findFuncExports("model_create");
  EXPECT_NE(ModelCreate, nullptr);
  EXPECT_TRUE(ModelCreate->isHostFunction());
  auto &HostFuncModelCreate =
      dynamic_cast<WasmEdge::Host::WasmEdgeLLMC::ModelCreate &>(
          ModelCreate->getHostFunc());

  auto *DataLoaderCreate = LLMCMod->findFuncExports("dataloader_create");
  EXPECT_NE(DataLoaderCreate, nullptr);
  EXPECT_TRUE(DataLoaderCreate->isHostFunction());
  auto &HostFuncDataLoadereCreate =
      dynamic_cast<WasmEdge::Host::WasmEdgeLLMC::DataLoaderCreate &>(
          DataLoaderCreate->getHostFunc());

  auto *TokenizerCreate = LLMCMod->findFuncExports("tokenizer_create");
  EXPECT_NE(TokenizerCreate, nullptr);
  EXPECT_TRUE(TokenizerCreate->isHostFunction());
  auto &HostFuncTokenizerCreate =
      dynamic_cast<WasmEdge::Host::WasmEdgeLLMC::TokenizerCreate &>(
          TokenizerCreate->getHostFunc());

  auto *ModelTrain = LLMCMod->findFuncExports("model_train");
  EXPECT_NE(ModelTrain, nullptr);
  EXPECT_TRUE(ModelTrain->isHostFunction());
  auto &HostFuncModelTrain =
      dynamic_cast<WasmEdge::Host::WasmEdgeLLMC::ModelTrain &>(
          ModelTrain->getHostFunc());

  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  std::string CheckPointString = "./wasmedge_llmc/gpt2_124M.bin";
  std::vector<char> CheckPointPath(CheckPointString.begin(),
                                   CheckPointString.end());
  uint32_t CheckPointPathPtr = UINT32_C(0);
  writeBinaries<char>(MemInst, CheckPointPath, CheckPointPathPtr);

  std::string TrainDataString = "./wasmedge_llmc/tiny_shakespeare_train.bin";
  std::vector<char> TrainDataPath(TrainDataString.begin(),
                                  TrainDataString.end());
  uint32_t TrainDataPathPtr = CheckPointPathPtr + CheckPointPath.size();
  writeBinaries<char>(MemInst, TrainDataPath, TrainDataPathPtr);

  std::string ValDataString = "./wasmedge_llmc/tiny_shakespeare_val.bin";
  std::vector<char> ValDataPath(ValDataString.begin(), ValDataString.end());
  uint32_t ValDataPathPtr = TrainDataPathPtr + TrainDataPath.size();
  writeBinaries<char>(MemInst, ValDataPath, ValDataPathPtr);

  std::string TokenizerBin = "./wasmedge_llmc/gpt2_tokenizer.bin";
  std::vector<char> TokenizerBinPath(TokenizerBin.begin(), TokenizerBin.end());
  uint32_t TokenizerBinPtr = ValDataPathPtr + ValDataPath.size();
  writeBinaries<char>(MemInst, TokenizerBinPath, TokenizerBinPtr);

  uint32_t ModelIdPtr = UINT32_C(0);
  uint32_t ModelId = UINT32_C(0);
  uint32_t TrainDataLoaderIdPtr = UINT32_C(0);
  uint32_t TrainDataLoaderId = UINT32_C(0);
  uint32_t ValDataLoaderIdPtr = UINT32_C(0);
  uint32_t ValDataLoaderId = UINT32_C(0);
  uint32_t TokenizerIdPtr = UINT32_C(0);
  uint32_t TokenizerId = UINT32_C(0);

  {
    EXPECT_TRUE(HostFuncModelCreate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            CheckPointPathPtr, static_cast<uint32_t>(CheckPointPath.size()),
            ModelIdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    ModelId = *MemInst.getPointer<uint32_t *>(ModelIdPtr);
    EXPECT_EQ(ModelId, 0);
  }

  {
    EXPECT_TRUE(HostFuncDataLoadereCreate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            TrainDataPathPtr, static_cast<uint32_t>(TrainDataPath.size()),
            /*B*/ 4,
            /*T*/ 64,
            /*ProcessRank*/ 0,
            /*NumProcesses*/ 1,
            /*ShouldShuffle*/ 1, TrainDataLoaderIdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    TrainDataLoaderId = *MemInst.getPointer<uint32_t *>(TrainDataLoaderIdPtr);
    EXPECT_EQ(TrainDataLoaderId, 0);
  }

  {
    EXPECT_TRUE(HostFuncDataLoadereCreate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            ValDataPathPtr, static_cast<uint32_t>(ValDataPath.size()),
            /*B*/ 4,
            /*T*/ 64,
            /*ProcessRank*/ 0,
            /*NumProcesses*/ 1,
            /*ShouldShuffle*/ 0, ValDataLoaderIdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    ValDataLoaderId = *MemInst.getPointer<uint32_t *>(ValDataLoaderIdPtr);
    EXPECT_EQ(ValDataLoaderId, 1);
  }

  {
    EXPECT_TRUE(HostFuncTokenizerCreate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            TokenizerBinPtr, static_cast<uint32_t>(TokenizerBinPath.size()),
            TokenizerIdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
    TokenizerId = *MemInst.getPointer<uint32_t *>(TokenizerIdPtr);
    EXPECT_EQ(TokenizerId, 0);
  }

  {

    EXPECT_TRUE(HostFuncModelTrain.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            ModelId, TrainDataLoaderId, ValDataLoaderId, TokenizerId,
            /*B*/ 4,
            /*T*/ 64,
            /*Lr*/ 1e-4f,
            /*Epoch*/ 20},
        Errno));
  }
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
