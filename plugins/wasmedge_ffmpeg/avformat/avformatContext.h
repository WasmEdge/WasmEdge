#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVFormatCtxIFormat : public WasmEdgeFFmpegAVFormat<AVFormatCtxIFormat> {
public:
  AVFormatCtxIFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t AvInputFormatPtr);
};

class AVFormatCtxOFormat : public WasmEdgeFFmpegAVFormat<AVFormatCtxOFormat> {
public:
  AVFormatCtxOFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t AvOutputFormatPtr);
};

class AVFormatCtxProbeScore
    : public WasmEdgeFFmpegAVFormat<AVFormatCtxProbeScore> {
public:
  AVFormatCtxProbeScore(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatCtxNbStreams
    : public WasmEdgeFFmpegAVFormat<AVFormatCtxNbStreams> {
public:
  AVFormatCtxNbStreams(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvFormatCtxId);
};

class AVFormatCtxBitRate : public WasmEdgeFFmpegAVFormat<AVFormatCtxBitRate> {
public:
  AVFormatCtxBitRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatCtxDuration : public WasmEdgeFFmpegAVFormat<AVFormatCtxDuration> {
public:
  AVFormatCtxDuration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatCtxNbChapters
    : public WasmEdgeFFmpegAVFormat<AVFormatCtxNbChapters> {
public:
  AVFormatCtxNbChapters(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvFormatCtxId);
};

class AVFormatCtxSetNbChapters
    : public WasmEdgeFFmpegAVFormat<AVFormatCtxSetNbChapters> {
public:
  AVFormatCtxSetNbChapters(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t NbChapters);
};

class AVFormatCtxMetadata : public WasmEdgeFFmpegAVFormat<AVFormatCtxMetadata> {
public:
  AVFormatCtxMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t DictPtr);
};

class AVFormatCtxSetMetadata
    : public WasmEdgeFFmpegAVFormat<AVFormatCtxSetMetadata> {
public:
  AVFormatCtxSetMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t DictId);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
