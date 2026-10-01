// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avcodec/avcodec_func.h"
#include "avcodec/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// TODO: Commented functions need tests.

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecAllocContext3 = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersAlloc = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersFromContext = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetType = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecGetType"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecGetType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ID}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0); // Video Type
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_decoder");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindDecoder = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindEncoder = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecOpen2 = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecIsEncoder = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecIsEncoder"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecIsEncoder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecEncoderId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_codec_is_decoder");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecIsDecoder = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecIsDecoder"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecIsDecoder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecDecoderId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_decoder_by_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindDecoderByName = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecFindDecoderByName"sv);
  {
    uint32_t Length = CodecName.length();
    EXPECT_TRUE(HostFuncAVCodecFindDecoderByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecDecoderPtr,
                                                    CodecNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t DecoderByNameId = readUInt32(MemInst, CodecDecoderPtr);
    EXPECT_TRUE(HostFuncAVCodecIsDecoder.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{DecoderByNameId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFindEncoderByName = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecFindEncoderByName"sv);
  {
    uint32_t Length = CodecName.length();
    EXPECT_TRUE(HostFuncAVCodecFindEncoderByName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{CodecEncoderPtr,
                                                    CodecNamePtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t EncoderByNameId = readUInt32(MemInst, CodecEncoderPtr);
    EXPECT_TRUE(HostFuncAVCodecIsEncoder.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{EncoderByNameId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersToContext = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecParametersToContext"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecParametersToContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_version");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecVersion = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecVersion"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    EXPECT_TRUE((Result[0].get<int32_t>() >> 16) > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_configuration_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecConfigurationLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecConfiguration = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecConfiguration"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_NE(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_license_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecLicenseLength = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecLicenseLength"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_license");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecLicense = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecLicense"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_free_context");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFreeContext = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecFreeContext"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_free");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersFree = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSendFrame = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecReceivePacket = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSendPacket = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  // Decoder Receives the Packet as Frame.
  auto &HostFuncAVCodecReceiveFrame = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketRescaleTs = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketMakeWritable = FuncInst->getHostFunc();

  spdlog::info("Testing AVPacketMakeWritable"sv);
  {
    EXPECT_TRUE(HostFuncAVPacketMakeWritable.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_flush_buffers");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecFlushBuffers = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecFlushBuffers"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecFlushBuffers.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_close");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecClose = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecClose"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVCodecFailuresZeroTheIdSlot) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVCodecMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  uint32_t SlotPtr = UINT32_C(4);
  uint32_t CtxPtr = UINT32_C(8);
  uint32_t const Unresolved = UINT32_C(0xDEAD);

  // A failed call leaves the null handle in its result slot, not whatever
  // the guest's memory held.
  writeUInt32(MemInst, UINT32_C(0xABCD), SlotPtr);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {Unresolved, SlotPtr}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SlotPtr), UINT32_C(0));
  writeUInt32(MemInst, UINT32_C(0xABCD), SlotPtr);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodeccontext_codec",
                {Unresolved, SlotPtr}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SlotPtr), UINT32_C(0));
  for (const char *Lookup :
       {"wasmedge_ffmpeg_avcodec_avcodec_find_decoder_by_name",
        "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name"}) {
    writeUInt32(MemInst, UINT32_C(0xABCD), SlotPtr);
    EXPECT_EQ(Run(Lookup, {SlotPtr, UINT32_C(0xFFFFFFF0), UINT32_C(64)}),
              static_cast<int32_t>(ErrNo::MissingMemory))
        << Lookup;
    EXPECT_EQ(readUInt32(MemInst, SlotPtr), UINT32_C(0)) << Lookup;
  }

  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {UINT32_C(0), CtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  writeUInt32(MemInst, UINT32_C(0xABCD), SlotPtr);
  EXPECT_LT(
      Run("wasmedge_ffmpeg_avcodec_avcodeccontext_codec", {CtxId, SlotPtr}), 0);
  EXPECT_EQ(readUInt32(MemInst, SlotPtr), UINT32_C(0));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecSendRejectsUnresolvedHandles) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t CodecCtxPtr = UINT32_C(4);
  auto *AllocInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
  ASSERT_NE(AllocInst, nullptr);
  auto &HostFuncAllocCtx = AllocInst->getHostFunc();
  EXPECT_TRUE(HostFuncAllocCtx.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), CodecCtxPtr},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecCtxId = readUInt32(MemInst, CodecCtxPtr);
  ASSERT_TRUE(CodecCtxId > 0);

  uint32_t UnresolvedId = UINT32_C(999999);

  auto *SendPacketInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_send_packet");
  ASSERT_NE(SendPacketInst, nullptr);
  auto &HostFuncSendPacket = SendPacketInst->getHostFunc();
  EXPECT_TRUE(HostFuncSendPacket.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecCtxId, UnresolvedId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  auto *SendFrameInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_send_frame");
  ASSERT_NE(SendFrameInst, nullptr);
  auto &HostFuncSendFrame = SendFrameInst->getHostFunc();
  EXPECT_TRUE(HostFuncSendFrame.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecCtxId, UnresolvedId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  auto *FreeInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_free_context");
  ASSERT_NE(FreeInst, nullptr);
  auto &HostFuncFreeCtx = FreeInst->getHostFunc();
  EXPECT_TRUE(HostFuncFreeCtx.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecCtxId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecSendFrameRejectsMismatchedFrame) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  uint32_t CodecPtr = UINT32_C(4);
  uint32_t CtxPtr = UINT32_C(8);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
                {UINT32_C(216), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, CtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format",
                {CtxId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_rate",
                {CtxId, INT32_C(48000)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channel_layout",
                {CtxId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));
  ASSERT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_open2",
                {CtxId, CodecId, UINT32_C(0)}),
            0);

  auto MakeAudioFrame = [&](uint32_t Ptr, uint32_t SampleFormatId,
                            uint64_t ChannelLayoutId) {
    initEmptyFrame(Ptr);
    uint32_t Id = readUInt32(MemInst, Ptr);
    EXPECT_TRUE(Id > 0);
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
                  {Id, SampleFormatId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                  {Id, ChannelLayoutId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                  {Id, INT32_C(64)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                  {Id, INT32_C(0)}),
              0);
    return Id;
  };

  uint32_t U8StereoId = MakeAudioFrame(UINT32_C(12), UINT32_C(1), UINT64_C(3));
  uint32_t S16MonoId = MakeAudioFrame(UINT32_C(16), UINT32_C(2), UINT64_C(1));
  uint32_t S16StereoId = MakeAudioFrame(UINT32_C(20), UINT32_C(2), UINT64_C(3));

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_send_frame",
                {CtxId, U8StereoId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_send_frame",
                {CtxId, S16MonoId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_send_frame",
                {CtxId, S16StereoId}),
            0);

  for (uint32_t Id : {U8StereoId, S16MonoId, S16StereoId}) {
    EXPECT_EQ(
        Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {Id}),
        static_cast<int32_t>(ErrNo::Success));
  }
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecSendFrameRejectsFrameWithoutBuffers) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  uint32_t CodecPtr = UINT32_C(4);
  uint32_t CtxPtr = UINT32_C(8);
  uint32_t FramePtr = UINT32_C(12);
  // mp2 has a fixed frame size and pads a short last frame by copying its
  // samples, so a frame without buffers reaches a memcpy from NULL.
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
                {UINT32_C(299), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, CtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format",
                {CtxId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_rate",
                {CtxId, INT32_C(48000)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channel_layout",
                {CtxId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));
  ASSERT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_open2",
                {CtxId, CodecId, UINT32_C(0)}),
            0);

  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run(AVUtilMod.get(),
                "wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
                {FrameId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVUtilMod.get(),
                "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                {FrameId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVUtilMod.get(),
                "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(100)}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_send_frame",
                {CtxId, FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                {FrameId, INT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_send_frame",
                {CtxId, FrameId}),
            0);

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecParametersToContextAcceptsOpenedParams) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  // The SDK opens a decoder from a stream's parameters and then re-applies
  // the same parameters to the open context (examples/transcode-audio.rs).
  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t ParPtr = UINT32_C(8);
  uint32_t CtxPtr = UINT32_C(12);
  uint32_t CodecPtr = UINT32_C(16);
  uint32_t PacketPtr = UINT32_C(20);
  uint32_t FramePtr = UINT32_C(24);
  uint32_t FilePtr = UINT32_C(100);
  initFormatCtx(FormatCtxPtr, FilePtr, std::string("ffmpeg-assets/dummy.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_find_stream_info",
                {FormatCtxId, UINT32_C(0)}),
            0);
  int32_t const StreamIdx =
      Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_av_find_best_stream",
          {FormatCtxId, INT32_C(0), INT32_C(-1), INT32_C(-1), UINT32_C(0),
           INT32_C(0)});
  ASSERT_GE(StreamIdx, 0);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_codecpar",
                {FormatCtxId, static_cast<uint32_t>(StreamIdx), ParPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t ParId = readUInt32(MemInst, ParPtr);
  ASSERT_TRUE(ParId > 0);

  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {UINT32_C(0), CtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context",
                {CtxId, ParId}),
            0);
  uint32_t const CodecIdx = static_cast<uint32_t>(
      Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id",
          {CtxId}));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
                {CodecIdx, CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  ASSERT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_open2",
                {CtxId, CodecId, UINT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context",
                {CtxId, ParId}),
            static_cast<int32_t>(ErrNo::Success));

  allocPacket(PacketPtr);
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);
  bool SentPacket = false;
  for (int Attempt = 0; Attempt < 64 && !SentPacket; ++Attempt) {
    if (Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_av_read_frame",
            {FormatCtxId, PacketId}) < 0) {
      break;
    }
    if (Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_stream_index",
            {PacketId}) != StreamIdx) {
      continue;
    }
    EXPECT_EQ(Run(AVCodecMod.get(),
                  "wasmedge_ffmpeg_avcodec_avcodec_send_packet",
                  {CtxId, PacketId}),
              0);
    SentPacket = true;
  }
  EXPECT_TRUE(SentPacket);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  int32_t const ReceiveRes =
      Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_receive_frame",
          {CtxId, FrameId});
  EXPECT_TRUE(ReceiveRes == 0 || ReceiveRes == -EAGAIN);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context",
                {CtxId, ParId}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {PacketId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {FormatCtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecFlushBuffersRequiresOpenContext) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVCodecMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  uint32_t CodecPtr = UINT32_C(4);
  uint32_t CtxPtr = UINT32_C(8);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
                {UINT32_C(27), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3", {CodecId, CtxPtr}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_flush_buffers", {CtxId}),
            static_cast<int32_t>(ErrNo::InternalError));
  ASSERT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_open2",
                {CtxId, CodecId, UINT32_C(0)}),
            0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_flush_buffers", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecLookupsHideWrappedAvframe) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVCodecMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t CodecPtr = UINT32_C(4);
  uint32_t NamePtr = UINT32_C(100);
  std::string Name("wrapped_avframe");
  fillMemContent(MemInst, NamePtr, Name);
  uint32_t const NameLen = static_cast<uint32_t>(Name.length());
  // 420 is the binding id of AV_CODEC_ID_WRAPPED_AVFRAME.
  uint32_t const WrappedId = UINT32_C(420);

  writeUInt32(MemInst, UINT32_C(99), CodecPtr);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
                {WrappedId, CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readUInt32(MemInst, CodecPtr), UINT32_C(0));
  writeUInt32(MemInst, UINT32_C(99), CodecPtr);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
                {WrappedId, CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readUInt32(MemInst, CodecPtr), UINT32_C(0));
  writeUInt32(MemInst, UINT32_C(99), CodecPtr);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_find_decoder_by_name",
                {CodecPtr, NamePtr, NameLen}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readUInt32(MemInst, CodecPtr), UINT32_C(0));
  writeUInt32(MemInst, UINT32_C(99), CodecPtr);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name",
                {CodecPtr, NamePtr, NameLen}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readUInt32(MemInst, CodecPtr), UINT32_C(0));

  // Ordinary codecs are still resolved.
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
                {UINT32_C(27), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_TRUE(readUInt32(MemInst, CodecPtr) > 0);
}

