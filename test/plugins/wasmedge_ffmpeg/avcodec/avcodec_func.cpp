#include "runtime/instance/module.h"
#include "common/types.h"

#include "avcodec/module.h"
#include "avcodec/avcodec_func.h"

#include <gtest/gtest.h>
#include "../testUtils.h"

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_ffmpeg/"
      "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
      WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avcodec"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
}

TEST(WasmEdgeAVCodecTest, AVFrameFunc) {

  // Create the wasmedge_process module instance.
  auto *AVCodecMod = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule *>(createModule());
  ASSERT_TRUE(AVCodecMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
  "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
      WasmEdge::AST::MemoryType(5)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  auto *FuncInst = AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCodecAllocContext3 = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecAllocContext3 &>(FuncInst->getHostFunc());


  uint32_t AvCodecPtr = UINT32_C(0);    // To separate memory address. Same number leading to Collision of addresses.

  {
    writeUInt32(MemInst,UINT32_C(0),AvCodecPtr);
    EXPECT_TRUE(HostFuncAVCodecAllocContext3.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       0,AvCodecPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst,AvCodecPtr) > 0);
  }
}
