// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
  MEM_SPAN_CHECK(UrlId, MemInst, char, UrlPtr, UrlSize,
                 "Failed when accessing the return URL memory"sv);
  MEM_PTR_CHECK(AvFormatCtxId, MemInst, uint32_t, AvFormatCtxPtr,
                "Failed when accessing the return AVFormatContext Memory"sv);

  std::string TargetUrl(UrlId.data(), UrlSize);

  AVFormatContext *AvFormatContext = nullptr;
  FFMPEG_PTR_FETCH(AvDictionary, AvDictionaryId, AVDictionary *);
  FFMPEG_PTR_FETCH(AvInputFormat, AvInputFormatId, AVInputFormat);
  FFMPEG_PTR_CHECK_NONZERO(AvDictionary, AvDictionaryId,
                           static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_CHECK_NONZERO(AvInputFormat, AvInputFormatId,
                           static_cast<int32_t>(ErrNo::InternalError));

  int const Res = avformat_open_input(&AvFormatContext, TargetUrl.c_str(),
                                      AvInputFormat, AvDictionary);
  if (Res < 0) {
    return Res;
  }
  FFMPEG_PTR_STORE(AvFormatContext, AvFormatCtxId);
  return Res;
}

Expect<int32_t> AVFormatFindStreamInfo::body(const Runtime::CallingFrame &,
                                             uint32_t AvFormatCtxId,
                                             uint32_t AvDictionaryId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(AvDictionary, AvDictionaryId, AVDictionary *);
  FFMPEG_PTR_CHECK_NONZERO(AvDictionary, AvDictionaryId,
                           static_cast<int32_t>(ErrNo::InternalError));
  return avformat_find_stream_info(AvFormatContext, AvDictionary);
}

Expect<int32_t> AVFormatCloseInput::body(const Runtime::CallingFrame &,
                                         uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK_FREE(AvFormatCtx, AvFormatCtxId,
                        static_cast<int32_t>(ErrNo::InternalError));
  avformat_close_input(&AvFormatCtx);
  Env.get()->deallocChildren(AvFormatCtxId);
  FFMPEG_PTR_DELETE(AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVReadPause::body(const Runtime::CallingFrame &,
                                  uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  return av_read_pause(AvFormatContext);
}

Expect<int32_t> AVReadPlay::body(const Runtime::CallingFrame &,
                                 uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  return av_read_play(AvFormatContext);
}

Expect<int32_t> AVFormatSeekFile::body(const Runtime::CallingFrame &,
                                       uint32_t AvFormatCtxId,
                                       uint32_t StreamIdx, int64_t MinTs,
                                       int64_t Ts, int64_t MaxTs,
                                       int32_t Flags) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  return avformat_seek_file(AvFormatContext, StreamIdx, MinTs, Ts, MaxTs,
                            Flags);
}

Expect<int32_t> AVDumpFormat::body(const Runtime::CallingFrame &Frame,
                                   uint32_t AvFormatCtxId, int32_t Idx,
                                   uint32_t UrlPtr, uint32_t UrlSize,
                                   int32_t IsOutput) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(UrlBuf, MemInst, char, UrlPtr, UrlSize, "");

  std::string TargetUrl(UrlBuf.data(), UrlSize);
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatCtx, static_cast<int32_t>(ErrNo::InternalError));

  av_dump_format(AvFormatCtx, Idx, TargetUrl.c_str(), IsOutput);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatFreeContext::body(const Runtime::CallingFrame &,
                                          uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK_FREE(AvFormatCtx, AvFormatCtxId,
                        static_cast<int32_t>(ErrNo::InternalError));
  avformat_free_context(AvFormatCtx);
  Env.get()->deallocChildren(AvFormatCtxId);
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
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));

  // No host function mints ids for a const AVCodec ** output cell, and tag
  // normalization makes a stored AVCodec handle indistinguishable from one;
  // passing it through would let av_find_best_stream overwrite the static
  // AVCodec object. Reject any nonzero id and always pass NULL.
  if (DecoderRetId != 0) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVMediaType const AvMediaType =
      FFmpegUtils::MediaType::intoMediaType(MediaTypeId);
  return av_find_best_stream(AvFormatContext, AvMediaType, WantedStream,
                             RelatedStream, nullptr, Flags);
}

