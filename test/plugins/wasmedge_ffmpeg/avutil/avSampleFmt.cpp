// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/module.h"
#include "avutil/samplefmt.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVSampleFmt) {
  using namespace std::literals::string_view_literals;
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t BufferPtr = UINT32_C(160);
  uint32_t NamePtr = UINT32_C(80);
  uint32_t LinesizePtr = UINT32_C(20);

  uint32_t SampleFmtId = 1; // AV_SAMPLE_FMT_S32
  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_packed_sample_fmt");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetPackedSampleFmt = FuncInst->getHostFunc();

  {
    HostFuncAVGetPackedSampleFmt.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), SampleFmtId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_planar_sample_fmt");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetPlanarSampleFmt = FuncInst->getHostFunc();

  {
    HostFuncAVGetPlanarSampleFmt.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 6);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_sample_fmt_is_planar");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVSampleFmtIsPlanar = FuncInst->getHostFunc();

  {
    HostFuncAVSampleFmtIsPlanar.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_bytes_per_sample");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetBytesPerSample = FuncInst->getHostFunc();

  {
    HostFuncAVGetBytesPerSample.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_get_sample_fmt");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetSampleFmt = FuncInst->getHostFunc();

  uint32_t SampleFmtStart = 100;
  uint32_t SampleFmtSize = 2;
  fillMemContent(MemInst, SampleFmtSize, SampleFmtSize);

  fillMemContent(MemInst, SampleFmtStart, "u8"sv);
  {
    HostFuncAVGetSampleFmt.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   SampleFmtStart, SampleFmtSize},
                               Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_samples_get_buffer_size");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVSamplesGetBufferSize = FuncInst->getHostFunc();

  int32_t NbChannels = 1;
  int32_t NbSamples = 5;
  int32_t Align = 1;
  int32_t BufSize = 0;
  {
    HostFuncAVSamplesGetBufferSize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NbChannels, NbSamples,
                                                    SampleFmtId, Align},
        Result);

    BufSize = Result[0].get<int32_t>();
    EXPECT_TRUE(BufSize);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_samples_alloc_array_and_samples");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVSamplesAllocArrayAndSamples = FuncInst->getHostFunc();

  {
    HostFuncAVSamplesAllocArrayAndSamples.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            BufferPtr, LinesizePtr, NbChannels, NbSamples, SampleFmtId, Align},
        Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  uint32_t BufId = readUInt32(MemInst, BufferPtr);
  ASSERT_TRUE(BufId > 0);

  int32_t Length = 0;
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_name_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetSampleFmtNameLength = FuncInst->getHostFunc();

  {
    HostFuncAVGetSampleFmtNameLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetSampleFmtName = FuncInst->getHostFunc();

  // Fill Memory with 0.
  fillMemContent(MemInst, NamePtr, Length);
  {
    HostFuncAVGetSampleFmtName.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       SampleFmtId, NamePtr, Length},
                                   Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(NamePtr),
                               static_cast<size_t>(Length)),
              "u8"sv);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_mask");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetSampleFmtMask = FuncInst->getHostFunc();

  {
    uint32_t SampleId = 2; // AV_SAMPLE_FMT_S16;
    HostFuncAVGetSampleFmtMask.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_freep");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFreep = FuncInst->getHostFunc();

  {
    uint32_t BufferId = readUInt32(MemInst, BufferPtr);
    HostFuncAVFreep.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{BufferId},
                        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
