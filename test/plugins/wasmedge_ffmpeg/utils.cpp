// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "utils.h"

#include "avcodec/avCodecContext.h"
#include "avcodec/avPacket.h"
#include "avcodec/avcodec_func.h"
#include "avformat/avStream.h"
#include "avformat/avformat_func.h"
#include "avutil/avDictionary.h"
#include "avutil/avFrame.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

void FFmpegTest::initEmptyFrame(uint32_t FramePtr) {
  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
}

void FFmpegTest::initFFmpegStructs(uint32_t AVCodecPtr, uint32_t AVFormatCtxPtr,
                                   uint32_t FilePtr, std::string FileName,
                                   uint32_t CodecParameterPtr,
                                   uint32_t AVCodecCtxPtr, uint32_t PacketPtr,
                                   uint32_t FramePtr) {
  initFormatCtx(AVFormatCtxPtr, FilePtr, FileName);

  uint32_t AvFormatCtxId = readUInt32(MemInst, AVFormatCtxPtr);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_av_find_best_stream");
  auto &HostFuncAVFindBestStream = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFindBestStream &>(
      FuncInst->getHostFunc());
  uint32_t MediaTypeId = 0; // Video
  uint32_t WantedStream = -1;
  uint32_t RelatedStream = -1;
  uint32_t DecoderRetId = 0;
  uint32_t Flags = 0;
  HostFuncAVFindBestStream.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   AvFormatCtxId, MediaTypeId, WantedStream,
                                   RelatedStream, DecoderRetId, Flags},
                               Result);

  uint32_t StreamIdx = Result[0].get<int32_t>();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_codecpar");

  auto &HostFuncAVStreamCodecpar = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamCodecPar &>(
      FuncInst->getHostFunc());

  HostFuncAVStreamCodecpar.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   AvFormatCtxId, StreamIdx, CodecParameterPtr},
                               Result);

  uint32_t CodecParametersId = readUInt32(MemInst, CodecParameterPtr);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
  auto &HostFuncAVCodecAllocContext3 = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecAllocContext3 &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecAllocContext3.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{0, AVCodecCtxPtr},
      Result);

  uint32_t AVCodecCtxId = readUInt32(MemInst, AVCodecCtxPtr);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context");
  auto &HostFuncAVCodecParametersToContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersToContext &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecParametersToContext.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                  CodecParametersId},
      Result);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id");
  auto &HostFuncAVCodecContextCodecId = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxCodecID &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecContextCodecId.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
      Result);

  uint32_t CodecId = Result[0].get<int32_t>();

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_decoder");
  auto &HostFuncAVCodecFindDecoder = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindDecoder &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecFindDecoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, AVCodecPtr}, Result);

  uint32_t AVCodecId = readUInt32(MemInst, AVCodecPtr);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_open2");
  auto &HostFuncAVCodecOpen2 =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecOpen2 &>(
          FuncInst->getHostFunc());

  HostFuncAVCodecOpen2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, AVCodecId, 0},
      Result);

  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_receive_frame");
  auto &HostFuncAVCodecReceiveFrame = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecReceiveFrame &>(
      FuncInst->getHostFunc());

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_frame");
  auto &HostFuncAVReadFrame =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadFrame &>(
          FuncInst->getHostFunc());

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_send_packet");
  auto &HostFuncAVCodecSendPacket = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSendPacket &>(
      FuncInst->getHostFunc());

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_stream_index");
  auto &HostFuncAVPacketStreamIndex = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketStreamIndex &>(
      FuncInst->getHostFunc());

  while (true) {
    HostFuncAVCodecReceiveFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, FrameId},
        Result);

    // Error returned by FFmpeg are negative.
    int32_t Error = Result[0].get<int32_t>() * (-1);

    if (Error == EAGAIN) {
      while (true) {
        allocPacket(PacketPtr);

        uint32_t PackedId = readUInt32(MemInst, PacketPtr);

        while (true) {
          HostFuncAVReadFrame.run(CallFrame,
                                  std::initializer_list<WasmEdge::ValVariant>{
                                      AvFormatCtxId, PackedId},
                                  Result);

          int32_t Res = Result[0].get<int32_t>();
          if (Res == 0 || Res == AVERROR_EOF) {
            break;
          }
        }

        HostFuncAVPacketStreamIndex.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, FrameId},
            Result);

        uint32_t PacketStreamIdx = Result[0].get<int32_t>();

        if (PacketStreamIdx != StreamIdx) {
          continue;
        }

        HostFuncAVCodecSendPacket.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, PackedId},
            Result);
        break;
      }
    } else {
      break;
    }
  }
}

void FFmpegTest::initFormatCtx(uint32_t AVFormatCtxPtr, uint32_t FilePtr,
                               std::string FileName) {
  int32_t Length = FileName.length();
  fillMemContent(MemInst, FilePtr, Length);
  fillMemContent(MemInst, FilePtr, FileName);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_open_input");
  auto &HostFuncAVFormatOpenInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatOpenInput &>(
      FuncInst->getHostFunc());
  HostFuncAVFormatOpenInput.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          AVFormatCtxPtr, FilePtr, Length, UINT32_C(0), UINT32_C(0)},
      Result);
}

void FFmpegTest::initDict(uint32_t DictPtr, uint32_t KeyPtr, std::string Key,
                          uint32_t ValuePtr, std::string Value) {
  uint32_t KeyLen = Key.length();
  uint32_t ValueLen = Value.length();
  fillMemContent(MemInst, KeyPtr, KeyLen + ValueLen);
  fillMemContent(MemInst, KeyPtr, Key);
  fillMemContent(MemInst, ValuePtr, Value);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  HostFuncAVDictSet.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            DictPtr, KeyPtr, KeyLen, ValuePtr, ValueLen, 0},
                        Result);
}

void FFmpegTest::allocPacket(uint32_t PacketPtr) {
  auto *FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc");
  auto &HostFuncAVPacketAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketAlloc &>(
          FuncInst->getHostFunc());

  HostFuncAVPacketAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr},
      Result);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
