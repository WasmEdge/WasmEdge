// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "httpsreqfunc.h"
#include "httpsreqmodule.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_httpsreq/"
      "libwasmedgePluginHttpsReq" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_httpsreq"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_httpsreq"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, const std::string &Str) noexcept {
  char *Buf = MemInst.getPointer<char *>(Offset);
  std::copy_n(Str.c_str(), Str.length(), Buf);
}

} // namespace

TEST(wasmedgeHttpsReqTests, SendData) {
  // Create the wasmedge httpsreq module instance.
  auto *HttpMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeHttpsReqModule *>(createModule());
  EXPECT_FALSE(HttpMod == nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  // Clear the memory[0, 256].
  fillMemContent(MemInst, 0, 256);
  // Set the memory[0, 11] as string "httpbin.org".
  fillMemContent(MemInst, 0, std::string("httpbin.org"));
  // Set the memory[30, 116] as string "GET / HTTP/1.1\nHost:
  // httpbin.org\r\nConnection: Close\r\nReferer: https://httpbin.org/\r\n\r\n".
  fillMemContent(MemInst, 30,
                 std::string("GET / HTTP/1.1\nHost: httpbin.org\r\nConnection: "
                             "Close\r\nReferer: https://httpbin.org/\r\n\r\n"));

  // Get the function "send_data"
  auto *FuncInst = HttpMod->findFuncExports("wasmedge_httpsreq_send_data");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeHttpsReqSendData &>(
      FuncInst->getHostFunc());

  // Test: Run function successfully for get requests
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(11), UINT32_C(443), UINT32_C(30), UINT32_C(86)},
      {}));
  delete HttpMod;
}

TEST(wasmedgeHttpsReqTests, GetRcv) {
  // Create the httpsreq module instance.
  auto *HttpMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeHttpsReqModule *>(createModule());
  EXPECT_FALSE(HttpMod == nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  fillMemContent(MemInst, 0, 256);

  // Set the memory[0, 11] as string "httpbin.org".
  fillMemContent(MemInst, 0, std::string("httpbin.org"));
  // Set the memory[30, 116] as string "GET / HTTP/1.1\nHost:
  // httpbin.org\r\nConnection: Close\r\nReferer: https://httpbin.org/\r\n\r\n".
  fillMemContent(MemInst, 30,
                 std::string("GET / HTTP/1.1\nHost: httpbin.org\r\nConnection: "
                             "Close\r\nReferer: https://httpbin.org/\r\n\r\n"));

  // Get the function "send_data"
  auto *FuncInst = HttpMod->findFuncExports("wasmedge_httpsreq_send_data");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSendData =
      dynamic_cast<WasmEdge::Host::WasmEdgeHttpsReqSendData &>(
          FuncInst->getHostFunc());

  // Get the function "get_rcv_len"
  FuncInst = HttpMod->findFuncExports("wasmedge_httpsreq_get_rcv_len");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetRcvLen =
      dynamic_cast<WasmEdge::Host::WasmEdgeHttpsReqGetRcvLen &>(
          FuncInst->getHostFunc());

  // Get the function "get_rcv"
  FuncInst = HttpMod->findFuncExports("wasmedge_httpsreq_get_rcv");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncGetRcv = dynamic_cast<WasmEdge::Host::WasmEdgeHttpsReqGetRcv &>(
      FuncInst->getHostFunc());

  // Test: Run function successfully for get requests
  EXPECT_TRUE(HostFuncSendData.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(0), UINT32_C(11), UINT32_C(443), UINT32_C(30), UINT32_C(86)},
      {}));

  // Test: Run function successfully for getrcvlen
  std::array<WasmEdge::ValVariant, 1> RetVal;
  EXPECT_TRUE(HostFuncGetRcvLen.run(CallFrame, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0U);

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncGetRcv.run(
      WasmEdge::Runtime::CallingFrame(nullptr, nullptr),
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));

  delete HttpMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
