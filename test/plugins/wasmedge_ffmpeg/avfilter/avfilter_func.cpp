// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avfilter//avfilter_func.h"
#include "avfilter/avFilter.h"
#include "avfilter/buffer_source_sink.h"
#include "avfilter/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVFilterFunc) {
  ASSERT_TRUE(AVFilterMod != nullptr);

  // Structs Ptr
  uint32_t FilterGraphPtr = UINT32_C(4);
  uint32_t FilterPtr = UINT32_C(8);
  uint32_t Filter2Ptr = UINT32_C(12);
  uint32_t InputFilterCtxPtr = UINT32_C(28);  // AVFilterContext
  uint32_t OutputFilterCtxPtr = UINT32_C(24); // AVFilterContext
  uint32_t InputInOutPtr = UINT32_C(32);
  uint32_t OutputInOutPtr = UINT32_C(36);
  uint32_t FramePtr = UINT32_C(40);

  // Strings.
  uint32_t InputNamePtr = UINT32_C(100);
  uint32_t OutputNamePtr = UINT32_C(150);
  uint32_t InputFilterNamePtr = UINT32_C(200);
  uint32_t OutputFilterNamePtr = UINT32_C(250);
  uint32_t ArgsPtr = UINT32_C(300);
  uint32_t SpecPtr = UINT32_C(450);
  uint32_t StrPtr = UINT32_C(500);

  std::string InputName = std::string("abuffer");
  fillMemContent(MemInst, InputNamePtr, InputName);

  std::string OutputName = std::string("abuffersink");
  fillMemContent(MemInst, OutputNamePtr, OutputName);

  std::string InputFilterName = std::string("in");
  fillMemContent(MemInst, InputFilterNamePtr, InputFilterName);

  std::string OutputFilterName = std::string("out");
  fillMemContent(MemInst, OutputFilterNamePtr, OutputFilterName);

  std::string Args = std::string(
      "time_base=1/44100:sample_rate=44100:sample_fmt=fltp:channel_layout=0x3");
  fillMemContent(MemInst, ArgsPtr, Args);

  std::string SpecStr = std::string("anull");
  fillMemContent(MemInst, SpecPtr, SpecStr);

  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);

  auto *FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_alloc");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphAlloc = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterGraphAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterGraphPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t FilterGraphId = readUInt32(MemInst, FilterGraphPtr);
  ASSERT_TRUE(FilterGraphId > 0);

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_by_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGetByName = FuncInst->getHostFunc();

  {
    int32_t Length = InputName.length();
    EXPECT_TRUE(HostFuncAVFilterGetByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterPtr, InputNamePtr,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    Length = OutputName.length();
    EXPECT_TRUE(HostFuncAVFilterGetByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Filter2Ptr, OutputNamePtr,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  uint32_t Filter2Id = readUInt32(MemInst, Filter2Ptr);
  ASSERT_TRUE(FilterId > 0);
  ASSERT_TRUE(Filter2Id > 0);

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_create_filter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphCreateFilter = FuncInst->getHostFunc();

  {
    int32_t NameLen = InputFilterName.length();
    int32_t ArgsLen = Args.length();
    EXPECT_TRUE(HostFuncAVFilterGraphCreateFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputFilterCtxPtr, FilterId, InputFilterNamePtr, NameLen, ArgsPtr,
            ArgsLen, FilterGraphId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
    writeUInt32(MemInst, 0, InputFilterCtxPtr); // Setting InputFilterCtx to 0

    NameLen = OutputFilterName.length();
    EXPECT_TRUE(HostFuncAVFilterGraphCreateFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutputFilterCtxPtr, Filter2Id, OutputFilterNamePtr, NameLen, 0, 0,
            FilterGraphId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
    writeUInt32(MemInst, 0, OutputFilterCtxPtr); // Setting OutputFilterCtx to 0
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_inout_alloc");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterInOutAlloc = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterInOutAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputInOutPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVFilterInOutAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{OutputInOutPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t InputInOutId = readUInt32(MemInst, InputInOutPtr);
  ASSERT_TRUE(InputInOutId > 0);

  uint32_t OutputInOutId = readUInt32(MemInst, OutputInOutPtr);
  ASSERT_TRUE(OutputInOutId > 0);

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_get_filter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphGetFilter = FuncInst->getHostFunc();

  {
    int32_t Length = OutputFilterName.length();
    EXPECT_TRUE(HostFuncAVFilterGraphGetFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            OutputFilterCtxPtr, FilterGraphId, OutputFilterNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    Length = InputFilterName.length();
    EXPECT_TRUE(HostFuncAVFilterGraphGetFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputFilterCtxPtr, FilterGraphId, InputFilterNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t OutputFilterCtxId = readUInt32(MemInst, OutputFilterCtxPtr);
  ASSERT_TRUE(OutputFilterCtxId > 0);

  uint32_t InputFilterCtxId = readUInt32(MemInst, InputFilterCtxPtr);
  ASSERT_TRUE(InputFilterCtxId > 0);

  // ==================================================================
  //                  Setting InOutId Values for Filtering
  // ==================================================================

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_inout_set_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterInOutSetName = FuncInst->getHostFunc();

  {
    int32_t Length = InputFilterName.length();
    EXPECT_TRUE(HostFuncAVFilterInOutSetName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputInOutId,
                                                    InputFilterNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    Length = OutputFilterName.length();
    EXPECT_TRUE(HostFuncAVFilterInOutSetName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            InputInOutId, OutputFilterNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_inout_set_filter_ctx");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterInOutSetFilterCtx = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterInOutSetFilterCtx.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputInOutId,
                                                    InputFilterCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVFilterInOutSetFilterCtx.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputInOutId,
                                                    OutputFilterCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_inout_set_pad_idx");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterInOutSetPadIdx = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterInOutSetPadIdx.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputInOutId, 0}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVFilterInOutSetPadIdx.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputInOutId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_inout_set_next");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterInOutSetNext = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterInOutSetNext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputInOutId, 0}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVFilterInOutSetNext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputInOutId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  // ==================================================================
  //                  End Setting InOutId Values for Filtering
  // ==================================================================

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_parse_ptr");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphParsePtr = FuncInst->getHostFunc();

  {
    int32_t Length = SpecStr.length();
    EXPECT_TRUE(HostFuncAVFilterGraphParsePtr.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FilterGraphId, SpecPtr, Length, InputInOutId, OutputInOutId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_config");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphConfig = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterGraphConfig.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterGraphId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_dump_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphDumpLength = FuncInst->getHostFunc();

  int32_t GraphStrLen = 0;
  {
    EXPECT_TRUE(HostFuncAVFilterGraphDumpLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterGraphId},
        Result));
    GraphStrLen = Result[0].get<int32_t>();
    ASSERT_TRUE(GraphStrLen > 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_dump");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphDump = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterGraphDump.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterGraphId, StrPtr,
                                                    GraphStrLen},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  // Crashing the program. Checked even from Rust side.

  //  FuncInst = AVFilterMod->findFuncExports(
  //      "wasmedge_ffmpeg_avfilter_avfilter_inout_free");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVFilterInOutFree = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterInOutFree &>(
  //      FuncInst->getHostFunc());
  //
  //  {
  //    EXPECT_TRUE(HostFuncAVFilterInOutFree.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{InputInOutId}, Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //
  //    EXPECT_TRUE(HostFuncAVFilterInOutFree.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{OutputInOutId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_version");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterVersion = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    ASSERT_TRUE((Result[0].get<int32_t>() >> 16) > 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_configuration_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterConfigurationLength = FuncInst->getHostFunc();

  int32_t Length = 0;
  {
    EXPECT_TRUE(HostFuncAVFilterConfigurationLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_configuration");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterConfiguration = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_NE(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_license_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterLicenseLength = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_license");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterLicense = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  // ==================================================================
  //              Start Test AVBufferSource, AVBufferSink Funcs
  // ==================================================================

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_av_buffersrc_get_nb_failed_requests");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVBufferSrcGetNbFailedRequests = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVBufferSrcGetNbFailedRequests.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFilterCtxId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_av_buffersrc_add_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVBufferSrcAddFrame = FuncInst->getHostFunc();

  // Returning Error Code -22 (Invalid Argument), Due to Passing Empty Frame.
  {
    EXPECT_TRUE(HostFuncAVBufferSrcAddFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFilterCtxId, FrameId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>());
  }

  // Need to send the last frame. Then only this test will pass. Else Null
  // pointer exception.
  //  FuncInst = AVFilterMod->findFuncExports(
  //      "wasmedge_ffmpeg_avfilter_av_buffersrc_close");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVBufferSrcClose = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVBufferSrcClose &>(
  //      FuncInst->getHostFunc());
  //
  //  {
  //    int64_t Pts = 20;
  //    uint32_t Flags = 30;
  //    EXPECT_TRUE(HostFuncAVBufferSrcClose.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{InputFilterCtxPtr, Pts,
  //                                                    Flags},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  // Passing Empty frames. Return AVERROR due to no frames presen Return AVERROR
  // due to no frames present.
  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_av_buffersink_get_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVBufferSinkGetFrame = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVBufferSinkGetFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterCtxId, FrameId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>());
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_av_buffersink_get_samples");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVBufferSinkGetSamples = FuncInst->getHostFunc();

  // Passing Empty frames. Return AVERROR due to no frames presen Return AVERROR
  // due to no frames present.
  {
    EXPECT_TRUE(HostFuncAVBufferSinkGetSamples.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterCtxId, FrameId,
                                                    20},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>());
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_av_buffersink_set_frame_size");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAvBufferSinkSetFrameSize = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAvBufferSinkSetFrameSize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterCtxId, 30},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  // ==================================================================
  //              End Test AVBufferSource, AVBufferSink Funcs
  // ==================================================================

  // ==================================================================
  //                        Clean Memory
  // ==================================================================

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_free_graph_str");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterFreeGraphStr = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterFreeGraphStr.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterGraphId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_drop");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterDrop = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterDrop.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_context_drop");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterContextDrop = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterContextDrop.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFilterCtxId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVFilterContextDrop.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_graph_free");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGraphFree = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterGraphFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterGraphId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  // ==================================================================
  //                        End Clean Memory
  // ==================================================================
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
