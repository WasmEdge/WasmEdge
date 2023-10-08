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

class AVChapterTimebase : public WasmEdgeFFmpegAVFormat<AVChapterTimebase> {
public:
  AVChapterTimebase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t ChapterIdx);
};

class AVChapterStart : public WasmEdgeFFmpegAVFormat<AVChapterStart> {
public:
  AVChapterStart(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterEnd : public WasmEdgeFFmpegAVFormat<AVChapterEnd> {
public:
  AVChapterEnd(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
