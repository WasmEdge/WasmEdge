// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

  uint32_t SampleFmtId = 1; // AV_SAMPLE_FMT_U8
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
    EXPECT_TRUE(HostFuncAVGetBytesPerSample.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SampleFmtId},
        Result));

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
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

TEST_F(FFmpegTest, AVSampleFmtNameBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_name");
  auto &HostFuncAVGetSampleFmtName = FuncInst->getHostFunc();

  uint32_t NamePtr = UINT32_C(4);
  uint32_t SampleFmtId = 1; // AV_SAMPLE_FMT_U8, name "u8".
  // NameLen is guest-controlled and far larger than the source name.
  uint32_t NameLen = UINT32_C(64);
  fillMemContent(MemInst, NamePtr, NameLen, UINT8_C(0xAA));

  HostFuncAVGetSampleFmtName.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     SampleFmtId, NamePtr, NameLen},
                                 Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // The host must write exactly the name and its terminator ("u8\0"); the
  // fence past the terminator detects any over-copy of adjacent host bytes.
  char *Buf = MemInst->getPointer<char *>(NamePtr);
  EXPECT_STREQ(Buf, "u8");
  uint32_t WrittenLen = 0;
  while (WrittenLen < NameLen && Buf[WrittenLen] != '\0') {
    ++WrittenLen;
  }
  for (uint32_t I = WrittenLen + 1; I < NameLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }
}

TEST_F(FFmpegTest, AVSampleFmtNameInvalid) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t NamePtr = UINT32_C(4);
  uint32_t NameLen = UINT32_C(16);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_name");
  auto &HostFuncAVGetSampleFmtName = FuncInst->getHostFunc();

  // av_get_sample_fmt_name returns nullptr for AV_SAMPLE_FMT_NONE (id 0); the
  // host writes an empty string and reports success, matching the length
  // getter's 0.
  fillMemContent(MemInst, NamePtr, NameLen, UINT8_C(0xFF));
  HostFuncAVGetSampleFmtName.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     UINT32_C(0), NamePtr, NameLen},
                                 Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readUInt32(MemInst, NamePtr) & 0xFFU, UINT32_C(0));

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_sample_fmt_name_length");
  auto &HostFuncAVGetSampleFmtNameLength = FuncInst->getHostFunc();

  HostFuncAVGetSampleFmtNameLength.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 0);
}

TEST_F(FFmpegTest, AVSampleFmtGetBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_get_sample_fmt");
  auto &HostFuncAVGetSampleFmt = FuncInst->getHostFunc();

  // The name pointer is in bounds but the guest-declared length runs off the
  // end of linear memory; the host must reject it, not read past the page.
  uint32_t OutOfBoundsStrPtr = UINT32_C(65000);
  uint32_t OutOfBoundsStrLen = UINT32_C(2000);
  HostFuncAVGetSampleFmt.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 OutOfBoundsStrPtr, OutOfBoundsStrLen},
                             Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

// av_freep mirrors FFmpeg's null-safe free: the null handle or an already
// released id is a no-op success, while a live handle of another type is
// rejected before any id-keyed deallocation.
TEST_F(FFmpegTest, AVFreepNullSafeAndIdempotent) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_freep", {UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));

  uint32_t BufferPtr = UINT32_C(4);
  uint32_t LinesizePtr = UINT32_C(8);
  writeUInt32(MemInst, UINT32_C(0), BufferPtr);
  EXPECT_GE(Run("wasmedge_ffmpeg_avutil_av_samples_alloc_array_and_samples",
                {BufferPtr, LinesizePtr, INT32_C(1), INT32_C(5), UINT32_C(1),
                 INT32_C(1)}),
            0);
  uint32_t BufferId = readUInt32(MemInst, BufferPtr);
  ASSERT_TRUE(BufferId > 0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_freep", {BufferId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_freep", {BufferId}),
            static_cast<int32_t>(ErrNo::Success));

  uint32_t FramePtr = UINT32_C(12);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_freep", {FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));

  // The frame handle survived the rejected free and still frees normally.
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
