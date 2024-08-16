// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/module.h"
#include "avutil/pixfmt.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVPixFmt) {
  uint32_t NamePtr = UINT32_C(4);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avpixfmtdescriptor_nb_components");
  auto &HostFuncAVPixFmtDescriptorNbComponents = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AvPixFmtDescriptorNbComponents &>(
      FuncInst->getHostFunc());

  uint32_t PixFmtId = 3; // RGB24

  {
    HostFuncAVPixFmtDescriptorNbComponents.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PixFmtId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), PixFmtId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromaw");
  auto &HostFuncAvPixFmtDescriptorLog2ChromaW = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AvPixFmtDescriptorLog2ChromaW &>(
      FuncInst->getHostFunc());

  {
    HostFuncAvPixFmtDescriptorLog2ChromaW.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromah");
  auto &HostFuncAvPixFmtDescriptorLog2ChromaH = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AvPixFmtDescriptorLog2ChromaH &>(
      FuncInst->getHostFunc());

  {
    HostFuncAvPixFmtDescriptorLog2ChromaH.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PixFmtId},
        Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  int32_t Length = 0;
  int32_t TransferCharacteristicId = 6; // (SMPTE170M)
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_color_transfer_name_length");
  auto &HostFuncAVColorTransferNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorTransferNameLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVColorTransferNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{TransferCharacteristicId},
        Result);
    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill memory with zero.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_color_transfer_name");
  auto &HostFuncAVColorTransferName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorTransferName &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVColorTransferName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{TransferCharacteristicId,
                                                    NamePtr, Length},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  int32_t ColorRangeId = 2; //; JPEG
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_color_range_name_length");
  auto &HostFuncAVColorRangeNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorRangeNameLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVColorRangeNameLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ColorRangeId},
        Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill memory with zero.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_color_range_name");
  auto &HostFuncAVColorRangeName =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorRangeName &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVColorRangeName.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     ColorRangeId, NamePtr, Length},
                                 Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  int32_t ColorSpaceId = 1; // BT709
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_color_space_name_length");
  auto &HostFuncAVColorSpaceNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorSpaceNameLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVColorSpaceNameLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ColorSpaceId},
        Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill memory with zero.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_color_space_name");
  auto &HostFuncAVColorSpaceName =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorSpaceName &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVColorSpaceName.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     ColorSpaceId, NamePtr, Length},
                                 Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  int32_t ColorPrimariesId = 1; // BT709
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_color_primaries_name_length");
  auto &HostFuncAVColorPrimariesNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorPrimariesNameLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVColorPrimariesNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ColorPrimariesId}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill memory with zero.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_color_primaries_name");
  auto &HostFuncAVColorPrimariesName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVColorPrimariesName &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVColorPrimariesName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ColorPrimariesId, NamePtr,
                                                    Length},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  PixFmtId = 1; // YUV420P
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_pix_format_name_length");
  auto &HostFuncAVPixFormatNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVPixelFormatNameLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVPixFormatNameLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PixFmtId},
        Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill memory with zero.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_pix_format_name");
  auto &HostFuncAVPixFormatName =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVPixelFormatName &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVPixFormatName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PixFmtId, NamePtr, Length},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_pix_format_mask");
  auto &HostFuncAVPixFormatMask =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVPixelFormatMask &>(
          FuncInst->getHostFunc());

  {
    uint32_t PixId = 3; //  AV_PIX_FMT_RGB24:
    HostFuncAVPixFormatMask.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PixId}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(),
              2); // Verify Mask. Position of Pix in Enum.
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
