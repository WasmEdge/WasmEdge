// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "executor/executor.h"
#include "func-attach-bpf-program.h"
#include "func-bpf-map-fd-by-name.h"
#include "func-bpf-map-operate.h"
#include "func-close-bpf-object.h"
#include "func-load-bpf-object.h"
#include "plugin/plugin.h"
#include "runtime/instance/module.h"
#include "wasm-bpf-module.h"

#include <chrono>
#include <cinttypes>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <string_view>
#include <thread>

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(
      std::filesystem::u8path("../../../plugins/wasm_bpf/" WASMEDGE_LIB_PREFIX
                              "wasmedgePluginWasmBpf" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasm_bpf"sv)) {
    if (const auto *Module = Plugin->findModule("wasm_bpf"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

std::filesystem::path getAssertsPath() {
  std::filesystem::path thisFile(__FILE__);
  return thisFile.parent_path() / "assets";
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &memInst,
                    uint32_t offset, const std::vector<char> &data) noexcept {
  char *buf = memInst.getPointer<char *>(offset);
  std::copy(data.begin(), data.end(), buf);
}
} // namespace

static const uint32_t INDICATING_KEY = 0xABCD;
static const uint32_t ADD_VALUE_1_KEY = 0xCDEF;
static const uint32_t ADD_VALUE_2_KEY = 0x1234;
static const uint32_t RESULT_VALUE_KEY = 0x7890;

TEST(WasmBpfTest, SimpleMapTest) {
  using namespace std::string_view_literals;
  // Test loading and attaching a bpf program, and some operations of maps
  auto module = dynamic_cast<WasmEdge::Host::WasmBpfModule *>(createModule());
  ASSERT_NE(module, nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance moduleInst("");
  // moduleInst.addHostFunc()
  moduleInst.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *memoryInst = moduleInst.findMemoryExports("memory");
  ASSERT_NE(memoryInst, nullptr);
  auto &memoryInstRef = *memoryInst;
  WasmEdge::Executor::Executor executor((WasmEdge::Configure()));
  WasmEdge::Runtime::CallingFrame CallFrame(&executor, &moduleInst);

  namespace fs = std::filesystem;
  auto bpfObject = getAssertsPath() / "simple_map.bpf.o";

  // Ensure the bpf object we need exists
  ASSERT_TRUE(fs::exists(bpfObject));

  // Read the bpf object into wasm memory
  std::ifstream bpfObjStream(bpfObject);
  ASSERT_TRUE(bpfObjStream.is_open());
  ASSERT_TRUE(bpfObjStream.good());
  std::vector<char> bpfObjectBytes(
      (std::istreambuf_iterator<char>(bpfObjStream)),
      std::istreambuf_iterator<char>());
  ASSERT_FALSE(bpfObjectBytes.empty());
  // Offset to put things into memory
  uint32_t nextOffset = 1;

  // Put the bpf object into memory
  const uint32_t bpfObjectMemoryOffset = nextOffset;
  fillMemContent(memoryInstRef, bpfObjectMemoryOffset, bpfObjectBytes);
  nextOffset += static_cast<uint32_t>(bpfObjectBytes.size());

  // Fill strings that will be used into memory
  std::array<const char *, 3> strings = {
      "test_map",     // Map name
      "sched_wakeup", // Program names
      ""              // An empty string
  };
  std::array<uint32_t, 3> stringOffsets;

  for (size_t i = 0; i < strings.size(); i++) {
    std::string currString(strings[i]);
    std::vector<char> bytes(currString.begin(), currString.end());
    // Ensure that strings are zero-terminated
    bytes.push_back('\0');
    fillMemContent(memoryInstRef, nextOffset, bytes);
    stringOffsets[i] = nextOffset;
    nextOffset += static_cast<uint32_t>(bytes.size());
  }

  // Get function "wasm_load_bpf_object"
  auto *loadFunc = module->findFuncExports("wasm_load_bpf_object");
  ASSERT_NE(loadFunc, nullptr);
  ASSERT_TRUE(loadFunc->isHostFunction());
  auto &loadFuncHost =
      dynamic_cast<WasmEdge::Host::LoadBpfObject &>(loadFunc->getHostFunc());

  // call "wasm_load_bpf_object" to Load `bootstrap.bpf.o`, and check the
  // result
  std::array<WasmEdge::ValVariant, 1> loadResult;
  ASSERT_TRUE(loadFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(bpfObjectMemoryOffset),
          WasmEdge::ValVariant(static_cast<uint32_t>(bpfObjectBytes.size()))},
      loadResult));
  auto handle = loadResult[0].get<int64_t>();
  ASSERT_NE(handle, 0);

  // Get function `wasm_attach_bpf_program`
  auto *attachFunc = module->findFuncExports("wasm_attach_bpf_program");
  ASSERT_NE(attachFunc, nullptr);
  ASSERT_TRUE(attachFunc->isHostFunction());
  auto &attachFuncHost = dynamic_cast<WasmEdge::Host::AttachBpfProgram &>(
      attachFunc->getHostFunc());

  // Call "wasm_attach_bpf_program" to attach, and check the result
  std::array<WasmEdge::ValVariant, 1> attachResult;
  ASSERT_TRUE(attachFuncHost.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     WasmEdge::ValVariant(handle),
                                     WasmEdge::ValVariant(stringOffsets[1]),
                                     // There should be '\0'
                                     WasmEdge::ValVariant(stringOffsets[2]),
                                 },
                                 attachResult));
  ASSERT_GE(attachResult[0].get<int32_t>(), 0);

  // Get function `wasm_bpf_map_fd_by_name`
  auto *mapFdFunc = module->findFuncExports("wasm_bpf_map_fd_by_name");
  ASSERT_NE(mapFdFunc, nullptr);
  ASSERT_TRUE(mapFdFunc->isHostFunction());
  auto &mapFdFuncHost =
      dynamic_cast<WasmEdge::Host::BpfMapFdByName &>(mapFdFunc->getHostFunc());

  // Call "wasm_bpf_map_fd_by_name" to get the map fd, and check the result
  std::array<WasmEdge::ValVariant, 1> mapFdResult;
  ASSERT_TRUE(mapFdFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(handle), WasmEdge::ValVariant(stringOffsets[0])},
      mapFdResult));
  auto mapFd = mapFdResult[0].get<int32_t>();
  ASSERT_GE(mapFd, 0);

  // Get function `wasm_bpf_map_fd_by_name`
  auto *mapOptFunc = module->findFuncExports("wasm_bpf_map_operate");
  EXPECT_NE(mapOptFunc, nullptr);
  EXPECT_TRUE(mapOptFunc->isHostFunction());
  auto &mapOptFuncHost =
      dynamic_cast<WasmEdge::Host::BpfMapOperate &>(mapOptFunc->getHostFunc());

  // A wrapper to call wasm_bpf_map_operate
  auto callMapOperate = [&](int32_t fd, int32_t cmd, uint32_t key,
                            uint32_t value, uint32_t nextKey,
                            uint64_t flags) -> int32_t {
    std::array<WasmEdge::ValVariant, 1> callResult;
    EXPECT_TRUE(mapOptFuncHost.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            WasmEdge::ValVariant(fd), WasmEdge::ValVariant(cmd),
            WasmEdge::ValVariant(key), WasmEdge::ValVariant(value),
            WasmEdge::ValVariant(nextKey), WasmEdge::ValVariant(flags)},
        callResult));
    return callResult[0].get<int32_t>();
  };

  auto mapLookupElem = [&](int32_t fd, uint32_t key,
                           uint32_t valueOut) -> int32_t {
    // key found -> returns 0
    // key not found -> returns -1
    return callMapOperate(fd,
                          1, // BPF_MAP_LOOKUP_ELEM
                          key, valueOut, 0, 0);
  };

  auto mapUpdateElem = [&](int32_t fd, uint32_t key,
                           uint32_t value) -> int32_t {
    return callMapOperate(fd,
                          2, // BPF_MAP_UPDATE_ELEM
                          key, value, 0, 0);
  };

  // Helper functions to make read & write more convenient
  auto readU64 = [&](uint32_t offset) -> uint64_t {
    const auto *ptr = memoryInstRef.getPointer<const uint64_t *>(offset);
    EXPECT_NE(ptr, nullptr);
    return *ptr;
  };
  auto writeU64 = [&](uint32_t offset, uint64_t val) {
    auto *ptr = memoryInstRef.getPointer<uint64_t *>(offset);
    EXPECT_NE(ptr, nullptr);
    *ptr = val;
  };

  auto writeU32 = [&](uint32_t offset, uint32_t val) {
    auto *ptr = memoryInstRef.getPointer<uint32_t *>(offset);
    EXPECT_NE(ptr, nullptr);
    *ptr = val;
  };

  // Generate two numbers, which will be stored in the map and calculated the
  // summation by the ebpf program
  std::mt19937 randGen;
  randGen.seed(std::random_device()());
  std::uniform_int_distribution<uint64_t> intDist(0,
                                                  static_cast<uint64_t>(1e16));
  uint64_t num1 = intDist(randGen);
  uint64_t num2 = intDist(randGen);

  // Prepare for wasm memory which is used to store numbers
  const uint32_t numOffset1 = nextOffset;
  nextOffset += 8;
  const uint32_t numOffset2 = nextOffset;
  nextOffset += 8;
  const uint32_t resultOffset = nextOffset;
  nextOffset += 8;
  const uint32_t indicatingKeyOffset = nextOffset;
  nextOffset += 4;
  const uint32_t num1KeyOffset = nextOffset;
  nextOffset += 4;
  const uint32_t num2KeyOffset = nextOffset;
  nextOffset += 4;
  const uint32_t resultKeyOffset = nextOffset;
  nextOffset += 4;

  writeU32(num1KeyOffset, ADD_VALUE_1_KEY);
  writeU32(num2KeyOffset, ADD_VALUE_2_KEY);
  writeU32(resultKeyOffset, RESULT_VALUE_KEY);
  writeU32(indicatingKeyOffset, INDICATING_KEY);

  writeU32(INDICATING_KEY, indicatingKeyOffset);

  writeU64(numOffset1, num1);
  writeU64(numOffset2, num2);

  writeU64(resultOffset, 0);

  // Write the add values into the map
  ASSERT_EQ(mapUpdateElem(mapFd, num1KeyOffset, numOffset1), 0);
  ASSERT_EQ(mapUpdateElem(mapFd, num2KeyOffset, numOffset2), 0);

  // Write the indicating key
  // Arbitrary values are correct. We only care the existence of the
  // indicating key
  ASSERT_EQ(mapUpdateElem(mapFd, indicatingKeyOffset, numOffset1), 0);

  // Sleep for 1s and wait for the ebpf program to process..
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Read the result and check it
  ASSERT_EQ(mapLookupElem(mapFd, resultKeyOffset, resultOffset), 0);
  uint64_t addResult = readU64(resultOffset);
  ASSERT_EQ(addResult, num1 + num2);

  // Get function `wasm_close_bpf_object`
  auto *closeFunc = module->findFuncExports("wasm_close_bpf_object");
  ASSERT_NE(closeFunc, nullptr);
  ASSERT_TRUE(closeFunc->isHostFunction());
  auto &closeFuncHost =
      dynamic_cast<WasmEdge::Host::CloseBpfObject &>(closeFunc->getHostFunc());

  // Call "wasm_close_bpf_object" to attach, and check the result
  std::array<WasmEdge::ValVariant, 1> closeResult;
  ASSERT_TRUE(closeFuncHost.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    WasmEdge::ValVariant(handle),
                                },
                                closeResult));
  ASSERT_EQ(closeResult[0].get<int32_t>(), 0);
}
