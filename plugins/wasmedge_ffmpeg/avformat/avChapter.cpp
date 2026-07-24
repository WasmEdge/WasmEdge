// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avChapter.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

namespace {
AVChapter **chapterAt(AVFormatContext *Ctx, uint32_t Idx) {
  if (Ctx == nullptr) {
    return nullptr;
  }
  return checkedArraySlot(Ctx->chapters, Ctx->nb_chapters, Idx);
}
} // namespace

Expect<int64_t> AVChapterId::body(const Runtime::CallingFrame &,
                                  uint32_t AvFormatCtxId, uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterId: invalid chapter index {} "
                  "(format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVChapter *>(*AvChapter)->id;
}

Expect<int32_t> AVChapterSetId::body(const Runtime::CallingFrame &,
                                     uint32_t AvFormatCtxId,
                                     uint32_t ChapterIdx, int64_t ChapterId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterSetId: invalid chapter index "
                  "{} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  (*AvChapter)->id = ChapterId;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVChapterTimebase::body(const Runtime::CallingFrame &Frame,
                                        uint32_t NumPtr, uint32_t DenPtr,
                                        uint32_t AvFormatCtxId,
                                        uint32_t ChapterIdx) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr, "");
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr, "");

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterTimebase: invalid chapter "
                  "index {} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  AVRational const AvRational = static_cast<AVChapter *>(*AvChapter)->time_base;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVChapterSetTimebase::body(const Runtime::CallingFrame &,
                                           int32_t Num, int32_t Den,
                                           uint32_t AvFormatCtxId,
                                           uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVRational const Timebase = av_make_q(Num, Den);

  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterSetTimebase: invalid chapter "
                  "index {} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  (*AvChapter)->time_base = Timebase;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVChapterStart::body(const Runtime::CallingFrame &,
                                     uint32_t AvFormatCtxId,
                                     uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterStart: invalid chapter index "
                  "{} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVChapter *>(*AvChapter)->start;
}

Expect<int32_t> AVChapterSetStart::body(const Runtime::CallingFrame &,
                                        uint32_t AvFormatCtxId,
                                        uint32_t ChapterIdx,
                                        int64_t StartValue) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterSetStart: invalid chapter "
                  "index {} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  (*AvChapter)->start = StartValue;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVChapterEnd::body(const Runtime::CallingFrame &,
                                   uint32_t AvFormatCtxId,
                                   uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterEnd: invalid chapter index {} "
                  "(format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  return static_cast<AVChapter *>(*AvChapter)->end;
}

Expect<int32_t> AVChapterSetEnd::body(const Runtime::CallingFrame &,
                                      uint32_t AvFormatCtxId,
                                      uint32_t ChapterIdx, int64_t EndValue) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = chapterAt(AvFormatContext, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterSetEnd: invalid chapter index "
                  "{} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatContext ? AvFormatContext->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  (*AvChapter)->end = EndValue;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVChapterMetadata::body(const Runtime::CallingFrame &Frame,
                                        uint32_t AvFormatCtxId,
                                        uint32_t ChapterIdx, uint32_t DictPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed when accessing the return AVDictionary memory"sv);

  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);

  AVChapter **AvChapter = chapterAt(AvFormatCtx, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterMetadata: invalid chapter "
                  "index {} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatCtx ? AvFormatCtx->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  FFMPEG_PTR_STORE_CHILD(&(*AvChapter)->metadata, DictId, AvFormatCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVChapterSetMetadata::body(const Runtime::CallingFrame &,
                                           uint32_t AvFormatCtxId,
                                           uint32_t ChapterIdx,
                                           uint32_t DictId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, DictId, AVDictionary *);

  AVChapter **AvChapter = chapterAt(AvFormatCtx, ChapterIdx);
  if (AvChapter == nullptr) {
    spdlog::error("[WasmEdge-FFmpeg] AVChapterSetMetadata: invalid chapter "
                  "index {} (format context id {}, nb_chapters={})"sv,
                  ChapterIdx, AvFormatCtxId,
                  AvFormatCtx ? AvFormatCtx->nb_chapters : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  FFMPEG_PTR_CHECK_NONZERO(AvDictionary, DictId,
                           static_cast<int32_t>(ErrNo::InternalError));
  return applyMetadataCopy(&(*AvChapter)->metadata, AvDictionary);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
