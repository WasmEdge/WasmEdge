// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec_func.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<int32_t> AVCodecAllocContext3::body(const Runtime::CallingFrame &Frame,
                                           uint32_t AvCodecId,
                                           uint32_t AvCodecCtxPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvCodecCtxId, MemInst, uint32_t, AvCodecCtxPtr,
                "Failed when accessing the return AVCodecContext Memory"sv);
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, AVCodec);

  AVCodecContext *AvCodecCtx = avcodec_alloc_context3(AvCodec);
  FFMPEG_PTR_STORE(AvCodecCtx, AvCodecCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecParametersFromContext::body(const Runtime::CallingFrame &,
                                   uint32_t AvCodecParamId,
                                   uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecParam, AvCodecParamId, AVCodecParameters);
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return avcodec_parameters_from_context(AvCodecParam, AvCodecCtx);
}

Expect<int32_t> AVCodecParametersFree::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecParamId) {
  FFMPEG_PTR_FETCH(AvCodecParam, AvCodecParamId, AVCodecParameters);

  avcodec_parameters_free(&AvCodecParam);
  FFMPEG_PTR_DELETE(AvCodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecFreeContext::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);

  avcodec_free_context(&AvCodecCtx);
  FFMPEG_PTR_DELETE(AvCodecCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecParametersAlloc::body(const Runtime::CallingFrame &Frame,
                                             uint32_t AvCodecParamPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvCodecParamId, MemInst, uint32_t, AvCodecParamPtr,
                "Failed when accessing the return AVCodecParameters Memory"sv);

  FFMPEG_PTR_FETCH(AvCodecParam, *AvCodecParamId, AVCodecParameters);
  AvCodecParam = avcodec_parameters_alloc();
  FFMPEG_PTR_STORE(AvCodecParam, AvCodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecGetType::body(const Runtime::CallingFrame &,
                                     uint32_t AvCodecIdIndex) {
  AVCodecID const AvCodecId =
      FFmpegUtils::CodecID::intoAVCodecID(AvCodecIdIndex);
  AVMediaType const MediaType = avcodec_get_type(AvCodecId);
  return FFmpegUtils::MediaType::fromMediaType(MediaType);
}

Expect<int32_t> AVCodecOpen2::body(const Runtime::CallingFrame &,
                                   uint32_t AvCodecCtxId, uint32_t AvCodecId,
                                   uint32_t AvDictionaryId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvDictionary, AvDictionaryId, AVDictionary *);
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, AVCodec);
  return avcodec_open2(AvCodecCtx, AvCodec, AvDictionary);
}

Expect<int32_t> AVCodecFindDecoder::body(const Runtime::CallingFrame &Frame,
                                         uint32_t ID, uint32_t AvCodecPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVCodecId, MemInst, uint32_t, AvCodecPtr,
                "Failed when accessing the return AVCodec Memory"sv);

  AVCodecID const Id = FFmpegUtils::CodecID::intoAVCodecID(ID);

  const AVCodec *AvCodec = avcodec_find_decoder(Id);

  // Setting AvCodec value as NULL.
  if (AvCodec == nullptr) {
    *AVCodecId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  FFMPEG_PTR_STORE(const_cast<AVCodec *>(AvCodec), AVCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecIsEncoder::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return av_codec_is_encoder(AvCodec);
}

Expect<int32_t> AVCodecIsDecoder::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return av_codec_is_decoder(AvCodec);
}

Expect<int32_t> AVCodecClose::body(const Runtime::CallingFrame &,
                                   uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  int Res = avcodec_close(AvCodecCtx);
  FFMPEG_PTR_DELETE(AvCodecCtxId);
  return Res;
}

Expect<int32_t> AVCodecParametersToContext::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecCtxId,
                                                 uint32_t AvCodecParamId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvCodecParam, AvCodecParamId, AVCodecParameters);

  return avcodec_parameters_to_context(AvCodecCtx, AvCodecParam);
}

Expect<int32_t> AVCodecReceiveFrame::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId,
                                          uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return avcodec_receive_frame(AvCodecCtx, AvFrame);
}

Expect<int32_t> AVCodecSendPacket::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecCtxId,
                                        uint32_t PacketId) {
  FFMPEG_PTR_FETCH(AVCodecCtx, AvCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvPacket, PacketId,
                   AVPacket); // Can send Null AVPacket, to close the stream.
  return avcodec_send_packet(AVCodecCtx, AvPacket);
}

