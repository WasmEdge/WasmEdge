// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ffmpeg_env.h"
#include "avutil/avFrame.h"

#include "utils.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST(FFmpegEnvHandleIds, TakeFreeHandleSkipsNullAndLiveIdsAcrossWrap) {
  constexpr uint32_t Max = std::numeric_limits<uint32_t>::max();
  std::map<uint32_t, int> Live;
  uint32_t Next = Max - 1;

  EXPECT_EQ(detail::takeFreeHandle(Next, Live), Max - 1);
  Live.emplace(Max - 1, 0);
  EXPECT_EQ(detail::takeFreeHandle(Next, Live), Max);
  Live.emplace(Max, 0);
  Live.emplace(1, 0);
  Live.emplace(2, 0);

  // The counter wraps here: 0 stays the null handle and the ids that are
  // still live must not be handed out again.
  EXPECT_EQ(detail::takeFreeHandle(Next, Live), UINT32_C(3));
  Live.emplace(3, 0);
  EXPECT_EQ(detail::takeFreeHandle(Next, Live), UINT32_C(4));
  EXPECT_EQ(Next, UINT32_C(5));

  Live.emplace(4, 0);
  Live.erase(1);
  Next = 0;
  EXPECT_EQ(detail::takeFreeHandle(Next, Live), UINT32_C(1));
  EXPECT_EQ(detail::takeFreeHandle(Next, Live), UINT32_C(5));
}

