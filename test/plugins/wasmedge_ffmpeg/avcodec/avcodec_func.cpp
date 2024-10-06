// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec/avcodec_func.h"
#include "avcodec/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// TODO: Commented functions need to be tested.

TEST_F(FFmpegTest, AVCodecFunc) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t CodecCtxPtr = UINT32_C(4);
  uint32_t CodecParamPtr = UINT32_C(8);
  uint32_t CodecParamPtr2 = UINT32_C(20);
  uint32_t CodecDecoderPtr = UINT32_C(12);
  uint32_t CodecEncoderPtr = UINT32_C(16);
  uint32_t StrPtr = UINT32_C(32);

  uint32_t CodecNamePtr = UINT32_C(150);
  std::string CodecName = "mpeg1video";
  spdlog::info("Filling memory CodecName into CodecNamePtr"sv);
  fillMemContent(MemInst, CodecNamePtr, CodecName);

  uint32_t ID = 1;

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecAllocContext3 = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecAllocContext3 &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AvCodecAllocContext3"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecAllocContext3.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{0, CodecCtxPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecCtxId = readUInt32(MemInst, CodecCtxPtr);
  ASSERT_TRUE(AVCodecCtxId > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersAlloc = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersAlloc &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecParametersAlloc"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecParametersAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecParamPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVCodecParametersAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecParamPtr2},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecParamId = readUInt32(MemInst, CodecParamPtr);
  ASSERT_TRUE(AVCodecParamId > 0);

  uint32_t AVCodecParamId2 = readUInt32(MemInst, CodecParamPtr2);
  ASSERT_TRUE(AVCodecParamId2 > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context"sv);
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersFromContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersFromContext &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecParametersFromContext"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecParametersFromContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId,
                                                    AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_get_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetType =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecGetType &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecGetType"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecGetType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ID}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0); // Video Type
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_decoder");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindDecoder = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindDecoder &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecFindDecoder"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecFindDecoder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ID, CodecDecoderPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecDecoderId = readUInt32(MemInst, CodecDecoderPtr);
  ASSERT_TRUE(AVCodecDecoderId > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindEncoder = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindEncoder &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecFindEncoder"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecFindEncoder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ID, CodecEncoderPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecEncoderId = readUInt32(MemInst, CodecEncoderPtr);
  ASSERT_TRUE(AVCodecEncoderId > 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_open2");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecOpen2 =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecOpen2 &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecOpen2"sv);
  // Invalid argument passed. Return -22 Error code. Means functionality
  // working.
  {
    EXPECT_TRUE(
        HostFuncAVCodecOpen2.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     AVCodecCtxId, AVCodecEncoderId, 0},
                                 Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_codec_is_encoder");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecIsEncoder =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecIsEncoder &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecIsEncoder"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecIsEncoder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecEncoderId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_codec_is_decoder");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecIsDecoder =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecIsDecoder &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecIsDecoder"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecIsDecoder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecDecoderId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_decoder_by_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindDecoderByName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindDecoderByName &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecFindDecoderByName"sv);
  {
    uint32_t Length = CodecName.length();
    EXPECT_TRUE(HostFuncAVCodecFindDecoderByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecDecoderPtr,
                                                    CodecNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindEncoderByName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindEncoderByName &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecFindEncoderByName"sv);
  {
    uint32_t Length = CodecName.length();
    EXPECT_TRUE(HostFuncAVCodecFindEncoderByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecEncoderPtr,
                                                    CodecNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersToContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersToContext &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecParametersToContext"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecParametersToContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  // TODO: Need FormatCtxId To test this func.
  //  FuncInst = AVCodecMod->findFuncExports(
  //      "wasmedge_ffmpeg_avcodec_avcodec_parameters_copy");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVCodecParametersCopy = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersCopy &>(
  //      FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecParametersCopy.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_version");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecVersion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecVersion &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecVersion"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    EXPECT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_configuration_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecConfigurationLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecConfigurationLength &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecConfigurationLength"sv);
  int32_t Length = 0;
  {
    EXPECT_TRUE(HostFuncAVCodecConfigurationLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_configuration");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecConfiguration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecConfiguration &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecConfiguration"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_license_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecLicenseLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecLicenseLength &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecLicenseLength"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_license");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecLicense =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecLicense &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecLicense"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_free_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFreeContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFreeContext &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecFreeContext"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_free");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersFree = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersFree &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecParametersFree"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecParametersFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, SendPacketReceiveFrame) {
  std::string FileName = "ffmpeg-assets/dummy.mp4"; // 32 chars
  uint32_t CodecCtxPtr = UINT32_C(64);
  uint32_t FramePtr = UINT32_C(72);
  uint32_t PacketPtr = UINT32_C(68);
  initFFmpegStructs(UINT32_C(20), UINT32_C(24), UINT32_C(28), FileName,
                    UINT32_C(60), CodecCtxPtr, PacketPtr, FramePtr);

  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  uint32_t CodecCtxId = readUInt32(MemInst, CodecCtxPtr);

  auto *FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_send_frame");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSendFrame =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSendFrame &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSendFrame"sv);
  // Invalid Argument Error. Should Use Encoder, I'm using decoder
  // Aim is to test the functionality.
  {
    EXPECT_TRUE(HostFuncAVCodecSendFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecCtxId, FrameId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }

  // Invalid Argument Error. Should Use Encoder, I'm using decoder
  // Aim is to test the functionality.
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_receive_packet");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecReceivePacket = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecReceivePacket &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecReceivePacket"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecReceivePacket.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecCtxId, PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_send_packet");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSendPacket = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSendPacket &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSendPacket"sv);
  // Send packet to Decoder.
  {
    EXPECT_TRUE(HostFuncAVCodecSendPacket.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecCtxId, PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_receive_frame");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  // Decoder Receives the Packet as Frame.
  auto &HostFuncAVCodecReceiveFrame = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecReceiveFrame &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecReceiveFrame"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecReceiveFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecCtxId, FrameId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_rescale_ts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketRescaleTs = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketRescaleTs &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVPacketRescaleTs"sv);
  {
    int32_t SrcNum = 2;
    int32_t SrcDen = 3;
    int32_t DestNum = 5;
    int32_t DestDen = 9;
    EXPECT_TRUE(HostFuncAVPacketRescaleTs.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, SrcNum, SrcDen,
                                                    DestNum, DestDen},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_make_writable");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketMakeWritable = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketMakeWritable &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVPacketMakeWritable"sv);
  {
    EXPECT_TRUE(HostFuncAVPacketMakeWritable.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_flush_buffers");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFlushBuffers = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFlushBuffers &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecFlushBuffers"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecFlushBuffers.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_close");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecClose =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecClose &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecClose"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
