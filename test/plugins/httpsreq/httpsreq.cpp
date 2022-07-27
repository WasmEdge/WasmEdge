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
      "../../../plugins/httpsreq/"
      "libwasmedgePluginHttpsReq" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("https_req"sv)) {
    if (const auto *Module = Plugin->findModule("https_req"sv)) {
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
  // Create the httpsreq module instance.
  auto *ProcMod =
      dynamic_cast<WasmEdge::Host::HttpsReqModule *>(createModule());
  EXPECT_FALSE(ProcMod == nullptr);

  // Create the memory instance.
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  // Clear the memory[0, 128].
  fillMemContent(MemInst, 0, 128);
  // Set the memory[0, 11] as string "echo".
  fillMemContent(MemInst, 0, std::string("httpbin.org"));
  // Set the memory[30, 116] as string "GET / HTTP/1.1\nHost:
  // httpbin.org\r\nConnection: Close\r\nReferer: https://httpbin.org/\r\n\r\n".
  fillMemContent(MemInst, 30,
                 std::string("GET / HTTP/1.1\nHost: httpbin.org\r\nConnection: "
                             "Close\r\nReferer: https://httpbin.org/\r\n\r\n"));

  // Get the function "send_data"
  auto *FuncInst = ProcMod->findFuncExports("send_data");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst =
      dynamic_cast<WasmEdge::Host::SendData &>(FuncInst->getHostFunc());

  // Test: Run function successfully for get requests
  EXPECT_TRUE(HostFuncInst.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(11), UINT32_C(443),UINT32_C(30),UINT32_C(86)},
      {}));
  EXPECT_TRUE(ProcMod->getEnv().Host == "httpbin.org");
  EXPECT_TRUE(ProcMod->getEnv().Body == "GET / HTTP/1.1\nHost: httpbin.org\r\nConnection: "
                             "Close\r\nReferer: https://httpbin.org/\r\n\r\n");
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
