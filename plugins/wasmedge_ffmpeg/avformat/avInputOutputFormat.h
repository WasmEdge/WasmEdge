#pragma once
#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVIOFormatNameLength
    : public WasmEdgeFFmpegAVFormat<AVIOFormatNameLength> {
public:
  AVIOFormatNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatName : public WasmEdgeFFmpegAVFormat<AVInputFormatName> {
public:
  AVInputFormatName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t NamePtr,
                       uint32_t NameLen);
};

class AVOutputFormatName : public WasmEdgeFFmpegAVFormat<AVOutputFormatName> {
public:
  AVOutputFormatName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t NamePtr,
                       uint32_t NameLen);
};

class AVIOFormatLongNameLength
    : public WasmEdgeFFmpegAVFormat<AVIOFormatLongNameLength> {
public:
  AVIOFormatLongNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatLongName
    : public WasmEdgeFFmpegAVFormat<AVInputFormatLongName> {
public:
  AVInputFormatLongName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t LongNamePtr,
                       uint32_t LongNameLen);
};

class AVOutputFormatLongName
    : public WasmEdgeFFmpegAVFormat<AVOutputFormatLongName> {
public:
  AVOutputFormatLongName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t LongNamePtr,
                       uint32_t LongNameLen);
};

class AVIOFormatExtensionsLength
    : public WasmEdgeFFmpegAVFormat<AVIOFormatExtensionsLength> {
public:
  AVIOFormatExtensionsLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatExtensions
    : public WasmEdgeFFmpegAVFormat<AVInputFormatExtensions> {
public:
  AVInputFormatExtensions(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t Extensions,
                       uint32_t ExtensionsLen);
};

class AVOutputFormatExtensions
    : public WasmEdgeFFmpegAVFormat<AVOutputFormatExtensions> {
public:
  AVOutputFormatExtensions(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t Extensions,
                       uint32_t ExtensionsLen);
};

class AVIOFormatMimeTypeLength
    : public WasmEdgeFFmpegAVFormat<AVIOFormatMimeTypeLength> {
public:
  AVIOFormatMimeTypeLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatMimeType
    : public WasmEdgeFFmpegAVFormat<AVInputFormatMimeType> {
public:
  AVInputFormatMimeType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t MimeTypePtr,
                       uint32_t MimeTypeLen);
};

class AVOutputFormatMimeType
    : public WasmEdgeFFmpegAVFormat<AVOutputFormatMimeType> {
public:
  AVOutputFormatMimeType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t MimeTypePtr,
                       uint32_t MimeTypeLen);
};

class AVOutputFormatFlags : public WasmEdgeFFmpegAVFormat<AVOutputFormatFlags> {
public:
  AVOutputFormatFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId);
};

class AVInputOutputFormatFree
    : public WasmEdgeFFmpegAVFormat<AVInputOutputFormatFree> {
public:
  AVInputOutputFormatFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputOutputId);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
