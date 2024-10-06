// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/type.h"
#include "common/defines.h"
#include "executor/executor.h"
#include "func-attach-bpf-program.h"
#include "func-bpf-buffer-poll.h"
#include "func-bpf-map-fd-by-name.h"
#include "func-bpf-map-operate.h"
#include "func-close-bpf-object.h"
#include "func-load-bpf-object.h"
#include "plugin/plugin.h"
#include "runtime/instance/module.h"
#include "wasm-bpf-module.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
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

std::unique_ptr<WasmEdge::Host::WasmBpfModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(
      std::filesystem::u8path("../../../plugins/wasm_bpf/" WASMEDGE_LIB_PREFIX
                              "wasmedgePluginWasmBpf" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasm_bpf"sv)) {
    if (const auto *Module = Plugin->findModule("wasm_bpf"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmBpfModule>(
          Module->create());
    }
  }
  return {};
}

std::filesystem::path getAssertsPath() {
  std::filesystem::path thisFile(__FILE__);
  return thisFile.parent_path() / "assets";
}
void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &memInst,
                    uint32_t offset, uint32_t count, char chr = 0) noexcept {
  std::fill_n(memInst.getPointer<char *>(offset), count, chr);
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &memInst,
                    uint32_t offset, const std::vector<char> &data) noexcept {
  char *buf = memInst.getPointer<char *>(offset);
  std::copy(data.begin(), data.end(), buf);
}

} // namespace

TEST(WasmBpfTest, Module) {
  auto module = createModule();
  ASSERT_TRUE(module);
  // Test whether functions are exported
  EXPECT_EQ(module->getFuncExportNum(), 6U);
  EXPECT_NE(module->findFuncExports("wasm_load_bpf_object"), nullptr);
  EXPECT_NE(module->findFuncExports("wasm_close_bpf_object"), nullptr);
  EXPECT_NE(module->findFuncExports("wasm_attach_bpf_program"), nullptr);
  EXPECT_NE(module->findFuncExports("wasm_bpf_buffer_poll"), nullptr);
  EXPECT_NE(module->findFuncExports("wasm_bpf_map_fd_by_name"), nullptr);
  EXPECT_NE(module->findFuncExports("wasm_bpf_map_operate"), nullptr);
}

static const size_t TASK_COMM_LEN = 16;
static const size_t MAX_FILENAME_LEN = 127;
struct event {
  int pid;
  int ppid;
  unsigned exit_code;
  unsigned long long duration_ns;
  char comm[TASK_COMM_LEN];
  char filename[MAX_FILENAME_LEN];
  char exit_event;
};

class PollCallbackFunction
    : public WasmEdge::Runtime::HostFunction<PollCallbackFunction> {
public:
  PollCallbackFunction() {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 uint32_t __attribute__((unused)) ctx,
                                 uint32_t data, uint32_t data_sz) {
    using namespace std;
    using WasmEdge::unlikely;
    auto *memory = Frame.getMemoryByIndex(0);
    if (unlikely(!memory)) {
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
    }
    if (data_sz < static_cast<uint32_t>(sizeof(event))) {
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
    }
    const event *dataPtr = memory->getSpan<const event>(data, 1).data();
    if (unlikely(!dataPtr)) {
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
    }
    auto nowTime = chrono::system_clock::now();
    if (dataPtr->exit_event == 1) {
      fmt::print("{:%H:%M:%S} EXIT {:<16} {:<7} {:<7} [{}]"sv, nowTime,
                 dataPtr->comm, dataPtr->pid, dataPtr->ppid,
                 dataPtr->exit_code);
      if (dataPtr->duration_ns != 0) {
        fmt::print(" ({})"sv, dataPtr->duration_ns / 1000000);
      }
      fmt::print("\n"sv);
    } else {
      fmt::print("{:%H:%M:%S} EXEC {:<16} {:<7} {:<7} {}\n"sv, nowTime,
                 dataPtr->comm, dataPtr->pid, dataPtr->ppid, dataPtr->filename);
    }
    return 0;
  }
};

