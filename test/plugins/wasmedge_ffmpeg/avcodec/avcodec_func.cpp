#include "common/types.h"
#include "runtime/instance/module.h"

#include "avcodec/avcodec_func.h"
#include "avcodec/module.h"

#include "../utils.h"
#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVCodecFunc) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  uint32_t AVCodecCtxPtr = UINT32_C(4);
  uint32_t AVCodecParamPtr = UINT32_C(8);
  uint32_t AVCodecPtr = UINT32_C(12);
  //  uint32_t AVDictPtr = UINT32_C(16);
  //  uint32_t KeyPtr = UINT32_C(20);
  //  uint32_t ValuePtr = UINT32_C(24);
  //  uint32_t FramePtr = UINT32_C(28);
  uint32_t StrPtr = UINT32_C(32);

  //  initDict(AVDictPtr, KeyPtr, std::string("KEY"), ValuePtr,
  //           std::string("VALUE"));

  //  initAVCodec(UINT32_C(32), UINT32_C(24), UINT32_C(28), FileName,
  //  UINT32_C(60),
  //              UINT32_C(64));

  //  uint32_t DictId = readUInt32(MemInst, AVDictPtr);
  //  uint32_t EmptyDictId = UINT32_C(0);
  uint32_t ID = 1;
  //  uint32_t FrameId = readUInt32(MemInst, FramePtr);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecAllocContext3 = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecAllocContext3 &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecAllocContext3.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{0, AVCodecCtxPtr}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecCtxId = readUInt32(MemInst, AVCodecCtxPtr);
  ASSERT_TRUE(AVCodecCtxId > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersAlloc = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersAlloc &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecParametersAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecParamId = readUInt32(MemInst, AVCodecParamPtr);
  ASSERT_TRUE(AVCodecParamId > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersFromContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersFromContext &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecParametersFromContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    AVCodecParamId},
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

  {
    EXPECT_TRUE(HostFuncAVCodecFindDecoder.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ID, AVCodecPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t AVCodecId = readUInt32(MemInst, AVCodecPtr);
  ASSERT_TRUE(AVCodecId > 0);

  //  FuncInst =
  //      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_open2");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVCodecOpen2 =
  //      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecOpen2 &>(
  //          FuncInst->getHostFunc());
  //
  //  {
  //    EXPECT_TRUE(
  //        HostFuncAVCodecOpen2.run(CallFrame,
  //                                 std::initializer_list<WasmEdge::ValVariant>{
  //                                     AVCodecCtxId, AVCodecId, DictId},
  //                                 Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //
  //    EXPECT_TRUE(
  //        HostFuncAVCodecOpen2.run(CallFrame,
  //                                 std::initializer_list<WasmEdge::ValVariant>{
  //                                     AVCodecCtxId, AVCodecId, EmptyDictId},
  //                                 Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_codec_is_encoder");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecIsEncoder =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecIsEncoder &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecIsEncoder.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_codec_is_decoder");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecIsDecoder =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecIsDecoder &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecIsDecoder.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  //  FuncInst = AVCodecMod->findFuncExports(
  //      "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVCodecParametersToContext = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersToContext
  //      &>( FuncInst->getHostFunc());
  //
  //  {
  //    EXPECT_TRUE(HostFuncAVCodecParametersToContext.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
  //                                                    AVCodecParamId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  //  }

  //  FuncInst = AVCodecMod->findFuncExports(
  //      "wasmedge_ffmpeg_avcodec_avcodec_receive_frame");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVCodecReceiveFrame = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecReceiveFrame &>(
  //      FuncInst->getHostFunc());
  //
  //  {
  //    EXPECT_TRUE(HostFuncAVCodecReceiveFrame.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, FrameId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_send_packet");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecSendPacket = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSendPacket &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecSendPacket.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_find_encoder");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecFindEncoder = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindEncoder &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecFindEncoder.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_receive_packet");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecReceivePacket = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecReceivePacket &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecReceivePacket.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst =
  //       AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_send_frame");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecSendFrame =
  //       dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSendFrame
  //       &>(
  //           FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecSendFrame.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_find_decoder_by_name");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecFindDecoderByName = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindDecoderByName &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecFindDecoderByName.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecFindEncoderByName = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindEncoderByName &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecFindEncoderByName.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_av_packet_rescale_ts");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVPacketRescaleTs = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketRescaleTs &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVPacketRescaleTs.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }

  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_av_packet_make_writable");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVPacketMakeWritable = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketMakeWritable &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVPacketMakeWritable.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_parameters_copy");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecParametersCopy = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersCopy &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecParametersCopy.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //   FuncInst = AVCodecMod->findFuncExports(
  //       "wasmedge_ffmpeg_avcodec_avcodec_flush_buffers");
  //   EXPECT_NE(FuncInst, nullptr);
  //   EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //   auto &HostFuncAVCodecFlushBuffers = dynamic_cast<
  //       WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFlushBuffers &>(
  //       FuncInst->getHostFunc());
  //
  //   {
  //     EXPECT_TRUE(HostFuncAVCodecFlushBuffers.run(
  //         CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //     EXPECT_EQ(Result[0].get<int32_t>(),
  //     static_cast<int32_t>(ErrNo::Success));
  //   }
  //
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_version");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecVersion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecVersion &>(
          FuncInst->getHostFunc());

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

  int32_t Length;
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

  {
    EXPECT_TRUE(HostFuncAVCodecFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  //  FuncInst =
  //      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_close");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //
  //  auto &HostFuncAVCodecClose =
  //      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecClose &>(
  //          FuncInst->getHostFunc());
  //
  //  {
  //    EXPECT_TRUE(HostFuncAVCodecClose.run(
  //        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_free");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParametersFree = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersFree &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecParametersFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
