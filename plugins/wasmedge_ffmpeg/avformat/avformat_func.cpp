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

Expect<int32_t> AVFormatNetworkInit::body(const Runtime::CallingFrame &) {
  return avformat_network_init();
}

Expect<int32_t> AVFormatNetworkDeInit::body(const Runtime::CallingFrame &) {
  return avformat_network_deinit();
}

Expect<int32_t> AVFormatWriteHeader::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId,
                                          uint32_t DictId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);
  return avformat_write_header(AvFormatContext, AvDict);
}

Expect<int32_t> AVFormatWriteTrailer::body(const Runtime::CallingFrame &,
                                           uint32_t AvFormatCtxId) {

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  return av_write_trailer(AvFormatContext);
}

Expect<int32_t> AVFormatAllocOutputContext2::body(
    const Runtime::CallingFrame &Frame, uint32_t AvFormatCtxPtr,
    uint32_t AVOutputFormatId, uint32_t FormatNamePtr, uint32_t FormatLen,
    uint32_t FileNamePtr, uint32_t FileNameLen) {

  std::string Format;
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(FileId, MemInst, char, FileNamePtr,
                "Failed when accessing the return FileName memory");
  if (FormatLen > 0) {
    MEM_PTR_CHECK(FormatId, MemInst, char, FormatNamePtr,
                  "Failed when accessing the return FormatName memory");

    std::copy_n(FormatId, FormatLen, std::back_inserter(Format));
  }
  MEM_PTR_CHECK(AvFormatCtxId, MemInst, uint32_t, AvFormatCtxPtr,
                "Failed when accessing the return AVFormatContext Memory");

  std::string File;
  std::copy_n(FileId, FileNameLen, std::back_inserter(File));

  AVFormatContext *AvFormatContext = NULL;
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);

  int Res;
  if (FormatLen == 0) {
    Res = avformat_alloc_output_context2(&AvFormatContext, AvOutputFormat, NULL,
                                         File.c_str());
  } else {
    Res = avformat_alloc_output_context2(&AvFormatContext, AvOutputFormat,
                                         Format.c_str(), File.c_str());
  }
  FFMPEG_PTR_STORE(AvFormatContext, AvFormatCtxId);
  return Res;
}

Expect<int32_t> AVIOOpen::body(const Runtime::CallingFrame &Frame,
                               uint32_t AvFormatCtxId, uint32_t FileNamePtr,
                               uint32_t FileNameLen, int32_t Flags) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(FileId, MemInst, char, FileNamePtr,
                "Failed when accessing the return FileName memory");

  std::string File;
  std::copy_n(FileId, FileNameLen, std::back_inserter(File));

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);

  return avio_open(&(AvFormatContext->pb), File.c_str(), Flags);
}

Expect<int32_t> AVIOOpen2::body(const Runtime::CallingFrame &Frame,
                                uint32_t AvFormatCtxtId, uint32_t UrlPtr,
                                uint32_t UrlLen, int32_t Flags,
                                uint32_t AVIOInterruptCBId,
                                uint32_t AVDictionaryId) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(UrlId, MemInst, char, UrlPtr,
                "Failed when accessing the return Url memory");

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxtId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, AVDictionaryId, AVDictionary *);
  FFMPEG_PTR_FETCH(AvIOInterruptCB, AVIOInterruptCBId, AVIOInterruptCB);

  std::string TargetUrl;
  std::copy_n(UrlId, UrlLen, std::back_inserter(TargetUrl));

  return avio_open2(&(AvFormatCtx->pb), TargetUrl.c_str(), Flags,
                    AvIOInterruptCB, AvDictionary);
}

Expect<uint32_t> AVFormatVersion::body(const Runtime::CallingFrame &) {
  return avformat_version();
}

Expect<int32_t> AVChapterMallocz::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AVChapterPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AvChapterId, MemInst, uint32_t, AVChapterPtr,
                "Failed to access Memory for AVChapterPtr")

  AVChapter *AvChapter =
      static_cast<AVChapter *>(av_mallocz(sizeof(AVChapter)));
  FFMPEG_PTR_STORE(AvChapter, AvChapterId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVChapterDynarrayAdd::body(const Runtime::CallingFrame &Frame,
                                           uint32_t AvFormatCtxId,
                                           int32_t NbChaptersPtr,
                                           uint32_t AvChapterId) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(NbChapters, MemInst, int32_t, NbChaptersPtr,
                "Failed to access Memory for NbChaptersPtr")

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvChapter, AvChapterId, AVChapter);

  av_dynarray_add(&(AvFormatContext->chapters), NbChapters, AvChapter);
  if (*(AvFormatContext->chapters) == NULL && *(NbChapters) == 0)
    return static_cast<int32_t>(ErrNo::InternalError);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFreeP::body(const Runtime::CallingFrame &,
                              uint32_t AvChapterId) {

  FFMPEG_PTR_FETCH(AvChapter, AvChapterId, AVChapter);
  av_freep(AvChapter);
  FFMPEG_PTR_DELETE(AvChapterId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