// Strands an owned object of each kind the plugin allocates (device lists
// aside), then drops the module instances so the shared env is destroyed:
// everything the guest never freed must go with it, including the two open
// files.
TEST_F(FFmpegTest, EnvTeardownReclaimsStrandedObjects) {
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);
  ASSERT_TRUE(AVFilterMod != nullptr);
  ASSERT_TRUE(SWScaleMod != nullptr);
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto IdAt = [&](uint32_t Ptr) {
    uint32_t Id = readUInt32(MemInst, Ptr);
    EXPECT_TRUE(Id > 0);
    return Id;
  };
  int32_t const Ok = static_cast<int32_t>(ErrNo::Success);
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  int const Before = countOpenDescriptors();
#endif

  // Demuxer with a borrowed codecpar and metadata view.
  uint32_t InCtxPtr = UINT32_C(4);
  uint32_t FilePtr = UINT32_C(200);
  initFormatCtx(InCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t InCtxId = IdAt(InCtxPtr);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_codecpar",
                {InCtxId, UINT32_C(0), UINT32_C(8)}),
            Ok);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformatContext_metadata",
                {InCtxId, UINT32_C(12)}),
            Ok);

  // Muxer with an open file.
  uint32_t OutCtxPtr = UINT32_C(16);
  uint32_t FormatPtr = UINT32_C(250);
  uint32_t OutFilePtr = UINT32_C(270);
  std::string Format("matroska");
  std::string OutFile("teardown.mkv");
  fillMemContent(MemInst, FormatPtr, Format);
  fillMemContent(MemInst, OutFilePtr, OutFile);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
                {OutCtxPtr, UINT32_C(0), FormatPtr,
                 static_cast<uint32_t>(Format.length()), OutFilePtr,
                 static_cast<uint32_t>(OutFile.length())}),
            0);
  uint32_t OutCtxId = IdAt(OutCtxPtr);
  EXPECT_GE(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avio_open",
                {OutCtxId, OutFilePtr, static_cast<uint32_t>(OutFile.length()),
                 INT32_C(2)}),
            0);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avchapter_mallocz",
                {UINT32_C(20)}),
            Ok);
  IdAt(UINT32_C(20));
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  EXPECT_EQ(countOpenDescriptors(), Before + 2);
#endif

  // Frame with planes, packet with a payload, an opened decoder, standalone
  // codec parameters, an owned dictionary and a sample-plane array.
  initEmptyFrame(UINT32_C(24));
  uint32_t FrameId = IdAt(UINT32_C(24));
  EXPECT_EQ(Run(AVUtilMod.get(),
                "wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(1)}),
            Ok);
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_width",
                {FrameId, UINT32_C(16)}),
            Ok);
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(16)}),
            Ok);
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                {FrameId, INT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_metadata",
                {FrameId, UINT32_C(28)}),
            Ok);
  allocPacket(UINT32_C(32));
  uint32_t PacketId = IdAt(UINT32_C(32));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_new_packet",
                {PacketId, INT32_C(64)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
                {UINT32_C(27), UINT32_C(36)}),
            Ok);
  uint32_t CodecId = IdAt(UINT32_C(36));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, UINT32_C(40)}),
            Ok);
  uint32_t DecCtxId = IdAt(UINT32_C(40));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_open2",
                {DecCtxId, CodecId, UINT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc",
                {UINT32_C(44)}),
            Ok);
  IdAt(UINT32_C(44));
  initDict(UINT32_C(48), UINT32_C(300), "KEY", UINT32_C(303), "VALUE");
  IdAt(UINT32_C(48));
  EXPECT_GE(Run(AVUtilMod.get(),
                "wasmedge_ffmpeg_avutil_av_samples_alloc_array_and_samples",
                {UINT32_C(52), UINT32_C(56), INT32_C(2), INT32_C(256),
                 UINT32_C(2), INT32_C(0)}),
            0);
  IdAt(UINT32_C(52));

  // Resampler, scaler, scaler filter with a borrowed vector, an owned
  // vector, a configured filter graph, a filter in/out node and a pad view.
  uint64_t const StereoId =
      FFmpegUtils::ChannelLayout::intoChannelLayoutID(AV_CH_LAYOUT_STEREO);
  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
          {UINT32_C(60), UINT32_C(0), StereoId, UINT32_C(2), INT32_C(48000),
           StereoId, UINT32_C(2), INT32_C(44100), INT32_C(0)}),
      Ok);
  uint32_t SwrId = IdAt(UINT32_C(60));
  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_init", {SwrId}),
      0);
  EXPECT_EQ(
      Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_getContext",
          {UINT32_C(64), INT32_C(16), INT32_C(16), UINT32_C(1), INT32_C(16),
           INT32_C(16), UINT32_C(3), INT32_C(2), UINT32_C(0), UINT32_C(0)}),
      Ok);
  IdAt(UINT32_C(64));
  float const Zero = 0.0F;
  EXPECT_EQ(Run(SWScaleMod.get(),
                "wasmedge_ffmpeg_swscale_sws_getDefaultFilter",
                {UINT32_C(68), Zero, Zero, Zero, Zero, Zero, Zero, INT32_C(0)}),
            Ok);
  uint32_t FilterId = IdAt(UINT32_C(68));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_getLumaH",
                {FilterId, UINT32_C(72)}),
            Ok);
  IdAt(UINT32_C(72));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_allocVec",
                {UINT32_C(76), INT32_C(8)}),
            Ok);
  IdAt(UINT32_C(76));
  std::string Spec(
      "abuffer=time_base=1/44100:sample_rate=44100:sample_fmt=fltp:"
      "channel_layout=0x3,anull,abuffersink");
  uint32_t SpecPtr = UINT32_C(400);
  fillMemContent(MemInst, SpecPtr, Spec);
  EXPECT_EQ(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_alloc",
                {UINT32_C(80)}),
            Ok);
  uint32_t GraphId = IdAt(UINT32_C(80));
  EXPECT_GE(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_parse_ptr",
                {GraphId, SpecPtr, static_cast<int32_t>(Spec.length()),
                 UINT32_C(0), UINT32_C(0)}),
            0);
  EXPECT_GE(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_config", {GraphId}),
            0);
  EXPECT_EQ(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_inout_alloc",
                {UINT32_C(84)}),
            Ok);
  IdAt(UINT32_C(84));
  std::string FilterName("abuffer");
  uint32_t FilterNamePtr = UINT32_C(600);
  fillMemContent(MemInst, FilterNamePtr, FilterName);
  EXPECT_EQ(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_get_by_name",
                {UINT32_C(88), FilterNamePtr,
                 static_cast<int32_t>(FilterName.length())}),
            Ok);
  uint32_t AbufferId = IdAt(UINT32_C(88));
  EXPECT_EQ(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_get_outputs_filter_pad",
                {AbufferId, UINT32_C(92)}),
            Ok);
  IdAt(UINT32_C(92));

  // Dropping every module releases the last references to the shared env.
  AVFormatMod.reset();
  AVCodecMod.reset();
  AVUtilMod.reset();
  AVFilterMod.reset();
  SWScaleMod.reset();
  SWResampleMod.reset();
  AVDeviceMod.reset();
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  EXPECT_EQ(countOpenDescriptors(), Before);
#endif
}