Expect<int32_t> AVCodecFindEncoder::body(const Runtime::CallingFrame &Frame,
                                         uint32_t ID, uint32_t AVCodecPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVCodecId, MemInst, uint32_t, AVCodecPtr,
                "Failed when accessing the return AVCodec Memory"sv);

  AVCodecID const Id = FFmpegUtils::CodecID::intoAVCodecID(ID);

  const AVCodec *AvCodec = avcodec_find_encoder(Id);

  // Setting AvCodec value as NULL.
  if (AvCodec == nullptr) {
    *AVCodecId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  FFMPEG_PTR_STORE(const_cast<AVCodec *>(AvCodec), AVCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecReceivePacket::body(const Runtime::CallingFrame &,
                                           uint32_t AVCodecCtxId,
                                           uint32_t PacketId) {
  FFMPEG_PTR_FETCH(AVCodecCtx, AVCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvPacket, PacketId, AVPacket);
  return avcodec_receive_packet(AVCodecCtx, AvPacket);
}

Expect<int32_t> AVCodecSendFrame::body(const Runtime::CallingFrame &,
                                       uint32_t AVCodecCtxId,
                                       uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AVCodecCtx, AVCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return avcodec_send_frame(AVCodecCtx, AvFrame);
}

Expect<int32_t>
AVCodecFindDecoderByName::body(const Runtime::CallingFrame &Frame,
                               uint32_t AVCodecPtr, uint32_t NamePtr,
                               uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVCodecId, MemInst, uint32_t, AVCodecPtr,
                "Failed when accessing the return AVCodec Memory"sv);
  MEM_PTR_CHECK(NameId, MemInst, char, NamePtr,
                "Failed when accessing the return URL memory"sv);

  std::string Name;
  std::copy_n(NameId, NameLen, std::back_inserter(Name));

  AVCodec const *AvCodec = avcodec_find_decoder_by_name(Name.c_str());

  if (AvCodec == nullptr) {
    *AVCodecId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  FFMPEG_PTR_STORE(const_cast<AVCodec *>(AvCodec), AVCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecFindEncoderByName::body(const Runtime::CallingFrame &Frame,
                               uint32_t AVCodecPtr, uint32_t NamePtr,
                               uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVCodecId, MemInst, uint32_t, AVCodecPtr,
                "Failed when accessing the return AVCodec Memory"sv);
  MEM_PTR_CHECK(NameId, MemInst, char, NamePtr,
                "Failed when accessing the return URL memory"sv);

  std::string Name;
  std::copy_n(NameId, NameLen, std::back_inserter(Name));

  AVCodec const *AvCodec = avcodec_find_encoder_by_name(Name.c_str());

  if (AvCodec == nullptr) {
    *AVCodecId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  FFMPEG_PTR_STORE(const_cast<AVCodec *>(AvCodec), AVCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPacketRescaleTs::body(const Runtime::CallingFrame &,
                                        uint32_t AvPacketId, int32_t SrcNum,
                                        int32_t SrcDen, int32_t DestNum,
                                        int32_t DestDen) {
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  AVRational const Src = av_make_q(SrcNum, SrcDen);
  AVRational const Dest = av_make_q(DestNum, DestDen);

  av_packet_rescale_ts(AvPacket, Src, Dest);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPacketMakeWritable::body(const Runtime::CallingFrame &,
                                           uint32_t AVPacketId) {
  FFMPEG_PTR_FETCH(AvPacket, AVPacketId, AVPacket);
  return av_packet_make_writable(AvPacket);
}

Expect<int32_t> AVCodecParametersCopy::body(const Runtime::CallingFrame &,
                                            uint32_t AvFormatCtxId,
                                            uint32_t AVCodecParamId,
                                            uint32_t StreamIdx) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvCodecParam, AVCodecParamId, AVCodecParameters);

  AVStream **AvStream = AvFormatCtx->streams;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= StreamIdx; I++)
    AvStream++;

  return avcodec_parameters_copy((*AvStream)->codecpar, AvCodecParam);
}

Expect<uint32_t> AVCodecVersion::body(const Runtime::CallingFrame &) {
  return avcodec_version();
}

Expect<int32_t> AVCodecFlushBuffers::body(const Runtime::CallingFrame &,
                                          uint32_t AVCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AVCodecCtxId, AVCodecContext);
  avcodec_flush_buffers(AvCodecCtx);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecConfigurationLength::body(const Runtime::CallingFrame &) {
  const char *Config = avcodec_configuration();
  return strlen(Config);
}

Expect<int32_t> AVCodecConfiguration::body(const Runtime::CallingFrame &Frame,
                                           uint32_t ConfigPtr,
                                           uint32_t ConfigLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ConfigBuf, MemInst, char, ConfigPtr, ConfigLen, "");

  const char *Config = avcodec_configuration();
  std::copy_n(Config, ConfigLen, ConfigBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecLicenseLength::body(const Runtime::CallingFrame &) {
  const char *License = avcodec_license();
  return strlen(License);
}

Expect<int32_t> AVCodecLicense::body(const Runtime::CallingFrame &Frame,
                                     uint32_t LicensePtr, uint32_t LicenseLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LicenseBuf, MemInst, char, LicensePtr, LicenseLen, "");

  const char *License = avcodec_license();
  std::copy_n(License, LicenseLen, LicenseBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
