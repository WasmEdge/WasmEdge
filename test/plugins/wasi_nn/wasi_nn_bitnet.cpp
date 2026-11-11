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
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {

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

  // BitNet WASI-NN set_input metadata tests
  {
    auto Graph = NNMod->getEnv().NNGraph.get(GraphId);
    ASSERT_NE(Graph, nullptr);
    const auto &BitNetGraph =
        Graph->get<WasmEdge::Host::WASINN::BitNet::Graph>();
    const auto &Params = BitNetGraph.Params;
    auto Context = NNMod->getEnv().NNContext.get(CtxId);
    ASSERT_NE(Context, nullptr);
    const auto &BitNetContext =
        Context->get<WasmEdge::Host::WASINN::BitNet::Context>();
    const std::vector<uint32_t> MetadataDim = {1};
    auto SetMetadata = [&](std::string_view Metadata) {
      std::vector<uint8_t> MetadataData(Metadata.begin(), Metadata.end());
      uint32_t EntryPtr = BuilderPtr;
      writeFatPointer(MemInst, StorePtr, UINT32_C(1), EntryPtr);
      writeUInt32(MemInst, static_cast<uint32_t>(TensorType::U8), EntryPtr);
      writeFatPointer(MemInst, StorePtr + UINT32_C(4),
                      static_cast<uint32_t>(MetadataData.size()), EntryPtr);
      writeBinaries<uint32_t>(MemInst, MetadataDim, StorePtr);
      writeBinaries<uint8_t>(MemInst, MetadataData, StorePtr + UINT32_C(4));
      return HostFuncSetInput.run(CallFrame,
                                  std::initializer_list<WasmEdge::ValVariant>{
                                      CtxId, UINT32_C(1), BuilderPtr},
                                  Errno);
    };

    // Test: set_input -- metadata that would overflow the fixed tensor-split
    // array, truncate n_batch / n_ubatch, or hand the bundled llama.cpp a
    // value it throws or aborts on is rejected and leaves the parameters
    // untouched.
    {
      const int32_t PrevNBatch = Params.n_batch;
      const int32_t PrevNUBatch = Params.n_ubatch;
      const int PrevNThreads = Params.cpuparams.n_threads;
      const int PrevNThreadsBatch = Params.cpuparams_batch.n_threads;
      const std::string PrevCacheTypeK = Params.cache_type_k;
      const std::string PrevCacheTypeV = Params.cache_type_v;
      const enum llama_pooling_type PrevPoolingType = Params.pooling_type;
      const int32_t PrevMirostat = Params.sparams.mirostat;
      std::string TooManySplits = R"({"tensor-split":")";
      for (size_t I = 0; I < 256; I++) {
        TooManySplits += I == 0 ? "1" : ",1";
      }
      TooManySplits += R"("})";
      const std::string InvalidCases[] = {
          TooManySplits,
          R"({"tensor-split":"1,abc"})",
          // Out of range for float: fails with eofbit also set.
          R"({"tensor-split":"1e100"})",
          R"({"tensor-split":"1,1e100"})",
          R"({"tensor-split":""})",
          R"({"batch-size":0})",
          R"({"batch-size":4294967295})",
          R"({"ubatch-size":-1})",
          R"({"ubatch-size":4294967296})",
          // llama_decode asserts n_threads > 0; -1 is not automatic here.
          R"({"threads":0})",
          R"({"threads":-1})",
          R"({"threads":513})",
          R"({"threads-batch":0})",
          R"({"threads-batch":-2})",
          R"({"threads-batch":513})",
          // kv_cache_type_from_str throws on anything but its own list.
          R"({"cache-type-k":"bf16"})",
          R"({"cache-type-k":"q4_k"})",
          R"({"cache-type-v":"abc"})",
          R"({"pooling-type":-2})",
          R"({"pooling-type":99})",
          // common_sampler_init asserts on unknown mirostat versions.
          R"({"mirostat":-1})",
          R"({"mirostat":3})",
      };
      for (const auto &Metadata : InvalidCases) {
        SCOPED_TRACE(Metadata);
        ASSERT_TRUE(SetMetadata(Metadata));
        EXPECT_EQ(Errno[0].get<int32_t>(),
                  static_cast<uint32_t>(ErrNo::InvalidArgument));
      }
      EXPECT_EQ(Params.n_batch, PrevNBatch);
      EXPECT_EQ(Params.n_ubatch, PrevNUBatch);
      EXPECT_EQ(Params.cpuparams.n_threads, PrevNThreads);
      EXPECT_EQ(Params.cpuparams_batch.n_threads, PrevNThreadsBatch);
      EXPECT_EQ(Params.cache_type_k, PrevCacheTypeK);
      EXPECT_EQ(Params.cache_type_v, PrevCacheTypeV);
      EXPECT_EQ(Params.pooling_type, PrevPoolingType);
      EXPECT_EQ(Params.sparams.mirostat, PrevMirostat);
      for (float Split : Params.tensor_split) {
        EXPECT_FLOAT_EQ(Split, 0.0f);
      }
    }

    // Test: set_input -- valid batch sizes are applied (the prompt batch is
    // reallocated on the change) and restored afterwards.
    {
      const int32_t PrevNBatch = Params.n_batch;
      const int32_t PrevNUBatch = Params.n_ubatch;
      ASSERT_TRUE(SetMetadata(R"({"batch-size":64,"ubatch-size":0})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.n_batch, 64);
      EXPECT_EQ(Params.n_ubatch, 0);
      EXPECT_EQ(BitNetContext.CurrentBatchSize, 64);
      EXPECT_NE(BitNetContext.LlamaBatch.token, nullptr);
      ASSERT_TRUE(SetMetadata(R"({"batch-size":)" + std::to_string(PrevNBatch) +
                              R"(,"ubatch-size":)" +
                              std::to_string(PrevNUBatch) + "}"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.n_batch, PrevNBatch);
      EXPECT_EQ(Params.n_ubatch, PrevNUBatch);
      EXPECT_EQ(BitNetContext.CurrentBatchSize, PrevNBatch);
    }

    // Test: set_input -- in-range KV cache types, pooling types, and mirostat
    // versions are applied and restored afterwards. None of them reloads the
    // llama context, so a V cache type that needs flash attention is fine.
    {
      const std::string PrevCacheTypeK = Params.cache_type_k;
      const std::string PrevCacheTypeV = Params.cache_type_v;
      const enum llama_pooling_type PrevPoolingType = Params.pooling_type;
      const int32_t PrevMirostat = Params.sparams.mirostat;
      ASSERT_TRUE(SetMetadata(R"({"cache-type-k":"q8_0","cache-type-v":"q4_0",)"
                              R"("pooling-type":1,"mirostat":2})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.cache_type_k, "q8_0");
      EXPECT_EQ(Params.cache_type_v, "q4_0");
      EXPECT_EQ(Params.pooling_type, LLAMA_POOLING_TYPE_MEAN);
      EXPECT_EQ(Params.sparams.mirostat, 2);
      ASSERT_TRUE(
          SetMetadata(R"({"cache-type-k":")" + PrevCacheTypeK +
                      R"(","cache-type-v":")" + PrevCacheTypeV +
                      R"(","pooling-type":)" + std::to_string(PrevPoolingType) +
                      R"(,"mirostat":)" + std::to_string(PrevMirostat) + "}"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.cache_type_k, PrevCacheTypeK);
      EXPECT_EQ(Params.cache_type_v, PrevCacheTypeV);
      EXPECT_EQ(Params.pooling_type, PrevPoolingType);
      EXPECT_EQ(Params.sparams.mirostat, PrevMirostat);
    }

    // Test: set_input -- in-range thread counts, including the threads-batch
    // -1 fallback, are applied through a successful llama context reload and
    // restored afterwards.
    {
      const int PrevNThreads = Params.cpuparams.n_threads;
      const int PrevNThreadsBatch = Params.cpuparams_batch.n_threads;
      ASSERT_TRUE(SetMetadata(R"({"threads":2,"threads-batch":-1})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.cpuparams.n_threads, 2);
      EXPECT_EQ(Params.cpuparams_batch.n_threads, -1);
      ASSERT_NE(BitNetGraph.LlamaContext, nullptr);
      ASSERT_TRUE(SetMetadata(R"({"threads":3,"threads-batch":1})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.cpuparams.n_threads, 3);
      EXPECT_EQ(Params.cpuparams_batch.n_threads, 1);
      ASSERT_NE(BitNetGraph.LlamaContext, nullptr);
      ASSERT_TRUE(SetMetadata(R"({"threads":)" + std::to_string(PrevNThreads) +
                              R"(,"threads-batch":)" +
                              std::to_string(PrevNThreadsBatch) + "}"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Params.cpuparams.n_threads, PrevNThreads);
      EXPECT_EQ(Params.cpuparams_batch.n_threads, PrevNThreadsBatch);
      ASSERT_NE(BitNetGraph.LlamaContext, nullptr);
    }

    // Test: set_input -- a valid tensor-split is applied and zero-padded,
    // with separators and surrounding whitespace tolerated.
    {
      ASSERT_TRUE(SetMetadata(R"({"tensor-split":"3,2"})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_FLOAT_EQ(Params.tensor_split[0], 3.0f);
      EXPECT_FLOAT_EQ(Params.tensor_split[1], 2.0f);
      EXPECT_FLOAT_EQ(Params.tensor_split[2], 0.0f);
      ASSERT_TRUE(SetMetadata(R"({"tensor-split":" 1.5 , 0.5, "})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_FLOAT_EQ(Params.tensor_split[0], 1.5f);
      EXPECT_FLOAT_EQ(Params.tensor_split[1], 0.5f);
      EXPECT_FLOAT_EQ(Params.tensor_split[2], 0.0f);
      ASSERT_TRUE(SetMetadata(R"({"tensor-split":"0"})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_FLOAT_EQ(Params.tensor_split[0], 0.0f);
      EXPECT_FLOAT_EQ(Params.tensor_split[1], 0.0f);
    }

    // Test: set_input -- embd-normalize reaches the context config that the
    // embedding path reads, and out-of-range values are rejected.
    {
      using WasmEdge::Host::WASINN::BitNet::EmbdNormalizeType;
      const auto &Conf = BitNetContext.Conf;
      const EmbdNormalizeType PrevEmbdNormalize = Conf.EmbdNormalize;
      EXPECT_EQ(PrevEmbdNormalize, EmbdNormalizeType::Euclidean);
      ASSERT_TRUE(SetMetadata(R"({"embd-normalize":-2})"));
      EXPECT_EQ(Errno[0].get<int32_t>(),
                static_cast<uint32_t>(ErrNo::InvalidArgument));
      EXPECT_EQ(Conf.EmbdNormalize, PrevEmbdNormalize);
      ASSERT_TRUE(SetMetadata(R"({"embd-normalize":1})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Conf.EmbdNormalize, EmbdNormalizeType::Taxicab);
      ASSERT_TRUE(SetMetadata(R"({"embd-normalize":2})"));
      ASSERT_EQ(Errno[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
      EXPECT_EQ(Conf.EmbdNormalize, PrevEmbdNormalize);
    }

    // Test: set_input -- a rejected metadata object is applied atomically:
    // the valid options that precede the invalid one are rolled back on the
    // graph settings, the graph parameters, and the context config.
    {
      using WasmEdge::Host::WASINN::BitNet::EmbdNormalizeType;
      const auto &Conf = BitNetContext.Conf;
      const bool PrevEnableLog = BitNetGraph.EnableLog;
      const int32_t PrevNCtx = Params.n_ctx;
      const int32_t PrevNBatch = Params.n_batch;
      const float PrevTemp = Params.sparams.temp;
      const int64_t PrevNPredict = Conf.NPredict;
      const EmbdNormalizeType PrevEmbdNormalize = Conf.EmbdNormalize;
      ASSERT_TRUE(SetMetadata(R"({"enable-log":true,"tensor-split":"3,2",)"
                              R"("ctx-size":1024,"temp":0.3,"n-predict":7,)"
                              R"("embd-normalize":0,"batch-size":0})"));
      EXPECT_EQ(Errno[0].get<int32_t>(),
                static_cast<uint32_t>(ErrNo::InvalidArgument));
      EXPECT_EQ(BitNetGraph.EnableLog, PrevEnableLog);
      for (float Split : Params.tensor_split) {
        EXPECT_FLOAT_EQ(Split, 0.0f);
      }
      EXPECT_EQ(Params.n_ctx, PrevNCtx);
      EXPECT_EQ(Params.n_batch, PrevNBatch);
      EXPECT_FLOAT_EQ(Params.sparams.temp, PrevTemp);
      EXPECT_EQ(Conf.NPredict, PrevNPredict);
      EXPECT_EQ(Conf.EmbdNormalize, PrevEmbdNormalize);
    }
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
    // The compute_single tests set this prompt again after get_output has
    // written into StorePtr, so keep the tensor out of the output region.
    StorePtr += static_cast<uint32_t>(PromptDim.size() * 4 + PromptData.size());
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
  // Test: set_input -- a prompt that is not valid UTF-8 is rejected instead of
  // escaping from the BPE tokenizer as std::invalid_argument.
  {
    const std::string BadPrompt = "Once upon a time, \xE2\x80";
    std::vector<uint8_t> BadPromptData(BadPrompt.begin(), BadPrompt.end());
    std::vector<uint32_t> BadPromptDim = {
        static_cast<uint32_t>(BadPromptData.size())};
    uint32_t EntryPtr = BuilderPtr;
    writeFatPointer(MemInst, StorePtr, BadPromptDim.size(), EntryPtr);
    writeUInt32(MemInst, static_cast<uint32_t>(TensorType::U8), EntryPtr);
    writeFatPointer(MemInst, StorePtr + BadPromptDim.size() * 4,
                    BadPromptData.size(), EntryPtr);
    writeBinaries<uint32_t>(MemInst, BadPromptDim, StorePtr);
    writeBinaries<uint8_t>(MemInst, BadPromptData,
                           StorePtr + BadPromptDim.size() * 4);
    EXPECT_TRUE(HostFuncSetInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CtxId, 0, BuilderPtr},
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

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