TEST_F(FFmpegTest, AVCodecReceiveFrameRequiresOpenContext) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t CtxPtr = UINT32_C(4);
  uint32_t CodecPtr = UINT32_C(8);
  uint32_t FramePtr = UINT32_C(12);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  // A context that was only allocated has no internal state for
  // avcodec_receive_frame to consult (the SDK's Context::new produces one).
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {UINT32_C(0), CtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_receive_frame",
                {CtxId, FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_decoder",
                {UINT32_C(27), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, CtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_receive_frame",
                {CtxId, FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));
  ASSERT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_avcodec_open2",
                {CtxId, CodecId, UINT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_receive_frame",
                {CtxId, FrameId}),
            -EAGAIN);

  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVCodecParametersWritesRejectLockedMuxer) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t CodecPtr = UINT32_C(4);
  uint32_t EncCtxPtr = UINT32_C(8);
  uint32_t CtxPtr = UINT32_C(12);
  uint32_t ParPtr = UINT32_C(16);
  uint32_t SrcParPtr = UINT32_C(20);
  uint32_t FormatPtr = UINT32_C(100);
  uint32_t FilePtr = UINT32_C(200);
  std::string Format("matroska");
  std::string File("params_lock.mkv");
  fillMemContent(MemInst, FormatPtr, Format);
  fillMemContent(MemInst, FilePtr, File);

  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
                {UINT32_C(216), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, EncCtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t EncCtxId = readUInt32(MemInst, EncCtxPtr);
  ASSERT_TRUE(EncCtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format",
                {EncCtxId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_rate",
                {EncCtxId, INT32_C(48000)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channel_layout",
                {EncCtxId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc",
                {SrcParPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t SrcParId = readUInt32(MemInst, SrcParPtr);
  ASSERT_TRUE(SrcParId > 0);
  EXPECT_GE(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
                {SrcParId, EncCtxId}),
            0);

  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
                {CtxPtr, UINT32_C(0), FormatPtr,
                 static_cast<uint32_t>(Format.length()), FilePtr,
                 static_cast<uint32_t>(File.length())}),
            0);
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_new_stream",
                {CtxId, UINT32_C(0)}),
            1);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_codecpar",
                {CtxId, UINT32_C(0), ParPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t ParId = readUInt32(MemInst, ParPtr);
  ASSERT_TRUE(ParId > 0);
  // Both ways of writing a stream's parameters are open until the header
  // is written.
  EXPECT_GE(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
                {ParId, EncCtxId}),
            0);
  EXPECT_GE(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_copy",
                {CtxId, SrcParId, UINT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodecparam_set_codec_tag",
                {ParId, UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avStream_set_timebase",
                {UINT32_C(1), UINT32_C(48000), CtxId, UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_GE(
      Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avio_open",
          {CtxId, FilePtr, static_cast<uint32_t>(File.length()), INT32_C(2)}),
      0);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_write_header",
                {CtxId, UINT32_C(0)}),
            0);

  // The muxer keeps pointers into codecpar from the header on, so none of
  // the writes may change them now.
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
                {ParId, EncCtxId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_copy",
                {CtxId, SrcParId, UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodecparam_set_codec_tag",
                {ParId, UINT32_C(0x74736574)}),
            static_cast<int32_t>(ErrNo::InternalError));
  // A standalone parameter set is not owned by any muxer and stays
  // writable.
  EXPECT_GE(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
                {SrcParId, EncCtxId}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodecparam_set_codec_tag",
                {SrcParId, UINT32_C(0x74736574)}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_write_trailer", {CtxId}),
            0);
  EXPECT_EQ(
      Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avio_close", {CtxId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_free", {SrcParId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {EncCtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
