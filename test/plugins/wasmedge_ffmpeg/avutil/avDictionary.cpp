#include "common/types.h"
#include "runtime/instance/module.h"

#include "../testUtils.h"
#include "avutil/avDictionary.h"
#include "avutil/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_ffmpeg/"
      "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avutil"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
} // namespace

TEST(WasmEdgeAVUtilTest, AVDictionary) {

  // Create the wasmedge_process module instance.
  auto *AVUtilMod = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule *>(
      createModule());
  ASSERT_TRUE(AVUtilMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  uint32_t DictPtr = UINT32_C(100);
  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  fillMemContent(MemInst, 1, 32);

  fillMemContent(MemInst, 1, std::string("KEY"));
  fillMemContent(MemInst, 4, std::string("VALUE"));
  {
    writeUInt32(MemInst, INT32_C(0), DictPtr);
    EXPECT_TRUE(HostFuncAVDictSet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            DictPtr, UINT32_C(1), UINT32_C(3), UINT32_C(4), UINT32_C(5), 0},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    ASSERT_TRUE(readUInt32(MemInst, DictPtr) > 0);
  }

  // Getting Subprocess error.
  //  FuncInst =
  //  AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_copy");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncAVDictCopy =
  //      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictCopy &>(
  //          FuncInst->getHostFunc());
  //
  //  {
  //    uint32_t DestDictPtr = UINT32_C(80);
  //    writeUInt32(MemInst, UINT32_C(0), DestDictPtr);
  //    uint32_t SrcDictId = readUInt32(MemInst, DictPtr);
  //    EXPECT_TRUE(HostFuncAVDictCopy.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{DestDictPtr, SrcDictId,
  //        0}, Result));
  //    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  //  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictGet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGet &>(
          FuncInst->getHostFunc());

  {
    uint32_t KeyLenPtr = UINT32_C(35);
    uint32_t ValueLenPtr = UINT32_C(42);
    writeUInt32(MemInst, UINT32_C(0), KeyLenPtr);
    writeUInt32(MemInst, UINT32_C(0), ValueLenPtr);
    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(
        HostFuncAVDictGet.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  DictId, UINT32_C(1), UINT32_C(3), UINT32_C(0),
                                  UINT32_C(0), KeyLenPtr, ValueLenPtr},
                              Result));
    EXPECT_TRUE(Result[0].get<int32_t>() == 1);
    EXPECT_EQ(readUInt32(MemInst, KeyLenPtr), 3);
    EXPECT_EQ(readUInt32(MemInst, ValueLenPtr), 5);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_dict_get_key_value");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictGetKeyValue =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGetKeyValue &>(
          FuncInst->getHostFunc());

  {
    uint32_t KeyBufPtr = UINT32_C(35);
    uint32_t ValueBufPtr = UINT32_C(42);
    writeUInt32(MemInst, UINT32_C(0), KeyBufPtr);
    writeUInt32(MemInst, UINT32_C(0), ValueBufPtr);
    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVDictGetKeyValue.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            DictId, UINT32_C(1), UINT32_C(3), ValueBufPtr, UINT32_C(5),
            KeyBufPtr, UINT32_C(3), UINT32_C(0), UINT32_C(2)},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() == 1);
    // Verify String. Read String from MemInst
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictFree &>(
          FuncInst->getHostFunc());

  {
    uint32_t dictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVDictFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{dictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}