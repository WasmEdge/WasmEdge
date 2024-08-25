// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avfilter/avFilter.h"
#include "avfilter//avfilter_func.h"
#include "avfilter/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVFilterStructs) {
  ASSERT_TRUE(AVFilterMod != nullptr);

  uint32_t FilterPtr = UINT32_C(8);
  uint32_t InputFilterPadPtr = UINT32_C(12);
  uint32_t OutputFilterPadPtr = UINT32_C(16);
  uint32_t InputNamePtr = UINT32_C(100);
  uint32_t StrPtr = UINT32_C(150);

  std::string InputName = std::string("abuffer");
  fillMemContent(MemInst, InputNamePtr, InputName);

  // ==================================================================
  //               Start Initialize AVFilter
  // ==================================================================

  auto *FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_by_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGetByName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetByName &>(
      FuncInst->getHostFunc());

  {
    int32_t Length = InputName.length();
    EXPECT_TRUE(HostFuncAVFilterGetByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterPtr, InputNamePtr,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  ASSERT_TRUE(FilterId > 0);
  // ==================================================================
  //               End Initialize AVFilter
  // ==================================================================

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_name_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterNameLength &>(
      FuncInst->getHostFunc());

  int32_t Length = 0;
  {
    EXPECT_TRUE(HostFuncAVFilterNameLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterName =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterName &>(
          FuncInst->getHostFunc());

  fillMemContent(MemInst, StrPtr, Length);
  {
    EXPECT_TRUE(HostFuncAVFilterName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_description_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterDescriptionLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterDescriptionLength &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterDescriptionLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_description");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterDescription = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterDescription &>(
      FuncInst->getHostFunc());

  fillMemContent(MemInst, StrPtr, Length);
  {
    EXPECT_TRUE(HostFuncAVFilterDescription.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_nb_inputs");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterNbInputs = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterNbInputs &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterNbInputs.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_nb_outputs");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterNbOutputs = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterNbOutputs &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterNbOutputs.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_flags");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterFlags &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_inputs_filter_pad");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGetInputsFilterPad = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetInputsFilterPad &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterGetInputsFilterPad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId,
                                                    InputFilterPadPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_outputs_filter_pad");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGetOutputsFilterPad = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetOutputsFilterPad &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterGetOutputsFilterPad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId,
                                                    OutputFilterPadPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t OutputFilterPadId = readUInt32(MemInst, OutputFilterPadPtr);
  ASSERT_TRUE(OutputFilterPadId > 0);

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_name_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadGetNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadGetNameLength &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterPadGetNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterPadId, 0},
        Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadGetName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadGetName &>(
      FuncInst->getHostFunc());

  {
    int32_t Idx = 0;
    EXPECT_TRUE(HostFuncAVFilterPadGetName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterPadId, Idx,
                                                    StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadGetType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadGetType &>(
      FuncInst->getHostFunc());

  {
    int32_t Idx = 0;
    EXPECT_TRUE(HostFuncAVFilterPadGetType.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterPadId, Idx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1); // Audio
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_drop");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadDrop =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadDrop &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVFilterPadDrop.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterPadId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
