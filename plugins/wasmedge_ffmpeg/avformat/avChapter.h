#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVChapterId : public WasmEdgeFFmpegAVFormat<AVChapterId> {
public:
  AVChapterId(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterSetId : public WasmEdgeFFmpegAVFormat<AVChapterSetId> {
public:
  AVChapterSetId(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       int64_t ChapterId);
};

class AVChapterTimebase : public WasmEdgeFFmpegAVFormat<AVChapterTimebase> {
public:
  AVChapterTimebase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t ChapterIdx);
};

class AVChapterSetTimebase
    : public WasmEdgeFFmpegAVFormat<AVChapterSetTimebase> {
public:
  AVChapterSetTimebase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Num,
                       int32_t Den, uint32_t AvFormatCtxId,
                       uint32_t ChapterIdx);
};

class AVChapterStart : public WasmEdgeFFmpegAVFormat<AVChapterStart> {
public:
  AVChapterStart(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterSetStart : public WasmEdgeFFmpegAVFormat<AVChapterSetStart> {
public:
  AVChapterSetStart(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       int64_t StartValue);
};

class AVChapterEnd : public WasmEdgeFFmpegAVFormat<AVChapterEnd> {
public:
  AVChapterEnd(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterSetEnd : public WasmEdgeFFmpegAVFormat<AVChapterSetEnd> {
public:
  AVChapterSetEnd(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       int64_t EndValue);
};

class AVChapterMetadata : public WasmEdgeFFmpegAVFormat<AVChapterMetadata> {
public:
  AVChapterMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       uint32_t DictPtr);
};

class AVChapterSetMetadata
    : public WasmEdgeFFmpegAVFormat<AVChapterSetMetadata> {
public:
  AVChapterSetMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       uint32_t DictId);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
