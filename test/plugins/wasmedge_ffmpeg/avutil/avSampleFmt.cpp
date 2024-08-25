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
  auto &HostFuncAVGetPackedSampleFmt = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetPackedSampleFmt &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetPackedSampleFmt.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), SampleFmtId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_planar_sample_fmt");
  auto &HostFuncAVGetPlanarSampleFmt = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetPlanarSampleFmt &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetPlanarSampleFmt.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 6);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_sample_fmt_is_planar");
  auto &HostFuncAVSampleFmtIsPlanar = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSampleFmtIsPlanar &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVSampleFmtIsPlanar.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    EXPECT_EQ(Result[0].get<uint32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_bytes_per_sample");
  auto &HostFuncAVGetBytesPerSample = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetBytesPerSample &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetBytesPerSample.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

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
  auto &HostFuncAVSamplesGetBufferSize = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSamplesGetBufferSize &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVSamplesAllocArrayAndSamples = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSamplesAllocArrayAndSamples &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVGetSampleFmtNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetSampleFmtNameLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetSampleFmtNameLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_name");
  auto &HostFuncAVGetSampleFmtName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetSampleFmtName &>(
      FuncInst->getHostFunc());

  // Fill Memory with 0.
  fillMemContent(MemInst, NamePtr, Length);
  {
    HostFuncAVGetSampleFmtName.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       SampleFmtId, NamePtr, Length},
                                   Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_mask");
  auto &HostFuncAVGetSampleFmtMask = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetSampleFmtMask &>(
      FuncInst->getHostFunc());

  {
    uint32_t SampleId = 2; // AV_SAMPLE_FMT_S16;
    HostFuncAVGetSampleFmtMask.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

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

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
