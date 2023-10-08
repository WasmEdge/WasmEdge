#include "avChapter.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int64_t> AVChapterId::body(const Runtime::CallingFrame &,
                     uint32_t AvFormatCtxId, uint32_t ChapterIdx){

  FFMPEG_PTR_FETCH(AvFormatContext, AvFormatCtxId, AVFormatContext);
  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int i = 1; i <= ChapterIdx; i++)
    AvChapter++;

  return static_cast<AVChapter *>(*AvChapter)->id;
}

Expect<int32_t> AVChapterTimebase::body(const Runtime::CallingFrame &Frame,uint32_t NumPtr,uint32_t DenPtr,
                     uint32_t AvFormatCtxId, uint32_t ChapterIdx){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(Num,MemInst,int32_t,NumPtr,"");
  MEM_PTR_CHECK(Den,MemInst,int32_t,DenPtr,"");

  FFMPEG_PTR_FETCH(AvFormatContext,AvFormatCtxId,AVFormatContext);

  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int i = 1; i <= ChapterIdx; i++)
    AvChapter++;

  AVRational const AvRational = static_cast<AVChapter *>(*AvChapter)->time_base;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVChapterStart::body(const Runtime::CallingFrame &,
                     uint32_t AvFormatCtxId, uint32_t ChapterIdx){

  FFMPEG_PTR_FETCH(AvFormatContext,AvFormatCtxId,AVFormatContext);

  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int i = 1; i <= ChapterIdx; i++)
    AvChapter++;

  return static_cast<AVChapter *>(*AvChapter)->start;
}

Expect<int64_t> AVChapterEnd::body(const Runtime::CallingFrame &,
                     uint32_t AvFormatCtxId, uint32_t ChapterIdx){

  FFMPEG_PTR_FETCH(AvFormatContext,AvFormatCtxId,AVFormatContext);

  AVChapter **AvChapter = AvFormatContext->chapters;

  // No check here (Check)
  // Raw Pointer Iteration.
  for (unsigned int i = 1; i <= ChapterIdx; i++)
    AvChapter++;

  return static_cast<AVChapter *>(*AvChapter)->end;
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