Expect<int32_t> AVReadFrame::body(const Runtime::CallingFrame &,
                                  uint32_t AvFormatCtxId, uint32_t PacketId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(AvPacket, PacketId, AVPacket);
  FFMPEG_PTR_CHECK(AvPacket, static_cast<int32_t>(ErrNo::InternalError));

  return av_read_frame(AvFormatContext, AvPacket);
}

Expect<int32_t> AVIOClose::body(const Runtime::CallingFrame &,
                                uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK_FREE(AvFormatCtx, AvFormatCtxId,
                        static_cast<int32_t>(ErrNo::InternalError));
  avio_closep(&AvFormatCtx->pb);
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
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);
  FFMPEG_PTR_CHECK_NONZERO(AvDict, DictId,
                           static_cast<int32_t>(ErrNo::InternalError));
  return avformat_write_header(AvFormatContext, AvDict);
}

Expect<int32_t> AVFormatWriteTrailer::body(const Runtime::CallingFrame &,
                                           uint32_t AvFormatCtxId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));
  return av_write_trailer(AvFormatContext);
}

Expect<int32_t> AVFormatAllocOutputContext2::body(
    const Runtime::CallingFrame &Frame, uint32_t AvFormatCtxPtr,
    uint32_t AVOutputFormatId, uint32_t FormatNamePtr, uint32_t FormatLen,
    uint32_t FileNamePtr, uint32_t FileNameLen) {
  std::string Format;
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(FileId, MemInst, char, FileNamePtr, FileNameLen,
                 "Failed when accessing the return FileName memory"sv);
  if (FormatLen > 0) {
    MEM_SPAN_CHECK(FormatId, MemInst, char, FormatNamePtr, FormatLen,
                   "Failed when accessing the return FormatName memory"sv);

    Format.assign(FormatId.data(), FormatLen);
  }
  MEM_PTR_CHECK(AvFormatCtxId, MemInst, uint32_t, AvFormatCtxPtr,
                "Failed when accessing the return AVFormatContext Memory"sv);

  std::string File(FileId.data(), FileNameLen);

  AVFormatContext *AvFormatContext = nullptr;
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);
  FFMPEG_PTR_CHECK_NONZERO(AvOutputFormat, AVOutputFormatId,
                           static_cast<int32_t>(ErrNo::InternalError));

  int Res = 0;
  if (FormatLen == 0) {
    Res = avformat_alloc_output_context2(&AvFormatContext, AvOutputFormat,
                                         nullptr, File.c_str());
  } else {
    Res = avformat_alloc_output_context2(&AvFormatContext, AvOutputFormat,
                                         Format.c_str(), File.c_str());
  }
  if (Res < 0) {
    return Res;
  }
  FFMPEG_PTR_STORE(AvFormatContext, AvFormatCtxId);
  return Res;
}

Expect<int32_t> AVIOOpen::body(const Runtime::CallingFrame &Frame,
                               uint32_t AvFormatCtxId, uint32_t FileNamePtr,
                               uint32_t FileNameLen, int32_t Flags) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(FileId, MemInst, char, FileNamePtr, FileNameLen,
                 "Failed when accessing the return FileName memory"sv);

  std::string File(FileId.data(), FileNameLen);

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatContext, static_cast<int32_t>(ErrNo::InternalError));

  return avio_open(&(AvFormatContext->pb), File.c_str(), Flags);
}