// The SDK never releases a borrowed view, so every getter that returns one
// must hand out one id per object: a second call returns the id of the
// first instead of a new handle that only the parent's free would release.
// Each request holds a reference, so a guest dropping one copy of the id
// leaves the other valid.
TEST_F(FFmpegTest, ViewGettersKeepOneIdPerObject) {
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);
  ASSERT_TRUE(AVFilterMod != nullptr);
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto Env =
      dynamic_cast<AVUtil::AVFrameAlloc &>(
          AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc")
              ->getHostFunc())
          .getEnv();
  int32_t const Ok = static_cast<int32_t>(ErrNo::Success);
  auto IdAt = [&](uint32_t Ptr) { return readUInt32(MemInst, Ptr); };
  uint32_t SlotPtr = UINT32_C(4);
  auto Twice = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                   const char *Name,
                   std::initializer_list<WasmEdge::ValVariant> Args) {
    EXPECT_EQ(Run(TargetMod, Name, Args), Ok) << Name;
    uint32_t const First = readUInt32(MemInst, SlotPtr);
    EXPECT_TRUE(First > 0) << Name;
    EXPECT_EQ(Run(TargetMod, Name, Args), Ok) << Name;
    EXPECT_EQ(readUInt32(MemInst, SlotPtr), First) << Name;
    return First;
  };
  // Drops an id handed out twice: the first drop leaves it valid.
  auto DropTwice = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                       const char *Drop, uint32_t Id) {
    EXPECT_EQ(Run(TargetMod, Drop, {Id}), Ok) << Drop;
    EXPECT_NE(Env->fetchData(Id), nullptr) << Drop;
    EXPECT_EQ(Run(TargetMod, Drop, {Id}), Ok) << Drop;
    EXPECT_EQ(Env->fetchData(Id), nullptr) << Drop;
  };

  // Format context, stream and chapter views.
  initFormatCtx(UINT32_C(8), UINT32_C(200),
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t InCtxId = IdAt(UINT32_C(8));
  ASSERT_TRUE(InCtxId > 0);
  Twice(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avformatContext_metadata",
        {InCtxId, SlotPtr});
  uint32_t const ParId =
      Twice(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_codecpar",
            {InCtxId, UINT32_C(0), SlotPtr});
  Twice(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_metadata",
        {InCtxId, UINT32_C(0), SlotPtr});
  EXPECT_NE(Twice(AVFormatMod.get(),
                  "wasmedge_ffmpeg_avformat_avStream_codecpar",
                  {InCtxId, UINT32_C(1), SlotPtr}),
            ParId);
  DropTwice(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_parameters_free",
            ParId);

  std::string Format("matroska");
  std::string File("view_ids.mkv");
  fillMemContent(MemInst, UINT32_C(300), Format);
  fillMemContent(MemInst, UINT32_C(320), File);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
                {UINT32_C(12), UINT32_C(0), UINT32_C(300),
                 static_cast<uint32_t>(Format.length()), UINT32_C(320),
                 static_cast<uint32_t>(File.length())}),
            0);
  uint32_t OutCtxId = IdAt(UINT32_C(12));
  ASSERT_TRUE(OutCtxId > 0);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avchapter_mallocz",
                {UINT32_C(16)}),
            Ok);
  uint32_t ChapterId = IdAt(UINT32_C(16));
  ASSERT_TRUE(ChapterId > 0);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avchapter_dynarray_add",
                {OutCtxId, INT32_C(20), ChapterId}),
            Ok);
  Twice(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avChapter_metadata",
        {OutCtxId, UINT32_C(0), SlotPtr});

  // Frame metadata; a view every holder dropped is handed out afresh.
  initEmptyFrame(UINT32_C(24));
  uint32_t FrameId = IdAt(UINT32_C(24));
  ASSERT_TRUE(FrameId > 0);
  uint32_t const ViewId =
      Twice(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_metadata",
            {FrameId, SlotPtr});
  DropTwice(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_dict_free", ViewId);
  EXPECT_NE(Twice(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_metadata",
                  {FrameId, SlotPtr}),
            ViewId);

  // Filter contexts of a configured graph.
  std::string Spec("abuffer=time_base=1/44100:sample_rate=44100:"
                   "sample_fmt=fltp:channel_layout=0x3,anull,abuffersink");
  fillMemContent(MemInst, UINT32_C(400), Spec);
  EXPECT_EQ(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_alloc",
                {UINT32_C(28)}),
            Ok);
  uint32_t GraphId = IdAt(UINT32_C(28));
  ASSERT_TRUE(GraphId > 0);
  EXPECT_GE(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_parse_ptr",
                {GraphId, UINT32_C(400), static_cast<int32_t>(Spec.length()),
                 UINT32_C(0), UINT32_C(0)}),
            0);
  EXPECT_GE(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_config", {GraphId}),
            0);
  std::string Name("Parsed_abuffersink_2");
  fillMemContent(MemInst, UINT32_C(600), Name);
  uint32_t const SinkId = Twice(
      AVFilterMod.get(), "wasmedge_ffmpeg_avfilter_avfilter_graph_get_filter",
      {SlotPtr, GraphId, UINT32_C(600), static_cast<uint32_t>(Name.length())});
  DropTwice(AVFilterMod.get(), "wasmedge_ffmpeg_avfilter_avfilter_context_drop",
            SinkId);

  // The four vectors of a scaler filter, one id each.
  float const Zero = 0.0F;
  EXPECT_EQ(Run(SWScaleMod.get(),
                "wasmedge_ffmpeg_swscale_sws_getDefaultFilter",
                {UINT32_C(32), Zero, Zero, Zero, Zero, Zero, Zero, INT32_C(0)}),
            Ok);
  uint32_t FilterId = IdAt(UINT32_C(32));
  ASSERT_TRUE(FilterId > 0);
  std::map<uint32_t, int> VectorIds;
  for (const char *Getter : {"wasmedge_ffmpeg_swscale_sws_getLumaH",
                             "wasmedge_ffmpeg_swscale_sws_getLumaV",
                             "wasmedge_ffmpeg_swscale_sws_getChromaH",
                             "wasmedge_ffmpeg_swscale_sws_getChromaV"}) {
    ++VectorIds[Twice(SWScaleMod.get(), Getter, {FilterId, SlotPtr})];
  }
  EXPECT_EQ(VectorIds.size(), 4U);
  DropTwice(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_freeVec",
            VectorIds.begin()->first);

  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_freeFilter",
                {FilterId}),
            Ok);
  EXPECT_EQ(Run(AVFilterMod.get(),
                "wasmedge_ffmpeg_avfilter_avfilter_graph_free", {GraphId}),
            Ok);
  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      Ok);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_free_context", {OutCtxId}),
            Ok);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {InCtxId}),
            Ok);
}

