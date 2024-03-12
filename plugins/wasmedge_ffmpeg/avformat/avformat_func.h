#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVFormatOpenInput : public WasmEdgeFFmpegAVFormat<AVFormatOpenInput> {
public:
  AVFormatOpenInput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxPtr, uint32_t UrlPtr,
                       uint32_t UrlSize, uint32_t AvInputFormatId,
                       uint32_t AvDictionaryId);
};

class AVFormatFindStreamInfo
    : public WasmEdgeFFmpegAVFormat<AVFormatFindStreamInfo> {
public:
  AVFormatFindStreamInfo(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t AvDictionaryId);
};

class AVFormatCloseInput : public WasmEdgeFFmpegAVFormat<AVFormatCloseInput> {
public:
  AVFormatCloseInput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVReadPause : public WasmEdgeFFmpegAVFormat<AVReadPause> {
public:
  AVReadPause(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId);
};

class AVReadPlay : public WasmEdgeFFmpegAVFormat<AVReadPlay> {
public:
  AVReadPlay(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId);
};

class AVFormatSeekFile : public WasmEdgeFFmpegAVFormat<AVFormatSeekFile> {
public:
  AVFormatSeekFile(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx, int64_t MinTs, int64_t Ts,
                       int64_t MaxTs, int32_t Flags);
};

class AVDumpFormat : public WasmEdgeFFmpegAVFormat<AVDumpFormat> {
public:
  AVDumpFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, int32_t Idx, uint32_t UrlPtr,
                       uint32_t UrlSize, int32_t IsOutput);
};

class AVFormatFreeContext : public WasmEdgeFFmpegAVFormat<AVFormatFreeContext> {
public:
  AVFormatFreeContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxPtr);
};

class AVFindBestStream : public WasmEdgeFFmpegAVFormat<AVFindBestStream> {
public:
  AVFindBestStream(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       int32_t MediaTypeId, int32_t WantedStream,
                       int32_t RelatedStream, uint32_t DecoderRetId,
                       int32_t Flags);
};

class AVReadFrame : public WasmEdgeFFmpegAVFormat<AVReadFrame> {
public:
  AVReadFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t PacketId);
};

class AVIOClose : public WasmEdgeFFmpegAVFormat<AVIOClose> {
public:
  AVIOClose(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatNetworkInit : public WasmEdgeFFmpegAVFormat<AVFormatNetworkInit> {
public:
  AVFormatNetworkInit(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatNetworkDeInit
    : public WasmEdgeFFmpegAVFormat<AVFormatNetworkDeInit> {
public:
  AVFormatNetworkDeInit(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatWriteHeader : public WasmEdgeFFmpegAVFormat<AVFormatWriteHeader> {
public:
  AVFormatWriteHeader(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t DictId);
};

class AVFormatWriteTrailer
    : public WasmEdgeFFmpegAVFormat<AVFormatWriteTrailer> {
public:
  AVFormatWriteTrailer(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatAllocOutputContext2
    : public WasmEdgeFFmpegAVFormat<AVFormatAllocOutputContext2> {
public:
  AVFormatAllocOutputContext2(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxPtr, uint32_t AVOutputFormatId,
                       uint32_t FormatNamePtr, uint32_t FormatLen,
                       uint32_t FileNamePtr, uint32_t FileNameLen);
};

class AVIOOpen : public WasmEdgeFFmpegAVFormat<AVIOOpen> {
public:
  AVIOOpen(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t FileNamePtr,
                       uint32_t FileNameLen, int32_t Flags);
};

class AVIOOpen2 : public WasmEdgeFFmpegAVFormat<AVIOOpen2> {
public:
  AVIOOpen2(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxtId, uint32_t UrlPtr,
                       uint32_t UrlLen, int32_t Flags,
                       uint32_t AVIOInterruptCBId, uint32_t AVDictionaryId);
};

class AVFormatVersion : public WasmEdgeFFmpegAVFormat<AVFormatVersion> {
public:
  AVFormatVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVChapterMallocz : public WasmEdgeFFmpegAVFormat<AVChapterMallocz> {
public:
  AVChapterMallocz(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVChapterPtr);
};

class AVChapterDynarrayAdd
    : public WasmEdgeFFmpegAVFormat<AVChapterDynarrayAdd> {
public:
  AVChapterDynarrayAdd(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       int32_t NbChaptersPtr, uint32_t AvChapterId);
};

class AVFreeP : public WasmEdgeFFmpegAVFormat<AVFreeP> {
public:
  AVFreeP(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvChapterId);
};

class AVInterleavedWriteFrame
    : public WasmEdgeFFmpegAVFormat<AVInterleavedWriteFrame> {
public:
  AVInterleavedWriteFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t AvPacketId);
};

class AVWriteFrame : public WasmEdgeFFmpegAVFormat<AVWriteFrame> {
public:
  AVWriteFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t AvPacketId);
};

class AVFormatNewStream : public WasmEdgeFFmpegAVFormat<AVFormatNewStream> {
public:
  AVFormatNewStream(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFormatCtxId, uint32_t AVCodecId);
};

class AVGuessCodec : public WasmEdgeFFmpegAVFormat<AVGuessCodec> {
public:
  AVGuessCodec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AVInputOutputId, uint32_t ShortNamePtr,
                        uint32_t ShortNameLen, uint32_t FileNamePtr,
                        uint32_t FileNameLen, uint32_t MimeTypePtr,
                        uint32_t MimeTypeLen, int32_t MediaTypeId);
};

class AVFormatConfigurationLength
    : public WasmEdgeFFmpegAVFormat<AVFormatConfigurationLength> {
public:
  AVFormatConfigurationLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatConfiguration
    : public WasmEdgeFFmpegAVFormat<AVFormatConfiguration> {
public:
  AVFormatConfiguration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVFormatLicenseLength
    : public WasmEdgeFFmpegAVFormat<AVFormatLicenseLength> {
public:
  AVFormatLicenseLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatLicense : public WasmEdgeFFmpegAVFormat<AVFormatLicense> {
public:
  AVFormatLicense(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