TEST(WasmBpfTest, RunBpfProgramWithPolling) {
  using namespace std::literals::string_view_literals;
  // Test loading and attaching a bpf program, and polling buffer
  auto module = createModule();
  ASSERT_TRUE(module);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance moduleInst("");
  // moduleInst.addHostFunc()
  moduleInst.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *memoryInst = moduleInst.findMemoryExports("memory");
  EXPECT_NE(memoryInst, nullptr);
  auto &memoryInstRef = *memoryInst;
  WasmEdge::Executor::Executor executor((WasmEdge::Configure()));
  WasmEdge::Runtime::CallingFrame CallFrame(&executor, &moduleInst);

  namespace fs = std::filesystem;
  auto bpfObject = getAssertsPath() / "bootstrap.bpf.o";

  // Ensure the bpf object we need exists
  EXPECT_TRUE(fs::exists(bpfObject));

  // Read the bpf object into wasm memory
  std::ifstream bpfObjStream(bpfObject);
  EXPECT_TRUE(bpfObjStream.is_open());
  EXPECT_TRUE(bpfObjStream.good());
  std::vector<char> bpfObjectBytes(
      (std::istreambuf_iterator<char>(bpfObjStream)),
      std::istreambuf_iterator<char>());
  EXPECT_FALSE(bpfObjectBytes.empty());

  // Fill bpf object into memory
  const uint32_t bpfObjectMemoryOffset = 1;
  fillMemContent(memoryInstRef, bpfObjectMemoryOffset, bpfObjectBytes);

  // Fill `handle_exec`, the bpf function name, into memory
  const uint32_t targetHandleExecNameMemoryOffset =
      bpfObjectMemoryOffset + static_cast<uint32_t>(bpfObjectBytes.size());
  const std::string targetHandleExecName("handle_exec");
  // Zero terminated..
  std::vector<char> targetHandleExecNameBytes(targetHandleExecName.size() + 1,
                                              0);
  std::copy(targetHandleExecName.begin(), targetHandleExecName.end(),
            targetHandleExecNameBytes.begin());
  fillMemContent(memoryInstRef, targetHandleExecNameMemoryOffset,
                 targetHandleExecNameBytes);

  // Fill `handle_exit`, the bpf function name, into memory
  const uint32_t targetHandleExitNameMemoryOffset =
      targetHandleExecNameMemoryOffset +
      static_cast<uint32_t>(targetHandleExecNameBytes.size());
  const std::string targetHandleExitName("handle_exit");
  // Zero terminated..
  std::vector<char> targetHandleExitNameBytes(targetHandleExitName.size() + 1,
                                              0);
  std::copy(targetHandleExitName.begin(), targetHandleExitName.end(),
            targetHandleExitNameBytes.begin());
  fillMemContent(memoryInstRef, targetHandleExitNameMemoryOffset,
                 targetHandleExitNameBytes);

  // Fill the map name `rb`
  const uint32_t mapNameMemoryOffset =
      targetHandleExitNameMemoryOffset +
      static_cast<uint32_t>(targetHandleExitNameBytes.size());
  const std::string mapName("rb");
  // Zero terminated..
  std::vector<char> mapNameBytes(mapName.size() + 1, 0);
  std::copy(mapName.begin(), mapName.end(), mapNameBytes.begin());
  fillMemContent(memoryInstRef, mapNameMemoryOffset, mapNameBytes);

  // Prepare a memory area for storing polled things
  const uint32_t bufferPollMemoryOffset =
      mapNameMemoryOffset + static_cast<uint32_t>(mapNameBytes.size());
  const uint32_t bufferPollSize = 1024;
  fillMemContent(memoryInstRef, bufferPollMemoryOffset, bufferPollSize, 0);

  // Get function "wasm_load_bpf_object"
  auto *loadFunc = module->findFuncExports("wasm_load_bpf_object");
  EXPECT_NE(loadFunc, nullptr);
  EXPECT_TRUE(loadFunc->isHostFunction());
  auto &loadFuncHost =
      dynamic_cast<WasmEdge::Host::LoadBpfObject &>(loadFunc->getHostFunc());

  // call "wasm_load_bpf_object" to Load `bootstrap.bpf.o`, and check the
  // result
  std::array<WasmEdge::ValVariant, 1> loadResult;
  EXPECT_TRUE(loadFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(bpfObjectMemoryOffset),
          WasmEdge::ValVariant(static_cast<uint32_t>(bpfObjectBytes.size()))},
      loadResult));
  auto handle = loadResult[0].get<int64_t>();
  EXPECT_NE(handle, 0);

  // Get function `wasm_attach_bpf_program`
  auto *attachFunc = module->findFuncExports("wasm_attach_bpf_program");
  EXPECT_NE(attachFunc, nullptr);
  EXPECT_TRUE(attachFunc->isHostFunction());
  auto &attachFuncHost = dynamic_cast<WasmEdge::Host::AttachBpfProgram &>(
      attachFunc->getHostFunc());

  // Call "wasm_attach_bpf_program" to attach, and check the result
  std::array<WasmEdge::ValVariant, 1> attachResult;
  EXPECT_TRUE(attachFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(handle),
          WasmEdge::ValVariant(targetHandleExecNameMemoryOffset),
          // There should be '\0'
          WasmEdge::ValVariant(
              targetHandleExecNameMemoryOffset +
              static_cast<uint32_t>(targetHandleExecName.size())),
      },
      attachResult));
  EXPECT_GE(attachResult[0].get<int32_t>(), 0);
  EXPECT_TRUE(attachFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(handle),
          WasmEdge::ValVariant(targetHandleExitNameMemoryOffset),
          // There should be '\0'
          WasmEdge::ValVariant(
              targetHandleExitNameMemoryOffset +
              static_cast<uint32_t>(targetHandleExitName.size())),
      },
      attachResult));
  EXPECT_GE(attachResult[0].get<int32_t>(), 0);

  // Get function `wasm_bpf_map_fd_by_name`
  auto *mapFdFunc = module->findFuncExports("wasm_bpf_map_fd_by_name");
  EXPECT_NE(mapFdFunc, nullptr);
  EXPECT_TRUE(mapFdFunc->isHostFunction());
  auto &mapFdFuncHost =
      dynamic_cast<WasmEdge::Host::BpfMapFdByName &>(mapFdFunc->getHostFunc());

  // Call "wasm_bpf_map_fd_by_name" to get the map fd, and check the result
  std::array<WasmEdge::ValVariant, 1> mapFdResult;
  EXPECT_TRUE(mapFdFuncHost.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    WasmEdge::ValVariant(handle),
                                    WasmEdge::ValVariant(mapNameMemoryOffset)},
                                mapFdResult));
  auto mapFd = mapFdResult[0].get<int32_t>();
  EXPECT_GE(mapFd, 0);

  // In the following several steps we will prepare for polling
  // Create an instance of the polling callback function
  moduleInst.addHostFunc("__polling_callback_hostfunc"sv,
                         std::make_unique<PollCallbackFunction>());
  auto *callbackFuncInst =
      moduleInst.findFuncExports("__polling_callback_hostfunc");
  // Create a function table, and fill the callback function into it
  auto funcTableInst =
      std::make_unique<WasmEdge::Runtime::Instance::TableInstance>(
          WasmEdge::AST::TableType(WasmEdge::TypeCode::FuncRef, 1));
  EXPECT_TRUE(funcTableInst->setRefs(
      std::initializer_list<const WasmEdge::RefVariant>{callbackFuncInst}, 0, 0,
      1));
  // Add the table to the main module
  moduleInst.addHostTable("__indirect_function_table"sv,
                          std::move(funcTableInst));

  // Get the "wasm_bpf_buffer_poll" function
  auto *bufferPollFunc = module->findFuncExports("wasm_bpf_buffer_poll");
  EXPECT_NE(bufferPollFunc, nullptr);
  EXPECT_TRUE(bufferPollFunc->isHostFunction());
  auto &bufferPollFuncHost = dynamic_cast<WasmEdge::Host::BpfBufferPoll &>(
      bufferPollFunc->getHostFunc());

  // Call the polling function
  std::array<WasmEdge::ValVariant, 1> pollResult;
  for (size_t i = 1; i <= 50; i++) {
    using namespace std;
    EXPECT_TRUE(bufferPollFuncHost.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            WasmEdge::ValVariant(handle), // object handle
            WasmEdge::ValVariant(mapFd),  // map fd
            UINT32_C(0),                  // callback function index
            UINT32_C(0),                  // Custom context pointer
            WasmEdge::ValVariant(bufferPollMemoryOffset), // buffer offset
            WasmEdge::ValVariant(bufferPollSize),         // buffer size
            UINT32_C(100)                                 // timeout (ms)
        },
        pollResult));
    EXPECT_GE(pollResult[0].get<int32_t>(), 0);
  }

  // Get function `wasm_close_bpf_object`
  auto *closeFunc = module->findFuncExports("wasm_close_bpf_object");
  EXPECT_NE(closeFunc, nullptr);
  EXPECT_TRUE(closeFunc->isHostFunction());
  auto &closeFuncHost =
      dynamic_cast<WasmEdge::Host::CloseBpfObject &>(closeFunc->getHostFunc());

  // Call "wasm_close_bpf_object" to attach, and check the result
  std::array<WasmEdge::ValVariant, 1> closeResult;
  EXPECT_TRUE(closeFuncHost.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    WasmEdge::ValVariant(handle),
                                },
                                closeResult));
  EXPECT_EQ(closeResult[0].get<int32_t>(), 0);
}

