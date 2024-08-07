// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avChapter.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int64_t> AVChapterId::body(const Runtime::CallingFrame &,
                                  uint32_t AvFormatCtxId, uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  return static_cast<AVChapter *>(*AvChapter)->id;
}

Expect<int32_t> AVChapterSetId::body(const Runtime::CallingFrame &,
                                     uint32_t AvFormatCtxId,
                                     uint32_t ChapterIdx, int64_t ChapterId) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
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
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
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

  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  (*AvChapter)->time_base = Timebase;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVChapterStart::body(const Runtime::CallingFrame &,
                                     uint32_t AvFormatCtxId,
                                     uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  return static_cast<AVChapter *>(*AvChapter)->start;
}

Expect<int32_t> AVChapterSetStart::body(const Runtime::CallingFrame &,
                                        uint32_t AvFormatCtxId,
                                        uint32_t ChapterIdx,
                                        int64_t StartValue) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  (*AvChapter)->start = StartValue;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVChapterEnd::body(const Runtime::CallingFrame &,
                                   uint32_t AvFormatCtxId,
                                   uint32_t ChapterIdx) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  return static_cast<AVChapter *>(*AvChapter)->end;
}

Expect<int32_t> AVChapterSetEnd::body(const Runtime::CallingFrame &,
                                      uint32_t AvFormatCtxId,
                                      uint32_t ChapterIdx, int64_t EndValue) {
  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
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

  AVDictionary **AvDictionary =
      static_cast<AVDictionary **>(av_malloc(sizeof(AVDictionary *)));
  AVChapter **AvChapter = AvFormatCtx->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  *AvDictionary = (*AvChapter)->metadata;
  FFMPEG_PTR_STORE(AvDictionary, DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVChapterSetMetadata::body(const Runtime::CallingFrame &,
                                           uint32_t AvFormatCtxId,
                                           uint32_t ChapterIdx,
                                           uint32_t DictId) {
  FFMPEG_PTR_FETCH(AvFormatCtx, AvFormatCtxId, AVFormatContext);
  FFMPEG_PTR_FETCH(AvDictionary, DictId, AVDictionary *);

  AVChapter **AvChapter = AvFormatCtx->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int I = 1; I <= ChapterIdx; I++) {
    AvChapter++;
  }

  if (AvDictionary == nullptr) {
    (*AvChapter)->metadata = nullptr;
  } else {
    (*AvChapter)->metadata = *AvDictionary;
  }
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
