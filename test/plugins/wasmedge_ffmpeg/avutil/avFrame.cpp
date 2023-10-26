#include "avutil/avFrame.h"
#include "../utils.h"
#include "avutil/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVFrame) {

  auto *AVUtilMod = TestUtils::InitModules::createAVUtilModule();
  ASSERT_TRUE(AVUtilMod != nullptr);

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

  uint32_t AVFramePtr = UINT32_C(8);
  //  TestUtils::AVFrame::initEmptyFrame(Mod, AVFramePtr, Result);
  TestUtils::AVFrame::initVideoFrame(Mod, AVFramePtr, UINT32_C(12),
                                     UINT32_C(16), UINT32_C(20), UINT32_C(24),
                                     UINT32_C(28), Result);
  uint32_t AVFrameId = readUInt32(MemInst, AVFramePtr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_width");
  auto &HostFuncAVFrameWidth =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameWidth &>(
          FuncInst->getHostFunc());

  {

    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1920);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_height");
  auto &HostFuncAVFrameHeight =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameHeight &>(
          FuncInst->getHostFunc());

  {

    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1080);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_video_format");
  auto &HostFuncAVFrameVideoFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameVideoFormat &>(
      FuncInst->getHostFunc());

  {

    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_isnull");
  auto &HostFuncAVFrameIsNull =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameIsNull &>(
          FuncInst->getHostFunc());

  {

    HostFuncAVFrameIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_linesize");
  auto &HostFuncAVFrameLinesize =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameLinesize &>(
          FuncInst->getHostFunc());

  uint32_t NumDataPointers = 0;
  {

    HostFuncAVFrameLinesize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, NumDataPointers},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1920);
  }
}