// Codec, filter and format descriptors are static and pad views describe a
// static pad array, so a guest asking for one again gets the same id with
// another reference instead of a new handle; a drop gives one back, and the
// id goes with the last.
TEST_F(FFmpegTest, DescriptorAndPadGettersShareCountedIds) {
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);
  ASSERT_TRUE(AVFilterMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto Env =
      dynamic_cast<AVUtil::AVFrameAlloc &>(
          AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc")
              ->getHostFunc())
          .getEnv();
  int32_t const Ok = static_cast<int32_t>(ErrNo::Success);
  auto IdAt = [&](uint32_t Ptr) { return readUInt32(MemInst, Ptr); };
  uint32_t SlotPtr = UINT32_C(4);
  auto Twice = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                   const char *Name,
                   std::initializer_list<WasmEdge::ValVariant> Args) {
    EXPECT_GE(Run(TargetMod, Name, Args), 0) << Name;
    uint32_t const First = readUInt32(MemInst, SlotPtr);
    EXPECT_TRUE(First > 0) << Name;
    EXPECT_GE(Run(TargetMod, Name, Args), 0) << Name;
    EXPECT_EQ(readUInt32(MemInst, SlotPtr), First) << Name;
    return First;
  };
  auto DropTwice = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                       const char *Drop, uint32_t Id) {
    EXPECT_EQ(Run(TargetMod, Drop, {Id}), Ok) << Drop;
    EXPECT_NE(Env->fetchData(Id), nullptr) << Drop;
    EXPECT_EQ(Run(TargetMod, Drop, {Id}), Ok) << Drop;
    EXPECT_EQ(Env->fetchData(Id), nullptr) << Drop;
  };

  // A codec found twice, and the codec of a context opened with it.
  uint32_t const CodecId =
      Twice(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
            {UINT32_C(27), SlotPtr});
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, UINT32_C(8)}),
            Ok);
  uint32_t CtxId = IdAt(UINT32_C(8));
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Twice(AVCodecMod.get(),
                  "wasmedge_ffmpeg_avcodec_avcodeccontext_codec",
                  {CtxId, SlotPtr}),
            CodecId);

  // A filter, and the view of its output pads.
  std::string FilterName("abuffer");
  fillMemContent(MemInst, UINT32_C(100), FilterName);
  uint32_t const FilterId = Twice(
      AVFilterMod.get(), "wasmedge_ffmpeg_avfilter_avfilter_get_by_name",
      {SlotPtr, UINT32_C(100), static_cast<int32_t>(FilterName.length())});
  uint32_t const PadsId =
      Twice(AVFilterMod.get(),
            "wasmedge_ffmpeg_avfilter_avfilter_get_outputs_filter_pad",
            {FilterId, SlotPtr});
  DropTwice(AVFilterMod.get(), "wasmedge_ffmpeg_avfilter_avfilter_pad_drop",
            PadsId);
  DropTwice(AVFilterMod.get(), "wasmedge_ffmpeg_avfilter_avfilter_drop",
            FilterId);

  // The input and output formats of a demuxer and a muxer.
  initFormatCtx(UINT32_C(12), UINT32_C(200),
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t InCtxId = IdAt(UINT32_C(12));
  ASSERT_TRUE(InCtxId > 0);
  DropTwice(AVFormatMod.get(),
            "wasmedge_ffmpeg_avformat_avInputOutputFormat_free",
            Twice(AVFormatMod.get(),
                  "wasmedge_ffmpeg_avformat_avformatContext_iformat",
                  {InCtxId, SlotPtr}));
  std::string Format("matroska");
  std::string File("shared_ids.mkv");
  fillMemContent(MemInst, UINT32_C(300), Format);
  fillMemContent(MemInst, UINT32_C(320), File);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
                {UINT32_C(16), UINT32_C(0), UINT32_C(300),
                 static_cast<uint32_t>(Format.length()), UINT32_C(320),
                 static_cast<uint32_t>(File.length())}),
            0);
  uint32_t OutCtxId = IdAt(UINT32_C(16));
  ASSERT_TRUE(OutCtxId > 0);
  DropTwice(AVFormatMod.get(),
            "wasmedge_ffmpeg_avformat_avInputOutputFormat_free",
            Twice(AVFormatMod.get(),
                  "wasmedge_ffmpeg_avformat_avformatContext_oformat",
                  {OutCtxId, SlotPtr}));

  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            Ok);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_free_context", {OutCtxId}),
            Ok);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {InCtxId}),
            Ok);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