Expect<int32_t> AVIOOpen2::body(const Runtime::CallingFrame &Frame,
                                uint32_t AvFormatCtxtId, uint32_t UrlPtr,
                                uint32_t UrlLen, int32_t Flags,
                                uint32_t AVIOInterruptCBId,
                                uint32_t AVDictionaryId) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(UrlId, MemInst, char, UrlPtr, UrlLen,
                 "Failed when accessing the return Url memory"sv);

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxtId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatCtx, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(AvDictionary, AVDictionaryId, AVDictionary *);
  FFMPEG_PTR_FETCH(AvIOInterruptCB, AVIOInterruptCBId, AVIOInterruptCB);
  FFMPEG_PTR_CHECK_NONZERO(AvDictionary, AVDictionaryId,
                           static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_CHECK_NONZERO(AvIOInterruptCB, AVIOInterruptCBId,
                           static_cast<int32_t>(ErrNo::InternalError));

  std::string TargetUrl(UrlId.data(), UrlLen);

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
                "Failed to access Memory for AVChapterPtr"sv)

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
                "Failed to access Memory for NbChaptersPtr"sv)

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvChapter, AvChapterId, AVChapter);

  if (AvFormatContext == nullptr || AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterDynarrayAdd: invalid format "
                  "context id {} or chapter id {}"sv,
                  AvFormatCtxId, AvChapterId);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  // A chapter already linked into a context is borrowed; re-adding it would
  // append the same pointer twice and double-free it on teardown.
  if (Env.get()->isBorrowed(AvChapterId)) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterDynarrayAdd: chapter id {} is "
                  "already linked into a format context"sv,
                  AvChapterId);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  // On a realloc failure av_dynarray_add frees and NULLs the array but leaves
  // nb_chapters at the old count; resync it to 0 and free the orphaned
  // chapters from the snapshot taken before the add.
  int const OldNbChaptersValue = static_cast<int>(AvFormatContext->nb_chapters);
  std::vector<AVChapter *> ExistingChapters;
  if (AvFormatContext->chapters != nullptr && OldNbChaptersValue > 0) {
    ExistingChapters.assign(AvFormatContext->chapters,
                            AvFormatContext->chapters + OldNbChaptersValue);
  }
  int NbChaptersValue = OldNbChaptersValue;
  av_dynarray_add(&(AvFormatContext->chapters), &NbChaptersValue, AvChapter);
  if (AvFormatContext->chapters == nullptr ||
      NbChaptersValue <= OldNbChaptersValue) {
    AvFormatContext->nb_chapters = 0;
    *NbChapters = 0;
    for (AVChapter *const OrphanChapter : ExistingChapters) {
      if (OrphanChapter == nullptr) {
        continue;
      }
      void *const MetadataKey = &OrphanChapter->metadata;
      av_dict_free(&OrphanChapter->metadata);
      AVChapter *ToFree = OrphanChapter;
      av_freep(&ToFree);
      Env.get()->deallocByValue(OrphanChapter);
      Env.get()->deallocByValue(MetadataKey);
    }
    spdlog::error("[WasmEdge-FFmpeg] AVChapterDynarrayAdd: av_dynarray_add "
                  "failed (format context id {})"sv,
                  AvFormatCtxId);
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  AvFormatContext->nb_chapters = static_cast<unsigned int>(NbChaptersValue);
  *NbChapters = NbChaptersValue;
  Env.get()->markBorrowedChild(AvChapterId, AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFreeP::body(const Runtime::CallingFrame &,
                              uint32_t AvChapterId) {
  FFMPEG_PTR_FETCH(AvChapter, AvChapterId, AVChapter);
  FFMPEG_PTR_CHECK_FREE(AvChapter, AvChapterId,
                        static_cast<int32_t>(ErrNo::InternalError));
  if (!Env.get()->isBorrowed(AvChapterId)) {
    av_dict_free(&AvChapter->metadata);
    av_freep(&AvChapter);
  }
  FFMPEG_PTR_DELETE(AvChapterId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVInterleavedWriteFrame::body(const Runtime::CallingFrame &,
                                              uint32_t AvFormatCtxId,
                                              uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatCtx, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  FFMPEG_PTR_CHECK_NONZERO(AvPacket, AvPacketId,
                           static_cast<int32_t>(ErrNo::InternalError));

  return av_interleaved_write_frame(AvFormatCtx, AvPacket);
}

Expect<int32_t> AVWriteFrame::body(const Runtime::CallingFrame &,
                                   uint32_t AvFormatCtxId,
                                   uint32_t AvPacketId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatCtx, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(AvPacket, AvPacketId, AVPacket);
  FFMPEG_PTR_CHECK_NONZERO(AvPacket, AvPacketId,
                           static_cast<int32_t>(ErrNo::InternalError));

  return av_write_frame(AvFormatCtx, AvPacket);
}

Expect<int32_t> AVFormatNewStream::body(const Runtime::CallingFrame &,
                                        uint32_t AvFormatCtxId,
                                        uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_CHECK(AvFormatCtx, 0);
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  FFMPEG_PTR_CHECK_NONZERO(AvCodec, AvCodecId, 0);
  AVStream *Stream = avformat_new_stream(AvFormatCtx, AvCodec);
  if (Stream == nullptr) {
    return 0;
  }
  return 1;
}

Expect<uint32_t> AVGuessCodec::body(const Runtime::CallingFrame &Frame,
                                    uint32_t AVIOFormatId,
                                    uint32_t ShortNamePtr,
                                    uint32_t ShortNameLen, uint32_t FileNamePtr,
                                    uint32_t FileNameLen, uint32_t MimeTypePtr,
                                    uint32_t MimeTypeLen, int32_t MediaTypeId) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (unlikely(MemInst == nullptr)) {
    spdlog::error("[WasmEdge-FFmpeg] Memory instance not found."sv);
    return FFmpegUtils::CodecID::fromAVCodecID(AV_CODEC_ID_NONE);
  }
  auto ShortNameBuf = MemInst->getSpan<char>(ShortNamePtr, ShortNameLen);
  auto FileNameBuf = MemInst->getSpan<char>(FileNamePtr, FileNameLen);
  auto MimeTypeBuf = MemInst->getSpan<char>(MimeTypePtr, MimeTypeLen);
  if (unlikely(ShortNameBuf.size() != ShortNameLen ||
               FileNameBuf.size() != FileNameLen ||
               MimeTypeBuf.size() != MimeTypeLen)) {
    spdlog::error("[WasmEdge-FFmpeg] AVGuessCodec: failed when accessing the "
                  "return string memory"sv);
    return FFmpegUtils::CodecID::fromAVCodecID(AV_CODEC_ID_NONE);
  }
  FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
  FFMPEG_PTR_CHECK(AvOutputFormat,
                   FFmpegUtils::CodecID::fromAVCodecID(AV_CODEC_ID_NONE));

  std::string ShortName(ShortNameBuf.data(), ShortNameLen);
  std::string FileName(FileNameBuf.data(), FileNameLen);
  std::string MimeType(MimeTypeBuf.data(), MimeTypeLen);

  AVMediaType const MediaType =
      FFmpegUtils::MediaType::intoMediaType(MediaTypeId);
  AVCodecID const Id =
      av_guess_codec(AvOutputFormat, ShortName.c_str(), FileName.c_str(),
                     MimeType.c_str(), MediaType);

  return FFmpegUtils::CodecID::fromAVCodecID(Id);
}

Expect<int32_t>
AVFormatConfigurationLength::body(const Runtime::CallingFrame &) {
  const char *Config = avformat_configuration();
  return strlen(Config);
}

Expect<int32_t> AVFormatConfiguration::body(const Runtime::CallingFrame &Frame,
                                            uint32_t ConfigPtr,
                                            uint32_t ConfigLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ConfigBuf, MemInst, char, ConfigPtr, ConfigLen, "");

  const char *Config = avformat_configuration();
  copyCStringToBuffer(ConfigBuf.data(), ConfigLen, Config);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFormatLicenseLength::body(const Runtime::CallingFrame &) {
  const char *License = avformat_license();
  return strlen(License);
}

Expect<int32_t> AVFormatLicense::body(const Runtime::CallingFrame &Frame,
                                      uint32_t LicensePtr,
                                      uint32_t LicenseLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LicenseBuf, MemInst, char, LicensePtr, LicenseLen, "");

  const char *License = avformat_license();
  copyCStringToBuffer(LicenseBuf.data(), LicenseLen, License);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
