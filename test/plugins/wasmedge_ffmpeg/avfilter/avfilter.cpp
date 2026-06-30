// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avfilter/avFilter.h"
#include "avfilter//avfilter_func.h"
#include "avfilter/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVFilterStructs) {
  using namespace std::literals::string_view_literals;
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
  }

  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  ASSERT_TRUE(FilterId > 0);
  // ==================================================================
  //               End Initialize AVFilter
  // ==================================================================

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_name_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterNameLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterName = FuncInst->getHostFunc();

  fillMemContent(MemInst, StrPtr, Length);
  {
    EXPECT_TRUE(HostFuncAVFilterName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length)),
              "abuffer"sv);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_description_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterDescriptionLength = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterDescriptionLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_description");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterDescription = FuncInst->getHostFunc();

  fillMemContent(MemInst, StrPtr, Length);
  {
    EXPECT_TRUE(HostFuncAVFilterDescription.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length)),
              "Buffer audio frames, and make them accessible to the "
              "filterchain."sv);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_nb_inputs");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterNbInputs = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterNbInputs.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_nb_outputs");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterNbOutputs = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterNbOutputs.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_flags");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterFlags = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFilterFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_inputs_filter_pad");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGetInputsFilterPad = FuncInst->getHostFunc();

  {
    writeUInt32(MemInst, UINT32_C(0), InputFilterPadPtr);
    EXPECT_TRUE(HostFuncAVFilterGetInputsFilterPad.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId,
                                                    InputFilterPadPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, InputFilterPadPtr), UINT32_C(0));
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_outputs_filter_pad");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterGetOutputsFilterPad = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadGetNameLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadGetName = FuncInst->getHostFunc();

  {
    int32_t Idx = 0;
    EXPECT_TRUE(HostFuncAVFilterPadGetName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OutputFilterPadId, Idx,
                                                    StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length)),
              "default"sv);
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadGetType = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVFilterPadDrop = FuncInst->getHostFunc();

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
