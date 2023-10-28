#include "../utils.h"
#include "avutil/module.h"
#include "avutil/samplefmt.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVSampleFmt) {

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

  uint32_t BufferPtr = UINT32_C(8);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_packed_sample_fmt");
  auto &HostFuncAVGetPackedSampleFmt = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetPackedSampleFmt &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetPackedSampleFmt.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{3}, Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 3);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_planar_sample_fmt");
  auto &HostFuncAVGetPlanarSampleFmt = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetPlanarSampleFmt &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetPlanarSampleFmt.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{3}, Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 8);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_sample_fmt_is_planar");
  auto &HostFuncAVSampleFmtIsPlanar = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSampleFmtIsPlanar &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVSampleFmtIsPlanar.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{3}, Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_bytes_per_sample");
  auto &HostFuncAVGetBytesPerSample = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetBytesPerSample &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetBytesPerSample.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{3}, Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_get_sample_fmt");
  auto &HostFuncAVGetSampleFmt =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetSampleFmt &>(
          FuncInst->getHostFunc());

  uint32_t SampleFmtStart = 100;
  uint32_t SampleFmtSize = 2;
  fillMemContent(MemInst, SampleFmtSize, SampleFmtSize);

  fillMemContent(MemInst, SampleFmtStart, std::string("u8"));
  {
    HostFuncAVGetSampleFmt.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   SampleFmtStart, SampleFmtSize},
                               Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_samples_get_buffer_size");
  auto &HostFuncAVSamplesGetBufferSize = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSamplesGetBufferSize &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVSamplesGetBufferSize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{2, 2, 2, 0},
        Result);

    EXPECT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_samples_alloc_array_and_samples");
  auto &HostFuncAVSamplesAllocArrayAndSamples = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSamplesAllocArrayAndSamples &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVSamplesAllocArrayAndSamples.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{BufferPtr, 0, 2, 2, 2, 0},
        Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  // Need to pass a buffer, read data and verify.
  //  FuncInst = AVUtilMod->findFuncExports(
  //      "wasmedge_ffmpeg_avutil_av_samples_get_buffer");
  //  auto &HostFuncAVSamplesGetBuffer = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSamplesGetBuffer &>(
  //      FuncInst->getHostFunc());
  //
  //  {
  //    HostFuncAVSamplesGetBuffer.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{BufferPtr, 0, 2, 2, 2,
  //        0}, Result);
  //
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_freep");
  auto &HostFuncAVFreep =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFreep &>(
          FuncInst->getHostFunc());

  {
    uint32_t BufferId = readUInt32(MemInst, BufferPtr);
    HostFuncAVFreep.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{BufferId},
                        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}
