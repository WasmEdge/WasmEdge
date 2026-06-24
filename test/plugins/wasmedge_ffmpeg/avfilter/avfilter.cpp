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

TEST_F(FFmpegTest, AVFilterNameBounds) {
  ASSERT_TRUE(AVFilterMod != nullptr);

  std::string InputName = std::string("abuffer");
  uint32_t FilterPtr = UINT32_C(8);
  uint32_t InputNamePtr = UINT32_C(100);
  uint32_t StrPtr = UINT32_C(150);
  fillMemContent(MemInst, InputNamePtr, InputName);

  auto *FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_by_name");
  auto &HostFuncAVFilterGetByName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetByName &>(
      FuncInst->getHostFunc());
  HostFuncAVFilterGetByName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          FilterPtr, InputNamePtr, static_cast<int32_t>(InputName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  ASSERT_TRUE(FilterId > 0);

  // The guest buffer is larger than the host string and fenced with 0xAA; the
  // host must copy only the string plus its terminator and never read past the
  // host string into the rest of the guest buffer.
  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_name_length");
  auto &HostFuncNameLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterNameLength &>(
      FuncInst->getHostFunc());
  HostFuncNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId}, Result);
  uint32_t NameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(NameLen > 0);

  FuncInst =
      AVFilterMod->findFuncExports("wasmedge_ffmpeg_avfilter_avfilter_name");
  auto &HostFuncName =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterName &>(
          FuncInst->getHostFunc());
  uint32_t NameBufLen = NameLen + UINT32_C(32);
  fillMemContent(MemInst, StrPtr, NameBufLen, UINT8_C(0xAA));
  HostFuncName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FilterId, StrPtr, NameBufLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  {
    char *Buf = MemInst->getPointer<char *>(StrPtr);
    for (uint32_t I = NameLen + 1; I < NameBufLen; ++I) {
      EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
    }
  }

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_description_length");
  auto &HostFuncDescLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterDescriptionLength &>(
      FuncInst->getHostFunc());
  HostFuncDescLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId}, Result);
  uint32_t DescLen = Result[0].get<int32_t>();
  ASSERT_TRUE(DescLen > 0);

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_description");
  auto &HostFuncDesc = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterDescription &>(
      FuncInst->getHostFunc());
  uint32_t DescBufLen = DescLen + UINT32_C(32);
  fillMemContent(MemInst, StrPtr, DescBufLen, UINT8_C(0xAA));
  HostFuncDesc.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FilterId, StrPtr, DescBufLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  {
    char *Buf = MemInst->getPointer<char *>(StrPtr);
    for (uint32_t I = DescLen + 1; I < DescBufLen; ++I) {
      EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
    }
  }
}

TEST_F(FFmpegTest, AVFilterGetByNameBounds) {
  ASSERT_TRUE(AVFilterMod != nullptr);

  auto *FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_by_name");
  auto &HostFuncAVFilterGetByName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetByName &>(
      FuncInst->getHostFunc());

  // The string pointer is in bounds but the guest-declared length runs off the
  // end of linear memory; the host must reject it, not read past the page.
  uint32_t FilterPtr = UINT32_C(8);
  uint32_t OutOfBoundsStrPtr = UINT32_C(65000);
  uint32_t OutOfBoundsStrLen = UINT32_C(2000);
  HostFuncAVFilterGetByName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FilterPtr, OutOfBoundsStrPtr,
                                                  OutOfBoundsStrLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

TEST_F(FFmpegTest, AVFilterPadGetterIndexBounds) {
  ASSERT_TRUE(AVFilterMod != nullptr);

  uint32_t FilterPtr = UINT32_C(8);
  uint32_t PadPtr = UINT32_C(16);
  uint32_t NamePtr = UINT32_C(100);
  uint32_t StrPtr = UINT32_C(200);

  std::string FilterName = std::string("abuffer");
  fillMemContent(MemInst, NamePtr, FilterName);

  auto *FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_by_name");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncGetByName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetByName &>(
      FuncInst->getHostFunc());
  HostFuncGetByName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          FilterPtr, NamePtr, static_cast<int32_t>(FilterName.length())},
      Result);
  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  ASSERT_TRUE(FilterId > 0);

  FuncInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_get_outputs_filter_pad");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncGetOutputs = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterGetOutputsFilterPad &>(
      FuncInst->getHostFunc());
  HostFuncGetOutputs.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId, PadPtr},
      Result);
  uint32_t PadId = readUInt32(MemInst, PadPtr);
  ASSERT_TRUE(PadId > 0);

  auto *LenInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_name_length");
  ASSERT_NE(LenInst, nullptr);
  auto &HostFuncLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadGetNameLength &>(
      LenInst->getHostFunc());
  auto *NameInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_name");
  ASSERT_NE(NameInst, nullptr);
  auto &HostFuncName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadGetName &>(
      NameInst->getHostFunc());
  auto *TypeInst = AVFilterMod->findFuncExports(
      "wasmedge_ffmpeg_avfilter_avfilter_pad_get_type");
  ASSERT_NE(TypeInst, nullptr);
  auto &HostFuncType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::AVFilterPadGetType &>(
      TypeInst->getHostFunc());

  // A valid index still resolves the pad (abuffer has one output pad).
  HostFuncLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PadId, INT32_C(0)},
      Result);
  EXPECT_GT(Result[0].get<int32_t>(), 0);

  // An out-of-range or negative index must report "absent" instead of reading
  // past the pad array; avfilter_pad_get_name/type do not bounds-check Idx.
  HostFuncLen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PadId, INT32_C(0x7FFFFFFF)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 0);
  HostFuncLen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PadId, INT32_C(-1)}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 0);

  uint32_t BufLen = UINT32_C(64);
  fillMemContent(MemInst, StrPtr, BufLen, UINT8_C(0xAA));
  HostFuncName.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       PadId, INT32_C(0x7FFFFFFF), StrPtr, BufLen},
                   Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  HostFuncType.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PadId, INT32_C(0x7FFFFFFF)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), -1); // AVMEDIA_TYPE_UNKNOWN
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
