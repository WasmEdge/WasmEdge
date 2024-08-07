// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "executor/executor.h"
#include "func-attach-bpf-program.h"
#include "func-bpf-buffer-poll.h"
#include "func-bpf-map-fd-by-name.h"
#include "func-close-bpf-object.h"
#include "func-load-bpf-object.h"
#include "plugin/plugin.h"
#include "runtime/instance/module.h"
#include "wasm-bpf-module.h"

#include <gtest/gtest.h>
#include <string>
#include <string_view>

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
    if (data_sz < static_cast<uint32_t>(sizeof(uint32_t))) {
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
    }
    const uint32_t *dataPtr = memory->getSpan<const uint32_t>(data, 1).data();
    if (unlikely(!dataPtr)) {
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
    }
    EXPECT_EQ(*dataPtr, UINT32_C(0xABCD1234));
    return 0;
  }
};

} // namespace

TEST(WasmBpfTest, SimpleRingbuf) {
  using namespace std::string_view_literals;
  // Test loading and attaching a bpf program, and polling buffer
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
  auto bpfObject = getAssertsPath() / "simple_ringbuf.bpf.o";

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
      "rb",          // Map name
      "handle_exec", // Program names
      ""             // An empty string
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

  const uint32_t bufferPollMemoryOffset = nextOffset;
  const uint32_t bufferPollSize = 256;
  nextOffset += bufferPollSize;

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
  ASSERT_TRUE(funcTableInst->setRefs(
      std::initializer_list<const WasmEdge::RefVariant>{callbackFuncInst}, 0, 0,
      1));
  // Add the table to the main module
  moduleInst.addHostTable("__indirect_function_table"sv,
                          std::move(funcTableInst));

  // Get the "wasm_bpf_buffer_poll" function
  auto *bufferPollFunc = module->findFuncExports("wasm_bpf_buffer_poll");
  ASSERT_NE(bufferPollFunc, nullptr);
  ASSERT_TRUE(bufferPollFunc->isHostFunction());
  auto &bufferPollFuncHost = dynamic_cast<WasmEdge::Host::BpfBufferPoll &>(
      bufferPollFunc->getHostFunc());

  // Call the polling function
  std::array<WasmEdge::ValVariant, 1> pollResult;
  for (size_t i = 1; i <= 50; i++) {
    using namespace std;
    ASSERT_TRUE(bufferPollFuncHost.run(
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
    ASSERT_GE(pollResult[0].get<int32_t>(), 0);
  }

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
