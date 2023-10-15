#include "avformat_func.h"

extern "C" {
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVFormatOpenInput::body(const Runtime::CallingFrame &Frame,
                                        uint32_t AvFormatCtxPtr,
                                        uint32_t UrlPtr, uint32_t UrlSize,
                                        uint32_t AvInputFormatId,
                                        uint32_t AvDictionaryId) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(urlId, MemInst, char, UrlPtr,
                "Failed when accessing the return URL memory");
  MEM_PTR_CHECK(AvFormatCtxId, MemInst, uint32_t, AvFormatCtxPtr,
                "Failed when accessing the return AVFormatContext Memory");

  std::string TargetUrl;
  std::copy_n(urlId, UrlSize, std::back_inserter(TargetUrl));

  AVFormatContext *AvFormatContext = NULL;
  FFMPEG_PTR_FETCH(AvDictionary, AvDictionaryId, AVDictionary *);
  FFMPEG_PTR_FETCH(AvInputFormat, AvInputFormatId, AVInputFormat);

  int const Res = avformat_open_input(&AvFormatContext, TargetUrl.c_str(),
                                      AvInputFormat, AvDictionary);
  FFMPEG_PTR_STORE(AvFormatContext, AvFormatCtxId);
  return Res;
}

Expect<int32_t> AVFormatFindStreamInfo::body(const Runtime::CallingFrame &,
                                             uint32_t AvFormatCtxId,
                                             uint32_t AvDictionaryId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, AvDictionaryId, AVDictionary *);
  return avformat_find_stream_info(AvFormatContext, AvDictionary);
}

Expect<int32_t> AVFormatCloseInput::body(const Runtime::CallingFrame &,
                                         uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  avformat_close_input(&AvFormatCtx);
  FFMPEG_PTR_DELETE(AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVReadPause::body(const Runtime::CallingFrame &,
                                  uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return av_read_pause(AvFormatContext);
}

Expect<int32_t> AVReadPlay::body(const Runtime::CallingFrame &,
                                 uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return av_read_play(AvFormatContext);
}

Expect<int32_t> AVFormatSeekFile::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx, int64_t MinTs,
                                       int64_t Ts, int64_t MaxTs,
                                       int32_t Flags) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return avformat_seek_file(AvFormatContext, StreamIdx, MinTs, Ts, MaxTs,
                            Flags);
}

Expect<int32_t> AVDumpFormat::body(const Runtime::CallingFrame &Frame,
                                   uint32_t AvFormatCtxId, int32_t Idx,
                                   uint32_t UrlPtr, uint32_t UrlSize,
                                   int32_t IsOutput) {
  std::string TargetUrl;

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(UrlBuf, MemInst, char, UrlPtr, "");

  std::copy_n(UrlBuf, UrlSize, std::back_inserter(TargetUrl));
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);

  av_dump_format(AvFormatCtx, Idx, TargetUrl.c_str(), IsOutput);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatFreeContext::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  avformat_free_context(AvFormatCtx);
  FFMPEG_PTR_DELETE(AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFindBestStream::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       int32_t MediaTypeId,
                                       int32_t WantedStream,
                                       int32_t RelatedStream,
                                       uint32_t DecoderRetId, int32_t Flags) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(DecoderRet, DecoderRetId, const AVCodec *);

  AVMediaType const AvMediaType =
      FFmpegUtils::MediaType::intoMediaType(MediaTypeId);
  return av_find_best_stream(AvFormatContext, AvMediaType, WantedStream,
                             RelatedStream, DecoderRet, Flags);
}

Expect<int32_t> AVReadFrame::body(const Runtime::CallingFrame &,
                                  uint32_t AvFormatCtxId, uint32_t PacketId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvPacket, PacketId, AVPacket);

  return av_read_frame(AvFormatContext, AvPacket);
}

Expect<int32_t> AVIOClose::body(const Runtime::CallingFrame &,
                                uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  avio_close(AvFormatCtx->pb);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
