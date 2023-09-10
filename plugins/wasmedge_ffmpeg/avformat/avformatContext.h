#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {


class AVFormatCtxIFormat : public WasmEdgeFFmpegAVFormat<AVFormatCtxIFormat> {
public:
    AVFormatCtxIFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avInputFormatPtr);
};

class AVFormatCtxProbeScore : public WasmEdgeFFmpegAVFormat<AVFormatCtxProbeScore> {
public:
    AVFormatCtxProbeScore(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};


class AVFormatCtxNbStreams : public WasmEdgeFFmpegAVFormat<AVFormatCtxNbStreams> {
public:
  AVFormatCtxNbStreams(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};


class AVFormatCtxBitRate : public WasmEdgeFFmpegAVFormat<AVFormatCtxBitRate> {
public:
  AVFormatCtxBitRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};

class AVFormatCtxDuration : public WasmEdgeFFmpegAVFormat<AVFormatCtxDuration> {
public:
  AVFormatCtxDuration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};

class AVFormatCtxNbChapters : public WasmEdgeFFmpegAVFormat<AVFormatCtxNbChapters> {
public:
  AVFormatCtxNbChapters(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};

class AVFormatCtxGetAVStream : public WasmEdgeFFmpegAVFormat<AVFormatCtxGetAVStream> {
public:
  AVFormatCtxGetAVStream(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avStreamPtr);
};

}
}
}
}