static const size_t MAX_SLOTS = 26;

struct hist {
  unsigned int slots[MAX_SLOTS];
  char comm[TASK_COMM_LEN];
} __attribute__((packed));

TEST(WasmBpfTest, RunBpfProgramWithMapOperation) {
  // Test loading and attaching a bpf program, and polling buffer
  auto module = createModule();
  ASSERT_TRUE(module);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance moduleInst("");
  // moduleInst.addHostFunc()
  moduleInst.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *memoryInst = moduleInst.findMemoryExports("memory");
  EXPECT_NE(memoryInst, nullptr);
  auto &memoryInstRef = *memoryInst;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &moduleInst);

  namespace fs = std::filesystem;
  auto bpfObject = getAssertsPath() / "runqlat.bpf.o";

  // Ensure the bpf object we need exists
  EXPECT_TRUE(fs::exists(bpfObject));

  // Read the bpf object into wasm memory
  std::ifstream bpfObjStream(bpfObject);
  EXPECT_TRUE(bpfObjStream.is_open());
  EXPECT_TRUE(bpfObjStream.good());
  std::vector<char> bpfObjectBytes(
      (std::istreambuf_iterator<char>(bpfObjStream)),
      std::istreambuf_iterator<char>());
  EXPECT_FALSE(bpfObjectBytes.empty());
  // Offset to put things into memory
  uint32_t nextOffset = 1;

  // Put the bpf object into memory
  const uint32_t bpfObjectMemoryOffset = nextOffset;
  fillMemContent(memoryInstRef, bpfObjectMemoryOffset, bpfObjectBytes);
  nextOffset += static_cast<uint32_t>(bpfObjectBytes.size());

  // Fill strings that will be used into memory
  std::array<const char *, 5> strings = {
      "hists",                                            // Map name
      "sched_wakeup", "sched_wakeup_new", "sched_switch", // Program names
      ""                                                  // An empty string
  };
  std::array<uint32_t, 5> stringOffsets;

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
  EXPECT_NE(loadFunc, nullptr);
  EXPECT_TRUE(loadFunc->isHostFunction());
  auto &loadFuncHost =
      dynamic_cast<WasmEdge::Host::LoadBpfObject &>(loadFunc->getHostFunc());

  // call "wasm_load_bpf_object" to Load `bootstrap.bpf.o`, and check the
  // result
  std::array<WasmEdge::ValVariant, 1> loadResult;
  EXPECT_TRUE(loadFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(bpfObjectMemoryOffset),
          WasmEdge::ValVariant(static_cast<uint32_t>(bpfObjectBytes.size()))},
      loadResult));
  auto handle = loadResult[0].get<int64_t>();
  EXPECT_NE(handle, 0);

  // Get function `wasm_attach_bpf_program`
  auto *attachFunc = module->findFuncExports("wasm_attach_bpf_program");
  EXPECT_NE(attachFunc, nullptr);
  EXPECT_TRUE(attachFunc->isHostFunction());
  auto &attachFuncHost = dynamic_cast<WasmEdge::Host::AttachBpfProgram &>(
      attachFunc->getHostFunc());
  std::array<size_t, 3> programNameIndexes = {1, 2, 3};

  // Attach the programs
  for (size_t index : programNameIndexes) {
    std::array<WasmEdge::ValVariant, 1> attachResult;
    EXPECT_TRUE(
        attachFuncHost.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               WasmEdge::ValVariant(handle),
                               WasmEdge::ValVariant(stringOffsets[index]),
                               // There should be '\0'
                               WasmEdge::ValVariant(stringOffsets[4]),
                           },
                           attachResult));
    EXPECT_GE(attachResult[0].get<int32_t>(), 0);
  }

  // Get function `wasm_bpf_map_fd_by_name`
  auto *mapFdFunc = module->findFuncExports("wasm_bpf_map_fd_by_name");
  EXPECT_NE(mapFdFunc, nullptr);
  EXPECT_TRUE(mapFdFunc->isHostFunction());
  auto &mapFdFuncHost =
      dynamic_cast<WasmEdge::Host::BpfMapFdByName &>(mapFdFunc->getHostFunc());

  // Call "wasm_bpf_map_fd_by_name" to get the map fd, and check the result
  std::array<WasmEdge::ValVariant, 1> mapFdResult;
  EXPECT_TRUE(mapFdFuncHost.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          WasmEdge::ValVariant(handle), WasmEdge::ValVariant(stringOffsets[0])},
      mapFdResult));
  auto histsFd = mapFdResult[0].get<int32_t>();
  EXPECT_GE(histsFd, 0);

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
  // Three helper functions that will be used
  auto mapGetNextKey = [&](int32_t fd, uint32_t lookupKey,
                           uint32_t nextKey) -> int32_t {
    // lookupKey is the last element -> returns -1
    // lookupKey found -> returns 0, set nextKey
    // lookupKey not found -> returns 0, set nextKey to the first key

    return callMapOperate(fd,
                          4, // BPF_MAP_GET_NEXT_KEY
                          lookupKey, 0, nextKey, 0);
  };
  auto mapLookupElem = [&](int32_t fd, uint32_t key,
                           uint32_t valueOut) -> int32_t {
    // key found -> returns 0
    // key not found -> returns -1
    return callMapOperate(fd,
                          1, // BPF_MAP_LOOKUP_ELEM
                          key, valueOut, 0, 0);
  };
  auto mapDeleteElem = [&](int32_t fd, uint32_t key) -> int32_t {
    // key found -> return 0
    // key not found -> returns -1
    return callMapOperate(fd,
                          3, // BPF_MAP_DELETE_ELEM
                          key, 0, 0, 0);
  };
  // Three helper functions to make read & write more convenient
  auto readU32 = [&](uint32_t offset) -> uint32_t {
    const auto *ptr = memoryInstRef.getPointer<const uint32_t *>(offset);
    EXPECT_NE(ptr, nullptr);
    return *ptr;
  };
  auto writeU32 = [&](uint32_t offset, uint32_t val) {
    auto *ptr = memoryInstRef.getPointer<uint32_t *>(offset);
    EXPECT_NE(ptr, nullptr);
    *ptr = val;
  };
  auto readHistRef = [&](uint32_t offset) -> const hist & {
    const auto *ptr = memoryInstRef.getPointer<const hist *>(offset);
    EXPECT_NE(ptr, nullptr);
    return *ptr;
  };
  const uint32_t lookUpKeyOffset = nextOffset;
  nextOffset += sizeof(uint32_t);
  const uint32_t nextKeyOffset = nextOffset;
  nextOffset += sizeof(uint32_t);
  const uint32_t histOffset = nextOffset;
  nextOffset += sizeof(hist);

  // Poll 10 times, with interval 1s
  for (size_t i = 1; i <= 10; i++) {
    using namespace std;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    writeU32(lookUpKeyOffset, static_cast<uint32_t>(-2));
    while (mapGetNextKey(histsFd, lookUpKeyOffset, nextKeyOffset) == 0) {
      EXPECT_GE(mapLookupElem(histsFd, nextKeyOffset, histOffset), 0);
      const auto &histRef = readHistRef(histOffset);
      size_t maxIdx = 0;
      for (size_t i = 0; i < std::size(histRef.slots); i++)
        if (histRef.slots[i] > 0)
          maxIdx = i;
      for (size_t i = 0; i < maxIdx; i++) {
        auto low = UINT64_C(1) << (i);
        auto high = (UINT64_C(1) << (i + 1)) - 1;
        fmt::print("{:<6}...{:<6} {:<6}\n"sv, low, high, histRef.slots[i]);
      }
      writeU32(lookUpKeyOffset, readU32(nextKeyOffset));
    }
    writeU32(lookUpKeyOffset, static_cast<uint32_t>(-2));
    while (mapGetNextKey(histsFd, lookUpKeyOffset, nextKeyOffset) == 0) {
      EXPECT_GE(mapDeleteElem(histsFd, nextKeyOffset), 0);
      writeU32(lookUpKeyOffset, readU32(nextKeyOffset));
    }
    fmt::print("\n"sv);
  }

  // Get function `wasm_close_bpf_object`
  auto *closeFunc = module->findFuncExports("wasm_close_bpf_object");
  EXPECT_NE(closeFunc, nullptr);
  EXPECT_TRUE(closeFunc->isHostFunction());
  auto &closeFuncHost =
      dynamic_cast<WasmEdge::Host::CloseBpfObject &>(closeFunc->getHostFunc());

  // Call "wasm_close_bpf_object" to attach, and check the result
  std::array<WasmEdge::ValVariant, 1> closeResult;
  EXPECT_TRUE(closeFuncHost.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    WasmEdge::ValVariant(handle),
                                },
                                closeResult));
  EXPECT_EQ(closeResult[0].get<int32_t>(), 0);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
